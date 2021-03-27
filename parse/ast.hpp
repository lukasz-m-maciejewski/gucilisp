#pragma once

#include <algorithm>
#include <functional>
#include <ostream>
#include <span>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace guci {

class Nil {
 public:
  bool operator==(Nil) const { return true; }
  bool operator!=(Nil) const { return false; }
};

inline Nil const NIL{};

class Number {
  int value_;

 public:
  Number(int value) : value_{value} {}
  bool operator==(Number const&) const = default;

  int value() const { return value_; }
};

class Identifier {
  std::string id;

 public:
  Identifier(std::string_view sv) : id{sv} {}
  bool operator==(Identifier const&) const = default;

  std::string const& value() const { return id; }
};

class String {
  std::string value_;

 public:
  String(std::string_view sv) : value_{sv} {}
  bool operator==(String const&) const = default;

  std::string const& value() const { return value_; }

  friend std::ostream& operator<<(std::ostream& out, String const& s) {
    return out << '"' << s.value() << '"';
  }
};

template <typename T>
class List {
  std::vector<T> terms_;

 public:
  List() : terms_{} {}
  List(std::initializer_list<T> ts) : terms_{ts} {}

  List& append(T&& t) {
    terms_.push_back(std::forward<T>(t));

    return *this;
  }

  bool operator==(List const&) const = default;

  T const& at(int i) const { return terms_.at(i); }

  int size() const { return terms_.size(); }

  bool empty() const { return terms_.empty(); }

  std::span<T const> tail() const {
    if (terms_.begin() == terms_.end()) return {};
    return {terms_.begin() + 1, terms_.end()};
  }
};

class Term {
  using ValueType = std::variant<Nil, Identifier, Number, String, List<Term>>;
  ValueType term_;

 public:
  Term(Term const&) = default;
  Term& operator=(Term const&) = default;
  Term(Term&&) = default;
  Term& operator=(Term&&) = default;

  Term(Nil v) : term_{std::move(v)} {}
  Term(Identifier v) : term_{std::move(v)} {}
  Term(Number v) : term_{std::move(v)} {}
  Term(String v) : term_{std::move(v)} {}
  Term(List<Term> v) : term_{std::move(v)} {}

  template <typename T>
  friend bool operator==(Term const& lhs, T const& rhs) {
    return lhs.term_ == rhs;
  }

  auto const& term() const { return term_; }
  auto const& operator*() const { return term_; }
};

}  // namespace guci
