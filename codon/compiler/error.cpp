// Copyright (C) 2022-2024 Exaloop Inc. <https://exaloop.io>

#include "error.h"

namespace codon {
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
