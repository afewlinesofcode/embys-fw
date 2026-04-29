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
