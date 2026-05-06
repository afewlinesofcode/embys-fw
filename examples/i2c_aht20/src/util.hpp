#pragma once

#include <stdint.h>

namespace Embys::Stm32::I2c::Dev::I2cAht20
{

void
write_temperature(char *buf, float v);

void
write_humidity(char *buf, float v);

}; // namespace Embys::Stm32::I2c::Dev::I2cAht20
