#ifdef STM32F7xx

#include "../../hal.hpp"

#include <embys/stm32/def.hpp>

#include "../../def.hpp"
#include "../../stm32xx.hpp"

// F4/F7 GPIO HAL implementation.
// GPIO clocks are on AHB1. EXTI source routing via SYSCFG->EXTICR.
// Pin configuration uses MODER/OTYPER/OSPEEDR/PUPDR registers.
// F7 shares an identical GPIO register layout with F4.

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

static bool
is_pin(GPIO_TypeDef *port, uint8_t index, GPIO_TypeDef *expected_port,
       uint8_t expected_index)
{
  return port == expected_port && index == expected_index;
}

static int
resolve_pwm_af(GPIO_TypeDef *port, uint8_t index, const PwmBinding *binding,
               uint8_t *af_num)
{
  if (binding == nullptr || af_num == nullptr ||
      !is_valid_pwm_binding(*binding))
    return PIN_CONFIG_CONFLICT;

  TIM_TypeDef *timer = binding->timer;
  uint8_t channel = binding->channel;

  if (timer == TIM1)
  {
    if ((channel == 1U &&
         (is_pin(port, index, GPIOA, 8) || is_pin(port, index, GPIOE, 9))) ||
        (channel == 2U &&
         (is_pin(port, index, GPIOA, 9) || is_pin(port, index, GPIOE, 11))) ||
        (channel == 3U &&
         (is_pin(port, index, GPIOA, 10) || is_pin(port, index, GPIOE, 13))) ||
        (channel == 4U &&
         (is_pin(port, index, GPIOA, 11) || is_pin(port, index, GPIOE, 14))))
    {
      *af_num = 1U;
      return 0;
    }
  }
  else if (timer == TIM2)
  {
    if ((channel == 1U &&
         (is_pin(port, index, GPIOA, 0) || is_pin(port, index, GPIOA, 5) ||
          is_pin(port, index, GPIOA, 15))) ||
        (channel == 2U &&
         (is_pin(port, index, GPIOA, 1) || is_pin(port, index, GPIOB, 3))) ||
        (channel == 3U &&
         (is_pin(port, index, GPIOA, 2) || is_pin(port, index, GPIOB, 10))) ||
        (channel == 4U &&
         (is_pin(port, index, GPIOA, 3) || is_pin(port, index, GPIOB, 11))))
    {
      *af_num = 1U;
      return 0;
    }
  }
  else if (timer == TIM3)
  {
    if ((channel == 1U &&
         (is_pin(port, index, GPIOA, 6) || is_pin(port, index, GPIOB, 4) ||
          is_pin(port, index, GPIOC, 6))) ||
        (channel == 2U &&
         (is_pin(port, index, GPIOA, 7) || is_pin(port, index, GPIOB, 5) ||
          is_pin(port, index, GPIOC, 7))) ||
        (channel == 3U &&
         (is_pin(port, index, GPIOB, 0) || is_pin(port, index, GPIOC, 8))) ||
        (channel == 4U &&
         (is_pin(port, index, GPIOB, 1) || is_pin(port, index, GPIOC, 9))))
    {
      *af_num = 2U;
      return 0;
    }
  }
  else if (timer == TIM4)
  {
    if ((channel == 1U &&
         (is_pin(port, index, GPIOB, 6) || is_pin(port, index, GPIOD, 12))) ||
        (channel == 2U &&
         (is_pin(port, index, GPIOB, 7) || is_pin(port, index, GPIOD, 13))) ||
        (channel == 3U &&
         (is_pin(port, index, GPIOB, 8) || is_pin(port, index, GPIOD, 14))) ||
        (channel == 4U &&
         (is_pin(port, index, GPIOB, 9) || is_pin(port, index, GPIOD, 15))))
    {
      *af_num = 2U;
      return 0;
    }
  }
#ifdef TIM5
  else if (timer == TIM5)
  {
    if ((channel == 1U && is_pin(port, index, GPIOA, 0)) ||
        (channel == 2U && is_pin(port, index, GPIOA, 1)) ||
        (channel == 3U && is_pin(port, index, GPIOA, 2)) ||
        (channel == 4U && is_pin(port, index, GPIOA, 3)))
    {
      *af_num = 2U;
      return 0;
    }
  }
#endif

  return PIN_CONFIG_CONFLICT;
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
configure_pin(GPIO_TypeDef *port, uint8_t index, PinCfg cfg,
              const PwmBinding *pwm_binding)
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
    TRY(resolve_pwm_af(port, index, pwm_binding, &af_num));
  }
  else if (has_cfg(cfg, PinCfg::ANALOG))
  {
    effective_cfg = PinCfg::ANALOG;
  }

  // MODER: 00=input, 01=output, 10=AF, 11=analog
  uint32_t moder = 0b00;
  if (has_cfg(effective_cfg, PinCfg::ANALOG))
    moder = 0b11;
  else if (has_cfg(effective_cfg, PinCfg::AF))
    moder = 0b10;
  else if (has_cfg(effective_cfg, PinCfg::OUT))
    moder = 0b01;

  MOD_BIT_V(port->MODER, index * 2U, 0b11U, moder);

  // OTYPER: 0=push-pull, 1=open-drain
  if (moder == 0b01 || moder == 0b10) // output or AF
  {
    uint32_t otyper = has_cfg(effective_cfg, PinCfg::OD) ? 1U : 0U;
    MOD_BIT_V(port->OTYPER, index, 0b1U, otyper);

    // OSPEEDR: 00=low, 01=medium, 10=high, 11=very high.
    MOD_BIT_V(port->OSPEEDR, index * 2U, 0b11U, speed_bits(cfg));

    if (af_num != 0xFF)
      MOD_BIT_V(port->AFR[index / 8U], (index % 8U) * 4U, 0xFU, af_num);
  }

  // PUPDR: 00=none, 01=pull-up, 10=pull-down
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
  // PUPDR: 01 = pull-up
  MOD_BIT_V(port->PUPDR, index * 2U, 0b11U, 0b01U);
  return 0;
}

int
configure_pin_pull_down(GPIO_TypeDef *port, uint8_t index)
{
  // PUPDR: 10 = pull-down
  MOD_BIT_V(port->PUPDR, index * 2U, 0b11U, 0b10U);
  return 0;
}

int
reset_pin(GPIO_TypeDef *port, uint8_t index)
{
  disable_pin_irq(port, index);

  // Input floating: MODER=00, OSPEEDR=00, OTYPER=0, PUPDR=00
  MOD_BIT_V(port->MODER, index * 2U, 0b11U, 0b00U);
  MOD_BIT_V(port->OSPEEDR, index * 2U, 0b11U, 0b00U);
  MOD_BIT_V(port->OTYPER, index, 0b1U, 0b0U);
  MOD_BIT_V(port->PUPDR, index * 2U, 0b11U, 0b00U);

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
is_valid_pwm_binding(TIM_TypeDef *timer, uint8_t channel)
{
  if (timer == TIM1)
    return channel >= 1U && channel <= 4U;
  if (timer == TIM2)
    return channel >= 1U && channel <= 4U;
  if (timer == TIM3)
    return channel >= 1U && channel <= 4U;
  if (timer == TIM4)
    return channel >= 1U && channel <= 4U;
#ifdef TIM5
  if (timer == TIM5)
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

  EXTI->PR = pin_bit;
  return true;
}

}; // namespace Embys::Stm32::Gpio

#endif // STM32F7xx
