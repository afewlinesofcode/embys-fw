/**
 * @file debug.hpp
 * @author Stanislav Yaranov (stanislav.yaranov@gmail.com)
 * @brief Debug values container for runtime diagnostics
 *
 * @version 0.1
 * @date 2026-04-29
 * @copyright Copyright (c) 2026
 *
 */
#pragma once

#define EMBYS_LOG_LEVEL_NONE 0
#define EMBYS_LOG_LEVEL_ERROR 1
#define EMBYS_LOG_LEVEL_WARN 2
#define EMBYS_LOG_LEVEL_INFO 3
#define EMBYS_LOG_LEVEL_DEBUG 4
#define EMBYS_LOG_LEVEL_TRACE 5

#ifdef EMBYS_LOG_LEVEL
#include <cstdio>
#else
#define EMBYS_LOG_LEVEL 0
#endif

#if EMBYS_LOG_LEVEL >= EMBYS_LOG_LEVEL_DEBUG
#define EMBYS_DEBUG(...) std::printf(__VA_ARGS__)
#else
#define EMBYS_DEBUG(...)
#endif

namespace Embys
{

struct DebugValues
{
  int int_value1{};
  int int_value2{};
  int int_value3{};
  int int_value4{};
};

extern DebugValues debug_values;

}; // namespace Embys
