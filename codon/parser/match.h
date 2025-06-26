// Copyright (C) 2022-2025 Exaloop Inc. <https://exaloop.io>

#pragma once

#include <string>

#include "codon/cir/base.h"

namespace codon::matcher {

template <typename T, typename... MA> struct match_t {
  std::tuple<MA...> args;
  std::function<void(T &)> fn;
  match_t(MA... args, std::function<void(T &)> fn)
      : args(std::tuple<MA...>(args...)), fn(fn) {}
};

template <typename... MA> struct match_or_t {
  std::tuple<MA...> args;
  match_or_t(MA... args) : args(std::tuple<MA...>(args...)) {}
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

template <typename T, typename... TA> match_t<T, TA...> M(TA... args) {
  return match_t<T, TA...>(args..., nullptr);
}

template <typename T, typename... TA>
match_t<T, TA...> MCall(TA... args, std::function<void(T &)> fn) {
  return match_t<T, TA...>(args..., fn);
}

template <typename T, typename... TA> match_t<T, TA...> MVar(TA... args, T &tp) {
  return match_t<T, TA...>(args..., [&tp](T &t) { tp = t; });
}

template <typename T, typename... TA> match_t<T, TA...> MVar(TA... args, T *&tp) {
  return match_t<T, TA...>(args..., [&tp](T &t) { tp = &t; });
}

template <typename... TA> match_or_t<TA...> MOr(TA... args) {
  return match_or_t<TA...>(args...);
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

template <int i, typename T, typename TM> bool match_help(T &t, TM m) {
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

template <int i, typename T, typename... TO>
bool match_or_help(T &t, match_or_t<TO...> m) {
  if constexpr (i >= 0 && i < std::tuple_size_v<decltype(m.args)>) {
    return match(t, std::get<i>(m.args)) || match_or_help<i + 1>(t, m);
  } else {
    return false;
  }
}

template <typename TM, typename... TA> bool match(TM &t, match_or_t<TA...> m) {
  return match_or_help<0, TM, TA...>(t, m);
}

template <typename TM, typename... TA> bool match(TM *t, match_or_t<TA...> m) {
  return match_or_help<0, TM *, TA...>(t, m);
}

template <typename T, typename TM, typename... TA>
bool match(T &t, match_t<TM, TA...> m) {
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
bool match(T *t, match_t<TM, TA...> m) {
  return match<T *, TM, TA...>(t, m);
}

} // namespace codon::matcher

#define M_ matcher::match_ignore_t()
