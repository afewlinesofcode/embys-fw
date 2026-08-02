#ifdef STM32F4xx

#include "../../hal.hpp"

#include <embys/stm32/def.hpp>

#include "../../def.hpp"
#include "../../stm32xx.hpp"

// GPIO clocks are on AHB1. EXTI source routing via SYSCFG->EXTICR.
// Pin configuration uses MODER/OTYPER/OSPEEDR/PUPDR registers.

namespace Embys::Stm32::Gpio
{

bool
is_valid_pwm_binding(const PwmBinding *binding);
Status
enable_pin_irq(GPIO_TypeDef *port, uint8_t pin_index);
Status
disable_pin_irq(GPIO_TypeDef *port, uint8_t pin_index);

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

  // Default - VHIGH
  return 0b11U;
}

static inline bool
is_pin(GPIO_TypeDef *port, uint8_t index, GPIO_TypeDef *expected_port,
       uint8_t expected_index)
{
  return port == expected_port && index == expected_index;
}

static Status
resolve_pwm_af(GPIO_TypeDef *port, uint8_t index, const PwmBinding *binding,
               uint8_t *af_num)
{
  if (binding == nullptr || af_num == nullptr || !is_valid_pwm_binding(binding))
    return Status::failure(Error::ConfigurationConflict);

  const TIM_TypeDef *timer = binding->timer->get_peripheral();
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
      return Status::success();
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
      return Status::success();
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
      return Status::success();
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
      return Status::success();
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
      return Status::success();
    }
  }
#endif

  return Status::failure(Error::ConfigurationConflict);
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
#ifdef GPIOF
  if (port == GPIOF)
    return 5;
#endif
#ifdef GPIOG
  if (port == GPIOG)
    return 6;
#endif
  if (port == GPIOH)
    return 7;
#ifdef GPIOI
  if (port == GPIOI)
    return 8;
#endif
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
#ifdef GPIOF
  if (port == GPIOF)
    return RCC_AHB1ENR_GPIOFEN;
#endif
#ifdef GPIOG
  if (port == GPIOG)
    return RCC_AHB1ENR_GPIOGEN;
#endif
  if (port == GPIOH)
    return RCC_AHB1ENR_GPIOHEN;
#ifdef GPIOI
  if (port == GPIOI)
    return RCC_AHB1ENR_GPIOIEN;
#endif
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
#ifdef GPIOF
  if (port == GPIOF)
    return RCC_AHB1RSTR_GPIOFRST;
#endif
#ifdef GPIOG
  if (port == GPIOG)
    return RCC_AHB1RSTR_GPIOGRST;
#endif
  if (port == GPIOH)
    return RCC_AHB1RSTR_GPIOHRST;
#ifdef GPIOI
  if (port == GPIOI)
    return RCC_AHB1RSTR_GPIOIRST;
#endif
  return 0;
}

Status
enable_gpio(GPIO_TypeDef *port)
{
  uint32_t en_mask = gpio_ahb1_en_mask(port);

  if (!en_mask)
    return Status::failure(Error::InvalidPort);

  if (RCC->AHB1ENR & en_mask)
    return Status::success();

  uint32_t rst_mask = gpio_ahb1_rst_mask(port);
  SET_BIT_V(RCC->AHB1ENR, en_mask);
  SET_BIT_V(RCC->AHB1RSTR, rst_mask);
  CLEAR_BIT_V(RCC->AHB1RSTR, rst_mask);
  (void)RCC->AHB1ENR;
  __DSB();

  return Status::success();
}

Status
disable_gpio(GPIO_TypeDef *port)
{
  uint32_t en_mask = gpio_ahb1_en_mask(port);

  if (!en_mask)
    return Status::failure(Error::InvalidPort);

  CLEAR_BIT_V(RCC->AHB1ENR, en_mask);
  (void)RCC->AHB1ENR;
  __DSB();

  return Status::success();
}

// ── EXTI source clock (SYSCFG on F4/F7) ──────────────────────────────────────

Status
enable_exti()
{
  if (RCC->APB2ENR & RCC_APB2ENR_SYSCFGEN)
    return Status::success();

  SET_BIT_V(RCC->APB2ENR, RCC_APB2ENR_SYSCFGEN);
  (void)RCC->APB2ENR;
  __DSB();

  return Status::success();
}

Status
disable_exti()
{
  CLEAR_BIT_V(RCC->APB2ENR, RCC_APB2ENR_SYSCFGEN);
  (void)RCC->APB2ENR;
  __DSB();

  return Status::success();
}

// ── Pin configuration
// ─────────────────────────────────────────────────────────

