#pragma once

#include <stdint.h>

#ifndef STM32_SIM
// Real hardware configs

constexpr uint32_t LED_BLINK_INTERVAL_US = 500000;
#else
// Simulation configs - time is slower during simulation

constexpr uint32_t LED_BLINK_INTERVAL_US = 5000;
#endif
