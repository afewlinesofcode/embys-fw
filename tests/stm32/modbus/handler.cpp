#include <embys/stm32/modbus/def.hpp>
#include <embys/stm32/modbus/handler.hpp>
#include <embys/stm32/modbus/store.hpp>

#include "test.hpp"

using namespace Embys::Stm32::Modbus;

namespace
{

static void
write_be(uint8_t *p, uint16_t v)
{
  p[0] = static_cast<uint8_t>(v >> 8U);
  p[1] = static_cast<uint8_t>(v & 0xFFU);
}

static uint16_t
read_be(const uint8_t *p)
{
  return static_cast<uint16_t>((static_cast<uint16_t>(p[0]) << 8U) | p[1]);
}

struct StoreFixture
{
  static constexpr uint16_t NC = 16;
  static constexpr uint16_t NDI = 8;
  static constexpr uint16_t NHR = 8;
  static constexpr uint16_t NIR = 4;

  uint8_t coils_buf[(NC + 7U) / 8U] = {};
  uint8_t di_buf[(NDI + 7U) / 8U] = {};
  uint16_t hr_buf[NHR] = {};
  uint16_t ir_buf[NIR] = {};

  Store store{coils_buf, NC, di_buf, NDI, hr_buf, NHR, ir_buf, NIR};
};

struct HandlerFixture : StoreFixture
{
  Handler handler{&store};

  uint8_t req[kFrameSize] = {};
  uint8_t rsp[kFrameSize] = {};
  uint16_t rsp_len = 0;

  void
  build_read(uint8_t fc, uint16_t addr, uint16_t qty)
  {
    req[0] = 1;
    req[1] = fc;
    write_be(&req[2], addr);
    write_be(&req[4], qty);
  }

  void
  build_write_single(uint8_t fc, uint16_t addr, uint16_t value)
  {
    req[0] = 1;
    req[1] = fc;
    write_be(&req[2], addr);
    write_be(&req[4], value);
  }
};

} // anonymous namespace

// ─────────────────────────────────────────────────────────────────────────────

