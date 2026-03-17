/**
 * @file base.hpp
 * @author Stanislav Yaranov (stanislav.yaranov@gmail.com)
 * @brief Base functionality for the STM32 simulation environment, including
 * core clock simulation, interrupt handling, and hook management.
 * @version 0.1
 * @date 2026-03-09
 * @copyright Copyright (c) 2026
 */
#pragma once

#include <functional>
#include <map>
#include <string>
#include <vector>

#include "core.hpp"

namespace Embys::Stm32::Sim
{

/**
 * @brief MCU core clock frequency in Hz. This variable can be modified during
 * tests to simulate different clock speeds.
 */
extern uint32_t core_clock;

/**
 * @brief Cycles per microsecond, calculated based on the core clock frequency.
 */
extern uint32_t cyc_per_us;

/**
 * @brief Get the current mock PRIMASK value.
 * @return The current mock PRIMASK value.
 */
uint32_t
get_primask(void);

/**
 * @brief Set the mock PRIMASK value.
 * @param priMask The value to set the mock PRIMASK to.
 */
void
set_primask(uint32_t priMask);

/**
 * @brief Simulate disabling interrupts by setting the mock PRIMASK.
 */
void
disable_irq();

/**
 * @brief Simulate enabling interrupts by clearing the mock PRIMASK.
 */
void
enable_irq();

/**
 * @brief Simulate a WFI (Wait For Interrupt) instruction. This function will
 * block until an interrupt is simulated, allowing the test to verify that the
 * firmware correctly handles low-power states and wakes up on interrupts.
 */
void
wfi(void);

/**
 * @brief Simulate a DSB (Data Synchronization Barrier) instruction. This
 * function can be used to ensure that all memory operations are completed
 * before proceeding, which is important for testing code that relies on memory
 * ordering.
 */
void
dsb(void);

/**
 * @brief Simulate a NOP (No Operation) instruction. This function can be used
 * to introduce a small delay or to align code for testing purposes.
 */
void
nop(void);

/**
 * @brief Simulate the passage of one cycle in the mock environment.
 * Increments the cycle count and triggers any hooks that are scheduled
 * to run at this cycle. Also simulates the behavior of timers by
 * incrementing their counters if they are enabled.
 */
void
cycle();

namespace Base
{

/**
 * @brief Type definition for a hook function. A hook is a user-defined function
 * that can be called at specific points during the mock execution to simulate
 * hardware behavior or to verify certain conditions.
 * The hook function takes the current cycle count as an argument, allowing it
 * to perform time-based actions or checks.
 */
using Hook = std::function<void(uint32_t)>;

/**
 * @brief Reset the mock base to its initial state. This function clears all
 * registered hooks and resets any internal state variables to their defaults.
 * It should be called at the beginning of each test to ensure a clean testing
 * environment.
 */
void
reset();

/**
 * @brief Add a hook function to be called during the mock execution.
 * The hook will be called on each cycle.
 * @param hook The hook function to add.
 */
void
add_hook(Hook hook);

/**
 * @brief Add a delayed hook function to be called after a specified number of
 * cycles.
 * Use this to postpone actions that should occur after a certain delay.
 * @param delay_cyc The number of cycles to wait before calling the hook.
 * @param hook The hook function to add.
 */
void
add_delayed_hook(uint32_t delay_cyc, Hook hook);

/**
 * @brief Add a test hook function that can be triggered by a key.
 * @param key The key to associate with the test hook.
 * @param hook The hook function to add.
 */
void
add_test_hook(const std::string &key, Hook hook);

/**
 * @brief Trigger a test hook based on its key.
 *
 * This allows for specific hooks to be called during testing based on a string
 * identifier, for example to signal the mock environment about a certain
 * register read or write.
 * Probably you would have in your code a definition like this:
 * ```cpp
 * #ifdef MOCK_STM32
 * #define TEST_HOOK(key) Embys::Stm32::Sim::Base::trigger_test_hook(key)
 * #else
 * #define TEST_HOOK(key)
 * #endif
 * // And then to signal SR1 register read:
 * TEST_HOOK("i2c_read_sr1");
 * ```
 * @param key The key associated with the test hook to trigger.
 */
void
trigger_test_hook(const std::string &key);

}; // namespace Base

}; // namespace Embys::Stm32::Sim
