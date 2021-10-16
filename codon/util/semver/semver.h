//          _____                            _   _
//         / ____|                          | | (_)
//        | (___   ___ _ __ ___   __ _ _ __ | |_ _  ___
//         \___ \ / _ \ '_ ` _ \ / _` | '_ \| __| |/ __|
//         ____) |  __/ | | | | | (_| | | | | |_| | (__
//        |_____/ \___|_| |_| |_|\__,_|_| |_|\__|_|\___|
// __      __           _             _                _____
// \ \    / /          (_)           (_)              / ____|_     _
//  \ \  / /__ _ __ ___ _  ___  _ __  _ _ __   __ _  | |   _| |_ _| |_
//   \ \/ / _ \ '__/ __| |/ _ \| '_ \| | '_ \ / _` | | |  |_   _|_   _|
//    \  /  __/ |  \__ \ | (_) | | | | | | | | (_| | | |____|_|   |_|
//     \/ \___|_|  |___/_|\___/|_| |_|_|_| |_|\__, |  \_____|
// https://github.com/Neargye/semver           __/ |
// version 0.3.0                              |___/
//
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.
// SPDX-License-Identifier: MIT
// Copyright (c) 2018 - 2021 Daniil Goncharov <neargye@gmail.com>.
// Copyright (c) 2020 - 2021 Alexander Gorbunov <naratzul@gmail.com>.
//
// Permission is hereby  granted, free of charge, to any  person obtaining a copy
// of this software and associated  documentation files (the "Software"), to deal
// in the Software  without restriction, including without  limitation the rights
// to  use, copy,  modify, merge,  publish, distribute,  sublicense, and/or  sell
// copies  of  the Software,  and  to  permit persons  to  whom  the Software  is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE  IS PROVIDED "AS  IS", WITHOUT WARRANTY  OF ANY KIND,  EXPRESS OR
// IMPLIED,  INCLUDING BUT  NOT  LIMITED TO  THE  WARRANTIES OF  MERCHANTABILITY,
// FITNESS FOR  A PARTICULAR PURPOSE AND  NONINFRINGEMENT. IN NO EVENT  SHALL THE
// AUTHORS  OR COPYRIGHT  HOLDERS  BE  LIABLE FOR  ANY  CLAIM,  DAMAGES OR  OTHER
// LIABILITY, WHETHER IN AN ACTION OF  CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE  OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#ifndef NEARGYE_SEMANTIC_VERSIONING_HPP
#define NEARGYE_SEMANTIC_VERSIONING_HPP

#define SEMVER_VERSION_MAJOR 0
#define SEMVER_VERSION_MINOR 3
#define SEMVER_VERSION_PATCH 0

#include <cstddef>
#include <cstdint>
#include <iosfwd>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#if __has_include(<charconv>)
#include <charconv>
#else
#include <system_error>
#endif

// Allow to disable exceptions.
#if (defined(__cpp_exceptions) || defined(__EXCEPTIONS) || defined(_CPPUNWIND)) &&     \
    !defined(SEMVER_NOEXCEPTION)
#include <stdexcept>
#define NEARGYE_THROW(exception) throw exception
#else
#include <cstdlib>
#define NEARGYE_THROW(exception) std::abort()
#endif

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored                                                       \
    "-Wmissing-braces" // Ignore warning: suggest braces around initialization of
                       // subobject 'return {first, std::errc::invalid_argument};'.
#endif

namespace semver {

enum struct prerelease : std::uint8_t { alpha = 0, beta = 1, rc = 2, none = 3 };

#if __has_include(<charconv>)
struct from_chars_result : std::from_chars_result {
  [[nodiscard]] constexpr operator bool() const noexcept { return ec == std::errc{}; }
};

struct to_chars_result : std::to_chars_result {
  [[nodiscard]] constexpr operator bool() const noexcept { return ec == std::errc{}; }
};
#else
struct from_chars_result {
  const char *ptr;
  std::errc ec;

  [[nodiscard]] constexpr operator bool() const noexcept { return ec == std::errc{}; }
};

struct to_chars_result {
  char *ptr;
  std::errc ec;

