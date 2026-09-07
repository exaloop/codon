// Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>

#include "format.h"

#include <cstdint>
#include <limits>
#include <optional>
#include <string>
#include <vector>

#include "codon/cir/util/cloning.h"
#include "codon/cir/util/irtools.h"

namespace codon {
namespace ir {
namespace transform {
namespace pythonic {
namespace {

constexpr int FORMAT_FLAG_ALT = 1;
constexpr int FORMAT_FLAG_ZERO = 2;
constexpr int FORMAT_FLAG_SPACE = 8;
constexpr int FORMAT_FLAG_PLUS = 16;

struct Directive {
  int64_t flags = 0;
  int64_t width = 0;
  int64_t precision = -1;
  int64_t conversion = 0;
  int64_t alignment = 0;
  int64_t fill = 0x20;
  int64_t grouping = 0;
  bool locale = false;
  bool coerceNegativeZero = false;
  bool signMinus = false;
};

struct FieldAccess {
  enum Kind { MEMBER, INTEGER_ELEMENT, STRING_ELEMENT };

  Kind kind;
  std::string name;
  int64_t index = 0;
};

struct Part {
  std::string literal;
  int64_t index = -1;
  std::vector<FieldAccess> accesses;
  int64_t fieldConversion = 0;
  Directive directive;

