#include "cadstudy/command_catalog.h"
#include "cadstudy/dynamic_library.h"
#include "cadstudy/parameter_graph.h"

#include <cassert>
#include <cmath>
#include <iostream>

namespace {

int noop(cadstudy::CommandContext&) { return 7; }

void test_dynamic_library_lifetime() {
  using GetCurrentProcessIdPointer = DWORD(WINAPI*)();
  auto imported = []() {
    auto library = cadstudy::SharedLibrary::load(
        L"kernel32.dll", cadstudy::SharedLibrary::SearchPolicy::system32);
    assert(library);
    return library.value().resolve<GetCurrentProcessIdPointer>(
        "GetCurrentProcessId");
  }();
  assert(imported);
  assert(imported.value()() == ::GetCurrentProcessId());
}

void test_missing_symbol_is_error() {
  auto library = cadstudy::SharedLibrary::load(
      L"kernel32.dll", cadstudy::SharedLibrary::SearchPolicy::system32);
  assert(library);
  using MissingPointer = void(WINAPI*)();
  auto missing = library.value().resolve<MissingPointer>(
      "CadStudySymbolThatMustNotExist");
  assert(!missing);
  assert(missing.error().kind == cadstudy::ErrorKind::operating_system);
}

void test_command_declarations_are_validated() {
  const cadstudy::CommandSpec duplicate[] = {
      {"LINE", "first", cadstudy::CommandFlag::none, &noop},
      {"LINE", "second", cadstudy::CommandFlag::none, &noop},
  };
  auto invalid = cadstudy::CommandCatalog::compile(duplicate);
  assert(!invalid);
  assert(invalid.error().kind == cadstudy::ErrorKind::conflict);

  const cadstudy::CommandSpec valid[] = {
      {"LINE", "draw", cadstudy::CommandFlag::none, &noop},
  };
  auto catalog = cadstudy::CommandCatalog::compile(valid);
  assert(catalog);
  cadstudy::CommandContext context{"test.cad"};
  auto invoked = catalog.value().invoke("LINE", context);
  assert(invoked && invoked.value() == 7);
}

void test_parameter_graph_compiles_then_evaluates() {
  const cadstudy::ParameterSpec parameters[] = {
      {"width", 4.0}, {"height", 3.0}, {"area", 0.0}, {"twice", 0.0}};
  const cadstudy::RuleSpec rules[] = {
      {"area", cadstudy::RuleOperation::multiply, "width", "height"},
      {"twice", cadstudy::RuleOperation::add, "area", "area"},
  };
  auto graph = cadstudy::ParameterGraph::compile(parameters, rules);
  assert(graph);
  auto values = graph.value().evaluate();
  assert(values);
  assert(std::abs(values.value().at("area") - 12.0) < 1e-9);
  assert(std::abs(values.value().at("twice") - 24.0) < 1e-9);
}

void test_parameter_cycle_is_rejected_at_compile_time() {
  const cadstudy::ParameterSpec parameters[] = {{"a", 0.0}, {"b", 0.0}};
  const cadstudy::RuleSpec rules[] = {
      {"a", cadstudy::RuleOperation::add, "b", "b"},
      {"b", cadstudy::RuleOperation::add, "a", "a"},
  };
  auto graph = cadstudy::ParameterGraph::compile(parameters, rules);
  assert(!graph);
  assert(graph.error().kind == cadstudy::ErrorKind::conflict);
}

} // namespace

int main() {
  test_dynamic_library_lifetime();
  test_missing_symbol_is_error();
  test_command_declarations_are_validated();
  test_parameter_graph_compiles_then_evaluates();
  test_parameter_cycle_is_rejected_at_compile_time();
  std::cout << "All cadstudy tests passed.\n";
}
