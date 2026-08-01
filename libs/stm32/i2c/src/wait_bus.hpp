/**
 * @file wait_bus.hpp
 * @author Stanislav Yaranov (stanislav.yaranov@gmail.com)
 * @brief Polls the I2C bus-busy flag at a fixed interval until the bus is
 * free or a check-count limit is exceeded.
 *
 * @version 0.1
 * @date 2026-05-04
 * @copyright Copyright (c) 2026
 *
 */
#pragma once

#include <stdint.h>

#include <embys/stm32/base/event.hpp>
#include <embys/stm32/types.hpp>

#include "def.hpp"
#include "stm32xx.hpp"

namespace Embys::Stm32::Base
{
class Loop;
}

namespace Embys::Stm32::I2c
{

class Bus;

class WaitBus
{
public:
  WaitBus() = delete;
  WaitBus(const WaitBus &) = delete;
  WaitBus(WaitBus &&) = delete;
  WaitBus &
  operator=(const WaitBus &) = delete;
  WaitBus &
  operator=(WaitBus &&) = delete;

  /**
   * @brief Construct a WaitBus object.
   * @param bus  I2C Bus to monitor; i2c peripheral, loop, and check count are
   *             derived from it (20 checks at >100 kHz, 50 checks otherwise,
   *             with a 1 ms polling interval).
   * @param cb   Callback invoked with 0 when the bus is free, or BUS_BUSY
   *             when the check limit is reached.
   */
  WaitBus(Bus *bus, Callback<int> cb);

  /**
   * @brief Start waiting for the bus to become idle.
   * Calls cb(0) immediately if the bus is already free. Otherwise schedules
   * periodic checks, calling cb(BUS_BUSY) if the limit is reached.
   * @return 0 on success, negative error code on scheduling failure.
   */
  int
  start();

private:
#ifndef STM32_SIM
  static constexpr uint32_t check_us = 1000u;
#else
  static constexpr uint32_t check_us = 10u; // Faster checks for simulation
#endif

  I2C_TypeDef *i2c;
  uint32_t checks_count;
  uint32_t count = 0;
  Callback<int> cb;
  Base::Event event;

  static void
  event_callback(void *context);
};

}; // namespace Embys::Stm32::I2c
