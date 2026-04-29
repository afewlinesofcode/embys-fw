/**
 * @file stm32f1xx.hpp
 * @author Stanislav Yaranov (stanislav.yaranov@gmail.com)
 * @brief STM32F1xx I2C peripheral header selector (hardware vs simulator)
 *
 * @version 0.1
 * @date 2026-04-29
 * @copyright Copyright (c) 2026
 *
 */
#ifdef STM32_SIM
#include <embys/stm32/sim/sim.hpp>
#else
#include <stm32f1xx.h>
#endif
