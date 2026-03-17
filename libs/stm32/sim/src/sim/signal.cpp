/**
 * @file signal.cpp
 * @author Stanislav Yaranov (stanislav.yaranov@gmail.com)
 * @brief Signal handling for the STM32 simulated environment.
 * @version 0.1
 * @date 2026-03-17
 * @copyright Copyright (c) 2026
 */

#include "signal.hpp"

namespace Embys::Stm32::Sim
{

static void
signal_handler(int signum)
{
  // Handle SIGINT (Ctrl+C) to gracefully stop the simulation
  if (signum == SIGINT)
  {
    std::cout << "SIGINT received, stopping simulation..." << std::endl;
    std::exit(0);
  }
}

void
register_int_signal()
{
  std::signal(SIGINT, signal_handler);
}

}; // namespace Embys::Stm32::Sim
