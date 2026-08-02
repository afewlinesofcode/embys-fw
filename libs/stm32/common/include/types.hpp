/**
 * @file types.hpp
 * @author Stanislav Yaranov (stanislav.yaranov@gmail.com)
 * @brief Generic callable/callback type
 *
 * @version 0.1
 * @date 2026-04-29
 * @copyright Copyright (c) 2026
 *
 */
#pragma once

#include "result.hpp"

namespace Embys
{

/**
 * @brief A generic callable type.
 *
 * This template defines a callback with a context. It is mostly for event
 * handlers.
 *
 * @tparam Args Argument types of the callable.
 */
template <typename... Args>
class Callback
{
public:
  using CallbackFn = void (*)(void *, Args...);

  constexpr Callback() noexcept = default;
  constexpr Callback(CallbackFn callback, void *context = nullptr) noexcept
    : callback_{callback}, context_{context}
  {
  }

  template <auto Method, typename Object>
  [[nodiscard]] static constexpr Callback
  bind(Object &object) noexcept
  {
    return Callback{[](void *context, Args... args)
                    { (static_cast<Object *>(context)->*Method)(args...); },
                    &object};
  }

  constexpr void
  operator()(Args... args) const noexcept
  {
    if (callback_)
    {
      callback_(context_, args...);
    }
  }

  [[nodiscard]] constexpr explicit
  operator bool() const noexcept
  {
    return callback_ != nullptr;
  }

  friend constexpr bool
  operator==(const Callback &, const Callback &) noexcept = default;

  [[nodiscard]] constexpr bool
  empty() const noexcept
  {
    return callback_ == nullptr;
  }

  constexpr void
  clear() noexcept
  {
    callback_ = nullptr;
    context_ = nullptr;
  }

private:
  CallbackFn callback_ = nullptr;
  void *context_ = nullptr;
};
}; // namespace Embys
