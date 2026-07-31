#pragma once

#include <cstdint>

#include "../../def.hpp"

namespace Embys::Stm32::Gpio
{

constexpr uint32_t
get_output_mode_bits(const PinCfg &cfg)
{
  if (has_cfg(cfg, PinCfg::LOW))
    return 0b10U; // 2MHz
  else if (has_cfg(cfg, PinCfg::MEDIUM))
    return 0b01U; // 10MHz

  return 0b11U; // default to highest speed if not specified
}

}; // namespace Embys::Stm32::Gpio