Status
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
    {
      const Status pwm_result =
          resolve_pwm_af(port, index, pwm_binding, &af_num);
      if (!pwm_result)
        return pwm_result;
    }
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

  MOD_BIT_V(port->MODER, index << 1, 0b11U, moder);

  // OTYPER: 0=push-pull, 1=open-drain
  if (moder == 0b01 || moder == 0b10) // output or AF
  {
    uint32_t otyper = has_cfg(effective_cfg, PinCfg::OD) ? 1U : 0U;
    MOD_BIT_V(port->OTYPER, index, 0b1U, otyper);

    // OSPEEDR: 00=low, 01=medium, 10=high, 11=very high.
    MOD_BIT_V(port->OSPEEDR, index << 1, 0b11U, speed_bits(cfg));

    if (af_num != 0xFF)
      MOD_BIT_V(port->AFR[index / 8U], (index % 8U) * 4U, 0xFU, af_num);
  }

  // PUPDR: 00=none, 01=pull-up, 10=pull-down
  uint32_t pupdr = 0b00;
  if (has_cfg(effective_cfg, PinCfg::PU))
    pupdr = 0b01;
  else if (has_cfg(effective_cfg, PinCfg::PD))
    pupdr = 0b10;
  MOD_BIT_V(port->PUPDR, index << 1, 0b11U, pupdr);

  if (!has_any_role(cfg) && has_cfg(cfg, PinCfg::LISTEN))
  {
    const Status irq_result = enable_pin_irq(port, index);
    if (!irq_result)
      return irq_result;
  }

  return Status::success();
}

Status
configure_pin_pull_up(GPIO_TypeDef *port, uint8_t index)
{
  // PUPDR: 01 = pull-up
  MOD_BIT_V(port->PUPDR, index << 1, 0b11U, 0b01U);
  return Status::success();
}

Status
configure_pin_pull_down(GPIO_TypeDef *port, uint8_t index)
{
  // PUPDR: 10 = pull-down
  MOD_BIT_V(port->PUPDR, index << 1, 0b11U, 0b10U);
  return Status::success();
}

Status
reset_pin(GPIO_TypeDef *port, uint8_t index, PinCfg cfg,
          [[maybe_unused]] const PwmBinding *pwm)
{
  PinCfg effective_cfg = get_effective_pin_cfg(cfg);

  if (has_cfg(effective_cfg, PinCfg::IN) && has_cfg(cfg, PinCfg::LISTEN))
    (void)disable_pin_irq(port, index);

  // Input floating: MODER=00, OSPEEDR=00, OTYPER=0, PUPDR=00
  MOD_BIT_V(port->MODER, index << 1, 0b11U, 0b00U);
  MOD_BIT_V(port->OSPEEDR, index << 1, 0b11U, 0b00U);
  MOD_BIT_V(port->OTYPER, index, 0b1U, 0b0U);
  MOD_BIT_V(port->PUPDR, index << 1, 0b11U, 0b00U);

  return Status::success();
}

// ── EXTI interrupt routing (via SYSCFG->EXTICR)
// ───────────────────────────────

Status
enable_pin_irq(GPIO_TypeDef *port, uint8_t pin_index)
{
  const Status exti_result = enable_exti();
  if (!exti_result)
    return exti_result;

  uint8_t port_num = get_port_num(port);

  if (port_num == 0xFF)
    return Status::failure(Error::InvalidPort);

  uint8_t exticr_index = pin_index >> 2;
  uint8_t exticr_shift = (pin_index & 0b11U) << 2;
  uint32_t exti_cfg = uint32_t(port_num) << exticr_shift;

  CLEAR_BIT_V(SYSCFG->EXTICR[exticr_index], 0xFU << exticr_shift);
  SET_BIT_V(SYSCFG->EXTICR[exticr_index], exti_cfg);

  if ((SYSCFG->EXTICR[exticr_index] & (0xFU << exticr_shift)) != exti_cfg)
    return Status::failure(Error::ExtiConfigurationFailed);

  uint32_t pin_bit = (1U << pin_index);
  SET_BIT_V(EXTI->IMR, pin_bit);
  SET_BIT_V(EXTI->RTSR, pin_bit);
  SET_BIT_V(EXTI->FTSR, pin_bit);

  return Status::success();
}

Status
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

  return Status::success();
}

bool
is_valid_pwm_binding(const PwmBinding *binding)
{
  if (!binding || !binding->timer || binding->channel == 0)
  {
    return false;
  }

  auto timer = binding->timer->get_peripheral();

  if (timer == TIM1)
    return binding->channel >= 1U && binding->channel <= 4U;
  if (timer == TIM2)
    return binding->channel >= 1U && binding->channel <= 4U;
  if (timer == TIM3)
    return binding->channel >= 1U && binding->channel <= 4U;
  if (timer == TIM4)
    return binding->channel >= 1U && binding->channel <= 4U;
#ifdef TIM5
  if (timer == TIM5)
    return binding->channel >= 1U && binding->channel <= 4U;
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

#endif // STM32F4xx