  [[nodiscard]] constexpr operator bool() const noexcept { return ec == std::errc{}; }
};
#endif

// Max version string length = 3(<major>) + 1(.) + 3(<minor>) + 1(.) + 3(<patch>) + 1(-)
// + 5(<prerelease>) + 1(.) + 3(<prereleaseversion>) = 21.
inline constexpr auto max_version_string_length = std::size_t{21};

namespace detail {

inline constexpr auto alpha = std::string_view{"alpha", 5};
inline constexpr auto beta = std::string_view{"beta", 4};
inline constexpr auto rc = std::string_view{"rc", 2};

// Min version string length = 1(<major>) + 1(.) + 1(<minor>) + 1(.) + 1(<patch>) = 5.
inline constexpr auto min_version_string_length = 5;

constexpr char to_lower(char c) noexcept {
  return (c >= 'A' && c <= 'Z') ? static_cast<char>(c + ('a' - 'A')) : c;
}

constexpr bool is_digit(char c) noexcept { return c >= '0' && c <= '9'; }

constexpr bool is_space(char c) noexcept { return c == ' '; }

constexpr bool is_operator(char c) noexcept { return c == '<' || c == '>' || c == '='; }

constexpr bool is_dot(char c) noexcept { return c == '.'; }

constexpr bool is_logical_or(char c) noexcept { return c == '|'; }

constexpr bool is_hyphen(char c) noexcept { return c == '-'; }

constexpr bool is_letter(char c) noexcept {
  return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}

constexpr std::uint8_t to_digit(char c) noexcept {
  return static_cast<std::uint8_t>(c - '0');
}

constexpr std::uint8_t length(std::uint8_t x) noexcept {
  return x < 10 ? 1 : (x < 100 ? 2 : 3);
}

constexpr std::uint8_t length(prerelease t) noexcept {
  if (t == prerelease::alpha) {
    return static_cast<std::uint8_t>(alpha.length());
  } else if (t == prerelease::beta) {
    return static_cast<std::uint8_t>(beta.length());
  } else if (t == prerelease::rc) {
    return static_cast<std::uint8_t>(rc.length());
  }

  return 0;
}

constexpr bool equals(const char *first, const char *last,
                      std::string_view str) noexcept {
  for (std::size_t i = 0; first != last && i < str.length(); ++i, ++first) {
    if (to_lower(*first) != to_lower(str[i])) {
      return false;
    }
  }

  return true;
}

constexpr char *to_chars(char *str, std::uint8_t x, bool dot = true) noexcept {
  do {
    *(--str) = static_cast<char>('0' + (x % 10));
    x /= 10;
  } while (x != 0);

  if (dot) {
    *(--str) = '.';
  }

  return str;
}

constexpr char *to_chars(char *str, prerelease t) noexcept {
  const auto p = t == prerelease::alpha  ? alpha
                 : t == prerelease::beta ? beta
                 : t == prerelease::rc   ? rc
                                         : std::string_view{};

  if (p.size() > 0) {
    for (auto it = p.rbegin(); it != p.rend(); ++it) {
      *(--str) = *it;
    }
    *(--str) = '-';
  }

  return str;
}

constexpr const char *from_chars(const char *first, const char *last,
                                 std::uint8_t &d) noexcept {
  if (first != last && is_digit(*first)) {
    std::int32_t t = 0;
    for (; first != last && is_digit(*first); ++first) {
      t = t * 10 + to_digit(*first);
    }
    if (t <= (std::numeric_limits<std::uint8_t>::max)()) {
      d = static_cast<std::uint8_t>(t);
      return first;
    }
  }

  return nullptr;
}

constexpr const char *from_chars(const char *first, const char *last,
                                 prerelease &p) noexcept {
  if (is_hyphen(*first)) {
    ++first;
  }

  if (equals(first, last, alpha)) {
    p = prerelease::alpha;
    return first + alpha.length();
  } else if (equals(first, last, beta)) {
    p = prerelease::beta;
    return first + beta.length();
  } else if (equals(first, last, rc)) {
    p = prerelease::rc;
    return first + rc.length();
  }

  return nullptr;
}

constexpr bool check_delimiter(const char *first, const char *last, char d) noexcept {
  return first != last && first != nullptr && *first == d;
}

} // namespace detail

struct version {
  std::uint8_t major = 0;
  std::uint8_t minor = 1;
  std::uint8_t patch = 0;
  prerelease prerelease_type = prerelease::none;
  std::uint8_t prerelease_number = 0;

