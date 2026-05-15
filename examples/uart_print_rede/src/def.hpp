#pragma once

#include <stdint.h>

#ifndef STM32_SIM
// Real hardware: 72 MHz system clock, 9600 baud
constexpr uint32_t UART_BAUD = 9600;
// Repeat every 2 seconds
constexpr uint32_t PRINT_INTERVAL_US = 2000000;
constexpr uint32_t LED_BLINK_US = 50000; // 50 ms
#else
// Simulation: collapse timing so the loop finishes quickly
constexpr uint32_t UART_BAUD = 9600;
constexpr uint32_t PRINT_INTERVAL_US = 20000;
constexpr uint32_t LED_BLINK_US = 500; // 5 ms
#endif
