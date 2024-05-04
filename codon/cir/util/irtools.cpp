// Copyright (C) 2022-2024 Exaloop Inc. <https://exaloop.io>

#include "irtools.h"

#include <iterator>

namespace codon {
namespace ir {
namespace util {

bool hasAttribute(const Func *func, const std::string &attribute) {
  if (auto *attr = func->getAttribute<KeyValueAttribute>()) {
    return attr->has(attribute);
  }
  return false;
}

bool isStdlibFunc(const Func *func, const std::string &submodule) {
  if (auto *attr = func->getAttribute<KeyValueAttribute>()) {
    std::string module = attr->get(".module");
    return module.rfind("std::" + submodule, 0) == 0;
  }
  return false;
}

CallInstr *call(Func *func, const std::vector<Value *> &args) {
  auto *M = func->getModule();
  return M->Nr<CallInstr>(M->Nr<VarValue>(func), args);
}

bool isCallOf(const Value *value, const std::string &name,
              const std::vector<types::Type *> &inputs, types::Type *output,
              bool method) {
  if (auto *call = cast<CallInstr>(value)) {
    auto *fn = getFunc(call->getCallee());
    if (!fn || fn->getUnmangledName() != name || call->numArgs() != inputs.size())
      return false;

    unsigned i = 0;
    for (auto *arg : *call) {
      if (!arg->getType()->is(inputs[i++]))
        return false;
    }

    if (output && !value->getType()->is(output))
      return false;

    if (method &&
        (inputs.empty() || !fn->getParentType() || !fn->getParentType()->is(inputs[0])))
      return false;

    return true;
  }

  return false;
}

bool isCallOf(const Value *value, const std::string &name, int numArgs,
              types::Type *output, bool method) {
  if (auto *call = cast<CallInstr>(value)) {
    auto *fn = getFunc(call->getCallee());
    if (!fn || fn->getUnmangledName() != name ||
        (numArgs >= 0 && call->numArgs() != numArgs))
      return false;

    if (output && !value->getType()->is(output))
      return false;

    if (method && (!fn->getParentType() || call->numArgs() == 0 ||
                   !call->front()->getType()->is(fn->getParentType())))
      return false;

    return true;
  }

  return false;
}

bool isMagicMethodCall(const Value *value) {
  if (auto *call = cast<CallInstr>(value)) {
    auto *fn = getFunc(call->getCallee());
    if (!fn || !fn->getParentType() || call->numArgs() == 0 ||
        !call->front()->getType()->is(fn->getParentType()))
      return false;

    auto name = fn->getUnmangledName();
    auto size = name.size();
    if (size < 5 || !(name[0] == '_' && name[1] == '_' && name[size - 1] == '_' &&
                      name[size - 2] == '_'))
      return false;

    return true;
  }

  return false;
}

Value *makeTuple(const std::vector<Value *> &args, Module *M) {
  if (!M) {
    seqassertn(!args.empty(), "unknown module for empty tuple construction");
    M = args[0]->getModule();
  }

  std::vector<types::Type *> types;
  for (auto *arg : args) {
    types.push_back(arg->getType());
  }
  auto *tupleType = M->getTupleType(types);
  auto *newFunc = M->getOrRealizeMethod(tupleType, "__new__", types);
  seqassertn(newFunc, "could not realize {} new function", *tupleType);
  return M->Nr<CallInstr>(M->Nr<VarValue>(newFunc), args);
}

VarValue *makeVar(Value *x, SeriesFlow *flow, BodiedFunc *parent, bool prepend) {
  const bool global = (parent == nullptr);
  auto *M = x->getModule();
  auto *v = M->Nr<Var>(x->getType(), global);
  if (global) {
    static int counter = 1;
    v->setName(".anon_global." + std::to_string(counter++));
  }
  auto *a = M->Nr<AssignInstr>(v, x);
  if (prepend) {
    flow->insert(flow->begin(), a);
  } else {
    flow->push_back(a);
  }
  if (!global) {
    parent->push_back(v);
  }
  return M->Nr<VarValue>(v);
}

Value *alloc(types::Type *type, Value *count) {
  auto *M = type->getModule();
  auto *ptrType = M->getPointerType(type);
  return (*ptrType)(*count);
}

Value *alloc(types::Type *type, int64_t count) {
  auto *M = type->getModule();
  return alloc(type, M->getInt(count));
}

Var *getVar(Value *x) {
  if (auto *v = cast<VarValue>(x)) {
    if (auto *var = cast<Var>(v->getVar())) {
      if (!isA<Func>(var)) {
        return var;
      }
    }
  }
  return nullptr;
}

const Var *getVar(const Value *x) {
  if (auto *v = cast<VarValue>(x)) {
    if (auto *var = cast<Var>(v->getVar())) {
      if (!isA<Func>(var)) {
        return var;
      }
    }
  }
  return nullptr;
}

Func *getFunc(Value *x) {
  if (auto *v = cast<VarValue>(x)) {
    if (auto *func = cast<Func>(v->getVar())) {
      return func;
    }
  }
  return nullptr;
}

const Func *getFunc(const Value *x) {
  if (auto *v = cast<VarValue>(x)) {
    if (auto *func = cast<Func>(v->getVar())) {
      return func;
    }
  }
  return nullptr;
}

Value *ptrLoad(Value *ptr) {
  auto *M = ptr->getModule();
  auto *deref = (*ptr)[*M->getInt(0)];
  seqassertn(deref, "pointer getitem not found [{}]", ptr->getSrcInfo());
  return deref;
}

Value *ptrStore(Value *ptr, Value *val) {
  auto *M = ptr->getModule();
  auto *setitem =
      M->getOrRealizeMethod(ptr->getType(), Module::SETITEM_MAGIC_NAME,
                            {ptr->getType(), M->getIntType(), val->getType()});
  seqassertn(setitem, "pointer setitem not found [{}]", ptr->getSrcInfo());
  return call(setitem, {ptr, M->getInt(0), val});
}

Value *tupleGet(Value *tuple, unsigned index) {
  auto *M = tuple->getModule();
  return M->Nr<ExtractInstr>(tuple, "item" + std::to_string(index + 1));
}

Value *tupleStore(Value *tuple, unsigned index, Value *val) {
  auto *M = tuple->getModule();
  auto *type = cast<types::RecordType>(tuple->getType());
  seqassertn(type, "argument is not a tuple [{}]", tuple->getSrcInfo());
  std::vector<Value *> newElements;
  for (unsigned i = 0; i < std::distance(type->begin(), type->end()); i++) {
    newElements.push_back(i == index ? val : tupleGet(tuple, i));
  }
  return makeTuple(newElements, M);
}

BodiedFunc *getStdlibFunc(Value *x, const std::string &name,
                          const std::string &submodule) {
  if (auto *f = getFunc(x)) {
    if (auto *g = cast<BodiedFunc>(f)) {
      if (isStdlibFunc(g, submodule) && g->getUnmangledName() == name) {
        return g;
      }
    }
  }
  return nullptr;
}

const BodiedFunc *getStdlibFunc(const Value *x, const std::string &name,
                                const std::string &submodule) {
  if (auto *f = getFunc(x)) {
    if (auto *g = cast<BodiedFunc>(f)) {
      if (isStdlibFunc(g, submodule) && g->getUnmangledName() == name) {
        return g;
      }
    }
  }
  return nullptr;
}

types::Type *getReturnType(const Func *func) {
  return cast<types::FuncType>(func->getType())->getReturnType();
}

void setReturnType(Func *func, types::Type *rType) {
  auto *M = func->getModule();
  auto *t = cast<types::FuncType>(func->getType());
  seqassertn(t, "{} is not a function type [{}]", *func->getType(), func->getSrcInfo());
  std::vector<types::Type *> argTypes(t->begin(), t->end());
  func->setType(M->getFuncType(rType, argTypes));
}

} // namespace util
} // namespace ir
} // namespace codon