  constexpr version(std::uint8_t mj, std::uint8_t mn, std::uint8_t pt,
                    prerelease prt = prerelease::none, std::uint8_t prn = 0) noexcept
      : major{mj}, minor{mn}, patch{pt}, prerelease_type{prt},
        prerelease_number{prt == prerelease::none ? static_cast<std::uint8_t>(0)
                                                  : prn} {}

  explicit constexpr version(std::string_view str)
      : version(0, 0, 0, prerelease::none, 0) {
    from_string(str);
  }

  constexpr version() =
      default; // https://semver.org/#how-should-i-deal-with-revisions-in-the-0yz-initial-development-phase

  constexpr version(const version &) = default;

  constexpr version(version &&) = default;

  ~version() = default;

  version &operator=(const version &) = default;

  version &operator=(version &&) = default;

  [[nodiscard]] constexpr from_chars_result from_chars(const char *first,
                                                       const char *last) noexcept {
    if (first == nullptr || last == nullptr ||
        (last - first) < detail::min_version_string_length) {
      return {first, std::errc::invalid_argument};
    }

    auto next = first;
    if (next = detail::from_chars(next, last, major);
        detail::check_delimiter(next, last, '.')) {
      if (next = detail::from_chars(++next, last, minor);
          detail::check_delimiter(next, last, '.')) {
        if (next = detail::from_chars(++next, last, patch); next == last) {
          prerelease_type = prerelease::none;
          prerelease_number = 0;
          return {next, std::errc{}};
        } else if (detail::check_delimiter(next, last, '-')) {
          if (next = detail::from_chars(next, last, prerelease_type); next == last) {
            prerelease_number = 0;
            return {next, std::errc{}};
          } else if (detail::check_delimiter(next, last, '.')) {
            if (next = detail::from_chars(++next, last, prerelease_number);
                next == last) {
              return {next, std::errc{}};
            }
          }
        }
      }
    }

    return {first, std::errc::invalid_argument};
  }

  [[nodiscard]] constexpr to_chars_result to_chars(char *first,
                                                   char *last) const noexcept {
    const auto length = string_length();
    if (first == nullptr || last == nullptr || (last - first) < length) {
      return {last, std::errc::value_too_large};
    }

    auto next = first + length;
    if (prerelease_type != prerelease::none) {
      if (prerelease_number != 0) {
        next = detail::to_chars(next, prerelease_number);
      }
      next = detail::to_chars(next, prerelease_type);
    }
    next = detail::to_chars(next, patch);
    next = detail::to_chars(next, minor);
    next = detail::to_chars(next, major, false);

    return {first + length, std::errc{}};
  }

  [[nodiscard]] constexpr bool from_string_noexcept(std::string_view str) noexcept {
    return from_chars(str.data(), str.data() + str.length());
  }

  constexpr version &from_string(std::string_view str) {
    if (!from_string_noexcept(str)) {
      NEARGYE_THROW(
          std::invalid_argument{"semver::version::from_string invalid version."});
    }

    return *this;
  }

  [[nodiscard]] std::string to_string() const {
    auto str = std::string(string_length(), '\0');
    if (!to_chars(str.data(), str.data() + str.length())) {
      NEARGYE_THROW(
          std::invalid_argument{"semver::version::to_string invalid version."});
    }

    return str;
  }

  [[nodiscard]] constexpr std::uint8_t string_length() const noexcept {
    // (<major>) + 1(.) + (<minor>) + 1(.) + (<patch>)
    auto length =
        detail::length(major) + detail::length(minor) + detail::length(patch) + 2;
    if (prerelease_type != prerelease::none) {
      // + 1(-) + (<prerelease>)
      length += detail::length(prerelease_type) + 1;
      if (prerelease_number != 0) {
        // + 1(.) + (<prereleaseversion>)
        length += detail::length(prerelease_number) + 1;
      }
    }

    return static_cast<std::uint8_t>(length);
  }

