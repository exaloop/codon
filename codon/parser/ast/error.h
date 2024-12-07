// Copyright (C) 2022-2024 Exaloop Inc. <https://exaloop.io>

#pragma once

#include "llvm/Support/Error.h"
#include <stdexcept>
#include <string>
#include <vector>

/**
 * WARNING: do not include anything else in this file, especially format.h
 * peglib.h uses this file. However, it is not compatible with format.h
 * (and possibly some other includes). Their inclusion will result in a succesful
 * compilation but extremely weird behaviour and hard-to-debug crashes (it seems that
 * some parts of peglib conflict with format.h in a weird way---further investigation
 * needed).
 */

namespace codon {
struct SrcInfo {
  std::string file;
  int line;
  int col;
  int len;
  int id; /// used to differentiate different instances

  SrcInfo();
  SrcInfo(std::string file, int line, int col, int len);
  bool operator==(const SrcInfo &src) const;
};

class ErrorMessage {
private:
  std::string msg;
  SrcInfo loc;
  int errorCode = -1;

public:
  explicit ErrorMessage(const std::string &msg, const SrcInfo &loc = SrcInfo(),
                        int errorCode = -1)
      : msg(msg), loc(loc), errorCode(-1) {}
  explicit ErrorMessage(const std::string &msg, const std::string &file = "",
                        int line = 0, int col = 0, int len = 0, int errorCode = -1)
      : msg(msg), loc(file, line, col, len), errorCode(-1) {}

  std::string getMessage() const { return msg; }
  std::string getFile() const { return loc.file; }
  int getLine() const { return loc.line; }
  int getColumn() const { return loc.col; }
  int getLength() const { return loc.len; }
  int getErrorCode() const { return errorCode; }
  SrcInfo getSrcInfo() const { return loc; }
  void setSrcInfo(const SrcInfo &s) { loc = s; }

  void log(llvm::raw_ostream &out) const {
    if (!getFile().empty()) {
      out << getFile();
      if (getLine() != 0) {
        out << ":" << getLine();
        if (getColumn() != 0) {
          out << ":" << getColumn();
        }
      }
      out << ": ";
    }
    out << getMessage();
  }
};

struct ParserErrors {
  struct Backtrace {
    std::vector<ErrorMessage> trace;
    const std::vector<ErrorMessage> &getMessages() const { return trace; }
    auto begin() const { return trace.begin(); }
    auto front() const { return trace.front(); }
    auto front() { return trace.front(); }
    auto end() const { return trace.end(); }
    auto back() { return trace.back(); }
    auto back() const { return trace.back(); }
    auto size() const { return trace.size(); }
    void addMessage(const std::string &msg, const SrcInfo &info = SrcInfo()) {
      trace.emplace_back(msg, info);
    }
  };
  std::vector<Backtrace> errors;

  ParserErrors() {}
  ParserErrors(const ErrorMessage &msg) : errors{Backtrace{{msg}}} {}
  ParserErrors(const std::string &msg, const SrcInfo &info)
      : ParserErrors({msg, info}) {}
  ParserErrors(const std::string &msg) : ParserErrors(msg, {}) {}
  ParserErrors(const ParserErrors &e) : errors(e.errors) {}
  ParserErrors(const std::vector<ErrorMessage> &m) : ParserErrors() {
    for (auto &msg : m)
      errors.push_back(Backtrace{{msg}});
  }

  auto begin() { return errors.begin(); }
  auto end() { return errors.end(); }
  auto begin() const { return errors.begin(); }
  auto end() const { return errors.end(); }
  auto empty() const { return errors.empty(); }
  auto size() const { return errors.size(); }
  void append(const ParserErrors &e) {
    errors.insert(errors.end(), e.errors.begin(), e.errors.end());
  }

  Backtrace &getLast() {
    assert(!empty() && "empty error trace");
    return errors.back();
  }

  /// Add an error message to the current backtrace
  void addError(const std::vector<ErrorMessage> &trace) { errors.push_back({trace}); }
  std::string getMessage() const {
    if (empty())
      return "";
    return errors.front().trace.front().getMessage();
  }
};
} // namespace codon

namespace codon::exc {

/**
 * Parser error exception.
 * Used for parsing, transformation and type-checking errors.
 */
class ParserException : public std::runtime_error {
  /// These vectors (stacks) store an error stack-trace.
  ParserErrors errors;

public:
  ParserException() noexcept : std::runtime_error("") {}
  ParserException(const ParserErrors &errors) noexcept
      : std::runtime_error(errors.getMessage()), errors(errors) {}
  ParserException(llvm::Error &&e) noexcept;

  ParserErrors getErrors() const { return errors; }
};

} // namespace codon::exc