  bool isField() const { return index >= 0; }
};

bool isAlignment(uint32_t c) { return c == '<' || c == '>' || c == '^' || c == '='; }

bool decodeCodepoint(const std::string &s, size_t &position, uint32_t &codepoint) {
  if (position >= s.size())
    return false;

  const auto first = static_cast<uint8_t>(s[position++]);
  if (first < 0x80) {
    codepoint = first;
    return true;
  }

  int continuationCount;
  uint32_t value;
  uint32_t minimum;
  if ((first & 0xe0) == 0xc0) {
    continuationCount = 1;
    value = first & 0x1f;
    minimum = 0x80;
  } else if ((first & 0xf0) == 0xe0) {
    continuationCount = 2;
    value = first & 0x0f;
    minimum = 0x800;
  } else if ((first & 0xf8) == 0xf0) {
    continuationCount = 3;
    value = first & 0x07;
    minimum = 0x10000;
  } else {
    return false;
  }

  for (int i = 0; i < continuationCount; ++i) {
    if (position >= s.size())
      return false;
    const auto next = static_cast<uint8_t>(s[position++]);
    if ((next & 0xc0) != 0x80)
      return false;
    value = (value << 6) | (next & 0x3f);
  }

  if (value < minimum || value > 0x10ffff || (value >= 0xd800 && value <= 0xdfff))
    return false;

  codepoint = value;
  return true;
}

bool appendDigit(int64_t &value, int digit) {
  if (value > (std::numeric_limits<int64_t>::max() - digit) / 10)
    return false;
  value = value * 10 + digit;
  return true;
}

bool parseDirective(const std::string &spec, Directive &directive) {
  size_t position = 0;

  if (!spec.empty()) {
    size_t firstEnd = 0;
    uint32_t first;
    if (!decodeCodepoint(spec, firstEnd, first))
      return false;

    if (firstEnd < spec.size() && isAlignment(static_cast<uint8_t>(spec[firstEnd]))) {
      directive.fill = first;
      directive.alignment = static_cast<uint8_t>(spec[firstEnd]);
      position = firstEnd + 1;
    } else if (isAlignment(first)) {
      directive.alignment = first;
      position = firstEnd;
    }
  }

  auto take = [&](char c) {
    if (position < spec.size() && spec[position] == c) {
      ++position;
      return true;
    }
    return false;
  };

  if (take('-'))
    directive.signMinus = true;
  else if (take(' '))
    directive.flags |= FORMAT_FLAG_SPACE;
  else if (take('+'))
    directive.flags |= FORMAT_FLAG_PLUS;

  directive.coerceNegativeZero = take('z');
  if (take('#'))
    directive.flags |= FORMAT_FLAG_ALT;
  if (take('0'))
    directive.flags |= FORMAT_FLAG_ZERO;

  while (position < spec.size() && spec[position] >= '0' && spec[position] <= '9') {
    if (!appendDigit(directive.width, spec[position++] - '0'))
      return false;
  }

  if (position < spec.size() && (spec[position] == ',' || spec[position] == '_'))
    directive.grouping = spec[position++];

  if (take('.')) {
    directive.precision = 0;
    while (position < spec.size() && spec[position] >= '0' && spec[position] <= '9') {
      if (!appendDigit(directive.precision, spec[position++] - '0'))
        return false;
    }
  }

  if (position == spec.size())
    return true;

  const auto conversion = static_cast<uint8_t>(spec[position++]);
  if (position != spec.size())
    return false;

  if (conversion == 'n') {
    directive.locale = true;
    return true;
  }

  const std::string allowed = "bsc%diuoxXeEfFgG";
  if (allowed.find(static_cast<char>(conversion)) == std::string::npos)
    return false;
  directive.conversion = conversion;
  return true;
}

bool parseIndex(const std::string &text, int64_t &index) {
  if (text.empty())
    return false;

  index = 0;
  for (char c : text) {
    if (c < '0' || c > '9' || !appendDigit(index, c - '0'))
      return false;
  }
  return true;
}

bool parseField(const std::string &text, int64_t &automaticIndex, int &numberingMode,
                Part &part) {
  size_t position = 0;
  while (position < text.size() && text[position] != '.' && text[position] != '[' &&
         text[position] != '!' && text[position] != ':')
    ++position;

  const auto indexText = text.substr(0, position);
  if (indexText.empty()) {
    if (numberingMode == 2)
      return false;
    numberingMode = 1;
    part.index = automaticIndex++;
  } else {
    if (numberingMode == 1 || !parseIndex(indexText, part.index))
      return false;
    numberingMode = 2;
  }

  while (position < text.size() && (text[position] == '.' || text[position] == '[')) {
    if (text[position] == '.') {
      const auto start = ++position;
      while (position < text.size() && text[position] != '.' && text[position] != '[' &&
             text[position] != '!' && text[position] != ':' && text[position] != ']')
        ++position;
      if (position == start)
        return false;
      part.accesses.push_back(
          {FieldAccess::MEMBER, text.substr(start, position - start), 0});
      continue;
    }

    const auto start = ++position;
    const auto end = text.find(']', start);
    if (end == std::string::npos || end == start)
      return false;

    const auto key = text.substr(start, end - start);
    int64_t elementIndex;
    if (parseIndex(key, elementIndex))
      part.accesses.push_back({FieldAccess::INTEGER_ELEMENT, "", elementIndex});
    else
      part.accesses.push_back({FieldAccess::STRING_ELEMENT, key, 0});
    position = end + 1;
  }

  if (position < text.size() && text[position] == '!') {
    ++position;
    if (position >= text.size())
      return false;
    const auto conversion = text[position++];
    if (conversion != 's' && conversion != 'r' && conversion != 'a')
      return false;
    part.fieldConversion = conversion;
  }

  if (position == text.size())
    return true;
  if (text[position++] != ':')
    return false;

  const auto spec = text.substr(position);
  if (spec.find('{') != std::string::npos || spec.find('}') != std::string::npos)
    return false;
  return parseDirective(spec, part.directive);
}

std::optional<std::vector<Part>> parseFormat(const std::string &format) {
  std::vector<Part> parts;
  std::string literal;
  int64_t automaticIndex = 0;
  int numberingMode = 0;

  for (size_t position = 0; position < format.size();) {
    const auto c = format[position];
    if (c == '{') {
      if (position + 1 < format.size() && format[position + 1] == '{') {
        literal.push_back('{');
        position += 2;
        continue;
      }

      if (!literal.empty()) {
        Part part;
        part.literal = std::move(literal);
        parts.push_back(std::move(part));
        literal.clear();
      }

      const auto end = format.find('}', position + 1);
      if (end == std::string::npos)
        return std::nullopt;

      const auto fieldText = format.substr(position + 1, end - position - 1);
      if (fieldText.find('{') != std::string::npos)
        return std::nullopt;

      Part part;
      if (!parseField(fieldText, automaticIndex, numberingMode, part))
        return std::nullopt;
      parts.push_back(std::move(part));
      position = end + 1;
      continue;
    }

    if (c == '}') {
      if (position + 1 >= format.size() || format[position + 1] != '}')
        return std::nullopt;
      literal.push_back('}');
      position += 2;
      continue;
    }

    literal.push_back(c);
    ++position;
  }

  if (!literal.empty()) {
    Part part;
    part.literal = std::move(literal);
    parts.push_back(std::move(part));
  }
  return parts;
}

bool supportsParsedFormatting(Type *type, Module *module) {
  return isA<IntType>(type) || type->is(module->getFloatType()) ||
         type->is(module->getStringType());
}

std::vector<Type *> parsedHelperTypes(Type *valueType, Module *module) {
  std::vector<Type *> types = {valueType};
  for (int i = 0; i < 8; ++i)
    types.push_back(module->getIntType());
  for (int i = 0; i < 3; ++i)
    types.push_back(module->getBoolType());
  return types;
}

std::vector<Value *> parsedHelperArgs(Value *value, const Part &part, Module *module) {
  const auto &d = part.directive;
  return {value,
          module->getInt(part.fieldConversion),
          module->getInt(d.conversion),
          module->getInt(d.flags),
          module->getInt(d.width),
          module->getInt(d.precision),
          module->getInt(d.alignment),
          module->getInt(d.fill),
          module->getInt(d.grouping),
          module->getBool(d.locale),
          module->getBool(d.coerceNegativeZero),
          module->getBool(d.signMinus)};
}

Func *getParsedHelper(Type *valueType, Module *module) {
  return module->getOrRealizeFunc("_format_parsed",
                                  parsedHelperTypes(valueType, module), {},
                                  "std.internal.format");
}

struct ResolvedPart {
  Type *type = nullptr;
  std::vector<Func *> elementAccessors;
};

std::optional<ResolvedPart>
resolvePart(const Part &part, const std::vector<Type *> &elementTypes, Module *module) {
  if (part.index < 0 || part.index >= static_cast<int64_t>(elementTypes.size()))
    return std::nullopt;

  ResolvedPart resolved;
  resolved.type = elementTypes[part.index];
  resolved.elementAccessors.reserve(part.accesses.size());
  for (const auto &access : part.accesses) {
    if (access.kind == FieldAccess::MEMBER) {
      auto *membered = cast<MemberedType>(resolved.type);
      if (!membered || !(resolved.type = membered->getMemberType(access.name)))
        return std::nullopt;
      resolved.elementAccessors.push_back(nullptr);
      continue;
    }

    if (access.kind == FieldAccess::INTEGER_ELEMENT) {
      auto *record = cast<RecordType>(resolved.type);
      if (record && record->getName() == "Tuple") {
        const auto field = "item" + std::to_string(access.index + 1);
        if (!(resolved.type = record->getMemberType(field)))
          return std::nullopt;
        resolved.elementAccessors.push_back(nullptr);
        continue;
      }
    }

    auto *keyType = access.kind == FieldAccess::INTEGER_ELEMENT
                        ? module->getIntType()
                        : module->getStringType();
    auto *getitem = module->getOrRealizeMethod(
        resolved.type, Module::GETITEM_MAGIC_NAME, {resolved.type, keyType});
    if (!getitem)
      return std::nullopt;
    resolved.type = util::getReturnType(getitem);
    resolved.elementAccessors.push_back(getitem);
  }
  return resolved;
}

Value *applyAccesses(Value *value, const Part &part, const ResolvedPart &resolved,
                     Module *module) {
  for (size_t i = 0; i < part.accesses.size(); ++i) {
    const auto &access = part.accesses[i];
    if (access.kind == FieldAccess::MEMBER) {
      value = module->Nr<ExtractInstr>(value, access.name);
    } else if (!resolved.elementAccessors[i]) {
      value = util::tupleGet(value, access.index);
    } else {
      Value *key = access.kind == FieldAccess::INTEGER_ELEMENT
                       ? static_cast<Value *>(module->getInt(access.index))
                       : static_cast<Value *>(module->getString(access.name));
      value = util::call(resolved.elementAccessors[i], {value, key});
    }
  }
  return value;
}

} // namespace

const std::string FormattingOptimization::KEY = "core-pythonic-formatting-opt";

void FormattingOptimization::handle(CallInstr *v) {
  auto *module = v->getModule();
  auto *func = util::getFunc(v->getCallee());
  if (!func || !func->getParentType())
    return;

  const auto &name = func->getUnmangledName();
  if (name == "format" && func->getParentType()->is(module->getStringType()) &&
      v->numArgs() == 2) {
    auto *format = cast<StringConst>(v->front());
    auto *tupleType = cast<RecordType>(v->back()->getType());
    if (!format || !tupleType)
      return;

    auto parsed = parseFormat(format->getVal());
    if (!parsed)
      return;

    std::vector<Type *> elementTypes;
    for (const auto &field : *tupleType)
      elementTypes.push_back(field.getType());

    std::vector<ResolvedPart> resolvedParts(parsed->size());
    std::vector<Func *> helpers;
    helpers.reserve(parsed->size());
    for (size_t i = 0; i < parsed->size(); ++i) {
      const auto &part = (*parsed)[i];
      if (!part.isField()) {
        helpers.push_back(nullptr);
        continue;
      }

      auto resolved = resolvePart(part, elementTypes, module);
      if (!resolved || !supportsParsedFormatting(resolved->type, module))
        return;
      resolvedParts[i] = std::move(*resolved);
      auto *helper = getParsedHelper(resolvedParts[i].type, module);
      if (!helper)
        return;
      helpers.push_back(helper);
    }

    std::vector<Type *> pieceTypes(parsed->size(), module->getStringType());
    Func *cat = nullptr;
    if (pieceTypes.size() > 1) {
      auto *piecesType = module->getTupleType(pieceTypes);
      cat = module->getOrRealizeMethod(module->getStringType(), "cat", {piecesType});
      if (!cat)
        return;
    }

    auto *series = module->Nr<SeriesFlow>();
    util::CloneVisitor clone(module);
    auto *tupleVar = util::makeVar(clone.clone(v->back()), series,
                                   cast<BodiedFunc>(getParentFunc()));
    std::vector<Value *> pieces;
    pieces.reserve(parsed->size());
    for (size_t i = 0; i < parsed->size(); ++i) {
      const auto &part = (*parsed)[i];
      if (!part.isField()) {
        pieces.push_back(module->getString(part.literal));
        continue;
      }

      auto *value = util::tupleGet(module->Nr<VarValue>(tupleVar), part.index);
      value = applyAccesses(value, part, resolvedParts[i], module);
      pieces.push_back(util::call(helpers[i], parsedHelperArgs(value, part, module)));
    }

    Value *result;
    if (pieces.empty())
      result = module->getString("");
    else if (pieces.size() == 1)
      result = pieces.front();
    else
      result = util::call(cat, {util::makeTuple(pieces, module)});

    auto *replacement = module->Nr<FlowInstr>(series, result);
    replacement->setSrcInfo(v->getSrcInfo());
    v->replaceAll(replacement);
    return;
  }

  if (name == "__format__" && v->numArgs() == 2 &&
      supportsParsedFormatting(v->front()->getType(), module)) {
    auto *spec = cast<StringConst>(v->back());
    if (!spec)
      return;

    Directive directive;
    if (!parseDirective(spec->getVal(), directive))
      return;

    Part part;
    part.index = 0;
    part.directive = directive;
    auto *helper = getParsedHelper(v->front()->getType(), module);
    if (!helper)
      return;

    util::CloneVisitor clone(module);
    auto *replacement =
        util::call(helper, parsedHelperArgs(clone.clone(v->front()), part, module));
    replacement->setSrcInfo(v->getSrcInfo());
    v->replaceAll(replacement);
  }
}

} // namespace pythonic
} // namespace transform
} // namespace ir
} // namespace codon
