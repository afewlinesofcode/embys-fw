/**
 * @file hal/v2/hal.hpp
 * @brief I2C V2 register-level helpers (STM32F7 / STM32H7).
 *
 * Provides inline register accessors and control helpers for the V2 I2C
 * peripheral (ISR/ICR/RXDR/TXDR/TIMINGR/CR2 layout).  Included by src/hal.hpp
 * when I2C_HAL_V2 is defined; also included directly by the family-specific
 * HAL implementation files (hal/f7/hal.cpp, hal/h7/hal.cpp).
 */
#pragma once

#include "../../stm32xx.hpp"

#ifdef I2C_HAL_V2

#include <stdint.h>

#include <embys/stm32/def.hpp>

#include "../../def.hpp"

namespace Embys::Stm32::I2c
{

static constexpr uint32_t err_mask = I2C_ISR_BERR | I2C_ISR_ARLO | I2C_ISR_OVR;

// ── register helpers ─────────────────────────────────────────────────────

inline uint8_t
read_rxdr(I2C_TypeDef *i2c)
{
  return static_cast<uint8_t>(i2c->RXDR);
}

inline void
write_txdr(I2C_TypeDef *i2c, uint8_t data)
{
  i2c->TXDR = static_cast<uint32_t>(data);
}

// ── status helpers ───────────────────────────────────────────────────────

inline bool
is_busy(I2C_TypeDef *i2c)
{
  return (i2c->ISR & I2C_ISR_BUSY) != 0;
}

inline bool
has_error(I2C_TypeDef *i2c)
{
  return (i2c->ISR & err_mask) != 0;
}

inline bool
is_txis(I2C_TypeDef *i2c)
{
  return (i2c->ISR & I2C_ISR_TXIS) != 0;
}

inline bool
is_rxne(I2C_TypeDef *i2c)
{
  return (i2c->ISR & I2C_ISR_RXNE) != 0;
}

inline bool
is_tc(I2C_TypeDef *i2c)
{
  return (i2c->ISR & I2C_ISR_TC) != 0;
}

inline bool
is_stopf(I2C_TypeDef *i2c)
{
  return (i2c->ISR & I2C_ISR_STOPF) != 0;
}

inline bool
is_nackf(I2C_TypeDef *i2c)
{
  return (i2c->ISR & I2C_ISR_NACKF) != 0;
}

// ── flag-clear helpers ───────────────────────────────────────────────────

inline void
clear_stopf(I2C_TypeDef *i2c)
{
  SET_BIT_V(i2c->ICR, I2C_ICR_STOPCF);
}

inline void
clear_nackf(I2C_TypeDef *i2c)
{
  SET_BIT_V(i2c->ICR, I2C_ICR_NACKCF);
}

inline void
clear_error_flags(I2C_TypeDef *i2c)
{
  SET_BIT_V(i2c->ICR, I2C_ICR_BERRCF | I2C_ICR_ARLOCF | I2C_ICR_OVRCF);
}

// ── transfer initiator ───────────────────────────────────────────────────

inline void
set_cr2_transfer(I2C_TypeDef *i2c, uint8_t addr7, uint8_t nbytes, bool read,
                 bool autoend, bool start_cond)
{
  uint32_t cr2 =
      (static_cast<uint32_t>(addr7) << 1U) | (read ? I2C_CR2_RD_WRN : 0U) |
      (static_cast<uint32_t>(nbytes) << I2C_CR2_NBYTES_Pos) |
      (autoend ? I2C_CR2_AUTOEND : 0U) | (start_cond ? I2C_CR2_START : 0U);
  i2c->CR2 = cr2;
}

}; // namespace Embys::Stm32::I2c

#endif // I2C_HAL_V2
