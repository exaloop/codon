// Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>

#include "error.h"
#include <mutex>
#include <unordered_set>

namespace codon {

InternedString::InternedString(std::string_view file) {
  if (file.empty())
    return;
  // Parsing usually creates many consecutive locations in the same file.
  static thread_local const std::string *last = nullptr;
  if (last && *last == file) {
    value = last;
    return;
  }
  struct Pool {
    std::mutex mutex;
    std::unordered_set<std::string> files;
  };
  // Locations can outlive compiler instances and static destructors.
  static auto *pool = new Pool;
  std::lock_guard<std::mutex> lock(pool->mutex);
  auto [entry, inserted] = pool->files.emplace(file);
  value = last = &*entry;
}

SrcInfo::SrcInfo(std::string_view file, int line, int col, int len)
    : file(file), line(line), col(col), len(len), id(0) {
  if (this->file.empty() && line != 0)
    line++;
  static int nextId = 0;
  id = nextId++;
};

SrcInfo::SrcInfo() : SrcInfo("", 0, 0, 0) {}

bool SrcInfo::operator==(const SrcInfo &src) const { return id == src.id; }

bool SrcInfo::operator<(const SrcInfo &src) const {
  return std::tie(file, line, col) < std::tie(src.file, src.line, src.col);
}

bool SrcInfo::operator<=(const SrcInfo &src) const {
  return std::tie(file, line, col) <= std::tie(src.file, src.line, src.col);
}

std::string ErrorMessage::toString() const {
  std::string s;
  if (!getFile().empty()) {
    s += getFile();
    if (getLine() != 0) {
      s += fmt::format(":{}", getLine());
      if (getColumn() != 0)
        s += fmt::format(":{}", getColumn());
    }
    s += ": ";
  }
  s += getMessage();
  return s;
}

namespace error {

char ParserErrorInfo::ID = 0;

char RuntimeErrorInfo::ID = 0;

char PluginErrorInfo::ID = 0;

char IOErrorInfo::ID = 0;

void E(llvm::Error &&error) { throw exc::ParserException(std::move(error)); }

} // namespace error

namespace exc {

ParserException::ParserException(llvm::Error &&e) noexcept : std::runtime_error("") {
  llvm::handleAllErrors(std::move(e), [this](const error::ParserErrorInfo &e) {
    errors = e.getErrors();
  });
}

} // namespace exc
} // namespace codon
