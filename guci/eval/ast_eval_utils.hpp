#pragma once

#include <variant>

#include "guci/eval/eval.hpp"
#include "guci/eval/eval_error.hpp"
#include "guci/parse/ast.hpp"

namespace guci {
inline eval_result<Identifier> as_identifier(Term const& t) {
  if (not std::holds_alternative<Identifier>(*t)) {
    return EvalError("term expected to be an identifier");
  }
  return std::get<Identifier>(*t);
}

inline bool has_unbound_variables(EvaluationContext const& ctx, Term const& t) {
  if (std::holds_alternative<Identifier>(*t)) {
    return not ctx.contains(std::get<Identifier>(*t));
  }

  if (std::holds_alternative<List<Term>>(*t)) {
    auto l = std::get<List<Term>>(*t);
    return std::none_of(l.begin(), l.end(), [&](Term const& a_t) {
      return has_unbound_variables(ctx, a_t);
    });
  }

  return false;
}
}  // namespace guci
