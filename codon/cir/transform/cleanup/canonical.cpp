// Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>

#include "canonical.h"

#include <algorithm>
#include <functional>
#include <tuple>
#include <unordered_set>
#include <utility>

#include "codon/cir/analyze/module/side_effect.h"
#include "codon/cir/transform/rewrite.h"
#include "codon/cir/util/irtools.h"
#include "codon/cir/util/matching.h"

namespace codon {
namespace ir {
namespace transform {
namespace cleanup {
namespace {
struct NodeRanker : public util::Operator {
  // Nodes are ranked lexicographically by:
  //   - Whether the node is constant (constants come last)
  //   - Max node depth (deeper nodes first)
  //   - Node hash
  // The hash imposes an arbitrary but well-defined ordering
  // to ensure a single canonical representation for (most)
  // nodes.
  using Rank = std::tuple<int, int, uint64_t>;
  Node *root = nullptr;
  int maxDepth = 0;
  uint64_t hash = 0;

  // boost's hash_combine
  template <class T> void hash_combine(const T &v) {
    std::hash<T> hasher;
    hash ^= hasher(v) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
  }

  void preHook(Node *node) {
    if (!root)
      root = node;
    maxDepth = std::max(maxDepth, depth());
    for (auto *v : node->getUsedVariables()) {
      hash_combine(v->getName());
    }
    for (auto *v : node->getUsedTypes()) {
      hash_combine(v->getName());
    }
  }

  Rank getRank() {
    return std::make_tuple((isA<Const>(root) ? 1 : -1), -maxDepth, hash);
  }
};

NodeRanker::Rank getRank(Node *node) {
  NodeRanker ranker;
  node->accept(ranker);
  return ranker.getRank();
}

bool isCommutativeOp(Func *fn) {
  return fn && util::hasAttribute(
                   fn, ast::getMangledFunc("std.internal.attributes", "commutative"));
}

bool isAssociativeOp(Func *fn) {
  return fn && util::hasAttribute(
                   fn, ast::getMangledFunc("std.internal.attributes", "associative"));
}

bool isDistributiveOp(Func *fn) {
  return fn && util::hasAttribute(
                   fn, ast::getMangledFunc("std.internal.attributes", "distributive"));
}

bool isInequalityOp(Func *fn) {
  static const std::unordered_set<std::string> ops = {
      Module::EQ_MAGIC_NAME, Module::NE_MAGIC_NAME, Module::LT_MAGIC_NAME,
      Module::LE_MAGIC_NAME, Module::GT_MAGIC_NAME, Module::GE_MAGIC_NAME};
  return fn && ops.find(fn->getUnmangledName()) != ops.end();
}

// c + b + a --> a + b + c
struct CanonOpChain : public RewriteRule {
  static void extractAssociativeOpChain(Value *v, const std::string &op, Type *type,
                                        std::vector<Value *> &result) {
    if (util::isCallOf(v, op, {type, type}, type, /*method=*/true)) {
      auto *call = cast<CallInstr>(v);
      extractAssociativeOpChain(call->front(), op, type, result);
      extractAssociativeOpChain(call->back(), op, type, result);
    } else {
      result.push_back(v);
    }
  }

  static void orderOperands(std::vector<Value *> &operands) {
    std::vector<std::pair<NodeRanker::Rank, Value *>> rankedOperands;
    for (auto *v : operands) {
      rankedOperands.push_back({getRank(v), v});
    }
    std::sort(rankedOperands.begin(), rankedOperands.end());

    operands.clear();
    for (auto &p : rankedOperands) {
      operands.push_back(std::get<1>(p));
    }
  }

