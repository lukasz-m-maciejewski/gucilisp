#pragma once

#include <string_view>

inline std::string_view trim(std::string_view sv) {
  std::string_view whitespace = " \t";

  int pos = sv.find_first_not_of(whitespace);
  int rpos = sv.find_last_not_of(whitespace);
  return sv.substr(pos, rpos - pos + 1);
}

inline bool is_one_of(char c, std::string_view chars) {
  for (const char elem : chars) {
    if (c == elem) return true;
  }

  return false;
}

inline bool all_chars_from_set(std::string_view in, std::string_view chars) {
  for (const char c : in) {
    if (not is_one_of(c, chars)) return false;
  }

  return true;
}

inline int parse_digits(std::string_view in) {
  int ret = 0;
  for (const char c : in) {
    int val = c - '0';
    ret = 10 * ret + val;
  }

  return ret;
}
