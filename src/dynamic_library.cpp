#include "cadstudy/dynamic_library.h"

#include <system_error>

namespace cadstudy {
namespace detail {

struct SharedLibraryState {
  HMODULE module{nullptr};
  std::wstring path;

  ~SharedLibraryState() {
    if (module != nullptr) {
      ::FreeLibrary(module);
    }
  }
};

} // namespace detail

namespace {

Error win32_error(std::string operation, std::string subject, DWORD code) {
  return Error{ErrorKind::operating_system,
               static_cast<std::uint32_t>(code),
               std::move(operation),
               std::move(subject),
               std::system_category().message(static_cast<int>(code))};
}

std::string narrow_for_diagnostics(std::wstring_view text) {
  if (text.empty()) {
    return {};
  }
  const int required = ::WideCharToMultiByte(
      CP_UTF8, 0, text.data(), static_cast<int>(text.size()), nullptr, 0,
      nullptr, nullptr);
  if (required <= 0) {
    return "<path conversion failed>";
  }
  std::string result(static_cast<std::size_t>(required), '\0');
  ::WideCharToMultiByte(CP_UTF8, 0, text.data(), static_cast<int>(text.size()),
                        result.data(), required, nullptr, nullptr);
  return result;
}

} // namespace

Result<SharedLibrary> SharedLibrary::load(std::wstring_view path,
                                          SearchPolicy policy) {
  if (path.empty()) {
    return Result<SharedLibrary>::failure(
        Error{ErrorKind::invalid_argument, 0, "LoadLibraryExW", {},
              "the library path must not be empty"});
  }

  DWORD flags = 0;
  switch (policy) {
  case SearchPolicy::system32:
    flags = LOAD_LIBRARY_SEARCH_SYSTEM32;
    break;
  case SearchPolicy::absolute_path_with_safe_dependencies:
    flags = LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR |
            LOAD_LIBRARY_SEARCH_DEFAULT_DIRS;
    break;
  case SearchPolicy::legacy_default:
    flags = 0;
    break;
  }

  std::wstring owned_path(path);
  HMODULE module = ::LoadLibraryExW(owned_path.c_str(), nullptr, flags);
  if (module == nullptr) {
    const DWORD code = ::GetLastError();
    return Result<SharedLibrary>::failure(
        win32_error("LoadLibraryExW", narrow_for_diagnostics(path), code));
  }

  auto state = std::make_shared<detail::SharedLibraryState>();
  state->module = module;
  state->path = std::move(owned_path);
  return Result<SharedLibrary>::success(SharedLibrary(std::move(state)));
}

Result<FARPROC> SharedLibrary::resolve_raw(
    std::string_view symbol_name) const {
  if (symbol_name.empty() ||
      symbol_name.find('\0') != std::string_view::npos) {
    return Result<FARPROC>::failure(
        Error{ErrorKind::invalid_argument, 0, "GetProcAddress",
              std::string(symbol_name),
              "the symbol name must be non-empty and contain no NUL"});
  }

  const std::string owned_name(symbol_name);
  FARPROC address = ::GetProcAddress(state_->module, owned_name.c_str());
  if (address == nullptr) {
    const DWORD code = ::GetLastError();
    return Result<FARPROC>::failure(
        win32_error("GetProcAddress", owned_name, code));
  }
  return Result<FARPROC>::success(address);
}

const std::wstring& SharedLibrary::path() const noexcept {
  return state_->path;
}

} // namespace cadstudy
