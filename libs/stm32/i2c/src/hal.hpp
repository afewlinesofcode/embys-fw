/**
 * @file hal.hpp
 * @author Stanislav Yaranov (stanislav.yaranov@gmail.com)
 * @brief I2C family backend selector and peripheral declarations.
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

#if defined(STM32F1xx)
#include "hal/f1/backend.hpp"
#elif defined(STM32F4xx)
#include "hal/f4/backend.hpp"
#else
#error "Unsupported I2C backend"
#endif

namespace Embys::Stm32::I2c
{

/**
 * @brief Enable I2C peripheral clock, reset it, and configure timing
 * registers. Enables EV/ER interrupt generation; buffer interrupts are
 * left disabled and enabled on demand by the state machine (V1), or all
 * event interrupts are enabled unconditionally (V2).
 * @param i2c   Peripheral instance (I2C1, I2C2, …).
 * @param scl_hz Desired SCL frequency in Hz (≤100000 for standard mode,
 *               ≤400000 for fast mode).
 * @return 0 on success, negative error code on failure.
 */
int
enable_i2c(I2C_TypeDef *i2c, uint32_t scl_hz);

/**
 * @brief Disable all I2C interrupts and turn off the peripheral APB clock.
 * @return 0 on success, negative error code on failure.
 */
int
disable_i2c(I2C_TypeDef *i2c);

/**
 * @brief Attempt to recover a stuck or busy I2C bus.
 * Tries peripheral reset, soft reset (SWRST), and APB hard reset in
 * sequence, stopping as soon as the bus is idle and error-free.
 * @return 0 if recovered, BUS_STUCK if all recovery attempts fail.
 */
int
reset_i2c(I2C_TypeDef *i2c);

}; // namespace Embys::Stm32::I2c
