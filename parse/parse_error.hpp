#pragma once

#include "utils/outcome.hpp"

namespace guci {

enum class ParseErrc {
  Success = 0,
  GenericError = 1,
};
}  // namespace guci

namespace std {
// Tell the C++ 11 STL metaprogramming that enum ConversionErrc
// is registered with the standard error code system
template <>
struct is_error_code_enum<guci::ParseErrc> : true_type {};
}  // namespace std

namespace guci {
namespace detail {
// Define a custom error code category derived from std::error_category
class ParseErrc_category : public std::error_category {
 public:
  // Return a short descriptive name for the category
  virtual char const* name() const noexcept override final {
    return "ParseError";
  }
  // Return what each enum means in text
  virtual std::string message(int c) const override final {
    switch (static_cast<ParseErrc>(c)) {
      case ParseErrc::Success:
        return "parse successful";
      case ParseErrc::GenericError:
        return "converting empty string";
      default:
        return "unknown";
    }
  }
  // OPTIONAL: Allow generic error conditions to be compared to me
  virtual std::error_condition default_error_condition(
      int c) const noexcept override final {
    switch (static_cast<ParseErrc>(c)) {
      case ParseErrc::GenericError:
        return make_error_condition(std::errc::invalid_argument);
      default:
        // I have no mapping for this code
        return std::error_condition(c, *this);
    }
  }
};
}  // namespace detail

// Declare a global function returning a static instance of the custom category
extern inline const detail::ParseErrc_category& ParseErrc_category() {
  static detail::ParseErrc_category c;
  return c;
}

// Overload the global make_error_code() free function with our
// custom enum. It will be found via ADL by the compiler if needed.
inline std::error_code make_error_code(ParseErrc e) {
  return {static_cast<int>(e), ParseErrc_category()};
}

struct ParseError {
  ParseErrc ec;
  std::string msg;
};

inline std::error_code make_error_code(ParseError const& err) { return err.ec; }

inline void outcome_throw_as_system_error_with_payload(ParseError const& err) {
  throw std::runtime_error{err.msg};
}

template <class T>
using parse_result = outcome::result<T, ParseError>;

}  // namespace guci
