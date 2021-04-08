#include "guci/eval/ast_eval_utils.hpp"
#include "catch2/catch.hpp"
#include "guci/parse/parse.hpp"

namespace guci {

TEST_CASE("has_unbound_variable", "[eval]") {
  Term t = Identifier("abc");
  EvaluationContext c{{}, {}};

  bool result = has_unbound_variables(c, t);
  REQUIRE(result == true);
}

TEST_CASE("expression with no variables") {
  Term t = parse("(+ 1 2 3)").value();
  EvaluationContext c{{},{}};

  bool result = has_unbound_variables(c, t);
  REQUIRE(result == false);
}
}  // namespace guci
