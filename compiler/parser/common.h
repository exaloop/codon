/*
 * common.h --- Common utilities.
 *
 * (c) Seq project. All rights reserved.
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 */

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

#include "util/common.h"

using std::make_shared;
using std::make_unique;
using std::map;
using std::pair;
using std::set;
using std::shared_ptr;
using std::string;
using std::unique_ptr;
using std::unordered_map;
using std::unordered_set;
using std::vector;

#include "compiler/util/fmt/format.h"
#include "compiler/util/fmt/ostream.h"

namespace seq {

namespace exc {

/**
 * Parser error exception.
 * Used for parsing, transformation and type-checking errors.
 */
class ParserException : public std::runtime_error {
public:
  /// These vectors (stacks) store an error stack-trace.
  vector<SrcInfo> locations;
  vector<string> messages;

public:
  ParserException(const string &msg, const SrcInfo &info) noexcept
      : std::runtime_error(msg) {
    messages.push_back(msg);
    locations.push_back(info);
  }
  ParserException() noexcept : ParserException("", {}) {}
  explicit ParserException(const string &msg) noexcept : ParserException(msg, {}) {}
  ParserException(const ParserException &e) noexcept
      : std::runtime_error(e), locations(e.locations), messages(e.messages) {}

  /// Add an error message to the current stack trace
  void trackRealize(const string &msg, const SrcInfo &info) {
    locations.push_back(info);
    messages.push_back(fmt::format("while realizing {}", msg));
  }

  /// Add an error message to the current stack trace
  void track(const string &msg, const SrcInfo &info) {
    locations.push_back(info);
    messages.push_back(msg);
  }
};
} // namespace exc

namespace ast {

/// String and collection utilities

/// Split a delimiter-separated string into a vector of strings.
vector<string> split(const string &str, char delim);
/// Escape a C string (replace \n with \\n etc.).
string escape(const string &str);
/// Unescape a C string (replace \\n with \n etc.).
string unescape(const string &str);
/// Escape an F-string braces (replace { and } with {{ and }}).
string escapeFStringBraces(const string &str, int start, int len);
/// True if a string str starts with a prefix.
bool startswith(const string &str, const string &prefix);
/// True if a string str ends with a suffix.
bool endswith(const string &str, const string &suffix);
/// Trims whitespace at the beginning of the string.
void ltrim(string &str);
/// Trims whitespace at the end of the string.
void rtrim(string &str);
/// Removes leading stars in front of the string and returns the number of such stars.
int trimStars(string &str);
/// True if a string only contains digits.
bool isdigit(const string &str);
/// Combine items separated by a delimiter into a string.
template <typename T>
string join(const T &items, const string &delim = " ", int start = 0, int end = -1) {
  string s;
  if (end == -1)
    end = items.size();
  for (int i = start; i < end; i++)
    s += (i > start ? delim : "") + items[i];
  return s;
}
/// Combine items separated by a delimiter into a string.
template <typename T>
string combine(const vector<T> &items, const string &delim = " ") {
  string s;
  for (int i = 0; i < items.size(); i++)
    if (items[i])
      s += (i ? delim : "") + items[i]->toString();
  return s;
}
/// @return True if an item is found in a vector vec.
template <typename T, typename U> bool in(const vector<T> &vec, const U &item) {
  auto f = std::find(vec.begin(), vec.end(), item);
  return f != vec.end();
}
/// @return True if an item is found in a set s.
template <typename T, typename U> bool in(const set<T> &s, const U &item) {
  auto f = s.find(item);
  return f != s.end();
}
/// @return True if an item is found in an unordered_set s.
template <typename T, typename U> bool in(const unordered_set<T> &s, const U &item) {
  auto f = s.find(item);
  return f != s.end();
}
/// @return True if an item is found in a map m.
template <typename K, typename V, typename U>
bool in(const map<K, V> &m, const U &item) {
  auto f = m.find(item);
  return f != m.end();
}
/// @return True if an item is found in an unordered_map m.
template <typename K, typename V, typename U>
bool in(const unordered_map<K, V> &m, const U &item) {
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
template <typename T> auto clone(const shared_ptr<T> &t) {
  return t ? t->clone() : nullptr;
}

/// Clones a vector of cloneable pointer objects.
template <typename T> vector<T> clone(const vector<T> &t) {
  vector<T> v;
  for (auto &i : t)
    v.push_back(clone(i));
  return v;
}

/// Clones a vector of cloneable objects.
template <typename T> vector<T> clone_nop(const vector<T> &t) {
  vector<T> v;
  for (auto &i : t)
    v.push_back(i.clone());
  return v;
}

/// Path utilities

/// Detect a absolute path of the current executable (whose argv0 is known).
/// @return Absolute executable path or argv0 if one cannot be found.
string executable_path(const char *argv0);

struct ImportFile {
  enum Status { STDLIB, PACKAGE };
  Status status;
  /// Absolute path of an import.
  string path;
  /// Module name (e.g. foo.bar.baz).
  string module;
};
/// Find an import file what given an executable path (argv0) either in the standard
/// library or relative to a file relativeTo. Set forceStdlib for searching only the
/// standard library.
shared_ptr<ImportFile> getImportFile(const string &argv0, const string &what,
                                     const string &relativeTo, bool forceStdlib = false,
                                     const string &module0 = "");

} // namespace ast
} // namespace seq
