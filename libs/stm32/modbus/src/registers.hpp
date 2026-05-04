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

  Registers(uint16_t *buffer, uint16_t capacity)
    : data(buffer), capacity(capacity)
  {
  }

  inline int
  get(uint16_t index, uint16_t *value) const
  {
    if (index >= capacity)
    {
      return Diag::REGISTER_OUT_OF_RANGE;
    }

    *value = data[index];
    return 0;
  }

  inline int
  get_be(uint16_t index, uint8_t *value) const
  {
    if (index >= capacity)
    {
      return Diag::REGISTER_OUT_OF_RANGE;
    }

    write_u16_be(value, data[index]);
    return 0;
  }

  inline int
  get(uint16_t index, uint16_t *value, uint16_t quantity) const
  {
    if (static_cast<uint32_t>(index) + quantity > capacity)
    {
      return Diag::REGISTER_OUT_OF_RANGE;
    }

    for (uint16_t i = 0; i < quantity; i++)
    {
      value[i] = data[index + i];
    }

    return 0;
  }

  inline int
  get_be(uint16_t index, uint8_t *value, uint16_t quantity) const
  {
    if (static_cast<uint32_t>(index) + quantity > capacity)
    {
      return Diag::REGISTER_OUT_OF_RANGE;
    }

    for (uint16_t i = 0; i < quantity; i++)
    {
      write_u16_be(&value[i * 2U], data[index + i]);
    }

    return 0;
  }

  inline int
  set(uint16_t index, uint16_t value)
  {
    if (index >= capacity)
    {
      return Diag::REGISTER_OUT_OF_RANGE;
    }

    data[index] = value;
    return 0;
  }

  inline int
  set(uint16_t index, const uint16_t *value, uint16_t quantity)
  {
    if (static_cast<uint32_t>(index) + quantity > capacity)
    {
      return Diag::REGISTER_OUT_OF_RANGE;
    }

    for (uint16_t i = 0; i < quantity; i++)
    {
      data[index + i] = value[i];
    }

    return 0;
  }

  inline int
  set_be(uint16_t index, const uint8_t *value, uint16_t quantity)
  {
    if (static_cast<uint32_t>(index) + quantity > capacity)
    {
      return Diag::REGISTER_OUT_OF_RANGE;
    }

    for (uint16_t i = 0; i < quantity; i++)
    {
      data[index + i] = read_u16_be(&value[i * 2U]);
    }

    return 0;
  }

private:
  uint16_t *data;
  uint16_t capacity;
};

}; // namespace Embys::Stm32::Modbus
