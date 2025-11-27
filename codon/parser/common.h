// Copyright (C) 2022-2025 Exaloop Inc. <https://exaloop.io>

#pragma once

#include <chrono>
#include <filesystem>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "codon/util/common.h"

namespace codon {
namespace ir {
struct Attribute;
}

namespace ast {

struct Cache;

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
/// True if a string only contains digits.
bool isdigit(const std::string &str);
/// Combine items separated by a delimiter into a string.
/// Combine items separated by a delimiter into a string.
template <typename T> std::string join(const T &items, const std::string &delim = " ") {
  std::string s;
  bool first = true;
  for (const auto &i : items) {
    if (!first)
      s += delim;
    s += i;
    first = false;
  }
  return s;
}
template <typename T>
std::string join(const T &items, const std::string &delim, size_t start,
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
std::string combine(const std::vector<T> &items, const std::string &delim = " ",
                    const int indent = -1) {
  std::string s;
  for (int i = 0; i < items.size(); i++)
    if (items[i])
      s += (i ? delim : "") + items[i]->toString(indent);
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
/// @return True if an item is found in an unordered_map m.
template <typename K, typename V, typename U>
V *in(std::unordered_map<K, V> &m, const U &item) {
  auto f = m.find(item);
  return f != m.end() ? &(f->second) : nullptr;
}
/// @return True if an item is found in an string m.
bool in(const std::string &m, const std::string &item);
bool in(const std::string &m, char item);

/// AST utilities

template <typename T> T clone(const T &t, bool clean = false) { return t.clone(clean); }

template <typename T> std::remove_const_t<T> *clone(T *t, bool clean = false) {
  return t ? static_cast<std::remove_const_t<T> *>(t->clone(clean)) : nullptr;
}

template <typename T> std::remove_const_t<T> *clean_clone(T *t) {
  return clone(t, true);
}

/// Clones a vector of cloneable pointer objects.
template <typename T>
std::vector<std::remove_const_t<T>> clone(const std::vector<T> &t, bool clean = false) {
  std::vector<std::remove_const_t<T>> v;
  for (auto &i : t)
    v.push_back(clone(i, clean));
  return v;
}

/// Path utilities

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

class IFilesystem {
public:
  using path_t = std::filesystem::path;

protected:
  std::vector<path_t> search_paths;

public:
  virtual ~IFilesystem() {};

  virtual std::vector<std::string> read_lines(const path_t &path) const = 0;
  virtual bool exists(const path_t &path) const = 0;
  virtual std::vector<path_t> get_stdlib_paths() const;
  virtual path_t canonical(const path_t &path) const;
  ImportFile get_root(const path_t &s) const;

  virtual path_t get_module0() const { return ""; }
  virtual void set_module0(const std::string &) {}
  virtual void add_search_path(const std::string &p);
};

class Filesystem : public IFilesystem {
public:
  using IFilesystem::path_t;

private:
  path_t argv0, module0;
  std::vector<path_t> extraPaths;

public:
  Filesystem(const std::string &argv0, const std::string &module0 = "");
  std::vector<std::string> read_lines(const path_t &path) const override;
  bool exists(const path_t &path) const override;

  path_t get_module0() const override;
  void set_module0(const std::string &s) override;

public:
  /// Detect an absolute path of the current executable (whose argv0 is known).
  /// @return Absolute executable path or argv0 if one cannot be found.
  static path_t executable_path(const char *argv0);

  /// @return The absolute canonical path of a given path.
  static std::string get_absolute_path(const std::string &path);
};

class ResourceFilesystem : public Filesystem {
  bool allowExternal;

public:
  ResourceFilesystem(const std::string &argv0, const std::string &module0 = "",
                     bool allowExternal = true);
  std::vector<std::string> read_lines(const path_t &path) const override;
  bool exists(const path_t &path) const override;
};

/// Find an import file what given an executable path (argv0) either in the standard
/// library or relative to a file relativeTo. Set forceStdlib for searching only the
/// standard library.
std::shared_ptr<ImportFile> getImportFile(Cache *cache, const std::string &what,
                                          const std::string &relativeTo,
                                          bool forceStdlib = false);

template <typename T> class SetInScope {
  T *t;
  T origVal;

public:
  SetInScope(T *t, const T &val) : t(t), origVal(*t) { *t = val; }
  ~SetInScope() { *t = origVal; }
};

std::string getMangledClass(const std::string &module, const std::string &cls,
                            size_t id = 0);

std::string getMangledFunc(const std::string &module, const std::string &fn,
                           size_t overload = 0, size_t id = 0);

std::string getMangledMethod(const std::string &module, const std::string &cls,
                             const std::string &method, size_t overload = 0,
                             size_t id = 0);

std::string getMangledVar(const std::string &module, const std::string &var,
                          size_t id = 0);

} // namespace ast
} // namespace codon
