#pragma once

#include <cstdint>
#include <stdexcept>
#include <string>
#include <utility>
#include <variant>

namespace cadstudy {

enum class ErrorKind {
  invalid_argument,
  not_found,
  conflict,
  operating_system,
  invalid_state,
  evaluation
};

struct Error {
  ErrorKind kind{ErrorKind::invalid_state};
  std::uint32_t native_code{0};
  std::string operation;
  std::string subject;
  std::string message;
};

template <typename T>
class Result {
public:
  static Result success(T value) {
    return Result(std::in_place_index<0>, std::move(value));
  }

  static Result failure(Error error) {
    return Result(std::in_place_index<1>, std::move(error));
  }

  [[nodiscard]] bool has_value() const noexcept {
    return storage_.index() == 0;
  }

  explicit operator bool() const noexcept { return has_value(); }

  T& value() & { return std::get<0>(storage_); }
  const T& value() const& { return std::get<0>(storage_); }
  T&& value() && { return std::get<0>(std::move(storage_)); }

  Error& error() & { return std::get<1>(storage_); }
  const Error& error() const& { return std::get<1>(storage_); }
  Error&& error() && { return std::get<1>(std::move(storage_)); }

private:
  template <std::size_t I, typename U>
  explicit Result(std::in_place_index_t<I> index, U&& value)
      : storage_(index, std::forward<U>(value)) {}

  std::variant<T, Error> storage_;
};

} // namespace cadstudy
