// Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>

#include "codon/parser/ast.h"
#include "codon/parser/visitors/scoping/scoping.h"

#include <algorithm>
#include <charconv>
#include <cmath>
#include <cstdint>
#include <map>
#include <optional>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace codon::ast {
namespace {

using Formatted = detail::CodonString;

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
  const bool useDouble =
      value.find('\'') != std::string::npos && value.find('"') == std::string::npos;
  const char delimiter = useDouble ? '"' : '\'';
  std::string result(1, delimiter);
  constexpr char hex[] = "0123456789abcdef";
  for (unsigned char c : value) {
    if (c == static_cast<unsigned char>(delimiter) || c == '\\') {
      result += '\\';
      result += static_cast<char>(c);
    } else if (c == '\a') {
      result += "\\a";
    } else if (c == '\b') {
      result += "\\b";
    } else if (c == '\f') {
      result += "\\f";
    } else if (c == '\n') {
      result += "\\n";
    } else if (c == '\r') {
      result += "\\r";
    } else if (c == '\t') {
      result += "\\t";
    } else if (c == '\v') {
      result += "\\v";
    } else if (c < 32 || c == 127) {
      result += "\\x";
      result += hex[c >> 4];
      result += hex[c & 15];
    } else {
      result += static_cast<char>(c);
    }
  }
  result += delimiter;
  return result;
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
  char buffer[64];
  auto [end, error] = std::to_chars(buffer, buffer + sizeof(buffer), value);
  if (error != std::errc())
    return {"", true};
  std::string result(buffer, end);
  if (result.find_first_of(".eE") == std::string::npos)
    result += ".0";
  return {result, true};
}

Formatted formatValue(const ASTNode *value, bool attributes, int indent, int level) {
  return value ? value->formatCodonString(attributes, indent, level)
               : Formatted{"", true};
}

template <typename T>
  requires std::is_base_of_v<ASTNode, T>
Formatted formatValue(const T *value, bool attributes, int indent, int level) {
  return formatValue(static_cast<const ASTNode *>(value), attributes, indent, level);
}

Formatted formatValue(const types::Type *value, bool, int, int) {
  return value ? Formatted{value->debugString(2), true} : Formatted{"", true};
}

Formatted formatValue(const types::TypePtr &value, bool attributes, int indent,
                      int level) {
  return formatValue(value.get(), attributes, indent, level);
}

Formatted formatValue(const Param &value, bool attributes, int indent, int level);
Formatted formatValue(const StringExpr::String &value, bool attributes, int indent,
                      int level);
Formatted formatValue(const Pipe &value, bool attributes, int indent, int level);
Formatted formatValue(const CallArg &value, bool attributes, int indent, int level);
Formatted formatValue(const MatchCase &value, bool attributes, int indent, int level);

template <typename T, typename U>
Formatted formatValue(const std::pair<T, U> &value, bool attributes, int indent,
                      int level);

template <typename T>
Formatted formatValue(const std::optional<T> &value, bool attributes, int indent,
                      int level) {
  return value ? formatValue(*value, attributes, indent, level) : Formatted{"", true};
}

template <typename T>
Formatted formatValue(const std::vector<T> &values, bool attributes, int indent,
                      int level) {
  if (values.empty())
    return {"", true};
  level = nestedLevel(indent, level);
  std::vector<std::string> items;
  items.reserve(values.size());
  for (const auto &value : values)
    items.push_back(formatValue(value, attributes, indent, level).value);
  auto formatted = formatItems(std::move(items), true, 1, indent, level);
  return {"[" + formatted.value + "]", formatted.simple};
}

template <typename T, typename U>
Formatted formatValue(const std::pair<T, U> &value, bool attributes, int indent,
                      int level) {
  level = nestedLevel(indent, level);
  std::vector<std::string> items{
      formatValue(value.first, attributes, indent, level).value,
      formatValue(value.second, attributes, indent, level).value};
  auto formatted = formatItems(std::move(items), true, 1, indent, level);
  return {"(" + formatted.value + ")", formatted.simple};
}

