#include "cadstudy/command_catalog.h"

#include <utility>

namespace cadstudy {

Result<CommandCatalog> CommandCatalog::compile(const CommandSpec* specs,
                                               std::size_t count) {
  if (specs == nullptr && count != 0U) {
    return Result<CommandCatalog>::failure(
        Error{ErrorKind::invalid_argument, 0, "compile command catalog", {},
              "a non-zero count requires a valid specification array"});
  }

  CommandCatalog catalog;
  catalog.entries_.reserve(count);
  catalog.index_.reserve(count);

  for (std::size_t index = 0; index < count; ++index) {
    const CommandSpec& spec = specs[index];
    if (spec.name.empty()) {
      return Result<CommandCatalog>::failure(
          Error{ErrorKind::invalid_argument, 0, "compile command catalog", {},
                "a command name must not be empty"});
    }
    if (spec.handler == nullptr) {
      return Result<CommandCatalog>::failure(
          Error{ErrorKind::invalid_argument, 0, "compile command catalog",
                std::string(spec.name), "the command handler is null"});
    }

    std::string owned_name(spec.name);
    if (catalog.index_.find(owned_name) != catalog.index_.end()) {
      return Result<CommandCatalog>::failure(
          Error{ErrorKind::conflict, 0, "compile command catalog", owned_name,
                "duplicate command name"});
    }

    catalog.entries_.push_back(Entry{owned_name, std::string(spec.description),
                                     spec.flags, spec.handler});
    catalog.index_.emplace(std::move(owned_name), index);
  }

  return Result<CommandCatalog>::success(std::move(catalog));
}

const CommandCatalog::Entry* CommandCatalog::find(
    std::string_view name) const noexcept {
  const auto iterator = index_.find(std::string(name));
  if (iterator == index_.end()) {
    return nullptr;
  }
  return &entries_[iterator->second];
}

Result<int> CommandCatalog::invoke(std::string_view name,
                                   CommandContext& context) const {
  const Entry* entry = find(name);
  if (entry == nullptr) {
    return Result<int>::failure(
        Error{ErrorKind::not_found, 0, "invoke command", std::string(name),
              "the command is not registered"});
  }
  return Result<int>::success(entry->handler(context));
}

} // namespace cadstudy
