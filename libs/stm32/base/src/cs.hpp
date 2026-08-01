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

/**
 * @brief Stores the previous PRIMASK value
 */
extern uint32_t cs_primask;

/**
 * @brief Stores the current critical section nesting level
 */
extern uint32_t cs_stack;

class IrqGuard
{
public:
  /**
   * @brief Enter a critical section, saving the current PRIMASK and disabling
   * interrupts
   */
  IrqGuard();

  /**
   * @brief Exit a critical section, restoring the previous PRIMASK if this is
   * the outermost level
   */
  ~IrqGuard();
}; // class IrqGuard

} // namespace Embys::Stm32
