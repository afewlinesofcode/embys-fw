#pragma once

#include <stdint.h>

namespace Embys::Stm32::Uart
{

enum class Parity : uint32_t
{
  None = 0,
  Even = 1,
  Odd = 2,
};

enum class StopBits : uint32_t
{
  One = 0,
  Two = 2,
};

enum class WordLength : uint32_t
{
  W8 = 0,
  W9 = 1,
};

}; // namespace Embys::Stm32::Uart
