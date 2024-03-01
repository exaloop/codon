// Copyright (C) 2022-2024 Exaloop Inc. <https://exaloop.io>

#include "common.h"

#include <cinttypes>
#include <climits>
#include <string>
#include <vector>

#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Path.h"
#include <fmt/format.h>

namespace codon::ast {

/// String and collection utilities

std::vector<std::string> split(const std::string &s, char delim) {
  std::vector<std::string> items;
  std::string item;
  std::istringstream iss(s);
  while (std::getline(iss, item, delim))
    items.push_back(item);
  return items;
}
// clang-format off
std::string escape(const std::string &str) {
  std::string r;
  r.reserve(str.size());
  for (unsigned char c : str) {
    switch (c) {
    case '\a': r += "\\a"; break;
    case '\b': r += "\\b"; break;
    case '\f': r += "\\f"; break;
    case '\n': r += "\\n"; break;
    case '\r': r += "\\r"; break;
    case '\t': r += "\\t"; break;
    case '\v': r += "\\v"; break;
    case '\'': r += "\\'"; break;
    case '\\': r += "\\\\"; break;
    default:
      if (c < 32 || c >= 127)
        r += fmt::format("\\x{:x}", c);
      else
        r += c;
    }
  }
  return r;
}
std::string unescape(const std::string &str) {
  std::string r;
  r.reserve(str.size());
  for (int i = 0; i < str.size(); i++) {
    if (str[i] == '\\' && i + 1 < str.size())
      switch(str[i + 1]) {
      case 'a': r += '\a'; i++; break;
      case 'b': r += '\b'; i++; break;
      case 'f': r += '\f'; i++; break;
      case 'n': r += '\n'; i++; break;
      case 'r': r += '\r'; i++; break;
      case 't': r += '\t'; i++; break;
      case 'v': r += '\v'; i++; break;
      case '"': r += '\"'; i++; break;
      case '\'': r += '\''; i++; break;
      case '\\': r += '\\'; i++; break;
      case 'x': {
        if (i + 3 > str.size())
          throw std::invalid_argument("invalid \\x code");
        size_t pos = 0;
        auto code = std::stoi(str.substr(i + 2, 2), &pos, 16);
        r += char(code);
        i += pos + 1;
        break;
      }
      default:
        if (str[i + 1] >= '0' && str[i + 1] <= '7') {
          size_t pos = 0;
          auto code = std::stoi(str.substr(i + 1, 3), &pos, 8);
          r += char(code);
          i += pos;
        } else {
          r += str[i];
        }
      }
    else
      r += str[i];
  }
  return r;
}
// clang-format on
std::string escapeFStringBraces(const std::string &str, int start, int len) {
  std::string t;
  t.reserve(len);
  for (int i = start; i < start + len; i++)
    if (str[i] == '{')
      t += "{{";
    else if (str[i] == '}')
      t += "}}";
    else
      t += str[i];
  return t;
}
int findStar(const std::string &s) {
  int i = 0;
  for (; i < s.size(); i++)
    if (s[i] == ' ' || s[i] == ')')
      break;
  return i;
}
size_t startswith(const std::string &str, const std::string &prefix) {
  return (str.size() >= prefix.size() && str.substr(0, prefix.size()) == prefix)
             ? prefix.size()
             : 0;
}
size_t endswith(const std::string &str, const std::string &suffix) {
  return (str.size() >= suffix.size() &&
          str.substr(str.size() - suffix.size()) == suffix)
             ? suffix.size()
             : 0;
}
void ltrim(std::string &str) {
  str.erase(str.begin(), std::find_if(str.begin(), str.end(), [](unsigned char ch) {
              return !std::isspace(ch);
            }));
}
void rtrim(std::string &str) {
  /// https://stackoverflow.com/questions/216823/whats-the-best-way-to-trim-stdstring
  str.erase(std::find_if(str.rbegin(), str.rend(),
                         [](unsigned char ch) { return !std::isspace(ch); })
                .base(),
            str.end());
}
int trimStars(std::string &str) {
  int stars = 0;
  for (; stars < str.size() && str[stars] == '*'; stars++)
    ;
  str = str.substr(stars);
  return stars;
}
bool isdigit(const std::string &str) {
  return std::all_of(str.begin(), str.end(), ::isdigit);
}

/// Path utilities

std::string executable_path(const char *argv0) {
  void *p = (void *)(intptr_t)executable_path;
  return llvm::sys::fs::getMainExecutable(argv0, p);
}

// Adapted from https://github.com/gpakosz/whereami/blob/master/src/whereami.c (MIT)
#ifdef __APPLE__
#include <dlfcn.h>
#include <mach-o/dyld.h>
#endif
std::string library_path() {
  std::string result;
#ifdef __APPLE__
  char buffer[PATH_MAX];
  for (;;) {
    Dl_info info;
    if (dladdr(__builtin_extract_return_addr(__builtin_return_address(0)), &info)) {
      char *resolved = realpath(info.dli_fname, buffer);
      if (!resolved)
        break;
      result = std::string(resolved);
    }
    break;
  }
#else
  for (int r = 0; r < 5; r++) {
    FILE *maps = fopen("/proc/self/maps", "r");
    if (!maps)
      break;

    for (;;) {
      char buffer[PATH_MAX < 1024 ? 1024 : PATH_MAX];
      uint64_t low, high;
      char perms[5];
      uint64_t offset;
      uint32_t major, minor;
      char path[PATH_MAX];
      uint32_t inode;

      if (!fgets(buffer, sizeof(buffer), maps))
        break;

      if (sscanf(buffer, "%" PRIx64 "-%" PRIx64 " %s %" PRIx64 " %x:%x %u %s\n", &low,
                 &high, perms, &offset, &major, &minor, &inode, path) == 8) {
        uint64_t addr =
            (uintptr_t)(__builtin_extract_return_addr(__builtin_return_address(0)));
        if (low <= addr && addr <= high) {
          char *resolved = realpath(path, buffer);
          if (resolved)
            result = std::string(resolved);
          break;
        }
      }
    }
    fclose(maps);
    if (!result.empty())
      break;
  }
#endif

  return result;
}

namespace {

bool addPath(std::vector<std::string> &paths, const std::string &path) {
  if (llvm::sys::fs::exists(path)) {
    paths.push_back(getAbsolutePath(path));
    return true;
  }
  return false;
}

std::vector<std::string> getStdLibPaths(const std::string &argv0,
                                        const std::vector<std::string> &plugins) {
  std::vector<std::string> paths;
  if (auto c = getenv("CODON_PATH")) {
    addPath(paths, c);
  }
  if (!argv0.empty()) {
    auto base = executable_path(argv0.c_str());
    for (auto loci : {"../lib/codon/stdlib", "../stdlib", "stdlib"}) {
      auto path = llvm::SmallString<128>(llvm::sys::path::parent_path(base));
      llvm::sys::path::append(path, loci);
      addPath(paths, std::string(path));
    }
  }
  for (auto &path : plugins) {
    addPath(paths, path);
  }
  return paths;
}

ImportFile getRoot(const std::string argv0, const std::vector<std::string> &plugins,
                   const std::string &module0Root, const std::string &s) {
  bool isStdLib = false;
  std::string root;
  for (auto &p : getStdLibPaths(argv0, plugins))
    if (startswith(s, p)) {
      root = p;
      isStdLib = true;
      break;
    }
  if (!isStdLib && startswith(s, module0Root))
    root = module0Root;
  std::string ext = ".codon";
  if (!((root.empty() || startswith(s, root)) && endswith(s, ext)))
    ext = ".py";
  seqassertn((root.empty() || startswith(s, root)) && endswith(s, ext),
             "bad path substitution: {}, {}", s, root);
  auto module = s.substr(root.size() + 1, s.size() - root.size() - ext.size() - 1);
  std::replace(module.begin(), module.end(), '/', '.');
  return ImportFile{(!isStdLib && root == module0Root) ? ImportFile::PACKAGE
                                                       : ImportFile::STDLIB,
                    s, module};
}
} // namespace

std::string getAbsolutePath(const std::string &path) {
  char *c = realpath(path.c_str(), nullptr);
  if (!c)
    return path;
  std::string result(c);
  free(c);
  return result;
}

std::shared_ptr<ImportFile> getImportFile(const std::string &argv0,
                                          const std::string &what,
                                          const std::string &relativeTo,
                                          bool forceStdlib, const std::string &module0,
                                          const std::vector<std::string> &plugins) {
  std::vector<std::string> paths;
  if (what != "<jit>") {
    auto parentRelativeTo = llvm::sys::path::parent_path(relativeTo);
    if (!forceStdlib) {
      auto path = llvm::SmallString<128>(parentRelativeTo);
      llvm::sys::path::append(path, what);
      llvm::sys::path::replace_extension(path, "codon");
      addPath(paths, std::string(path));
      path = llvm::SmallString<128>(parentRelativeTo);
      llvm::sys::path::append(path, what, "__init__.codon");
      addPath(paths, std::string(path));

      path = llvm::SmallString<128>(parentRelativeTo);
      llvm::sys::path::append(path, what);
      llvm::sys::path::replace_extension(path, "py");
      addPath(paths, std::string(path));
      path = llvm::SmallString<128>(parentRelativeTo);
      llvm::sys::path::append(path, what, "__init__.py");
      addPath(paths, std::string(path));
    }
  }
  for (auto &p : getStdLibPaths(argv0, plugins)) {
    auto path = llvm::SmallString<128>(p);
    llvm::sys::path::append(path, what);
    llvm::sys::path::replace_extension(path, "codon");
    addPath(paths, std::string(path));
    path = llvm::SmallString<128>(p);
    llvm::sys::path::append(path, what, "__init__.codon");
    addPath(paths, std::string(path));
  }

  auto module0Root = llvm::sys::path::parent_path(getAbsolutePath(module0)).str();
  return paths.empty() ? nullptr
                       : std::make_shared<ImportFile>(
                             getRoot(argv0, plugins, module0Root, paths[0]));
}

} // namespace codon::ast
