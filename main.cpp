#include <cstdio>
#include <cstdlib>
#include <readline/readline.h>
#include <readline/history.h>
#include <vector>
#include <memory>
#include <string>
#include <string_view>
#include <sstream>

struct CStringDeleter {

  void operator()(char* ptr) {
    free(ptr);
  }
};

class List;
class Number;
class Identifier;

class TermVisitor {
public:

  virtual ~TermVisitor() = default;
  virtual void visit(List const&) = 0;
  virtual void visit(Number const&) = 0;
  virtual void visit(Identifier const&) = 0;
};

class Term {
public:
  virtual ~Term() = default;

  virtual void accept(TermVisitor&) const = 0;
};

class List : public Term {
  std::unique_ptr<Term> head_;
  std::vector<std::unique_ptr<Term>> tail_;

public:
  List() : head_{nullptr}, tail_{} {}

  List& append(std::unique_ptr<Term> t) {
    if (head_ == nullptr) {
      head_ = std::move(t);
    } else {
      tail_.push_back(std::move(t));
    }

    return *this;
  }

  Term const& head() const { return *head_; }
  Term const& tail(int i) const { return *tail_[i]; }
  int tail_size() const { return tail_.size(); }

  void accept(TermVisitor& v) const override {
    v.visit(*this);
  }
};

class Atom : public Term {

};

class Number : public Atom {
  int value;

public:
  Number(int a_value) : value{a_value} { }

  void accept(TermVisitor& v) const override {
   v.visit(*this);
  }
};

class Identifier : public Atom {
  std::string id;

public:
  Identifier(std::string_view sv) : id{sv} {}

  void accept(TermVisitor& v) const override {
    v.visit(*this);
  }

};

class Expression {
  std::unique_ptr<Term> term;
  bool parse_error;

public:
  Expression() : term{nullptr}, parse_error{false} {}
  Expression(std::unique_ptr<Term> a_term) : term{std::move(a_term)}, parse_error{false} {}

  static Expression ParseError() {
    Expression e;
    e.parse_error = true;
    return e;
  }

  std::unique_ptr<Term> release_term() && {
    return std::move(term);
  }

  void accept(TermVisitor& v) {
    term->accept(v);
  }
};



std::string_view trim(std::string_view sv) {
  std::string_view whitespace = " \t";
  
  int pos = sv.find_first_not_of(whitespace);
  int rpos = sv.find_last_not_of(whitespace);
  return sv.substr(pos, rpos - pos + 1);
}

bool is_one_of(char c, std::string_view chars) {
  for (const char elem : chars) {
    if (c == elem)
      return true;
  }

  return false;
}

bool all_chars_from_set(std::string_view in, std::string_view chars) {
  for (const char c : in) {
    if (not is_one_of(c, chars))
      return false;
  }

  return true;
}

int parse_digits(std::string_view in) {
  int ret = 0;
  for (const char c : in) {
    int val = c - '0';
    ret = 10 * ret + val;
  }

  return ret;
}

std::unique_ptr<Number> parse_number(std::string_view in) {
  int sign = in[0] == '-' ? -1 : 1;
  std::string_view digits = in[0] == '-' ? in.substr(1) : in;
  if (not all_chars_from_set(digits, "0123456789")) {
    return nullptr;
  }

  return std::make_unique<Number>(sign * parse_digits(digits));
}

std::unique_ptr<Atom> parse_atom(std::string_view in) {
  const auto trimmed = trim(in);
  if (is_one_of(trimmed[0], "-0123456789")) {
    return parse_number(trimmed);
  }

  return std::make_unique<Identifier>(trimmed);
}

Expression parse(std::string_view);

void append_terms(List& l, std::string_view in) {
  std::string_view::size_type begin = 0;
  std::string_view::size_type end = 0;

  while (end != in.size()) {
    while (in[begin] == ' ') ++begin;
    end = begin; // TODO: should check if paren and find closing paren position
    while (in[end] != ' ' and end != in.size()) ++end;
    l.append(parse(in.substr(begin, end - begin + 1)).release_term());
    begin = end + 1;
  }
}

Expression parse(std::string_view untrimmed_input) {
  const auto in = trim(untrimmed_input);
  if (in.empty()) {
    return Expression{};
  }
  if (in[0] != '(') {
    return Expression{parse_atom(in)};
  }

  if (!in.ends_with(')')) {
    return Expression::ParseError();
  }

  const auto inner_in = trim(in.substr(1, in.size() - 2));


  auto list = std::make_unique<List>();
  append_terms(*list, inner_in);

  return Expression(std::move(list));
}

class TypePrintingVisitor : public TermVisitor {
  std::stringstream s;

public:
  std::string get() { return s.str(); }

  void visit(List const& l) override{
    s << "LIST";
  }

  void visit(Number const& n) override {
    s << "NUMBER";
  }

  void visit(Identifier const& i) override {
    s << "IDENTIFIER";
  }
};

std::string show_result(std::string_view input) {
  Expression e = parse(input);
  TypePrintingVisitor v;
  e.accept(v);

  return v.get();
}


int main(int argc, char** argv) {
  rl_bind_key('\t', rl_complete);

  while(true) {
    std::unique_ptr<char, CStringDeleter> input{readline("prompt> ")}; //, free};

    if (input == nullptr) {
      break;
    }

    add_history(input.get());

    auto result = show_result(input.get());
    puts(result.c_str());
  }
}
