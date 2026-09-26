// Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>

#include "generator.h"

#include <algorithm>

#include "codon/cir/util/cloning.h"
#include "codon/cir/util/inlining.h"
#include "codon/cir/util/irtools.h"
#include "codon/cir/util/matching.h"

namespace codon {
namespace ir {
namespace transform {
namespace pythonic {
namespace {
bool isSum(Func *f) {
  return f &&
         f->getName().rfind(ast::getMangledFunc("std.internal.builtin", "sum"), 0) == 0;
}

bool isAny(Func *f) {
  return f &&
         f->getName().rfind(ast::getMangledFunc("std.internal.builtin", "any"), 0) == 0;
}

bool isAll(Func *f) {
  return f &&
         f->getName().rfind(ast::getMangledFunc("std.internal.builtin", "all"), 0) == 0;
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

Func *genToSum(BodiedFunc *gen, Type *startType, Type *outType) {
  if (!gen || !gen->isGenerator())
    return nullptr;

  auto *M = gen->getModule();
  auto *genType = cast<FuncType>(gen->getType());
  if (!genType)
    return nullptr;

  auto *fn = M->Nr<BodiedFunc>("__sum_wrapper");
  std::vector<Type *> argTypes(genType->begin(), genType->end());
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

  if (!init || !init->getType()->is(outType)) {
    M->remove(fn);
    return nullptr;
  }

  auto *accumulator = util::makeVar(init, body, fn, /*prepend=*/true);
  GeneratorSumTransformer xgen(accumulator);
  fn->accept(xgen);
  body->push_back(M->Nr<ReturnInstr>(M->Nr<VarValue>(accumulator)));

  if (!xgen.valid) {
    M->remove(fn);
    return nullptr;
  }

  return fn;
}

Func *genToAnyAll(BodiedFunc *gen, bool any) {
  if (!gen || !gen->isGenerator())
    return nullptr;

  auto *M = gen->getModule();
  auto *fn = M->Nr<BodiedFunc>(any ? "__any_wrapper" : "__all_wrapper");
  auto *genType = cast<FuncType>(gen->getType());

  std::vector<Type *> argTypes(genType->begin(), genType->end());
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

  if (!xgen.valid) {
    M->remove(fn);
    return nullptr;
  }

  return fn;
}
} // namespace

namespace {
struct FusionVerifier : public util::Operator {
  int nodes = 0;
  int yields = 0;
  bool valid = true;