Field field(const std::string &name, Formatted value) {
  return {value.value.empty() ? "" : name + "=" + value.value, value.simple};
}

Formatted formatRecord(const std::string &name, std::vector<Field> fields, int indent,
                       int level) {
  level = nestedLevel(indent, level);
  bool allSimple = true;
  std::vector<std::string> args;
  for (auto &value : fields) {
    allSimple = allSimple && value.simple;
    if (!value.value.empty())
      args.push_back(std::move(value.value));
  }
  auto formatted = formatItems(std::move(args), allSimple, 1, indent, level);
  return {name + "(" + formatted.value + ")", formatted.simple};
}

Formatted formatValue(BindingsAttribute::CaptureType value, bool, int, int) {
  switch (value) {
  case BindingsAttribute::Read:
    return {"Read", true};
  case BindingsAttribute::Global:
    return {"Global", true};
  case BindingsAttribute::Nonlocal:
    return {"Nonlocal", true};
  }
  return {"", true};
}

Formatted formatValue(const BindingsAttribute::Binding &value, bool, int, int) {
  return {"Bindings.Binding(name=" + quote(value.name) +
              ", count=" + std::to_string(value.count) +
              ", is_nonlocal=" + (value.isNonlocal ? "True" : "False") + ")",
          true};
}

template <typename T>
Formatted formatDict(const std::unordered_map<std::string, T> &values, bool attributes,
                     int indent, int level) {
  if (values.empty())
    return {"", true};
  level = nestedLevel(indent, level);
  std::map<std::string, const T *> sorted;
  for (const auto &[key, value] : values)
    sorted.emplace(key, &value);
  std::vector<std::string> items;
  items.reserve(sorted.size());
  for (const auto &[key, value] : sorted)
    items.push_back(quote(key) + "=" +
                    formatValue(*value, attributes, indent, level).value);
  auto formatted = formatItems(std::move(items), true, 1, indent, level);
  return {"{" + formatted.value + "}", formatted.simple};
}

Formatted formatBindings(const BindingsAttribute &value, bool attributes, int indent,
                         int level) {
  const int child = nestedLevel(indent, level);
  return formatRecord(
      "Bindings",
      {field("captures", formatDict(value.captures, attributes, indent, child)),
       field("bindings", formatDict(value.bindings, attributes, indent, child)),
       field("local_renames",
             formatDict(value.localRenames, attributes, indent, child))},
      indent, level);
}

