#pragma once

#include <stdint.h>

#ifndef STM32_SIM
// Real hardware configs

constexpr uint32_t LED_BLINK_INTERVAL_US = 500000;
#else
// Simulation configs — time is slower during simulation

constexpr uint32_t LED_BLINK_INTERVAL_US = 5000;
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

namespace Embys::Stm32::I2c::Dev::I2cBtnBlink
{
class Lcd;
};

/**
 * @brief Application context to be used in callbacks
 */
struct AppContext
{
  bool led_on = false;
  bool blink_on = false;
  uint32_t blink_count = 0;

  Embys::Stm32::Gpio::Pin *led = nullptr;
  Embys::Stm32::Base::Event *blink_event = nullptr;
  Embys::Stm32::I2c::Dev::I2cBtnBlink::Lcd *lcd = nullptr;
};
