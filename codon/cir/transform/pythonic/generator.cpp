// Copyright (C) 2022-2024 Exaloop Inc. <https://exaloop.io>

#include "generator.h"

#include <algorithm>

#include "codon/cir/util/cloning.h"
#include "codon/cir/util/irtools.h"
#include "codon/cir/util/matching.h"

namespace codon {
namespace ir {
namespace transform {
namespace pythonic {
namespace {
bool isSum(Func *f) {
  return f && f->getName().rfind("std.internal.builtin.sum:", 0) == 0;
}

bool isAny(Func *f) {
  return f && f->getName().rfind("std.internal.builtin.any:", 0) == 0;
}

bool isAll(Func *f) {
  return f && f->getName().rfind("std.internal.builtin.all:", 0) == 0;
}

// Replaces yields with updates to the accumulator variable.
struct GeneratorSumTransformer : public util::Operator {
  Var *accumulator;
  bool valid;

  explicit GeneratorSumTransformer(Var *accumulator)
      : util::Operator(), accumulator(accumulator), valid(true) {}

  void handle(YieldInstr *v) override {
    auto *M = v->getModule();
    auto *val = v->getValue();
    if (!val) {
      valid = false;
      return;
    }

    Value *rhs = val;
    if (val->getType()->is(M->getBoolType())) {
      rhs = M->Nr<TernaryInstr>(rhs, M->getInt(1), M->getInt(0));
    }

    Value *add = *M->Nr<VarValue>(accumulator) + *rhs;
    if (!add || !add->getType()->is(accumulator->getType())) {
      valid = false;
      return;
    }

    auto *assign = M->Nr<AssignInstr>(accumulator, add);
    v->replaceAll(assign);
  }

  void handle(ReturnInstr *v) override {
    auto *M = v->getModule();
    auto *newReturn = M->Nr<ReturnInstr>(M->Nr<VarValue>(accumulator));
    see(newReturn);
    if (v->getValue()) {
      v->replaceAll(util::series(v->getValue(), newReturn));
    } else {
      v->replaceAll(newReturn);
    }
  }

  void handle(YieldInInstr *v) override { valid = false; }
};

// Replaces yields with conditional returns of the any/all answer.
struct GeneratorAnyAllTransformer : public util::Operator {
  bool any; // true=any, false=all
  bool valid;

  explicit GeneratorAnyAllTransformer(bool any)
      : util::Operator(), any(any), valid(true) {}

  void handle(YieldInstr *v) override {
    auto *M = v->getModule();
    auto *val = v->getValue();
    auto *valBool = val ? (*M->getBoolType())(*val) : nullptr;
    if (!valBool) {
      valid = false;
      return;
    } else if (!any) {
      valBool = M->Nr<TernaryInstr>(valBool, M->getBool(false), M->getBool(true));
    }

    auto *newReturn = M->Nr<ReturnInstr>(M->getBool(any));
    see(newReturn);
    auto *rep = M->Nr<IfFlow>(valBool, util::series(newReturn));
    v->replaceAll(rep);
  }

  void handle(ReturnInstr *v) override {
    if (saw(v))
      return;
    auto *M = v->getModule();
    auto *newReturn = M->Nr<ReturnInstr>(M->getBool(!any));
    see(newReturn);
    if (v->getValue()) {
      v->replaceAll(util::series(v->getValue(), newReturn));
    } else {
      v->replaceAll(newReturn);
    }
  }