  [[nodiscard]] constexpr int compare(const version &other) const noexcept {
    if (major != other.major) {
      return major - other.major;
    }

    if (minor != other.minor) {
      return minor - other.minor;
    }

    if (patch != other.patch) {
      return patch - other.patch;
    }

    if (prerelease_type != other.prerelease_type) {
      return static_cast<std::uint8_t>(prerelease_type) -
             static_cast<std::uint8_t>(other.prerelease_type);
    }

    if (prerelease_number != other.prerelease_number) {
      return prerelease_number - other.prerelease_number;
    }

    return 0;
  }
};

[[nodiscard]] constexpr bool operator==(const version &lhs,
                                        const version &rhs) noexcept {
  return lhs.compare(rhs) == 0;
}

[[nodiscard]] constexpr bool operator!=(const version &lhs,
                                        const version &rhs) noexcept {
  return lhs.compare(rhs) != 0;
}

[[nodiscard]] constexpr bool operator>(const version &lhs,
                                       const version &rhs) noexcept {
  return lhs.compare(rhs) > 0;
}

[[nodiscard]] constexpr bool operator>=(const version &lhs,
                                        const version &rhs) noexcept {
  return lhs.compare(rhs) >= 0;
}

[[nodiscard]] constexpr bool operator<(const version &lhs,
                                       const version &rhs) noexcept {
  return lhs.compare(rhs) < 0;
}

[[nodiscard]] constexpr bool operator<=(const version &lhs,
                                        const version &rhs) noexcept {
  return lhs.compare(rhs) <= 0;
}

[[nodiscard]] constexpr version operator""_version(const char *str,
                                                   std::size_t length) {
  return version{std::string_view{str, length}};
}

[[nodiscard]] constexpr bool valid(std::string_view str) noexcept {
  return version{}.from_string_noexcept(str);
}

[[nodiscard]] constexpr from_chars_result
from_chars(const char *first, const char *last, version &v) noexcept {
  return v.from_chars(first, last);
}

[[nodiscard]] constexpr to_chars_result to_chars(char *first, char *last,
                                                 const version &v) noexcept {
  return v.to_chars(first, last);
}

[[nodiscard]] constexpr std::optional<version>
from_string_noexcept(std::string_view str) noexcept {
  if (version v{}; v.from_string_noexcept(str)) {
    return v;
  }

  return std::nullopt;
}

[[nodiscard]] constexpr version from_string(std::string_view str) {
  return version{str};
}

[[nodiscard]] inline std::string to_string(const version &v) { return v.to_string(); }

template <typename Char, typename Traits>
inline std::basic_ostream<Char, Traits> &
operator<<(std::basic_ostream<Char, Traits> &os, const version &v) {
  for (const auto c : v.to_string()) {
    os.put(c);
  }

  return os;
}

inline namespace comparators {

enum struct comparators_option : std::uint8_t {
  exclude_prerelease,
  include_prerelease
};

[[nodiscard]] constexpr int
compare(const version &lhs, const version &rhs,
        comparators_option option = comparators_option::include_prerelease) noexcept {
  if (option == comparators_option::exclude_prerelease) {
    return version{lhs.major, lhs.minor, lhs.patch}.compare(
        version{rhs.major, rhs.minor, rhs.patch});
  }
  return lhs.compare(rhs);
}

[[nodiscard]] constexpr bool
equal_to(const version &lhs, const version &rhs,
         comparators_option option = comparators_option::include_prerelease) noexcept {
  return compare(lhs, rhs, option) == 0;
}

[[nodiscard]] constexpr bool not_equal_to(
    const version &lhs, const version &rhs,
    comparators_option option = comparators_option::include_prerelease) noexcept {
  return compare(lhs, rhs, option) != 0;
}

[[nodiscard]] constexpr bool
greater(const version &lhs, const version &rhs,
        comparators_option option = comparators_option::include_prerelease) noexcept {
  return compare(lhs, rhs, option) > 0;
}

[[nodiscard]] constexpr bool greater_equal(
    const version &lhs, const version &rhs,
    comparators_option option = comparators_option::include_prerelease) noexcept {
  return compare(lhs, rhs, option) >= 0;
}

[[nodiscard]] constexpr bool
less(const version &lhs, const version &rhs,
     comparators_option option = comparators_option::include_prerelease) noexcept {
  return compare(lhs, rhs, option) < 0;
}

[[nodiscard]] constexpr bool less_equal(
    const version &lhs, const version &rhs,
    comparators_option option = comparators_option::include_prerelease) noexcept {
  return compare(lhs, rhs, option) <= 0;
}

} // namespace comparators

namespace range {

namespace detail {

using namespace semver::detail;

class range {
public:
  constexpr explicit range(std::string_view str) noexcept : str_{str} {}

