/**
 * @file def.hpp
 * @author Stanislav Yaranov (stanislav.yaranov@gmail.com)
 * @brief AHT20 sensor driver definitions and callback types
 *
 * @version 0.1
 * @date 2026-04-29
 * @copyright Copyright (c) 2026
 *
 */
#pragma once

#include <embys/stm32/types.hpp>

namespace Embys::Stm32::I2c::Dev::Aht20
{

using Cb = Embys::Callable<int>;

enum Diag : int
{
  BASE_ERROR = -4000,
  CRC_ERROR,
  NOT_INITIALIZED,
  NOT_READY,
};

}; // namespace Embys::Stm32::I2c::Dev::Aht20
