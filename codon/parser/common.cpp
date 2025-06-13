// Copyright (C) 2022-2025 Exaloop Inc. <https://exaloop.io>

#include "common.h"

#include <cinttypes>
#include <climits>
#include <filesystem>
#include <string>
#include <vector>

#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Path.h"
#include <fmt/format.h>

#include <cmrc/cmrc.hpp>
CMRC_DECLARE(codon);

namespace codon::ast {

IFilesystem::path_t IFilesystem::canonical(const path_t &path) const {
  return std::filesystem::weakly_canonical(path);
}

void IFilesystem::add_search_path(const std::string &p) {
  auto path = path_t(p);
  if (exists(path)) {
    search_paths.emplace_back(canonical(path));
  }
}

std::vector<IFilesystem::path_t> IFilesystem::get_stdlib_paths() const {
  return search_paths;
}

ImportFile IFilesystem::get_root(const path_t &sp) {
  bool isStdLib = false;
  std::string s = sp;
  std::string root;
  for (auto &p : get_stdlib_paths())
    if (startswith(s, p)) {
      root = p;
      isStdLib = true;
      break;
    }
  auto module0 = get_module0().parent_path();
  if (!isStdLib && !module0.empty() && startswith(s, module0))
    root = module0;
  std::string ext = ".codon";
  if (!((root.empty() || startswith(s, root)) && endswith(s, ext)))
    ext = ".py";
  seqassertn((root.empty() || startswith(s, root)) && endswith(s, ext),
             "bad path substitution: {}, {}", s, root);
  auto module = s.substr(root.size() + 1, s.size() - root.size() - ext.size() - 1);
  std::replace(module.begin(), module.end(), '/', '.');
  return ImportFile{(!isStdLib && root == module0) ? ImportFile::PACKAGE
                                                   : ImportFile::STDLIB,
                    s, module};
}

Filesystem::Filesystem(const std::string &argv0, const std::string &module0)
    : argv0(argv0), module0(module0) {
  if (auto p = getenv("CODON_PATH")) {
    add_search_path(p);
  }
  if (!argv0.empty()) {
    auto root = executable_path(argv0.c_str()).parent_path();
    for (auto loci : {"../lib/codon/stdlib", "../stdlib", "stdlib"}) {
      add_search_path(root / loci);
    }
  }
}

std::vector<std::string> Filesystem::read_lines(const path_t &path) const {
  std::vector<std::string> lines;
  if (path == "-") {
    for (std::string line; getline(std::cin, line);) {
      lines.push_back(line);
    }
  } else {
    std::ifstream fin(path);
    if (!fin)
      E(error::Error::COMPILER_NO_FILE, SrcInfo(), path);
    for (std::string line; getline(fin, line);) {
      lines.push_back(line);
    }
    fin.close();
  }
  return lines;
}

void Filesystem::set_module0(const std::string &s) { module0 = canonical(path_t(s)); }

IFilesystem::path_t Filesystem::get_module0() const {
  return module0.empty() ? IFilesystem::path_t() : canonical(module0);
}

IFilesystem::path_t Filesystem::executable_path(const char *argv0) {
  void *p = (void *)(intptr_t)executable_path;
  auto exc = llvm::sys::fs::getMainExecutable(argv0, p);
  return path_t(exc);
}

bool Filesystem::exists(const IFilesystem::path_t &path) const {
  return std::filesystem::exists(path);
}

ResourceFilesystem::ResourceFilesystem(const std::string &argv0,
                                       const std::string &module0, bool allowExternal)
    : Filesystem(argv0, module0), allowExternal(allowExternal) {
  search_paths = {"/stdlib"};
}

std::vector<std::string> ResourceFilesystem::read_lines(const path_t &path) const {
  auto fs = cmrc::codon::get_filesystem();

  if (!fs.exists(path) && allowExternal)
    return Filesystem::read_lines(path);

  std::vector<std::string> lines;
  if (path == "-") {
    E(error::Error::COMPILER_NO_FILE, SrcInfo(), "<stdin>");
  } else {
    try {
      auto fd = fs.open(path);
      auto contents = std::string(fd.begin(), fd.end());
      lines = split(contents, '\n');
    } catch (std::system_error &) {
      E(error::Error::COMPILER_NO_FILE, SrcInfo(), path);
    }
  }
  return lines;
}

bool ResourceFilesystem::exists(const path_t &path) const {
  auto fs = cmrc::codon::get_filesystem();
  if (fs.exists(path))
    return true;
  if (allowExternal)
    return Filesystem::exists(path);
  return false;
}

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
  bool start = false;
  int i = 0;
  for (; i < s.size(); i++) {
    if (s[i] == '(')
      return i + 1;
    if (!isspace(s[i]))
      return i;
  }
  return i;
}
bool in(const std::string &m, const std::string &item) {
  auto f = m.find(item);
  return f != std::string::npos;
}
size_t startswith(const std::string &str, const std::string &prefix) {
  if (prefix.empty())
    return true;
  return (str.size() >= prefix.size() && str.substr(0, prefix.size()) == prefix)
             ? prefix.size()
             : 0;
}
size_t endswith(const std::string &str, const std::string &suffix) {
  if (suffix.empty())
    return true;
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
bool isdigit(const std::string &str) {
  return std::all_of(str.begin(), str.end(), ::isdigit);
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

std::string Filesystem::get_absolute_path(const std::string &path) {
  char *c = realpath(path.c_str(), nullptr);
  if (!c)
    return path;
  std::string result(c);
  free(c);
  return result;
}

std::shared_ptr<ImportFile> getImportFile(IFilesystem *fs, const std::string &what,
                                          const std::string &relativeTo,
                                          bool forceStdlib) {
  std::vector<std::string> paths;
  if (what != "<jit>") {
    auto parentRelativeTo = IFilesystem::path_t(relativeTo).parent_path();
    if (!forceStdlib) {
      auto path = parentRelativeTo / what;
      path.replace_extension("codon");
      if (fs->exists(path))
        paths.emplace_back(fs->canonical(path));

      path = parentRelativeTo / what / "__init__.codon";
      if (fs->exists(path))
        paths.emplace_back(fs->canonical(path));

      path = parentRelativeTo / what;
      path.replace_extension("py");
      if (fs->exists(path))
        paths.emplace_back(fs->canonical(path));

      path = parentRelativeTo / what / "__init__.py";
      if (fs->exists(path))
        paths.emplace_back(fs->canonical(path));
    }
  }
  for (auto &p : fs->get_stdlib_paths()) {
    auto path = p / what;
    path.replace_extension("codon");
    if (fs->exists(path))
      paths.emplace_back(fs->canonical(path));

    path = p / what / "__init__.codon";
    if (fs->exists(path))
      paths.emplace_back(fs->canonical(path));
  }

  if (paths.empty())
    return nullptr;
  return std::make_shared<ImportFile>(fs->get_root(paths[0]));
}

} // namespace codon::ast
