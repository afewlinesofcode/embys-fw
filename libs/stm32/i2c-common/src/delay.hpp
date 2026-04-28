#pragma once

#include <stdint.h>

#include <embys/stm32/base/event.hpp>
#include <embys/stm32/base/loop.hpp>

#include "def.hpp"

namespace Embys::Stm32::I2c::Dev
{

class Delay
{
public:
  explicit Delay(Base::Loop *loop);

  void
  exec(uint32_t us, Cb cb);

private:
  Base::Event ev;
  Cb cb;

  static void
  fired(void *ctx);
};

}; // namespace Embys::Stm32::I2c::Dev
