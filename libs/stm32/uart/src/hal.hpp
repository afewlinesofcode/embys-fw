/**
 * @file hal.hpp
 * @author Stanislav Yaranov (stanislav.yaranov@gmail.com)
 * @brief UART HAL shim — selects the versioned register-level helpers.
 *
 * Delegates to hal/v1/hal.hpp (F1/F4/sim, SR/DR layout) or
 * hal/v2/hal.hpp (F7/H7, ISR/RDR/TDR/ICR layout) based on UART_HAL_V1/V2
 * macros defined in stm32xx.hpp.
 *
 * @version 0.1
 * @date 2026-04-29
 * @copyright Copyright (c) 2026
 *
 */
#pragma once

#include <stdint.h>

#include <embys/stm32/def.hpp>

#include "def.hpp"
#include "stm32xx.hpp"

#if defined(UART_HAL_V1)
#include "hal/v1/hal.hpp"
#elif defined(UART_HAL_V2)
#include "hal/v2/hal.hpp"
#else
#error "No UART HAL version selected. Check stm32xx.hpp."
#endif

namespace Embys::Stm32::Uart
{

/**
 * @brief Enable USART clock, reset peripheral, and configure CR1/CR2/BRR.
 * Enables RXNE interrupt; TXE and TC interrupts are disabled.
 * @return 0 on success, negative error code on failure.
 */
int
enable_uart(USART_TypeDef *usart, uint32_t baud_rate, WordLength word_length,
            StopBits stop_bits, Parity parity);

/**
 * @brief Disable all USART interrupts, clear UE, disable peripheral clock.
 * @return 0 on success, negative error code on failure.
 */
int
disable_uart(USART_TypeDef *usart);

/**
 * @brief Return total frame bits for a given configuration.
 * Includes start bit, data bits, and stop bits. Parity is encoded inside
 * the data word on STM32 (uses one of the data bits), so it is not
 * counted separately.
 */
inline uint32_t
calc_frame_bits(WordLength word_length, StopBits stop_bits)
{
  uint32_t bits = 1u; // start bit
  bits += (word_length == WordLength::W9) ? 9u : 8u;
  bits += (stop_bits == StopBits::Two) ? 2u : 1u;
  return bits;
}

}; // namespace Embys::Stm32::Uart
