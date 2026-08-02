#pragma once

#include <utility>

namespace Embys
{

template <typename T, typename E>
class [[nodiscard]] Result
{
public:
  [[nodiscard]] static constexpr Result
  success(T value) noexcept
  {
    return Result{std::move(value), E{}, true};
  }

  [[nodiscard]] static constexpr Result
  failure(E error) noexcept
  {
    return Result{T{}, error, false};
  }

  [[nodiscard]] constexpr bool
  has_value() const noexcept
  {
    return has_value_;
  }

  [[nodiscard]] constexpr explicit
  operator bool() const noexcept
  {
    return has_value();
  }

  [[nodiscard]] constexpr T &
  value() & noexcept
  {
    return value_;
  }

  [[nodiscard]] constexpr const T &
  value() const & noexcept
  {
    return value_;
  }

  [[nodiscard]] constexpr T &&
  value() && noexcept
  {
    return std::move(value_);
  }

  [[nodiscard]] constexpr E
  error() const noexcept
  {
    return error_;
  }

private:
  constexpr Result(T value, E error, bool has_value) noexcept
    : value_{std::move(value)}, error_{error}, has_value_{has_value}
  {
  }

  T value_{};
  E error_{};
  bool has_value_ = false;
};

template <typename E>
class [[nodiscard]] Result<void, E>
{
public:
  [[nodiscard]] static constexpr Result
  success() noexcept
  {
    return Result{E{}, true};
  }

  [[nodiscard]] static constexpr Result
  failure(E error) noexcept
  {
    return Result{error, false};
  }

  [[nodiscard]] constexpr bool
  has_value() const noexcept
  {
    return has_value_;
  }

  [[nodiscard]] constexpr explicit
  operator bool() const noexcept
  {
    return has_value();
  }

  [[nodiscard]] constexpr E
  error() const noexcept
  {
    return error_;
  }

private:
  constexpr Result(E error, bool has_value) noexcept
    : error_{error}, has_value_{has_value}
  {
  }

  E error_{};
  bool has_value_ = false;
};

} // namespace Embys
