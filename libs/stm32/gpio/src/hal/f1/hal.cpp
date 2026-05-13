#include "../../hal.hpp"

#include <embys/stm32/def.hpp>

#include "../../diag.hpp"
#include "../../stm32xx.hpp"

// F1-specific GPIO HAL implementation.
// GPIO clocks are on APB2. EXTI source routing is via AFIO->EXTICR.
// Pin configuration uses the CRL/CRH 4-bit nibble format (MODE[1:0] |
// CNF[1:0]).

namespace Embys::Stm32::Gpio
{

// ── Internal helpers
// ──────────────────────────────────────────────────────────

static constexpr volatile uint32_t *
pin_cr(GPIO_TypeDef *port, uint8_t index)
{
  return (index < 8) ? &(port->CRL) : &(port->CRH);
}

static uint8_t
get_port_num(GPIO_TypeDef *port)
{
  if (port == GPIOA)
    return 0;
  if (port == GPIOB)
    return 1;
  if (port == GPIOC)
    return 2;
  if (port == GPIOD)
    return 3;
  return 0xFF;
}

// ── GPIO port clock
// ───────────────────────────────────────────────────────────

int
enable_gpio(GPIO_TypeDef *port)
{
  uint32_t en_mask = 0;
  uint32_t rst_mask = 0;

  if (port == GPIOA)
  {
    en_mask = RCC_APB2ENR_IOPAEN;
    rst_mask = RCC_APB2RSTR_IOPARST;
  }
  else if (port == GPIOB)
  {
    en_mask = RCC_APB2ENR_IOPBEN;
    rst_mask = RCC_APB2RSTR_IOPBRST;
  }
  else if (port == GPIOC)
  {
    en_mask = RCC_APB2ENR_IOPCEN;
    rst_mask = RCC_APB2RSTR_IOPCRST;
  }
  else if (port == GPIOD)
  {
    en_mask = RCC_APB2ENR_IOPDEN;
    rst_mask = RCC_APB2RSTR_IOPDRST;
  }
  else
  {
    return INVALID_PORT;
  }

  if (RCC->APB2ENR & en_mask)
    return 0; // Already enabled

  SET_BIT_V(RCC->APB2ENR, en_mask);
  SET_BIT_V(RCC->APB2RSTR, rst_mask);
  CLEAR_BIT_V(RCC->APB2RSTR, rst_mask);
  (void)RCC->APB2ENR; // Read back for clock stability
  __DSB();

  return 0;
}

int
disable_gpio(GPIO_TypeDef *port)
{
  if (port == GPIOA)
    CLEAR_BIT_V(RCC->APB2ENR, RCC_APB2ENR_IOPAEN);
  else if (port == GPIOB)
    CLEAR_BIT_V(RCC->APB2ENR, RCC_APB2ENR_IOPBEN);
  else if (port == GPIOC)
    CLEAR_BIT_V(RCC->APB2ENR, RCC_APB2ENR_IOPCEN);
  else if (port == GPIOD)
    CLEAR_BIT_V(RCC->APB2ENR, RCC_APB2ENR_IOPDEN);
  else
    return INVALID_PORT;

  (void)RCC->APB2ENR;
  __DSB();

  return 0;
}

// ── EXTI source clock (AFIO on F1) ───────────────────────────────────────────

int
enable_exti_source_clock()
{
  if (RCC->APB2ENR & RCC_APB2ENR_AFIOEN)
    return 0; // Already enabled

  SET_BIT_V(RCC->APB2ENR, RCC_APB2ENR_AFIOEN);
  SET_BIT_V(RCC->APB2RSTR, RCC_APB2RSTR_AFIORST);
  CLEAR_BIT_V(RCC->APB2RSTR, RCC_APB2RSTR_AFIORST);
  (void)RCC->APB2ENR;
  __DSB();

  return 0;
}

int
disable_exti_source_clock()
{
  CLEAR_BIT_V(RCC->APB2ENR, RCC_APB2ENR_AFIOEN);
  (void)RCC->APB2ENR;
  __DSB();

  return 0;
}

// ── Pin configuration
// ─────────────────────────────────────────────────────────

static uint32_t
mode_bits(Mode mode)
{
  switch (mode)
  {
    case Mode::IN:
      return 0b00;
    case Mode::OUT_10:
      return 0b01;
    case Mode::OUT_2:
      return 0b10;
    case Mode::OUT_50:
      return 0b11;
    default:
      return 0b00;
  }
}

static uint32_t
cnf_bits(Cnf cnf)
{
  switch (cnf)
  {
    case Cnf::IN_AN:
      return 0b00;
    case Cnf::IN_FL:
      return 0b01;
    case Cnf::IN_PU:
      return 0b10;
    case Cnf::OUT_PP:
      return 0b00;
    case Cnf::OUT_OD:
      return 0b01;
    case Cnf::OUT_PP_AF:
      return 0b10;
    case Cnf::OUT_OD_AF:
      return 0b11;
    default:
      return 0b00;
  }
}

int
configure_pin(GPIO_TypeDef *port, uint8_t index, Mode mode, Cnf cnf)
{
  // Pack mode and cnf into the F1 4-bit nibble: [CNF1:CNF0:MODE1:MODE0]
  uint32_t nibble = (cnf_bits(cnf) << 2) | mode_bits(mode);

  volatile uint32_t *cr = pin_cr(port, index);
  uint8_t shift = (index & 0x7U) << 2;

  CLEAR_BIT_V(*cr, 0xFU << shift);
  SET_BIT_V(*cr, nibble << shift);

  if (((*cr >> shift) & 0xFU) != nibble)
    return PIN_CONFIG_FAILED;

  return 0;
}

int
configure_pin_pull_up(GPIO_TypeDef *port, uint8_t index)
{
  // On F1, pull-up is selected by writing 1 to the ODR bit when CNF is IN_PU.
  SET_BIT_V(port->ODR, (1U << index));
  if ((port->ODR & (1U << index)) == 0)
    return PIN_PULLUP_CONFIG_FAILED;
  return 0;
}

int
configure_pin_pull_down(GPIO_TypeDef *port, uint8_t index)
{
  // On F1, pull-down is selected by writing 0 to the ODR bit when CNF is IN_PU.
  CLEAR_BIT_V(port->ODR, (1U << index));
  if ((port->ODR & (1U << index)) != 0)
    return PIN_PULLDOWN_CONFIG_FAILED;
  return 0;
}

int
reset_pin(GPIO_TypeDef *port, uint8_t index)
{
  volatile uint32_t *cr = pin_cr(port, index);
  uint8_t shift = (index & 0x7U) << 2;

  // Input floating: CNF=01 MODE=00 → nibble = 0b0100
  uint32_t input_floating = 0b0100U;
  CLEAR_BIT_V(*cr, 0xFU << shift);
  SET_BIT_V(*cr, input_floating << shift);
  CLEAR_BIT_V(port->ODR, 1U << index);

  return 0;
}

// ── EXTI interrupt routing (via AFIO->EXTICR) ────────────────────────────────

int
enable_pin_irq(GPIO_TypeDef *port, uint8_t pin_index)
{
  uint8_t port_num = get_port_num(port);

  if (port_num == 0xFF)
    return INVALID_PORT;

  uint8_t exticr_index = pin_index >> 2;
  uint8_t exticr_shift = (pin_index & 0b11U) << 2;
  uint32_t exti_cfg = uint32_t(port_num) << exticr_shift;

  CLEAR_BIT_V(AFIO->EXTICR[exticr_index], 0xFU << exticr_shift);
  SET_BIT_V(AFIO->EXTICR[exticr_index], exti_cfg);

  if ((AFIO->EXTICR[exticr_index] & (0xFU << exticr_shift)) != exti_cfg)
    return EXTI_CONFIG_FAILED;

  uint32_t pin_bit = (1U << pin_index);
  SET_BIT_V(EXTI->IMR, pin_bit);
  SET_BIT_V(EXTI->RTSR, pin_bit);
  SET_BIT_V(EXTI->FTSR, pin_bit);

  return 0;
}

int
disable_pin_irq(GPIO_TypeDef *, uint8_t pin_index)
{
  uint32_t pin_bit = (1U << pin_index);

  CLEAR_BIT_V(EXTI->IMR, pin_bit);
  CLEAR_BIT_V(EXTI->RTSR, pin_bit);
  CLEAR_BIT_V(EXTI->FTSR, pin_bit);
  SET_BIT_V(EXTI->PR, pin_bit); // Clear pending

  uint8_t exticr_index = pin_index >> 2;
  uint8_t exticr_shift = (pin_index & 0b11U) << 2;
  CLEAR_BIT_V(AFIO->EXTICR[exticr_index], 0xFU << exticr_shift);

  return 0;
}

bool
exti_get_and_clear_pending(uint8_t pin_index)
{
  uint32_t pin_bit = (1U << pin_index);

  if (!(EXTI->PR & pin_bit))
    return false;

  EXTI->PR = pin_bit; // Write 1 to clear
  return true;
}

}; // namespace Embys::Stm32::Gpio
