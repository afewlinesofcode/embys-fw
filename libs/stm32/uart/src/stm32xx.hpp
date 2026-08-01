/**
 * @file stm32xx.hpp
 * @author Stanislav Yaranov (stanislav.yaranov@gmail.com)
 * @brief STM32 UART peripheral register header selector (hardware vs
 * simulator). STM32F1 and STM32F4 are the currently supported families.
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
#else
#error "Unsupported UART family. STM32F1 and STM32F4 are supported."
#endif

#ifdef STM32_SIM
#include <embys/stm32/sim/sim.hpp>
#endif

#define EMBYS_UART_CLASSIC_REGISTERS
