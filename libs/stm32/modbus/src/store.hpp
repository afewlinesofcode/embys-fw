/**
 * @file store.hpp
 * @author Stanislav Yaranov (stanislav.yaranov@gmail.com)
 * @brief Modbus data store: coils, discrete inputs, holding and input registers
 *
 * Store aggregates and owns the four Modbus data models. Capacities are part
 * of the type, so no heap allocation or caller-managed backing storage is
 * required.
 *
 * Example:
 * ```
 * Modbus::Store<64, 32, 16, 8> store;
 * ```
 *
 * @version 0.1
 * @date 2026-04-29
 * @copyright Copyright (c) 2026
 *
 */
#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include "coils.hpp"
#include "registers.hpp"

namespace Embys::Stm32::Modbus
{

class StoreCore
{
public:
  StoreCore(const StoreCore &) = delete;
  StoreCore(StoreCore &&) = delete;
  StoreCore &
  operator=(const StoreCore &) = delete;
  StoreCore &
  operator=(StoreCore &&) = delete;

protected:
  StoreCore(uint8_t *coils_buf, uint16_t coils_capacity, uint8_t *di_buf,
            uint16_t di_capacity, uint16_t *hr_buf, uint16_t hr_capacity,
            uint16_t *ir_buf, uint16_t ir_capacity)
    : coils(coils_buf, coils_capacity), discrete_inputs(di_buf, di_capacity),
      holding_registers(hr_buf, hr_capacity),
      input_registers(ir_buf, ir_capacity)
  {
  }

public:

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

namespace Detail
{

template <std::size_t CoilsN, std::size_t DiscreteN, std::size_t HoldingN,
          std::size_t InputN>
struct StoreStorage
{
  std::array<uint8_t, (CoilsN + 7U) / 8U> coils{};
  std::array<uint8_t, (DiscreteN + 7U) / 8U> discrete_inputs{};
  std::array<uint16_t, HoldingN> holding_registers{};
  std::array<uint16_t, InputN> input_registers{};
};

} // namespace Detail

/**
 * @brief Fixed-capacity, allocation-free Modbus data store.
 *
 * Capacities count logical bits for coils/discrete inputs and 16-bit values
 * for holding/input registers.
 */
template <std::size_t CoilsN, std::size_t DiscreteN, std::size_t HoldingN,
          std::size_t InputN>
class Store final
  : private Detail::StoreStorage<CoilsN, DiscreteN, HoldingN, InputN>,
    public StoreCore
{
  using Storage = Detail::StoreStorage<CoilsN, DiscreteN, HoldingN, InputN>;

  static_assert(CoilsN > 0U && DiscreteN > 0U && HoldingN > 0U && InputN > 0U,
                "Modbus store capacities must be non-zero");
  static_assert(CoilsN <= UINT16_MAX && DiscreteN <= UINT16_MAX &&
                    HoldingN <= UINT16_MAX && InputN <= UINT16_MAX,
                "Modbus store capacities must fit in uint16_t");

public:
  Store()
    : Storage{},
      StoreCore(Storage::coils.data(), static_cast<uint16_t>(CoilsN),
                Storage::discrete_inputs.data(),
                static_cast<uint16_t>(DiscreteN),
                Storage::holding_registers.data(),
                static_cast<uint16_t>(HoldingN),
                Storage::input_registers.data(), static_cast<uint16_t>(InputN))
  {
  }
};

}; // namespace Embys::Stm32::Modbus
