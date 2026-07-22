#pragma once

#include "cadstudy/result.h"

#include <cstddef>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace cadstudy {

struct ParameterSpec {
  std::string_view name;
  double initial_value{0.0};
};

enum class RuleOperation { add, subtract, multiply, divide };

struct RuleSpec {
  std::string_view output;
  RuleOperation operation{RuleOperation::add};
  std::string_view left;
  std::string_view right;
};

class ParameterGraph {
public:
  static Result<ParameterGraph> compile(const ParameterSpec* parameters,
                                        std::size_t parameter_count,
                                        const RuleSpec* rules,
                                        std::size_t rule_count);

  template <std::size_t P, std::size_t R>
  static Result<ParameterGraph> compile(
      const ParameterSpec (&parameters)[P], const RuleSpec (&rules)[R]) {
    return compile(parameters, P, rules, R);
  }

  Result<bool> set_driver(std::string_view name, double value);
  Result<std::unordered_map<std::string, double>> evaluate() const;

private:
  struct Rule {
    std::string output;
    RuleOperation operation{RuleOperation::add};
    std::string left;
    std::string right;
  };

  std::unordered_map<std::string, double> values_;
  std::unordered_set<std::string> derived_;
  std::vector<Rule> rules_;
  std::vector<std::size_t> evaluation_order_;
};

} // namespace cadstudy
