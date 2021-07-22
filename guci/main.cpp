#include <fmt/core.h>
#include <readline/history.h>
#include <readline/readline.h>
#include <unistd.h>

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
#include <vector>

#include "guci/eval/ast_eval_utils.hpp"
#include "guci/eval/eval.hpp"
#include "guci/eval/eval_error.hpp"
#include "guci/parse/ast.hpp"
#include "guci/parse/parse.hpp"
#include "guci/utils/outcome.hpp"
#include "guci/utils/overload.hpp"
#include "guci/utils/string_manipulation.hpp"

namespace guci {

struct CStringDeleter {
  void operator()(char* ptr) { free(ptr); }
};

using SafeCStr = std::unique_ptr<char, CStringDeleter>;

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

class ActionVisitor {
  EvaluationContext** pctx_;

 public:
  ActionVisitor(EvaluationContext** pctx) : pctx_{pctx} {}
  eval_result<void> operator()(SetValue id_to_value) {
    return (*pctx_)->set_value(id_to_value.id, id_to_value.value);
  }
};

EvaluationResult execute_actions(EvaluationContext** ctx,
                                 EvaluationSuccess result) {
  for (Action const& a : result.as) {
    OUTCOME_TRYV(std::visit(ActionVisitor{ctx}, *a));
  }

  return EvaluationSuccess{result.t};
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
          Function const* f = context_->find_function(id.value());
          if (f == nullptr) return EvalError{"function not found"};

          return apply(*f, *context_, l.tail());
        },
        [](auto const&) -> EvaluationResult {
          return EvalError{"not a function"};
        });
    return execute_actions(&context_,
                           OUTCOME_TRYX(std::visit(v, l.at(0).term())));
  }

  EvaluationResult operator()(Number const& n) { return Term{n}; }

  EvaluationResult operator()(Identifier const& id) {
    Term const* t = context_->find_value(id.value());
    if (t == nullptr) {
      return Term{id};
    }

    return *t;
  }

  EvaluationResult operator()(Nil const&) { return Term{NIL}; }

  EvaluationResult operator()(String const& s) { return Term{s}; }

  EvaluationResult operator()(Boolean const& b) { return Term{b}; }
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

  template <typename T>
  void operator()(T const& t) {
    s << t;
  }
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
      es.merge_action_from(std::move(t_eval.as));
      EvaluationSuccess r = OUTCOME_TRYX(std::visit(*this, *es.t, *t_eval.t));
      es.t = r.t;
      es.merge_action_from(std::move(r.as));
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
    es.merge_action_from(std::move(arg_0_eval.as));

    for (std::size_t i = 1; i < args.size(); ++i) {
      EvaluationSuccess arg_eval =
          OUTCOME_TRYX(std::visit(EvaluatingVisitor{ctx}, *args[i]));
      es.merge_action_from(std::move(arg_eval.as));
      EvaluationSuccess r = OUTCOME_TRYX(std::visit(*this, *es.t, *arg_eval.t));
      es.t = r.t;
      es.merge_action_from(std::move(r.as));
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

class Multiply {
public:
  EvaluationResult operator()(EvaluationContext& ctx,
                              std::span<Term const> ts) {
    EvaluationSuccess es{Number(1)};
    for (auto const& t : ts) {
      EvaluationSuccess t_eval =
          OUTCOME_TRYX(std::visit(EvaluatingVisitor{ctx}, *t));
      es.merge_action_from(std::move(t_eval.as));
      EvaluationSuccess r = OUTCOME_TRYX(std::visit(*this, *es.t, *t_eval.t));
      es.t = r.t;
      es.merge_action_from(std::move(r.as));
    }
    return es;
  }

  EvaluationResult operator()(Number const& lhs, Number const& rhs) {
    return Term{Number{lhs.value() * rhs.value()}};
  }

  template <typename LHS, typename RHS>
  EvaluationResult operator()(LHS const&, RHS const&) {
    return EvalError("unbound variables in arithmetic expression");
  }
};

EvaluationResult builtin_let(EvaluationContext&, std::span<Term const> args) {
  Identifier id = OUTCOME_TRYX(as_identifier(args[0]));
  auto set_value_action = SetValue(id.value(), args[1]);
  return EvaluationSuccess(args[1], {set_value_action});
}

EvaluationResult builtin_eval(EvaluationContext& ctx,
                              std::span<Term const> args) {
  if (args.size() % 2 != 1) {
    return EvalError("mismatched number of local variables and values");
  }
  auto local_ctx = EvaluationContext(&ctx, {}, {});
  for (auto i = 1u; i < args.size(); i += 2) {
    OUTCOME_TRYV(
        local_ctx.set_value(OUTCOME_TRYX(as_identifier(args[i])), args[i + 1]));
  }
  return std::visit(EvaluatingVisitor{local_ctx}, *args[0]);
}

outcome::result<void> RunRepl() {
  rl_bind_key('\t', rl_complete);

  bool program_termination_requested = false;

  EvaluationContext global_context{
      {
          {"+", BuiltInFunction(BuiltInFunction::kAnyArity, Add{})},
          {"-",
           BuiltInFunction(BuiltInFunction::kAnyPositiveArity, Subtract{})},
          {"*", BuiltInFunction(BuiltInFunction::kAnyArity, Multiply{})},
          {"quit",
           BuiltInFunction(0,
                           [&program_termination_requested](
                               EvaluationContext&,
                               std::span<Term const>) -> EvaluationResult {
                             program_termination_requested = true;
                             return EvaluationSuccess(NIL);
                           })},
          {"let", BuiltInFunction(2, builtin_let)},
          {"eval",
           BuiltInFunction(BuiltInFunction::kAnyPositiveArity, builtin_eval)},
      },

      {}};

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
  if (isatty(fileno(stdin))) {
    return RunRepl();
  } else {
    std::array<char, 128> chunk;

    while (fgets(chunk.data(), chunk.size(), stdin) != NULL) {
      fputs(chunk.data(), stdout);
      fputs("|*\n", stdout);
    }
    return outcome::success();
  }
}

}  // namespace guci

int main(int argc, char** argv) {
  std::vector<std::string_view> args{argv, argv + argc};
  auto result = guci::Main(args);
  return result ? 0 : 1;
}