  void visit(CallInstr *v) override {
    auto *fn = util::getFunc(v->getCallee());
    if (!fn)
      return;

    std::string op = fn->getUnmangledName();
    Type *type = v->getType();
    const bool isAssociative = isAssociativeOp(fn);
    const bool isCommutative = isCommutativeOp(fn);

    if (util::isCallOf(v, op, {type, type}, type, /*method=*/true)) {
      std::vector<Value *> operands;
      if (isAssociative) {
        extractAssociativeOpChain(v, op, type, operands);
      } else {
        operands.push_back(v->front());
        operands.push_back(v->back());
      }
      seqassertn(operands.size() >= 2, "bad call canonicalization");

      if (isCommutative)
        orderOperands(operands);

      Value *newCall = util::call(fn, {operands[0], operands[1]});
      for (auto it = operands.begin() + 2; it != operands.end(); ++it) {
        newCall = util::call(fn, {newCall, *it});
      }

      if (!util::match(v, newCall, /*checkNames=*/false, /*varIdMatch=*/true))
        return setResult(newCall);
    }
  }
};

// b > a --> a < b (etc.)
struct CanonInequality : public RewriteRule {
  void visit(CallInstr *v) override {
    auto *fn = util::getFunc(v->getCallee());
    if (!fn)
      return;

    std::string op = fn->getUnmangledName();
    Type *type = v->getType();

    // canonicalize inequalities
    if (v->numArgs() == 2 && isInequalityOp(fn)) {
      Value *newCall = nullptr;
      auto *lhs = v->front();
      auto *rhs = v->back();
      if (getRank(lhs) > getRank(rhs)) { // are we out of order?
        // re-order
        if (op == Module::EQ_MAGIC_NAME) { // lhs == rhs
          newCall = *rhs == *lhs;
        } else if (op == Module::NE_MAGIC_NAME) { // lhs != rhs
          newCall = *rhs != *lhs;
        } else if (op == Module::LT_MAGIC_NAME) { // lhs < rhs
          newCall = *rhs > *lhs;
        } else if (op == Module::LE_MAGIC_NAME) { // lhs <= rhs
          newCall = *rhs >= *lhs;
        } else if (op == Module::GT_MAGIC_NAME) { // lhs > rhs
          newCall = *rhs < *lhs;
        } else if (op == Module::GE_MAGIC_NAME) { // lhs >= rhs
          newCall = *rhs <= *lhs;
        } else {
          seqassertn(false, "unknown comparison op: {}", op);
        }

        if (newCall && newCall->getType()->is(type) &&
            !util::match(v, newCall, /*checkNames=*/false, /*varIdMatch=*/true))
          return setResult(newCall);
      }
    }
  }
};

// a*x + b*x --> (a + b) * x
struct CanonAddMul : public RewriteRule {
  static bool varMatch(Value *a, Value *b) {
    auto *v1 = cast<VarValue>(a);
    auto *v2 = cast<VarValue>(b);
    return v1 && v2 && v1->getVar()->getId() == v2->getVar()->getId();
  }

  static Func *getOp(Value *v) {
    return isA<CallInstr>(v) ? util::getFunc(cast<CallInstr>(v)->getCallee()) : nullptr;
  }

  // (a + b) * x, or null if invalid
  static Value *addMul(Value *a, Value *b, Value *x) {
    if (!a || !b || !x)
      return nullptr;

    auto *y = (*a + *b);
    if (!y) {
      y = (*b + *a);
      if (y && !isCommutativeOp(getOp(y)))
        return nullptr;
    }
    if (!y)
      return nullptr;

    auto *z = (*y) * (*x);
    if (!z) {
      z = (*x) * (*y);
      if (z && !isCommutativeOp(getOp(z)))
        return nullptr;
    }
    if (!z)
      return nullptr;

    return z;
  }

