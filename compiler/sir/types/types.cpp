#include "types.h"

#include <algorithm>
#include <memory>
#include <utility>

#include "util/fmt/format.h"

#include "parser/cache.h"

#include "sir/module.h"
#include "sir/util/iterators.h"
#include "sir/util/visitor.h"
#include "sir/value.h"

namespace seq {
namespace ir {
namespace types {
namespace {
std::vector<seq::ast::types::TypePtr>
extractTypes(const std::vector<seq::ast::types::ClassType::Generic> &gens) {
  std::vector<seq::ast::types::TypePtr> ret;
  for (auto &g : gens)
    ret.push_back(g.type);
  return ret;
}
} // namespace

const char Type::NodeId = 0;

std::vector<Generic> Type::doGetGenerics() const {
  if (!astType)
    return {};

  std::vector<Generic> ret;
  for (auto &g : astType->getClass()->generics) {
    if (auto cls = g.type->getClass())
      ret.emplace_back(
          getModule()->getCache()->realizeType(cls, extractTypes(cls->generics)));
    else {
      seqassert(g.type->getStatic()->expr->staticValue.type == ast::StaticValue::INT,
                "IR only supports int statics");
      ret.emplace_back(g.type->getStatic()->expr->staticValue.getInt());
    }
  }

  return ret;
}

Value *Type::doConstruct(std::vector<Value *> args) {
  auto *module = getModule();
  std::vector<Type *> argTypes;
  for (auto *a : args)
    argTypes.push_back(a->getType());

  auto *fn = module->getOrRealizeMethod(this, Module::NEW_MAGIC_NAME, argTypes);
  if (!fn)
    return nullptr;

  return module->Nr<CallInstr>(module->Nr<VarValue>(fn), args);
}

const char PrimitiveType::NodeId = 0;

const char IntType::NodeId = 0;

const char FloatType::NodeId = 0;

const char BoolType::NodeId = 0;

const char ByteType::NodeId = 0;

const char VoidType::NodeId = 0;

const char MemberedType::NodeId = 0;

const char RecordType::NodeId = 0;

RecordType::RecordType(std::string name, std::vector<Type *> fieldTypes,
                       std::vector<std::string> fieldNames)
    : AcceptorExtend(std::move(name)) {
  for (auto i = 0; i < fieldTypes.size(); ++i) {
    fields.emplace_back(fieldNames[i], fieldTypes[i]);
  }
}

RecordType::RecordType(std::string name, std::vector<Type *> mTypes)
    : AcceptorExtend(std::move(name)) {
  for (int i = 0; i < mTypes.size(); ++i) {
    fields.emplace_back(std::to_string(i + 1), mTypes[i]);
  }
}

std::vector<Type *> RecordType::doGetUsedTypes() const {
  std::vector<Type *> ret;
  for (auto &f : fields)
    ret.push_back(const_cast<Type *>(f.getType()));
  return ret;
}

Type *RecordType::getMemberType(const std::string &n) const {
  auto it = std::find_if(fields.begin(), fields.end(),
                         [n](auto &x) { return x.getName() == n; });
  return it->getType();
}

int RecordType::getMemberIndex(const std::string &n) const {
  auto it = std::find_if(fields.begin(), fields.end(),
                         [n](auto &x) { return x.getName() == n; });
  int index = std::distance(fields.begin(), it);
  return (index < fields.size()) ? index : -1;
}

void RecordType::realize(std::vector<Type *> mTypes, std::vector<std::string> mNames) {
  fields.clear();
  for (auto i = 0; i < mTypes.size(); ++i) {
    fields.emplace_back(mNames[i], mTypes[i]);
  }
}

const char RefType::NodeId = 0;

Value *RefType::doConstruct(std::vector<Value *> args) {
  auto *module = getModule();

  auto *series = module->Nr<SeriesFlow>();
  auto *newFn = module->getOrRealizeMethod(this, Module::NEW_MAGIC_NAME, {});
  if (!newFn)
    return nullptr;

  auto *newValue = module->Nr<CallInstr>(module->Nr<VarValue>(newFn));
  series->push_back(newValue);

  std::vector<Type *> argTypes = {newValue->getType()};
  std::vector<Value *> newArgs = {newValue};
  for (auto *a : args) {
    argTypes.push_back(a->getType());
    newArgs.push_back(a);
  }

  auto *initFn = module->getOrRealizeMethod(this, Module::INIT_MAGIC_NAME, argTypes);
  if (!initFn)
    return nullptr;

  return module->Nr<FlowInstr>(
      series, module->Nr<CallInstr>(module->Nr<VarValue>(initFn), newArgs));
}

const char FuncType::NodeId = 0;

std::vector<Generic> FuncType::doGetGenerics() const {
  auto t = getAstType();
  if (!t)
    return {};
  auto astType = t->getFunc();
  if (!astType)
    return {};

  std::vector<Generic> ret;
  for (auto &g : astType->funcGenerics) {
    if (auto cls = g.type->getClass())
      ret.emplace_back(
          getModule()->getCache()->realizeType(cls, extractTypes(cls->generics)));
    else {
      seqassert(g.type->getStatic()->expr->staticValue.type == ast::StaticValue::INT,
                "IR only supports int statics");
      ret.emplace_back(g.type->getStatic()->expr->staticValue.getInt());
    }
  }

  return ret;
}

std::vector<Type *> FuncType::doGetUsedTypes() const {
  auto ret = argTypes;
  ret.push_back(rType);
  return ret;
}

const char DerivedType::NodeId = 0;

const char PointerType::NodeId = 0;

std::string PointerType::getInstanceName(Type *base) {
  return fmt::format(FMT_STRING("Pointer[{}]"), base->referenceString());
}

const char OptionalType::NodeId = 0;

std::string OptionalType::getInstanceName(Type *base) {
  return fmt::format(FMT_STRING("Optional[{}]"), base->referenceString());
}

const char GeneratorType::NodeId = 0;

std::string GeneratorType::getInstanceName(Type *base) {
  return fmt::format(FMT_STRING("Generator[{}]"), base->referenceString());
}

const char IntNType::NodeId = 0;

std::string IntNType::getInstanceName(unsigned int len, bool sign) {
  return fmt::format(FMT_STRING("{}Int{}"), sign ? "" : "U", len);
}

} // namespace types
} // namespace ir
} // namespace seq
