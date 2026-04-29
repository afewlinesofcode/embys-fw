/**
 * @file def.hpp
 * @author Stanislav Yaranov (stanislav.yaranov@gmail.com)
 * @brief HD44780 LCD driver definitions and callback types
 *
 * @version 0.1
 * @date 2026-04-29
 * @copyright Copyright (c) 2026
 *
 */
#pragma once

#include <stdint.h>

#include <embys/stm32/types.hpp>

namespace Embys::Stm32::I2c::Dev::Hd44780
{

using Cb = Embys::Callable<int>;

// LCD Commands
static constexpr uint8_t LCD_CLEAR_DISPLAY = 0x01;
static constexpr uint8_t LCD_RETURN_HOME = 0x02;
static constexpr uint8_t LCD_ENTRY_MODE_SET = 0x04;
static constexpr uint8_t LCD_DISPLAY_CONTROL = 0x08;
static constexpr uint8_t LCD_CURSOR_SHIFT = 0x10;
static constexpr uint8_t LCD_FUNCTION_SET = 0x20;
static constexpr uint8_t LCD_SET_CGRAM_ADDR = 0x40;
static constexpr uint8_t LCD_SET_DDRAM_ADDR = 0x80;

// Entry Mode flags
static constexpr uint8_t LCD_ENTRY_RIGHT = 0x00;
static constexpr uint8_t LCD_ENTRY_LEFT = 0x02;
static constexpr uint8_t LCD_ENTRY_SHIFT_INCREMENT = 0x01;
static constexpr uint8_t LCD_ENTRY_SHIFT_DECREMENT = 0x00;

// Display Control flags
static constexpr uint8_t LCD_DISPLAY_ON = 0x04;
static constexpr uint8_t LCD_DISPLAY_OFF = 0x00;
static constexpr uint8_t LCD_CURSOR_ON = 0x02;
static constexpr uint8_t LCD_CURSOR_OFF = 0x00;
static constexpr uint8_t LCD_BLINK_ON = 0x01;
static constexpr uint8_t LCD_BLINK_OFF = 0x00;

// Function Set flags
static constexpr uint8_t LCD_4BIT_MODE = 0x00;
static constexpr uint8_t LCD_8BIT_MODE = 0x10;
static constexpr uint8_t LCD_1LINE = 0x00;
static constexpr uint8_t LCD_2LINE = 0x08;
static constexpr uint8_t LCD_5x8DOTS = 0x00;
static constexpr uint8_t LCD_5x10DOTS = 0x04;

// Cursor/Display Shift flags
static constexpr uint8_t LCD_DISPLAY_MOVE = 0x08;
static constexpr uint8_t LCD_CURSOR_MOVE = 0x00;
static constexpr uint8_t LCD_MOVE_RIGHT = 0x04;
static constexpr uint8_t LCD_MOVE_LEFT = 0x00;

// PCF8574 I2C backpack pin mapping
static constexpr uint8_t LCD_RS = 0x01;
static constexpr uint8_t LCD_RW = 0x02;
static constexpr uint8_t LCD_EN = 0x04;
static constexpr uint8_t LCD_BL = 0x08;
static constexpr uint8_t LCD_DATA_MASK = 0xF0;
static constexpr uint8_t LCD_DATA_SHIFT = 4;

// LCD Send Modes
static constexpr uint8_t LCD_COMMAND = 0x00;
static constexpr uint8_t LCD_DATA = LCD_RS;

// Row addresses for 20x4 LCD
inline constexpr uint8_t ROW_ADDRESSES[4] = {0x00, 0x40, 0x14, 0x54};

// Constants
static constexpr uint8_t LCD_COLS = 20;
static constexpr uint8_t LCD_ROWS = 4;

}; // namespace Embys::Stm32::I2c::Dev::Hd44780
