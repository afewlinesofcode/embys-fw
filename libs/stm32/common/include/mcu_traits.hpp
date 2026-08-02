/**
 * @file mcu_traits.hpp
 * @author Stanislav Yaranov (stanislav.yaranov@gmail.com)
 * @brief Compatibility alias for the build-selected exact-device traits.
 *
 * New code may use DeviceTraits<TargetDevice> directly. This alias keeps
 * capability checks concise without introducing family preprocessor guards.
 *
 * Usage:
 * ```cpp
 * #include <embys/stm32/mcu_traits.hpp>
 * using T = Embys::Stm32::McuTraits;
 * if constexpr (T::has_data_cache) SCB_EnableDCache();
 * ```
 *
 * @version 0.1
 * @date 2026-05-13
 * @copyright Copyright (c) 2026
 *
 */
#pragma once

#include "mcu.hpp"

namespace Embys::Stm32
{

using McuTraits = DeviceTraits<TargetDevice>;

} // namespace Embys::Stm32
