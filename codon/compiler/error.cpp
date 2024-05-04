// Copyright (C) 2022-2024 Exaloop Inc. <https://exaloop.io>

#include "error.h"

namespace codon {

SrcInfo::SrcInfo(std::string file, int line, int col, int len)
    : file(std::move(file)), line(line), col(col), len(len), id(0) {
  if (this->file.empty() && line != 0)
    line++;
  static int nextId = 0;
  id = nextId++;
};

SrcInfo::SrcInfo() : SrcInfo("", 0, 0, 0) {}

bool SrcInfo::operator==(const SrcInfo &src) const { return id == src.id; }

namespace error {

char ParserErrorInfo::ID = 0;

char RuntimeErrorInfo::ID = 0;

char PluginErrorInfo::ID = 0;

char IOErrorInfo::ID = 0;

void raise_error(const char *format) { throw exc::ParserException(format); }

void raise_error(int e, const ::codon::SrcInfo &info, const char *format) {
  throw exc::ParserException(e, format, info);
}

void raise_error(int e, const ::codon::SrcInfo &info, const std::string &format) {
  throw exc::ParserException(e, format, info);
}

} // namespace error
} // namespace codon
