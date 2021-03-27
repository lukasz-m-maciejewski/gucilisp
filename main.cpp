#include <readline/history.h>
#include <readline/readline.h>

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <memory>
#include <span>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_set>
#include <variant>
#include <fmt/core.h>
#include <vector>

#include "parse/ast.hpp"
#include "parse/eval_error.hpp"
#include "parse/parse.hpp"
#include "utils/outcome.hpp"
#include "utils/overload.hpp"
#include "utils/string_manipulation.hpp"

namespace guci {

struct CStringDeleter {
  void operator()(char* ptr) { free(ptr); }
};

using SafeCStr = std::unique_ptr<char, CStringDeleter>;

class Action {};

class EvaluationSuccess {
 public:
  Term t;
  std::vector<Action> actions;

  EvaluationSuccess(Term term) : t{term}, actions{} {}

  void merge_action_from(std::vector<Action>&& other) {
    actions.insert(actions.end(), other.begin(), other.end());
  }
};

using EvaluationResult = eval_result<EvaluationSuccess>;

class EvaluationContext;

// Design decision: expression cannot modify global context while it's begin
// evaluated; a context modification can be either done by spawning a child
// context or by returning an action modifying global context, to be apply after
// evaluation is complete.

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
  using ValueContainer = std::unordered_map<std::string, Function>;
  ValueContainer values_;
  EvaluationContext const* parent_;

 public:
  using ValueType = ValueContainer::value_type;

  EvaluationContext(std::initializer_list<ValueType> values)
      : values_{values}, parent_{nullptr} {}
  EvaluationContext(EvaluationContext const* parent,
                    std::initializer_list<ValueType> values)
      : values_{values}, parent_{parent} {}

  Function const* find(std::string const& id) const {
    auto const it = values_.find(id);
    if (it == values_.end()) {
      return parent_ != nullptr ? parent_->find(id) : nullptr;
    }

    return std::addressof(it->second);
  }
};

EvaluationResult apply(Function const& fun, EvaluationContext& ctx,
                       std::span<Term const> args) {
  auto const v = overload(
      [&](BuiltInFunction const& f) -> EvaluationResult {
        if (not f.acceptsArgumentNumber(args.size())) {
          return EvalError{"arity mismatch"};
        }
        return f.apply(ctx, args);
      },
      [](UserDefinedFunction const&) -> EvaluationResult {
        return EvalError{"not defined"};
      });
  return std::visit(v, fun());
}

class EvaluatingVisitor {
  EvaluationContext* context_;

 public:
  EvaluatingVisitor(EvaluationContext& context)
      : context_{std::addressof(context)} {}

  EvaluationResult operator()(List<Term> const& l) {
    if (l.empty()) return Term{Nil{}};

    auto v = overload(
        [this, &l](Identifier const& id) -> EvaluationResult {
          Function const* f = context_->find(id.value());
          if (f == nullptr) return EvalError{"function not found"};

          return apply(*f, *context_, l.tail());
        },
        [](auto const&) -> EvaluationResult {
          return EvalError{"not a function"};
        });
    return std::visit(v, l.at(0).term());
  }

  EvaluationResult operator()(Number const& n) { return Term{n}; }

  EvaluationResult operator()(Identifier const& id) { return Term{id}; }

  EvaluationResult operator()(Nil const&) { return Term{NIL}; }

  EvaluationResult operator()(String const& s) { return Term{s}; }
};

class PrintingVisitor {
  std::stringstream s;

 public:
  std::string get() { return s.str(); }

  void operator()(List<Term> const& l) {
    s << '[';
    const auto num_elements = l.size();
    for (int i = 0; i < num_elements; ++i) {
      if (i > 0) s << ' ';
      std::visit(*this, l.at(i).term());
    }
    s << ']';
  }

  void operator()(Number const& n) { s << n.value(); }

  void operator()(Identifier const& i) { s << i.value(); }

  void operator()(Nil const&) { s << "NIL"; }

  void operator()(String const& str) { s << str;}

