#include "cadstudy/command_catalog.h"

#include <iostream>

namespace {

int draw_line(cadstudy::CommandContext& context) {
  std::cout << "LINE modifies " << context.active_document << '\n';
  return 0;
}

int zoom_extents(cadstudy::CommandContext& context) {
  std::cout << "ZOOM_EXTENTS observes " << context.active_document << '\n';
  return 0;
}

constexpr cadstudy::CommandSpec commands[] = {
    {"LINE", "Create a line entity",
     cadstudy::CommandFlag::requires_document |
         cadstudy::CommandFlag::modifies_database,
     &draw_line},
    {"ZOOM_EXTENTS", "Fit visible geometry in the viewport",
     cadstudy::CommandFlag::transparent |
         cadstudy::CommandFlag::requires_document,
     &zoom_extents},
};

} // namespace

int main() {
  auto catalog = cadstudy::CommandCatalog::compile(commands);
  if (!catalog) {
    std::cerr << catalog.error().message << '\n';
    return 1;
  }

  for (const auto& command : catalog.value().entries()) {
    std::cout << command.name << ": " << command.description << '\n';
  }

  cadstudy::CommandContext context{"drawing-01.cad"};
  auto result = catalog.value().invoke("LINE", context);
  if (!result) {
    std::cerr << result.error().message << '\n';
    return 1;
  }
  return result.value();
}
