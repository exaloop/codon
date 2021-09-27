#pragma once

#define SEQ_VERSION_MAJOR 0
#define SEQ_VERSION_MINOR 11
#define SEQ_VERSION_PATCH 0

#include <cassert>
#include <climits>
#include <cstdint>
#include <libgen.h>
#include <memory>
#include <ostream>
#include <stdexcept>
#include <sys/stat.h>

#include "util/fmt/format.h"
#include "util/fmt/ostream.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wgnu-zero-variadic-macro-arguments"

extern int _dbg_level;
extern int _level;
#define DBG(c, ...) fmt::print("{}" c "\n", std::string(2 * _level, ' '), ##__VA_ARGS__)
#define LOG(c, ...) DBG(c, ##__VA_ARGS__)
#define LOG_TIME(c, ...)                                                               \
  {                                                                                    \
    if (_dbg_level & (1 << 0))                                                         \
      DBG(c, ##__VA_ARGS__);                                                           \
  }
#define LOG_REALIZE(c, ...)                                                            \
  {                                                                                    \
    if (_dbg_level & (1 << 2))                                                         \
      DBG(c, ##__VA_ARGS__);                                                           \
  }
#define LOG_TYPECHECK(c, ...)                                                          \
  {                                                                                    \
    if (_dbg_level & (1 << 4))                                                         \
      DBG(c, ##__VA_ARGS__);                                                           \
  }
#define LOG_IR(c, ...)                                                                 \
  {                                                                                    \
    if (_dbg_level & (1 << 6))                                                         \
      DBG(c, ##__VA_ARGS__);                                                           \
  }
#define LOG_USER(c, ...)                                                               \
  {                                                                                    \
    if (_dbg_level & (1 << 7))                                                         \
      DBG(c, ##__VA_ARGS__);                                                           \
  }
#define CAST(s, T) dynamic_cast<T *>(s.get())

#ifndef NDEBUG
#define seqassert(expr, msg, ...)                                                      \
  ((expr) ? (void)(0)                                                                  \
          : _seqassert(#expr, __FILE__, __LINE__, fmt::format(msg, ##__VA_ARGS__)))
#else
#define seqassert(expr, msg, ...) ;
#endif
#pragma clang diagnostic pop
void _seqassert(const char *expr_str, const char *file, int line,
                const std::string &msg);

namespace seq {
struct SrcInfo {
  std::string file;
  int line;
  int col;
  int len;
  int id; /// used to differentiate different
  SrcInfo(std::string file, int line, int col, int len)
      : file(std::move(file)), line(line), col(col), len(len) {
    static int _id(0);
    id = _id++;
  };
  SrcInfo() : SrcInfo("", 0, 0, 0){};
  friend std::ostream &operator<<(std::ostream &out, const seq::SrcInfo &c) {
    char buf[PATH_MAX + 1];
    strncpy(buf, c.file.c_str(), PATH_MAX);
    auto f = basename(buf);
    out << f << ":" << c.line << ":" << c.col;
    return out;
  }
  bool operator==(const SrcInfo &src) const { return id == src.id; }
};

struct SrcObject {
private:
  SrcInfo info;

public:
  SrcObject() : info() {}
  SrcObject(const SrcObject &s) { setSrcInfo(s.getSrcInfo()); }

  virtual ~SrcObject() = default;

  SrcInfo getSrcInfo() const { return info; }

  void setSrcInfo(SrcInfo info) { this->info = std::move(info); }
};

void compilationError(const std::string &msg, const std::string &file = "",
                      int line = 0, int col = 0, bool terminate = true);

void compilationWarning(const std::string &msg, const std::string &file = "",
                        int line = 0, int col = 0, bool terminate = false);
} // namespace seq
