#pragma once

#include "cadstudy/result.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace cadstudy {

struct CommandContext {
  std::string active_document;
};

using CommandHandler = int (*)(CommandContext&);

enum class CommandFlag : std::uint32_t {
  none = 0,
  transparent = 1U << 0U,
  requires_document = 1U << 1U,
  modifies_database = 1U << 2U
};

constexpr CommandFlag operator|(CommandFlag left, CommandFlag right) noexcept {
  return static_cast<CommandFlag>(static_cast<std::uint32_t>(left) |
                                  static_cast<std::uint32_t>(right));
}

struct CommandSpec {
  std::string_view name;
  std::string_view description;
  CommandFlag flags{CommandFlag::none};
  CommandHandler handler{nullptr};
};

class CommandCatalog {
public:
  struct Entry {
    std::string name;
    std::string description;
    CommandFlag flags{CommandFlag::none};
    CommandHandler handler{nullptr};
  };

  static Result<CommandCatalog> compile(const CommandSpec* specs,
                                        std::size_t count);

  template <std::size_t N>
  static Result<CommandCatalog> compile(const CommandSpec (&specs)[N]) {
    return compile(specs, N);
  }

  [[nodiscard]] const Entry* find(std::string_view name) const noexcept;
  Result<int> invoke(std::string_view name, CommandContext& context) const;
  [[nodiscard]] const std::vector<Entry>& entries() const noexcept {
    return entries_;
  }

private:
  std::vector<Entry> entries_;
  std::unordered_map<std::string, std::size_t> index_;
};

} // namespace cadstudy
