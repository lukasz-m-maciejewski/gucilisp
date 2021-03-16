#include <readline/history.h>
#include <readline/readline.h>

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

#include "utils/outcome.hpp"
#include "utils/overload.hpp"
#include "utils/string_manipulation.hpp"
#include "parse/ast.hpp"
#include "parse/parse.hpp"

namespace guci {

struct CStringDeleter {
  void operator()(char* ptr) { free(ptr); }
};

using SafeCStr = std::unique_ptr<char, CStringDeleter>;

class PrintingVisitor {
  std::stringstream s;

 public:
  std::string get() { return s.str(); }

  void operator()(List<Term> const& l) {
    s << '[';
    const auto num_elements = l.size();
    for (int i = 0; i < num_elements; ++i) {
      if (i > 0) s << ' ';
      std::visit(*this, l.at(i).value());
    }
    s << ']';
  }

  void operator()(Number const& n) { s << n.value(); }

  void operator()(Identifier const& i) { s << i.value(); }

  void operator()(Nil const&) { s << "NIL"; }
};

class Action {};

class Error {};

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
  auto const& operator()() const { return fun_; }
};

class EvaluationContext {
  std::unordered_map<std::string, Function> values_;
  EvaluationContext const* parent_;

 public:
  EvaluationContext(
      std::initializer_list<std::pair<const std::string, Function>> values)
      : values_{values}, parent_{nullptr} {}
  EvaluationContext(
      EvaluationContext const* parent,
      std::initializer_list<std::pair<std::string const, Function>> values)
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
          return Error{};
        }
        return f.apply(ctx, args);
      },
      [](UserDefinedFunction const&) -> EvaluationResult { return Error{}; });
  return std::visit(v, fun());
}

class EvaluatingVisitor {
  EvaluationContext context_;

 public:
  EvaluationResult operator()(List<Term> const& l) {
    if (l.empty()) return Term{Nil{}};

    auto v = overload(
        [this, &l](Identifier const& id) -> EvaluationResult {
          Function const* f = context_.find(id.value());
          if (f == nullptr) return Error{};
          return apply(*f, context_, l.tail());
        },
        [](auto const&) -> EvaluationResult { return Error{}; });
    return std::visit(v, l.at(0).value());
  }

  Term operator()(Number const& n) { return n; }

  Term operator()(Identifier const& id) { return id; }

  Term operator()(Nil const&) { return Nil{}; }
};

std::string show_result(std::string_view input) {
  Term t = parse(input).value();
  PrintingVisitor v;
  std::visit(v, t.value());

  return v.get();
}

outcome::result<void> Main(std::vector<std::string_view> const& args
                           [[maybe_unused]]) {
  rl_bind_key('\t', rl_complete);

  while (true) {
    SafeCStr input{readline("prompt> ")};

    if (input == nullptr) {
      break;
    }

    add_history(input.get());

    auto result = show_result(input.get());
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
