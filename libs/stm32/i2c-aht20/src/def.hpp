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
