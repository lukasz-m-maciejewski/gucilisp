#include <readline/history.h>
#include <readline/readline.h>

#include <cstdio>
#include <cstdlib>
#include <memory>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_set>
#include <variant>
#include <vector>

#include "utils/outcome.hpp"
#include "utils/string_manipulation.hpp"

struct CStringDeleter {
  void operator()(char* ptr) { free(ptr); }
};

class Nil {};

class Number {
  int value_;

 public:
  Number(int value) : value_{value} {}

  int value() const { return value_; }
};

class Identifier {
  std::string id;

 public:
  Identifier(std::string_view sv) : id{sv} {}

  std::string const& value() const { return id; }
};

template <typename T>
class List {
  std::vector<T> terms_;

 public:
  List() : terms_{} {}

  List& append(T&& t) {
    terms_.push_back(std::forward<T>(t));

    return *this;
  }

  T const& at(int i) const { return terms_.at(i); }
  int size() const { return terms_.size(); }
};

class Term {
  std::variant<Nil, Identifier, Number, List<Term>> term_;

 public:
  template <typename T>
  Term(T&& t) : term_{std::forward<T>(t)} {}

  auto const& value() const { return term_; }
};

class Expression {
  std::shared_ptr<Term const> term;
  bool parse_error;

 public:
  Expression() : term{nullptr}, parse_error{false} {}
  Expression(std::unique_ptr<Term> a_term)
      : term{std::move(a_term)}, parse_error{false} {}

  static Expression ParseError() {
    Expression e;
    e.parse_error = true;
    return e;
  }

  std::shared_ptr<Term const> release_term() && { return term; }
};

outcome::result<Term> parse_number(std::string_view in) {
  int sign = in[0] == '-' ? -1 : 1;
  std::string_view digits = in[0] == '-' ? in.substr(1) : in;
  if (not all_chars_from_set(digits, "0123456789")) {
    return outcome::failure(std::errc::operation_not_supported);
  }

  return outcome::success(Number{sign * parse_digits(digits)});
}

outcome::result<Term> parse_atom(std::string_view untrimmed) {
  const auto decimal_digits = "0123456789";
  const auto in = trim(untrimmed);
  if (is_one_of(in[0], decimal_digits) or
      (in[0] == '-' and in.size() > 1 and is_one_of(in[1], decimal_digits))) {
    return parse_number(in);
  }

  return outcome::success(Identifier{in});
}

outcome::result<Term> parse(std::string_view);

outcome::result<void> append_terms(List<Term>& l, std::string_view in) {
  std::string_view::size_type begin = 0;
  std::string_view::size_type end = 0;

  while (end != in.size()) {
    while (in[begin] == ' ') ++begin;
    end = begin;  // TODO: should check if paren and find closing paren position
    while (in[end] != ' ' and end != in.size()) ++end;
    auto t = OUTCOME_TRYX(parse(in.substr(begin, end - begin + 1)));
    l.append(std::move(t));
    begin = end + 1;
  }

  return outcome::success();
}

outcome::result<Term> parse(std::string_view untrimmed_input) {
  const auto in = trim(untrimmed_input);
  if (in.empty()) {
    return outcome::success(Nil{});
  }
  if (in[0] != '(') {
    return parse_atom(in);
  }

  if (!in.ends_with(')')) {
    return outcome::failure(std::errc::operation_not_supported);
  }

  const auto inner_in = trim(in.substr(1, in.size() - 2));

  auto list = List<Term>{};
  OUTCOME_TRYV(append_terms(list, inner_in));

  return outcome::success(std::move(list));
}

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

// class EvaluationContext;
class BuiltInFunction {};

class UserDefinedFunction {};

class Function {
  std::variant<BuiltInFunction, UserDefinedFunction> fun;
};

class EvaluatingVisitor {
  Term result_;
  // EvaluationContext context_;
 public:
  Term result() const { return result_; }

  void operator()(List const& l) {
    if (l.size() >= 3) {
      Term const& head = l.at(0);
      if (head.type() == Type::Identifier) {
        if (dynamic_cast<Identifier const*>(&head)->value() == "+") {
        }
      }
    }
  }

  void visit(Number const& n) override {}

  void visit(Identifier const& i)
};

std::string show_result(std::string_view input) {
  Term t = parse(input).value();
  PrintingVisitor v;
  std::visit(v, t.value());

  return v.get();
}

int main(int argc, char** argv) {
  rl_bind_key('\t', rl_complete);

  while (true) {
    std::unique_ptr<char, CStringDeleter> input{
        readline("prompt> ")};  //, free};

    if (input == nullptr) {
      break;
    }

    add_history(input.get());

    auto result = show_result(input.get());
    puts(result.c_str());
  }
}
