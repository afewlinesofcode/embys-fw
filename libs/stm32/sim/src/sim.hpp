/**
 * @file sim.hpp
 * @author Stanislav Yaranov (stanislav.yaranov@gmail.com)
 * @brief STM32 simulation environment header.
 * @version 0.1
 * @date 2026-03-17
 * @copyright Copyright (c) 2026
 */

#pragma once

#include "sim/base.hpp"
#include "sim/core.hpp"
#include "sim/gpio.hpp"
#include "sim/i2c.hpp"
#include "sim/signal.hpp"
#include "sim/uart.hpp"

namespace Embys::Stm32::Sim
{

void
reset();

}; // namespace Embys::Stm32::Sim
