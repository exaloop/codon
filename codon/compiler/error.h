#pragma once

#include <string>
#include <vector>

#include "codon/parser/common.h"

#include "llvm/Support/Error.h"

namespace codon {
namespace error {

class Message {
private:
  std::string msg;
  std::string file;
  int line = 0;
  int col = 0;

public:
  explicit Message(const std::string &msg, const std::string &file = "", int line = 0,
                   int col = 0)
      : msg(msg), file(file), line(line), col(col) {}

  std::string getMessage() const { return msg; }
  std::string getFile() const { return file; }
  int getLine() const { return line; }
  int getColumn() const { return col; }

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

class ParserErrorInfo : public llvm::ErrorInfo<ParserErrorInfo> {
private:
  std::vector<Message> messages;

public:
  explicit ParserErrorInfo(std::vector<Message> messages)
      : messages(std::move(messages)) {}
  explicit ParserErrorInfo(const exc::ParserException &e) : messages() {
    for (unsigned i = 0; i < e.messages.size(); i++) {
      if (!e.messages[i].empty())
        messages.emplace_back(e.messages[i], e.locations[i].file, e.locations[i].line,
                              e.locations[i].col);
    }
  }

  auto begin() { return messages.begin(); }
  auto end() { return messages.end(); }
  auto begin() const { return messages.begin(); }
  auto end() const { return messages.end(); }

  void log(llvm::raw_ostream &out) const override {
    for (auto &msg : *this) {
      msg.log(out);
      out << "\n";
    }
  }

  std::error_code convertToErrorCode() const override {
    return llvm::inconvertibleErrorCode();
  }

  static char ID;
};

class RuntimeErrorInfo : public llvm::ErrorInfo<RuntimeErrorInfo> {
private:
  std::string output;
  std::string type;
  Message message;
  std::vector<std::string> backtrace;

public:
  RuntimeErrorInfo(const std::string &output, const std::string &type,
                   const std::string &msg, const std::string &file = "", int line = 0,
                   int col = 0, std::vector<std::string> backtrace = {})
      : output(output), type(type), message(msg, file, line, col),
        backtrace(std::move(backtrace)) {}

  std::string getOutput() const { return output; }
  std::string getType() const { return type; }
  std::string getMessage() const { return message.getMessage(); }
  std::string getFile() const { return message.getFile(); }
  int getLine() const { return message.getLine(); }
  int getColumn() const { return message.getColumn(); }
  std::vector<std::string> getBacktrace() const { return backtrace; }

  void log(llvm::raw_ostream &out) const override {
    out << type << ": ";
    message.log(out);
  }

  std::error_code convertToErrorCode() const override {
    return llvm::inconvertibleErrorCode();
  }

  static char ID;
};

class PluginErrorInfo : public llvm::ErrorInfo<PluginErrorInfo> {
private:
  std::string message;

public:
  explicit PluginErrorInfo(const std::string &message) : message(message) {}

  std::string getMessage() const { return message; }

  void log(llvm::raw_ostream &out) const override { out << message; }

  std::error_code convertToErrorCode() const override {
    return llvm::inconvertibleErrorCode();
  }

  static char ID;
};

class IOErrorInfo : public llvm::ErrorInfo<IOErrorInfo> {
private:
  std::string message;

public:
  explicit IOErrorInfo(const std::string &message) : message(message) {}

  std::string getMessage() const { return message; }

  void log(llvm::raw_ostream &out) const override { out << message; }

  std::error_code convertToErrorCode() const override {
    return llvm::inconvertibleErrorCode();
  }

  static char ID;
};

} // namespace error
} // namespace codon
