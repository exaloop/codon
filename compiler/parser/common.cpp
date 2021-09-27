/*
 * common.cpp --- Common utilities.
 *
 * (c) Seq project. All rights reserved.
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 */

#include <libgen.h>
#include <string>
#include <sys/stat.h>

#include "parser/common.h"
#include "util/fmt/format.h"

namespace seq {
namespace ast {

/// String and collection utilities

vector<string> split(const string &s, char delim) {
  vector<string> items;
  string item;
  std::istringstream iss(s);
  while (std::getline(iss, item, delim))
    items.push_back(item);
  return items;
}
// clang-format off
string escape(const string &str) {
  string r;
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
string unescape(const string &str) {
  string r;
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
string escapeFStringBraces(const string &str, int start, int len) {
  string t;
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
bool startswith(const string &str, const string &prefix) {
  return str.size() >= prefix.size() && str.substr(0, prefix.size()) == prefix;
}
bool endswith(const string &str, const string &suffix) {
  return str.size() >= suffix.size() &&
         str.substr(str.size() - suffix.size()) == suffix;
}
void ltrim(string &str) {
  str.erase(str.begin(), std::find_if(str.begin(), str.end(), [](unsigned char ch) {
              return !std::isspace(ch);
            }));
}
void rtrim(string &str) {
  /// https://stackoverflow.com/questions/216823/whats-the-best-way-to-trim-stdstring
  str.erase(std::find_if(str.rbegin(), str.rend(),
                         [](unsigned char ch) { return !std::isspace(ch); })
                .base(),
            str.end());
}
int trimStars(string &str) {
  int stars = 0;
  for (; stars < str.size() && str[stars] == '*'; stars++)
    ;
  str = str.substr(stars);
  return stars;
}
bool isdigit(const string &str) {
  return std::all_of(str.begin(), str.end(), ::isdigit);
}

/// AST utilities

void error(const char *format) { throw exc::ParserException(format); }
void error(const ::seq::SrcInfo &info, const char *format) {
  throw exc::ParserException(format, info);
}

/// Path utilities

#ifdef __APPLE__
#include <mach-o/dyld.h>

string executable_path(const char *argv0) {
  typedef vector<char> char_vector;
  char_vector buf(1024, 0);
  auto size = static_cast<uint32_t>(buf.size());
  bool havePath = false;
  bool shouldContinue = true;
  do {
    int result = _NSGetExecutablePath(&buf[0], &size);
    if (result == -1) {
      buf.resize(size + 1);
      std::fill(std::begin(buf), std::end(buf), 0);
    } else {
      shouldContinue = false;
      if (buf.at(0) != 0) {
        havePath = true;
      }
    }
  } while (shouldContinue);
  if (!havePath) {
    return string(argv0);
  }
  return string(&buf[0], size);
}
#elif __linux__
#include <unistd.h>

string executable_path(const char *argv0) {
  typedef vector<char> char_vector;
  typedef vector<char>::size_type size_type;
  char_vector buf(1024, 0);
  size_type size = buf.size();
  bool havePath = false;
  bool shouldContinue = true;
  do {
    ssize_t result = readlink("/proc/self/exe", &buf[0], size);
    if (result < 0) {
      shouldContinue = false;
    } else if (static_cast<size_type>(result) < size) {
      havePath = true;
      shouldContinue = false;
      size = result;
    } else {
      size *= 2;
      buf.resize(size);
      std::fill(std::begin(buf), std::end(buf), 0);
    }
  } while (shouldContinue);
  if (!havePath) {
    return string(argv0);
  }
  return string(&buf[0], size);
}
#else
string executable_path(const char *argv0) { return string(argv0); }
#endif

shared_ptr<ImportFile> getImportFile(const string &argv0, const string &what,
                                     const string &relativeTo, bool forceStdlib,
                                     const string &module0) {
  using fmt::format;

  auto getStdLibPaths = [](const string &argv0) {
    vector<string> paths;
    char abs[PATH_MAX + 1];
    if (auto c = getenv("SEQ_PATH")) {
      if (realpath(c, abs))
        paths.push_back(abs);
    }
    if (!argv0.empty())
      for (auto loci : {"../lib/seq/stdlib", "../stdlib", "stdlib"}) {
        strncpy(abs, executable_path(argv0.c_str()).c_str(), PATH_MAX);
        if (realpath(format("{}/{}", dirname(abs), loci).c_str(), abs))
          paths.push_back(abs);
      }
    return paths;
  };

  char abs[PATH_MAX + 1];
  strncpy(abs, module0.c_str(), PATH_MAX);
  auto module0Root = string(dirname(abs));
  auto getRoot = [&](const string &s) {
    bool isStdLib = false;
    string root;
    for (auto &p : getStdLibPaths(argv0))
      if (startswith(s, p)) {
        root = p;
        isStdLib = true;
        break;
      }
    if (!isStdLib && startswith(s, module0Root))
      root = module0Root;
    seqassert(startswith(s, root) && endswith(s, ".seq"),
              "bad path substitution: {}, {}", s, root);
    auto module = s.substr(root.size() + 1, s.size() - root.size() - 5);
    std::replace(module.begin(), module.end(), '/', '.');
    return ImportFile{(!isStdLib && root == module0Root) ? ImportFile::PACKAGE
                                                         : ImportFile::STDLIB,
                      s, module};
  };

  vector<string> paths;
  if (!forceStdlib) {
    realpath(relativeTo.c_str(), abs);
    auto parent = dirname(abs);
    paths.push_back(format("{}/{}.seq", parent, what));
    paths.push_back(format("{}/{}/__init__.seq", parent, what));
  }
  for (auto &p : getStdLibPaths(argv0)) {
    paths.push_back(format("{}/{}.seq", p, what));
    paths.push_back(format("{}/{}/__init__.seq", p, what));
  }
  for (auto &p : paths) {
    if (!realpath(p.c_str(), abs))
      continue;
    auto path = string(abs);
    struct stat buffer;
    if (!stat(path.c_str(), &buffer))
      return std::make_shared<ImportFile>(getRoot(path));
  }
  return nullptr;
}

} // namespace ast
} // namespace seq
