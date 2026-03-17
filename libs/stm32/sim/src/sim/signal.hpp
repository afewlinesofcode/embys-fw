/**
 * @file signal.hpp
 * @author Stanislav Yaranov (stanislav.yaranov@gmail.com)
 * @brief Signal handling for the STM32 simulated environment.
 *
 * Provides a signal handler for SIGINT to allow graceful shutdown of the
 * simulation when the user presses Ctrl+C without needing to forcefully
 * terminate the docker container.
 *
 * The `register_int_signal` function should be
 * called during initialization to set up the signal handler.
 *
 * @version 0.1
 * @date 2026-03-17
 * @copyright Copyright (c) 2026
 *
 */

#pragma once

#include <csignal>
#include <cstdlib>
#include <iostream>

namespace Embys::Stm32::Sim
{

void
register_int_signal();

}; // namespace Embys::Stm32::Sim
