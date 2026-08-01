/**
 * @file device.hpp
 * @author Stanislav Yaranov (stanislav.yaranov@gmail.com)
 * @brief AHT20 temperature and humidity sensor driver over I2C
 *
 * @version 0.1
 * @date 2026-04-29
 * @copyright Copyright (c) 2026
 *
 */
#pragma once

#include <array>
#include <cstdint>
#include <span>

#include <embys/stm32/base/loop.hpp>
#include <embys/stm32/i2c-common/delay.hpp>
#include <embys/stm32/i2c-common/read.hpp>
#include <embys/stm32/i2c-common/write.hpp>
#include <embys/stm32/i2c/bus.hpp>
#include <embys/stm32/types.hpp>

#include "def.hpp"

namespace Embys::Stm32::I2c::Dev::Aht20
{

struct Temperature
{
  int16_t centi_celsius = 0;
};

struct RelativeHumidity
{
  uint16_t centi_percent = 0;
};

[[nodiscard]] inline constexpr Temperature
temperature_from_raw(uint32_t raw) noexcept
{
  const uint64_t scaled = static_cast<uint64_t>(raw & 0xFFFFFU) * 20000U;
  return {static_cast<int16_t>(((scaled + (1U << 19U)) >> 20U) - 5000)};
}

[[nodiscard]] inline constexpr RelativeHumidity
humidity_from_raw(uint32_t raw) noexcept
{
  const uint64_t scaled = static_cast<uint64_t>(raw & 0xFFFFFU) * 10000U;
  return {static_cast<uint16_t>((scaled + (1U << 19U)) >> 20U)};
}

class Device
{
public:
  struct Values
  {
    RelativeHumidity humidity;
    Temperature temperature;
  };

  using QueryCb = Embys::Callback<int, Values>;

  Device() = delete;
  Device(const Device &) = delete;
  Device(Device &&) = delete;
  Device &
  operator=(const Device &) = delete;
  Device &
  operator=(Device &&) = delete;

  Device(Base::LoopCore &loop, I2c::BusCore &bus);

  inline bool
  is_initialized() const
  {
    return initialized;
  }

  void
  enable(Cb cb);

  void
  query(QueryCb cb);

private:
  I2c::Dev::Delay timeout;
  I2c::Dev::Write write;
  I2c::Dev::Read read;

  bool initialized = false;
  std::array<uint8_t, 8> buffer{};
  Values values;
  Cb enable_cb;
  QueryCb cb;

  enum
  {
    PowerUp,
    ReadCalibration,
    SendInitialization,
    WaitInitialization,
    RequestQuery,
    WaitQuery,
    ReadQuery,
  } stage = PowerUp;

  void
  power_up();

  void
  read_calibration_status();

  void
  handle_calibration_status();

  void
  send_initialization();

  void
  wait_initialization();

  void
  finish_initialization();

  void
  request_query();

  void
  wait_request_query();

  void
  read_query();

  void
  parse_query();

  void
  response(int rc);

  bool
  check_crc(std::span<const uint8_t, 7> data);

  static void
  command_callback(void *ctx, int result);
};

}; // namespace Embys::Stm32::I2c::Dev::Aht20
