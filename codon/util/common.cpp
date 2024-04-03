// Copyright (C) 2022-2024 Exaloop Inc. <https://exaloop.io>

#include "common.h"

#include "llvm/Support/Path.h"
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace codon {
namespace {
void compilationMessage(const std::string &header, const std::string &msg,
                        const std::string &file, int line, int col, int len,
                        int errorCode, MessageGroupPos pos) {
  auto &out = getLogger().err;
  seqassertn(!(file.empty() && (line > 0 || col > 0)),
             "empty filename with non-zero line/col: file={}, line={}, col={}", file,
             line, col);
  seqassertn(!(col > 0 && line <= 0), "col but no line: file={}, line={}, col={}", file,
             line, col);

  switch (pos) {
  case MessageGroupPos::NONE:
    break;
  case MessageGroupPos::HEAD:
    break;
  case MessageGroupPos::MID:
    fmt::print(out, "├─ ");
    break;
  case MessageGroupPos::LAST:
    fmt::print(out, "╰─ ");
    break;
  }

  fmt::print(out, "\033[1m");
  if (!file.empty()) {
    auto f = file.substr(file.rfind('/') + 1);
    fmt::print(out, "{}", f == "-" ? "<stdin>" : f);
  }
  if (line > 0)
    fmt::print(out, ":{}", line);
  if (col > 0)
    fmt::print(out, ":{}", col);
  if (len > 0)
    fmt::print(out, "-{}", col + len);
  if (!file.empty())
    fmt::print(out, ": ");
  fmt::print(out, "{}\033[1m {}\033[0m{}\n", header, msg,
             errorCode != -1
                 ? fmt::format(" (see https://exaloop.io/error/{:04d})", errorCode)
                 : "");
}

std::vector<Logger> loggers;
} // namespace

std::ostream &operator<<(std::ostream &out, const codon::SrcInfo &src) {
  out << llvm::sys::path::filename(src.file).str() << ":" << src.line << ":" << src.col;
  return out;
}

void compilationError(const std::string &msg, const std::string &file, int line,
                      int col, int len, int errorCode, bool terminate,
                      MessageGroupPos pos) {
  compilationMessage("\033[1;31merror:\033[0m", msg, file, line, col, len, errorCode,
                     pos);
  if (terminate)
    exit(EXIT_FAILURE);
}

void compilationWarning(const std::string &msg, const std::string &file, int line,
                        int col, int len, int errorCode, bool terminate,
                        MessageGroupPos pos) {
  compilationMessage("\033[1;33mwarning:\033[0m", msg, file, line, col, len, errorCode,
                     pos);
  if (terminate)
    exit(EXIT_FAILURE);
}

void Logger::parse(const std::string &s) {
  flags |= s.find('t') != std::string::npos ? FLAG_TIME : 0;
  flags |= s.find('r') != std::string::npos ? FLAG_REALIZE : 0;
  flags |= s.find('T') != std::string::npos ? FLAG_TYPECHECK : 0;
  flags |= s.find('i') != std::string::npos ? FLAG_IR : 0;
  flags |= s.find('l') != std::string::npos ? FLAG_USER : 0;
}
} // namespace codon

codon::Logger &codon::getLogger() {
  if (loggers.empty())
    loggers.emplace_back();
  return loggers.back();
}

void codon::pushLogger() { loggers.emplace_back(); }

bool codon::popLogger() {
  if (loggers.empty())
    return false;
  loggers.pop_back();
  return true;
}

void codon::assertionFailure(const char *expr_str, const char *file, int line,
                             const std::string &msg) {
  auto &out = getLogger().err;
  out << "Assert failed:\t" << msg << "\n"
      << "Expression:\t" << expr_str << "\n"
      << "Source:\t\t" << file << ":" << line << "\n";
  abort();
}
