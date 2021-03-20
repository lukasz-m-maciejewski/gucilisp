#pragma once

#include <algorithm>
#include <functional>
#include <string_view>

#include "parse/ast.hpp"
#include "parse/parse_error.hpp"
#include "utils/string_manipulation.hpp"

namespace guci {

class PartialParse {
 public:
  Term t;
  std::string_view rest;
};

using MaybeParse = ::guci::parse_result<PartialParse>;

using Parser = std::function<MaybeParse(std::string_view)>;
using JoinOp = std::function<Term(Term, Term)>;

bool is_whitespace(char c) { return is_one_of(c, " \n\t"); }

bool is_alpha(char c) {
  return (c >= 'a' and c <= 'z') or (c >= 'A' and c <= 'Z');
}

bool is_decimal(char c) { return c >= '0' and c <= '9'; }

class skip_one_of {
  std::string_view chars_;

 public:
  skip_one_of(std::string_view chars) : chars_{chars} {}

  MaybeParse operator()(std::string_view in) {
    if (is_one_of(in[0], chars_)) {
      return outcome::success(PartialParse{Nil{}, in.substr(1)});
    }

    return outcome::failure(
        ParseError{ParseErrc::GenericError, "character mismatch"});
  }
};

class kleene_star {
  Parser p_;
  JoinOp op_;

 public:
  kleene_star(Parser p, JoinOp op) : p_{std::move(p)}, op_{std::move(op)} {}

  kleene_star(Parser p) : p_{p}, op_{nullptr} {}

  MaybeParse operator()(std::string_view in) {
    Term ret = Nil{};
    std::string_view rest = in;

    while (auto maybe_result = p_(rest)) {
      PartialParse result = std::move(maybe_result).value();
      if (op_ != nullptr) {
        ret = op_(ret, result.t);
      }
      rest = result.rest;
    }

    return outcome::success(PartialParse{ret, rest});
  }
};

MaybeParse skip_whitespace(std::string_view in) {
  return kleene_star{skip_one_of{" \t\n"}}(in);
}

class alternative {
  std::vector<Parser> ps_;

 public:
  alternative(std::initializer_list<Parser> ps) : ps_{ps} {}

  MaybeParse operator()(std::string_view in) {
    for (auto const& p : ps_) {
      auto res = p(in);
      if (res) return res;
    }

    return outcome::failure(ParseError{ParseErrc::GenericError,
                                       "none of the alternatives matched"});
  }
};

MaybeParse parse_number(std::string_view untrimed) {
  auto const decimal_digits = "0123456789";

  auto [res, in] = skip_whitespace(untrimed).value();

  int sign = in[0] == '-' ? -1 : 1;
  std::string_view::size_type idx = in[0] == '-' ? 1 : 0;

  if (not is_one_of(in[idx], decimal_digits))
    return ParseError{ParseErrc::GenericError, "not a number"};

  int acc = 0;
  while (is_one_of(in[idx], decimal_digits)) {
    acc = 10 * acc + (in[idx] - '0');
    idx++;
  }

  return outcome::success(PartialParse{Number{sign * acc}, in.substr(idx)});
}

MaybeParse parse_identifier(std::string_view untrimmed) {
  auto [res, in] = skip_whitespace(untrimmed).value();
  auto valid_begin = [](char c) {
    return is_alpha(c) or is_one_of(c, "_+-*/%^@?!");
  };
  auto valid_rest = [&](char c) { return valid_begin(c) or is_decimal(c); };

  if (not valid_begin(in[0])) {
    return ParseError{ParseErrc::GenericError, "invalid identifier"};
  }

  // cannot begin in the same way as a number
  if (in[0] == '-' and valid_rest(in[1]) and is_decimal(in[1])) {
    return ParseError{ParseErrc::GenericError,
                      "identifier cannot begin like a number"};
  }

  std::string_view::size_type pos = 1;
  while (valid_rest(in[pos])) ++pos;

  return PartialParse{Identifier{in.substr(0, pos)}, in.substr(pos)};
}

MaybeParse parse_atom(std::string_view untrimmed) {
  return alternative{parse_number, parse_identifier}(untrimmed);
}

MaybeParse parse_list(std::string_view untrimmed) {
  auto [ignored, in_with_paren] = skip_whitespace(untrimmed).value();
  auto res = skip_one_of("(")(in_with_paren);
  if (not res)
    return ParseError{ParseErrc::GenericError, "list should begin with '('"};

  std::string_view rest = res.value().rest;
  List<Term> list{};

  while (auto mp = alternative{parse_atom, parse_list}(rest)) {
    PartialParse p = std::move(mp).value();
    rest = p.rest;
    list.append(std::move(p.t));
  }

  auto end_res = skip_one_of(")")(rest);
  std::string err = "list should end with ')'; rest is: ";
  err += rest;
  if (not end_res) return ParseError(ParseErrc::GenericError, err);

  return PartialParse{std::move(list), end_res.value().rest};
}

MaybeParse parse_term(std::string_view in) {
  return alternative{parse_atom, parse_list}(in);
}

parse_result<Term> parse(std::string_view in) {
  PartialParse result = OUTCOME_TRYX(parse_term(in));
  if (not std::ranges::all_of(result.rest, is_whitespace)) {
    return ParseError(ParseErrc::GenericError, "incomplete parse");
  }

  return result.t;
}

}  // namespace guci
