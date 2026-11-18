// Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>

#include <algorithm>
#include <charconv>
#include <cmath>
#include <fmt/format.h>
#include <fmt/ostream.h>
#include <functional>
#include <map>
#include <sstream>
#include <unordered_map>
#include <unordered_set>

#include "codon/cir/util/format.h"
#include "codon/cir/util/visitor.h"
#include "codon/parser/ast/dump.h"

namespace codon {
namespace ir {
namespace util {

/// Dump-local names and ordering, independent of frontend cache iteration and
/// CIR allocation order. No node, signature, call or compiler counter is changed.
struct FormatContext {
  bool canonical;
  std::unordered_map<id_t, id_t> ids;
  std::vector<const Var *> vars;
  std::vector<const Type *> types;
  std::unordered_map<std::string, std::vector<size_t>> captureOrder;
  std::unordered_map<std::string, std::string> aliases;

  explicit FormatContext(const Node *node) : canonical(isA<Module>(node)) {
    auto *module = cast<Module>(node);
    if (!module)
      return;
    std::unordered_map<id_t, std::string> keys;
    std::vector<const Func *> functions{module->getMainFunc()};
    for (const auto *var : *module)
      if (auto *fn = cast<Func>(var))
        functions.push_back(fn);
    generatorAliases(functions);
    for (const auto *fn : functions) {
      const auto owner = name(fn);
      keys[fn->getId()] = "func:" + owner;
      auto arguments = args(fn);
      // A generated comprehension closure captures an unordered identifier set.
      // Show bindings in name order at declarations, signatures and calls, while
      // leaving ordinary positional arguments and the actual capture ABI alone.
      if (fn->isGenerator() && fn->getType() && !fn->getType()->getAstType()) {
        std::vector<size_t> order;
        for (size_t i = 0; i < arguments.size(); ++i)
          order.push_back(i);
        std::sort(order.begin(), order.end(), [&](size_t a, size_t b) {
          return arguments[a]->getName() < arguments[b]->getName();
        });
        captureOrder[fn->getType()->getName()] = order;
        arguments = reorder(arguments, fn->getType()->getName());
      }
      for (size_t i = 0; i < arguments.size(); ++i)
        keys[arguments[i]->getId()] =
            "arg:" + owner + ":" + arguments[i]->getName() + ":" + std::to_string(i);
      if (auto *body = cast<BodiedFunc>(fn)) {
        size_t i = 0;
        for (const auto *var : *body)
          keys[var->getId()] =
              "local:" + owner + ":" + var->getName() + ":" + std::to_string(i++);
      }
    }
    for (const auto *var : *module) {
      vars.push_back(var);
      keys.try_emplace(var->getId(), "var:" + name(var));
    }
    std::stable_sort(vars.begin(), vars.end(), [&](const Var *a, const Var *b) {
      return keys.at(a->getId()) < keys.at(b->getId());
    });
    for (auto it = module->types_begin(); it != module->types_end(); ++it)
      types.push_back(*it);
    std::sort(types.begin(), types.end(),
              [&](const Type *a, const Type *b) { return name(a) < name(b); });
    nodeId(module->getArgvVar()->getId());
    nodeId(module->getArgcVar()->getId());
    nodeId(module->getMainFunc()->getId());
    for (const auto *var : vars)
      nodeId(var->getId());
    for (const auto *var : vars)
      if (auto *fn = cast<BodiedFunc>(var))
        numberValues(fn->getBody());
    if (auto *main = cast<BodiedFunc>(module->getMainFunc()))
      numberValues(main->getBody());
  }

  id_t nodeId(id_t id) {
    if (!canonical)
      return id;
    return ids.try_emplace(id, ids.size()).first->second;
  }

  std::string name(const Node *node) const {
    auto it = aliases.find(node->getName());
    return it == aliases.end() ? node->getName() : it->second;
  }

