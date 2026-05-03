#include <embys/stm32/modbus/diag.hpp>
#include <embys/stm32/modbus/registers.hpp>

#include "test.hpp"

using namespace Embys::Stm32::Modbus;

namespace
{

struct RegistersFixture
{
  static constexpr uint16_t N = 8;
  uint16_t buf[N] = {};
  Registers regs{buf, N};
};

} // anonymous namespace

// ─────────────────────────────────────────────────────────────────────────────

TEST_SUITE("modbus_registers")
{

  TEST_CASE_FIXTURE(RegistersFixture, "set and get single register")
  {
    CHECK(regs.set(0, 0xABCDU) == 0);
    uint16_t v = 0;
    CHECK(regs.get(0, &v) == 0);
    CHECK(v == 0xABCDU);
  }

  TEST_CASE_FIXTURE(RegistersFixture, "get_be returns big-endian bytes")
  {
    regs.set(1, 0x1234U);
    uint8_t be[2] = {};
    CHECK(regs.get_be(1, be) == 0);
    CHECK(be[0] == 0x12U);
    CHECK(be[1] == 0x34U);
  }

  TEST_CASE_FIXTURE(RegistersFixture, "batch get returns values in order")
  {
    regs.set(0, 0x0001U);
    regs.set(1, 0x0002U);
    regs.set(2, 0x0003U);
    uint16_t out[3] = {};
    CHECK(regs.get(0, out, 3) == 0);
    CHECK(out[0] == 0x0001U);
    CHECK(out[1] == 0x0002U);
    CHECK(out[2] == 0x0003U);
  }

  TEST_CASE_FIXTURE(RegistersFixture,
                    "batch get_be returns big-endian words in order")
  {
    regs.set(0, 0xABCDU);
    regs.set(1, 0xEF01U);
    uint8_t be[4] = {};
    CHECK(regs.get_be(0, be, 2) == 0);
    CHECK(be[0] == 0xABU);
    CHECK(be[1] == 0xCDU);
    CHECK(be[2] == 0xEFU);
    CHECK(be[3] == 0x01U);
  }

  TEST_CASE_FIXTURE(RegistersFixture, "set_be writes big-endian bytes")
  {
    const uint8_t src[] = {0x12U, 0x34U, 0x56U, 0x78U};
    CHECK(regs.set_be(0, src, 2) == 0);
    uint16_t v0 = 0, v1 = 0;
    regs.get(0, &v0);
    regs.get(1, &v1);
    CHECK(v0 == 0x1234U);
    CHECK(v1 == 0x5678U);
  }

  TEST_CASE_FIXTURE(RegistersFixture, "out-of-range get returns error")
  {
    uint16_t v = 0;
    CHECK(regs.get(N, &v) == Diag::REGISTER_OUT_OF_RANGE);
  }

  TEST_CASE_FIXTURE(RegistersFixture, "out-of-range batch set returns error")
  {
    const uint16_t src[] = {1, 2};
    CHECK(regs.set(N - 1U, src, 2) == Diag::REGISTER_OUT_OF_RANGE);
  }

} // TEST_SUITE("modbus_registers")
