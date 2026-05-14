#ifdef STM32F4xx

#include "../../hal.hpp"

#include <embys/stm32/def.hpp>

#include "../../diag.hpp"
#include "../../stm32xx.hpp"

// F4/F7 GPIO HAL implementation.
// GPIO clocks are on AHB1. EXTI source routing via SYSCFG->EXTICR.
// Pin configuration uses MODER/OTYPER/OSPEEDR/PUPDR registers.
// F7 shares an identical GPIO register layout with F4.

namespace Embys::Stm32::Gpio
{

// ── Internal helpers
// ──────────────────────────────────────────────────────────

// Modify a multi-bit field inside a 32-bit register.
// field_shift: bit position of the LSB; field_width: number of bits.
static inline void
modify_field(volatile uint32_t &reg, uint8_t field_shift, uint32_t field_mask,
             uint32_t value)
{
  reg = (reg & ~(field_mask << field_shift)) |
        ((value & field_mask) << field_shift);
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
  return 0xFF;
}

// ── GPIO port clock (AHB1)
// ────────────────────────────────────────────────────

static uint32_t
gpio_ahb1_en_mask(GPIO_TypeDef *port)
{
  if (port == GPIOA)
    return RCC_AHB1ENR_GPIOAEN;
  if (port == GPIOB)
    return RCC_AHB1ENR_GPIOBEN;
  if (port == GPIOC)
    return RCC_AHB1ENR_GPIOCEN;
  if (port == GPIOD)
    return RCC_AHB1ENR_GPIODEN;
  if (port == GPIOE)
    return RCC_AHB1ENR_GPIOEEN;
  if (port == GPIOF)
    return RCC_AHB1ENR_GPIOFEN;
  if (port == GPIOG)
    return RCC_AHB1ENR_GPIOGEN;
  if (port == GPIOH)
    return RCC_AHB1ENR_GPIOHEN;
  return 0;
}

static uint32_t
gpio_ahb1_rst_mask(GPIO_TypeDef *port)
{
  if (port == GPIOA)
    return RCC_AHB1RSTR_GPIOARST;
  if (port == GPIOB)
    return RCC_AHB1RSTR_GPIOBRST;
  if (port == GPIOC)
    return RCC_AHB1RSTR_GPIOCRST;
  if (port == GPIOD)
    return RCC_AHB1RSTR_GPIODRST;
  if (port == GPIOE)
    return RCC_AHB1RSTR_GPIOERST;
  if (port == GPIOF)
    return RCC_AHB1RSTR_GPIOFRST;
  if (port == GPIOG)
    return RCC_AHB1RSTR_GPIOGRST;
  if (port == GPIOH)
    return RCC_AHB1RSTR_GPIOHRST;
  return 0;
}

int
enable_gpio(GPIO_TypeDef *port)
{
  uint32_t en_mask = gpio_ahb1_en_mask(port);

  if (!en_mask)
    return INVALID_PORT;

  if (RCC->AHB1ENR & en_mask)
    return 0; // Already enabled

  uint32_t rst_mask = gpio_ahb1_rst_mask(port);
  SET_BIT_V(RCC->AHB1ENR, en_mask);
  SET_BIT_V(RCC->AHB1RSTR, rst_mask);
  CLEAR_BIT_V(RCC->AHB1RSTR, rst_mask);
  (void)RCC->AHB1ENR;
  __DSB();

  return 0;
}

int
disable_gpio(GPIO_TypeDef *port)
{
  uint32_t en_mask = gpio_ahb1_en_mask(port);

  if (!en_mask)
    return INVALID_PORT;

  CLEAR_BIT_V(RCC->AHB1ENR, en_mask);
  (void)RCC->AHB1ENR;
  __DSB();

  return 0;
}

// ── EXTI source clock (SYSCFG on F4/F7) ──────────────────────────────────────

int
enable_exti_source_clock()
{
  if (RCC->APB2ENR & RCC_APB2ENR_SYSCFGEN)
    return 0;

  SET_BIT_V(RCC->APB2ENR, RCC_APB2ENR_SYSCFGEN);
  (void)RCC->APB2ENR;
  __DSB();

  return 0;
}

int
disable_exti_source_clock()
{
  CLEAR_BIT_V(RCC->APB2ENR, RCC_APB2ENR_SYSCFGEN);
  (void)RCC->APB2ENR;
  __DSB();

  return 0;
}

// ── Pin configuration
// ─────────────────────────────────────────────────────────

int
configure_pin(GPIO_TypeDef *port, uint8_t index, Mode mode, Cnf cnf)
{
  // MODER: 00=input, 01=output, 10=AF, 11=analog
  uint32_t moder = 0;
  switch (cnf)
  {
    case Cnf::IN_AN:
      moder = 0b11;
      break;
    case Cnf::IN_FL:
    case Cnf::IN_PU:
      moder = 0b00;
      break;
    case Cnf::OUT_PP:
    case Cnf::OUT_OD:
      moder = 0b01;
      break;
    case Cnf::OUT_PP_AF:
    case Cnf::OUT_OD_AF:
      moder = 0b10;
      break;
    default:
      moder = 0b00;
      break;
  }
  modify_field(port->MODER, index * 2U, 0b11U, moder);

  // OTYPER: 0=push-pull, 1=open-drain
  if (moder == 0b01 || moder == 0b10) // output or AF
  {
    uint32_t otyper = (cnf == Cnf::OUT_OD || cnf == Cnf::OUT_OD_AF) ? 1U : 0U;
    modify_field(port->OTYPER, index, 0b1U, otyper);

    // OSPEEDR: 00=low, 01=medium, 10=fast, 11=high
    uint32_t ospeedr = 0b10; // fast default (for AF)
    switch (mode)
    {
      case Mode::OUT_2:
        ospeedr = 0b00;
        break;
      case Mode::OUT_10:
        ospeedr = 0b01;
        break;
      case Mode::OUT_50:
        ospeedr = 0b11;
        break;
      default:
        ospeedr = 0b10;
        break;
    }
    modify_field(port->OSPEEDR, index * 2U, 0b11U, ospeedr);
  }

  return 0;
}

int
configure_pin_pull_up(GPIO_TypeDef *port, uint8_t index)
{
  // PUPDR: 01 = pull-up
  modify_field(port->PUPDR, index * 2U, 0b11U, 0b01U);
  return 0;
}

int
configure_pin_pull_down(GPIO_TypeDef *port, uint8_t index)
{
  // PUPDR: 10 = pull-down
  modify_field(port->PUPDR, index * 2U, 0b11U, 0b10U);
  return 0;
}

int
reset_pin(GPIO_TypeDef *port, uint8_t index)
{
  // Input floating: MODER=00, OSPEEDR=00, OTYPER=0, PUPDR=00
  modify_field(port->MODER, index * 2U, 0b11U, 0b00U);
  modify_field(port->OSPEEDR, index * 2U, 0b11U, 0b00U);
  modify_field(port->OTYPER, index, 0b1U, 0b0U);
  modify_field(port->PUPDR, index * 2U, 0b11U, 0b00U);

  return 0;
}

// ── EXTI interrupt routing (via SYSCFG->EXTICR)
// ───────────────────────────────

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
  CLEAR_BIT_V(SYSCFG->EXTICR[exticr_index], 0xFU << exticr_shift);

  return 0;
}

bool
exti_get_and_clear_pending(uint8_t pin_index)
{
  uint32_t pin_bit = (1U << pin_index);

  if (!(EXTI->PR & pin_bit))
    return false;

  EXTI->PR = pin_bit;
  return true;
}

}; // namespace Embys::Stm32::Gpio

#endif // STM32F4xx
