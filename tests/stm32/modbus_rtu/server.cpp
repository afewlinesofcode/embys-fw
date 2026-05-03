#include <vector>

#include <embys/stm32/base/loop.hpp>
#include <embys/stm32/modbus-rtu/server.hpp>
#include <embys/stm32/modbus/def.hpp>
#include <embys/stm32/modbus/handler.hpp>
#include <embys/stm32/modbus/store.hpp>
#include <embys/stm32/sim/sim.hpp>
#include <embys/stm32/uart/bus.hpp>

#include "test.hpp"

using namespace Embys::Stm32;

// ── helpers ───────────────────────────────────────────────────────────────

static uint16_t
crc16(const uint8_t *data, uint16_t len)
{
  uint16_t crc = 0xFFFFU;
  for (uint16_t i = 0; i < len; i++)
  {
    crc ^= data[i];
    for (uint8_t j = 0; j < 8U; j++)
    {
      crc = (crc & 0x0001U) ? ((crc >> 1U) ^ 0xA001U) : (crc >> 1U);
    }
  }
  return crc;
}

static void
append_crc(std::vector<uint8_t> &frame)
{
  uint16_t crc = crc16(frame.data(), static_cast<uint16_t>(frame.size()));
  frame.push_back(static_cast<uint8_t>(crc >> 8U));
  frame.push_back(static_cast<uint8_t>(crc & 0xFFU));
}

static void
inject_frame(std::vector<uint8_t> frame)
{
  append_crc(frame);
  Sim::Uart::simulate_rx(frame);
}

static std::vector<uint8_t>
run_and_capture(Base::Loop &loop, uint32_t timeout_us = 5000U)
{
  loop.stop(timeout_us);
  loop.run();
  if (Sim::Uart::tx_buffers.empty())
    return {};
  return Sim::Uart::tx_buffers.back();
}

static bool
response_crc_valid(const std::vector<uint8_t> &buf)
{
  if (buf.size() < 2U)
    return false;
  uint16_t crc = crc16(buf.data(), static_cast<uint16_t>(buf.size() - 2U));
  return buf[buf.size() - 2U] == static_cast<uint8_t>(crc >> 8U) &&
         buf[buf.size() - 1U] == static_cast<uint8_t>(crc & 0xFFU);
}

namespace
{

struct RtuBaseFixture
{
  inline static Base::Timer *timer_ptr = nullptr;
  inline static Uart::Bus *uart_bus_ptr = nullptr;

  static void
  TIM2_IRQHandler()
  {
    CLEAR_BIT_V(TIM2->SR, TIM_SR_UIF);
    if (timer_ptr)
      timer_ptr->handle_irq();
  }

  static void
  USART2_IRQHandler()
  {
    if (uart_bus_ptr)
      uart_bus_ptr->handle_irq();
  }

  RtuBaseFixture()
  {
    Sim::reset();
    Sim::Uart::usart = USART2;
    USART2->DR = 0;
    USART2->SR = USART_SR_TXE | USART_SR_TC;
    timer_ptr = nullptr;
    uart_bus_ptr = nullptr;
    Sim::TIM2_IRQHandler_ptr = TIM2_IRQHandler;
    Sim::USART2_IRQHandler_ptr = USART2_IRQHandler;
  }
};

static constexpr size_t kEventsCapacity = 5;
static constexpr size_t kModulesCapacity = 1;

struct RtuLoopFixture : RtuBaseFixture
{
  Base::Event *event_slots[kEventsCapacity];
  Base::Event *active_event_slots[kEventsCapacity];
  Base::Module module_slots[kModulesCapacity];

  uint8_t rx_buf[Modbus::kFrameSize];

  Base::Timer timer;
  Base::Loop loop;
  Uart::Bus uart;

  RtuLoopFixture()
    : timer(TIM2), loop(&timer, event_slots, active_event_slots,
                        kEventsCapacity, module_slots, kModulesCapacity),
      uart(USART2, &loop, rx_buf, sizeof(rx_buf))
  {
    timer_ptr = &timer;
    uart_bus_ptr = &uart;
    uart.enable(9600);
  }
};

struct StoreFixture
{
  static constexpr uint16_t NC = 16, NDI = 8, NHR = 8, NIR = 4;
  uint8_t coils_buf[(NC + 7U) / 8U] = {};
  uint8_t di_buf[(NDI + 7U) / 8U] = {};
  uint16_t hr_buf[NHR] = {};
  uint16_t ir_buf[NIR] = {};
  Modbus::Store store{coils_buf, NC, di_buf, NDI, hr_buf, NHR, ir_buf, NIR};
  Modbus::Handler handler{&store};
};

struct ServerFixture : RtuLoopFixture, StoreFixture
{
  Modbus::Rtu::Server server{1, &handler, &uart};