  constexpr bool satisfies(const version &ver, bool include_prerelease) const {
    range_parser parser{str_};

    auto is_logical_or = [&parser]() constexpr noexcept->bool {
      return parser.current_token.type == range_token_type::logical_or;
    };

    auto is_operator = [&parser]() constexpr noexcept->bool {
      return parser.current_token.type == range_token_type::range_operator;
    };

    auto is_number = [&parser]() constexpr noexcept->bool {
      return parser.current_token.type == range_token_type::number;
    };

    const bool has_prerelease = ver.prerelease_type != prerelease::none;

    do {
      if (is_logical_or()) {
        parser.advance_token(range_token_type::logical_or);
      }

      bool contains = true;
      bool allow_compare = include_prerelease;

      while (is_operator() || is_number()) {
        const auto range = parser.parse_range();
        const bool equal_without_tags =
            equal_to(range.ver, ver, comparators_option::exclude_prerelease);

        if (has_prerelease && equal_without_tags) {
          allow_compare = true;
        }

        if (!range.satisfies(ver)) {
          contains = false;
          break;
        }
      }

      if (has_prerelease) {
        if (allow_compare && contains) {
          return true;
        }
      } else if (contains) {
        return true;
      }

    } while (is_logical_or());

    return false;
  }

private:
  enum struct range_operator : std::uint8_t {
    less,
    less_or_equal,
    greater,
    greater_or_equal,
    equal
  };

  struct range_comparator {
    range_operator op;
    version ver;

    constexpr bool satisfies(const version &version) const {
      switch (op) {
      case range_operator::equal:
        return version == ver;
      case range_operator::greater:
        return version > ver;
      case range_operator::greater_or_equal:
        return version >= ver;
      case range_operator::less:
        return version < ver;
      case range_operator::less_or_equal:
        return version <= ver;
      default:
        NEARGYE_THROW(std::invalid_argument{"semver::range unexpected operator."});
      }
    }
  };

  enum struct range_token_type : std::uint8_t {
    none,
    number,
    range_operator,
    dot,
    logical_or,
    hyphen,
    prerelease,
    end_of_line
  };

  struct range_token {
    range_token_type type = range_token_type::none;
    std::uint8_t number = 0;
    range_operator op = range_operator::equal;
    prerelease prerelease_type = prerelease::none;
  };

  struct range_lexer {
    std::string_view text;
    std::size_t pos;

    constexpr explicit range_lexer(std::string_view text) noexcept
        : text{text}, pos{0} {}

    constexpr range_token get_next_token() noexcept {
      while (!end_of_line()) {

        if (is_space(text[pos])) {
          advance(1);
          continue;
        }

        if (is_logical_or(text[pos])) {
          advance(2);
          return {range_token_type::logical_or};
        }

        if (is_operator(text[pos])) {
          const auto op = get_operator();
          return {range_token_type::range_operator, 0, op};
        }

        if (is_digit(text[pos])) {
          const auto number = get_number();
          return {range_token_type::number, number};
        }

        if (is_dot(text[pos])) {
          advance(1);
          return {range_token_type::dot};
        }

        if (is_hyphen(text[pos])) {
          advance(1);
          return {range_token_type::hyphen};
        }

        if (is_letter(text[pos])) {
          const auto prerelease = get_prerelease();
          return {range_token_type::prerelease, 0, range_operator::equal, prerelease};
        }
      }

      return {range_token_type::end_of_line};
    }

