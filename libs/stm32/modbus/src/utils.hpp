/**
 * @file utils.hpp
 * @author Stanislav Yaranov (stanislav.yaranov@gmail.com)
 * @brief Modbus utility functions: coil byte calculation and big-endian I/O
 *
 * @version 0.1
 * @date 2026-04-29
 * @copyright Copyright (c) 2026
 *
 */
#pragma once

#include <span>

#include <stdint.h>

namespace Embys::Stm32::Modbus
{

inline uint8_t
calculate_coil_bytes(uint16_t coil_count)
{
  return static_cast<uint8_t>((coil_count + 7U) >> 3U);
}

inline uint16_t
read_u16_be(std::span<const uint8_t> bytes)
{
  return static_cast<uint16_t>((static_cast<uint16_t>(bytes[0]) << 8U) |
                               static_cast<uint16_t>(bytes[1]));
}

inline void
write_u16_be(std::span<uint8_t> bytes, uint16_t value)
{
  bytes[0] = static_cast<uint8_t>((value >> 8U) & 0xFFU);
  bytes[1] = static_cast<uint8_t>(value & 0xFFU);
}

}; // namespace Embys::Stm32::Modbus
