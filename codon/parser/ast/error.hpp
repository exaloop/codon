// Copyright (C) 2022-2024 Exaloop Inc. <https://exaloop.io>

#pragma once

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

  SrcInfo(std::string file, int line, int col, int len)
      : file(std::move(file)), line(line), col(col), len(len), id(0) {
    static int nextId = 0;
    id = nextId++;
  }

  SrcInfo() : SrcInfo("", 0, 0, 0) {}

  bool operator==(const SrcInfo &src) const { return id == src.id; }
};

} // namespace codon

namespace codon::exc {

/**
 * Parser error exception.
 * Used for parsing, transformation and type-checking errors.
 */
class ParserException : public std::runtime_error {
public:
  /// These vectors (stacks) store an error stack-trace.
  std::vector<SrcInfo> locations;
  std::vector<std::string> messages;
  int errorCode = -1;

public:
  ParserException(int errorCode, const std::string &msg, const SrcInfo &info) noexcept
      : std::runtime_error(msg), errorCode(errorCode) {
    messages.push_back(msg);
    locations.push_back(info);
  }
  ParserException() noexcept : std::runtime_error("") {}
  ParserException(int errorCode, const std::string &msg) noexcept
      : ParserException(errorCode, msg, {}) {}
  explicit ParserException(const std::string &msg) noexcept
      : ParserException(-1, msg, {}) {}
  ParserException(const ParserException &e) noexcept
      : std::runtime_error(e), locations(e.locations), messages(e.messages),
        errorCode(e.errorCode) {}

  /// Add an error message to the current stack trace
  void trackRealize(const std::string &msg, const SrcInfo &info) {
    locations.push_back(info);
    messages.push_back("during the realization of " + msg);
  }

  /// Add an error message to the current stack trace
  void track(const std::string &msg, const SrcInfo &info) {
    locations.push_back(info);
    messages.push_back(msg);
  }
};

} // namespace codon::exc
