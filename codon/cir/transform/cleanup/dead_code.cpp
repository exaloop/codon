// Copyright (C) 2022-2024 Exaloop Inc. <https://exaloop.io>

#include "dead_code.h"

#include "codon/cir/analyze/module/side_effect.h"
#include "codon/cir/util/cloning.h"

namespace codon {
namespace ir {
namespace transform {
namespace cleanup {
namespace {
BoolConst *boolConst(Value *v) { return cast<BoolConst>(v); }

IntConst *intConst(Value *v) { return cast<IntConst>(v); }
} // namespace

const std::string DeadCodeCleanupPass::KEY = "core-cleanup-dce";

void DeadCodeCleanupPass::run(Module *m) {
  numReplacements = 0;
  OperatorPass::run(m);
}

void DeadCodeCleanupPass::handle(SeriesFlow *v) {
  auto *r = getAnalysisResult<analyze::module::SideEffectResult>(sideEffectsKey);
  auto it = v->begin();
  while (it != v->end()) {
    if (!r->hasSideEffect(*it)) {
      LOG_IR("[{}] no side effect, deleting: {}", KEY, **it);
      numReplacements++;
      it = v->erase(it);
    } else {
      ++it;
    }
  }
}

void DeadCodeCleanupPass::handle(IfFlow *v) {
  auto *cond = boolConst(v->getCond());
  if (!cond)
    return;

  auto *M = v->getModule();
  auto condVal = cond->getVal();

  util::CloneVisitor cv(M);
  if (condVal) {
    doReplacement(v, cv.clone(v->getTrueBranch()));
  } else if (auto *f = v->getFalseBranch()) {
    doReplacement(v, cv.clone(f));
  } else {
    doReplacement(v, M->Nr<SeriesFlow>());
  }
}

void DeadCodeCleanupPass::handle(WhileFlow *v) {
  auto *cond = boolConst(v->getCond());
  if (!cond)
    return;

  auto *M = v->getModule();
  auto condVal = cond->getVal();
  if (!condVal) {
    doReplacement(v, M->Nr<SeriesFlow>());
  }
}

void DeadCodeCleanupPass::handle(ImperativeForFlow *v) {
  auto *start = intConst(v->getStart());
  auto *end = intConst(v->getEnd());
  if (!start || !end)
    return;

  auto stepVal = v->getStep();
  auto startVal = start->getVal();
  auto endVal = end->getVal();

  auto *M = v->getModule();
  if ((stepVal < 0 && startVal <= endVal) || (stepVal > 0 && startVal >= endVal)) {
    doReplacement(v, M->Nr<SeriesFlow>());
  }
}

void DeadCodeCleanupPass::handle(TernaryInstr *v) {
  auto *cond = boolConst(v->getCond());
  if (!cond)
    return;

  auto *M = v->getModule();
  auto condVal = cond->getVal();

  util::CloneVisitor cv(M);
  if (condVal) {
    doReplacement(v, cv.clone(v->getTrueValue()));
  } else {
    doReplacement(v, cv.clone(v->getFalseValue()));
  }
}

void DeadCodeCleanupPass::doReplacement(Value *og, Value *v) {
  numReplacements++;
  og->replaceAll(v);
}

} // namespace cleanup
} // namespace transform
} // namespace ir
} // namespace codon