  void preHook(Node *) override { valid &= ++nodes <= 256; }
  void handle(YieldInstr *value) override {
    valid &= value->getValue() && !value->isFinal();
    ++yields;
  }
  void handle(YieldInInstr *) override { valid = false; }
  void handle(AwaitInstr *) override { valid = false; }
  void handle(TryCatchFlow *) override { valid = false; }
  void handle(PointerValue *value) override { valid &= value->getVar()->isGlobal(); }
  void handle(StackAllocInstr *) override { valid = false; }
  void handle(ForFlow *loop) override {
    valid &= !loop->isParallel() && !loop->isAsync();
  }
};

struct ConsumerVerifier : public util::Operator {
  const std::unordered_set<id_t> &wrappers;
  bool valid = true;
  explicit ConsumerVerifier(const std::unordered_set<id_t> &wrappers)
      : wrappers(wrappers) {}
  void handle(BreakInstr *value) override {
    valid &= value->getLoop() && wrappers.count(value->getLoop()->getId());
  }
  void handle(ContinueInstr *) override { valid = false; }
};

struct IteratorUseVerifier : public util::Operator {
  Var *iterator;
  ForFlow *consumer;
  const std::unordered_set<id_t> &wrappers;
  AssignInstr *assignment = nullptr;
  int reads = 0;
  int writes = 0;
  int position = 0;
  int creationPosition = 0;
  int consumptionPosition = 0;
  bool addressTaken = false;
  std::vector<id_t> creationLoops;
  std::vector<id_t> consumptionLoops;
  IteratorUseVerifier(Var *iterator, ForFlow *consumer,
                      const std::unordered_set<id_t> &wrappers)
      : iterator(iterator), consumer(consumer), wrappers(wrappers) {}
  std::vector<id_t> enclosingLoops() {
    std::vector<id_t> result;
    for (auto position = parent_begin(); position != parent_end(); ++position) {
      auto *node = cast<Flow>(*position);
      if (node && node != consumer && !wrappers.count(node->getId()) &&
          (isA<ForFlow>(node) || isA<WhileFlow>(node) || isA<ImperativeForFlow>(node) ||
           isA<IfFlow>(node) || isA<TryCatchFlow>(node))) {
        result.push_back(node->getId());
        if ((isA<IfFlow>(node) || isA<TryCatchFlow>(node)) &&
            position + 1 != parent_end())
          if (auto *branch = cast<Value>(*(position + 1)))
            result.push_back(branch->getId());
      }
    }
    return result;
  }
  void preHook(Node *node) override {
    ++position;
    auto *value = cast<Value>(node);
    if (!value || isA<VarValue>(value) || isA<PointerValue>(value) ||
        isA<AssignInstr>(value))
      return;
    for (auto *variable : value->getUsedVariables())
      addressTaken |= variable->getId() == iterator->getId();
  }
  void handle(VarValue *value) override {
    if (value->getVar()->getId() == iterator->getId()) {
      ++reads;
      consumptionPosition = position;
      consumptionLoops = enclosingLoops();
    }
  }
  void handle(PointerValue *value) override {
    addressTaken |= value->getVar()->getId() == iterator->getId();
  }
  void handle(AssignInstr *value) override {
    if (value->getLhs()->getId() == iterator->getId()) {
      assignment = value;
      ++writes;
      creationPosition = position;
      creationLoops = enclosingLoops();
    }
  }
  bool valid() const {
    return reads == 1 && writes == 1 && !addressTaken &&
           creationPosition < consumptionPosition && creationLoops == consumptionLoops;
  }
};

struct FusionTransformer : public util::Operator {
  ForFlow *consumer;
  WhileFlow *exit;
  FusionTransformer(ForFlow *consumer, WhileFlow *exit)
      : util::Operator(true), consumer(consumer), exit(exit) {}

