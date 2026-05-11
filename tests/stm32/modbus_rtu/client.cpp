#include <iostream>
#include <vector>

#include <embys/stm32/base/loop.hpp>
#include <embys/stm32/modbus-rtu/client.hpp>
#include <embys/stm32/modbus-rtu/diag.hpp>
#include <embys/stm32/modbus/def.hpp>
#include <embys/stm32/sim/sim.hpp>
#include <embys/stm32/uart/bus.hpp>

#include "test.hpp"

using namespace Embys::Stm32;

// ── helpers ───────────────────────────────────────────────────────────────

static void
append_crc_to_vec(std::vector<uint8_t> &frame)
{
  uint16_t crc = 0xFFFFU;
  for (uint8_t b : frame)
  {
    crc ^= b;
    for (uint8_t j = 0; j < 8U; j++)
    {
      crc = (crc & 0x0001U) ? ((crc >> 1U) ^ 0xA001U) : (crc >> 1U);
    }
  }
  frame.push_back(static_cast<uint8_t>(crc & 0xFFU));
  frame.push_back(static_cast<uint8_t>(crc >> 8U));
}

static void
inject_frame(std::vector<uint8_t> frame)
{
  append_crc_to_vec(frame);
  Sim::Uart::simulate_rx(frame);
}

namespace
{

// ── TestablClient ─────────────────────────────────────────────────────────
// Exposes protected Base members so test cases can inspect the outgoing frame
// without having to capture UART TX buffers.

struct TestablClient : Modbus::Rtu::Client
{
  TestablClient(Uart::Bus *uart, Embys::Stm32::Base::Loop *loop)
    : Modbus::Rtu::Client(uart, loop)
  {
  }

  using Modbus::Rtu::Base::buffer_out;
  using Modbus::Rtu::Base::buffer_out_len;
  using Modbus::Rtu::Base::validate_crc;
};

// ── Fixtures ──────────────────────────────────────────────────────────────

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
    timer_ptr = nullptr;
    uart_bus_ptr = nullptr;
    Sim::TIM2_IRQHandler_ptr = TIM2_IRQHandler;
    Sim::USART2_IRQHandler_ptr = USART2_IRQHandler;
  }
};

static constexpr size_t kEventsCapacity = 5;
static constexpr size_t kModulesCapacity = 1;

struct ClientFixture : RtuBaseFixture
{
  Base::Event *event_slots[kEventsCapacity];
  Base::Event *active_event_slots[kEventsCapacity];
  Base::Module module_slots[kModulesCapacity];

  uint8_t rx_buf[Modbus::kFrameSize];

  Base::Timer timer;
  Base::Loop loop;
  Uart::Bus uart;
  TestablClient client;

  ClientFixture()
    : timer(TIM2), loop(&timer, event_slots, active_event_slots,
                        kEventsCapacity, module_slots, kModulesCapacity),
      uart(USART2, &loop, rx_buf, sizeof(rx_buf)), client(&uart, &loop)
  {
    timer_ptr = &timer;
    uart_bus_ptr = &uart;
    uart.enable(9600);
    client.enable();
  }
};

} // anonymous namespace

// ─────────────────────────────────────────────────────────────────────────────

