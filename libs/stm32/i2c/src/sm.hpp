/**
 * @file sm.hpp
 * @author Stanislav Yaranov (stanislav.yaranov@gmail.com)
 * @brief I2C state machine family selector.
 *
 * @version 0.1
 * @date 2026-04-29
 * @copyright Copyright (c) 2026
 *
 */
#pragma once

#include "stm32xx.hpp"

#if defined(STM32F1xx)
#include "hal/f1/state_machine.hpp"
#elif defined(STM32F4xx)
#include "hal/f4/state_machine.hpp"
#else
#error "Unsupported I2C state machine backend"
#endif