  /// Generated ordinals depend on unordered function-cache traversal. Use the
  /// semantic owner and occurrence within its body only in printed names.
  void generatorAliases(const std::vector<const Func *> &functions) {
    std::unordered_map<id_t, const Func *> generators, owners;
    std::unordered_map<id_t, size_t> positions;
    for (const auto *fn : functions) {
      auto raw = fn->getName();
      if (fn->isGenerator() && raw.starts_with("_generator.") && raw.size() > 11 &&
          std::all_of(raw.begin() + 11, raw.end(),
                      [](char ch) { return ch >= '0' && ch <= '9'; }))
        generators[fn->getId()] = fn;
    }
    for (const auto *fn : functions) {
      auto *body = cast<BodiedFunc>(fn);
      if (!body)
        continue;
      std::vector<const Value *> pending{body->getBody()};
      std::unordered_set<id_t> seen;
      size_t ordinal = 0;
      while (!pending.empty()) {
        auto *node = pending.back();
        pending.pop_back();
        if (!node || !seen.insert(node->getId()).second)
          continue;
        if (auto *ref = cast<VarValue>(node)) {
          auto id = ref->getVar()->getId();
          if (generators.count(id) && !positions.count(id)) {
            owners[id] = fn;
            positions[id] = ordinal++;
          }
        }
        for (const auto *child : node->getUsedValues())
          pending.push_back(child);
      }
    }
    std::function<std::string(const Func *)> alias = [&](const Func *fn) {
      if (auto it = aliases.find(fn->getName()); it != aliases.end())
        return it->second;
      const auto *parent = owners.count(fn->getId()) ? owners.at(fn->getId()) : nullptr;
      // Do not collapse distinct orphan declarations onto one name.
      if (!parent)
        return aliases[fn->getName()] = fn->getName();
      auto owner =
          generators.count(parent->getId()) ? alias(parent) : parent->getName();
      auto result = "_generator.context." + std::to_string(owner.size()) + ":" + owner +
                    "." + std::to_string(positions[fn->getId()]);
      aliases[fn->getName()] = result;
      return result;
    };
    for (const auto &[_, fn] : generators)
      alias(fn);
  }

  std::string reference(const Type *node) { return node->referenceString(); }
  std::string reference(const Var *node) {
    return fmt::format("{}.{}", node->getName(), nodeId(node->getId()));
  }
  std::string reference(const Value *node) {
    return fmt::format("{}.{}", node->getName(), nodeId(node->getId()));
  }

  template <typename T>
  std::vector<T> reorder(const std::vector<T> &items, const std::string &type) const {
    auto it = captureOrder.find(type);
    if (it == captureOrder.end())
      return items;
    std::vector<T> result;
    for (auto index : it->second)
      result.push_back(items.at(index));
    return result;
  }

  std::vector<const Var *> args(const Func *fn) const {
    std::vector<const Var *> result(fn->arg_begin(), fn->arg_end());
    return fn->getType() ? reorder(result, fn->getType()->getName()) : result;
  }

  std::vector<const Value *> callArgs(const CallInstr *call) const {
    std::vector<const Value *> result(call->begin(), call->end());
    auto *callee = cast<VarValue>(call->getCallee());
    auto *fn = callee ? cast<Func>(callee->getVar()) : nullptr;
    return fn && fn->getType() ? reorder(result, fn->getType()->getName()) : result;
  }

  std::vector<const Value *> seriesValues(const SeriesFlow *series) const {
    std::vector<const Value *> values;
    for (const auto *value : *series)
      values.push_back(value);
    if (!canonical)
      return values;
    // Adjacent literal stores to distinct variables commute. The frontend, in
    // particular, emits compile-time defines from an unordered map. Preserve all
    // other evaluation order, including successive writes to the same variable.
    size_t start = 0;
    while (start < values.size()) {
      size_t end = start;
      std::unordered_set<id_t> seen;
      while (end < values.size()) {
        auto *assignment = cast<AssignInstr>(values[end]);
        if (!assignment || !isA<Const>(assignment->getRhs()) ||
            !seen.insert(assignment->getLhs()->getId()).second)
          break;
        ++end;
      }
      std::stable_sort(values.begin() + start, values.begin() + end,
                       [](const Value *a, const Value *b) {
                         return cast<AssignInstr>(a)->getLhs()->getName() <
                                cast<AssignInstr>(b)->getLhs()->getName();
                       });
      start = std::max(end, start + 1);
    }
    return values;
  }

