// Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>

#pragma once

#include "llvm/Support/Error.h"
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
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
/// Compiler filenames and type identifiers have process lifetime. Copies retain
/// a pointer instead of allocating repeated copies of the same text.
class InternedString {
  const std::string *value = nullptr;

public:
  InternedString() = default;
  InternedString(std::string_view value);
  const std::string &str() const {
    static const std::string empty;
    return value ? *value : empty;
  }
  operator const std::string &() const { return str(); }
  operator std::string_view() const { return str(); }
  bool empty() const { return !value || value->empty(); }
  const char *c_str() const { return str().c_str(); }
  InternedString &operator=(std::string_view text) {
    *this = InternedString(text);
    return *this;
  }
  bool operator==(const InternedString &other) const { return value == other.value; }
  bool operator==(std::string_view other) const { return str() == other; }
  bool operator<(const InternedString &other) const { return str() < other.str(); }
  bool operator<=(const InternedString &other) const { return str() <= other.str(); }
  size_t size() const { return str().size(); }
  size_t find(char c) const { return str().find(c); }
  size_t rfind(char c) const { return str().rfind(c); }
  std::string substr(size_t pos, size_t count = std::string::npos) const {
    return str().substr(pos, count);
  }
  friend std::string operator+(const InternedString &a, std::string_view b) {
    std::string result;
    result.reserve(a.size() + b.size());
    result.append(a.str());
    result.append(b);
    return result;
  }
  friend std::string operator+(std::string_view a, const InternedString &b) {
    std::string result;
    result.reserve(a.size() + b.size());
    result.append(a);
    result.append(b.str());
    return result;
  }
};
using SourceFile = InternedString;

struct SrcInfo {
  SourceFile file;
  int line;
  int col;
  int len;
  int id; /// used to differentiate different instances

  SrcInfo();
  SrcInfo(std::string_view file, int line, int col, int len);
  bool operator==(const SrcInfo &src) const;
  bool operator<(const SrcInfo &src) const;
  bool operator<=(const SrcInfo &src) const;
};

class ErrorMessage {
private:
  std::string msg;
  SrcInfo loc;
  int errorCode = -1;

public:
  explicit ErrorMessage(std::string msg, SrcInfo loc = SrcInfo(), int errorCode = -1)
      : msg(std::move(msg)), loc(std::move(loc)), errorCode(-1) {}
  explicit ErrorMessage(std::string msg, const std::string &file = "", int line = 0,
                        int col = 0, int len = 0, int errorCode = -1)
      : msg(std::move(msg)), loc(file, line, col, len), errorCode(-1) {}

  std::string getMessage() const { return msg; }
  std::string getFile() const { return loc.file; }
  int getLine() const { return loc.line; }
  int getColumn() const { return loc.col; }
  int getLength() const { return loc.len; }
  int getErrorCode() const { return errorCode; }
  SrcInfo getSrcInfo() const { return loc; }
  void setSrcInfo(const SrcInfo &s) { loc = s; }
  bool operator==(const ErrorMessage &t) const { return msg == t.msg && loc == t.loc; }

  std::string toString() const;

  void log(llvm::raw_ostream &out) const { out << toString(); }
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
    bool operator==(const Backtrace &t) const { return trace == t.trace; }
  };
  std::vector<Backtrace> errors;

  ParserErrors() = default;
  explicit ParserErrors(const ErrorMessage &msg) : errors{Backtrace{{msg}}} {}
  ParserErrors(const std::string &msg, const SrcInfo &info)
      : ParserErrors(ErrorMessage{msg, info}) {}
  explicit ParserErrors(const std::string &msg)
      : ParserErrors(ErrorMessage{msg, SrcInfo{}}) {}
  ParserErrors(const ParserErrors &e) = default;
  explicit ParserErrors(const std::vector<ErrorMessage> &m) : ParserErrors() {
    for (auto &msg : m)
      errors.push_back(Backtrace{{msg}});
  }

  auto begin() { return errors.begin(); }
  auto end() { return errors.end(); }
  auto begin() const { return errors.begin(); }
  auto end() const { return errors.end(); }
  auto empty() const { return errors.empty(); }
  auto size() const { return errors.size(); }
  auto &back() { return errors.back(); }
  const auto &back() const { return errors.back(); }
  void append(const ParserErrors &e) {
    for (auto &trace : e)
      addError(trace);
  }

  Backtrace getLast() {
    assert(!empty() && "empty error trace");
    return errors.back();
  }

  /// Add an error message to the current backtrace
  void addError(const Backtrace &trace) {
    if (!errors.empty() && errors.back() == trace)
      return;
    errors.push_back({trace});
  }
  void addError(const std::vector<ErrorMessage> &trace) { addError(Backtrace{trace}); }
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
  explicit ParserException(const ParserErrors &errors) noexcept
      : std::runtime_error(errors.getMessage()), errors(errors) {}
  explicit ParserException(llvm::Error &&e) noexcept;

  const ParserErrors &getErrors() const { return errors; }
  ParserErrors &getErrors() { return errors; }
};

} // namespace codon::exc
