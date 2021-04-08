#pragma once

#include <string>
#include <variant>

#include "guci/parse/ast.hpp"

namespace guci {
class SetValue {
 public:
  std::string id;
  Term value;
};

class Action {
 public:
  using ActionType = std::variant<SetValue>;
  ActionType a;
  ActionType& operator*() { return a; }
  ActionType const& operator*() const { return a; }

  Action(Action const&) = default;
  Action& operator=(Action const&) = default;
  Action(Action&&) = default;
  Action& operator=(Action&&) = default;

  Action(SetValue& action) : a{action} {}
  Action(SetValue&& action) : a{std::move(action)} {}
};
}  // namespace guci
