#include "error.h"

namespace codon {
namespace error {

char ParserErrorInfo::ID = 0;

char RuntimeErrorInfo::ID = 0;

char PluginErrorInfo::ID = 0;

char IOErrorInfo::ID = 0;

void raise_error(const char *format) { throw exc::ParserException(format); }

void raise_error(const ::codon::SrcInfo &info, const char *format) {
  throw exc::ParserException(format, info);
}

void raise_error(const ::codon::SrcInfo &info, const std::string &format) {
  throw exc::ParserException(format, info);
}

} // namespace error

} // namespace codon
