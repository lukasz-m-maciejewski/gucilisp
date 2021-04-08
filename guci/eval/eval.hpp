#pragma once

#include <functional>
#include <initializer_list>
#include <span>
#include <string>
#include <unordered_map>

#include "guci/eval/eval_result.hpp"
#include "guci/parse/ast.hpp"

namespace guci {
class EvaluationContext;

class BuiltInFunction {
  using Impl = std::function<EvaluationResult(EvaluationContext&,
                                              std::span<Term const>)>;

  int arity_;
  Impl fun_;

 public:
  static constexpr auto kAnyArity = -1;
  static constexpr auto kAnyPositiveArity = -2;
  BuiltInFunction(int arity, Impl fun) : arity_{arity}, fun_{fun} {}

  int arity() const { return arity_; }
  bool acceptsArgumentNumber(std::size_t arg_num) const {
    if (arity_ == kAnyArity) {
      return true;
    }

    if (arity_ == kAnyPositiveArity and arg_num != 0) {
      return true;
    }

    return static_cast<std::size_t>(arity_) == arg_num;
  }
  EvaluationResult apply(EvaluationContext& ctx,
                         std::span<Term const> args) const {
    return fun_(ctx, args);
  }
};

class UserDefinedFunction {};

class Function {
  std::variant<BuiltInFunction, UserDefinedFunction> fun_;

 public:
  Function(BuiltInFunction f) : fun_{std::move(f)} {}
  Function(UserDefinedFunction f) : fun_{std::move(f)} {}

  auto const& operator()() const { return fun_; }
};
class EvaluationContext {
  using FunctionContainer = std::unordered_map<std::string, Function>;
  using ValueContainer = std::unordered_map<std::string, Term>;
  FunctionContainer functions_;
  ValueContainer values_;
  EvaluationContext const* parent_;

 public:
  using FunctionType = FunctionContainer::value_type;
  using ValueType = ValueContainer::value_type;

  EvaluationContext(std::initializer_list<FunctionType> functions,
                    std::initializer_list<ValueType> values)
      : functions_{functions}, values_{values}, parent_{nullptr} {}
  EvaluationContext(EvaluationContext const* parent,
                    std::initializer_list<FunctionType> functions,
                    std::initializer_list<ValueType> values)
      : functions_{functions}, values_{values}, parent_{parent} {}

  Term const* find_value(std::string const& id) const {
    auto const it = values_.find(id);
    if (it == values_.end()) {
      return parent_ != nullptr ? parent_->find_value(id) : nullptr;
    }

    return std::addressof(it->second);
  }

  Function const* find_function(std::string const& id) const {
    auto const it = functions_.find(id);
    if (it == functions_.end()) {
      return parent_ != nullptr ? parent_->find_function(id) : nullptr;
    }

    return std::addressof(it->second);
  }

  eval_result<void> set_value(std::string const& id, Term t) {
    if (values_.contains(id)) {
      return EvalError("value already exists");
    }
    values_.insert({id, t});
    return outcome::success();
  }

  eval_result<void> set_value(Identifier const& id, Term t) {
    return set_value(id.value(), std::move(t));
  }

  bool contains(std::string const& id) const {
    return functions_.contains(id) or values_.contains(id);
  }

  bool contains(Identifier const& id) const { return contains(id.value()); }
};

}  // namespace guci