  void visit(CallInstr *v) override {
    auto *M = v->getModule();
    auto *fn = util::getFunc(v->getCallee());
    if (!isCommutativeOp(fn) ||
        !util::isCallOf(v, Module::ADD_MAGIC_NAME, 2, /*output=*/nullptr,
                        /*method=*/true))
      return;

    // decompose the operation
    Value *lhs = v->front();
    Value *rhs = v->back();
    Value *lhs1 = nullptr, *lhs2 = nullptr, *rhs1 = nullptr, *rhs2 = nullptr;

    if (util::isCallOf(lhs, Module::MUL_MAGIC_NAME, 2, /*output=*/nullptr,
                       /*method=*/true)) {
      auto *lhsCall = cast<CallInstr>(lhs);
      lhs1 = lhsCall->front();
      lhs2 = lhsCall->back();
    } else {
      lhs1 = lhs;
      lhs2 = M->getInt(1);
    }

    if (util::isCallOf(rhs, Module::MUL_MAGIC_NAME, 2, /*output=*/nullptr,
                       /*method=*/true)) {
      auto *rhsCall = cast<CallInstr>(rhs);
      rhs1 = rhsCall->front();
      rhs2 = rhsCall->back();
    } else {
      rhs1 = rhs;
      rhs2 = M->getInt(1);
    }

    Value *newCall = nullptr;
    if (varMatch(lhs1, rhs1)) {
      newCall = addMul(lhs2, rhs2, lhs1);
    } else if (varMatch(lhs1, rhs2)) {
      newCall = addMul(lhs2, rhs1, lhs1);
    } else if (varMatch(lhs2, rhs1)) {
      newCall = addMul(lhs1, rhs2, lhs2);
    } else if (varMatch(lhs2, rhs2)) {
      newCall = addMul(lhs1, rhs1, lhs2);
    }

    if (newCall && isDistributiveOp(getOp(newCall)) &&
        newCall->getType()->is(v->getType()) &&
        !util::match(v, newCall, /*checkNames=*/false, /*varIdMatch=*/true))
      return setResult(newCall);
  }
};

// x - c --> x + (-c)
struct CanonConstSub : public RewriteRule {
  void visit(CallInstr *v) override {
    auto *M = v->getModule();
    auto *type = v->getType();

    if (!util::isCallOf(v, Module::SUB_MAGIC_NAME, 2, /*output=*/nullptr,
                        /*method=*/true))
      return;

    Value *lhs = v->front();
    Value *rhs = v->back();

    if (!lhs->getType()->is(rhs->getType()))
      return;

    Value *newCall = nullptr;
    if (util::isConst<int64_t>(rhs)) {
      auto c = util::getConst<int64_t>(rhs);
      if (c != -(1ull << 63)) // ensure no overflow
        newCall = *lhs + *(M->getInt(-c));
    } else if (util::isConst<double>(rhs)) {
      auto c = util::getConst<double>(rhs);
      newCall = *lhs + *(M->getFloat(-c));
    }

    if (newCall && newCall->getType()->is(type) &&
        !util::match(v, newCall, /*checkNames=*/false, /*varIdMatch=*/true))
      return setResult(newCall);
  }
};
} // namespace

const std::string CanonicalizationPass::KEY = "core-cleanup-canon";

void CanonicalizationPass::run(Module *m) {
  registerStandardRules(m);
  Rewriter::reset();
  flowReplacements = 0;
  OperatorPass::run(m);
}

void CanonicalizationPass::handle(CallInstr *v) {
  // Lift sequenced operands out of calls to expose their values to folding.
  // Writing seq(prefix, value) for FlowInstr:
  //   func(value, seq(effect(), 0), later)
  //     -> seq(saved = value; effect(), func(saved, 0, later)).
  // The callee is evaluated first. Only operands through the last sequence
  // need lifting; later operands can still be evaluated inside the call.
  size_t prefixEnd = isA<FlowInstr>(v->getCallee()) ? 1 : 0;
  size_t position = 1;
  for (auto *argument : *v) {
    if (isA<FlowInstr>(argument))
      prefixEnd = position + 1;
    ++position;
  }

  if (prefixEnd) {
    auto *parent = cast<BodiedFunc>(getParentFunc());
    if (!parent)
      return;
    std::vector<Value *> operands{v->getCallee()};
    operands.insert(operands.end(), v->begin(), v->end());
    auto *module = v->getModule();
    auto *flow = module->N<SeriesFlow>(v->getSrcInfo());
    for (size_t index = 0; index < prefixEnd; ++index) {
      auto *operand = operands[index];
      while (auto *sequence = cast<FlowInstr>(operand)) {
        flow->push_back(sequence->getFlow());
        operand = sequence->getValue();
      }
      // Capture even variable reads before later prefixes can mutate them.
      // Constants and direct function references need no such snapshot.
      if (index + 1 < prefixEnd && !isA<Const>(operand) && !util::getFunc(operand)) {
        auto *temporary = util::makeVar(operand, flow, parent);
        operand = module->N<VarValue>(operand->getSrcInfo(), temporary);
      }
      operands[index] = operand;
    }
    auto *call = module->N<CallInstr>(
        v->getSrcInfo(), operands.front(),
        std::vector<Value *>(operands.begin() + 1, operands.end()), v->getName());
    call->cloneAttributesFrom(v);
    v->replaceAll(module->N<FlowInstr>(v->getSrcInfo(), flow, call));
    ++flowReplacements;
    return;
  }

  auto *r = getAnalysisResult<analyze::module::SideEffectResult>(sideEffectsKey);
  if (!r->hasSideEffect(v))
    rewrite(v);
}

void CanonicalizationPass::handle(SeriesFlow *v) {
  auto it = v->begin();
  while (it != v->end()) {
    if (auto *series = cast<SeriesFlow>(*it)) {
      it = v->erase(it);
      ++flowReplacements;
      for (auto *x : *series) {
        it = v->insert(it, x);
        ++it;
      }
    } else if (auto *flowInstr = cast<FlowInstr>(*it)) {
      it = v->erase(it);
      ++flowReplacements;
      // inserting in reverse order causes [flow, value] to be added
      it = v->insert(it, flowInstr->getValue());
      it = v->insert(it, flowInstr->getFlow());
      // don't increment; re-traverse in case a new series flow added
    } else {
      // Split result = seq(effect(), value) into effect(); result = value,
      // and return seq(effect(), value) into effect(); return value.
      // This exposes plain RHS/return values to later CIR passes while keeping
      // the effects inside their original enclosing control-flow context.
      auto *assignment = cast<AssignInstr>(*it);
      auto *returnValue = cast<ReturnInstr>(*it);
      auto *sequence = cast<FlowInstr>(assignment    ? assignment->getRhs()
                                       : returnValue ? returnValue->getValue()
                                                     : nullptr);
      if (sequence) {
        if (assignment)
          assignment->setRhs(sequence->getValue());
        else
          returnValue->setValue(sequence->getValue());
        it = v->insert(it, sequence->getFlow());
        ++flowReplacements;
      } else {
        ++it;
      }
    }
  }
}

void CanonicalizationPass::registerStandardRules(Module *m) {
  registerRule("op-chain", std::make_unique<CanonOpChain>());
  registerRule("inequality", std::make_unique<CanonInequality>());
  registerRule("add-mul", std::make_unique<CanonAddMul>());
  registerRule("const-sub", std::make_unique<CanonConstSub>());
}

} // namespace cleanup
} // namespace transform
} // namespace ir
} // namespace codon
