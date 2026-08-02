#pragma once

#include <cstdint>

namespace Embys::Stm32::Register
{

template <typename Register, typename Mask>
inline void
set(volatile Register &reg, Mask mask) noexcept
{
  reg = static_cast<Register>(reg | static_cast<Register>(mask));
}

template <typename Register, typename Mask>
inline void
clear(volatile Register &reg, Mask mask) noexcept
{
  reg = static_cast<Register>(reg & ~static_cast<Register>(mask));
}

template <typename Register, typename Value>
inline void
modify(volatile Register &reg, std::uint8_t shift, Register mask,
       Value value) noexcept
{
  reg = static_cast<Register>((reg & ~(mask << shift)) |
                              ((static_cast<Register>(value) & mask) << shift));
}

template <typename Register, typename Mask>
[[nodiscard]] inline bool
is_set(volatile const Register &reg, Mask mask) noexcept
{
  return (reg & static_cast<Register>(mask)) != 0;
}

} // namespace Embys::Stm32::Register
