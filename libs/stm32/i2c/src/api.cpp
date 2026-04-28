#include "api.hpp"

#include <embys/stm32/def.hpp>

namespace Embys::Stm32::I2c
{

// ── local helpers
// ─────────────────────────────────────────────────────────────

static void
busy_wait_us(uint32_t us)
{
  uint32_t start_cyc = DWT->CYCCNT;
  uint32_t wait_cyc = us * (SystemCoreClock / 1'000'000);

#ifdef STM32_SIM
  wait_cyc /= 100;
#endif

  while ((DWT->CYCCNT - start_cyc) < wait_cyc)
    __NOP();
}

static void
restore_timing(I2C_TypeDef *i2c, uint16_t cr2, uint16_t ccr, uint16_t trise)
{
  CLEAR_BIT_V(i2c->CR1, I2C_CR1_PE);
  i2c->CR2 = cr2;
  i2c->CCR = ccr;
  i2c->TRISE = trise;
  SET_BIT_V(i2c->CR1, I2C_CR1_PE);
}

static void
peripheral_reset(I2C_TypeDef *i2c)
{
  stop_condition(i2c);
  busy_wait_us(5u); // tBUF min
  CLEAR_BIT_V(i2c->CR1, I2C_CR1_PE);
  busy_wait_us(1u);
  SET_BIT_V(i2c->CR1, I2C_CR1_PE);
  busy_wait_us(1u);

  ack(i2c);
  pos_disable(i2c);
}

static void
soft_reset(I2C_TypeDef *i2c)
{
  SET_BIT_V(i2c->CR1, I2C_CR1_SWRST);
  busy_wait_us(2u);
  CLEAR_BIT_V(i2c->CR1, I2C_CR1_SWRST);
  busy_wait_us(2u);
  ack(i2c);
  pos_disable(i2c);
}

static void
hard_reset(I2C_TypeDef *i2c)
{
  if (i2c == I2C1)
  {
    SET_BIT_V(RCC->APB1RSTR, RCC_APB1RSTR_I2C1RST);
    busy_wait_us(1u);
    CLEAR_BIT_V(RCC->APB1RSTR, RCC_APB1RSTR_I2C1RST);
  }
  else
  {
    SET_BIT_V(RCC->APB1RSTR, RCC_APB1RSTR_I2C2RST);
    busy_wait_us(1u);
    CLEAR_BIT_V(RCC->APB1RSTR, RCC_APB1RSTR_I2C2RST);
  }
  busy_wait_us(3u);
  ack(i2c);
  pos_disable(i2c);
}

static void
clear_errors(I2C_TypeDef *i2c)
{
  CLEAR_BIT_V(i2c->SR1, err_mask);
  if (addr_latched(i2c))
    clear_addr(i2c);
}

static void
wait_not_busy(I2C_TypeDef *i2c)
{
  uint32_t start_cyc = DWT->CYCCNT;
  uint32_t timeout_cyc = 5000u * (SystemCoreClock / 1'000'000); // 5 ms

  while (is_busy(i2c) && (DWT->CYCCNT - start_cyc) < timeout_cyc)
    __NOP();
}

// ── public functions
// ──────────────────────────────────────────────────────────

int
enable_i2c(I2C_TypeDef *i2c, uint32_t scl_hz)
{
  if (i2c == I2C1)
  {
    SET_BIT_V(RCC->APB1ENR, RCC_APB1ENR_I2C1EN);
    SET_BIT_V(RCC->APB1RSTR, RCC_APB1RSTR_I2C1RST);
    CLEAR_BIT_V(RCC->APB1RSTR, RCC_APB1RSTR_I2C1RST);
  }
  else if (i2c == I2C2)
  {
    SET_BIT_V(RCC->APB1ENR, RCC_APB1ENR_I2C2EN);
    SET_BIT_V(RCC->APB1RSTR, RCC_APB1RSTR_I2C2RST);
    CLEAR_BIT_V(RCC->APB1RSTR, RCC_APB1RSTR_I2C2RST);
  }
  else
  {
    return INVALID_I2C;
  }

  (void)RCC->APB1ENR; // read-back for completion
  __DSB();            // ensure clock stability

  // I2C1 and I2C2 are on APB1 (SystemCoreClock / 2 for 72 MHz config)
  uint32_t pclk_hz = SystemCoreClock / 2u;
  uint32_t pclk_mhz = (pclk_hz / 1'000'000u) & 0x3Fu;

  if (pclk_mhz < 2u)
    return INVALID_PCLK;

  CLEAR_BIT_V(i2c->CR1, I2C_CR1_PE); // disable before config

  // Set FREQ, enable EV and ER interrupts, disable ITBUFEN (enabled on demand)
  i2c->CR2 = (i2c->CR2 & ~0x3Fu) | pclk_mhz | I2C_CR2_ITEVTEN | I2C_CR2_ITERREN;
  CLEAR_BIT_V(i2c->CR2, I2C_CR2_ITBUFEN);

  int error = 0;

  if (scl_hz <= 100000u)
  {
    // Standard mode (FS=0)
    uint32_t ccr = (pclk_hz + (2u * scl_hz - 1u)) / (2u * scl_hz); // ceil
    if (ccr < 4u)
    {
      ccr = 4u;
      error = INVALID_CCR;
    }
    i2c->CCR = ccr;
    i2c->TRISE = pclk_mhz + 1u; // max rise time 1000 ns
  }
  else
  {
    // Fast mode (FS=1, DUTY=0)
    uint32_t ccr = (pclk_hz + (3u * scl_hz - 1u)) / (3u * scl_hz); // ceil
    if (ccr < 1u)
    {
      ccr = 1u;
      error = INVALID_CCR;
    }
    i2c->CCR = I2C_CCR_FS | ccr;
    i2c->TRISE = (pclk_mhz * 300u + 999u) / 1000u + 1u; // max rise 300 ns
  }

  SET_BIT_V(i2c->CR1, I2C_CR1_PE);
  ack(i2c);
  pos_disable(i2c);

  return error;
}

int
disable_i2c(I2C_TypeDef *i2c)
{
  CLEAR_BIT_V(i2c->CR2, I2C_CR2_ITEVTEN | I2C_CR2_ITERREN | I2C_CR2_ITBUFEN);

  if (i2c == I2C1)
    CLEAR_BIT_V(RCC->APB1ENR, RCC_APB1ENR_I2C1EN);
  else if (i2c == I2C2)
    CLEAR_BIT_V(RCC->APB1ENR, RCC_APB1ENR_I2C2EN);
  else
    return INVALID_I2C;

  return 0;
}

int
reset_i2c(I2C_TypeDef *i2c)
{
  if (!is_busy(i2c) && !has_error(i2c) && !addr_latched(i2c))
    return 0;

  uint16_t cr2_save = static_cast<uint16_t>(i2c->CR2);
  uint16_t ccr_save = static_cast<uint16_t>(i2c->CCR);
  uint16_t trise_save = static_cast<uint16_t>(i2c->TRISE);

  if (addr_latched(i2c))
    clear_addr(i2c);

  peripheral_reset(i2c);
  clear_errors(i2c);
  wait_not_busy(i2c);
  if (!is_busy(i2c) && !has_error(i2c))
    return 0;

  soft_reset(i2c);
  restore_timing(i2c, cr2_save, ccr_save, trise_save);
  clear_errors(i2c);
  wait_not_busy(i2c);
  if (!is_busy(i2c) && !has_error(i2c))
    return 0;

  hard_reset(i2c);
  restore_timing(i2c, cr2_save, ccr_save, trise_save);
  clear_errors(i2c);
  wait_not_busy(i2c);
  if (!is_busy(i2c) && !has_error(i2c))
    return 0;

  return BUS_STUCK;
}

}; // namespace Embys::Stm32::I2c
