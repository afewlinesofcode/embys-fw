/**
 * @file def.hpp
 * @author Stanislav Yaranov (stanislav.yaranov@gmail.com)
 * @brief Mock STM32 defs.
 * @version 0.1
 * @date 2026-03-09
 * @copyright Copyright (c) 2026
 *
 */

#pragma once

#include <stm32f1xx.h>

#undef TIM2
#undef TIM3
#undef TIM4
#undef RCC
#undef EXTI
#undef AFIO
#undef GPIOA
#undef GPIOB
#undef GPIOC
#undef I2C1
#undef I2C2
#undef SPI1
#undef SPI2
#undef USART1
#undef USART2
#undef USART3
#undef CoreDebug
#undef DWT
#undef SysTick
#undef NVIC
#undef SCB

#define TIM2 (Embys::Stm32::Mock::tim2)
#define TIM3 (Embys::Stm32::Mock::tim3)
#define TIM4 (Embys::Stm32::Mock::tim4)
#define RCC (Embys::Stm32::Mock::rcc)
#define EXTI (Embys::Stm32::Mock::exti)
#define AFIO (Embys::Stm32::Mock::afio)
#define GPIOA (Embys::Stm32::Mock::gpioa)
#define GPIOB (Embys::Stm32::Mock::gpiob)
#define GPIOC (Embys::Stm32::Mock::gpioc)
#define I2C1 (Embys::Stm32::Mock::i2c1)
#define I2C2 (Embys::Stm32::Mock::i2c2)
#define SPI1 (Embys::Stm32::Mock::spi1)
#define SPI2 (Embys::Stm32::Mock::spi2)
#define USART1 (Embys::Stm32::Mock::usart1)
#define USART2 (Embys::Stm32::Mock::usart2)
#define USART3 (Embys::Stm32::Mock::usart3)
#define CoreDebug (Embys::Stm32::Mock::coredebug)
#define DWT (Embys::Stm32::Mock::dwt)
#define SysTick (Embys::Stm32::Mock::systick)
#define NVIC (Embys::Stm32::Mock::nvic)
#define SCB (Embys::Stm32::Mock::scb)

#undef __get_PRIMASK
#undef __set_PRIMASK
#undef __enable_irq
#undef __disable_irq
#undef __NVIC_EnableIRQ
#undef __NVIC_DisableIRQ
#undef __NVIC_SetPriority
#undef SysTick_Config
#undef __WFI
#undef __DSB
#undef __NOP

#define __get_PRIMASK() Embys::Stm32::Mock::get_primask()
#define __set_PRIMASK(primask) Embys::Stm32::Mock::set_primask(primask)
#define __enable_irq()
#define __disable_irq()
#define __NVIC_EnableIRQ(irq_no) Embys::Stm32::Mock::enable_irq_no(irq_no)
#define __NVIC_DisableIRQ(irq_no) Embys::Stm32::Mock::disable_irq_no(irq_no)
#define __NVIC_SetPriority(irq_no, priority)
#define SysTick_Config(ticks)
#define __WFI() Embys::Stm32::Mock::wfi()
#define __DSB() Embys::Stm32::Mock::dsb()
#define __NOP() Embys::Stm32::Mock::nop()

#define SET_BIT_V(REG, BIT) ((REG) = (REG) | (BIT))
#define CLEAR_BIT_V(REG, BIT) ((REG) = (REG) & ~(BIT))
