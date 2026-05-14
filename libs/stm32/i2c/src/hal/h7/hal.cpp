#ifdef STM32H7xx

#include "../v2/hal.hpp"

#include <embys/stm32/def.hpp>

// H7 I2C HAL implementation (I2C V2 peripheral).
// I2C1–3 are on APB1L.  I2C4 is on APB4 and is not supported here.
// PCLK1 is derived from the prescaler chain:
//   SystemCoreClock (= cpu_ck = SYSCLK / D1CPRE)
//   → D1CFGR HPRE  [3:0]  (AHB clock)
//   → D2CFGR D2PPRE1 [6:4] (APB1 clock)
// TIMINGR is computed identically to the F7 HAL.

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

// AHB prescaler encoding (4-bit, RCC_D1CFGR HPRE):
//   0xxx → /1,  1000 → /2,  1001 → /4,  1010 → /8,  1011 → /16
//   1100 → /64, 1101 → /128, 1110 → /256, 1111 → /512
static uint32_t
ahb_div_from_hpre(uint32_t hpre)
{
  static const uint32_t table[16] = {1U, 1U, 1U, 1U,  1U,  1U,   1U,   1U,
                                     2U, 4U, 8U, 16U, 64U, 128U, 256U, 512U};
  return table[hpre & 0xFU];
}

// Derive APB1 (PCLK1) from the full prescaler chain.
// SystemCoreClock is the CPU clock (cpu_ck = SYSCLK / D1CPRE).
static uint32_t
pclk1_hz()
{
  uint32_t hpre = (RCC->D1CFGR >> 0U) & 0xFU;
  uint32_t ppre1 = (RCC->D2CFGR >> 4U) & 0x7U;

  uint32_t ahb_div = ahb_div_from_hpre(hpre);
  uint32_t apb1_div = (ppre1 & 0x4U) ? (1U << ((ppre1 & 0x3U) + 1U)) : 1U;

  return SystemCoreClock / ahb_div / apb1_div;
}

static uint32_t
compute_timingr(uint32_t pclk_hz, uint32_t scl_hz)
{
  uint32_t presc = 0;
  while (presc < 15U && (pclk_hz / (presc + 1U)) > 16'000'000U)
    ++presc;

  uint32_t fpresc_khz = pclk_hz / ((presc + 1U) * 1000U);

  auto timing_field = [&](uint32_t ns) -> uint32_t
  {
    uint32_t c = (ns * fpresc_khz + 999999U) / 1000000U;
    return (c > 0U) ? (c - 1U) : 0U;
  };

  uint32_t scll, sclh, scldel;

  if (scl_hz <= 100000U)
  {
    scll = timing_field(4700U);
    sclh = timing_field(4000U);
    scldel = timing_field(250U);
  }
  else
  {
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
  SET_BIT_V(RCC->APB1LRSTR, rst_mask);
  busy_wait_us(1U);
  CLEAR_BIT_V(RCC->APB1LRSTR, rst_mask);
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
    en_mask = RCC_APB1LENR_I2C1EN;
    rst_mask = RCC_APB1LRSTR_I2C1RST;
  }
  else if (i2c == I2C2)
  {
    en_mask = RCC_APB1LENR_I2C2EN;
    rst_mask = RCC_APB1LRSTR_I2C2RST;
  }
  else if (i2c == I2C3)
  {
    en_mask = RCC_APB1LENR_I2C3EN;
    rst_mask = RCC_APB1LRSTR_I2C3RST;
  }
  else
  {
    return INVALID_I2C;
  }

  SET_BIT_V(RCC->APB1LENR, en_mask);
  hard_reset(rst_mask);
  (void)RCC->APB1LENR; // read-back for completion
  __DSB();             // ensure clock stability

  uint32_t pclk_hz = pclk1_hz();
  uint32_t timingr = compute_timingr(pclk_hz, scl_hz);

  CLEAR_BIT_V(i2c->CR1, I2C_CR1_PE);

  i2c->TIMINGR = timingr;

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
    CLEAR_BIT_V(RCC->APB1LENR, RCC_APB1LENR_I2C1EN);
  else if (i2c == I2C2)
    CLEAR_BIT_V(RCC->APB1LENR, RCC_APB1LENR_I2C2EN);
  else if (i2c == I2C3)
    CLEAR_BIT_V(RCC->APB1LENR, RCC_APB1LENR_I2C3EN);
  else
    return INVALID_I2C;

  return 0;
}

int
reset_i2c(I2C_TypeDef *i2c)
{
  if (!is_busy(i2c) && !has_error(i2c))
    return 0;

  CLEAR_BIT_V(i2c->CR1, I2C_CR1_PE);
  busy_wait_us(1U);
  SET_BIT_V(i2c->CR1, I2C_CR1_PE);
  busy_wait_us(1U);

  wait_not_busy(i2c);
  if (!is_busy(i2c) && !has_error(i2c))
    return 0;

  uint32_t rst_mask = 0;
  if (i2c == I2C1)
    rst_mask = RCC_APB1LRSTR_I2C1RST;
  else if (i2c == I2C2)
    rst_mask = RCC_APB1LRSTR_I2C2RST;
  else if (i2c == I2C3)
    rst_mask = RCC_APB1LRSTR_I2C3RST;
  else
    return BUS_STUCK;

  hard_reset(rst_mask);
  wait_not_busy(i2c);
  if (!is_busy(i2c) && !has_error(i2c))
    return 0;

  return BUS_STUCK;
}

}; // namespace Embys::Stm32::I2c

#endif // STM32H7xx
