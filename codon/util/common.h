#pragma once

#include "llvm/Support/Path.h"
#include <chrono>
#include <iostream>
#include <ostream>

#include "codon/config/config.h"
#include "codon/util/fmt/format.h"
#include "codon/util/fmt/ostream.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wgnu-zero-variadic-macro-arguments"

#define DBG(c, ...)                                                                    \
  fmt::print(codon::getLogger().log, "{}" c "\n",                                      \
             std::string(2 * codon::getLogger().level, ' '), ##__VA_ARGS__)
#define LOG(c, ...) DBG(c, ##__VA_ARGS__)
#define LOG_TIME(c, ...)                                                               \
  {                                                                                    \
    if (codon::getLogger().flags & codon::Logger::FLAG_TIME)                           \
      DBG(c, ##__VA_ARGS__);                                                           \
  }
#define LOG_REALIZE(c, ...)                                                            \
  {                                                                                    \
    if (codon::getLogger().flags & codon::Logger::FLAG_REALIZE)                        \
      DBG(c, ##__VA_ARGS__);                                                           \
  }
#define LOG_TYPECHECK(c, ...)                                                          \
  {                                                                                    \
    if (codon::getLogger().flags & codon::Logger::FLAG_TYPECHECK)                      \
      DBG(c, ##__VA_ARGS__);                                                           \
  }
#define LOG_IR(c, ...)                                                                 \
  {                                                                                    \
    if (codon::getLogger().flags & codon::Logger::FLAG_IR)                             \
      DBG(c, ##__VA_ARGS__);                                                           \
  }
#define LOG_USER(c, ...)                                                               \
  {                                                                                    \
    if (codon::getLogger().flags & codon::Logger::FLAG_USER)                           \
      DBG(c, ##__VA_ARGS__);                                                           \
  }

#define TIME(name) codon::Timer __timer(name)

#ifndef NDEBUG
#define seqassert(expr, msg, ...)                                                      \
  ((expr) ? (void)(0)                                                                  \
          : codon::assertionFailure(#expr, __FILE__, __LINE__,                         \
                                    fmt::format(msg, ##__VA_ARGS__)))
#else
#define seqassert(expr, msg, ...) ;
#endif
#pragma clang diagnostic pop

namespace codon {

void assertionFailure(const char *expr_str, const char *file, int line,
                      const std::string &msg);

struct Logger {
  static constexpr int FLAG_TIME = (1 << 0);
  static constexpr int FLAG_REALIZE = (1 << 1);
  static constexpr int FLAG_TYPECHECK = (1 << 2);
  static constexpr int FLAG_IR = (1 << 3);
  static constexpr int FLAG_USER = (1 << 4);

  int flags;
  int level;
  std::ostream &out;
  std::ostream &err;
  std::ostream &log;

  Logger() : flags(0), level(0), out(std::cout), err(std::cerr), log(std::clog) {}

  void parse(const std::string &logs);
};

Logger &getLogger();
void pushLogger();
bool popLogger();

class Timer {
private:
  using clock_type = std::chrono::high_resolution_clock;
  std::string name;
  std::chrono::time_point<clock_type> start, end;
  bool logged;

public:
  void log() {
    if (!logged) {
      end = clock_type::now();
      auto elapsed =
          std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() /
          1000.0;
      LOG_TIME("[T] {} = {:.3f}", name, elapsed);
      logged = true;
    }
  }

  Timer(std::string name) : name(std::move(name)), start(), end(), logged(false) {
    start = clock_type::now();
  }

  ~Timer() { log(); }
};

struct SrcInfo {
  std::string file;
  int line;
  int col;
  int len;
  int id; /// used to differentiate different instances

  SrcInfo(std::string file, int line, int col, int len)
      : file(std::move(file)), line(line), col(col), len(len), id(0) {
    static int nextId = 0;
    id = nextId++;
  };

  SrcInfo() : SrcInfo("", 0, 0, 0) {}

  friend std::ostream &operator<<(std::ostream &out, const codon::SrcInfo &src) {
    out << llvm::sys::path::filename(src.file).str() << ":" << src.line << ":"
        << src.col;
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

} // namespace codon
