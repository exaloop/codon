// Copyright (C) 2022-2024 Exaloop Inc. <https://exaloop.io>

#include "codon/parser/match.h"

namespace codon::matcher {

match_zero_or_more_t MAny() { return match_zero_or_more_t(); }

match_startswith_t MStarts(std::string s) { return match_startswith_t{std::move(s)}; }

match_endswith_t MEnds(std::string s) { return match_endswith_t{std::move(s)}; }

match_contains_t MContains(std::string s) { return match_contains_t{std::move(s)}; }

template <> bool match(const char *c, const char *d) {
  return std::string(c) == std::string(d);
}

template <> bool match(const char *c, std::string d) { return std::string(c) == d; }

template <> bool match(std::string c, const char *d) { return std::string(d) == c; }

template <> bool match(double &a, double b) { return abs(a - b) < __FLT_EPSILON__; }

template <> bool match(std::string s, match_startswith_t m) {
  return m.s.size() <= s.size() && s.substr(0, m.s.size()) == m.s;
}

template <> bool match(std::string s, match_endswith_t m) {
  return m.s.size() <= s.size() && s.substr(s.size() - m.s.size(), m.s.size()) == m.s;
}

template <> bool match(std::string s, match_contains_t m) {
  return s.find(m.s) != std::string::npos;
}

template <> bool match(const char *s, match_startswith_t m) {
  return match(std::string(s), m);
}

template <> bool match(const char *s, match_endswith_t m) {
  return match(std::string(s), m);
}

template <> bool match(const char *s, match_contains_t m) {
  return match(std::string(s), m);
}

}
