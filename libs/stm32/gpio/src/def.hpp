#pragma once

#include <cstdint>

#include <embys/stm32/result.hpp>

#include "def/pin_cfg.hpp"
#include "def/pwm_binding.hpp"
#include "stm32xx.hpp"

namespace Embys::Stm32::Gpio
{

/**
 * @brief GPIO operation failures.
 */
enum class Error : uint8_t
{
  InvalidPort,
  PinConfigurationFailed,
  PullUpConfigurationFailed,
  PullDownConfigurationFailed,
  CnfConfigurationFailed,
  ExtiConfigurationFailed,
  ConfigurationConflict,
  BusFull,
  BusNotEnabled,
  ModuleCapacity,
};

using Status = Embys::Result<void, Error>;
using ReadResult = Embys::Result<bool, Error>;

}; // namespace Embys::Stm32::Gpio
