// Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>

#include "enumerate.h"

#include "codon/cir/util/cloning.h"
#include "codon/cir/util/irtools.h"

namespace codon {
namespace ir {
namespace transform {
namespace pythonic {
namespace {
// The yielded tuple can disappear only if its sole uses are the loop target
// definition and the two matched unpacking reads.
struct TupleUseChecker : public util::Operator {
  ForFlow *loop;
  Value *index;
  Value *element;
  bool valid = true;

  TupleUseChecker(ForFlow *loop, Value *index, Value *element)
      : loop(loop), index(index), element(element) {}

  void preHook(Node *node) override {
    auto *value = cast<Value>(node);
    if (!value || value == loop || value == index || value == element)
      return;
    for (auto *var : value->getUsedVariables()) {
      if (var->getId() == loop->getVar()->getId())
        valid = false;
    }
  }
};

AssignInstr *getUnpack(Value *value, Var *tuple, const std::string &field) {
  auto *assign = cast<AssignInstr>(value);
  auto *extract = assign ? cast<ExtractInstr>(assign->getRhs()) : nullptr;
  auto *var = extract ? util::getVar(extract->getVal()) : nullptr;
  return var && var->getId() == tuple->getId() && extract->getField() == field
             ? assign
             : nullptr;
}
} // namespace

const std::string EnumerateOptimization::KEY = "core-pythonic-enumerate-opt";

void EnumerateOptimization::handle(ForFlow *loop) {
  // A shared incrementing counter assumes serial, synchronous iteration.
  if (loop->isParallel() || loop->isAsync())
    return;

  // The local use check cannot see other functions observing a global target.
  if (loop->getVar()->isGlobal())
    return;

  // Require a direct builtin call: stored iterators may have other consumers,
  // and a user-defined enumerate need not have the builtin's semantics.
  auto *call = cast<CallInstr>(loop->getIter());
  auto *func = call ? util::getFunc(call->getCallee()) : nullptr;
  if (!call || call->numArgs() != 2 || !func ||
      func->getName().rfind(ast::getMangledFunc("std.internal.builtin", "enumerate"),
                            0) != 0)
    return;

  // A loop target such as "index, value" becomes a temporary tuple variable
  // followed by two leading assignments: index = tuple.item1, value = tuple.item2.
  // Match these explicitly so we can replace the reads without constructing tuples.
  auto *parent = cast<BodiedFunc>(getParentFunc());
  auto *body = cast<SeriesFlow>(loop->getBody());
  if (!parent || !body || body->begin() == body->end())
    return;
  auto position = body->begin();
  auto *indexAssign = getUnpack(*position++, loop->getVar(), "item1");
  if (!indexAssign || position == body->end())
    return;
  auto *elementAssign = getUnpack(*position++, loop->getVar(), "item2");
  if (!elementAssign)
    return;

  // Check the whole function, not just the loop body: the tuple could also be
  // read after the loop, in which case changing the loop variable would be unsafe.
  TupleUseChecker uses(loop, cast<ExtractInstr>(indexAssign->getRhs())->getVal(),
                       cast<ExtractInstr>(elementAssign->getRhs())->getVal());
  uses.process(parent->getBody());
  if (!uses.valid)
    return;

  auto *module = loop->getModule();
  auto *iterable = call->front();
  auto *start = call->back();
  if (!start->getType()->is(module->getIntType()))
    return;
  bool array = iterable->getType()->getName().rfind(
                   ast::getMangledClass("std.numpy.ndarray", "ndarray") + "[", 0) == 0;
  Func *lenFunc = nullptr;
  BodiedFunc *iterFunc = nullptr;
  Value *iterExpr = nullptr;
  // ndarrays support direct axis-zero indexing. Other iterables still need
  // __iter__, unless the argument is already a generator.
  if (array) {
    lenFunc = module->getOrRealizeMethod(iterable->getType(), Module::LEN_MAGIC_NAME,
                                         {iterable->getType()});
    if (!lenFunc)
      return;
  } else if (!isA<GeneratorType>(iterable->getType())) {
    // Realize iter(x) through the frontend rather than calling the statically
    // resolved __iter__ method: its return expression preserves virtual dispatch
    // and default arguments. Decline if the helper has a more complicated body.
    iterFunc = cast<BodiedFunc>(module->getOrRealizeFunc("iter", {iterable->getType()},
                                                         {}, "std.internal.builtin"));
    if (!iterFunc)
      return;
    auto *iterBody = cast<SeriesFlow>(iterFunc->getBody());
    if (!iterBody || iterBody->begin() == iterBody->end())
      return;
    auto statement = iterBody->begin();
    auto *ret = cast<ReturnInstr>(*statement++);
    if (statement != iterBody->end() || !ret || !ret->getValue() ||
        !isA<GeneratorType>(ret->getValue()->getType()))
      return;
    iterExpr = ret->getValue();
  }

  // Evaluate source and start once, in that order, before iteration begins.
  // Keep the counter private because the user's body may reassign its index target.
  auto *setup = module->Nr<SeriesFlow>();
  auto *source = util::makeVar(iterable, setup, parent);
  auto *counter = util::makeVar(start, setup, parent);
  auto *element = module->Nr<Var>(elementAssign->getLhs()->getType());
  parent->push_back(element);
  indexAssign->setRhs(module->Nr<VarValue>(counter));
  elementAssign->setRhs(module->Nr<VarValue>(element));
  // Expose the current counter, then advance it before the user's body so that
  // continue cannot skip the increment.
  body->insert(position,
               module->Nr<AssignInstr>(counter, *module->Nr<VarValue>(counter) +
                                                    *module->getInt(1)));

  if (array) {
    // Use a zero-based offset independent of enumerate's start. __getitem__
    // preserves ndarray strides and yields row views for higher-dimensional arrays.
    auto *offset = module->Nr<Var>(module->getIntType());
    parent->push_back(offset);
    auto *length = util::call(lenFunc, {module->Nr<VarValue>(source)});
    auto *end = module->Nr<FlowInstr>(setup, length);
    auto *load = (*module->Nr<VarValue>(source))[*module->Nr<VarValue>(offset)];
    // Fetch before assigning either visible target, just as a generator would:
    // an indexing exception must leave their previous values intact.
    body->insert(body->begin(), module->Nr<AssignInstr>(element, load));
    loop->replaceAll(module->N<ImperativeForFlow>(loop->getSrcInfo(), module->getInt(0),
                                                  1, end, body, offset));
  } else {
    Value *iter = module->Nr<VarValue>(source);
    if (iterExpr) {
      util::CloneVisitor clone(module);
      iter =
          clone.clone(iterExpr, parent, {{(*iterFunc->arg_begin())->getId(), source}});
    }
    // Preserve direct __iter__(source) calls so list lowering can recognize them.
    // For virtual calls, setup must precede the entire expression, including the
    // vtable lookup, rather than only evaluation of the receiver argument.
    auto *iterCall = cast<CallInstr>(iter);
    if (util::isCallOf(iter, Module::ITER_MAGIC_NAME, 1) &&
        util::getVar(iterCall->front()) == source) {
      iterCall->front()->replaceAll(
          module->Nr<FlowInstr>(setup, module->Nr<VarValue>(source)));
    } else {
      iter = module->Nr<FlowInstr>(setup, iter);
    }
    loop->setIter(iter);
    loop->setVar(element);
  }
}

} // namespace pythonic
} // namespace transform
} // namespace ir
} // namespace codon