const char *attributeName(int key) {
  switch (key) {
#define ATTRIBUTE_NAME(name)                                                           \
  case Attr::name:                                                                     \
    return #name
    ATTRIBUTE_NAME(Module);
    ATTRIBUTE_NAME(ParentClass);
    ATTRIBUTE_NAME(Bindings);
    ATTRIBUTE_NAME(LLVM);
    ATTRIBUTE_NAME(Python);
    ATTRIBUTE_NAME(Atomic);
    ATTRIBUTE_NAME(Property);
    ATTRIBUTE_NAME(StaticMethod);
    ATTRIBUTE_NAME(Attribute);
    ATTRIBUTE_NAME(C);
    ATTRIBUTE_NAME(Internal);
    ATTRIBUTE_NAME(HiddenFromUser);
    ATTRIBUTE_NAME(ForceRealize);
    ATTRIBUTE_NAME(AllowPassThrough);
    ATTRIBUTE_NAME(ParentCallExpr);
    ATTRIBUTE_NAME(TupleCall);
    ATTRIBUTE_NAME(Validated);
    ATTRIBUTE_NAME(AutoGenerated);
    ATTRIBUTE_NAME(CVarArg);
    ATTRIBUTE_NAME(Method);
    ATTRIBUTE_NAME(Capture);
    ATTRIBUTE_NAME(HasSelf);
    ATTRIBUTE_NAME(IsGenerator);
    ATTRIBUTE_NAME(Extend);
    ATTRIBUTE_NAME(Tuple);
    ATTRIBUTE_NAME(Dataclass);
    ATTRIBUTE_NAME(ClassDeduce);
    ATTRIBUTE_NAME(ClassNoTuple);
    ATTRIBUTE_NAME(Test);
    ATTRIBUTE_NAME(Overload);
    ATTRIBUTE_NAME(Export);
    ATTRIBUTE_NAME(Inline);
    ATTRIBUTE_NAME(NoArgReorder);
    ATTRIBUTE_NAME(FunctionAttributes);
    ATTRIBUTE_NAME(NoExtend);
    ATTRIBUTE_NAME(ClassMagic);
    ATTRIBUTE_NAME(ExprSequenceItem);
    ATTRIBUTE_NAME(ExprStarSequenceItem);
    ATTRIBUTE_NAME(ExprList);
    ATTRIBUTE_NAME(ExprSet);
    ATTRIBUTE_NAME(ExprDict);
    ATTRIBUTE_NAME(ExprPartial);
    ATTRIBUTE_NAME(ExprDominated);
    ATTRIBUTE_NAME(ExprStarArgument);
    ATTRIBUTE_NAME(ExprKwStarArgument);
    ATTRIBUTE_NAME(ExprOrderedCall);
    ATTRIBUTE_NAME(ExprExternVar);
    ATTRIBUTE_NAME(ExprNoUndefCheck);
    ATTRIBUTE_NAME(ExprDominatedUsed);
    ATTRIBUTE_NAME(ExprTime);
    ATTRIBUTE_NAME(ExprDoNotRealize);
    ATTRIBUTE_NAME(ExprNoSpecial);
    ATTRIBUTE_NAME(TryPyVar);
    ATTRIBUTE_NAME(LocalRenames);
#undef ATTRIBUTE_NAME
  default:
    return nullptr;
  }
}

Formatted
formatKeyValueAttribute(const std::unordered_map<std::string, std::string> &values) {
  std::map<std::string, std::string> sorted(values.begin(), values.end());
  std::vector<std::string> items;
  items.reserve(sorted.size());
  for (const auto &[key, value] : sorted)
    items.push_back(quote(key) + ": " + quote(value));
  return {"KeyValueAttribute(attributes={" + join(items, ", ") + "})", true};
}

Formatted formatAttribute(const ir::Attribute *attribute, bool attributes, int indent,
                          int level) {
  if (!attribute)
    return {"", true};
  if (auto *value = dynamic_cast<const ir::StringValueAttribute *>(attribute))
    return formatValue(value->value, attributes, indent, level);
  if (auto *value = dynamic_cast<const ir::IntValueAttribute *>(attribute))
    return formatValue(value->value, attributes, indent, level);
  if (auto *value = dynamic_cast<const ir::StringListAttribute *>(attribute))
    return formatValue(value->values, attributes, indent, level);
  if (auto *value = dynamic_cast<const ir::KeyValueAttribute *>(attribute))
    return formatKeyValueAttribute(value->attributes);
  if (auto *value = dynamic_cast<const BindingsAttribute *>(attribute))
    return formatBindings(*value, attributes, indent, level);
  std::ostringstream out;
  out << *attribute;
  return out.str().empty() ? Formatted{"", true} : Formatted{out.str(), true};
}

