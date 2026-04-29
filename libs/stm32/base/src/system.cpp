#include "system.hpp"

#include <embys/stm32/def.hpp>

#include "stm32f1xx.hpp"

namespace Embys::Stm32::Base
{

static bool system_initialized = false;

void
system_init(bool force)
{
  if (system_initialized && !force)
    return;

  system_initialized = true;

  // Initialize DWT cycle counter for precise timing
  SET_BIT_V(CoreDebug->DEMCR, CoreDebug_DEMCR_TRCENA_Msk);
  DWT->CYCCNT = 0;
  SET_BIT_V(DWT->CTRL, DWT_CTRL_CYCCNTENA_Msk);
}

}; // namespace Embys::Stm32::Base