  ServerFixture()
  {
    server.enable();
  }
};

} // anonymous namespace

// ─────────────────────────────────────────────────────────────────────────────

TEST_SUITE("ONLY modbus_rtu_server")
{

  TEST_CASE_FIXTURE(ServerFixture,
                    "Server: valid ReadHoldingRegisters request gets response")
  {
    store.set_holding_register(0, 0x1234U);
    store.set_holding_register(1, 0x5678U);

    std::vector<uint8_t> sent;
    uart.set_tx_callback({[](void *ctx, int)
                          {
                            auto *v = static_cast<std::vector<uint8_t> *>(ctx);
                            if (!Embys::Stm32::Sim::Uart::tx_buffers.empty())
                              *v = Embys::Stm32::Sim::Uart::tx_buffers.back();
                          },
                          &sent});

    inject_frame({0x01, 0x03, 0x00, 0x00, 0x00, 0x02});

    loop.stop(5000);
    loop.run();

    // Response: device=01 FC=03 byte_count=04 [0x12 0x34 0x56 0x78] + CRC
    REQUIRE(sent.size() >= 7U);
    CHECK(sent[0] == 0x01U);
    CHECK(sent[1] == 0x03U);
    CHECK(sent[2] == 0x04U); // byte count
    CHECK(sent[3] == 0x12U);
    CHECK(sent[4] == 0x34U);
    CHECK(sent[5] == 0x56U);
    CHECK(sent[6] == 0x78U);
  }

  TEST_CASE_FIXTURE(ServerFixture, "Server: frame with bad CRC is ignored")
  {
    std::vector<uint8_t> frame = {0x01, 0x03, 0x00, 0x00,
                                  0x00, 0x01, 0xFF, 0xFF}; // wrong CRC
    Embys::Stm32::Sim::Uart::simulate_rx(frame);

    loop.stop(5000);
    loop.run();

    CHECK(server.get_statistics().get_crc_errors() == 1U);
    CHECK(Embys::Stm32::Sim::Uart::tx_buffers.empty());
  }

  TEST_CASE_FIXTURE(ServerFixture,
                    "Server: frame for different device ID is ignored")
  {
    inject_frame({0x02, 0x03, 0x00, 0x00, 0x00, 0x01}); // device=2, not 1

    loop.stop(5000);
    loop.run();

    CHECK(Embys::Stm32::Sim::Uart::tx_buffers.empty());
    CHECK(server.get_statistics().get_responses() == 0U);
  }

  TEST_CASE_FIXTURE(ServerFixture,
                    "Server: broadcast request (device ID 0) is executed, "
                    "no response sent")
  {
    inject_frame({0x00, 0x05, 0x00, 0x00, 0xFF, 0x00}); // write coil 0 ON

    loop.stop(5000);
    loop.run();

    CHECK(Embys::Stm32::Sim::Uart::tx_buffers.empty());
    // Coil should still be written
    uint8_t out[1] = {};
    store.get_coils(0, out, 1);
    CHECK((out[0] & 0x01U) != 0U);
  }

  TEST_CASE_FIXTURE(ServerFixture, "Server: unknown FC gets exception response")
  {
    std::vector<uint8_t> sent;
    uart.set_tx_callback({[](void *ctx, int)
                          {
                            auto *v = static_cast<std::vector<uint8_t> *>(ctx);
                            if (!Embys::Stm32::Sim::Uart::tx_buffers.empty())
                              *v = Embys::Stm32::Sim::Uart::tx_buffers.back();
                          },
                          &sent});

    inject_frame({0x01, 0x42, 0x00, 0x00, 0x00, 0x01});

    loop.stop(5000);
    loop.run();

    REQUIRE(sent.size() >= 3U);
    CHECK(sent[1] == (0x42U | 0x80U));
    CHECK(sent[2] == Modbus::ExceptionCode::IllegalFunction);
    CHECK(server.get_statistics().get_exceptions() >= 1U);
  }

  TEST_CASE_FIXTURE(ServerFixture,
                    "Server: statistics count responses correctly")
  {
    inject_frame({0x01, 0x03, 0x00, 0x00, 0x00, 0x01});

    loop.stop(5000);
    loop.run();

    CHECK(server.get_statistics().get_responses() == 1U);
  }

  // ── Response byte layout ─────────────────────────────────────────────────

  TEST_CASE_FIXTURE(ServerFixture,
                    "ReadCoils response: device_id, FC 0x01, byte_count, "
                    "packed coil bits LSB-first, valid CRC")
  {
    store.set_coil(0, true);
    store.set_coil(2, true);
    // 4 coils: bit0=1 bit1=0 bit2=1 bit3=0 → 0b00000101 = 0x05

    inject_frame({0x01, 0x01, 0x00, 0x00, 0x00, 0x04});
    auto sent = run_and_capture(loop);

    // 2 header + 1 byte_count + 1 data byte + 2 CRC = 6
    REQUIRE(sent.size() == 6U);
    CHECK(sent[0] == 0x01U);
    CHECK(sent[1] == Modbus::FunctionCode::ReadCoils);
    CHECK(sent[2] == 0x01U); // byte_count = ceil(4/8) = 1
    CHECK(sent[3] == 0x05U); // coil bits: coil0=1, coil1=0, coil2=1, coil3=0
    CHECK(response_crc_valid(sent));
  }

  TEST_CASE_FIXTURE(ServerFixture,
                    "ReadDiscreteInputs response: device_id, FC 0x02, "
                    "byte_count, packed DI bits LSB-first, valid CRC")
  {
    store.set_discrete_input(1, true);
    store.set_discrete_input(3, true);
    // 4 DIs: bit0=0 bit1=1 bit2=0 bit3=1 → 0b00001010 = 0x0A

    inject_frame({0x01, 0x02, 0x00, 0x00, 0x00, 0x04});
    auto sent = run_and_capture(loop);

    REQUIRE(sent.size() == 6U);
    CHECK(sent[0] == 0x01U);
    CHECK(sent[1] == Modbus::FunctionCode::ReadDiscreteInputs);
    CHECK(sent[2] == 0x01U); // byte_count = ceil(4/8) = 1
    CHECK(sent[3] == 0x0AU); // DI bits: DI0=0, DI1=1, DI2=0, DI3=1
    CHECK(response_crc_valid(sent));
  }

  TEST_CASE_FIXTURE(ServerFixture,
                    "ReadHoldingRegisters response: device_id, FC 0x03, "
                    "byte_count, register values big-endian, valid CRC")
  {
    store.set_holding_register(0, 0xDEADU);
    store.set_holding_register(1, 0xBEEFU);

    inject_frame({0x01, 0x03, 0x00, 0x00, 0x00, 0x02});
    auto sent = run_and_capture(loop);

    // 2 header + 1 byte_count + 4 data bytes + 2 CRC = 9
    REQUIRE(sent.size() == 9U);
    CHECK(sent[0] == 0x01U);
    CHECK(sent[1] == Modbus::FunctionCode::ReadHoldingRegisters);
    CHECK(sent[2] == 0x04U); // byte_count = 2 * 2 = 4
    CHECK(sent[3] == 0xDEU); // reg 0 high
    CHECK(sent[4] == 0xADU); // reg 0 low
    CHECK(sent[5] == 0xBEU); // reg 1 high
    CHECK(sent[6] == 0xEFU); // reg 1 low
    CHECK(response_crc_valid(sent));
  }

  TEST_CASE_FIXTURE(ServerFixture,
                    "ReadInputRegisters response: device_id, FC 0x04, "
                    "byte_count, register value big-endian, valid CRC")
  {
    store.set_input_register(0, 0xABCDU);

    inject_frame({0x01, 0x04, 0x00, 0x00, 0x00, 0x01});
    auto sent = run_and_capture(loop);

    // 2 header + 1 byte_count + 2 data bytes + 2 CRC = 7
    REQUIRE(sent.size() == 7U);
    CHECK(sent[0] == 0x01U);
    CHECK(sent[1] == Modbus::FunctionCode::ReadInputRegisters);
    CHECK(sent[2] == 0x02U); // byte_count = 1 * 2 = 2
    CHECK(sent[3] == 0xABU); // value high
    CHECK(sent[4] == 0xCDU); // value low
    CHECK(response_crc_valid(sent));
  }

  TEST_CASE_FIXTURE(ServerFixture,
                    "WriteSingleCoil response: echoes device_id, FC 0x05, "
                    "address and output value big-endian, valid CRC")
  {
    // Write coil 3 ON (0xFF00)
    inject_frame({0x01, 0x05, 0x00, 0x03, 0xFF, 0x00});
    auto sent = run_and_capture(loop);

    // 2 header + 4 echo + 2 CRC = 8
    REQUIRE(sent.size() == 8U);
    CHECK(sent[0] == 0x01U);
    CHECK(sent[1] == Modbus::FunctionCode::WriteSingleCoil);
    CHECK(sent[2] == 0x00U); // address high
    CHECK(sent[3] == 0x03U); // address low
    CHECK(sent[4] == 0xFFU); // output value high (ON = 0xFF00)
    CHECK(sent[5] == 0x00U); // output value low
    CHECK(response_crc_valid(sent));
  }

  TEST_CASE_FIXTURE(ServerFixture,
                    "WriteSingleRegister response: echoes device_id, FC 0x06, "
                    "address and register value big-endian, valid CRC")
  {
    // Write register 2 = 0x1234
    inject_frame({0x01, 0x06, 0x00, 0x02, 0x12, 0x34});
    auto sent = run_and_capture(loop);

    // 2 header + 4 echo + 2 CRC = 8
    REQUIRE(sent.size() == 8U);
    CHECK(sent[0] == 0x01U);
    CHECK(sent[1] == Modbus::FunctionCode::WriteSingleRegister);
    CHECK(sent[2] == 0x00U); // address high
    CHECK(sent[3] == 0x02U); // address low
    CHECK(sent[4] == 0x12U); // value high
    CHECK(sent[5] == 0x34U); // value low
    CHECK(response_crc_valid(sent));
  }

  TEST_CASE_FIXTURE(ServerFixture,
                    "WriteMultipleCoils response: device_id, FC 0x0F, "
                    "starting address and quantity big-endian, valid CRC")
  {
    // Write 5 coils at address 0: 0b10101 = 0x15 packed in 1 byte
    inject_frame({0x01, 0x0F, 0x00, 0x00, 0x00, 0x05, 0x01, 0x15});
    auto sent = run_and_capture(loop);

    // 2 header + 4 (addr+qty) + 2 CRC = 8
    REQUIRE(sent.size() == 8U);
    CHECK(sent[0] == 0x01U);
    CHECK(sent[1] == Modbus::FunctionCode::WriteMultipleCoils);
    CHECK(sent[2] == 0x00U); // starting address high
    CHECK(sent[3] == 0x00U); // starting address low
    CHECK(sent[4] == 0x00U); // quantity high
    CHECK(sent[5] == 0x05U); // quantity low (5 coils)
    CHECK(response_crc_valid(sent));
  }

  TEST_CASE_FIXTURE(ServerFixture,
                    "WriteMultipleRegisters response: device_id, FC 0x10, "
                    "starting address and quantity big-endian, valid CRC")
  {
    // Write 2 registers at address 0: [0x0102, 0x0304]
    inject_frame(
        {0x01, 0x10, 0x00, 0x00, 0x00, 0x02, 0x04, 0x01, 0x02, 0x03, 0x04});
    auto sent = run_and_capture(loop);

    // 2 header + 4 (addr+qty) + 2 CRC = 8
    REQUIRE(sent.size() == 8U);
    CHECK(sent[0] == 0x01U);
    CHECK(sent[1] == Modbus::FunctionCode::WriteMultipleRegisters);
    CHECK(sent[2] == 0x00U); // starting address high
    CHECK(sent[3] == 0x00U); // starting address low
    CHECK(sent[4] == 0x00U); // quantity high
    CHECK(sent[5] == 0x02U); // quantity low (2 registers)
    CHECK(response_crc_valid(sent));
  }

} // TEST_SUITE("modbus_rtu_server")
