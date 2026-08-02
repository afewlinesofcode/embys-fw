/**
 * @file system.hpp
 * @author Stanislav Yaranov (stanislav.yaranov@gmail.com)
 * @brief STM32 system initialization
 *
 * @version 0.1
 * @date 2026-04-29
 * @copyright Copyright (c) 2026
 *
 */
#pragma once

#include <embys/stm32/def.hpp>

#include "stm32xx.hpp"

namespace Embys::Stm32::Base
{

void
reset_dwt() noexcept;

template <typename Family>
void
reset() noexcept
{
  reset_dwt();
}

inline void
reset_dwt() noexcept
{
  // Initialize DWT cycle counter for precise timing.
  SET_BIT_V(CoreDebug->DEMCR, CoreDebug_DEMCR_TRCENA_Msk);
  DWT->CYCCNT = 0;
  SET_BIT_V(DWT->CTRL, DWT_CTRL_CYCCNTENA_Msk);
}

}; // namespace Embys::Stm32::Base