Formatted formatAttributes(const ASTNode *node, int indent, int level) {
  struct AttributeValue {
    std::string name;
    Formatted value;
  };
  std::vector<AttributeValue> values;
  for (auto it = node->attributes_begin(); it != node->attributes_end(); ++it) {
    if (auto *name = attributeName(*it)) {
      values.push_back(
          {name, formatAttribute(node->getAttribute(*it), true, indent, level + 2)});
    }
  }
  if (values.empty())
    return {"", true};
  std::sort(values.begin(), values.end(),
            [](const auto &lhs, const auto &rhs) { return lhs.name < rhs.name; });
  bool allSimple = true;
  std::vector<std::string> items;
  items.reserve(values.size());
  for (const auto &value : values) {
    allSimple = allSimple && value.value.simple;
    items.push_back(value.name +
                    (value.value.value.empty() ? "" : "=" + value.value.value));
  }
  auto formatted = formatItems(std::move(items), allSimple, 1, indent, level + 1);
  return {"attrs=(" + formatted.value + ")", formatted.simple};
}

Formatted formatNode(const std::string &name, const ASTNode *node, bool done,
                     std::vector<Field> fields, bool attributes, int indent,
                     int level) {
  level = nestedLevel(indent, level);
  bool allSimple = true;
  std::vector<std::string> args;
  for (auto &value : fields) {
    allSimple = allSimple && value.simple;
    if (!value.value.empty())
      args.push_back(std::move(value.value));
  }
  if (attributes) {
    auto value = formatAttributes(node, indent, level);
    allSimple = allSimple && value.simple;
    if (!value.value.empty())
      args.push_back(std::move(value.value));
  }

  auto nodeName = name + (done ? "*" : "");
  auto formatted = formatItems(std::move(args), allSimple, 1, indent, level);
  return {nodeName + "(" + formatted.value + ")", formatted.simple};
}

Formatted formatExprNode(const std::string &name, const Expr *node,
                         std::vector<Field> fields, bool attributes, int indent,
                         int level) {
  const int child = nestedLevel(indent, level);
  std::vector<Field> all{
      field("type", formatValue(node->getType(), attributes, indent, child)),
      field("orig", formatValue(node->getOrigExpr(), attributes, indent, child)),
      field("expected_type",
            formatValue(node->getExpectedType(), attributes, indent, child))};
  all.insert(all.end(), std::make_move_iterator(fields.begin()),
             std::make_move_iterator(fields.end()));
  return formatNode(name, node, node->isDone(), std::move(all), attributes, indent,
                    level);
}

Formatted formatSuite(const SuiteStmt *node, const std::vector<Stmt *> &items,
                      bool attributes, int indent, int level) {
  if (items.empty())
    return {"", true};
  level = nestedLevel(indent, level);
  std::vector<std::string> args;
  args.reserve(items.size() + 1);
  bool allSimple = true;
  for (auto *item : items) {
    auto value = formatValue(item, attributes, indent, level);
    allSimple = allSimple && value.simple;
    if (!value.value.empty())
      args.push_back(std::move(value.value));
  }
  if (attributes) {
    auto value = formatAttributes(node, indent, level);
    allSimple = allSimple && value.simple;
    if (!value.value.empty())
      args.push_back(std::move(value.value));
  }
  auto formatted = formatItems(std::move(args), allSimple, 1, indent, level);
  return {(node->isDone() ? "*[" : "[") + formatted.value + "]", formatted.simple};
}

Formatted formatValue(decltype(Param::status) value, bool, int, int) {
  switch (value) {
  case Param::Value:
    return {"Value", true};
  case Param::Generic:
    return {"Generic", true};
  case Param::HiddenGeneric:
    return {"HiddenGeneric", true};
  }
  return {std::to_string(value), true};
}

Formatted formatValue(GeneratorExpr::GeneratorKind value, bool, int, int) {
  switch (value) {
  case GeneratorExpr::Generator:
    return {"Generator", true};
  case GeneratorExpr::ListGenerator:
    return {"ListGenerator", true};
  case GeneratorExpr::SetGenerator:
    return {"SetGenerator", true};
  case GeneratorExpr::TupleGenerator:
    return {"TupleGenerator", true};
  case GeneratorExpr::DictGenerator:
    return {"DictGenerator", true};
  }
  return {std::to_string(value), true};
}

