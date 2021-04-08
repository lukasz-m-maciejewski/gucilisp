#pragma once

#include "guci/eval/actions.hpp"
#include "guci/eval/eval_error.hpp"

namespace guci {
class EvaluationSuccess {
 public:
  Term t;
  std::vector<Action> as;

  EvaluationSuccess(Term term) : t{term}, as{} {}
  EvaluationSuccess(Term term, std::vector<Action> actions)
      : t{term}, as(std::move(actions)) {}

  void merge_action_from(std::vector<Action>&& other) {
    as.insert(as.end(), other.begin(), other.end());
  }
};

using EvaluationResult = eval_result<EvaluationSuccess>;
}  // namespace guci
