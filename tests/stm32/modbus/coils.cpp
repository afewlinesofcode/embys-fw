#include <embys/stm32/modbus/coils.hpp>
#include <embys/stm32/modbus/diag.hpp>

#include "test.hpp"

using namespace Embys::Stm32::Modbus;

namespace
{

struct CoilsFixture
{
  static constexpr uint16_t N = 16;
  uint8_t buf[(N + 7U) / 8U] = {};
  Coils coils{buf, N};
};

} // anonymous namespace

// ─────────────────────────────────────────────────────────────────────────────

TEST_SUITE("modbus_coils")
{

  TEST_CASE_FIXTURE(CoilsFixture, "set and get single coil")
  {
    CHECK(coils.set(0, true) == 0);
    bool v = false;
    CHECK(coils.get(0, &v) == 0);
    CHECK(v == true);
  }

  TEST_CASE_FIXTURE(CoilsFixture, "coil 0 set does not affect coil 1")
  {
    coils.set(0, true);
    bool v = false;
    coils.get(1, &v);
    CHECK(v == false);
  }

  TEST_CASE_FIXTURE(CoilsFixture, "set false clears a previously set coil")
  {
    coils.set(3, true);
    coils.set(3, false);
    bool v = true;
    coils.get(3, &v);
    CHECK(v == false);
  }

  TEST_CASE_FIXTURE(CoilsFixture,
                    "set/get batch: 10 coils packed into two bytes correctly")
  {
    // Set alternating coils 0,2,4,6,8
    for (uint16_t i = 0; i < 10; i += 2)
    {
      coils.set(i, true);
    }

    uint8_t out[2] = {};
    CHECK(coils.get(0, out, 10) == 0);

    // Bits 0,2,4,6,8 set → byte0=0x55, byte1 bit 0 = 1 → byte1=0x01
    CHECK(out[0] == 0x55);
    CHECK(out[1] == 0x01);
  }

  TEST_CASE_FIXTURE(CoilsFixture, "batch set from byte array")
  {
    // 0b00000101 → coils 0 and 2 set
    const uint8_t src[] = {0x05};
    CHECK(coils.set(0, src, 3) == 0);

    bool v0 = false, v1 = true, v2 = false;
    coils.get(0, &v0);
    coils.get(1, &v1);
    coils.get(2, &v2);
    CHECK(v0 == true);
    CHECK(v1 == false);
    CHECK(v2 == true);
  }

  TEST_CASE_FIXTURE(CoilsFixture, "out-of-range single get returns error")
  {
    bool v = false;
    CHECK(coils.get(N, &v) == Diag::COIL_OUT_OF_RANGE);
  }

  TEST_CASE_FIXTURE(CoilsFixture, "out-of-range batch get returns error")
  {
    uint8_t out[4] = {};
    CHECK(coils.get(N - 1U, out, 2) == Diag::COIL_OUT_OF_RANGE);
  }

} // TEST_SUITE("modbus_coils")
