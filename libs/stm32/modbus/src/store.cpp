#include "store.hpp"

namespace Embys::Stm32::Modbus
{

int
StoreCore::get_coils(uint16_t address, uint8_t *value, uint16_t quantity) const
{
  return coils.get(address, value, quantity);
}

int
StoreCore::get_discrete_inputs(uint16_t address, uint8_t *value,
                           uint16_t quantity) const
{
  return discrete_inputs.get(address, value, quantity);
}

int
StoreCore::get_holding_register(uint16_t address, uint16_t *value) const
{
  return holding_registers.get(address, value);
}

int
StoreCore::get_holding_registers(uint16_t address, uint16_t *value,
                             uint16_t quantity) const
{
  return holding_registers.get(address, value, quantity);
}

int
StoreCore::get_holding_registers_be(uint16_t address, uint8_t *value,
                                uint16_t quantity) const
{
  return holding_registers.get_be(address, value, quantity);
}

int
StoreCore::get_input_registers_be(uint16_t address, uint8_t *value,
                              uint16_t quantity) const
{
  return input_registers.get_be(address, value, quantity);
}

int
StoreCore::set_coil(uint16_t address, bool value)
{
  return coils.set(address, value);
}

int
StoreCore::set_coils(uint16_t address, const uint8_t *value, uint16_t quantity)
{
  return coils.set(address, value, quantity);
}

int
StoreCore::set_discrete_input(uint16_t address, bool value)
{
  return discrete_inputs.set(address, value);
}

int
StoreCore::set_discrete_inputs(uint16_t address, const uint8_t *value,
                           uint16_t quantity)
{
  return discrete_inputs.set(address, value, quantity);
}

int
StoreCore::set_holding_register(uint16_t address, uint16_t value)
{
  return holding_registers.set(address, value);
}

int
StoreCore::set_holding_registers(uint16_t address, const uint16_t *value,
                             uint16_t quantity)
{
  return holding_registers.set(address, value, quantity);
}

int
StoreCore::set_holding_registers_be(uint16_t address, const uint8_t *value,
                                uint16_t quantity)
{
  return holding_registers.set_be(address, value, quantity);
}

int
StoreCore::set_input_register(uint16_t address, uint16_t value)
{
  return input_registers.set(address, value);
}

int
StoreCore::set_input_registers(uint16_t address, const uint16_t *value,
                           uint16_t quantity)
{
  return input_registers.set(address, value, quantity);
}

}; // namespace Embys::Stm32::Modbus
