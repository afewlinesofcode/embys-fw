/**
 * @file hal/common/registers.hpp
 * @brief Shared SR1/SR2/DR I2C helpers (STM32F1 / STM32F4 / sim).
 *
 * Both supported families use the classic SR1/SR2/DR/CCR/TRISE layout.
 * Family backends include these helpers while retaining family-specific
 * clock, reset, and peripheral-selection logic.
 */
#pragma once

#include "../../stm32xx.hpp"

#ifdef EMBYS_I2C_CLASSIC_REGISTERS

#include <stdint.h>

#include <embys/stm32/def.hpp>

#include "../../def.hpp"

namespace Embys::Stm32::I2c
{

static constexpr uint32_t err_mask =
    I2C_SR1_BERR | I2C_SR1_ARLO | I2C_SR1_AF | I2C_SR1_OVR | I2C_SR1_TIMEOUT;

// ── register helpers (sim-hooked) ────────────────────────────────────────

inline uint32_t
read_sr1(I2C_TypeDef *i2c)
{
  uint32_t val = i2c->SR1;
#ifdef STM32_SIM
  ::Embys::Stm32::Sim::Base::trigger_test_hook("i2c_read_sr1");
#endif
  return val;
}

inline uint32_t
read_sr2(I2C_TypeDef *i2c)
{
  uint32_t val = i2c->SR2;
#ifdef STM32_SIM
  ::Embys::Stm32::Sim::Base::trigger_test_hook("i2c_read_sr2");
#endif
  return val;
}

inline uint8_t
read_dr(I2C_TypeDef *i2c)
{
  uint8_t val = static_cast<uint8_t>(i2c->DR);
#ifdef STM32_SIM
  ::Embys::Stm32::Sim::Base::trigger_test_hook("i2c_read_dr");
#endif
  return val;
}

inline void
write_dr(I2C_TypeDef *i2c, uint8_t data)
{
  i2c->DR = static_cast<uint32_t>(data);
#ifdef STM32_SIM
  ::Embys::Stm32::Sim::Base::trigger_test_hook("i2c_write_dr");
#endif
}

// ── control helpers ──────────────────────────────────────────────────────

inline void
start_condition(I2C_TypeDef *i2c)
{
  SET_BIT_V(i2c->CR1, I2C_CR1_START);
}

inline void
stop_condition(I2C_TypeDef *i2c)
{
  SET_BIT_V(i2c->CR1, I2C_CR1_STOP);
}

inline void
ack(I2C_TypeDef *i2c)
{
  SET_BIT_V(i2c->CR1, I2C_CR1_ACK);
}

inline void
nack(I2C_TypeDef *i2c)
{
  CLEAR_BIT_V(i2c->CR1, I2C_CR1_ACK);
}

inline void
pos_enable(I2C_TypeDef *i2c)
{
  SET_BIT_V(i2c->CR1, I2C_CR1_POS);
}

inline void
pos_disable(I2C_TypeDef *i2c)
{
  CLEAR_BIT_V(i2c->CR1, I2C_CR1_POS);
}

inline void
enable_buf_irq(I2C_TypeDef *i2c)
{
  SET_BIT_V(i2c->CR2, I2C_CR2_ITBUFEN);
}

inline void
disable_buf_irq(I2C_TypeDef *i2c)
{
  CLEAR_BIT_V(i2c->CR2, I2C_CR2_ITBUFEN);
}

/**
 * @brief Clear the ADDR flag by reading SR1 then SR2 (hardware requirement).
 * Triggers both sim hooks as a side effect.
 */
inline void
clear_addr(I2C_TypeDef *i2c)
{
  (void)read_sr1(i2c);
  (void)read_sr2(i2c);
}

// ── status helpers ───────────────────────────────────────────────────────

inline bool
is_sb(I2C_TypeDef *i2c)
{
  return (read_sr1(i2c) & I2C_SR1_SB) != 0;
}

inline bool
is_addr(I2C_TypeDef *i2c)
{
  return (read_sr1(i2c) & I2C_SR1_ADDR) != 0;
}

inline bool
is_txe(I2C_TypeDef *i2c)
{
  return (i2c->SR1 & I2C_SR1_TXE) != 0;
}

inline bool
is_rxne(I2C_TypeDef *i2c)
{
  return (i2c->SR1 & I2C_SR1_RXNE) != 0;
}

inline bool
is_btf(I2C_TypeDef *i2c)
{
  return (i2c->SR1 & I2C_SR1_BTF) != 0;
}

inline bool
is_busy(I2C_TypeDef *i2c)
{
  return (i2c->SR2 & I2C_SR2_BUSY) != 0;
}

inline bool
has_error(I2C_TypeDef *i2c)
{
  return (i2c->SR1 & err_mask) != 0;
}

inline bool
addr_latched(I2C_TypeDef *i2c)
{
  return (i2c->SR1 & I2C_SR1_ADDR) != 0;
}

inline void
clear_error_flags(I2C_TypeDef *i2c)
{
  i2c->SR1 = 0;
}

}; // namespace Embys::Stm32::I2c

#endif // EMBYS_I2C_CLASSIC_REGISTERS
