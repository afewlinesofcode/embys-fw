#include <embys/stm32/i2c-aht20/device.hpp>

#include "test.hpp"

using namespace Embys::Stm32::I2c::Dev::Aht20;

TEST_SUITE("aht20")
{
  TEST_CASE("Raw measurements convert to fixed-point engineering units")
  {
    CHECK(humidity_from_raw(0x00000U).centi_percent == 0U);
    CHECK(humidity_from_raw(0x80000U).centi_percent == 5000U);
    CHECK(humidity_from_raw(0xFFFFFU).centi_percent == 10000U);

    CHECK(temperature_from_raw(0x00000U).centi_celsius == -5000);
    CHECK(temperature_from_raw(0x60000U).centi_celsius == 2500);
    CHECK(temperature_from_raw(0x80000U).centi_celsius == 5000);
    CHECK(temperature_from_raw(0xFFFFFU).centi_celsius == 15000);
  }
}
