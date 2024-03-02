// Copyright (C) 2022-2024 Exaloop Inc. <https://exaloop.io>

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

#define CAST(s, T) dynamic_cast<T *>(s.get())

namespace codon {

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
int findStar(const std::string &s);
/// True if a string str starts with a prefix.
size_t startswith(const std::string &str, const std::string &prefix);
/// True if a string str ends with a suffix.
size_t endswith(const std::string &str, const std::string &suffix);
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
std::string join(const T &items, const std::string &delim = " ", size_t start = 0,
                 size_t end = (1ull << 31)) {
  std::string s;
  if (end > items.size())
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
template <typename T>
std::string combine2(const std::vector<T> &items, const std::string &delim = ",",
                     int start = 0, int end = -1) {
  std::string s;
  if (end == -1)
    end = items.size();
  for (int i = start; i < end; i++)
    s += (i ? delim : "") + fmt::format("{}", items[i]);
  return s;
}
/// @return True if an item is found in a vector vec.
template <typename T, typename U>
const T *in(const std::vector<T> &vec, const U &item, size_t start = 0) {
  auto f = std::find(vec.begin() + start, vec.end(), item);
  return f != vec.end() ? &(*f) : nullptr;
}
/// @return True if an item is found in a set s.
template <typename T, typename U> const T *in(const std::set<T> &s, const U &item) {
  auto f = s.find(item);
  return f != s.end() ? &(*f) : nullptr;
}
/// @return True if an item is found in an unordered_set s.
template <typename T, typename U>
const T *in(const std::unordered_set<T> &s, const U &item) {
  auto f = s.find(item);
  return f != s.end() ? &(*f) : nullptr;
}
/// @return True if an item is found in a map m.
template <typename K, typename V, typename U>
const V *in(const std::map<K, V> &m, const U &item) {
  auto f = m.find(item);
  return f != m.end() ? &(f->second) : nullptr;
}
/// @return True if an item is found in an unordered_map m.
template <typename K, typename V, typename U>
const V *in(const std::unordered_map<K, V> &m, const U &item) {
  auto f = m.find(item);
  return f != m.end() ? &(f->second) : nullptr;
}
/// @return vector c transformed by the function f.
template <typename T, typename F> auto vmap(const std::vector<T> &c, F &&f) {
  std::vector<typename std::result_of<F(const T &)>::type> ret;
  std::transform(std::begin(c), std::end(c), std::inserter(ret, std::end(ret)), f);
  return ret;
}

/// AST utilities

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

/// @return The absolute canonical path of a given path.
std::string getAbsolutePath(const std::string &path);

/// Detect an absolute path of the current executable (whose argv0 is known).
/// @return Absolute executable path or argv0 if one cannot be found.
std::string executable_path(const char *argv0);
/// Detect an absolute path of the current libcodonc.
/// @return Absolute executable path or argv0 if one cannot be found.
std::string library_path();

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
