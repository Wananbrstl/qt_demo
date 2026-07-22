#include "cadstudy/parameter_graph.h"

#include <iostream>

namespace {

constexpr cadstudy::ParameterSpec parameters[] = {
    {"width", 100.0},
    {"height", 50.0},
    {"area", 0.0},
    {"double_area", 0.0},
};

constexpr cadstudy::RuleSpec rules[] = {
    {"area", cadstudy::RuleOperation::multiply, "width", "height"},
    {"double_area", cadstudy::RuleOperation::add, "area", "area"},
};

} // namespace

int main() {
  auto graph = cadstudy::ParameterGraph::compile(parameters, rules);
  if (!graph) {
    std::cerr << graph.error().message << '\n';
    return 1;
  }

  auto changed = graph.value().set_driver("width", 120.0);
  if (!changed) {
    std::cerr << changed.error().message << '\n';
    return 1;
  }

  auto values = graph.value().evaluate();
  if (!values) {
    std::cerr << values.error().message << '\n';
    return 1;
  }

  std::cout << "area=" << values.value().at("area") << '\n';
  std::cout << "double_area=" << values.value().at("double_area") << '\n';
}
