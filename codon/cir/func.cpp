// Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>

#include "func.h"

#include <algorithm>

#include "codon/cir/module.h"
#include "codon/cir/util/iterators.h"
#include "codon/cir/util/operator.h"
#include "codon/cir/util/visitor.h"
#include "codon/cir/var.h"
#include "codon/parser/common.h"

namespace codon {
namespace ir {
namespace {
int findAndReplace(id_t id, codon::ir::Var *newVal,
                   std::list<codon::ir::Var *> &values) {
  auto replacements = 0;
  for (auto &value : values) {
    if (value->getId() == id) {
      value = newVal;
      ++replacements;
    }
  }
  return replacements;
}
} // namespace

const char Func::NodeId = 0;

void Func::realize(Type *newType, const std::vector<std::string> &names) {
  auto *funcType = cast<FuncType>(newType);
  seqassert(funcType, "{} is not a function type", *newType);

  setType(funcType);
  args.clear();

  auto i = 0;
  for (auto *t : *funcType) {
    args.push_back(getModule()->Nr<Var>(t, false, false, false, names[i]));
    ++i;
  }
}

Var *Func::getArgVar(const std::string &n) {
  auto it = std::find_if(args.begin(), args.end(),
                         [n](auto *other) { return other->getName() == n; });
  return (it != args.end()) ? *it : nullptr;
}

std::vector<Var *> Func::doGetUsedVariables() const {
  std::vector<Var *> ret(args.begin(), args.end());
  return ret;
}

int Func::doReplaceUsedVariable(id_t id, Var *newVar) {
  return findAndReplace(id, newVar, args);
}

std::vector<Type *> Func::doGetUsedTypes() const {
  std::vector<Type *> ret;

  for (auto *t : Var::doGetUsedTypes())
    ret.push_back(const_cast<Type *>(t));

  if (parentType)
    ret.push_back(parentType);

  return ret;
}

int Func::doReplaceUsedType(const std::string &name, Type *newType) {
  auto count = Var::doReplaceUsedType(name, newType);
  if (parentType && parentType->getName() == name) {
    parentType = newType;
    ++count;
  }
  return count;
}

const char BodiedFunc::NodeId = 0;

int BodiedFunc::doReplaceUsedValue(id_t id, Value *newValue) {
  if (body && body->getId() == id) {
    auto *flow = cast<Flow>(newValue);
    seqassert(flow, "{} is not a flow", *newValue);
    body = flow;
    return 1;
  }
  return 0;
}

std::vector<Var *> BodiedFunc::doGetUsedVariables() const {
  auto ret = Func::doGetUsedVariables();
  ret.insert(ret.end(), symbols.begin(), symbols.end());
  return ret;
}

int BodiedFunc::doReplaceUsedVariable(id_t id, Var *newVar) {
  return Func::doReplaceUsedVariable(id, newVar) + findAndReplace(id, newVar, symbols);
}

const char ExternalFunc::NodeId = 0;

const char InternalFunc::NodeId = 0;

const char *InternalFunc::getIntrinsicName(Intrinsic kind) {
  switch (kind) {
  case Intrinsic::NONE:
    return "none";
  case Intrinsic::ALLOCATE:
    return "allocate";
  case Intrinsic::ALLOCATION_SIZE:
    return "allocation_size";
  case Intrinsic::TYPEINFO:
    return "typeinfo";
  case Intrinsic::TYPEINFO_LOOKUP:
    return "typeinfo_lookup";
  case Intrinsic::VIRTUAL_LOOKUP:
    return "virtual_lookup";
  case Intrinsic::RECORD_NEW:
    return "record_new";
  case Intrinsic::GENERATOR_PROMISE:
    return "generator_promise";
  case Intrinsic::SIZEOF:
    return "sizeof";
  case Intrinsic::IS_ATOMIC:
    return "is_atomic";
  case Intrinsic::IS_CONTENT_ATOMIC:
    return "is_content_atomic";
  }
  return "invalid";
}

bool InternalFunc::isTypeQuery(Intrinsic kind) {
  return kind == Intrinsic::SIZEOF || kind == Intrinsic::IS_ATOMIC ||
         kind == Intrinsic::IS_CONTENT_ATOMIC;
}

bool InternalFunc::hasValidIntrinsicSignature() const {
  auto *signature = cast<FuncType>(getType());
  if (!signature || signature->isVariadic() || isGenerator() || isAsync())
    return false;
  auto *result = signature->getReturnType();
  std::vector<Type *> arguments(signature->begin(), signature->end());
  auto *module = getModule();
  switch (intrinsic) {
  case Intrinsic::NONE:
    return false;
  case Intrinsic::SIZEOF:
    return intrinsicType && arguments.empty() && result == module->getIntType();
  case Intrinsic::IS_ATOMIC:
  case Intrinsic::IS_CONTENT_ATOMIC:
    return intrinsicType && arguments.empty() && result == module->getBoolType();
  case Intrinsic::ALLOCATE:
    return isA<RefType>(intrinsicType) && arguments.empty() && result == intrinsicType;
  case Intrinsic::ALLOCATION_SIZE:
    return isA<RefType>(intrinsicType) && arguments.empty() &&
           result == module->getIntType();
  case Intrinsic::TYPEINFO:
    return intrinsicType && arguments.empty() &&
           (isA<RefType>(result) || isA<PointerType>(result));
  case Intrinsic::TYPEINFO_LOOKUP:
    return !intrinsicType && arguments.size() == 1 &&
           arguments[0] == module->getIntType() &&
           (isA<RefType>(result) || isA<PointerType>(result));
  case Intrinsic::VIRTUAL_LOOKUP:
    return isA<RefType>(intrinsicType) && arguments.size() == 2 &&
           arguments[0] == intrinsicType && arguments[1] == module->getIntType() &&
           isA<FuncType>(result);
  case Intrinsic::RECORD_NEW: {
    auto *record = cast<RecordType>(intrinsicType);
    return record && result == record &&
           std::equal(arguments.begin(), arguments.end(), record->begin(),
                      record->end(),
                      [](Type *argument, const MemberedType::Field &field) {
                        return argument == field.getType();
                      });
  }
  case Intrinsic::GENERATOR_PROMISE: {
    auto *generator = cast<GeneratorType>(intrinsicType);
    auto *pointer = cast<PointerType>(result);
    return generator && pointer && pointer->getBase() == generator->getBase() &&
           arguments.size() == 1 && arguments[0] == generator;
  }
  }
  return false;
}

util::SideEffectStatus InternalFunc::getIntrinsicSideEffectStatus() const {
  switch (intrinsic) {
  case Intrinsic::SIZEOF:
  case Intrinsic::IS_ATOMIC:
  case Intrinsic::IS_CONTENT_ATOMIC:
  case Intrinsic::ALLOCATION_SIZE:
  case Intrinsic::TYPEINFO:
  case Intrinsic::RECORD_NEW:
  case Intrinsic::GENERATOR_PROMISE:
    return util::PURE;
  case Intrinsic::ALLOCATE:
    // An unused allocation can disappear, but two allocations are not equal.
    return util::NO_SIDE_EFFECT;
  case Intrinsic::TYPEINFO_LOOKUP:
  case Intrinsic::VIRTUAL_LOOKUP:
    // Lookups can trap and observe registry/method-table updates in the JIT.
    return util::UNKNOWN;
  case Intrinsic::NONE:
    return util::UNKNOWN;
  }
  return util::UNKNOWN;
}

bool InternalFunc::intrinsicDerivesFromArguments() const {
  return intrinsic == Intrinsic::RECORD_NEW ||
         intrinsic == Intrinsic::GENERATOR_PROMISE;
}

const char LLVMFunc::NodeId = 0;

std::vector<Type *> LLVMFunc::doGetUsedTypes() const {
  std::vector<Type *> ret;

  for (auto *t : Func::doGetUsedTypes())
    ret.push_back(const_cast<Type *>(t));

  for (auto &l : llvmLiterals)
    if (l.isType())
      ret.push_back(const_cast<Type *>(l.getTypeValue()));

  return ret;
}

int LLVMFunc::doReplaceUsedType(const std::string &name, Type *newType) {
  auto count = Func::doReplaceUsedType(name, newType);
  for (auto &l : llvmLiterals)
    if (l.isType() && l.getTypeValue()->getName() == name) {
      l = newType;
      ++count;
    }
  return count;
}

} // namespace ir
} // namespace codon
