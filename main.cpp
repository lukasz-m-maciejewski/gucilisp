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
#include <vector>

#include "parse/ast.hpp"
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

class Error {
  std::string msg_;

 public:
  Error(std::string_view msg) : msg_{msg} {}
  std::string const& msg() const { return msg_; }
};

using EvaluationResult = std::variant<Term, Action, Error>;

class EvaluationContext;

// Design decision: expression cannot modify global context while it's begin
// evaluated; a context modification can be either done by spawning a child
// context or by returning an action modifying global context, to be apply after
// evaluation is complete.

class BuiltInFunction {
  using Impl = std::function<EvaluationResult(EvaluationContext const&,
                                              std::span<Term const>)>;

  int arity_;
  Impl fun_;

 public:
  BuiltInFunction(int arity, Impl fun) : arity_{arity}, fun_{fun} {}

  int arity() const { return arity_; }
  EvaluationResult apply(EvaluationContext const& ctx,
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

EvaluationResult apply(Function const& fun, EvaluationContext const& ctx,
                       std::span<Term const> args) {
  auto const v = overload(
      [&](BuiltInFunction const& f) -> EvaluationResult {
        if (static_cast<std::size_t>(f.arity()) != args.size()) {
          return Error{"arity mismatch"};
        }
        return f.apply(ctx, args);
      },
      [](UserDefinedFunction const&) -> EvaluationResult {
        return Error{"not defined"};
      });
  return std::visit(v, fun());
}

class EvaluatingVisitor {
  EvaluationContext& global_context_;
  EvaluationContext* context_;

 public:
  EvaluatingVisitor(EvaluationContext& global_context)
      : global_context_{global_context},
        context_{std::addressof(global_context)} {}

  EvaluationResult operator()(List<Term> const& l) {
    if (l.empty()) return Term{Nil{}};

    auto v = overload(
        [this, &l](Identifier const& id) -> EvaluationResult {
          Function const* f = context_->find(id.value());
          if (f == nullptr) return Error{"function not found"};

          auto const tail = eval_tail(l.tail());
          return apply(*f, *context_, tail);
        },
        [](auto const&) -> EvaluationResult {
          return Error{"not a function"};
        });
    return std::visit(v, l.at(0).term());
  }

  EvaluationResult operator()(Number const& n) { return Term{n}; }

  EvaluationResult operator()(Identifier const& id) { return Term{id}; }

  EvaluationResult operator()(Nil const&) { return Term{NIL}; }

 private:
  std::vector<Term> eval_tail(std::span<Term const> tail) {
    std::vector<Term> evaluated_tail;
    evaluated_tail.reserve(tail.size());
    std::ranges::transform(
        tail, std::back_inserter(evaluated_tail), [this](Term const& t) {
          Term res = std::get<0>(std::visit(*this, t.term()));
          return res;
        });

    return evaluated_tail;
  }
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

  void operator()(Term const& t) { std::visit(*this, t.term()); }

  void operator()(Action const&) { s << "ACTION"; }
  void operator()(Error const& e) { s << "ERROR: " << e.msg(); }
};

std::string show_result(EvaluationContext& context, std::string_view input) {
  Term t = parse(input).value();

  EvaluatingVisitor ev(context);
  EvaluationResult res = std::visit(ev, t.term());

  PrintingVisitor v;
  std::visit(v, res);

  return v.get();
}

class Add {
 public:
  EvaluationResult operator()(EvaluationContext const&, std::span<Term const>) {
    return Term{Number{3}};
  }
};

class Subtract {
 public:
  EvaluationResult operator()(EvaluationContext const&, std::span<Term const>) {
    return Term{Number{1}};
  }
};

outcome::result<void> Main(std::vector<std::string_view> const& args
                           [[maybe_unused]]) {
  rl_bind_key('\t', rl_complete);

  EvaluationContext global_context{
      {{"+", BuiltInFunction(2, Add{})},
       {"-", BuiltInFunction(2, Subtract{})}}};

  while (true) {
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

}  // namespace guci

int main(int argc, char** argv) {
  std::vector<std::string_view> args{argv, argv + argc};
  auto result = guci::Main(args);
  return result ? 0 : 1;
}
