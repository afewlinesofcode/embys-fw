#ifdef STM32F1xx

#include "../../hal.hpp"

#include <embys/stm32/def.hpp>

#include "../../def.hpp"
#include "../../stm32xx.hpp"
#include "utility.hpp"

// F1-specific GPIO HAL implementation.
// GPIO clocks are on APB2. EXTI source routing is via AFIO->EXTICR.
// Pin configuration uses the CRL/CRH 4-bit nibble format (MODE[1:0] |
// CNF[1:0]).

namespace Embys::Stm32::Gpio
{

int
configure_pin_pull_up(GPIO_TypeDef *port, uint8_t index);
int
configure_pin_pull_down(GPIO_TypeDef *port, uint8_t index);
int
configure_pin_irq(GPIO_TypeDef *port, uint8_t index);
int
reset_pin_irq(GPIO_TypeDef *, uint8_t pin_index);

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

int
enable_exti()
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
disable_exti()
{
  CLEAR_BIT_V(RCC->APB2ENR, RCC_APB2ENR_AFIOEN);
  (void)RCC->APB2ENR;
  __DSB();

  return 0;
}

int
configure_pin(GPIO_TypeDef *port, uint8_t index, PinCfg cfg,
              [[maybe_unused]] const PwmBinding *pwm)
{
  PinCfg effective_cfg = get_effective_pin_cfg(cfg);

  TRY(enable_gpio(port));

  uint32_t mode = 0b00U;
  uint32_t cnf = 0b01U; // default input floating

  if (has_cfg(effective_cfg, PinCfg::ANALOG))
  {
    mode = 0b00U;
    cnf = 0b00U;
  }
  else if (has_cfg(effective_cfg, PinCfg::AF))
  {
    mode = get_output_mode_bits(effective_cfg);
    cnf = has_cfg(effective_cfg, PinCfg::OD) ? 0b11U : 0b10U;
  }
  else if (has_cfg(effective_cfg, PinCfg::OUT))
  {
    mode = get_output_mode_bits(effective_cfg);
    cnf = has_cfg(effective_cfg, PinCfg::OD) ? 0b01U : 0b00U;
  }
  else if (has_cfg(effective_cfg, PinCfg::IN))
  {
    mode = 0b00U;
    cnf = has_cfg(effective_cfg, PinCfg::PU | PinCfg::PD) ? 0b10U : 0b01U;
  }

  uint32_t nibble = (cnf << 2) | mode;

  volatile uint32_t *cr = pin_cr(port, index);
  uint8_t shift = (index & 0x7U) << 2;

  CLEAR_BIT_V(*cr, 0xFU << shift);
  SET_BIT_V(*cr, nibble << shift);

  if (((*cr >> shift) & 0xFU) != nibble)
    return PIN_CONFIG_FAILED;

  if (has_cfg(effective_cfg, PinCfg::PU))
    TRY(configure_pin_pull_up(port, index));

  if (has_cfg(effective_cfg, PinCfg::PD))
    TRY(configure_pin_pull_down(port, index));

  if (has_cfg(effective_cfg, PinCfg::IN) && has_cfg(cfg, PinCfg::LISTEN))
    TRY(configure_pin_irq(port, index));

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
configure_pin_irq(GPIO_TypeDef *port, uint8_t pin_index)
{
  TRY(enable_exti());

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

// Reset previously configured pin to a safe state (input floating) and disable
// IRQ if applicable.
int
reset_pin(GPIO_TypeDef *port, uint8_t index, PinCfg cfg,
          [[maybe_unused]] const PwmBinding *pwm)
{
  PinCfg effective_cfg = get_effective_pin_cfg(cfg);

  if (has_cfg(effective_cfg, PinCfg::IN) && has_cfg(cfg, PinCfg::LISTEN))
    reset_pin_irq(port, index);

  volatile uint32_t *cr = pin_cr(port, index);
  uint8_t shift = (index & 0x7U) << 2;

  // Input floating: CNF=01 MODE=00 → nibble = 0b0100
  uint32_t input_floating = 0b0100U;
  CLEAR_BIT_V(*cr, 0xFU << shift);
  SET_BIT_V(*cr, input_floating << shift);
  CLEAR_BIT_V(port->ODR, 1U << index);

  return 0;
}

int
reset_pin_irq(GPIO_TypeDef *, uint8_t pin_index)
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
is_valid_pwm_binding(const PwmBinding *binding)
{
  if (!binding || !binding->timer || binding->channel == 0)
  {
    return false;
  }

  auto timer = binding->timer->get_peripheral();
  auto channel = binding->channel;

  if (timer == TIM1)
    return channel >= 1U && channel <= 4U;
  else if (timer == TIM2)
    return channel >= 1U && channel <= 4U;
  else if (timer == TIM3)
    return channel >= 1U && channel <= 4U;
  else if (timer == TIM4)
    return channel >= 1U && channel <= 4U;
#ifdef TIM5
  else if (timer == TIM5)
    return channel >= 1U && channel <= 4U;
#endif
  return false;
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

#endif // STM32F1xx
