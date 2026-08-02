#pragma once

#include <stdint.h>

#include <embys/stm32/i2c-aht20/device.hpp>

namespace Embys::Stm32::I2c::Dev::I2cAht20
{

void
write_temperature(char *buf, Aht20::Temperature value);

void
write_humidity(char *buf, Aht20::RelativeHumidity value);

}; // namespace Embys::Stm32::I2c::Dev::I2cAht20