  void numberValues(const Value *value) {
    if (!value || ids.count(value->getId()))
      return;
    nodeId(value->getId());
    auto children = value->getUsedValues();
    if (auto *call = cast<CallInstr>(value)) {
      children = callArgs(call);
      children.push_back(call->getCallee());
    } else if (auto *series = cast<SeriesFlow>(value))
      children = seriesValues(series);
    for (const auto *child : children)
      numberValues(child);
  }
};

namespace {

struct Formatted {
  std::string value;
  bool simple;
};

struct Field {
  std::string value;
  bool simple;
};

int nestedLevel(int indent, int level) { return indent > 0 ? level + 1 : level; }

std::string prefix(int indent, int level) {
  return indent > 0 ? "\n" + std::string(indent * level, ' ') : "";
}

std::string separator(int indent, int level) {
  return indent > 0 ? ",\n" + std::string(indent * level, ' ') : ", ";
}

std::string join(const std::vector<std::string> &values, const std::string &sep) {
  std::string result;
  for (size_t i = 0; i < values.size(); ++i) {
    if (i)
      result += sep;
    result += values[i];
  }
  return result;
}

Formatted formatItems(std::vector<std::string> items, bool simple, size_t maxItems,
                      int indent, int level) {
  items.erase(std::remove(items.begin(), items.end(), ""), items.end());
  if ((simple && items.size() <= maxItems) || indent <= 0)
    return {join(items, ", "), true};
  return {prefix(indent, level) + join(items, separator(indent, level)), false};
}

std::string quote(const std::string &value) {
  return codon::ast::quoteDumpString(value);
}

Formatted formatValue(const std::string &value, bool, int, int) {
  return value.empty() ? Formatted{"", true} : Formatted{quote(value), true};
}

Formatted formatValue(bool value, bool, int, int) {
  return {value ? "True" : "False", true};
}

template <typename T>
  requires(std::is_integral_v<T> && !std::is_same_v<T, bool>)
Formatted formatValue(T value, bool, int, int) {
  return {std::to_string(value), true};
}

Formatted formatValue(double value, bool, int, int) {
  if (std::isnan(value))
    return {"nan", true};
  if (std::isinf(value))
    return {value < 0 ? "-inf" : "inf", true};
  std::string result = fmt::format("{}", value);
  // Python's float repr uses fixed notation for decimal exponents in [-4, 16).
  auto exponentPos = result.find_first_of("eE");
  if (exponentPos != std::string::npos) {
    auto exponentBegin = result.data() + exponentPos + 1;
    if (exponentBegin != result.data() + result.size() && *exponentBegin == '+')
      ++exponentBegin;
    int exponent = 0;
    auto [exponentEnd, exponentError] =
        std::from_chars(exponentBegin, result.data() + result.size(), exponent);
    if (exponentError == std::errc() && exponentEnd == result.data() + result.size() &&
        exponent >= -4 && exponent < 16) {
      const bool negative = result.front() == '-';
      std::string digits =
          result.substr(negative ? 1 : 0, exponentPos - (negative ? 1 : 0));
      digits.erase(std::remove(digits.begin(), digits.end(), '.'), digits.end());

      const auto decimalPos = exponent + 1;
      result = negative ? "-" : "";
      if (decimalPos <= 0) {
        result += "0." + std::string(-decimalPos, '0') + digits;
      } else if (decimalPos >= static_cast<int>(digits.size())) {
        result += digits + std::string(decimalPos - digits.size(), '0') + ".0";
      } else {
        result += digits.substr(0, decimalPos) + "." + digits.substr(decimalPos);
      }
    }
  }
  if (result.find_first_of(".eE") == std::string::npos)
    result += ".0";
  return {result, true};
}

class FormatVisitor : public util::ConstVisitor {
  FormatContext &ctx;
  bool annotateFields, includeAttributes, showTypes;
  int indent, level = 0;
  bool payloadReferences = false;
  std::unordered_set<id_t> seenNodes;
  std::unordered_set<std::string> seenTypes;
  Formatted result;

  int childLevel() const { return nestedLevel(indent, level); }

  Field field(const std::string &name, Formatted value) const {
    return {value.value.empty() ? "" : (annotateFields ? name + "=" : "") + value.value,
            value.simple};
  }

  Formatted record(const std::string &name, std::vector<Field> fields, int at,
                   const Node *node = nullptr) {
    bool simple = true;
    std::vector<std::string> values;
    for (auto &item : fields) {
      simple &= item.simple;
      if (!item.value.empty())
        values.push_back(std::move(item.value));
    }
    if (node && includeAttributes) {
      auto attrs = attributes(node, nestedLevel(indent, at));
      simple &= attrs.simple;
      if (!attrs.value.empty())
        values.push_back(std::move(attrs.value));
    }
    auto items =
        formatItems(std::move(values), simple, 1, indent, nestedLevel(indent, at));
    return {name + "(" + items.value + ")", items.simple};
  }

