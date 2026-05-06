#pragma once

#include <stdint.h>

#ifndef STM32_SIM
// Real hardware configs

constexpr uint32_t QUERY_INTERVAL_US = 1000000u;
constexpr uint32_t LED_BLINK_US = 100000u;
#else
// Simulation configs — time is slower during simulation

constexpr uint32_t QUERY_INTERVAL_US = 10000u;
constexpr uint32_t LED_BLINK_US = 1000u;
#endif

// Forward declarations for types used in the application context

namespace Embys::Stm32::Gpio
{
class Pin;
};

namespace Embys::Stm32::Base
{
class Event;
};

namespace Embys::Stm32::I2c::Dev
{
namespace Aht20
{
class Device;
};
namespace I2cAht20
{
class Lcd;
};
}; // namespace Embys::Stm32::I2c::Dev

/**
 * @brief Application context to be used in callbacks
 */
struct AppContext
{
  Embys::Stm32::Gpio::Pin *led = nullptr;
  Embys::Stm32::Base::Event *query_event = nullptr;
  Embys::Stm32::Base::Event *led_off_event = nullptr;
  Embys::Stm32::I2c::Dev::Aht20::Device *aht20 = nullptr;
  Embys::Stm32::I2c::Dev::I2cAht20::Lcd *lcd = nullptr;
};
