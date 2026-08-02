#include "sim.hpp"

#ifdef STM32_SIM

#include <cctype>
#include <iomanip>
#include <iostream>

#include <embys/stm32/i2c-hd44780/def.hpp>

using namespace Embys::Stm32::I2c::Dev::Hd44780;

/**
 * @brief LCD I2C byte-stream parser (same as i2c_btn_blink)
 */
struct LcdI2cParser
{
  uint8_t nibble[2] = {};
  uint8_t rs_flag = 0;
  int nibble_count = 0;
  bool en_was_high = false;

  void
  feed(uint8_t byte)
  {
    bool en = (byte & LCD_EN) != 0U;

    if (!en_was_high && en)
    {
      nibble[nibble_count] = static_cast<uint8_t>((byte & LCD_DATA_MASK) >> 4);
      rs_flag = byte & LCD_RS;
    }
    else if (en_was_high && !en)
    {
      ++nibble_count;
      if (nibble_count == 2)
      {
        emit(static_cast<uint8_t>((nibble[0] << 4) | nibble[1]), rs_flag);
        nibble_count = 0;
      }
    }

    en_was_high = en;
  }

  static void
  emit(uint8_t value, uint8_t rs)
  {
    if (rs != 0U)
    {
      char ch = static_cast<char>(value);
      std::cout << "[LCD] PRINT '"
                << (std::isprint(static_cast<unsigned char>(ch)) ? ch : '?')
                << "'" << std::endl;
      return;
    }

    if (value == LCD_CLEAR_DISPLAY)
    {
      std::cout << "[LCD] CLEAR" << std::endl;
    }
    else if (value == LCD_RETURN_HOME)
    {
      std::cout << "[LCD] HOME" << std::endl;
    }
    else if ((value & LCD_SET_DDRAM_ADDR) != 0U)
    {
      uint8_t addr = value & static_cast<uint8_t>(~LCD_SET_DDRAM_ADDR);
      uint8_t row = 0;
      uint8_t col = 0;
      if (addr >= ROW_ADDRESSES[3])
      {
        row = 3;
        col = static_cast<uint8_t>(addr - ROW_ADDRESSES[3]);
      }
      else if (addr >= ROW_ADDRESSES[1])
      {
        row = 1;
        col = static_cast<uint8_t>(addr - ROW_ADDRESSES[1]);
      }
      else if (addr >= ROW_ADDRESSES[2])
      {
        row = 2;
        col = static_cast<uint8_t>(addr - ROW_ADDRESSES[2]);
      }
      else
      {
        row = 0;
        col = addr;
      }
      std::cout << "[LCD] CURSOR AT row=" << static_cast<int>(row)
                << " col=" << static_cast<int>(col) << std::endl;
    }
  }
};

static LcdI2cParser lcd_parser;

void
on_tx(void *context, uint8_t addr, std::vector<uint8_t> data) noexcept
{
  (void)context;

  if (addr == 0x27U)
  {
    // HD44780 LCD via PCF8574 backpack
    for (uint8_t byte : data)
      lcd_parser.feed(byte);
    return;
  }

  if (addr == 0x38U)
  {
    // AHT20 sensor — reads are handled via simulate_response in SIM_RESET
    if (data.size() == 3u && data[0] == 0xACu)
      std::cout << "[AHT20] Trigger measurement" << std::endl;
    else if (data.size() == 3u && data[0] == 0xE1u)
      std::cout << "[AHT20] Send initialization" << std::endl;
  }
}

void
SIM_RESET(AppContext *context)
{
  Embys::Stm32::Sim::reset();
  Embys::Stm32::Sim::TIM2_IRQHandler_ptr = TIM2_IRQHandler;
  Embys::Stm32::Sim::I2C1_EV_IRQHandler_ptr = I2C1_EV_IRQHandler;
  Embys::Stm32::Sim::I2C1_ER_IRQHandler_ptr = I2C1_ER_IRQHandler;
  Embys::Stm32::Sim::register_int_signal();

  // AHT20 (0x38) calibration status read (reg=0x71): calibrated flag set
  Embys::Stm32::Sim::I2C::simulate_response(0x38u, 0x71u, {0x08u});

  // AHT20 (0x38) measurement read (plain read, reg=0): 50 % RH, 25.0 °C
  // Layout: status | hum[19:4] | hum[3:0]+temp[19:16] | temp[15:8] | temp[7:0]
  // | CRC raw_humidity=0x080000 (50.0 %), raw_temperature=0x060000 (25.0 °C)
  Embys::Stm32::Sim::I2C::simulate_response(
      0x38u, 0x00u, {0x08u, 0x80u, 0x00u, 0x06u, 0x00u, 0x00u, 0xA6u});

  Embys::Stm32::Sim::I2C::on_tx = {on_tx, context};
}

#endif
