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

template <>
inline void
reset<Stm32f7>() noexcept
{
  // Enable instruction and data caches on cores that have them (F7, H7).
  // SCB_EnableICache/SCB_EnableDCache are only defined in core_cm7.h, so
  // guard with preprocessor to avoid undefined-symbol errors on other targets.
#if defined(__DCACHE_PRESENT) && (__DCACHE_PRESENT == 1)
  // TODO: explore what to do before this
  SCB_InvalidateICache();
  SCB_EnableICache();
  SCB_EnableDCache();
#endif

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
