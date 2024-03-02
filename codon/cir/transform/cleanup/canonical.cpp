// Copyright (C) 2022-2024 Exaloop Inc. <https://exaloop.io>

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
  return fn && util::hasAttribute(fn, "std.internal.attributes.commutative");
}

bool isAssociativeOp(Func *fn) {
  return fn && util::hasAttribute(fn, "std.internal.attributes.associative");
}

bool isDistributiveOp(Func *fn) {
  return fn && util::hasAttribute(fn, "std.internal.attributes.distributive");
}

bool isInequalityOp(Func *fn) {
  static const std::unordered_set<std::string> ops = {
      Module::EQ_MAGIC_NAME, Module::NE_MAGIC_NAME, Module::LT_MAGIC_NAME,
      Module::LE_MAGIC_NAME, Module::GT_MAGIC_NAME, Module::GE_MAGIC_NAME};
  return fn && ops.find(fn->getUnmangledName()) != ops.end();
}

// c + b + a --> a + b + c
struct CanonOpChain : public RewriteRule {
  static void extractAssociativeOpChain(Value *v, const std::string &op,
                                        types::Type *type,
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
    types::Type *type = v->getType();
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
    types::Type *type = v->getType();

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
  OperatorPass::run(m);
}

void CanonicalizationPass::handle(CallInstr *v) {
  auto *r = getAnalysisResult<analyze::module::SideEffectResult>(sideEffectsKey);
  if (!r->hasSideEffect(v))
    rewrite(v);
}

void CanonicalizationPass::handle(SeriesFlow *v) {
  auto it = v->begin();
  while (it != v->end()) {
    if (auto *series = cast<SeriesFlow>(*it)) {
      it = v->erase(it);
      for (auto *x : *series) {
        it = v->insert(it, x);
        ++it;
      }
    } else if (auto *flowInstr = cast<FlowInstr>(*it)) {
      it = v->erase(it);
      // inserting in reverse order causes [flow, value] to be added
      it = v->insert(it, flowInstr->getValue());
      it = v->insert(it, flowInstr->getFlow());
      // don't increment; re-traverse in case a new series flow added
    } else {
      ++it;
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
