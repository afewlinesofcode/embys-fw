/**
 * @file sim.cpp
 * @author Stanislav Yaranov (stanislav.yaranov@gmail.com)
 * @brief STM32 simulation environment implementation.
 * @version 0.1
 * @date 2026-03-17
 * @copyright Copyright (c) 2026
 */

#include "sim.hpp"

namespace Embys::Stm32::Sim
{

void
reset()
{
  Core::reset();
  Base::reset();
  I2C::reset();
  Uart::reset();
}

}; // namespace Embys::Stm32::Sim