  Formatted reference(const Node *node, int at) {
    if (!node)
      return {"", true};
    std::vector<Field> fields{
        field("name", value(ctx.name(node), nestedLevel(indent, at)))};
    const char *name = "TypeRef";
    if (auto *var = cast<Var>(node)) {
      fields.push_back(
          field("id", value(ctx.nodeId(var->getId()), nestedLevel(indent, at))));
      name = "VarRef";
    } else if (auto *val = cast<Value>(node)) {
      fields.push_back(
          field("id", value(ctx.nodeId(val->getId()), nestedLevel(indent, at))));
      name = "ValueRef";
    }
    return record(name, std::move(fields), at);
  }

  Formatted node(const Node *node, int at, bool full = false) {
    if (!node)
      return {"", true};
    node = cast<Node>(node);
    if (payloadReferences)
      return reference(node, at);
    if (auto *type = cast<Type>(node)) {
      if (!full || !seenTypes.insert(type->getName()).second)
        return reference(node, at);
    } else if (auto *var = cast<Var>(node)) {
      if (!full || !seenNodes.insert(var->getId()).second)
        return reference(node, at);
    } else if (auto *val = cast<Value>(node)) {
      if (!seenNodes.insert(val->getId()).second)
        return reference(node, at);
    }
    int previous = level;
    level = at;
    node->accept(*this);
    auto formatted = result;
    level = previous;
    return formatted;
  }

  Formatted value(const Node *v, int at) { return node(v, at); }
  Formatted value(const std::string &v, int at) {
    return formatValue(v, includeAttributes, indent, at);
  }
  Formatted value(bool v, int at) {
    return formatValue(v, includeAttributes, indent, at);
  }
  template <typename T>
    requires(std::is_integral_v<T> && !std::is_same_v<T, bool>)
  Formatted value(T v, int at) {
    return formatValue(v, includeAttributes, indent, at);
  }
  Formatted value(double v, int at) {
    return formatValue(v, includeAttributes, indent, at);
  }
  Formatted value(const ast::types::TypePtr &v, int) {
    return {v ? v->debugString(2) : "", true};
  }

  template <typename It> Formatted sequence(It begin, It end, int at) {
    std::vector<std::string> values;
    for (auto it = begin; it != end; ++it) {
      auto formatted = value(*it, nestedLevel(indent, at));
      // Null positional slots carry meaning in pipelines and partial functions.
      // Omit null fields like AST dumps, but never collapse a positional slot.
      if constexpr (std::is_pointer_v<std::decay_t<decltype(*it)>>)
        if (!*it)
          formatted.value = "None";
      values.push_back(std::move(formatted.value));
    }
    if (values.empty())
      return {"", true};
    auto items =
        formatItems(std::move(values), true, 1, indent, nestedLevel(indent, at));
    return {"[" + items.value + "]", items.simple};
  }

  template <typename T> Formatted value(const std::vector<T> &v, int at) {
    return sequence(v.begin(), v.end(), at);
  }

  template <typename It> Formatted definitions(It begin, It end, int at) {
    std::vector<std::string> values;
    for (auto it = begin; it != end; ++it)
      values.push_back(node(*it, nestedLevel(indent, at), true).value);
    if (values.empty())
      return {"", true};
    auto items =
        formatItems(std::move(values), true, 1, indent, nestedLevel(indent, at));
    return {"[" + items.value + "]", items.simple};
  }