  void handle(YieldInstr *value) override {
    auto *module = value->getModule();
    value->replaceAll(
        util::series(module->Nr<AssignInstr>(consumer->getVar(), value->getValue()),
                     consumer->getBody()));
  }
  void handle(ReturnInstr *value) override {
    auto *module = value->getModule();
    auto *replacement = module->Nr<SeriesFlow>();
    if (value->getValue())
      replacement->push_back(value->getValue());
    replacement->push_back(module->Nr<BreakInstr>(exit));
    value->replaceAll(replacement);
  }
};
} // namespace

const std::string GeneratorLoopFusion::KEY = "core-pythonic-generator-loop-fusion";

void GeneratorLoopFusion::handle(CallInstr *call) {
  auto *parent = cast<BodiedFunc>(getParentFunc());
  auto *function = cast<BodiedFunc>(util::getFunc(call->getCallee()));
  if (!parent || !function || function->getName() != "__sum_wrapper")
    return;
  FusionVerifier verifier;
  verifier.process(function->getBody());
  if (!verifier.valid || growth[parent->getId()] + verifier.nodes > 1024)
    return;
  auto inlined = util::inlineCall(call, /*aggressive=*/true);
  if (!inlined)
    return;
  for (auto *variable : inlined.newVars)
    parent->push_back(variable);
  if (auto *expression = cast<FlowInstr>(inlined.result))
    if (auto *body = cast<SeriesFlow>(expression->getFlow()))
      if (auto *wrapper = cast<WhileFlow>(body->back()))
        wrappers.insert(wrapper->getId());
  growth[parent->getId()] += verifier.nodes;
  call->replaceAll(inlined.result);
}

void GeneratorLoopFusion::handle(ForFlow *loop) {
  if (loop->isParallel() || loop->isAsync())
    return;
  auto *parent = cast<BodiedFunc>(getParentFunc());
  if (!parent)
    return;
  if (auto *extract = cast<ExtractInstr>(loop->getIter())) {
    auto *variable = util::getVar(extract->getVal());
    if (!variable || variable->isGlobal())
      return;
    IteratorUseVerifier uses(variable, loop, wrappers);
    uses.process(parent->getBody());
    auto *tuple = uses.valid() ? cast<CallInstr>(uses.assignment->getRhs()) : nullptr;
    auto *constructor = tuple ? util::getFunc(tuple->getCallee()) : nullptr;
    auto *type = constructor ? cast<RecordType>(constructor->getParentType()) : nullptr;
    if (!type || type->getName() != "Tuple" ||
        constructor->getUnmangledName() != Module::NEW_MAGIC_NAME)
      return;
    auto index = cast<RecordType>(extract->getVal()->getType())
                     ->getMemberIndex(extract->getField());
    if (index < 0 || index >= tuple->numArgs())
      return;
    auto *setup = loop->getModule()->Nr<SeriesFlow>();
    Var *selected = nullptr;
    int position = 0;
    for (auto *value : *tuple) {
      auto *variable = util::makeVar(value, setup, parent);
      if (position++ == index)
        selected = variable;
    }
    uses.assignment->replaceAll(setup);
    loop->setIter(loop->getModule()->Nr<VarValue>(selected));
    handle(loop);
    return;
  }
  auto *call = cast<CallInstr>(loop->getIter());
  AssignInstr *creation = nullptr;
  if (auto *iterator = util::getVar(loop->getIter())) {
    if (iterator->isGlobal())
      return;
    IteratorUseVerifier uses(iterator, loop, wrappers);
    uses.process(parent->getBody());
    if (uses.valid()) {
      creation = uses.assignment;
      call = cast<CallInstr>(creation->getRhs());
    }
  }
  auto *generator = call ? cast<BodiedFunc>(util::getFunc(call->getCallee())) : nullptr;
  if (!generator || !generator->isGenerator() || !generator->getBody() ||
      generator->isAsync() || parent == generator ||
      call->numArgs() != std::distance(generator->arg_begin(), generator->arg_end()) ||
      util::hasAttribute(generator,
                         ast::getMangledFunc("std.internal.attributes", "noinline")))
    return;
  FusionVerifier verifier;
  verifier.process(generator->getBody());
  ConsumerVerifier consumerVerifier(wrappers);
  consumerVerifier.process(loop->getBody());
  if (!verifier.valid || verifier.yields != 1 || !consumerVerifier.valid ||
      growth[parent->getId()] + verifier.nodes > 1024)
    return;
  growth[parent->getId()] += verifier.nodes;

  auto *module = loop->getModule();
  auto *setup = module->Nr<SeriesFlow>();
  std::unordered_map<id_t, Var *> arguments;
  auto argument = generator->arg_begin();
  for (auto *value : *call)
    arguments.emplace((*argument++)->getId(), util::makeVar(value, setup, parent));
  if (creation) {
    creation->replaceAll(setup);
    setup = module->Nr<SeriesFlow>();
  }
  util::CloneVisitor clone(module);
  auto *body = cast<Flow>(clone.clone(generator->getBody(), parent, arguments));
  auto *wrapper = module->Nr<SeriesFlow>();
  auto *exit = module->Nr<WhileFlow>(module->getBool(true), wrapper);
  wrappers.insert(exit->getId());
  FusionTransformer transformer(loop, exit);
  transformer.process(body);
  wrapper->push_back(body);
  wrapper->push_back(module->Nr<BreakInstr>(exit));
  setup->push_back(exit);
  loop->replaceAll(setup);
}

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
