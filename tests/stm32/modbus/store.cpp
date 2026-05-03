#include <embys/stm32/modbus/store.hpp>

#include "test.hpp"

using namespace Embys::Stm32::Modbus;

namespace
{

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

} // anonymous namespace

// ─────────────────────────────────────────────────────────────────────────────

TEST_SUITE("modbus_store")
{

  TEST_CASE_FIXTURE(StoreFixture, "coil round-trip via store")
  {
    CHECK(store.set_coil(0, true) == 0);
    uint8_t out[1] = {};
    CHECK(store.get_coils(0, out, 1) == 0);
    CHECK((out[0] & 0x01U) != 0U);
  }

  TEST_CASE_FIXTURE(StoreFixture, "discrete input round-trip via store")
  {
    CHECK(store.set_discrete_input(3, true) == 0);
    uint8_t out[1] = {};
    CHECK(store.get_discrete_inputs(3, out, 1) == 0);
    CHECK((out[0] & 0x01U) != 0U);
  }

  TEST_CASE_FIXTURE(StoreFixture, "holding register round-trip via store")
  {
    CHECK(store.set_holding_register(2, 0x9876U) == 0);
    uint16_t v = 0;
    CHECK(store.get_holding_register(2, &v) == 0);
    CHECK(v == 0x9876U);
  }

  TEST_CASE_FIXTURE(StoreFixture, "input register round-trip via store")
  {
    CHECK(store.set_input_register(1, 0x5A5AU) == 0);
    uint8_t be[2] = {};
    CHECK(store.get_input_registers_be(1, be, 1) == 0);
    CHECK(be[0] == 0x5AU);
    CHECK(be[1] == 0x5AU);
  }

  TEST_CASE_FIXTURE(StoreFixture, "holding registers set batch, get big-endian")
  {
    const uint16_t src[] = {0x0102U, 0x0304U};
    CHECK(store.set_holding_registers(0, src, 2) == 0);
    uint8_t be[4] = {};
    CHECK(store.get_holding_registers_be(0, be, 2) == 0);
    CHECK(be[0] == 0x01U);
    CHECK(be[1] == 0x02U);
    CHECK(be[2] == 0x03U);
    CHECK(be[3] == 0x04U);
  }

} // TEST_SUITE("modbus_store")