  template <typename K, typename V> Formatted value(const std::map<K, V> &v, int at) {
    std::vector<std::string> values;
    for (const auto &[key, item] : v) {
      std::string name;
      if constexpr (std::is_same_v<K, std::string>)
        name = quote(key);
      else
        name = std::to_string(key);
      values.push_back(name + "=" + value(item, nestedLevel(indent, at)).value);
    }
    if (values.empty())
      return {"", true};
    auto items =
        formatItems(std::move(values), true, 1, indent, nestedLevel(indent, at));
    return {"{" + items.value + "}", items.simple};
  }

#define C(name, x) field(name, value(x, child))
  Formatted value(const Type::RuntimeInfo *v, int at) {
    if (!v)
      return {"", true};
    int child = nestedLevel(indent, at);
    return record("RuntimeInfo",
                  {C("id", v->id), C("raw_name", v->rawName),
                   C("nice_name", v->niceName), C("base_name", v->baseName),
                   C("mro", v->mro), C("field_owners", v->fieldOwners),
                   C("parameters", v->parameters), C("methods", v->methods),
                   C("value_accessed", v->valueAccessed),
                   C("exception_base", v->exceptionBase),
                   C("system_exit_base", v->systemExitBase)},
                  at);
  }
  Formatted value(const MemberedType::Field &v, int at) {
    int child = nestedLevel(indent, at);
    return record("Field", {C("name", v.getName()), C("type", v.getType())}, at);
  }
  Formatted value(const transform::parallel::OMPSched *v, int at) {
    if (!v)
      return {"", true};
    int child = nestedLevel(indent, at);
    return record("OMPSched",
                  {C("code", v->code), C("dynamic", v->dynamic),
                   C("threads", v->threads), C("chunk", v->chunk),
                   C("ordered", v->ordered), C("collapse", v->collapse),
                   C("gpu", v->gpu)},
                  at);
  }
  Formatted value(const TryCatchFlow::Catch &v, int at) {
    int child = nestedLevel(indent, at);
    return record(
        "Catch",
        {C("handler", v.getHandler()), C("type", v.getType()), C("var", v.getVar())},
        at);
  }
  Formatted value(const PipelineFlow::Stage &v, int at) {
    int child = nestedLevel(indent, at);
    return record("Stage",
                  {C("callee", v.getCallee()),
                   field("args", sequence(v.begin(), v.end(), child)),
                   C("generator", v.isGenerator()), C("parallel", v.isParallel())},
                  at);
  }
  Formatted value(const Generic &v, int at) {
    int child = nestedLevel(indent, at);
    return record(
        "Generic",
        {field("value", v.isStatic()      ? value(v.getStaticValue(), child)
                        : v.isStaticStr() ? value(v.getStaticStringValue(), child)
                                          : reference(v.getTypeValue(), child))},
        at);
  }
  Formatted value(const LiteralElement &v, int at) {
    int child = nestedLevel(indent, at);
    return record("LiteralElement", {C("value", v.value), C("star", v.star)}, at);
  }
  Formatted value(const DictLiteralAttribute::KeyValuePair &v, int at) {
    int child = nestedLevel(indent, at);
    return record("KeyValuePair", {C("key", v.key), C("value", v.value)}, at);
  }

