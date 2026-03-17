/**
 * @file core.hpp
 * @author Stanislav Yaranov (stanislav.yaranov@gmail.com)
 * @brief Definitions for core instances, and pointers to simulate the CMSIS
 * environment in the STM32 simulation.
 * @version 0.1
 * @date 2026-03-09
 * @copyright Copyright (c) 2026
 */

#pragma once

#include <iostream> // Provide std::cout during simulation

#include "def.hpp"

namespace Embys::Stm32::Sim
{

/*
 * Mock hardware register structures (copied from CMSIS headers)
 * and global pointers to simulate STM32 peripherals.
 */

extern TIM_TypeDef tim2_instance;
extern TIM_TypeDef tim3_instance;
extern TIM_TypeDef tim4_instance;
extern RCC_TypeDef rcc_instance;
extern EXTI_TypeDef exti_instance;
extern AFIO_TypeDef afio_instance;
extern GPIO_TypeDef gpioa_instance;
extern GPIO_TypeDef gpiob_instance;
extern GPIO_TypeDef gpioc_instance;
extern I2C_TypeDef i2c1_instance;
extern I2C_TypeDef i2c2_instance;
extern SPI_TypeDef spi1_instance;
extern SPI_TypeDef spi2_instance;
extern USART_TypeDef usart1_instance;
extern USART_TypeDef usart2_instance;
extern USART_TypeDef usart3_instance;
extern CoreDebug_Type coredebug_instance;
extern DWT_Type dwt_instance;
extern SysTick_Type systick_instance;
extern NVIC_Type nvic_instance;
extern SCB_Type scb_instance;

/*
 * Global pointers to mock hardware register instances.
 * These pointers simulate the addresses of the actual hardware registers
 * in the STM32 microcontroller.
 */

extern TIM_TypeDef *tim2;
extern TIM_TypeDef *tim3;
extern TIM_TypeDef *tim4;
extern RCC_TypeDef *rcc;
extern EXTI_TypeDef *exti;
extern AFIO_TypeDef *afio;
extern GPIO_TypeDef *gpioa;
extern GPIO_TypeDef *gpiob;
extern GPIO_TypeDef *gpioc;
extern I2C_TypeDef *i2c1;
extern I2C_TypeDef *i2c2;
extern SPI_TypeDef *spi1;
extern SPI_TypeDef *spi2;
extern USART_TypeDef *usart1;
extern USART_TypeDef *usart2;
extern USART_TypeDef *usart3;
extern CoreDebug_Type *coredebug;
extern DWT_Type *dwt;
extern SysTick_Type *systick;
extern NVIC_Type *nvic;
extern SCB_Type *scb;

/*
 * Mock interrupt handler function pointers.
 * These pointers simulate the addresses of the actual interrupt handlers
 * in the STM32 microcontroller.
 * Whenever an interrupt handler needs to be callable during testing, the
 * corresponding pointer can be set to point to the handler function.
 */

extern void (*TIM2_IRQHandler_ptr)();
extern void (*TIM3_IRQHandler_ptr)();
extern void (*TIM4_IRQHandler_ptr)();
extern void (*SysTick_Handler_ptr)();
extern void (*EXTI0_IRQHandler_ptr)();
extern void (*EXTI1_IRQHandler_ptr)();
extern void (*EXTI2_IRQHandler_ptr)();
extern void (*EXTI3_IRQHandler_ptr)();
extern void (*EXTI4_IRQHandler_ptr)();
extern void (*EXTI9_5_IRQHandler_ptr)();
extern void (*EXTI15_10_IRQHandler_ptr)();
extern void (*I2C1_EV_IRQHandler_ptr)();
extern void (*I2C1_ER_IRQHandler_ptr)();
extern void (*I2C2_EV_IRQHandler_ptr)();
extern void (*I2C2_ER_IRQHandler_ptr)();
extern void (*SPI1_IRQHandler_ptr)();
extern void (*SPI2_IRQHandler_ptr)();
extern void (*USART1_IRQHandler_ptr)();
extern void (*USART2_IRQHandler_ptr)();
extern void (*USART3_IRQHandler_ptr)();
extern void (*PendSV_Handler_ptr)();


namespace Core
{

/**
 * @brief Reset the mock core to its initial state.
 */
void
reset();

}; // namespace Core

}; // namespace Embys::Stm32::Sim
