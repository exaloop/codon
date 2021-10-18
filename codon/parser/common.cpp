#include "common.h"

#include <libgen.h>
#include <string>
#include <sys/stat.h>

#include "codon/parser/common.h"
#include "codon/util/fmt/format.h"

namespace codon {
namespace ast {

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
bool startswith(const std::string &str, const std::string &prefix) {
  return str.size() >= prefix.size() && str.substr(0, prefix.size()) == prefix;
}
bool endswith(const std::string &str, const std::string &suffix) {
  return str.size() >= suffix.size() &&
         str.substr(str.size() - suffix.size()) == suffix;
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

/// AST utilities

void error(const char *format) { throw exc::ParserException(format); }
void error(const ::codon::SrcInfo &info, const char *format) {
  throw exc::ParserException(format, info);
}

/// Path utilities

#ifdef __APPLE__
#include <mach-o/dyld.h>

std::string executable_path(const char *argv0) {
  typedef std::vector<char> char_vector;
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
    return std::string(argv0);
  }
  return std::string(&buf[0], size);
}
#elif __linux__
#include <unistd.h>

std::string executable_path(const char *argv0) {
  typedef std::vector<char> char_vector;
  typedef std::vector<char>::size_type size_type;
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
    return std::string(argv0);
  }
  return std::string(&buf[0], size);
}
#else
std::string executable_path(const char *argv0) { return std::string(argv0); }
#endif

std::shared_ptr<ImportFile> getImportFile(const std::string &argv0,
                                          const std::string &what,
                                          const std::string &relativeTo,
                                          bool forceStdlib, const std::string &module0,
                                          const std::vector<std::string> &plugins) {
  using fmt::format;

  auto getStdLibPaths = [](const std::string &argv0,
                           const std::vector<std::string> &plugins) {
    std::vector<std::string> paths;
    char abs[PATH_MAX + 1];
    if (auto c = getenv("CODON_PATH")) {
      if (realpath(c, abs))
        paths.push_back(abs);
    }
    if (!argv0.empty()) {
      for (auto loci : {"../lib/codon/stdlib", "../stdlib", "stdlib"}) {
        strncpy(abs, executable_path(argv0.c_str()).c_str(), PATH_MAX);
        if (realpath(format("{}/{}", dirname(abs), loci).c_str(), abs))
          paths.push_back(abs);
      }
    }
    for (auto &path : plugins) {
      if (realpath(path.c_str(), abs))
        paths.push_back(abs);
    }
    return paths;
  };

  char abs[PATH_MAX + 1];
  strncpy(abs, module0.c_str(), PATH_MAX);
  auto module0Root = std::string(dirname(abs));
  auto getRoot = [&](const std::string &s) {
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
    const std::string ext = ".codon";
    seqassert(startswith(s, root) && endswith(s, ext), "bad path substitution: {}, {}",
              s, root);
    auto module = s.substr(root.size() + 1, s.size() - root.size() - ext.size() - 1);
    std::replace(module.begin(), module.end(), '/', '.');
    return ImportFile{(!isStdLib && root == module0Root) ? ImportFile::PACKAGE
                                                         : ImportFile::STDLIB,
                      s, module};
  };

  std::vector<std::string> paths;
  if (!forceStdlib) {
    realpath(relativeTo.c_str(), abs);
    auto parent = dirname(abs);
    paths.push_back(format("{}/{}.codon", parent, what));
    paths.push_back(format("{}/{}/__init__.codon", parent, what));
  }
  for (auto &p : getStdLibPaths(argv0, plugins)) {
    paths.push_back(format("{}/{}.codon", p, what));
    paths.push_back(format("{}/{}/__init__.codon", p, what));
  }
  for (auto &p : paths) {
    if (!realpath(p.c_str(), abs))
      continue;
    auto path = std::string(abs);
    struct stat buffer;
    if (!stat(path.c_str(), &buffer))
      return std::make_shared<ImportFile>(getRoot(path));
  }
  return nullptr;
}

} // namespace ast
} // namespace codon
