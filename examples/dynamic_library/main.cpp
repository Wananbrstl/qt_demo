#include "cadstudy/dynamic_library.h"

#include <iostream>

namespace {

void print_error(const cadstudy::Error& error) {
  std::cerr << error.operation << " failed for '" << error.subject
            << "': " << error.message << " (native=" << error.native_code
            << ")\n";
}

} // namespace

int main() {
  using GetCurrentProcessIdPointer = DWORD(WINAPI*)();

  // The library object is deliberately kept in this inner scope. The imported
  // function remains valid because it shares ownership of the module state.
  auto imported = []() {
    auto library = cadstudy::SharedLibrary::load(
        L"kernel32.dll", cadstudy::SharedLibrary::SearchPolicy::system32);
    if (!library) {
      return cadstudy::Result<
          cadstudy::ImportedFunction<GetCurrentProcessIdPointer>>::failure(
          std::move(library).error());
    }
    return library.value().resolve<GetCurrentProcessIdPointer>(
        "GetCurrentProcessId");
  }();

  if (!imported) {
    print_error(imported.error());
    return 1;
  }

  auto get_current_process_id = std::move(imported).value();
  std::cout << "Current process id: " << get_current_process_id() << '\n';

  auto missing_library = cadstudy::SharedLibrary::load(
      L"cadstudy-not-present.dll",
      cadstudy::SharedLibrary::SearchPolicy::system32);
  if (!missing_library) {
    std::cout << "Expected failure is data, not a half-valid object: "
              << missing_library.error().message << '\n';
  }
}
