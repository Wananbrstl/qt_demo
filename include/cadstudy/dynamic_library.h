#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include "cadstudy/result.h"

#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

namespace cadstudy {

namespace detail {
struct SharedLibraryState;
}

template <typename FunctionPointer>
class ImportedFunction {
  static_assert(std::is_pointer_v<FunctionPointer>,
                "FunctionPointer must be a function pointer");
  static_assert(std::is_function_v<std::remove_pointer_t<FunctionPointer>>,
                "FunctionPointer must point to a function type");

public:
  ImportedFunction() = delete;

  template <typename... Args>
  decltype(auto) operator()(Args&&... args) const {
    return std::invoke(function_, std::forward<Args>(args)...);
  }

  [[nodiscard]] FunctionPointer get() const noexcept { return function_; }

private:
  friend class SharedLibrary;

  ImportedFunction(std::shared_ptr<const detail::SharedLibraryState> keep_alive,
                   FunctionPointer function) noexcept
      : keep_alive_(std::move(keep_alive)), function_(function) {}

  std::shared_ptr<const detail::SharedLibraryState> keep_alive_;
  FunctionPointer function_;
};

class SharedLibrary {
public:
  enum class SearchPolicy {
    system32,
    absolute_path_with_safe_dependencies,
    legacy_default
  };

  SharedLibrary() = delete;

  static Result<SharedLibrary> load(
      std::wstring_view path,
      SearchPolicy policy = SearchPolicy::absolute_path_with_safe_dependencies);

  template <typename FunctionPointer>
  Result<ImportedFunction<FunctionPointer>> resolve(
      std::string_view symbol_name) const {
    static_assert(std::is_pointer_v<FunctionPointer>,
                  "resolve<T> requires a function pointer type");
    static_assert(std::is_function_v<std::remove_pointer_t<FunctionPointer>>,
                  "resolve<T> requires a function pointer type");

    auto raw = resolve_raw(symbol_name);
    if (!raw) {
      return Result<ImportedFunction<FunctionPointer>>::failure(
          std::move(raw).error());
    }

    // Win32 specifies FARPROC as its untyped symbol boundary. Keep the cast
    // localized here and expose only the caller-supplied typed pointer.
    auto typed = reinterpret_cast<FunctionPointer>(raw.value());
    return Result<ImportedFunction<FunctionPointer>>::success(
        ImportedFunction<FunctionPointer>(state_, typed));
  }

  [[nodiscard]] const std::wstring& path() const noexcept;

private:
  explicit SharedLibrary(
      std::shared_ptr<detail::SharedLibraryState> state) noexcept
      : state_(std::move(state)) {}

  Result<FARPROC> resolve_raw(std::string_view symbol_name) const;

  std::shared_ptr<detail::SharedLibraryState> state_;
};

} // namespace cadstudy
