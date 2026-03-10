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

#define TIM2 (Embys::Stm32::Sim::tim2)
#define TIM3 (Embys::Stm32::Sim::tim3)
#define TIM4 (Embys::Stm32::Sim::tim4)
#define RCC (Embys::Stm32::Sim::rcc)
#define EXTI (Embys::Stm32::Sim::exti)
#define AFIO (Embys::Stm32::Sim::afio)
#define GPIOA (Embys::Stm32::Sim::gpioa)
#define GPIOB (Embys::Stm32::Sim::gpiob)
#define GPIOC (Embys::Stm32::Sim::gpioc)
#define I2C1 (Embys::Stm32::Sim::i2c1)
#define I2C2 (Embys::Stm32::Sim::i2c2)
#define SPI1 (Embys::Stm32::Sim::spi1)
#define SPI2 (Embys::Stm32::Sim::spi2)
#define USART1 (Embys::Stm32::Sim::usart1)
#define USART2 (Embys::Stm32::Sim::usart2)
#define USART3 (Embys::Stm32::Sim::usart3)
#define CoreDebug (Embys::Stm32::Sim::coredebug)
#define DWT (Embys::Stm32::Sim::dwt)
#define SysTick (Embys::Stm32::Sim::systick)
#define NVIC (Embys::Stm32::Sim::nvic)
#define SCB (Embys::Stm32::Sim::scb)

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

#define __get_PRIMASK() Embys::Stm32::Sim::get_primask()
#define __set_PRIMASK(primask) Embys::Stm32::Sim::set_primask(primask)
#define __enable_irq()
#define __disable_irq()
#define __NVIC_EnableIRQ(irq_no) Embys::Stm32::Sim::enable_irq_no(irq_no)
#define __NVIC_DisableIRQ(irq_no) Embys::Stm32::Sim::disable_irq_no(irq_no)
#define __NVIC_SetPriority(irq_no, priority)
#define SysTick_Config(ticks)
#define __WFI() Embys::Stm32::Sim::wfi()
#define __DSB() Embys::Stm32::Sim::dsb()
#define __NOP() Embys::Stm32::Sim::nop()

#define SET_BIT_V(REG, BIT) ((REG) = (REG) | (BIT))
#define CLEAR_BIT_V(REG, BIT) ((REG) = (REG) & ~(BIT))