    constexpr bool end_of_line() const noexcept { return pos >= text.length(); }

    constexpr void advance(std::size_t i) noexcept { pos += i; }

    constexpr range_operator get_operator() noexcept {
      if (text[pos] == '<') {
        advance(1);
        if (text[pos] == '=') {
          advance(1);
          return range_operator::less_or_equal;
        }
        return range_operator::less;
      } else if (text[pos] == '>') {
        advance(1);
        if (text[pos] == '=') {
          advance(1);
          return range_operator::greater_or_equal;
        }
        return range_operator::greater;
      } else if (text[pos] == '=') {
        advance(1);
        return range_operator::equal;
      }

      return range_operator::equal;
    }

    constexpr std::uint8_t get_number() noexcept {
      const auto first = text.data() + pos;
      const auto last = text.data() + text.length();
      if (std::uint8_t n{}; from_chars(first, last, n) != nullptr) {
        advance(length(n));
        return n;
      }

      return 0;
    }

    constexpr prerelease get_prerelease() noexcept {
      const auto first = text.data() + pos;
      const auto last = text.data() + text.length();
      if (first > last) {
        advance(1);
        return prerelease::none;
      }

      if (prerelease p{}; from_chars(first, last, p) != nullptr) {
        advance(length(p));
        return p;
      }

      advance(1);

      return prerelease::none;
    }
  };

  struct range_parser {
    range_lexer lexer;
    range_token current_token;

    constexpr explicit range_parser(std::string_view str)
        : lexer{str}, current_token{range_token_type::none} {
      advance_token(range_token_type::none);
    }

    constexpr void advance_token(range_token_type token_type) {
      if (current_token.type != token_type) {
        NEARGYE_THROW(std::invalid_argument{"semver::range unexpected token."});
      }
      current_token = lexer.get_next_token();
    }

    constexpr range_comparator parse_range() {
      if (current_token.type == range_token_type::number) {
        const auto version = parse_version();
        return {range_operator::equal, version};
      } else if (current_token.type == range_token_type::range_operator) {
        const auto range_operator = current_token.op;
        advance_token(range_token_type::range_operator);
        const auto version = parse_version();
        return {range_operator, version};
      }

      return {range_operator::equal, version{}};
    }

    constexpr version parse_version() {
      const auto major = parse_number();

      advance_token(range_token_type::dot);
      const auto minor = parse_number();

      advance_token(range_token_type::dot);
      const auto patch = parse_number();

      prerelease prerelease = prerelease::none;
      std::uint8_t prerelease_number = 0;

      if (current_token.type == range_token_type::hyphen) {
        advance_token(range_token_type::hyphen);
        prerelease = parse_prerelease();
        advance_token(range_token_type::dot);
        prerelease_number = parse_number();
      }

      return {major, minor, patch, prerelease, prerelease_number};
    }

    constexpr std::uint8_t parse_number() {
      const auto token = current_token;
      advance_token(range_token_type::number);

      return token.number;
    }

    constexpr prerelease parse_prerelease() {
      const auto token = current_token;
      advance_token(range_token_type::prerelease);

      return token.prerelease_type;
    }
  };

  std::string_view str_;
};

} // namespace detail

enum struct satisfies_option : std::uint8_t { exclude_prerelease, include_prerelease };

constexpr bool
satisfies(const version &ver, std::string_view str,
          satisfies_option option = satisfies_option::exclude_prerelease) {
  switch (option) {
  case satisfies_option::exclude_prerelease:
    return detail::range{str}.satisfies(ver, false);
  case satisfies_option::include_prerelease:
    return detail::range{str}.satisfies(ver, true);
  default:
    NEARGYE_THROW(std::invalid_argument{"semver::range unexpected satisfies_option."});
  }
}

} // namespace range

// Version lib semver.
inline constexpr auto semver_version =
    version{SEMVER_VERSION_MAJOR, SEMVER_VERSION_MINOR, SEMVER_VERSION_PATCH};

} // namespace semver

#undef NEARGYE_THROW

#if defined(__clang__)
#pragma clang diagnostic pop
#endif

#endif // NEARGYE_SEMANTIC_VERSIONING_HPP
