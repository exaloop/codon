// Copyright (C) 2022-2025 Exaloop Inc. <https://exaloop.io>

#include "attribute.h"

#include "codon/cir/func.h"
#include "codon/cir/util/cloning.h"
#include "codon/cir/value.h"
#include <fmt/ostream.h>

namespace codon {
namespace ir {

const int StringValueAttribute::AttributeID = 101;
const int IntValueAttribute::AttributeID = 102;
const int StringListAttribute::AttributeID = 103;
const int KeyValueAttribute::AttributeID = 104;
const int MemberAttribute::AttributeID = 105;
const int PythonWrapperAttribute::AttributeID = 106;
const int SrcInfoAttribute::AttributeID = 107;
const int DocstringAttribute::AttributeID = 108;
const int TupleLiteralAttribute::AttributeID = 109;
const int ListLiteralAttribute::AttributeID = 111;
const int SetLiteralAttribute::AttributeID = 111;
const int DictLiteralAttribute::AttributeID = 112;
const int PartialFunctionAttribute::AttributeID = 113;

std::ostream &StringListAttribute::doFormat(std::ostream &os) const {
  fmt::print(os, FMT_STRING("{}"), fmt::join(values.begin(), values.end(), ","));
  return os;
}

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

std::ostream &MemberAttribute::doFormat(std::ostream &os) const {
  std::vector<std::string> strings;
  for (auto &val : memberSrcInfo)
    strings.push_back(fmt::format(FMT_STRING("{}={}"), val.first, val.second));
  fmt::print(os, FMT_STRING("({})"), fmt::join(strings.begin(), strings.end(), ","));
  return os;
}

std::unique_ptr<Attribute> PythonWrapperAttribute::clone(util::CloneVisitor &cv) const {
  return std::make_unique<PythonWrapperAttribute>(cast<Func>(cv.clone(original)));
}

std::unique_ptr<Attribute>
PythonWrapperAttribute::forceClone(util::CloneVisitor &cv) const {
  return std::make_unique<PythonWrapperAttribute>(cv.forceClone(original));
}

std::ostream &PythonWrapperAttribute::doFormat(std::ostream &os) const {
  fmt::print(os, FMT_STRING("(pywrap {})"), original->referenceString());
  return os;
}

std::unique_ptr<Attribute> TupleLiteralAttribute::clone(util::CloneVisitor &cv) const {
  std::vector<Value *> elementsCloned;
  for (auto *val : elements)
    elementsCloned.push_back(cv.clone(val));
  return std::make_unique<TupleLiteralAttribute>(elementsCloned);
}

std::unique_ptr<Attribute>
TupleLiteralAttribute::forceClone(util::CloneVisitor &cv) const {
  std::vector<Value *> elementsCloned;
  for (auto *val : elements)
    elementsCloned.push_back(cv.forceClone(val));
  return std::make_unique<TupleLiteralAttribute>(elementsCloned);
}

std::ostream &TupleLiteralAttribute::doFormat(std::ostream &os) const {
  std::vector<std::string> strings;
  for (auto *val : elements)
    strings.push_back(fmt::format(FMT_STRING("{}"), *val));
  fmt::print(os, FMT_STRING("({})"), fmt::join(strings.begin(), strings.end(), ","));
  return os;
}

std::unique_ptr<Attribute> ListLiteralAttribute::clone(util::CloneVisitor &cv) const {
  std::vector<LiteralElement> elementsCloned;
  for (auto &e : elements)
    elementsCloned.push_back({cv.clone(e.value), e.star});
  return std::make_unique<ListLiteralAttribute>(elementsCloned);
}

std::unique_ptr<Attribute>
ListLiteralAttribute::forceClone(util::CloneVisitor &cv) const {
  std::vector<LiteralElement> elementsCloned;
  for (auto &e : elements)
    elementsCloned.push_back({cv.forceClone(e.value), e.star});
  return std::make_unique<ListLiteralAttribute>(elementsCloned);
}

std::ostream &ListLiteralAttribute::doFormat(std::ostream &os) const {
  std::vector<std::string> strings;
  for (auto &e : elements)
    strings.push_back(fmt::format(FMT_STRING("{}{}"), e.star ? "*" : "", *e.value));
  fmt::print(os, FMT_STRING("[{}]"), fmt::join(strings.begin(), strings.end(), ","));
  return os;
}

std::unique_ptr<Attribute> SetLiteralAttribute::clone(util::CloneVisitor &cv) const {
  std::vector<LiteralElement> elementsCloned;
  for (auto &e : elements)
    elementsCloned.push_back({cv.clone(e.value), e.star});
  return std::make_unique<SetLiteralAttribute>(elementsCloned);
}

std::unique_ptr<Attribute>
SetLiteralAttribute::forceClone(util::CloneVisitor &cv) const {
  std::vector<LiteralElement> elementsCloned;
  for (auto &e : elements)
    elementsCloned.push_back({cv.forceClone(e.value), e.star});
  return std::make_unique<SetLiteralAttribute>(elementsCloned);
}

std::ostream &SetLiteralAttribute::doFormat(std::ostream &os) const {
  std::vector<std::string> strings;
  for (auto &e : elements)
    strings.push_back(fmt::format(FMT_STRING("{}{}"), e.star ? "*" : "", *e.value));
  fmt::print(os, FMT_STRING("set([{}])"),
             fmt::join(strings.begin(), strings.end(), ","));
  return os;
}

std::unique_ptr<Attribute> DictLiteralAttribute::clone(util::CloneVisitor &cv) const {
  std::vector<DictLiteralAttribute::KeyValuePair> elementsCloned;
  for (auto &val : elements)
    elementsCloned.push_back(
        {cv.clone(val.key), val.value ? cv.clone(val.value) : nullptr});
  return std::make_unique<DictLiteralAttribute>(elementsCloned);
}

std::unique_ptr<Attribute>
DictLiteralAttribute::forceClone(util::CloneVisitor &cv) const {
  std::vector<DictLiteralAttribute::KeyValuePair> elementsCloned;
  for (auto &val : elements)
    elementsCloned.push_back(
        {cv.forceClone(val.key), val.value ? cv.forceClone(val.value) : nullptr});
  return std::make_unique<DictLiteralAttribute>(elementsCloned);
}

std::ostream &DictLiteralAttribute::doFormat(std::ostream &os) const {
  std::vector<std::string> strings;
  for (auto &val : elements) {
    if (val.value) {
      strings.push_back(fmt::format(FMT_STRING("{}:{}"), *val.key, *val.value));
    } else {
      strings.push_back(fmt::format(FMT_STRING("**{}"), *val.key));
    }
  }
  fmt::print(os, FMT_STRING("dict([{}])"),
             fmt::join(strings.begin(), strings.end(), ","));
  return os;
}

std::unique_ptr<Attribute>
PartialFunctionAttribute::clone(util::CloneVisitor &cv) const {
  std::vector<Value *> argsCloned;
  for (auto *val : args)
    argsCloned.push_back(cv.clone(val));
  return std::make_unique<PartialFunctionAttribute>(name, argsCloned);
}

std::unique_ptr<Attribute>
PartialFunctionAttribute::forceClone(util::CloneVisitor &cv) const {
  std::vector<Value *> argsCloned;
  for (auto *val : args)
    argsCloned.push_back(cv.forceClone(val));
  return std::make_unique<PartialFunctionAttribute>(name, argsCloned);
}

std::ostream &PartialFunctionAttribute::doFormat(std::ostream &os) const {
  std::vector<std::string> strings;
  for (auto *val : args)
    strings.push_back(val ? fmt::format(FMT_STRING("{}"), *val) : "...");
  fmt::print(os, FMT_STRING("{}({})"), name,
             fmt::join(strings.begin(), strings.end(), ","));
  return os;
}

} // namespace ir

std::unordered_map<int, std::unique_ptr<ir::Attribute>>
clone(const std::unordered_map<int, std::unique_ptr<ir::Attribute>> &t) {
  std::unordered_map<int, std::unique_ptr<ir::Attribute>> r;
  for (auto &[k, v] : t)
    r[k] = v->clone();
  return r;
}

} // namespace codon
