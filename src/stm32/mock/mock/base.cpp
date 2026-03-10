#include "base.hpp"

namespace Embys::Stm32::Mock
{

uint32_t core_clock = 72000000; // Default core clock frequency (72 MHz)
uint32_t cyc_per_us = 1;

static uint32_t cyc_within_us = 0;
static uint32_t mock_primask = 0; // 0 = interrupts enabled, 1 = disabled
static bool interrupted = false;

// EXTI interrupt handler lookup table
static void (**irq_handler_lookup[])() = {
    &EXTI0_IRQHandler_ptr,     &EXTI1_IRQHandler_ptr,
    &EXTI2_IRQHandler_ptr,     &EXTI3_IRQHandler_ptr,
    &EXTI4_IRQHandler_ptr,     &EXTI9_5_IRQHandler_ptr,
    &EXTI9_5_IRQHandler_ptr,   &EXTI9_5_IRQHandler_ptr,
    &EXTI9_5_IRQHandler_ptr,   &EXTI9_5_IRQHandler_ptr,
    &EXTI15_10_IRQHandler_ptr, &EXTI15_10_IRQHandler_ptr,
    &EXTI15_10_IRQHandler_ptr, &EXTI15_10_IRQHandler_ptr,
    &EXTI15_10_IRQHandler_ptr, &EXTI15_10_IRQHandler_ptr};

/**
 * @brief Check for timer interrupts and call the corresponding handler if
 * conditions are met.
 * @param tim_instance Pointer to the timer instance to check.
 * @param TIM_IRQHandler_ptr Pointer to the timer's interrupt handler function
 * pointer.
 * @return True if an interrupt was triggered and the handler was called, false
 * otherwise.
 */
bool
check_irq_tim(TIM_TypeDef *tim_instance, void (**TIM_IRQHandler_ptr)())
{
  if ((tim_instance->CR1 & TIM_CR1_CEN) &&
      (tim_instance->CNT >= tim_instance->ARR))
  {
    // Simulate timer overflow
    SET_BIT_V(tim_instance->SR, TIM_SR_UIF);
    tim_instance->CNT = 0;

    if (tim_instance->CR1 & TIM_CR1_OPM)
    {
      // One-pulse mode: disable counter
      CLEAR_BIT_V(tim_instance->CR1, TIM_CR1_CEN);
    }
  }

  if ((tim_instance->SR & TIM_SR_UIF))
  {
    if (*TIM_IRQHandler_ptr)
      (*TIM_IRQHandler_ptr)();

    return true;
  }

  return false;
}

/**
 * @brief Check for EXTI interrupts and call the corresponding handler if
 * conditions are met.
 * @return True if an interrupt was triggered and the handler was called, false
 * otherwise.
 */
bool
check_irq_exti()
{
  // Check for EXTI interrupts
  uint32_t pin_bit = 1;
  bool interrupted = false;

  for (uint8_t pin_index = 0; pin_index < 16; ++pin_index, pin_bit <<= 1)
  {
    if ((exti_instance.IMR & pin_bit) && // Interrupt mask enabled
        (exti_instance.PR & pin_bit)     // Pending register set
    )
    {
      auto irq_handler = irq_handler_lookup[pin_index];

      if (*irq_handler)
      {
        (*irq_handler)();
        interrupted = true;
      }
    }
  }

  // Clear pending register
  // Assuming all handlers have cleared their bits correctly
  // This is pure trust, may behave unexpectedly on the hardware if handlers
  // don't clear the bits!
  exti_instance.PR = 0;

  return interrupted;
}

/**
 * @brief Check for I2C event and error interrupts and call the corresponding
 * handlers if conditions are met.
 * @param i2c_instance Pointer to the I2C instance to check.
 * @param I2C_EV_IRQHandler_ptr Pointer to the I2C event interrupt handler
 * function pointer.
 * @param I2C_ER_IRQHandler_ptr Pointer to the I2C error interrupt handler
 * function pointer.
 * @return True if an interrupt was triggered and a handler was called, false
 * otherwise.
 */
bool
check_irq_i2c(I2C_TypeDef *i2c_instance, void (**I2C_EV_IRQHandler_ptr)(),
              void (**I2C_ER_IRQHandler_ptr)())
{
  bool interrupted = false;

  // Check for I2C event interrupts
  if (*I2C_EV_IRQHandler_ptr)
  {
    if (i2c_instance->SR1 &
        (I2C_SR1_SB | I2C_SR1_ADDR | I2C_SR1_BTF | I2C_SR1_RXNE | I2C_SR1_TXE))
    {
      (*I2C_EV_IRQHandler_ptr)();
      interrupted = true;
    }
  }

  // Check for I2C error interrupts
  if (*I2C_ER_IRQHandler_ptr)
  {
    if (i2c_instance->SR1 & (I2C_SR1_BERR | I2C_SR1_ARLO | I2C_SR1_AF |
                             I2C_SR1_OVR | I2C_SR1_TIMEOUT))
    {
      (*I2C_ER_IRQHandler_ptr)();
      interrupted = true;
    }
  }

  return interrupted;
}

/**
 * @brief Check for USART interrupts and call the corresponding handler if
 * conditions are met.
 * @param usart_instance Pointer to the USART instance to check.
 * @param USART_IRQHandler_ptr Pointer to the USART interrupt handler function
 * pointer.
 * @return True if an interrupt was triggered and the handler was called, false
 * otherwise.
 */
bool
check_irq_usart(USART_TypeDef *usart_instance, void (**USART_IRQHandler_ptr)())
{
  if (!*USART_IRQHandler_ptr)
  {
    return false;
  }

  const uint32_t sr = usart_instance->SR;
  const uint32_t cr1 = usart_instance->CR1;

  const bool rxne_irq = (sr & USART_SR_RXNE) && (cr1 & USART_CR1_RXNEIE);
  const bool txe_irq = (sr & USART_SR_TXE) && (cr1 & USART_CR1_TXEIE);
  const bool tc_irq = (sr & USART_SR_TC) && (cr1 & USART_CR1_TCIE);

  if (rxne_irq || txe_irq || tc_irq)
  {
    (*USART_IRQHandler_ptr)();
    return true;
  }

  return false;
}

/**
 * @brief Check for PendSV interrupt and call the corresponding handler if
 * conditions are met.
 * @return True if an interrupt was triggered and the handler was called, false
 * otherwise.
 */
bool
check_irq_pendsv()
{
  if (scb_instance.ICSR & SCB_ICSR_PENDSVSET_Msk)
  {
    if (PendSV_Handler_ptr)
      PendSV_Handler_ptr();

    CLEAR_BIT_V(scb_instance.ICSR, SCB_ICSR_PENDSVSET_Msk);
    return true;
  }

  return false;
}

uint32_t
get_primask()
{
  return mock_primask;
}

void
set_primask(uint32_t priMask)
{
  mock_primask = priMask;
}

void
disable_irq()
{
  mock_primask = 1;
}

void
enable_irq()
{
  mock_primask = 0;
}

void
wfi(void)
{
  interrupted = false;

  do
  {
    cycle();
  } while (!interrupted);
}

void
dsb(void)
{
  cycle();
}

void
nop(void)
{
  cycle();
}

namespace Base
{

struct DelayedHook
{
  uint32_t trigger_cyc;
  Hook hook;
};

// Persistent hooks that are called on every cycle
std::vector<Hook> hooks;

// Delayed hooks that are called once after a certain number of cycles
std::vector<DelayedHook> delayed_hooks;

// Test hooks that can be triggered manually from tests to simulate specific
// events
std::map<std::string, Hook> test_hooks;

void
reset()
{
  hooks.clear();
  delayed_hooks.clear();
  test_hooks.clear();
  cyc_per_us = core_clock / 1'000'000;
  cyc_within_us = 0;
  mock_primask = 0;
  interrupted = false;
}

void
add_hook(Hook hook)
{
  uint32_t base_cyc = dwt_instance.CYCCNT;
  Hook hook_fn = [base_cyc, hook](uint32_t cyc) { hook(cyc - base_cyc); };
  hooks.push_back(hook_fn);
}

void
add_delayed_hook(uint32_t delay_cyc, Hook hook)
{
  DelayedHook delayed_hook;
  delayed_hook.trigger_cyc =
      delay_cyc > 0 ? delay_cyc : 1; // Ensure at least 1 cycle delay
  delayed_hook.hook = hook;
  delayed_hooks.push_back(delayed_hook);
}

void
add_test_hook(const std::string &key, Hook hook)
{
  test_hooks[key] = hook;
}

void
trigger_test_hook(const std::string &key)
{
  if (test_hooks.contains(key))
    test_hooks[key](dwt_instance.CYCCNT);
}

}; // namespace Base

void
cycle()
{
  // Increment cycle count

  dwt_instance.CYCCNT = dwt_instance.CYCCNT + 1;
  ++cyc_within_us;

  // Increment timers if enabled

  if (cyc_within_us >= cyc_per_us)
  {
    cyc_within_us = 0;

    if (tim2_instance.CR1 & TIM_CR1_CEN)
      tim2_instance.CNT = tim2_instance.CNT + 1;

    if (tim3_instance.CR1 & TIM_CR1_CEN)
      tim3_instance.CNT = tim3_instance.CNT + 1;

    if (tim4_instance.CR1 & TIM_CR1_CEN)
      tim4_instance.CNT = tim4_instance.CNT + 1;
  }

  // Check for interrupts and call handlers

  if (check_irq_tim(&tim2_instance, &TIM2_IRQHandler_ptr))
    interrupted = true;
  if (check_irq_tim(&tim3_instance, &TIM3_IRQHandler_ptr))
    interrupted = true;
  if (check_irq_tim(&tim4_instance, &TIM4_IRQHandler_ptr))
    interrupted = true;

  if (check_irq_exti())
    interrupted = true;

  if (check_irq_i2c(&i2c1_instance, &I2C1_EV_IRQHandler_ptr,
                    &I2C1_ER_IRQHandler_ptr))
    interrupted = true;
  if (check_irq_i2c(&i2c2_instance, &I2C2_EV_IRQHandler_ptr,
                    &I2C2_ER_IRQHandler_ptr))
    interrupted = true;

  if (check_irq_usart(&usart1_instance, &USART1_IRQHandler_ptr))
    interrupted = true;
  if (check_irq_usart(&usart2_instance, &USART2_IRQHandler_ptr))
    interrupted = true;
  if (check_irq_usart(&usart3_instance, &USART3_IRQHandler_ptr))
    interrupted = true;

  if (check_irq_pendsv())
    interrupted = true;

  // Trigger persistent hooks

  for (auto &hook : Base::hooks)
    hook(dwt_instance.CYCCNT);

  // Trigger delayed hooks

  auto it = Base::delayed_hooks.begin();

  while (it != Base::delayed_hooks.end())
  {
    --it->trigger_cyc;

    if (it->trigger_cyc == 0)
    {
      it->hook(dwt_instance.CYCCNT);
      it = Base::delayed_hooks.erase(it);
    }
    else
      ++it;
  }
}

}; // namespace Embys::Stm32::Mock
