// Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>

#pragma once

#include <functional>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>

#include "codon/cir/base.h"

namespace codon::matcher {

template <typename T, typename... MA> struct match_t {
  std::tuple<MA...> args;
  std::function<void(T &)> fn;
  template <typename... Args>
  match_t(std::function<void(T &)> fn, Args &&...args)
      : args(std::forward<Args>(args)...), fn(std::move(fn)) {}
};

template <typename... MA> struct match_or_t {
  std::tuple<MA...> args;
  template <typename... Args>
  explicit match_or_t(Args &&...args) : args(std::forward<Args>(args)...) {}
};

struct match_ignore_t {};

struct match_zero_or_more_t {};

struct match_startswith_t {
  std::string s;
};

struct match_endswith_t {
  std::string s;
};

struct match_contains_t {
  std::string s;
};

template <typename T, typename... TA> auto M(TA &&...args) {
  return match_t<T, std::decay_t<TA>...>(nullptr, std::forward<TA>(args)...);
}

template <typename T, typename... TA>
auto MCall(TA &&...args, std::function<void(T &)> fn) {
  return match_t<T, std::decay_t<TA>...>(std::move(fn), std::forward<TA>(args)...);
}

template <typename T, typename... TA> auto MVar(TA &&...args, T &tp) {
  return match_t<T, std::decay_t<TA>...>([&tp](T &t) { tp = t; },
                                         std::forward<TA>(args)...);
}

template <typename T, typename... TA> auto MVar(TA &&...args, T *&tp) {
  return match_t<T, std::decay_t<TA>...>([&tp](T &t) { tp = &t; },
                                         std::forward<TA>(args)...);
}

template <typename... TA> auto MOr(TA &&...args) {
  return match_or_t<std::decay_t<TA>...>(std::forward<TA>(args)...);
}

match_zero_or_more_t MAny();

match_startswith_t MStarts(std::string s);

match_endswith_t MEnds(std::string s);

match_contains_t MContains(std::string s);

//////////////////////////////////////////////////////////////////////////////

template <class T, class M> bool match(T t, M m) {
  if constexpr (std::is_same_v<T, M>)
    return t == m;
  return false;
}

template <class T> bool match(T &t, match_ignore_t) { return true; }

template <class T> bool match(T &t, match_zero_or_more_t) { return true; }

template <> bool match(const char *c, const char *d);

template <> bool match(const char *c, std::string d);

template <> bool match(std::string c, const char *d);

template <> bool match(double &a, double b);

template <> bool match(std::string s, match_startswith_t m);

template <> bool match(std::string s, match_endswith_t m);

template <> bool match(std::string s, match_contains_t m);

template <> bool match(const char *s, match_startswith_t m);

template <> bool match(const char *s, match_endswith_t m);

template <> bool match(const char *s, match_contains_t m);

template <int i, typename T, typename TM> bool match_help(T &t, const TM &m) {
  if constexpr (i == std::tuple_size_v<decltype(m.args)>) {
    return i == std::tuple_size_v<decltype(t.match_members())>;
  } else if constexpr (i < std::tuple_size_v<decltype(m.args)>) {
    if constexpr (std::is_same_v<std::remove_reference_t<decltype(std::get<i>(m.args))>,
                                 match_zero_or_more_t>) {
      return true;
    }
    return match(std::get<i>(t.match_members()), std::get<i>(m.args)) &&
           match_help<i + 1>(t, m);
  } else {
    return false;
  }
}

template <typename T, typename... TA> bool match_or(T &t, const match_or_t<TA...> &m) {
  static_assert(sizeof...(TA) > 0, "match_or_t requires at least one alternative");
  return std::apply([&](const auto &...alts) { return (... || match(t, alts)); },
                    m.args);
}

template <typename TM, typename... TA> bool match(TM &t, const match_or_t<TA...> &m) {
  return match_or(t, m);
}

template <typename TM, typename... TA> bool match(TM *t, const match_or_t<TA...> &m) {
  return match_or<TM *>(t, m);
}

template <typename T, typename TM, typename... TA>
bool match(T &t, const match_t<TM, TA...> &m) {
  if constexpr (std::is_pointer_v<T>) {
    TM *tm = ir::cast<TM>(t);
    if (!tm)
      return false;
    if constexpr (sizeof...(TA) == 0) {
      if (m.fn)
        m.fn(*tm);
      return true;
    } else {
      auto r = match_help<0>(*tm, m);
      if (r && m.fn)
        m.fn(*tm);
      return r;
    }
  } else {
    if constexpr (!std::is_same_v<T, TM>)
      return false;
    if constexpr (sizeof...(TA) == 0) {
      if (m.fn)
        m.fn(t);
      return true;
    } else {
      auto r = match_help<0>(t, m);
      if (r && m.fn)
        m.fn(t);
      return r;
    }
  }
}

template <typename T, typename TM, typename... TA>
bool match(T *t, const match_t<TM, TA...> &m) {
  return match<T *, TM, TA...>(t, m);
}

} // namespace codon::matcher

#define M_ matcher::match_ignore_t()
