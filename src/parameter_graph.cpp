#include "cadstudy/parameter_graph.h"

#include <cmath>
#include <queue>
#include <utility>

namespace cadstudy {

Result<ParameterGraph> ParameterGraph::compile(
    const ParameterSpec* parameters, std::size_t parameter_count,
    const RuleSpec* rules, std::size_t rule_count) {
  if ((parameters == nullptr && parameter_count != 0U) ||
      (rules == nullptr && rule_count != 0U)) {
    return Result<ParameterGraph>::failure(
        Error{ErrorKind::invalid_argument, 0, "compile parameter graph", {},
              "a non-zero count requires a valid specification array"});
  }

  ParameterGraph graph;
  graph.values_.reserve(parameter_count);
  graph.rules_.reserve(rule_count);

  for (std::size_t index = 0; index < parameter_count; ++index) {
    const std::string name(parameters[index].name);
    if (name.empty()) {
      return Result<ParameterGraph>::failure(
          Error{ErrorKind::invalid_argument, 0, "compile parameter graph", {},
                "a parameter name must not be empty"});
    }
    if (!graph.values_.emplace(name, parameters[index].initial_value).second) {
      return Result<ParameterGraph>::failure(
          Error{ErrorKind::conflict, 0, "compile parameter graph", name,
                "duplicate parameter name"});
    }
  }

  std::unordered_map<std::string, std::size_t> producer;
  producer.reserve(rule_count);
  for (std::size_t index = 0; index < rule_count; ++index) {
    const RuleSpec& spec = rules[index];
    const std::string output(spec.output);
    const std::string left(spec.left);
    const std::string right(spec.right);
    if (graph.values_.find(output) == graph.values_.end() ||
        graph.values_.find(left) == graph.values_.end() ||
        graph.values_.find(right) == graph.values_.end()) {
      return Result<ParameterGraph>::failure(
          Error{ErrorKind::not_found, 0, "compile parameter graph", output,
                "a rule references an undeclared parameter"});
    }
    if (!producer.emplace(output, index).second) {
      return Result<ParameterGraph>::failure(
          Error{ErrorKind::conflict, 0, "compile parameter graph", output,
                "a parameter has more than one producing rule"});
    }
    graph.derived_.insert(output);
    graph.rules_.push_back(Rule{output, spec.operation, left, right});
  }

  std::vector<std::vector<std::size_t>> consumers(rule_count);
  std::vector<std::size_t> indegree(rule_count, 0U);
  for (std::size_t index = 0; index < rule_count; ++index) {
    const Rule& rule = graph.rules_[index];
    for (const std::string* input : {&rule.left, &rule.right}) {
      const auto producer_it = producer.find(*input);
      if (producer_it != producer.end()) {
        consumers[producer_it->second].push_back(index);
        ++indegree[index];
      }
    }
  }

  std::queue<std::size_t> ready;
  for (std::size_t index = 0; index < rule_count; ++index) {
    if (indegree[index] == 0U) {
      ready.push(index);
    }
  }

  while (!ready.empty()) {
    const std::size_t current = ready.front();
    ready.pop();
    graph.evaluation_order_.push_back(current);
    for (const std::size_t consumer : consumers[current]) {
      --indegree[consumer];
      if (indegree[consumer] == 0U) {
        ready.push(consumer);
      }
    }
  }

  if (graph.evaluation_order_.size() != rule_count) {
    return Result<ParameterGraph>::failure(
        Error{ErrorKind::conflict, 0, "compile parameter graph", {},
              "the rule dependency graph contains a cycle"});
  }

  return Result<ParameterGraph>::success(std::move(graph));
}

Result<bool> ParameterGraph::set_driver(std::string_view name, double value) {
  const std::string owned_name(name);
  const auto iterator = values_.find(owned_name);
  if (iterator == values_.end()) {
    return Result<bool>::failure(
        Error{ErrorKind::not_found, 0, "set driver", owned_name,
              "the parameter is not declared"});
  }
  if (derived_.find(owned_name) != derived_.end()) {
    return Result<bool>::failure(
        Error{ErrorKind::invalid_state, 0, "set driver", owned_name,
              "derived parameters cannot be assigned directly"});
  }
  iterator->second = value;
  return Result<bool>::success(true);
}

Result<std::unordered_map<std::string, double>>
ParameterGraph::evaluate() const {
  auto evaluated = values_;
  for (const std::size_t index : evaluation_order_) {
    const Rule& rule = rules_[index];
    const double left = evaluated.at(rule.left);
    const double right = evaluated.at(rule.right);
    double value = 0.0;
    switch (rule.operation) {
    case RuleOperation::add:
      value = left + right;
      break;
    case RuleOperation::subtract:
      value = left - right;
      break;
    case RuleOperation::multiply:
      value = left * right;
      break;
    case RuleOperation::divide:
      if (right == 0.0) {
        return Result<std::unordered_map<std::string, double>>::failure(
            Error{ErrorKind::evaluation, 0, "evaluate rule", rule.output,
                  "division by zero"});
      }
      value = left / right;
      break;
    }
    if (!std::isfinite(value)) {
      return Result<std::unordered_map<std::string, double>>::failure(
          Error{ErrorKind::evaluation, 0, "evaluate rule", rule.output,
                "the result is not finite"});
    }
    evaluated[rule.output] = value;
  }
  return Result<std::unordered_map<std::string, double>>::success(
      std::move(evaluated));
}

} // namespace cadstudy
