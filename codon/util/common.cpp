#include "common.h"

#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace codon {
namespace {
void compilationMessage(const std::string &header, const std::string &msg,
                        const std::string &file, int line, int col) {
  auto &out = getLogger().err;
  seqassertn(!(file.empty() && (line > 0 || col > 0)),
            "empty filename with non-zero line/col: file={}, line={}, col={}", file,
            line, col);
  seqassertn(!(col > 0 && line <= 0), "col but no line: file={}, line={}, col={}", file,
            line, col);
  out << "\033[1m";
  if (!file.empty())
    out << file.substr(file.rfind('/') + 1);
  if (line > 0)
    out << ":" << line;
  if (col > 0)
    out << ":" << col;
  if (!file.empty())
    out << ": ";
  out << header << "\033[1m " << msg << "\033[0m" << std::endl;
}

std::vector<Logger> loggers;
} // namespace

void compilationError(const std::string &msg, const std::string &file, int line,
                      int col, bool terminate) {
  compilationMessage("\033[1;31merror:\033[0m", msg, file, line, col);
  if (terminate)
    exit(EXIT_FAILURE);
}

void compilationWarning(const std::string &msg, const std::string &file, int line,
                        int col, bool terminate) {
  compilationMessage("\033[1;33mwarning:\033[0m", msg, file, line, col);
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
