#include "system.hpp"

#include <embys/stm32/def.hpp>
#include <embys/stm32/mcu_traits.hpp>

#include "stm32xx.hpp"

namespace Embys::Stm32::Base
{

using T = McuTraits;

static bool system_initialized = false;

void
system_init(bool force)
{
  if (system_initialized && !force)
    return;

  system_initialized = true;

  // Enable instruction and data caches on cores that have them (F7, H7).
  // SCB_EnableICache/SCB_EnableDCache are only defined in core_cm7.h, so
  // guard with preprocessor to avoid undefined-symbol errors on other targets.
#if defined(__DCACHE_PRESENT) && (__DCACHE_PRESENT == 1)
  if constexpr (T::has_instruction_cache)
    SCB_EnableICache();

  if constexpr (T::has_data_cache)
    SCB_EnableDCache();
#endif

  // Initialize DWT cycle counter for precise timing.
  SET_BIT_V(CoreDebug->DEMCR, CoreDebug_DEMCR_TRCENA_Msk);
  DWT->CYCCNT = 0;
  SET_BIT_V(DWT->CTRL, DWT_CTRL_CYCCNTENA_Msk);
}

}; // namespace Embys::Stm32::Base
