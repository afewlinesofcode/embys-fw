#pragma once

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
struct Callable
{
  void (*callback)(void *, Args...) = nullptr;
  void *context = nullptr;

  void
  operator()(Args... args) const
  {
    if (callback)
    {
      callback(context, args...);
    }
  }

  bool
  operator==(const Callable &other) const
  {
    return callback == other.callback && context == other.context;
  }

  inline bool
  empty() const
  {
    return callback == nullptr;
  }

  void
  clear()
  {
    callback = nullptr;
    context = nullptr;
  }
};
}; // namespace Embys
