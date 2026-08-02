#ifdef STM32F7xx

#include "../v2/hal.hpp"

#include <embys/stm32/def.hpp>

// F7 I2C HAL implementation (I2C V2 peripheral).
// I2C1–4 are on APB1. PCLK1 is derived from RCC->CFGR PPRE1 [12:10].
// TIMINGR is computed from PCLK1 at run-time using the formula from RM0385.

namespace Embys::Stm32::I2c
{

// ── local helpers
// ─────────────────────────────────────────────────────────────

static void
busy_wait_us(uint32_t us)
{
  uint32_t start_cyc = DWT->CYCCNT;
  uint32_t wait_cyc = us * (SystemCoreClock / 1'000'000u);

  while ((DWT->CYCCNT - start_cyc) < wait_cyc)
    __NOP();
}

// Derive APB1 (PCLK1) from SystemCoreClock and RCC->CFGR PPRE1 [12:10].
// Encoding: 0xx → /1, 100 → /2, 101 → /4, 110 → /8, 111 → /16.
static uint32_t
pclk1_hz()
{
  uint32_t ppre1 = (RCC->CFGR >> 10U) & 0x7U;
  uint32_t div = (ppre1 & 0x4U) ? (1U << ((ppre1 & 0x3U) + 1U)) : 1U;
  return SystemCoreClock / div;
}

// Compute TIMINGR value for the requested SCL frequency.
// Returns packed PRESC[31:28] | SCLDEL[23:20] | SDADEL[19:16] | SCLH[15:8] |
// SCLL[7:0].
//
// Strategy: choose the smallest PRESC (0–15) such that
//   fpresc = pclk_hz / (PRESC+1) ≤ 16 MHz.
// Then derive SCLL and SCLH from RM0385 timing specs:
//   SM (≤100 kHz): t_SCLL ≥ 4700 ns, t_SCLH ≥ 4000 ns, t_SU;DAT ≥ 250 ns
//   FM (≤400 kHz): t_SCLL ≥ 1300 ns, t_SCLH ≥  600 ns, t_SU;DAT ≥ 100 ns
// Each field = ceil(t_ns * fpresc_kHz / 1e6) − 1.
static uint32_t
compute_timingr(uint32_t pclk_hz, uint32_t scl_hz)
{
  uint32_t presc = 0;
  while (presc < 15U && (pclk_hz / (presc + 1U)) > 16'000'000U)
    ++presc;

  uint32_t fpresc_khz = pclk_hz / ((presc + 1U) * 1000U);

  // Helper: cycles = ceil(ns * fpresc_kHz / 1e6) − 1
  // Uses integer arithmetic: (ns * fpresc_kHz + 999999) / 1000000 − 1
  auto timing_field = [&](uint32_t ns) -> uint32_t
  {
    uint32_t c = (ns * fpresc_khz + 999999U) / 1000000U;
    return (c > 0U) ? (c - 1U) : 0U;
  };

  uint32_t scll, sclh, scldel;

  if (scl_hz <= 100000U)
  {
    // Standard mode
    scll = timing_field(4700U);
    sclh = timing_field(4000U);
    scldel = timing_field(250U);
  }
  else
  {
    // Fast mode
    scll = timing_field(1300U);
    sclh = timing_field(600U);
    scldel = timing_field(100U);
  }

  if (scll > 0xFFU)
    scll = 0xFFU;
  if (sclh > 0xFFU)
    sclh = 0xFFU;
  if (scldel > 0xFU)
    scldel = 0xFU;

  return (presc << 28U) | (scldel << 20U) | (sclh << 8U) | scll;
}

static void
wait_not_busy(I2C_TypeDef *i2c)
{
  uint32_t start_cyc = DWT->CYCCNT;
  uint32_t timeout_cyc = 5000U * (SystemCoreClock / 1'000'000U);

  while (is_busy(i2c) && (DWT->CYCCNT - start_cyc) < timeout_cyc)
    __NOP();
}

static void
hard_reset(uint32_t rst_mask)
{
  SET_BIT_V(RCC->APB1RSTR, rst_mask);
  busy_wait_us(1U);
  CLEAR_BIT_V(RCC->APB1RSTR, rst_mask);
  busy_wait_us(3U);
}

// ── public functions
// ──────────────────────────────────────────────────────────

int
enable_i2c(I2C_TypeDef *i2c, uint32_t scl_hz)
{
  uint32_t en_mask = 0;
  uint32_t rst_mask = 0;

  if (i2c == I2C1)
  {
    en_mask = RCC_APB1ENR_I2C1EN;
    rst_mask = RCC_APB1RSTR_I2C1RST;
  }
  else if (i2c == I2C2)
  {
    en_mask = RCC_APB1ENR_I2C2EN;
    rst_mask = RCC_APB1RSTR_I2C2RST;
  }
  else if (i2c == I2C3)
  {
    en_mask = RCC_APB1ENR_I2C3EN;
    rst_mask = RCC_APB1RSTR_I2C3RST;
  }
#ifdef I2C4
  else if (i2c == I2C4)
  {
    en_mask = RCC_APB1ENR_I2C4EN;
    rst_mask = RCC_APB1RSTR_I2C4RST;
  }
#endif
  else
  {
    return INVALID_I2C;
  }

  SET_BIT_V(RCC->APB1ENR, en_mask);
  hard_reset(rst_mask);
  (void)RCC->APB1ENR; // read-back for completion
  __DSB();            // ensure clock stability

  uint32_t pclk_hz = pclk1_hz();
  uint32_t timingr = compute_timingr(pclk_hz, scl_hz);

  CLEAR_BIT_V(i2c->CR1, I2C_CR1_PE); // disable before config

  i2c->TIMINGR = timingr;

  // Enable EV interrupts: TXIS, RXNE, NACKF, STOPF, TC, ERRIE
  SET_BIT_V(i2c->CR1, I2C_CR1_TXIE | I2C_CR1_RXIE | I2C_CR1_NACKIE |
                          I2C_CR1_STOPIE | I2C_CR1_TCIE | I2C_CR1_ERRIE);

  SET_BIT_V(i2c->CR1, I2C_CR1_PE);

  return 0;
}

int
disable_i2c(I2C_TypeDef *i2c)
{
  CLEAR_BIT_V(i2c->CR1, I2C_CR1_TXIE | I2C_CR1_RXIE | I2C_CR1_NACKIE |
                            I2C_CR1_STOPIE | I2C_CR1_TCIE | I2C_CR1_ERRIE);
  CLEAR_BIT_V(i2c->CR1, I2C_CR1_PE);

  if (i2c == I2C1)
    CLEAR_BIT_V(RCC->APB1ENR, RCC_APB1ENR_I2C1EN);
  else if (i2c == I2C2)
    CLEAR_BIT_V(RCC->APB1ENR, RCC_APB1ENR_I2C2EN);
  else if (i2c == I2C3)
    CLEAR_BIT_V(RCC->APB1ENR, RCC_APB1ENR_I2C3EN);
#ifdef I2C4
  else if (i2c == I2C4)
    CLEAR_BIT_V(RCC->APB1ENR, RCC_APB1ENR_I2C4EN);
#endif
  else
    return INVALID_I2C;

  return 0;
}

int
reset_i2c(I2C_TypeDef *i2c)
{
  if (!is_busy(i2c) && !has_error(i2c))
    return 0;

  // PE off/on cycle clears most flags and resets the FSM.
  CLEAR_BIT_V(i2c->CR1, I2C_CR1_PE);
  busy_wait_us(1U);
  SET_BIT_V(i2c->CR1, I2C_CR1_PE);
  busy_wait_us(1U);

  wait_not_busy(i2c);
  if (!is_busy(i2c) && !has_error(i2c))
    return 0;

  // APB hard reset as last resort.
  uint32_t rst_mask = 0;
  if (i2c == I2C1)
    rst_mask = RCC_APB1RSTR_I2C1RST;
  else if (i2c == I2C2)
    rst_mask = RCC_APB1RSTR_I2C2RST;
  else if (i2c == I2C3)
    rst_mask = RCC_APB1RSTR_I2C3RST;
#ifdef I2C4
  else if (i2c == I2C4)
    rst_mask = RCC_APB1RSTR_I2C4RST;
#endif
  else
    return BUS_STUCK;

  hard_reset(rst_mask);
  wait_not_busy(i2c);
  if (!is_busy(i2c) && !has_error(i2c))
    return 0;

  return BUS_STUCK;
}

}; // namespace Embys::Stm32::I2c

#endif // STM32F7xx
