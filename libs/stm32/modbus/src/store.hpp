/**
 * @file store.hpp
 * @author Stanislav Yaranov (stanislav.yaranov@gmail.com)
 * @brief Modbus data store: coils, discrete inputs, holding and input registers
 *
 * The Store aggregates the four Modbus data models.  All backing memory is
 * provided by the caller, so no heap allocation occurs.
 *
 * Example:
 * ```
 * static uint8_t  coils_buf[8];          // 64 coils
 * static uint8_t  di_buf[4];             // 32 discrete inputs
 * static uint16_t hr_buf[16];            // 16 holding registers
 * static uint16_t ir_buf[8];             // 8 input registers
 *
 * Modbus::Store store(coils_buf, 64, di_buf, 32, hr_buf, 16, ir_buf, 8);
 * ```
 *
 * @version 0.1
 * @date 2026-04-29
 * @copyright Copyright (c) 2026
 *
 */
#pragma once

#include <stdint.h>

#include "coils.hpp"
#include "registers.hpp"

namespace Embys::Stm32::Modbus
{

class Store
{
public:
  Store() = delete;
  Store(const Store &) = delete;
  Store(Store &&) = delete;
  Store &
  operator=(const Store &) = delete;
  Store &
  operator=(Store &&) = delete;

  /**
   * @brief Construct a Modbus data store with caller-provided buffers.
   *
   * @param coils_buf         Byte array for coil bits (capacity in coils).
   * @param coils_capacity    Number of coils backed by coils_buf.
   * @param di_buf            Byte array for discrete-input bits.
   * @param di_capacity       Number of discrete inputs.
   * @param hr_buf            uint16_t array for holding registers.
   * @param hr_capacity       Number of holding registers.
   * @param ir_buf            uint16_t array for input registers.
   * @param ir_capacity       Number of input registers.
   */
  Store(uint8_t *coils_buf, uint16_t coils_capacity, uint8_t *di_buf,
        uint16_t di_capacity, uint16_t *hr_buf, uint16_t hr_capacity,
        uint16_t *ir_buf, uint16_t ir_capacity)
    : coils(coils_buf, coils_capacity), discrete_inputs(di_buf, di_capacity),
      holding_registers(hr_buf, hr_capacity),
      input_registers(ir_buf, ir_capacity)
  {
  }

  int
  get_coils(uint16_t address, uint8_t *value, uint16_t quantity) const;

  int
  get_discrete_inputs(uint16_t address, uint8_t *value,
                      uint16_t quantity) const;

  int
  get_holding_register(uint16_t address, uint16_t *value) const;

  int
  get_holding_registers(uint16_t address, uint16_t *value,
                        uint16_t quantity) const;

  int
  get_holding_registers_be(uint16_t address, uint8_t *value,
                           uint16_t quantity) const;

  int
  get_input_registers_be(uint16_t address, uint8_t *value,
                         uint16_t quantity) const;

  int
  set_coil(uint16_t address, bool value);

  int
  set_coils(uint16_t address, const uint8_t *value, uint16_t quantity);

  int
  set_discrete_input(uint16_t address, bool value);

  int
  set_discrete_inputs(uint16_t address, const uint8_t *value,
                      uint16_t quantity);

  int
  set_holding_register(uint16_t address, uint16_t value);

  int
  set_holding_registers(uint16_t address, const uint16_t *value,
                        uint16_t quantity);

  int
  set_holding_registers_be(uint16_t address, const uint8_t *value,
                           uint16_t quantity);

  int
  set_input_register(uint16_t address, uint16_t value);

  int
  set_input_registers(uint16_t address, const uint16_t *value,
                      uint16_t quantity);

private:
  Coils coils;
  Coils discrete_inputs;
  Registers holding_registers;
  Registers input_registers;
};

}; // namespace Embys::Stm32::Modbus
