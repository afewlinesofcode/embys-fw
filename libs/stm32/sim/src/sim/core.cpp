/**
 * @file core.cpp
 * @author Stanislav Yaranov (stanislav.yaranov@gmail.com)
 * @brief Core definitions for the STM32 simulation environment, including mock
 * hardware register structures and global variables to mimic the CMSIS
 * environment.
 * @version 0.1
 * @date 2026-03-09
 *
 * @copyright Copyright (c) 2026
 *
 */

#include "core.hpp"

#include <cstring> // For memset

/*
 * Compiling this means we're in simulation and no CMSIS sources are linked, so
 * we need to define the SystemCoreClock variable and the hardware register
 * structures that CMSIS would normally provide.
 */

uint32_t SystemCoreClock = 72000000;

namespace Embys::Stm32::Sim
{

// Mock hardware register instances
TIM_TypeDef tim2_instance = {};
TIM_TypeDef tim3_instance = {};
TIM_TypeDef tim4_instance = {};
RCC_TypeDef rcc_instance = {};
EXTI_TypeDef exti_instance = {};
AFIO_TypeDef afio_instance = {};
GPIO_TypeDef gpioa_instance = {};
GPIO_TypeDef gpiob_instance = {};
GPIO_TypeDef gpioc_instance = {};
I2C_TypeDef i2c1_instance = {};
I2C_TypeDef i2c2_instance = {};
SPI_TypeDef spi1_instance = {};
SPI_TypeDef spi2_instance = {};
USART_TypeDef usart1_instance = {};
USART_TypeDef usart2_instance = {};
USART_TypeDef usart3_instance = {};
CoreDebug_Type coredebug_instance = {};
DWT_Type dwt_instance = {};
SysTick_Type systick_instance = {};
NVIC_Type nvic_instance = {};
SCB_Type scb_instance = {};

// Global pointers (as expected by real STM32 code)
TIM_TypeDef *tim2 = &tim2_instance;
TIM_TypeDef *tim3 = &tim3_instance;
TIM_TypeDef *tim4 = &tim4_instance;
RCC_TypeDef *rcc = &rcc_instance;
EXTI_TypeDef *exti = &exti_instance;
AFIO_TypeDef *afio = &afio_instance;
GPIO_TypeDef *gpioa = &gpioa_instance;
GPIO_TypeDef *gpiob = &gpiob_instance;
GPIO_TypeDef *gpioc = &gpioc_instance;
I2C_TypeDef *i2c1 = &i2c1_instance;
I2C_TypeDef *i2c2 = &i2c2_instance;
SPI_TypeDef *spi1 = &spi1_instance;
SPI_TypeDef *spi2 = &spi2_instance;
USART_TypeDef *usart1 = &usart1_instance;
USART_TypeDef *usart2 = &usart2_instance;
USART_TypeDef *usart3 = &usart3_instance;
CoreDebug_Type *coredebug = &coredebug_instance;
DWT_Type *dwt = &dwt_instance;
SysTick_Type *systick = &systick_instance;
NVIC_Type *nvic = &nvic_instance;
SCB_Type *scb = &scb_instance;

// Mock interrupt handler function pointers
void (*TIM2_IRQHandler_ptr)() = nullptr;
void (*TIM3_IRQHandler_ptr)() = nullptr;
void (*TIM4_IRQHandler_ptr)() = nullptr;
void (*SysTick_Handler_ptr)() = nullptr;
void (*EXTI0_IRQHandler_ptr)() = nullptr;
void (*EXTI1_IRQHandler_ptr)() = nullptr;
void (*EXTI2_IRQHandler_ptr)() = nullptr;
void (*EXTI3_IRQHandler_ptr)() = nullptr;
void (*EXTI4_IRQHandler_ptr)() = nullptr;
void (*EXTI9_5_IRQHandler_ptr)() = nullptr;
void (*EXTI15_10_IRQHandler_ptr)() = nullptr;
void (*I2C1_EV_IRQHandler_ptr)() = nullptr;
void (*I2C1_ER_IRQHandler_ptr)() = nullptr;
void (*I2C2_EV_IRQHandler_ptr)() = nullptr;
void (*I2C2_ER_IRQHandler_ptr)() = nullptr;
void (*SPI1_IRQHandler_ptr)() = nullptr;
void (*SPI2_IRQHandler_ptr)() = nullptr;
void (*USART1_IRQHandler_ptr)() = nullptr;
void (*USART2_IRQHandler_ptr)() = nullptr;
void (*USART3_IRQHandler_ptr)() = nullptr;
void (*PendSV_Handler_ptr)() = nullptr;

// Static variable to track PRIMASK state


namespace Core
{

void
reset()
{
  memset(&tim2_instance, 0, sizeof(tim2_instance));
  memset(&tim3_instance, 0, sizeof(tim3_instance));
  memset(&tim4_instance, 0, sizeof(tim4_instance));
  // Suppress -Wclass-memaccess warning
  memset(static_cast<void *>(&systick_instance), 0, sizeof(systick_instance));
  memset(&nvic_instance, 0, sizeof(nvic_instance));
  // Suppress -Wclass-memaccess warning
  memset(static_cast<void *>(&scb_instance), 0, sizeof(scb_instance));
  memset(&rcc_instance, 0, sizeof(rcc_instance));
  memset(&exti_instance, 0, sizeof(exti_instance));
  memset(&afio_instance, 0, sizeof(afio_instance));
  memset(&gpioa_instance, 0, sizeof(gpioa_instance));
  memset(&gpiob_instance, 0, sizeof(gpiob_instance));
  memset(&gpioc_instance, 0, sizeof(gpioc_instance));
  memset(&i2c1_instance, 0, sizeof(i2c1_instance));
  memset(&i2c2_instance, 0, sizeof(i2c2_instance));
  memset(&spi1_instance, 0, sizeof(spi1_instance));
  memset(&spi2_instance, 0, sizeof(spi2_instance));
  memset(&usart1_instance, 0, sizeof(usart1_instance));
  memset(&usart2_instance, 0, sizeof(usart2_instance));
  memset(&usart3_instance, 0, sizeof(usart3_instance));
  memset(&coredebug_instance, 0, sizeof(coredebug_instance));
  // Suppress -Wclass-memaccess warning
  memset(static_cast<void *>(&dwt_instance), 0, sizeof(dwt_instance));

  TIM2_IRQHandler_ptr = nullptr;
  TIM3_IRQHandler_ptr = nullptr;
  TIM4_IRQHandler_ptr = nullptr;
  SysTick_Handler_ptr = nullptr;
  EXTI0_IRQHandler_ptr = nullptr;
  EXTI1_IRQHandler_ptr = nullptr;
  EXTI2_IRQHandler_ptr = nullptr;
  EXTI3_IRQHandler_ptr = nullptr;
  EXTI4_IRQHandler_ptr = nullptr;
  EXTI9_5_IRQHandler_ptr = nullptr;
  EXTI15_10_IRQHandler_ptr = nullptr;
  I2C1_EV_IRQHandler_ptr = nullptr;
  I2C1_ER_IRQHandler_ptr = nullptr;
  I2C2_EV_IRQHandler_ptr = nullptr;
  I2C2_ER_IRQHandler_ptr = nullptr;
  SPI1_IRQHandler_ptr = nullptr;
  SPI2_IRQHandler_ptr = nullptr;
  USART1_IRQHandler_ptr = nullptr;
  USART2_IRQHandler_ptr = nullptr;
  USART3_IRQHandler_ptr = nullptr;
  PendSV_Handler_ptr = nullptr;
}

}; // namespace Core

}; // namespace Embys::Stm32::Sim
