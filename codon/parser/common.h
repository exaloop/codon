#pragma once

#include <chrono>
#include <iostream>
#include <map>
#include <memory>
#include <set>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "codon/util/common.h"
#include "codon/util/fmt/format.h"
#include "codon/util/fmt/ostream.h"

#define CAST(s, T) dynamic_cast<T *>(s.get())

namespace codon {

namespace exc {

/**
 * Parser error exception.
 * Used for parsing, transformation and type-checking errors.
 */
class ParserException : public std::runtime_error {
public:
  /// These vectors (stacks) store an error stack-trace.
  std::vector<SrcInfo> locations;
  std::vector<std::string> messages;

public:
  ParserException(const std::string &msg, const SrcInfo &info) noexcept
      : std::runtime_error(msg) {
    messages.push_back(msg);
    locations.push_back(info);
  }
  ParserException() noexcept : ParserException("", {}) {}
  explicit ParserException(const std::string &msg) noexcept
      : ParserException(msg, {}) {}
  ParserException(const ParserException &e) noexcept
      : std::runtime_error(e), locations(e.locations), messages(e.messages) {}

  /// Add an error message to the current stack trace
  void trackRealize(const std::string &msg, const SrcInfo &info) {
    locations.push_back(info);
    messages.push_back(fmt::format("while realizing {}", msg));
  }

  /// Add an error message to the current stack trace
  void track(const std::string &msg, const SrcInfo &info) {
    locations.push_back(info);
    messages.push_back(msg);
  }
};
} // namespace exc

namespace ast {

/// String and collection utilities

/// Split a delimiter-separated string into a vector of strings.
std::vector<std::string> split(const std::string &str, char delim);
/// Escape a C string (replace \n with \\n etc.).
std::string escape(const std::string &str);
/// Unescape a C string (replace \\n with \n etc.).
std::string unescape(const std::string &str);
/// Escape an F-string braces (replace { and } with {{ and }}).
std::string escapeFStringBraces(const std::string &str, int start, int len);
/// True if a string str starts with a prefix.
bool startswith(const std::string &str, const std::string &prefix);
/// True if a string str ends with a suffix.
bool endswith(const std::string &str, const std::string &suffix);
/// Trims whitespace at the beginning of the string.
void ltrim(std::string &str);
/// Trims whitespace at the end of the string.
void rtrim(std::string &str);
/// Removes leading stars in front of the string and returns the number of such stars.
int trimStars(std::string &str);
/// True if a string only contains digits.
bool isdigit(const std::string &str);
/// Combine items separated by a delimiter into a string.
template <typename T>
std::string join(const T &items, const std::string &delim = " ", int start = 0,
                 int end = -1) {
  std::string s;
  if (end == -1)
    end = items.size();
  for (int i = start; i < end; i++)
    s += (i > start ? delim : "") + items[i];
  return s;
}
/// Combine items separated by a delimiter into a string.
template <typename T>
std::string combine(const std::vector<T> &items, const std::string &delim = " ") {
  std::string s;
  for (int i = 0; i < items.size(); i++)
    if (items[i])
      s += (i ? delim : "") + items[i]->toString();
  return s;
}
/// @return True if an item is found in a vector vec.
template <typename T, typename U> bool in(const std::vector<T> &vec, const U &item) {
  auto f = std::find(vec.begin(), vec.end(), item);
  return f != vec.end();
}
/// @return True if an item is found in a set s.
template <typename T, typename U> bool in(const std::set<T> &s, const U &item) {
  auto f = s.find(item);
  return f != s.end();
}
/// @return True if an item is found in an unordered_set s.
template <typename T, typename U>
bool in(const std::unordered_set<T> &s, const U &item) {
  auto f = s.find(item);
  return f != s.end();
}
/// @return True if an item is found in a map m.
template <typename K, typename V, typename U>
bool in(const std::map<K, V> &m, const U &item) {
  auto f = m.find(item);
  return f != m.end();
}
/// @return True if an item is found in an unordered_map m.
template <typename K, typename V, typename U>
bool in(const std::unordered_map<K, V> &m, const U &item) {
  auto f = m.find(item);
  return f != m.end();
}
/// @return vector c transformed by the function f.
template <typename T, typename F> auto vmap(const std::vector<T> &c, F &&f) {
  std::vector<typename std::result_of<F(const T &)>::type> ret;
  std::transform(std::begin(c), std::end(c), std::inserter(ret, std::end(ret)), f);
  return ret;
}

/// AST utilities

/// Raise a parsing error.
void error(const char *format);
/// Raise a parsing error at a source location p.
void error(const SrcInfo &info, const char *format);

/// Clones a pointer even if it is a nullptr.
template <typename T> auto clone(const std::shared_ptr<T> &t) {
  return t ? t->clone() : nullptr;
}

/// Clones a vector of cloneable pointer objects.
template <typename T> std::vector<T> clone(const std::vector<T> &t) {
  std::vector<T> v;
  for (auto &i : t)
    v.push_back(clone(i));
  return v;
}

/// Clones a vector of cloneable objects.
template <typename T> std::vector<T> clone_nop(const std::vector<T> &t) {
  std::vector<T> v;
  for (auto &i : t)
    v.push_back(i.clone());
  return v;
}

/// Path utilities

/// Detect a absolute path of the current executable (whose argv0 is known).
/// @return Absolute executable path or argv0 if one cannot be found.
std::string executable_path(const char *argv0);

struct ImportFile {
  enum Status { STDLIB, PACKAGE };
  Status status;
  /// Absolute path of an import.
  std::string path;
  /// Module name (e.g. foo.bar.baz).
  std::string module;
};
/// Find an import file what given an executable path (argv0) either in the standard
/// library or relative to a file relativeTo. Set forceStdlib for searching only the
/// standard library.
std::shared_ptr<ImportFile> getImportFile(const std::string &argv0,
                                          const std::string &what,
                                          const std::string &relativeTo,
                                          bool forceStdlib = false,
                                          const std::string &module0 = "",
                                          const std::vector<std::string> &plugins = {});

} // namespace ast
} // namespace codon