Formatted formatValue(EllipsisExpr::EllipsisType value, bool, int, int) {
  switch (value) {
  case EllipsisExpr::PIPE:
    return {"Pipe", true};
  case EllipsisExpr::PARTIAL:
    return {"Partial", true};
  case EllipsisExpr::STANDALONE:
    return {"Standalone", true};
  }
  return {std::to_string(value), true};
}

Formatted formatValue(AssignStmt::UpdateMode value, bool, int, int) {
  switch (value) {
  case AssignStmt::Assign:
    return {"Assign", true};
  case AssignStmt::Update:
    return {"Update", true};
  case AssignStmt::UpdateAtomic:
    return {"UpdateAtomic", true};
  case AssignStmt::ThreadLocalAssign:
    return {"ThreadLocalAssign", true};
  }
  return {std::to_string(value), true};
}

Formatted formatStruct(const std::string &name, std::vector<Field> fields, int indent,
                       int level) {
  level = nestedLevel(indent, level);
  bool allSimple = true;
  std::vector<std::string> args;
  for (auto &value : fields) {
    allSimple = allSimple && value.simple;
    if (!value.value.empty())
      args.push_back(std::move(value.value));
  }
  auto formatted = formatItems(std::move(args), allSimple, 1, indent, level);
  return {name + "(" + formatted.value + ")", formatted.simple};
}

Formatted formatValue(const StringExpr::FormatSpec &value, bool attributes, int indent,
                      int level) {
  const int child = nestedLevel(indent, level);
  if (value.text.empty() && value.conversion.empty() && value.spec.empty())
    return {"", true};
  return formatStruct(
      "StringExpr.FormatSpec",
      {field("text", formatValue(value.text, attributes, indent, child)),
       field("conversion", formatValue(value.conversion, attributes, indent, child)),
       field("spec", formatValue(value.spec, attributes, indent, child))},
      indent, level);
}

Formatted formatValue(const Param &value, bool attributes, int indent, int level) {
  const int child = nestedLevel(indent, level);
  return formatStruct(
      "Param",
      {field("name", formatValue(value.name, attributes, indent, child)),
       field("type", formatValue(value.type, attributes, indent, child)),
       field("default", formatValue(value.defaultValue, attributes, indent, child)),
       field("status", formatValue(value.status, attributes, indent, child))},
      indent, level);
}

Formatted formatValue(const StringExpr::String &value, bool attributes, int indent,
                      int level) {
  const int child = nestedLevel(indent, level);
  return formatStruct(
      "String",
      {field("value", formatValue(value.value, attributes, indent, child)),
       field("prefix", formatValue(value.prefix, attributes, indent, child)),
       field("expr", formatValue(value.expr, attributes, indent, child)),
       field("format", formatValue(value.format, attributes, indent, child))},
      indent, level);
}

Formatted formatValue(const Pipe &value, bool attributes, int indent, int level) {
  const int child = nestedLevel(indent, level);
  return formatStruct(
      "Pipe",
      {field("op", formatValue(value.op, attributes, indent, child)),
       field("expr", formatValue(value.expr, attributes, indent, child))},
      indent, level);
}

Formatted formatValue(const CallArg &value, bool attributes, int indent, int level) {
  const int child = nestedLevel(indent, level);
  return formatStruct(
      "Arg",
      {field("name", formatValue(value.name, attributes, indent, child)),
       field("value", formatValue(value.value, attributes, indent, child))},
      indent, level);
}

Formatted formatValue(const MatchCase &value, bool attributes, int indent, int level) {
  const int child = nestedLevel(indent, level);
  return formatStruct(
      "Case",
      {field("pattern", formatValue(value.getPattern(), attributes, indent, child)),
       field("guard", formatValue(value.getGuard(), attributes, indent, child)),
       field("suite", formatValue(value.getSuite(), attributes, indent, child))},
      indent, level);
}

#define FIELD(name, value)                                                             \
  field(name, formatValue(value, attributes, indent, nestedLevel(indent, level)))

