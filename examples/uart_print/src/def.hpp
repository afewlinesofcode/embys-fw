#pragma once

#include <stdint.h>

// USART1: PA9 = TX (AF push-pull), PA10 = RX (input floating)
// Routed to the on-board USB-UART adapter on most Blue Pill breakout boards.

#ifndef STM32_SIM
// Real hardware: 72 MHz system clock, 115200 baud
constexpr uint32_t UART_BAUD = 115200;
// Repeat every 2 seconds
constexpr uint32_t PRINT_INTERVAL_US = 2000000;
#else
// Simulation: collapse timing so the loop finishes quickly
constexpr uint32_t UART_BAUD = 115200;
constexpr uint32_t PRINT_INTERVAL_US = 20000;
#endif
