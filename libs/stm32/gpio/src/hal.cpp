#include "hal.hpp"

#include <embys/stm32/def.hpp>

// All family-specific GPIO HAL functions are implemented in
// src/hal/<family>/hal.cpp, selected by the build system.
// This file contains only the portable pin I/O functions that share
// the same register layout (IDR, BSRR) across all supported families.

namespace Embys::Stm32::Gpio
{

int
read_pin(GPIO_TypeDef *port, uint8_t index, uint8_t *value)
{
  *value = (port->IDR & (1U << index)) ? 1 : 0;
  return 0;
}

int
write_pin(GPIO_TypeDef *port, uint8_t index, uint8_t value)
{
  if (value)
    port->BSRR = (1U << index);
  else
    port->BSRR = (1U << (index + 16));

  return 0;
}

}; // namespace Embys::Stm32::Gpio