#define EXPR_IMPL(type, ...)                                                           \
  detail::CodonString type::formatCodonString(bool attributes, int indent, int level)  \
      const {                                                                          \
    return formatExprNode(#type, this, {__VA_ARGS__}, attributes, indent, level);      \
  }

#define STMT_IMPL(type, ...)                                                           \
  detail::CodonString type::formatCodonString(bool attributes, int indent, int level)  \
      const {                                                                          \
    return formatNode(#type, this, isDone(), {__VA_ARGS__}, attributes, indent,        \
                      level);                                                          \
  }

} // namespace

std::string ASTNode::toCodonString(bool attributes, int indent, int level) const {
  return formatCodonString(attributes, indent, level).value;
}

EXPR_IMPL(NoneExpr)
EXPR_IMPL(BoolExpr, FIELD("value", value))
EXPR_IMPL(IntExpr, FIELD("value", value), FIELD("suffix", suffix),
          FIELD("int_value", intValue))
EXPR_IMPL(FloatExpr, FIELD("value", value), FIELD("suffix", suffix),
          FIELD("float_value", floatValue))

detail::CodonString StringExpr::formatCodonString(bool attributes, int indent,
                                                  int level) const {
  const auto value = strings.size() == 1 ? strings.front().value : "";
  const auto stringPrefix = strings.size() == 1 ? strings.front().prefix : "";
  return formatExprNode(
      "StringExpr", this,
      {FIELD("strings", strings), FIELD("value", value), FIELD("prefix", stringPrefix)},
      attributes, indent, level);
}

EXPR_IMPL(IdExpr, FIELD("value", value))
EXPR_IMPL(StarExpr, FIELD("expr", expr))
EXPR_IMPL(KeywordStarExpr, FIELD("expr", getExpr()))
EXPR_IMPL(TupleExpr, FIELD("items", items))
EXPR_IMPL(ListExpr, FIELD("items", items))
EXPR_IMPL(SetExpr, FIELD("items", items))
EXPR_IMPL(DictExpr, FIELD("items", items))
EXPR_IMPL(GeneratorExpr, FIELD("kind", kind), FIELD("loops", loops))
EXPR_IMPL(IfExpr, FIELD("cond", cond), FIELD("ifexpr", ifexpr),
          FIELD("elsexpr", elsexpr))
EXPR_IMPL(UnaryExpr, FIELD("op", op), FIELD("expr", expr))
EXPR_IMPL(BinaryExpr, FIELD("lexpr", lexpr), FIELD("op", op), FIELD("rexpr", rexpr),
          FIELD("in_place", inPlace))
EXPR_IMPL(ChainBinaryExpr, FIELD("exprs", exprs))
EXPR_IMPL(PipeExpr, FIELD("items", items), FIELD("in_types", inTypes))
EXPR_IMPL(IndexExpr, FIELD("expr", expr), FIELD("index", index))
EXPR_IMPL(CallExpr, FIELD("items", items), FIELD("expr", expr),
          FIELD("ordered", ordered), FIELD("partial", partial))
EXPR_IMPL(DotExpr, FIELD("expr", expr), FIELD("member", member))
EXPR_IMPL(SliceExpr, FIELD("start", start), FIELD("stop", stop), FIELD("step", step))
EXPR_IMPL(EllipsisExpr, FIELD("mode", mode))
EXPR_IMPL(LambdaExpr, FIELD("items", items), FIELD("expr", expr))
EXPR_IMPL(YieldExpr)
EXPR_IMPL(AwaitExpr, FIELD("expr", expr), FIELD("transformed", transformed))
EXPR_IMPL(AssignExpr, FIELD("var", var), FIELD("expr", expr))
EXPR_IMPL(RangeExpr, FIELD("start", start), FIELD("stop", stop))
EXPR_IMPL(StmtExpr, FIELD("items", items), FIELD("expr", expr))
EXPR_IMPL(InstantiateExpr, FIELD("items", items), FIELD("expr", expr))

