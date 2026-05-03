#include "sim.hpp"

#ifdef STM32_SIM

#include <iomanip>

#include <embys/stm32/i2c-hd44780/def.hpp>

using namespace Embys::Stm32::I2c::Dev::Hd44780;

/**
 * @brief LCD I2C byte-stream parser
 * The PCF8574 backpack sends one byte per I2C transaction.  Every logical LCD
 * byte (command or data) is transmitted as two nibbles, each accompanied by an
 * Enable pulse, so 6 raw I2C bytes per LCD byte.  We reconstruct the byte by
 * watching EN edges.
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
      // EN rising edge — capture nibble and RS
      nibble[nibble_count] = static_cast<uint8_t>((byte & LCD_DATA_MASK) >> 4);
      rs_flag = byte & LCD_RS;
    }
    else if (en_was_high && !en)
    {
      // EN falling edge — nibble is committed
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
      // LCD_DATA — a printable character
      char ch = static_cast<char>(value);
      std::cout << "[LCD] PRINT '" << (std::isprint(ch) ? ch : '?') << "'"
                << std::endl;
      return;
    }

    // LCD_COMMAND
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
on_tx(void *context, uint8_t addr, std::vector<uint8_t> data)
{
  (void)context;

  // LCD I2C backpack address is usually 0x27 or 0x3F, we use 0x27 here
  if (addr == 0x27U)
  {
    for (uint8_t byte : data)
    {
      lcd_parser.feed(byte);
    }
  }
}

void
SIM_RESET(AppContext *context)
{
  Embys::Stm32::Sim::reset();
  Embys::Stm32::Sim::TIM2_IRQHandler_ptr = TIM2_IRQHandler;
  Embys::Stm32::Sim::EXTI0_IRQHandler_ptr = EXTI0_IRQHandler;
  Embys::Stm32::Sim::I2C1_EV_IRQHandler_ptr = I2C1_EV_IRQHandler;
  Embys::Stm32::Sim::I2C1_ER_IRQHandler_ptr = I2C1_ER_IRQHandler;
  Embys::Stm32::Sim::register_int_signal();

  Embys::Stm32::Sim::I2C::on_tx = {on_tx, context};

  Embys::Stm32::Sim::input_pipe.register_command(
      "btn_toggle",
      [](const std::string &, const std::vector<std::string> &)
      {
        std::cout << "Simulating button click" << std::endl;
        Embys::Stm32::Sim::Gpio::trigger_pin(GPIOA, 0, 0);
        Embys::Stm32::Sim::Base::add_delayed_hook(
            5, [](uint32_t)
            { Embys::Stm32::Sim::Gpio::trigger_pin(GPIOA, 0, 1); });
      });
}

#endif
