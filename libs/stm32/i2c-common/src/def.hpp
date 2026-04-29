/**
 * @file def.hpp
 * @author Stanislav Yaranov (stanislav.yaranov@gmail.com)
 * @brief Common I2C device definitions and callback type alias
 *
 * @version 0.1
 * @date 2026-04-29
 * @copyright Copyright (c) 2026
 *
 */
#pragma once

#include <embys/stm32/types.hpp>

namespace Embys::Stm32::I2c::Dev
{

using Cb = Embys::Callable<int>;

}; // namespace Embys::Stm32::I2c::Dev
