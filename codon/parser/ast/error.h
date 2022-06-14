#pragma once

#include <string>
#include <vector>

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
  };

  SrcInfo() : SrcInfo("", 0, 0, 0) {}

  bool operator==(const SrcInfo &src) const { return id == src.id; }
};
}

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

public:
  ParserException(const std::string &msg, const SrcInfo &info) noexcept
      : std::runtime_error(msg) {
    messages.push_back(msg);
    locations.push_back(info);
  }
  ParserException() noexcept : std::runtime_error("") {}
  explicit ParserException(const std::string &msg) noexcept
      : ParserException(msg, {}) {}
  ParserException(const ParserException &e) noexcept
      : std::runtime_error(e), locations(e.locations), messages(e.messages) {}

  /// Add an error message to the current stack trace
  void trackRealize(const std::string &msg, const SrcInfo &info) {
    locations.push_back(info);
    messages.push_back("while realizing " + msg);
  }

  /// Add an error message to the current stack trace
  void track(const std::string &msg, const SrcInfo &info) {
    locations.push_back(info);
    messages.push_back(msg);
  }

  // const char *what() const noexcept override {
  //   return messages.empty() ? nullptr : messages[0].c_str();
  // }
};

} // namespace codon::exc