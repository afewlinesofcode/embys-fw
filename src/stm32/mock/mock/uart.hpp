/**
 * @file uart.hpp
 * @author Stanislav Yaranov (stanislav.yaranov@gmail.com)
 * @brief UART simulation for the STM32 mock environment.
 * @version 0.1
 * @date 2026-03-09
 * @copyright Copyright (c) 2026
 */

#pragma once

#include "core.hpp"
#include "uart/runtime.hpp"

namespace Embys::Stm32::Mock::Uart
{

/**
 * @brief Pointer to the USART peripheral instance being used in the mock
 * environment. This pointer can be switched between usart1, usart2, and
 * usart3 to simulate different USART peripherals.
 */
extern USART_TypeDef *usart;

/**
 * @brief Reset the UART simulation state, including the runtime state and
 * resetting the USART pointer to the default instance (usart1).
 */
void
reset();

} // namespace Embys::Stm32::Mock::Uart