TEST_SUITE("modbus_rtu_client")
{

  TEST_CASE_FIXTURE(ClientFixture, "Client: is_available before any request")
  {
    CHECK(client.is_available() == true);
  }

  TEST_CASE_FIXTURE(ClientFixture,
                    "Client: ReadHoldingRegisters receives response")
  {
    static uint8_t resp_data[4] = {};
    static bool response_received = false;

    client.override_frame_delay_us(400); // use short delay for faster test

    client.read_holding_registers(
        1, 0, 2,
        {[](void *ctx, uint8_t dev, uint8_t fc, uint8_t qty, uint8_t *data)
         {
           response_received = true;
           auto *arr = static_cast<uint8_t *>(ctx);
           arr[0] = dev;
           arr[1] = fc;
           arr[2] = qty;
           if (data)
             arr[3] = data[0]; // first byte of response data
         },
         resp_data});

    auto response_handler = [](void *ctx)
    {
      (void)ctx;
      // Response frame: dev=1, FC=03, byte_count=4, [0xAA,0xBB,0xCC,0xDD]
      inject_frame({0x01, 0x03, 0x04, 0xAAU, 0xBBU, 0xCCU, 0xDDU});
    };

    Base::Event response_event(&loop, 0, {response_handler, nullptr});
    response_event.enable(100);

    // Run long enough for both events to execute and for the client to process
    // the response after frame delay
    loop.stop(500);
    loop.run();

    REQUIRE(response_received);
    CHECK(resp_data[0] == 0x01U); // device id
    CHECK(resp_data[1] == 0x03U); // function code
    CHECK(resp_data[2] == 0x02U); // quantity (byte_count / 2)
    CHECK(resp_data[3] == 0xAAU); // first data byte
  }

  TEST_CASE_FIXTURE(ClientFixture,
                    "Client: not available while waiting for response")
  {
    client.read_holding_registers(1, 0, 1, {});

    CHECK(client.is_available() == false);
  }

  TEST_CASE_FIXTURE(ClientFixture,
                    "Client: second request while pending returns error")
  {
    client.read_holding_registers(1, 0, 1, {});
    CHECK(client.read_holding_registers(1, 0, 1, {}) ==
          Modbus::Rtu::Diag::EXPECTING_RESPONSE);
  }

  TEST_CASE_FIXTURE(ClientFixture,
                    "Client: broadcast write does not wait for response")
  {
    // device_id = 0 → broadcast, no response expected
    CHECK(client.write_single_coil(0, 0, true, {}) == 0);
    CHECK(client.is_available() == true);
  }

  TEST_CASE_FIXTURE(ClientFixture,
                    "Client: timeout callback fires with null data")
  {
    uint8_t timeout_device = 0xFF;
    uint8_t timeout_qty = 0xFF;
    uint8_t *timeout_data = reinterpret_cast<uint8_t *>(1); // non-null sentinel

    struct Ctx
    {
      uint8_t &dev;
      uint8_t &qty;
      uint8_t *&data;
    } ctx{timeout_device, timeout_qty, timeout_data};

    Modbus::Rtu::Client::kRequestTimeoutUs = 500U; // shorten timeout for test

    client.read_holding_registers(
        1, 0, 2,
        {[](void *c, uint8_t dev, uint8_t, uint8_t qty, uint8_t *data)
         {
           auto *ctx = static_cast<Ctx *>(c);
           ctx->dev = dev;
           ctx->qty = qty;
           ctx->data = data;
         },
         &ctx});

    // Run long enough for the timeout to fire
    loop.stop(Modbus::Rtu::Client::kRequestTimeoutUs + 100U);
    loop.run();

    CHECK(timeout_device == 0x01U);
    CHECK(timeout_qty == 0U);
    CHECK(timeout_data == nullptr);
    CHECK(client.is_available() == true);
  }

  // ── Request frame construction ────────────────────────────────────────────
  // After each request method returns, buffer_out contains the fully built
  // frame (header + payload + CRC appended by send_frame).

  TEST_CASE_FIXTURE(ClientFixture,
                    "read_coils: frame encodes device_id, FC 0x01, address and "
                    "quantity big-endian, with valid CRC")
  {
    client.read_coils(0x01U, 0x0013U, 0x0008U, {});

    // 2 header + 4 payload + 2 CRC
    REQUIRE(client.buffer_out_len == 8U);
    CHECK(client.buffer_out[0] == 0x01U);                           // device_id
    CHECK(client.buffer_out[1] == Modbus::FunctionCode::ReadCoils); // FC 0x01
    CHECK(client.buffer_out[2] == 0x00U);                           // addr high
    CHECK(client.buffer_out[3] == 0x13U);                           // addr low
    CHECK(client.buffer_out[4] == 0x00U);                           // qty high
    CHECK(client.buffer_out[5] == 0x08U);                           // qty low
    CHECK(client.validate_crc(client.buffer_out, client.buffer_out_len));
  }

  TEST_CASE_FIXTURE(
      ClientFixture,
      "read_discrete_inputs: frame encodes device_id, FC 0x02, address and "
      "quantity big-endian, with valid CRC")
  {
    client.read_discrete_inputs(0x02U, 0x00C4U, 0x0010U, {});

    REQUIRE(client.buffer_out_len == 8U);
    CHECK(client.buffer_out[0] == 0x02U);
    CHECK(client.buffer_out[1] == Modbus::FunctionCode::ReadDiscreteInputs);
    CHECK(client.buffer_out[2] == 0x00U); // addr high
    CHECK(client.buffer_out[3] == 0xC4U); // addr low
    CHECK(client.buffer_out[4] == 0x00U); // qty high
    CHECK(client.buffer_out[5] == 0x10U); // qty low
    CHECK(client.validate_crc(client.buffer_out, client.buffer_out_len));
  }

  TEST_CASE_FIXTURE(ClientFixture,
                    "read_holding_registers: frame matches Modbus spec example "
                    "(device=0x11, addr=0x006B, qty=3 -> CRC 0x8776)")
  {
    // Modbus Application Protocol spec, section 6.3:
    // Request PDU bytes before CRC: 11 03 00 6B 00 03
    client.read_holding_registers(0x11U, 0x006BU, 0x0003U, {});

    REQUIRE(client.buffer_out_len == 8U);
    CHECK(client.buffer_out[0] == 0x11U);
    CHECK(client.buffer_out[1] == Modbus::FunctionCode::ReadHoldingRegisters);
    CHECK(client.buffer_out[2] == 0x00U); // addr high
    CHECK(client.buffer_out[3] == 0x6BU); // addr low
    CHECK(client.buffer_out[4] == 0x00U); // qty high
    CHECK(client.buffer_out[5] == 0x03U); // qty low
    CHECK(client.buffer_out[6] == 0x76U); // CRC low  (known from spec)
    CHECK(client.buffer_out[7] == 0x87U); // CRC high (known from spec)
  }

  TEST_CASE_FIXTURE(
      ClientFixture,
      "read_input_registers: frame encodes device_id, FC 0x04, address and "
      "quantity big-endian, with valid CRC")
  {
    client.read_input_registers(0x01U, 0x0008U, 0x0002U, {});

    REQUIRE(client.buffer_out_len == 8U);
    CHECK(client.buffer_out[0] == 0x01U);
    CHECK(client.buffer_out[1] == Modbus::FunctionCode::ReadInputRegisters);
    CHECK(client.buffer_out[2] == 0x00U); // addr high
    CHECK(client.buffer_out[3] == 0x08U); // addr low
    CHECK(client.buffer_out[4] == 0x00U); // qty high
    CHECK(client.buffer_out[5] == 0x02U); // qty low
    CHECK(client.validate_crc(client.buffer_out, client.buffer_out_len));
  }

  TEST_CASE_FIXTURE(
      ClientFixture,
      "write_single_coil ON: output value field is 0xFF00 big-endian")
  {
    client.write_single_coil(0x01U, 0x00ACU, true, {});

    REQUIRE(client.buffer_out_len == 8U);
    CHECK(client.buffer_out[0] == 0x01U);
    CHECK(client.buffer_out[1] == Modbus::FunctionCode::WriteSingleCoil);
    CHECK(client.buffer_out[2] == 0x00U); // addr high
    CHECK(client.buffer_out[3] == 0xACU); // addr low
    CHECK(client.buffer_out[4] == 0xFFU); // output value high (ON = 0xFF00)
    CHECK(client.buffer_out[5] == 0x00U); // output value low
    CHECK(client.validate_crc(client.buffer_out, client.buffer_out_len));
  }

  TEST_CASE_FIXTURE(
      ClientFixture,
      "write_single_coil OFF: output value field is 0x0000 big-endian")
  {
    client.write_single_coil(0x01U, 0x00ACU, false, {});

    REQUIRE(client.buffer_out_len == 8U);
    CHECK(client.buffer_out[1] == Modbus::FunctionCode::WriteSingleCoil);
    CHECK(client.buffer_out[2] == 0x00U);
    CHECK(client.buffer_out[3] == 0xACU);
    CHECK(client.buffer_out[4] == 0x00U); // output value high (OFF = 0x0000)
    CHECK(client.buffer_out[5] == 0x00U); // output value low
    CHECK(client.validate_crc(client.buffer_out, client.buffer_out_len));
  }

  TEST_CASE_FIXTURE(
      ClientFixture,
      "write_single_register: frame encodes address and register value "
      "big-endian, with valid CRC")
  {
    client.write_single_register(0x01U, 0x0001U, 0x0003U, {});

    REQUIRE(client.buffer_out_len == 8U);
    CHECK(client.buffer_out[0] == 0x01U);
    CHECK(client.buffer_out[1] == Modbus::FunctionCode::WriteSingleRegister);
    CHECK(client.buffer_out[2] == 0x00U); // addr high
    CHECK(client.buffer_out[3] == 0x01U); // addr low
    CHECK(client.buffer_out[4] == 0x00U); // value high
    CHECK(client.buffer_out[5] == 0x03U); // value low
    CHECK(client.validate_crc(client.buffer_out, client.buffer_out_len));
  }

  TEST_CASE_FIXTURE(
      ClientFixture,
      "write_multiple_coils: frame encodes address, quantity, byte_count "
      "and coil data bytes in order, with valid CRC")
  {
    // 10 coils packed into 2 bytes: 0b11001101 0b00000011
    const uint8_t coil_data[] = {0xCDU, 0x03U};
    client.write_multiple_coils(0x01U, 0x0013U, 0x000AU, coil_data, {});

    // 2 header + 4 (addr+qty) + 1 byte_count + 2 coil bytes + 2 CRC = 11
    REQUIRE(client.buffer_out_len == 11U);
    CHECK(client.buffer_out[0] == 0x01U);
    CHECK(client.buffer_out[1] == Modbus::FunctionCode::WriteMultipleCoils);
    CHECK(client.buffer_out[2] == 0x00U); // starting addr high
    CHECK(client.buffer_out[3] == 0x13U); // starting addr low
    CHECK(client.buffer_out[4] == 0x00U); // quantity high
    CHECK(client.buffer_out[5] == 0x0AU); // quantity low (10)
    CHECK(client.buffer_out[6] == 0x02U); // byte_count = ceil(10/8) = 2
    CHECK(client.buffer_out[7] == 0xCDU); // coil data byte 0
    CHECK(client.buffer_out[8] == 0x03U); // coil data byte 1
    CHECK(client.validate_crc(client.buffer_out, client.buffer_out_len));
  }

  TEST_CASE_FIXTURE(
      ClientFixture,
      "write_multiple_registers: frame encodes address, quantity, byte_count "
      "and register values big-endian, with valid CRC")
  {
    const uint16_t regs[] = {0x000AU, 0x0102U};
    client.write_multiple_registers(0x01U, 0x0001U, 0x0002U, regs, {});

    // 2 header + 4 (addr+qty) + 1 byte_count + 4 register bytes + 2 CRC = 13
    REQUIRE(client.buffer_out_len == 13U);
    CHECK(client.buffer_out[0] == 0x01U);
    CHECK(client.buffer_out[1] == Modbus::FunctionCode::WriteMultipleRegisters);
    CHECK(client.buffer_out[2] == 0x00U);  // starting addr high
    CHECK(client.buffer_out[3] == 0x01U);  // starting addr low
    CHECK(client.buffer_out[4] == 0x00U);  // quantity high
    CHECK(client.buffer_out[5] == 0x02U);  // quantity low (2 registers)
    CHECK(client.buffer_out[6] == 0x04U);  // byte_count = 2 * 2 = 4
    CHECK(client.buffer_out[7] == 0x00U);  // register 0 high
    CHECK(client.buffer_out[8] == 0x0AU);  // register 0 low
    CHECK(client.buffer_out[9] == 0x01U);  // register 1 high
    CHECK(client.buffer_out[10] == 0x02U); // register 1 low
    CHECK(client.validate_crc(client.buffer_out, client.buffer_out_len));
  }

} // TEST_SUITE("modbus_rtu_client")
