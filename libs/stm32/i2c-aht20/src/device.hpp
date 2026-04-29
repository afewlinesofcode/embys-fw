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

#include <stdint.h>

#include <embys/stm32/base/loop.hpp>
#include <embys/stm32/i2c-common/delay.hpp>
#include <embys/stm32/i2c-common/read.hpp>
#include <embys/stm32/i2c-common/write.hpp>
#include <embys/stm32/i2c/bus.hpp>
#include <embys/stm32/types.hpp>

#include "def.hpp"

namespace Embys::Stm32::I2c::Dev::Aht20
{

class Device
{
public:
  struct Values
  {
    float humidity = 0.0f;
    float temperature = 0.0f;
  };

  using QueryCb = Embys::Callable<int, Values *>;

  Device() = delete;
  Device(const Device &) = delete;
  Device(Device &&) = delete;
  Device &
  operator=(const Device &) = delete;
  Device &
  operator=(Device &&) = delete;

  Device(Base::Loop *loop, I2c::Bus *bus);

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
  uint8_t buffer[8] = {};
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
  check_crc(const uint8_t *buf);

  static void
  command_callback(void *ctx, int result);
};

}; // namespace Embys::Stm32::I2c::Dev::Aht20
