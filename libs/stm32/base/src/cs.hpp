/**
 * @file cs.hpp
 * @author Stanislav Yaranov (stanislav.yaranov@gmail.com)
 * @brief Critical section management for STM32
 * @version 0.1
 * @date 2026-03-13
 * @copyright Copyright (c) 2026
 *
 */
#pragma once

#include <stdint.h>

namespace Embys::Stm32
{

class IrqGuard
{
public:
  /**
   * @brief Enter a critical section, saving the current PRIMASK and disabling
   * interrupts
   */
  IrqGuard();

  IrqGuard(const IrqGuard &) = delete;
  IrqGuard(IrqGuard &&) = delete;
  IrqGuard &
  operator=(const IrqGuard &) = delete;
  IrqGuard &
  operator=(IrqGuard &&) = delete;

  /**
   * @brief Exit a critical section, restoring the previous PRIMASK if this is
   * the outermost level
   */
  ~IrqGuard();

private:
  uint32_t primask;
}; // class IrqGuard

} // namespace Embys::Stm32
