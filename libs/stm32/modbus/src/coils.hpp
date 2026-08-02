/**
 * @file coils.hpp
 * @author Stanislav Yaranov (stanislav.yaranov@gmail.com)
 * @brief Modbus coil and discrete-input storage (bit-packed, caller-provided
 * buffer)
 *
 * @version 0.1
 * @date 2026-04-29
 * @copyright Copyright (c) 2026
 *
 */
#pragma once

#include <algorithm>
#include <span>

#include <stdint.h>

#include "diag.hpp"

namespace Embys::Stm32::Modbus
{

/**
 * @brief Bit-packed coil / discrete-input storage backed by a caller-provided
 * byte array.
 *
 * Capacity is expressed in number of coils (bits).  The caller must supply a
 * buffer of at least @c (capacity + 7) / 8 bytes.
 */
class Coils
{
public:
  Coils() = delete;
  Coils(const Coils &) = delete;
  Coils(Coils &&) = delete;
  Coils &
  operator=(const Coils &) = delete;
  Coils &
  operator=(Coils &&) = delete;

  Coils(std::span<uint8_t> buffer, uint16_t capacity)
    : data(buffer), capacity(capacity)
  {
  }

  inline int
  get(uint16_t index, bool &value) const
  {
    if (index >= capacity)
    {
      return Diag::COIL_OUT_OF_RANGE;
    }

    uint16_t byte_index = index / 8U;
    uint8_t bit_index = static_cast<uint8_t>(index % 8U);
    if (byte_index >= data.size())
    {
      return Diag::BUFFER_TOO_SMALL;
    }

    value = ((data[byte_index] >> bit_index) & 0x01U) != 0U;
    return 0;
  }

  inline int
  get(uint16_t index, std::span<uint8_t> value, uint16_t quantity) const
  {
    if (static_cast<uint32_t>(index) + quantity > capacity)
    {
      return Diag::COIL_OUT_OF_RANGE;
    }

    const auto bytes = static_cast<std::size_t>((quantity + 7U) / 8U);
    if (value.size() < bytes)
    {
      return Diag::BUFFER_TOO_SMALL;
    }

    if (quantity != 0U &&
        static_cast<std::size_t>(index + quantity - 1U) / 8U >= data.size())
    {
      return Diag::BUFFER_TOO_SMALL;
    }

    std::fill_n(value.begin(), bytes, uint8_t{0});

    for (uint16_t i = 0; i < quantity; i++)
    {
      uint16_t coil_index = static_cast<uint16_t>(index + i);
      uint16_t byte_index = coil_index / 8U;
      uint8_t bit_index = static_cast<uint8_t>(coil_index % 8U);
      value[i / 8U] |= static_cast<uint8_t>(
          ((data[byte_index] >> bit_index) & 0x01U) << (i % 8U));
    }

    uint8_t bits_count = static_cast<uint8_t>(quantity % 8U);
    if (bits_count != 0U)
    {
      value[quantity / 8U] &= static_cast<uint8_t>((1U << bits_count) - 1U);
    }

    return 0;
  }

  inline int
  set(uint16_t index, bool value)
  {
    if (index >= capacity)
    {
      return Diag::COIL_OUT_OF_RANGE;
    }

    uint16_t byte_index = index / 8U;
    uint8_t bit_index = static_cast<uint8_t>(index % 8U);
    if (byte_index >= data.size())
    {
      return Diag::BUFFER_TOO_SMALL;
    }

    if (value)
    {
      data[byte_index] |= static_cast<uint8_t>(1U << bit_index);
    }
    else
    {
      data[byte_index] &= static_cast<uint8_t>(~(1U << bit_index));
    }

    return 0;
  }

  inline int
  set(uint16_t index, std::span<const uint8_t> value, uint16_t quantity)
  {
    if (static_cast<uint32_t>(index) + quantity > capacity)
    {
      return Diag::COIL_OUT_OF_RANGE;
    }

    if (value.size() < static_cast<std::size_t>((quantity + 7U) / 8U))
    {
      return Diag::BUFFER_TOO_SMALL;
    }

    if (quantity != 0U &&
        static_cast<std::size_t>(index + quantity - 1U) / 8U >= data.size())
    {
      return Diag::BUFFER_TOO_SMALL;
    }

    for (uint16_t i = 0; i < quantity; i++)
    {
      uint16_t coil_index = static_cast<uint16_t>(index + i);
      uint16_t byte_index = coil_index / 8U;
      uint8_t bit_index = static_cast<uint8_t>(coil_index % 8U);
      bool bit_value = ((value[i / 8U] >> (i % 8U)) & 0x01U) != 0U;

      if (bit_value)
      {
        data[byte_index] |= static_cast<uint8_t>(1U << bit_index);
      }
      else
      {
        data[byte_index] &= static_cast<uint8_t>(~(1U << bit_index));
      }
    }

    return 0;
  }

private:
  std::span<uint8_t> data;
  uint16_t capacity;
};

}; // namespace Embys::Stm32::Modbus
