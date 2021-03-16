#pragma once

#include <utility>

namespace guci {
template <typename... Fs>
struct overload : Fs... {
  template <typename... Ts>
  overload(Ts&&... ts) : Fs{std::forward<Ts>(ts)}... {}

  using Fs::operator()...;
};

template <typename... Ts>
overload(Ts&&...) -> overload<std::remove_reference_t<Ts>...>;

}  // namespace guci