detail::CodonString SuiteStmt::formatCodonString(bool attributes, int indent,
                                                 int level) const {
  return formatSuite(this, items, attributes, indent, level);
}

STMT_IMPL(BreakStmt)
STMT_IMPL(ContinueStmt)
STMT_IMPL(ExprStmt, FIELD("expr", expr))
STMT_IMPL(AssignStmt, FIELD("lhs", lhs), FIELD("rhs", rhs), FIELD("type_expr", type),
          FIELD("update", update))
STMT_IMPL(DelStmt, FIELD("expr", expr))
STMT_IMPL(PrintStmt, FIELD("items", items), FIELD("no_newline", noNewline))
STMT_IMPL(ReturnStmt, FIELD("expr", expr))
STMT_IMPL(YieldStmt, FIELD("expr", expr))
STMT_IMPL(AssertStmt, FIELD("expr", expr), FIELD("message", message))
STMT_IMPL(WhileStmt, FIELD("cond", cond), FIELD("suite", suite),
          FIELD("else_suite", elseSuite), FIELD("goto_var", gotoVar))
STMT_IMPL(ForStmt, FIELD("var", var), FIELD("iter", iter), FIELD("suite", suite),
          FIELD("else_suite", elseSuite), FIELD("decorator", decorator),
          FIELD("omp_args", ompArgs), FIELD("async_", async), FIELD("wrapped", wrapped),
          FIELD("flat", flat))
STMT_IMPL(IfStmt, FIELD("cond", cond), FIELD("if_suite", ifSuite),
          FIELD("else_suite", elseSuite))
STMT_IMPL(MatchStmt, FIELD("items", items), FIELD("expr", expr))
STMT_IMPL(ImportStmt, FIELD("from_expr", from), FIELD("what", what),
          FIELD("args", args), FIELD("ret", ret), FIELD("as_", as), FIELD("dots", dots),
          FIELD("is_function", isFunction))
detail::CodonString ExceptStmt::formatCodonString(bool attributes, int indent,
                                                  int level) const {
  return formatNode("Except", this, isDone(),
                    {FIELD("var", var), FIELD("exc", exc), FIELD("suite", suite)},
                    attributes, indent, level);
}
STMT_IMPL(TryStmt, FIELD("items", items), FIELD("suite", suite),
          FIELD("else_suite", elseSuite), FIELD("finally_suite", finally))
STMT_IMPL(ThrowStmt, FIELD("expr", expr), FIELD("from_expr", from),
          FIELD("transformed", transformed))
STMT_IMPL(GlobalStmt, FIELD("var", var), FIELD("non_local", nonLocal))
STMT_IMPL(FunctionStmt, FIELD("items", items), FIELD("name", name), FIELD("ret", ret),
          FIELD("suite", suite), FIELD("decorators", decorators),
          FIELD("async_", async), FIELD("signature", signature))
STMT_IMPL(ClassStmt, FIELD("items", items), FIELD("name", name), FIELD("suite", suite),
          FIELD("decorators", decorators), FIELD("base_classes", baseClasses))
STMT_IMPL(YieldFromStmt, FIELD("expr", expr))
STMT_IMPL(WithStmt, FIELD("items", items), FIELD("vars", vars), FIELD("suite", suite),
          FIELD("async_", async))
STMT_IMPL(CustomStmt, FIELD("keyword", keyword), FIELD("expr", expr),
          FIELD("suite", suite))
STMT_IMPL(DirectiveStmt, FIELD("key", key), FIELD("value", value))
STMT_IMPL(AssignMemberStmt, FIELD("lhs", lhs), FIELD("member", member),
          FIELD("rhs", rhs), FIELD("type_expr", type))
STMT_IMPL(CommentStmt, FIELD("comment", comment))

#undef STMT_IMPL
#undef EXPR_IMPL
#undef FIELD

} // namespace codon::ast
