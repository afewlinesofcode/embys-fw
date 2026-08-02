/**
 * @file def.hpp
 * @author Stanislav Yaranov (stanislav.yaranov@gmail.com)
 * @brief Common macros for bit manipulation and error handling
 *
 * @version 0.1
 * @date 2026-04-29
 * @copyright Copyright (c) 2026
 *
 */
#pragma once

#include "debug.hpp"
#include "mcu.hpp"
#include "mcu_traits.hpp"
#include "register.hpp"
#include "types.hpp"

#define SET_BIT_V(REG, BIT) ((REG) = (REG) | (BIT))
#define CLEAR_BIT_V(REG, BIT) ((REG) = (REG) & ~(BIT))
#define MOD_BIT_V(REG, SHIFT, MASK, VAL)                                       \
  ((REG) = ((REG) & ~((MASK) << (SHIFT))) | (((VAL) & (MASK)) << (SHIFT)))
#define INC_V(NUM) NUM = NUM + 1
#define DEC_V(NUM) NUM = NUM - 1
#define TRY(EXPR)                                                              \
  {                                                                            \
    int _try_result = (EXPR);                                                  \
    if (_try_result < 0)                                                       \
      return _try_result;                                                      \
  }
