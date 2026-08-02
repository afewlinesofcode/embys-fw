/**
 * @file registers.hpp
 * @author Stanislav Yaranov (stanislav.yaranov@gmail.com)
 * @brief Modbus holding/input register storage (caller-provided buffer)
 *
 * @version 0.1
 * @date 2026-04-29
 * @copyright Copyright (c) 2026
 *
 */
#pragma once

#include <span>

#include <stdint.h>

#include "diag.hpp"
#include "utils.hpp"

namespace Embys::Stm32::Modbus
{

/**
 * @brief 16-bit register storage backed by a caller-provided uint16_t array.
 *
 * Capacity is the number of 16-bit registers.
 */
class Registers
{
public:
  Registers() = delete;
  Registers(const Registers &) = delete;
  Registers(Registers &&) = delete;
  Registers &
  operator=(const Registers &) = delete;
  Registers &
  operator=(Registers &&) = delete;

  explicit Registers(std::span<uint16_t> buffer) : data(buffer)
  {
  }

  inline int
  get(uint16_t index, uint16_t &value) const
  {
    if (index >= data.size())
    {
      return Diag::REGISTER_OUT_OF_RANGE;
    }

    value = data[index];
    return 0;
  }

  inline int
  get(uint16_t index, std::span<uint16_t> value) const
  {
    if (static_cast<std::size_t>(index) + value.size() > data.size())
    {
      return Diag::REGISTER_OUT_OF_RANGE;
    }

    for (std::size_t i = 0; i < value.size(); i++)
    {
      value[i] = data[index + i];
    }

    return 0;
  }

  inline int
  get_be(uint16_t index, std::span<uint8_t> value) const
  {
    if ((value.size() % 2U) != 0U)
    {
      return Diag::INVALID_BUFFER_SIZE;
    }

    const std::size_t quantity = value.size() / 2U;
    if (static_cast<std::size_t>(index) + quantity > data.size())
    {
      return Diag::REGISTER_OUT_OF_RANGE;
    }

    for (std::size_t i = 0; i < quantity; i++)
    {
      write_u16_be(value.subspan(i * 2U, 2U), data[index + i]);
    }

    return 0;
  }

  inline int
  set(uint16_t index, uint16_t value)
  {
    if (index >= data.size())
    {
      return Diag::REGISTER_OUT_OF_RANGE;
    }

    data[index] = value;
    return 0;
  }

  inline int
  set(uint16_t index, std::span<const uint16_t> value)
  {
    if (static_cast<std::size_t>(index) + value.size() > data.size())
    {
      return Diag::REGISTER_OUT_OF_RANGE;
    }

    for (std::size_t i = 0; i < value.size(); i++)
    {
      data[index + i] = value[i];
    }

    return 0;
  }

  inline int
  set_be(uint16_t index, std::span<const uint8_t> value)
  {
    if ((value.size() % 2U) != 0U)
    {
      return Diag::INVALID_BUFFER_SIZE;
    }

    const std::size_t quantity = value.size() / 2U;
    if (static_cast<std::size_t>(index) + quantity > data.size())
    {
      return Diag::REGISTER_OUT_OF_RANGE;
    }

    for (std::size_t i = 0; i < quantity; i++)
    {
      data[index + i] = read_u16_be(value.subspan(i * 2U, 2U));
    }

    return 0;
  }

private:
  std::span<uint16_t> data;
};

}; // namespace Embys::Stm32::Modbus
