/**
 * @file sm.hpp
 * @author Stanislav Yaranov (stanislav.yaranov@gmail.com)
 * @brief I2C state machine version selector.
 *
 * Includes the appropriate version-specific state machine header based on the
 * HAL version defined in def.hpp.
 *
 * @version 0.1
 * @date 2026-04-29
 * @copyright Copyright (c) 2026
 *
 */
#pragma once

#include "stm32xx.hpp"

#ifdef I2C_HAL_V1
#include "hal/v1/sm.hpp"
#elif defined(I2C_HAL_V2)
#include "hal/v2/sm.hpp"
#endif
