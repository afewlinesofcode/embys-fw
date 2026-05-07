#pragma once

#include <stdint.h>

#ifndef STM32_SIM
constexpr uint32_t UART_BAUD = 9600;
constexpr uint32_t LED_BLINK_US = 50000; // 50 ms
#else
constexpr uint32_t UART_BAUD = 9600;
constexpr uint32_t LED_BLINK_US = 500;
#endif

// Modbus RTU slave address
constexpr uint8_t MODBUS_SLAVE_ADDR = 0x01;

// All four tables start at this on-wire Modbus address
constexpr uint16_t MODBUS_BASE_ADDR = 0x1000;

// Number of items in each table
constexpr uint16_t MODBUS_TABLE_SIZE = 10;

// Forward declarations
namespace Embys::Stm32::Gpio
{
class Pin;
};

namespace Embys::Stm32::Base
{
class Event;
};

namespace ModbusRtuServer
{
class Lcd;
};

struct AppContext
{
  Embys::Stm32::Gpio::Pin *led = nullptr;
  Embys::Stm32::Base::Event *blink_off_event = nullptr;
  ModbusRtuServer::Lcd *lcd = nullptr;
};
