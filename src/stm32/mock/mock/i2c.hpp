/**
 * @file i2c.hpp
 * @author Stanislav Yaranov (stanislav.yaranov@gmail.com)
 * @brief I2C simulation for the STM32 mock environment.
 * @version 0.1
 * @date 2026-03-09
 * @copyright Copyright (c) 2026
 */

#pragma once

#include <map>
#include <optional>
#include <vector>

#include "core.hpp"
#include "i2c/runtime.hpp"

namespace Embys::Stm32::Mock
{

namespace I2C
{

/**
 * @brief Pointer to the I2C peripheral instance being used in the mock
 * environment. This pointer can be switched between i2c1 and i2c2 to simulate
 * different I2C peripherals.
 */
extern I2C_TypeDef *i2c;

/**
 * @brief Reset the I2C simulation state, including the runtime state and
 * resetting the I2C pointer to the default instance (i2c1).
 */
void
reset();

/**
 * @brief Simulate the I2C bus being busy by setting the BUSY flag in the SR2
 * register. This can be used to test how code handles a busy I2C bus condition.
 */
void
simulate_busy();

}; // namespace I2C

}; // namespace Embys::Stm32::Mock