  void handle(YieldInInstr *v) override { valid = false; }
};

Func *genToSum(BodiedFunc *gen, types::Type *startType, types::Type *outType) {
  if (!gen || !gen->isGenerator())
    return nullptr;

  auto *M = gen->getModule();
  auto *fn = M->Nr<BodiedFunc>("__sum_wrapper");
  auto *genType = cast<types::FuncType>(gen->getType());
  if (!genType)
    return nullptr;

  std::vector<types::Type *> argTypes(genType->begin(), genType->end());
  argTypes.push_back(startType);

  std::vector<std::string> names;
  for (auto it = gen->arg_begin(); it != gen->arg_end(); ++it) {
    names.push_back((*it)->getName());
  }
  names.push_back("start");

  auto *fnType = M->getFuncType(outType, argTypes);
  fn->realize(fnType, names);

  std::unordered_map<id_t, Var *> argRemap;
  for (auto it1 = gen->arg_begin(), it2 = fn->arg_begin();
       it1 != gen->arg_end() && it2 != fn->arg_end(); ++it1, ++it2) {
    argRemap.emplace((*it1)->getId(), *it2);
  }

  util::CloneVisitor cv(M);
  auto *body = cast<SeriesFlow>(cv.clone(gen->getBody(), fn, argRemap));
  fn->setBody(body);

  Value *init = M->Nr<VarValue>(fn->arg_back());
  if (startType->is(M->getIntType()) && outType->is(M->getFloatType()))
    init = (*M->getFloatType())(*init);

  if (!init || !init->getType()->is(outType))
    return nullptr;

  auto *accumulator = util::makeVar(init, body, fn, /*prepend=*/true)->getVar();
  GeneratorSumTransformer xgen(accumulator);
  fn->accept(xgen);
  body->push_back(M->Nr<ReturnInstr>(M->Nr<VarValue>(accumulator)));

  if (!xgen.valid)
    return nullptr;

  return fn;
}

Func *genToAnyAll(BodiedFunc *gen, bool any) {
  if (!gen || !gen->isGenerator())
    return nullptr;

  auto *M = gen->getModule();
  auto *fn = M->Nr<BodiedFunc>(any ? "__any_wrapper" : "__all_wrapper");
  auto *genType = cast<types::FuncType>(gen->getType());

  std::vector<types::Type *> argTypes(genType->begin(), genType->end());
  std::vector<std::string> names;
  for (auto it = gen->arg_begin(); it != gen->arg_end(); ++it) {
    names.push_back((*it)->getName());
  }

  auto *fnType = M->getFuncType(M->getBoolType(), argTypes);
  fn->realize(fnType, names);

  std::unordered_map<id_t, Var *> argRemap;
  for (auto it1 = gen->arg_begin(), it2 = fn->arg_begin();
       it1 != gen->arg_end() && it2 != fn->arg_end(); ++it1, ++it2) {
    argRemap.emplace((*it1)->getId(), *it2);
  }

  util::CloneVisitor cv(M);
  auto *body = cast<SeriesFlow>(cv.clone(gen->getBody(), fn, argRemap));
  fn->setBody(body);

  GeneratorAnyAllTransformer xgen(any);
  fn->accept(xgen);
  body->push_back(M->Nr<ReturnInstr>(M->getBool(!any)));

  if (!xgen.valid)
    return nullptr;

  return fn;
}
} // namespace

const std::string GeneratorArgumentOptimization::KEY =
    "core-pythonic-generator-argument-opt";

void GeneratorArgumentOptimization::handle(CallInstr *v) {
  auto *M = v->getModule();
  auto *func = util::getFunc(v->getCallee());

  if (isSum(func) && v->numArgs() == 2) {
    auto *call = cast<CallInstr>(v->front());
    if (!call)
      return;

    auto *gen = util::getFunc(call->getCallee());
    auto *start = v->back();

    if (auto *fn = genToSum(cast<BodiedFunc>(gen), start->getType(), v->getType())) {
      std::vector<Value *> args(call->begin(), call->end());
      args.push_back(start);
      v->replaceAll(util::call(fn, args));
    }
  } else {
    bool any = isAny(func), all = isAll(func);
    if (!(any || all) || v->numArgs() != 1 || !v->getType()->is(M->getBoolType()))
      return;

    auto *call = cast<CallInstr>(v->front());
    if (!call)
      return;

    auto *gen = util::getFunc(call->getCallee());

    if (auto *fn = genToAnyAll(cast<BodiedFunc>(gen), any)) {
      std::vector<Value *> args(call->begin(), call->end());
      v->replaceAll(util::call(fn, args));
    }
  }
}

} // namespace pythonic
} // namespace transform
} // namespace ir
} // namespace codon
