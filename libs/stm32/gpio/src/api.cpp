#include "api.hpp"

#include <embys/stm32/def.hpp>

#include "diag.hpp"

namespace Embys::Stm32::Gpio
{

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
  else
  {
    return INVALID_PORT; // Invalid port
  }

  if (RCC->APB2ENR & en_mask)
  {
    // Already enabled
    return 0;
  }

  SET_BIT_V(RCC->APB2ENR, en_mask);
  SET_BIT_V(RCC->APB2RSTR, rst_mask);
  CLEAR_BIT_V(RCC->APB2RSTR, rst_mask);

  (void)RCC->APB2ENR; // Read back for completion
  __DSB();            // Ensure clock stability

  return 0;
}

int
disable_gpio(GPIO_TypeDef *port)
{
  if (port == GPIOA)
  {
    CLEAR_BIT_V(RCC->APB2ENR, RCC_APB2ENR_IOPAEN);
  }
  else if (port == GPIOB)
  {
    CLEAR_BIT_V(RCC->APB2ENR, RCC_APB2ENR_IOPBEN);
  }
  else if (port == GPIOC)
  {
    CLEAR_BIT_V(RCC->APB2ENR, RCC_APB2ENR_IOPCEN);
  }
  else
  {
    return INVALID_PORT; // Invalid port
  }

  (void)RCC->APB2ENR; // Read back for completion
  __DSB();            // Ensure clock stability

  return 0;
}

int
enable_afio()
{
  if (RCC->APB2ENR & RCC_APB2ENR_AFIOEN)
  {
    // Already enabled
    return 0;
  }

  SET_BIT_V(RCC->APB2ENR, RCC_APB2ENR_AFIOEN);
  SET_BIT_V(RCC->APB2RSTR, RCC_APB2RSTR_AFIORST);
  CLEAR_BIT_V(RCC->APB2RSTR, RCC_APB2RSTR_AFIORST);
  (void)RCC->APB2ENR; // Read back for completion
  __DSB();            // Ensure clock stability

  return 0;
}

int
disable_afio()
{
  CLEAR_BIT_V(RCC->APB2ENR, RCC_APB2ENR_AFIOEN);
  (void)RCC->APB2ENR; // Read back for completion
  __DSB();            // Ensure clock stability

  return 0;
}

int
configure_pin(GPIO_TypeDef *port, uint8_t index, uint32_t gpio_cfg)
{
  auto cr = pin_cr(port, index);
  uint8_t shift = (index & 0x7) << 2;

  CLEAR_BIT_V(*cr, 0xF << shift);
  SET_BIT_V(*cr, gpio_cfg << shift);

  if (((*cr >> shift) & 0xF) != (gpio_cfg & 0xF))
    return PIN_CONFIG_FAILED;

  return 0;
}

int
configure_pin_pull_up(GPIO_TypeDef *port, uint8_t index)
{
  SET_BIT_V(port->ODR, (1 << index));
  if ((port->ODR & (1 << index)) == 0)
    return PIN_PULLUP_CONFIG_FAILED;
  return 0;
}

int
configure_pin_pull_down(GPIO_TypeDef *port, uint8_t index)
{
  CLEAR_BIT_V(port->ODR, (1 << index));
  if ((port->ODR & (1 << index)) != 0)
    return PIN_PULLDOWN_CONFIG_FAILED;
  return 0;
}

int
reset_pin(GPIO_TypeDef *port, uint8_t index)
{
  auto cr = pin_cr(port, index);
  uint8_t shift = (index & 0x7) << 2;

  uint32_t input_floating_cfg = 0b0100; // CNF=0b01 MODE=0b00
  CLEAR_BIT_V(*cr, 0xF << shift);
  SET_BIT_V(*cr, input_floating_cfg << shift);
  CLEAR_BIT_V(port->ODR, 1 << index);

  return 0;
}

static uint8_t
get_port_num(GPIO_TypeDef *port)
{
  if (port == GPIOA)
    return 0;
  else if (port == GPIOB)
    return 1;
  else if (port == GPIOC)
    return 2;
  else
    return 0xFF; // Invalid port
}

int
enable_pin_irq(GPIO_TypeDef *port, uint8_t pin_index)
{
  uint8_t exticr_index = pin_index >> 2;
  uint8_t exticr_shift = (pin_index & 0b11) << 2;
  uint8_t port_num = get_port_num(port);

  if (port_num == 0xFF)
    return INVALID_PORT; // Invalid port

  uint32_t exti_cfg = port_num << exticr_shift;

  CLEAR_BIT_V(AFIO->EXTICR[exticr_index], 0xF << exticr_shift);
  SET_BIT_V(AFIO->EXTICR[exticr_index], exti_cfg);

  if ((AFIO->EXTICR[exticr_index] & exti_cfg) != exti_cfg)
    return EXTI_CONFIG_FAILED; // Exit line configuration failed

  uint32_t pin_bit = (1 << pin_index);

  // Enable interrupt mask and edge triggers
  SET_BIT_V(EXTI->IMR, pin_bit);
  SET_BIT_V(EXTI->RTSR, pin_bit);
  SET_BIT_V(EXTI->FTSR, pin_bit);

  return 0;
}

int
disable_pin_irq(GPIO_TypeDef *, uint8_t pin_index)
{
  uint32_t pin_bit = (1 << pin_index);

  // Disable EXTI configuration
  CLEAR_BIT_V(EXTI->IMR, pin_bit);
  CLEAR_BIT_V(EXTI->RTSR, pin_bit);
  CLEAR_BIT_V(EXTI->FTSR, pin_bit);
  SET_BIT_V(EXTI->PR, pin_bit); // Clear pending

  // Clear EXTICR routing
  uint8_t exticr_index = pin_index >> 2;
  uint8_t exticr_shift = (pin_index & 0b11) << 2;
  CLEAR_BIT_V(AFIO->EXTICR[exticr_index], 0xF << exticr_shift);

  return 0;
}

int
read_pin(GPIO_TypeDef *port, uint8_t index, uint8_t *value)
{
  *value = (port->IDR & (1 << index)) ? 1 : 0;
  return 0;
}

int
write_pin(GPIO_TypeDef *port, uint8_t index, uint8_t value)
{
  if (value)
    port->BSRR = (1 << index);
  else
    port->BSRR = (1 << (index + 16));

  return 0;
}
}; // namespace Embys::Stm32::Gpio
