#ifdef STM32H7xx

#include "../../hal.hpp"

#include <embys/stm32/def.hpp>

#include "../../diag.hpp"
#include "../../stm32xx.hpp"

// H7 GPIO HAL implementation.
// GPIO clocks are on AHB4. EXTI source routing via SYSCFG->EXTICR (APB4).
// Pin configuration uses MODER/OTYPER/OSPEEDR/PUPDR (identical to F4/F7).
// EXTI pending register is PR1 (lines 0-31) instead of PR.

namespace Embys::Stm32::Gpio
{

// ── Internal helpers
// ──────────────────────────────────────────────────────────

static inline uint32_t
speed_bits(PinCfg cfg)
{
  if (has_cfg(cfg, PinCfg::LOW))
    return 0b00U;
  if (has_cfg(cfg, PinCfg::MEDIUM))
    return 0b01U;
  if (has_cfg(cfg, PinCfg::HIGH))
    return 0b10U;
  if (has_cfg(cfg, PinCfg::VHIGH))
    return 0b11U;
  return 0b11U;
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
  if (port == GPIOE)
    return 4;
  if (port == GPIOF)
    return 5;
  if (port == GPIOG)
    return 6;
  if (port == GPIOH)
    return 7;
  if (port == GPIOI)
    return 8;
  if (port == GPIOJ)
    return 9;
  if (port == GPIOK)
    return 10;
  return 0xFF;
}

// ── GPIO port clock (AHB4)
// ────────────────────────────────────────────────────

static uint32_t
gpio_ahb4_en_mask(GPIO_TypeDef *port)
{
  if (port == GPIOA)
    return RCC_AHB4ENR_GPIOAEN;
  if (port == GPIOB)
    return RCC_AHB4ENR_GPIOBEN;
  if (port == GPIOC)
    return RCC_AHB4ENR_GPIOCEN;
  if (port == GPIOD)
    return RCC_AHB4ENR_GPIODEN;
  if (port == GPIOE)
    return RCC_AHB4ENR_GPIOEEN;
  if (port == GPIOF)
    return RCC_AHB4ENR_GPIOFEN;
  if (port == GPIOG)
    return RCC_AHB4ENR_GPIOGEN;
  if (port == GPIOH)
    return RCC_AHB4ENR_GPIOHEN;
  if (port == GPIOI)
    return RCC_AHB4ENR_GPIOIEN;
  if (port == GPIOJ)
    return RCC_AHB4ENR_GPIOJEN;
  if (port == GPIOK)
    return RCC_AHB4ENR_GPIOKEN;
  return 0;
}

static uint32_t
gpio_ahb4_rst_mask(GPIO_TypeDef *port)
{
  if (port == GPIOA)
    return RCC_AHB4RSTR_GPIOARST;
  if (port == GPIOB)
    return RCC_AHB4RSTR_GPIOBRST;
  if (port == GPIOC)
    return RCC_AHB4RSTR_GPIOCRST;
  if (port == GPIOD)
    return RCC_AHB4RSTR_GPIODRST;
  if (port == GPIOE)
    return RCC_AHB4RSTR_GPIOERST;
  if (port == GPIOF)
    return RCC_AHB4RSTR_GPIOFRST;
  if (port == GPIOG)
    return RCC_AHB4RSTR_GPIOGRST;
  if (port == GPIOH)
    return RCC_AHB4RSTR_GPIOHRST;
  if (port == GPIOI)
    return RCC_AHB4RSTR_GPIOIRST;
  if (port == GPIOJ)
    return RCC_AHB4RSTR_GPIOJRST;
  if (port == GPIOK)
    return RCC_AHB4RSTR_GPIOKRST;
  return 0;
}

int
enable_gpio(GPIO_TypeDef *port)
{
  uint32_t en_mask = gpio_ahb4_en_mask(port);

  if (!en_mask)
    return INVALID_PORT;

  if (RCC->AHB4ENR & en_mask)
    return 0;

  uint32_t rst_mask = gpio_ahb4_rst_mask(port);
  SET_BIT_V(RCC->AHB4ENR, en_mask);
  SET_BIT_V(RCC->AHB4RSTR, rst_mask);
  CLEAR_BIT_V(RCC->AHB4RSTR, rst_mask);
  (void)RCC->AHB4ENR;
  __DSB();

  return 0;
}

int
disable_gpio(GPIO_TypeDef *port)
{
  uint32_t en_mask = gpio_ahb4_en_mask(port);

  if (!en_mask)
    return INVALID_PORT;

  CLEAR_BIT_V(RCC->AHB4ENR, en_mask);
  (void)RCC->AHB4ENR;
  __DSB();

  return 0;
}

// ── EXTI source clock (SYSCFG on APB4 on H7) ─────────────────────────────────

int
enable_exti_source_clock()
{
  if (RCC->APB4ENR & RCC_APB4ENR_SYSCFGEN)
    return 0;

  SET_BIT_V(RCC->APB4ENR, RCC_APB4ENR_SYSCFGEN);
  (void)RCC->APB4ENR;
  __DSB();

  return 0;
}

int
disable_exti_source_clock()
{
  CLEAR_BIT_V(RCC->APB4ENR, RCC_APB4ENR_SYSCFGEN);
  (void)RCC->APB4ENR;
  __DSB();

  return 0;
}

// ── Pin configuration (identical to F4/F7)
// ────────────────────────────────────

int
configure_pin(GPIO_TypeDef *port, uint8_t index, PinCfg cfg)
{
  PinCfg effective_cfg = cfg;
  uint8_t af_num = 0xFF;

  if (has_cfg(cfg, PinCfg::I2C))
  {
    effective_cfg = static_cast<PinCfg>(PinCfg::OUT | PinCfg::AF | PinCfg::OD);
    af_num = 4;
  }
  else if (has_cfg(cfg, PinCfg::UART))
  {
    effective_cfg = static_cast<PinCfg>(PinCfg::OUT | PinCfg::AF);
    af_num = 7;
  }
  else if (has_cfg(cfg, PinCfg::SPI))
  {
    effective_cfg = static_cast<PinCfg>(PinCfg::OUT | PinCfg::AF);
    af_num = 5;
  }
  else if (has_cfg(cfg, PinCfg::PWM))
  {
    effective_cfg = static_cast<PinCfg>(PinCfg::OUT | PinCfg::AF);
    af_num = 1;
  }
  else if (has_cfg(cfg, PinCfg::ANALOG))
  {
    effective_cfg = PinCfg::ANALOG;
  }

  uint32_t moder = 0b00;
  if (has_cfg(effective_cfg, PinCfg::ANALOG))
    moder = 0b11;
  else if (has_cfg(effective_cfg, PinCfg::AF))
    moder = 0b10;
  else if (has_cfg(effective_cfg, PinCfg::OUT))
    moder = 0b01;

  MOD_BIT_V(port->MODER, index * 2U, 0b11U, moder);

  if (moder == 0b01 || moder == 0b10)
  {
    uint32_t otyper = has_cfg(effective_cfg, PinCfg::OD) ? 1U : 0U;
    MOD_BIT_V(port->OTYPER, index, 0b1U, otyper);

    MOD_BIT_V(port->OSPEEDR, index * 2U, 0b11U, speed_bits(cfg));

    if (af_num != 0xFF)
      MOD_BIT_V(port->AFR[index / 8U], (index % 8U) * 4U, 0xFU, af_num);
  }

  uint32_t pupdr = 0b00;
  if (has_cfg(effective_cfg, PinCfg::PU))
    pupdr = 0b01;
  else if (has_cfg(effective_cfg, PinCfg::PD))
    pupdr = 0b10;
  MOD_BIT_V(port->PUPDR, index * 2U, 0b11U, pupdr);

  if (!has_any_role(cfg) && has_cfg(cfg, PinCfg::LISTEN))
    TRY(enable_pin_irq(port, index));

  return 0;
}

int
configure_pin_pull_up(GPIO_TypeDef *port, uint8_t index)
{
  MOD_BIT_V(port->PUPDR, index * 2U, 0b11U, 0b01U);
  return 0;
}

int
configure_pin_pull_down(GPIO_TypeDef *port, uint8_t index)
{
  MOD_BIT_V(port->PUPDR, index * 2U, 0b11U, 0b10U);
  return 0;
}

int
reset_pin(GPIO_TypeDef *port, uint8_t index)
{
  disable_pin_irq(port, index);

  MOD_BIT_V(port->MODER, index * 2U, 0b11U, 0b00U);
  MOD_BIT_V(port->OSPEEDR, index * 2U, 0b11U, 0b00U);
  MOD_BIT_V(port->OTYPER, index, 0b1U, 0b0U);
  MOD_BIT_V(port->PUPDR, index * 2U, 0b11U, 0b00U);

  return 0;
}

// ── EXTI interrupt routing (via SYSCFG->EXTICR on H7) ────────────────────────

int
enable_pin_irq(GPIO_TypeDef *port, uint8_t pin_index)
{
  uint8_t port_num = get_port_num(port);

  if (port_num == 0xFF)
    return INVALID_PORT;

  uint8_t exticr_index = pin_index >> 2;
  uint8_t exticr_shift = (pin_index & 0b11U) << 2;
  uint32_t exti_cfg = uint32_t(port_num) << exticr_shift;

  CLEAR_BIT_V(SYSCFG->EXTICR[exticr_index], 0xFU << exticr_shift);
  SET_BIT_V(SYSCFG->EXTICR[exticr_index], exti_cfg);

  if ((SYSCFG->EXTICR[exticr_index] & (0xFU << exticr_shift)) != exti_cfg)
    return EXTI_CONFIG_FAILED;

  uint32_t pin_bit = (1U << pin_index);
  SET_BIT_V(EXTI->IMR1, pin_bit);
  SET_BIT_V(EXTI->RTSR1, pin_bit);
  SET_BIT_V(EXTI->FTSR1, pin_bit);

  return 0;
}

int
disable_pin_irq(GPIO_TypeDef *, uint8_t pin_index)
{
  uint32_t pin_bit = (1U << pin_index);

  CLEAR_BIT_V(EXTI->IMR1, pin_bit);
  CLEAR_BIT_V(EXTI->RTSR1, pin_bit);
  CLEAR_BIT_V(EXTI->FTSR1, pin_bit);
  SET_BIT_V(EXTI->PR1, pin_bit); // Write 1 to clear pending

  uint8_t exticr_index = pin_index >> 2;
  uint8_t exticr_shift = (pin_index & 0b11U) << 2;
  CLEAR_BIT_V(SYSCFG->EXTICR[exticr_index], 0xFU << exticr_shift);

  return 0;
}

bool
exti_get_and_clear_pending(uint8_t pin_index)
{
  uint32_t pin_bit = (1U << pin_index);

  if (!(EXTI->PR1 & pin_bit))
    return false;

  EXTI->PR1 = pin_bit;
  return true;
}

}; // namespace Embys::Stm32::Gpio

#endif // STM32H7xx