  std::string attributeName(int key, const Attribute *v) {
    if (auto *name = ast::attributeName(key))
      return name;
#define A(T)                                                                           \
  if (dynamic_cast<const T##Attribute *>(v))                                           \
  return #T
    A(StringValue);
    A(IntValue);
    A(StringList);
    A(KeyValue);
    A(Member);
    A(PythonWrapper);
    A(SrcInfo);
    A(Docstring);
    A(TupleLiteral);
    A(ListLiteral);
    A(SetLiteral);
    A(DictLiteral);
    A(PartialFunction);
    A(ExactType);
#undef A
    return std::to_string(key);
  }
  Formatted value(const Attribute *v, int at) {
    if (!v)
      return {"", true};
    int child = nestedLevel(indent, at);
    if (auto *a = dynamic_cast<const StringValueAttribute *>(v))
      return value(a->value, at);
    if (auto *a = dynamic_cast<const IntValueAttribute *>(v))
      return value(a->value, at);
    if (auto *a = dynamic_cast<const StringListAttribute *>(v))
      return value(a->values, at);
    if (auto *a = dynamic_cast<const KeyValueAttribute *>(v)) {
      std::map<std::string, std::string> sorted(a->attributes.begin(),
                                                a->attributes.end());
      std::vector<std::string> items;
      for (const auto &[key, item] : sorted)
        items.push_back(quote(key) + ": " + quote(item));
      return {"KeyValueAttribute(attributes={" + join(items, ", ") + "})", true};
    }
    if (auto *a = dynamic_cast<const MemberAttribute *>(v)) {
      std::vector<std::string> names;
      for (const auto &[name, _] : a->memberSrcInfo)
        names.push_back(name);
      std::sort(names.begin(), names.end());
      return record("MemberAttribute", {C("members", names)}, at);
    }
    if (auto *a = dynamic_cast<const ExactTypeAttribute *>(v))
      return record("ExactTypeAttribute", {C("type", a->type)}, at);
    if (auto *a = dynamic_cast<const DocstringAttribute *>(v))
      return record("DocstringAttribute", {C("docstring", a->docstring)}, at);
    if (auto *a = dynamic_cast<const PythonWrapperAttribute *>(v))
      return record("PythonWrapperAttribute", {C("original", a->original)}, at);
    if (auto *a = dynamic_cast<const TupleLiteralAttribute *>(v))
      return record("TupleLiteralAttribute", {C("elements", a->elements)}, at);
    if (auto *a = dynamic_cast<const ListLiteralAttribute *>(v))
      return record("ListLiteralAttribute", {C("elements", a->elements)}, at);
    if (auto *a = dynamic_cast<const SetLiteralAttribute *>(v))
      return record("SetLiteralAttribute", {C("elements", a->elements)}, at);
    if (auto *a = dynamic_cast<const DictLiteralAttribute *>(v))
      return record("DictLiteralAttribute", {C("elements", a->elements)}, at);
    if (auto *a = dynamic_cast<const PartialFunctionAttribute *>(v))
      return record("PartialFunctionAttribute",
                    {C("name", a->name), C("args", a->args)}, at);
    return {"", true};
  }
#undef C

  Formatted attributes(const Node *node, int at) {
    std::vector<std::pair<std::string, Formatted>> values;
    bool previous = payloadReferences;
    payloadReferences = true;
    for (auto it = node->attributes_begin(); it != node->attributes_end(); ++it) {
      if (*it == SrcInfoAttribute::AttributeID)
        continue;
      const auto *attr = node->getAttribute(*it);
      auto formatted = value(attr, at + 2);
      values.emplace_back(attributeName(*it, attr) +
                              (formatted.value.empty() ? "" : "=" + formatted.value),
                          formatted);
    }
    payloadReferences = previous;
    if (values.empty())
      return {"", true};
    std::sort(values.begin(), values.end(),
              [](auto &a, auto &b) { return a.first < b.first; });
    std::vector<std::string> items;
    bool simple = true;
    for (const auto &[name, formatted] : values) {
      items.push_back(name);
      simple &= formatted.simple;
    }
    auto formatted = formatItems(std::move(items), simple, 1, indent, at + 1);
    return {"attrs=(" + formatted.value + ")", formatted.simple};
  }

  std::vector<Field> common(const Node *v) {
    std::vector<Field> fields{field("name", value(ctx.name(v), childLevel()))};
    if (auto *var = cast<Var>(v)) {
      fields.push_back(field("id", value(ctx.nodeId(var->getId()), childLevel())));
      fields.push_back(field("type", value(var->getType(), childLevel())));
      fields.push_back(field("global_", value(var->isGlobal(), childLevel())));
      fields.push_back(field("external", value(var->isExternal(), childLevel())));
      fields.push_back(field("tls", value(var->isThreadLocal(), childLevel())));
    } else if (auto *val = cast<Value>(v)) {
      fields.push_back(field("id", value(ctx.nodeId(val->getId()), childLevel())));
      if (showTypes)
        fields.push_back(field("type", value(val->getType(), childLevel())));
    }
    if (auto *fn = cast<Func>(v)) {
      fields.push_back(
          field("unmangled_name", value(fn->getUnmangledName(), childLevel())));
      fields.push_back(field("generator", value(fn->isGenerator(), childLevel())));
      fields.push_back(field("async_", value(fn->isAsync(), childLevel())));
      fields.push_back(field("parent_type", value(fn->getParentType(), childLevel())));
      auto args = ctx.args(fn);
      fields.push_back(
          field("args", definitions(args.begin(), args.end(), childLevel())));
    }
    if (auto *type = cast<Type>(v)) {
      if (showTypes)
        fields.push_back(field("ast_type", value(type->getAstType(), childLevel())));
      fields.push_back(
          field("runtime_info", value(type->getRuntimeInfo(), childLevel())));
    }
    return fields;
  }

#define F(name, x) field(name, value(x, childLevel()))
#define EMIT(T, ...)                                                                   \
  void visit(const T *v) override {                                                    \
    auto fields = common(v);                                                           \
    std::vector<Field> extra{__VA_ARGS__};                                             \
    fields.insert(fields.end(), std::make_move_iterator(extra.begin()),                \
                  std::make_move_iterator(extra.end()));                               \
    result = record(#T, std::move(fields), level, v);                                  \
  }

  EMIT(Module, field("argv_var", node(v->getArgvVar(), childLevel(), true)),
       field("argc_var", node(v->getArgcVar(), childLevel(), true)),
       field("types", definitions(ctx.types.begin(), ctx.types.end(), childLevel())),
       field("vars", definitions(ctx.vars.begin(), ctx.vars.end(), childLevel())),
       field("main_func", node(v->getMainFunc(), childLevel(), true)))
  EMIT(Var)
  EMIT(BodiedFunc, field("symbols", definitions(v->begin(), v->end(), childLevel())),
       F("body", v->getBody()), F("jit", v->isJIT()))
  EMIT(ExternalFunc)
  EMIT(InternalFunc,
       field("intrinsic", Formatted{intrinsicName(v->getIntrinsic()), true}),
       F("intrinsic_type", v->getIntrinsicType()))
  EMIT(LLVMFunc,
       field("llvm_literals",
             sequence(v->literal_begin(), v->literal_end(), childLevel())),
       F("llvm_declares", v->getLLVMDeclarations()), F("llvm_body", v->getLLVMBody()))
  EMIT(VarValue, F("var", v->getVar()))
  EMIT(PointerValue, F("var", v->getVar()), F("fields", v->getFields()))
  EMIT(SeriesFlow, F("series", ctx.seriesValues(v)))
  EMIT(WhileFlow, F("cond", v->getCond()), F("body", v->getBody()))
  EMIT(ForFlow, F("iter", v->getIter()), F("body", v->getBody()), F("var", v->getVar()),
       F("schedule", v->getSchedule()), F("async_", v->isAsync()))
  EMIT(ImperativeForFlow, F("start", v->getStart()), F("step", v->getStep()),
       F("end", v->getEnd()), F("body", v->getBody()), F("var", v->getVar()),
       F("schedule", v->getSchedule()))
  EMIT(IfFlow, F("cond", v->getCond()), F("true_branch", v->getTrueBranch()),
       F("false_branch", v->getFalseBranch()))
  EMIT(TryCatchFlow, field("catches", sequence(v->begin(), v->end(), childLevel())),
       F("body", v->getBody()), F("else_", v->getElse()),
       F("finally_", v->getFinally()))
  EMIT(PipelineFlow, field("stages", sequence(v->begin(), v->end(), childLevel())))
  EMIT(IntConst, F("val", v->getVal()))
  EMIT(FloatConst, F("val", v->getVal()))
  EMIT(BoolConst, F("val", v->getVal()))
  EMIT(StringConst, F("val", v->getVal()))
  EMIT(BytesConst, F("val", v->getVal()))
  EMIT(AssignInstr, F("lhs", v->getLhs()), F("rhs", v->getRhs()))
  EMIT(ExtractInstr, F("val", v->getVal()), F("field", v->getField()),
       F("polymorphic", v->isPolymorphic()))
  EMIT(InsertInstr, F("lhs", v->getLhs()), F("field", v->getField()),
       F("rhs", v->getRhs()), F("polymorphic", v->isPolymorphic()))
  EMIT(CallInstr, F("callee", v->getCallee()), F("args", ctx.callArgs(v)))
  EMIT(StackAllocInstr, F("ptr_type", v->getPtrType()), F("count", v->getCount()))
  EMIT(YieldInInstr, F("suspend", v->isSuspending()))
  EMIT(TernaryInstr, F("cond", v->getCond()), F("true_value", v->getTrueValue()),
       F("false_value", v->getFalseValue()))
  EMIT(BreakInstr, field("loop", reference(v->getLoop(), childLevel())))
  EMIT(ContinueInstr, field("loop", reference(v->getLoop(), childLevel())))
  EMIT(ReturnInstr, F("value", v->getValue()))
  EMIT(YieldInstr, F("value", v->getValue()), F("final", v->isFinal()))
  EMIT(AwaitInstr, F("value", v->getValue()), F("generator", v->isGenerator()))
  EMIT(ThrowInstr, F("value", v->getValue()), F("fallback", v->getFallback()))
  EMIT(FlowInstr, F("flow", v->getFlow()), F("value", v->getValue()))
  EMIT(IntType, F("len", v->getLen()), F("sign", v->isSigned()))
  EMIT(FloatType)
  EMIT(Float32Type)
  EMIT(Float16Type)
  EMIT(BFloat16Type)
  EMIT(Float128Type)
  EMIT(BoolType)
  EMIT(RecordType, field("fields", sequence(v->begin(), v->end(), childLevel())))
  EMIT(RefType, F("contents", v->getContents()), F("polymorphic", v->isPolymorphic()))
      EMIT(FuncType, F("return_type", v->getReturnType()),
           F("arg_types",
             ctx.reorder(std::vector<Type *>(v->begin(), v->end()), v->getName())),
           F("variadic", v->isVariadic())) EMIT(PointerType, F("base", v->getBase()))
          EMIT(OptionalType, F("base", v->getBase()))
              EMIT(GeneratorType, F("base", v->getBase()))
                  EMIT(VectorType, F("count", v->getCount()), F("base", v->getBase()))
                      EMIT(UnionType,
                           field("types", sequence(v->begin(), v->end(), childLevel())))
#undef EMIT
#undef F

                          static const
      char *intrinsicName(InternalFunc::Intrinsic kind) {
    static const char *names[]{"NONE",
                               "ALLOCATE",
                               "ALLOCATION_SIZE",
                               "TYPEINFO",
                               "TYPEINFO_LOOKUP",
                               "TYPEINFO_METHOD_LOOKUP",
                               "VIRTUAL_LOOKUP",
                               "RECORD_NEW",
                               "REPRESENTATION_CAST",
                               "GENERATOR_PROMISE",
                               "SIZEOF",
                               "IS_ATOMIC",
                               "IS_CONTENT_ATOMIC"};
    return names[static_cast<size_t>(kind)];
  }

  void defaultVisit(const Node *v) override {
    result = record("Node", common(v), level, v);
  }

public:
  FormatVisitor(FormatContext &ctx, bool annotateFields, bool includeAttributes,
                int indent, bool types)
      : ctx(ctx), annotateFields(annotateFields), includeAttributes(includeAttributes),
        showTypes(types), indent(indent) {}
  std::string format(const Node *v) { return node(v, 0, true).value; }

  void chunks(const Node *v, const std::function<void(const std::string &)> &write) {
    auto *module = cast<Module>(v);
    if (!module || indent > 0) {
      write(format(v));
      return;
    }
    write("Module(");
    bool first = true;
    auto begin = [&](const std::string &key) {
      if (!first)
        write(", ");
      first = false;
      if (annotateFields && !key.empty())
        write(key + "=");
    };
    auto emit = [&](const std::string &key, const std::string &text) {
      if (!text.empty()) {
        begin(key);
        write(text);
      }
    };
    for (const auto &item : common(module))
      emit("", item.value);
    emit("argv_var", node(module->getArgvVar(), 0, true).value);
    emit("argc_var", node(module->getArgcVar(), 0, true).value);
    auto declarations = [&](const std::string &key, const auto &items) {
      if (items.empty())
        return;
      begin(key);
      write("[");
      bool firstItem = true;
      for (const auto *item : items) {
        if (!firstItem)
          write(", ");
        firstItem = false;
        write(node(item, 0, true).value);
      }
      write("]");
    };
    declarations("types", ctx.types);
    declarations("vars", ctx.vars);
    emit("main_func", node(module->getMainFunc(), 0, true).value);
    if (includeAttributes)
      emit("", attributes(module, 0).value);
    write(")");
  }
};

} // namespace

std::string format(const Node *node, bool annotateFields, bool includeAttributes,
                   int indent, bool types) {
  FormatContext ctx(node);
  return FormatVisitor(ctx, annotateFields, includeAttributes, indent, types)
      .format(node);
}

void formatChunks(const Node *node,
                  const std::function<void(const std::string &)> &write,
                  bool annotateFields, bool includeAttributes, int indent, bool types) {
  FormatContext ctx(node);
  FormatVisitor(ctx, annotateFields, includeAttributes, indent, types)
      .chunks(node, write);
}

std::ostream &format(std::ostream &os, const Node *node, bool annotateFields,
                     bool includeAttributes, int indent, bool types) {
  return os << format(node, annotateFields, includeAttributes, indent, types);
}

} // namespace util
} // namespace ir
} // namespace codon
