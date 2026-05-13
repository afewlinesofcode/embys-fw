/**
 * @file mcu_traits.hpp
 * @author Stanislav Yaranov (stanislav.yaranov@gmail.com)
 * @brief Compile-time MCU family capability traits.
 *
 * Provides a single McuTraits struct with constexpr members describing
 * hardware capabilities that differ across STM32 families. Consumers use
 * these traits instead of scattered preprocessor guards.
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

#include <stdint.h>

namespace Embys::Stm32
{

struct McuTraits
{
#if defined(STM32F1xx)

  // ── Cache ──────────────────────────────────────────────────────────────────
  static constexpr bool has_data_cache = false;
  static constexpr bool has_instruction_cache = false;

  // ── GPIO clock bus ─────────────────────────────────────────────────────────
  /// GPIO clocks are on APB2 (F1); not AHB1 or AHB4.
  static constexpr bool gpio_clock_ahb1 = false;
  static constexpr bool gpio_clock_ahb4 = false;

  // ── GPIO configuration registers ───────────────────────────────────────────
  /// F1 uses CRL/CRH nibbles; F4+ uses MODER/OTYPER/OSPEEDR/PUPDR/AFR.
  static constexpr bool has_gpio_afr = false;

  // ── EXTI source routing ────────────────────────────────────────────────────
  /// F1 routes EXTI via AFIO->EXTICR.
  static constexpr bool has_afio = true;
  /// F4/F7/H7 route EXTI via SYSCFG->EXTICR (requires SYSCFG clock).
  static constexpr bool has_syscfg_exti = false;
  /// H7 EXTI pending register is PR1 instead of PR.
  static constexpr bool exti_pending_reg_pr1 = false;

  // ── Timer ──────────────────────────────────────────────────────────────────
  /// H7 timer APB1 enable register is APB1LENR instead of APB1ENR.
  static constexpr bool timer_apb1lenr = false;
  /// TIM2 ARR register width in bits (16 on F1, 32 on F4/F7/H7).
  static constexpr uint32_t timer2_arr_max = UINT16_MAX;

#elif defined(STM32F4xx)

  static constexpr bool has_data_cache = false;
  static constexpr bool has_instruction_cache = false;
  static constexpr bool gpio_clock_ahb1 = true;
  static constexpr bool gpio_clock_ahb4 = false;
  static constexpr bool has_gpio_afr = true;
  static constexpr bool has_afio = false;
  static constexpr bool has_syscfg_exti = true;
  static constexpr bool exti_pending_reg_pr1 = false;
  static constexpr bool timer_apb1lenr = false;
  static constexpr uint32_t timer2_arr_max = UINT32_MAX;

#elif defined(STM32F7xx)

  /// Cortex-M7 has both instruction and data caches.
  static constexpr bool has_data_cache = true;
  static constexpr bool has_instruction_cache = true;
  static constexpr bool gpio_clock_ahb1 = true;
  static constexpr bool gpio_clock_ahb4 = false;
  static constexpr bool has_gpio_afr = true;
  static constexpr bool has_afio = false;
  static constexpr bool has_syscfg_exti = true;
  static constexpr bool exti_pending_reg_pr1 = false;
  static constexpr bool timer_apb1lenr = false;
  static constexpr uint32_t timer2_arr_max = UINT32_MAX;

#elif defined(STM32H7xx)

  static constexpr bool has_data_cache = true;
  static constexpr bool has_instruction_cache = true;
  static constexpr bool gpio_clock_ahb1 = false;
  static constexpr bool gpio_clock_ahb4 = true;
  static constexpr bool has_gpio_afr = true;
  static constexpr bool has_afio = false;
  static constexpr bool has_syscfg_exti = true;
  /// H7 uses EXTI->PR1 for lines 0-31.
  static constexpr bool exti_pending_reg_pr1 = true;
  /// H7 timer APB1 enable register is RCC->APB1LENR.
  static constexpr bool timer_apb1lenr = true;
  static constexpr uint32_t timer2_arr_max = UINT32_MAX;

#else
#error                                                                         \
    "No STM32 family defined. Define STM32F1xx, STM32F4xx, STM32F7xx, or STM32H7xx."
#endif
};

} // namespace Embys::Stm32
