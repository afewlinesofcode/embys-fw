#include "sim.hpp"

#ifdef STM32_SIM

#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include <embys/stm32/i2c-hd44780/def.hpp>

using namespace Embys::Stm32::I2c::Dev::Hd44780;

// ── LCD I2C byte-stream parser ────────────────────────────────────────────

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
      std::cout << "[LCD] CLEAR" << std::endl;
    else if (value == LCD_RETURN_HOME)
      std::cout << "[LCD] HOME" << std::endl;
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

static void
on_i2c_tx(void *, uint8_t addr, std::vector<uint8_t> data)
{
  if (addr == 0x27U)
  {
    for (uint8_t byte : data)
      lcd_parser.feed(byte);
  }
}

// ── UART TX callback ──────────────────────────────────────────────────────

static void
on_uart_tx(void *, std::vector<uint8_t> data)
{
  std::cout << "[UART TX] (" << std::dec << data.size() << " bytes):";
  for (uint8_t b : data)
    std::cout << " " << std::hex << std::setw(2) << std::setfill('0')
              << static_cast<int>(b);
  std::cout << std::dec << std::endl;
}

// ── pipe command: inject a raw Modbus RTU frame ───────────────────────────
// Usage: echo "modbus_request <hex byte> <hex byte> ..." > /tmp/..._pipe
// The frame must include the 2-byte CRC.

static void
pipe_modbus_request(const std::string &, const std::vector<std::string> &args)
{
  std::vector<uint8_t> frame;
  frame.reserve(args.size());

  for (const auto &token : args)
  {
    unsigned long val = 0;
    try
    {
      val = std::stoul(token, nullptr, 16);
    }
    catch (...)
    {
      std::cerr << "[SIM] modbus_request: invalid hex byte '" << token << "'"
                << std::endl;
      return;
    }
    frame.push_back(static_cast<uint8_t>(val & 0xFFU));
  }

  if (frame.size() < 4U) // minimum: addr + FC + CRC
  {
    std::cerr << "[SIM] modbus_request: frame too short (" << frame.size()
              << " bytes)" << std::endl;
    return;
  }

  std::cout << "[SIM] Injecting Modbus frame (" << frame.size() << " bytes):";
  for (uint8_t b : frame)
    std::cout << " " << std::hex << std::setw(2) << std::setfill('0')
              << static_cast<int>(b);
  std::cout << std::dec << std::endl;

  Embys::Stm32::Sim::Uart::simulate_rx(frame);
}

// ── SIM_RESET ─────────────────────────────────────────────────────────────

void
SIM_RESET(AppContext *context)
{
  (void)context;

  Embys::Stm32::Sim::reset();
  Embys::Stm32::Sim::TIM2_IRQHandler_ptr = TIM2_IRQHandler;
  Embys::Stm32::Sim::USART1_IRQHandler_ptr = USART1_IRQHandler;
  Embys::Stm32::Sim::I2C1_EV_IRQHandler_ptr = I2C1_EV_IRQHandler;
  Embys::Stm32::Sim::I2C1_ER_IRQHandler_ptr = I2C1_ER_IRQHandler;
  Embys::Stm32::Sim::register_int_signal();

  Embys::Stm32::Sim::I2C::on_tx = {on_i2c_tx, nullptr};
  Embys::Stm32::Sim::Uart::on_tx = {on_uart_tx, nullptr};

  // HD44780 LCD at address 0x27 — calibration status: calibrated
  Embys::Stm32::Sim::I2C::simulate_response(0x27U, 0x00U, {});

  Embys::Stm32::Sim::input_pipe.register_command("modbus_request",
                                                 pipe_modbus_request);
}

#endif