  // //
  // void operator()(Term const& t) { std::visit(*this, t.term()); }
  // void operator()(Action const&) { s << "ACTION"; }
  // void operator()(Error const& e) { s << "ERROR: " << e.msg(); }
};

std::string show_result(EvaluationContext& context, std::string_view input) {
  Term t = parse(input).value();

  EvaluatingVisitor ev(context);
  EvaluationResult res = std::visit(ev, t.term());

  if (!res) {
    return res.error().msg();
  }
  EvaluationSuccess es = res.value();
  PrintingVisitor v;
  std::visit(v, *es.t);

  return v.get();
}

class Add {
 public:
  EvaluationResult operator()(EvaluationContext& ctx,
                              std::span<Term const> ts) {
    EvaluationSuccess es{Number(0)};
    for (auto const& t : ts) {
      EvaluationSuccess t_eval =
          OUTCOME_TRYX(std::visit(EvaluatingVisitor{ctx}, *t));
      es.merge_action_from(std::move(t_eval.actions));
      EvaluationSuccess r = OUTCOME_TRYX(std::visit(*this, *es.t, *t_eval.t));
      es.t = r.t;
      es.merge_action_from(std::move(r.actions));
    }
    return es;
  }

  EvaluationResult operator()(Number const& lhs, Number const& rhs) {
    return Term{Number{lhs.value() + rhs.value()}};
  }

  template <typename LHS, typename RHS>
  EvaluationResult operator()(LHS const&, RHS const&) {
    return EvalError("unbound variables in arithmetic expression");
  }
};

class Subtract {
 public:
  EvaluationResult operator()(EvaluationContext& ctx,
                              std::span<Term const> args) {
    auto arg_0_eval =
        OUTCOME_TRYX(std::visit(EvaluatingVisitor{ctx}, *args[0]));

    EvaluationSuccess es{arg_0_eval.t};
    es.merge_action_from(std::move(arg_0_eval.actions));

    for (std::size_t i = 1; i < args.size(); ++i) {
      EvaluationSuccess arg_eval =
          OUTCOME_TRYX(std::visit(EvaluatingVisitor{ctx}, *args[0]));
      es.merge_action_from(std::move(arg_eval.actions));
      EvaluationSuccess r = OUTCOME_TRYX(std::visit(*this, *es.t, *arg_eval.t));
      es.t = r.t;
      es.merge_action_from(std::move(r.actions));
    }

    return es;
  }

  EvaluationResult operator()(Number const& lhs, Number const& rhs) {
    return Term{Number{lhs.value() - rhs.value()}};
  }

  template <typename LHS, typename RHS>
  EvaluationResult operator()(LHS const&, RHS const&) {
    return EvalError("unbound variables in arithmetic expression");
  }
};

outcome::result<void> RunRepl() {
  rl_bind_key('\t', rl_complete);

  bool program_termination_requested = false;

  EvaluationContext global_context{
      {{"+", BuiltInFunction(BuiltInFunction::kAnyArity, Add{})},
       {"-", BuiltInFunction(BuiltInFunction::kAnyPositiveArity, Subtract{})},
       {"quit", BuiltInFunction(0,
                                [&program_termination_requested](
                                    EvaluationContext&,
                                    std::span<Term const>) -> EvaluationResult {
                                  program_termination_requested = true;
                                  return EvaluationSuccess(NIL);
                                })}}};

  while (not program_termination_requested) {
    SafeCStr input{readline("prompt> ")};

    if (input == nullptr) {
      break;
    }

    add_history(input.get());

    auto result = show_result(global_context, input.get());
    puts(result.c_str());
  }

  return outcome::success();
}

outcome::result<void> Main(std::vector<std::string_view> const& args
                           [[maybe_unused]]) {
  return RunRepl();
}

}  // namespace guci

int main(int argc, char** argv) {
  std::vector<std::string_view> args{argv, argv + argc};
  auto result = guci::Main(args);
  fmt::print("Hello World!\n");
  return result ? 0 : 1;
}
