// Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>

#include "type.h"

#include <algorithm>
#include <memory>
#include <utility>

#include "codon/cir/module.h"
#include "codon/cir/util/irtools.h"
#include "codon/cir/util/iterators.h"
#include "codon/cir/util/visitor.h"
#include "codon/cir/value.h"
#include "codon/parser/cache.h"
#include <fmt/format.h>

namespace codon {
namespace ir {
namespace {
std::vector<codon::ast::types::TypePtr>
extractTypes(const std::vector<codon::ast::types::ClassType::Generic> &gens) {
  std::vector<codon::ast::types::TypePtr> ret;
  for (auto &g : gens)
    ret.push_back(g.type);
  return ret;
}
} // namespace

const char Type::NodeId = 0;

std::vector<Type *> Type::getUsedTypes() const {
  auto result = getActual()->doGetUsedTypes();
  if (auto *info = getRuntimeInfo()) {
    auto append = [&](Type *type) {
      if (type && type != getActual() &&
          std::find(result.begin(), result.end(), type) == result.end())
        result.push_back(type);
    };
    for (auto *type : info->mro)
      append(type);
    for (auto *type : info->fieldOwners)
      append(type);
    for (auto *type : info->parameters)
      append(type);
    append(info->exceptionBase);
    append(info->systemExitBase);
  }
  return result;
}

std::vector<Var *> Type::getUsedVariables() {
  std::vector<Var *> result;
  if (auto *info = getRuntimeInfo())
    for (const auto &[id, function] : info->methods)
      result.push_back(function);
  return result;
}

std::vector<const Var *> Type::getUsedVariables() const {
  auto variables = const_cast<Type *>(this)->getUsedVariables();
  return {variables.begin(), variables.end()};
}

int Type::replaceUsedVariable(id_t id, Var *newVar) {
  auto *replacement = cast<Func>(newVar);
  seqassertn(!newVar || replacement, "runtime method target must be a function");
  int count = 0;
  if (auto *info = getRuntimeInfo()) {
    for (auto iterator = info->methods.begin(); iterator != info->methods.end();) {
      if (iterator->second->getId() != id) {
        ++iterator;
        continue;
      }
      ++count;
      if (replacement) {
        iterator->second = replacement;
        ++iterator;
      } else {
        iterator = info->methods.erase(iterator);
      }
    }
  }
  return count;
}

std::vector<Generic> Type::doGetGenerics() const {
  if (!astType)
    return {};

  std::vector<Generic> ret;
  for (auto &g : astType->getClass()->generics) {
    if (auto ai = g.type->getIntStatic()) {
      ret.emplace_back(ai->value);
    } else if (auto ai = g.type->getBoolStatic()) {
      ret.emplace_back(int(ai->value));
    } else if (auto as = g.type->getStrStatic()) {
      ret.emplace_back(as->value);
    } else if (auto ac = g.type->getClass()) {
      ret.emplace_back(
          getModule()->getCache()->realizeType(ac, extractTypes(ac->generics)));
    } else {
      seqassertn(false, "IR only supports int, bool or str statics [{}]",
                 g.type->getSrcInfo());
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

std::string IntType::getInstanceName(unsigned int len, bool sign) {
  return fmt::format(FMT_STRING("{}Int{}"), sign ? "" : "U", len);
}

const char FloatType::NodeId = 0;

const char Float32Type::NodeId = 0;

const char Float16Type::NodeId = 0;

const char BFloat16Type::NodeId = 0;

const char Float128Type::NodeId = 0;

const char BoolType::NodeId = 0;

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
  return (it != fields.end()) ? it->getType() : nullptr;
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

bool RefType::doIsContentAtomic() const {
  auto *contents = getContents();
  return !std::any_of(contents->begin(), contents->end(),
                      [](auto &field) { return !field.getType()->isAtomic(); });
}

Value *RefType::doConstruct(std::vector<Value *> args) {
  auto *module = getModule();
  auto *argsTuple = util::makeTuple(args, module);
  auto *constructFn = module->getOrRealizeFunc("construct_ref", {argsTuple->getType()},
                                               {this}, "std.internal.gc");
  if (!constructFn)
    return nullptr;

  std::vector<Value *> callArgs = {argsTuple};
  return module->Nr<CallInstr>(module->Nr<VarValue>(constructFn), callArgs);
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
    if (auto ai = g.type->getIntStatic()) {
      ret.emplace_back(ai->value);
    } else if (auto ai = g.type->getBoolStatic()) {
      ret.emplace_back(int(ai->value));
    } else if (auto as = g.type->getStrStatic()) {
      ret.emplace_back(as->value);
    } else if (auto ac = g.type->getClass()) {
      ret.emplace_back(
          getModule()->getCache()->realizeType(ac, extractTypes(ac->generics)));
    } else {
      seqassertn(false, "IR only supports int, bool or str statics [{}]",
                 g.type->getSrcInfo());
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

const char VectorType::NodeId = 0;

std::string VectorType::getInstanceName(unsigned int count, PrimitiveType *base) {
  return fmt::format(FMT_STRING("Vector[{}, {}]"), count, base->referenceString());
}

const char UnionType::NodeId = 0;

std::string UnionType::getInstanceName(const std::vector<Type *> &types) {
  std::vector<std::string> names;
  for (auto *type : types) {
    names.push_back(type->referenceString());
  }
  return fmt::format(FMT_STRING("Union[{}]"),
                     fmt::join(names.begin(), names.end(), ", "));
}

} // namespace ir
} // namespace codon