TEST_SUITE("modbus_handler")
{

  TEST_CASE_FIXTURE(HandlerFixture, "ReadCoils: returns packed bits")
  {
    store.set_coil(0, true);
    store.set_coil(2, true);

    build_read(FunctionCode::ReadCoils, 0, 4);
    uint8_t rc = handler.handle(req, kFrameHeaderSize + 4U, rsp, &rsp_len);

    CHECK(rc == 0U);
    CHECK(rsp_len == kFrameHeaderSize + 1U + 1U);
    CHECK(rsp[2] == 1U);              // byte count
    CHECK((rsp[3] & 0x05U) == 0x05U); // bits 0 and 2
  }

  TEST_CASE_FIXTURE(HandlerFixture, "ReadDiscreteInputs: returns packed bits")
  {
    store.set_discrete_input(1, true);

    build_read(FunctionCode::ReadDiscreteInputs, 0, 4);
    uint8_t rc = handler.handle(req, kFrameHeaderSize + 4U, rsp, &rsp_len);

    CHECK(rc == 0U);
    CHECK((rsp[3] & 0x02U) != 0U);
  }

  TEST_CASE_FIXTURE(HandlerFixture,
                    "ReadHoldingRegisters: returns big-endian words")
  {
    store.set_holding_register(0, 0x1122U);
    store.set_holding_register(1, 0x3344U);

    build_read(FunctionCode::ReadHoldingRegisters, 0, 2);
    uint8_t rc = handler.handle(req, kFrameHeaderSize + 4U, rsp, &rsp_len);

    CHECK(rc == 0U);
    CHECK(rsp_len == kFrameHeaderSize + 1U + 4U);
    CHECK(rsp[2] == 4U); // byte count = 2 regs × 2 bytes
    CHECK(read_be(&rsp[3]) == 0x1122U);
    CHECK(read_be(&rsp[5]) == 0x3344U);
  }

  TEST_CASE_FIXTURE(HandlerFixture,
                    "ReadInputRegisters: returns big-endian words")
  {
    store.set_input_register(0, 0xBEEFU);

    build_read(FunctionCode::ReadInputRegisters, 0, 1);
    uint8_t rc = handler.handle(req, kFrameHeaderSize + 4U, rsp, &rsp_len);

    CHECK(rc == 0U);
    CHECK(read_be(&rsp[3]) == 0xBEEFU);
  }

  TEST_CASE_FIXTURE(HandlerFixture, "WriteSingleCoil: 0xFF00 sets coil")
  {
    build_write_single(FunctionCode::WriteSingleCoil, 0, 0xFF00U);
    uint8_t rc = handler.handle(req, kFrameHeaderSize + 4U, rsp, &rsp_len);

    CHECK(rc == 0U);
    uint8_t out[1] = {};
    store.get_coils(0, out, 1);
    CHECK((out[0] & 0x01U) != 0U);
  }

  TEST_CASE_FIXTURE(HandlerFixture, "WriteSingleCoil: 0x0000 clears coil")
  {
    store.set_coil(0, true);
    build_write_single(FunctionCode::WriteSingleCoil, 0, 0x0000U);
    handler.handle(req, kFrameHeaderSize + 4U, rsp, &rsp_len);

    uint8_t out[1] = {};
    store.get_coils(0, out, 1);
    CHECK((out[0] & 0x01U) == 0U);
  }

  TEST_CASE_FIXTURE(HandlerFixture,
                    "WriteSingleCoil: invalid value returns IllegalDataValue")
  {
    build_write_single(FunctionCode::WriteSingleCoil, 0, 0x1234U);
    CHECK(handler.handle(req, kFrameHeaderSize + 4U, rsp, &rsp_len) ==
          ExceptionCode::IllegalDataValue);
  }

  TEST_CASE_FIXTURE(HandlerFixture, "WriteSingleRegister: stores value")
  {
    build_write_single(FunctionCode::WriteSingleRegister, 3, 0xCAFEU);
    uint8_t rc = handler.handle(req, kFrameHeaderSize + 4U, rsp, &rsp_len);

    CHECK(rc == 0U);
    uint16_t v = 0;
    store.get_holding_register(3, &v);
    CHECK(v == 0xCAFEU);
  }

  TEST_CASE_FIXTURE(HandlerFixture, "WriteMultipleCoils: sets coils from bytes")
  {
    // Write 8 coils: pattern 0b10101010 → coils 1,3,5,7 set
    req[0] = 1;
    req[1] = FunctionCode::WriteMultipleCoils;
    write_be(&req[2], 0);
    write_be(&req[4], 8);
    req[6] = 1;     // byte count
    req[7] = 0xAAU; // 0b10101010

    uint8_t rc = handler.handle(req, kFrameHeaderSize + 5U + 1U, rsp, &rsp_len);

    CHECK(rc == 0U);
    uint8_t out[1] = {};
    store.get_coils(0, out, 8);
    CHECK(out[0] == 0xAAU);
  }

  TEST_CASE_FIXTURE(HandlerFixture,
                    "WriteMultipleRegisters: stores big-endian words")
  {
    req[0] = 1;
    req[1] = FunctionCode::WriteMultipleRegisters;
    write_be(&req[2], 0);
    write_be(&req[4], 2);
    req[6] = 4; // byte count
    write_be(&req[7], 0x1111U);
    write_be(&req[9], 0x2222U);

    uint8_t rc = handler.handle(req, kFrameHeaderSize + 5U + 4U, rsp, &rsp_len);

    CHECK(rc == 0U);
    uint16_t v0 = 0, v1 = 0;
    store.get_holding_register(0, &v0);
    store.get_holding_register(1, &v1);
    CHECK(v0 == 0x1111U);
    CHECK(v1 == 0x2222U);
  }

  TEST_CASE_FIXTURE(HandlerFixture,
                    "Unknown function code returns IllegalFunction")
  {
    req[0] = 1;
    req[1] = 0x42U; // unsupported FC
    CHECK(handler.handle(req, 2U, rsp, &rsp_len) ==
          ExceptionCode::IllegalFunction);
  }

  TEST_CASE_FIXTURE(
      HandlerFixture,
      "ReadHoldingRegisters: zero quantity returns IllegalDataValue")
  {
    build_read(FunctionCode::ReadHoldingRegisters, 0, 0);
    CHECK(handler.handle(req, kFrameHeaderSize + 4U, rsp, &rsp_len) ==
          ExceptionCode::IllegalDataValue);
  }

  TEST_CASE_FIXTURE(HandlerFixture,
                    "ReadHoldingRegisters: address offset remap")
  {
    handler.set_holding_registers_offset(10);
    store.set_holding_register(0, 0xABCDU);

    // On-wire address 10 → store index 0
    build_read(FunctionCode::ReadHoldingRegisters, 10, 1);
    uint8_t rc = handler.handle(req, kFrameHeaderSize + 4U, rsp, &rsp_len);

    CHECK(rc == 0U);
    CHECK(read_be(&rsp[3]) == 0xABCDU);
  }

} // TEST_SUITE("modbus_handler")
