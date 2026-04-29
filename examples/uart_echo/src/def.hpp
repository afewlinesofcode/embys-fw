#pragma once

#include <stdint.h>

// USART1: PA9 = TX (AF push-pull), PA10 = RX (input floating)

#ifndef STM32_SIM
constexpr uint32_t UART_BAUD = 115200;
constexpr size_t LINE_BUF_LEN = 64; // max echoed line length
#else
constexpr uint32_t UART_BAUD = 115200;
constexpr size_t LINE_BUF_LEN = 64;
#endif
