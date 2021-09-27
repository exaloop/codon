#include "value.h"

#include "util/fmt/ostream.h"

namespace seq {
namespace ir {

const std::string KeyValueAttribute::AttributeName = "kvAttribute";

bool KeyValueAttribute::has(const std::string &key) const {
  return attributes.find(key) != attributes.end();
}

std::string KeyValueAttribute::get(const std::string &key) const {
  auto it = attributes.find(key);
  return it != attributes.end() ? it->second : "";
}

std::ostream &KeyValueAttribute::doFormat(std::ostream &os) const {
  std::vector<std::string> keys;
  for (auto &val : attributes)
    keys.push_back(val.second);
  fmt::print(os, FMT_STRING("{}"), fmt::join(keys.begin(), keys.end(), ","));
  return os;
}

const std::string MemberAttribute::AttributeName = "memberAttribute";

std::ostream &MemberAttribute::doFormat(std::ostream &os) const {
  std::vector<std::string> strings;
  for (auto &val : memberSrcInfo)
    strings.push_back(fmt::format(FMT_STRING("{}={}"), val.first, val.second));
  fmt::print(os, FMT_STRING("({})"), fmt::join(strings.begin(), strings.end(), ","));
  return os;
}

const std::string SrcInfoAttribute::AttributeName = "srcInfoAttribute";

} // namespace ir
} // namespace seq
