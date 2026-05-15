/**
 * @file stm32xx.hpp
 * @author Stanislav Yaranov (stanislav.yaranov@gmail.com)
 * @brief STM32 UART peripheral register header selector (hardware vs
 * simulator). Supports STM32F1xx, STM32F4xx, STM32F7xx, STM32H7xx families.
 *
 * @version 0.1
 * @date 2026-04-29
 * @copyright Copyright (c) 2026
 *
 */

#if defined(STM32F1xx)
#include <stm32f1xx.h>
#elif defined(STM32F4xx)
#include <stm32f4xx.h>
#elif defined(STM32F7xx)
#include <stm32f7xx.h>
#elif defined(STM32H7xx)
#include <stm32h7xx.h>
#else
#error                                                                         \
    "No STM32 family defined. Define STM32F1xx, STM32F4xx, STM32F7xx, or STM32H7xx."
#endif

#ifdef STM32_SIM
#include <embys/stm32/sim/sim.hpp>
#endif

// ── UART HAL version ──────────────────────────────────────────────────────
// V1: F1/F4 and sim (which emulates F1) — SR/DR register layout.
// V2: F7/H7 — ISR/RDR/TDR/ICR register layout.
#if defined(STM32F1xx) || defined(STM32F4xx)
#define UART_HAL_V1
#elif defined(STM32F7xx) || defined(STM32H7xx)
#define UART_HAL_V2
#endif
