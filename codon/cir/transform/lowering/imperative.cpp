// Copyright (C) 2022-2024 Exaloop Inc. <https://exaloop.io>

#include "imperative.h"

#include <algorithm>

#include "codon/cir/util/cloning.h"
#include "codon/cir/util/irtools.h"
#include "codon/cir/util/matching.h"

namespace codon {
namespace ir {
namespace transform {
namespace lowering {
namespace {
CallInstr *getRangeIter(Value *iter) {
  auto *M = iter->getModule();

  auto *iterCall = cast<CallInstr>(iter);
  if (!iterCall || iterCall->numArgs() != 1)
    return nullptr;

  auto *iterFunc = util::getFunc(iterCall->getCallee());
  if (!iterFunc || iterFunc->getUnmangledName() != Module::ITER_MAGIC_NAME)
    return nullptr;

  auto *rangeCall = cast<CallInstr>(iterCall->front());
  if (!rangeCall)
    return nullptr;

  auto *newRangeFunc = util::getFunc(rangeCall->getCallee());
  if (!newRangeFunc || newRangeFunc->getUnmangledName() != Module::NEW_MAGIC_NAME)
    return nullptr;
  auto *parentType = newRangeFunc->getParentType();
  auto *rangeType = M->getOrRealizeType("range", {}, "std.internal.types.range");

  if (!parentType || !rangeType || parentType->getName() != rangeType->getName())
    return nullptr;

  return rangeCall;
}

Value *getListIter(Value *iter) {
  auto *iterCall = cast<CallInstr>(iter);
  if (!iterCall || iterCall->numArgs() != 1)
    return nullptr;

  auto *iterFunc = util::getFunc(iterCall->getCallee());
  if (!iterFunc || iterFunc->getUnmangledName() != Module::ITER_MAGIC_NAME)
    return nullptr;

  auto *list = iterCall->front();
  if (list->getType()->getName().rfind("std.internal.types.ptr.List[", 0) != 0)
    return nullptr;

  return list;
}
} // namespace

const std::string ImperativeForFlowLowering::KEY = "core-imperative-for-lowering";

void ImperativeForFlowLowering::handle(ForFlow *v) {
  auto *M = v->getModule();
  auto *iter = v->getIter();
  std::unique_ptr<parallel::OMPSched> sched;
  if (v->isParallel())
    sched = std::make_unique<parallel::OMPSched>(*v->getSchedule());

  if (auto *rangeCall = getRangeIter(iter)) {
    auto it = rangeCall->begin();
    auto argCount = std::distance(it, rangeCall->end());

    util::CloneVisitor cv(M);

    IntConst *stepConst;
    Value *start;
    Value *end;
    int64_t step = 0;

    switch (argCount) {
    case 1:
      start = M->getInt(0);
      end = cv.clone(*it);
      step = 1;
      break;
    case 2:
      start = cv.clone(*it++);
      end = cv.clone(*it);
      step = 1;
      break;
    case 3:
      start = cv.clone(*it++);
      end = cv.clone(*it++);
      stepConst = cast<IntConst>(*it);
      if (!stepConst)
        return;
      step = stepConst->getVal();
      break;
    default:
      seqassertn(false, "unknown range constructor");
    }
    if (step == 0)
      return;

    v->replaceAll(M->N<ImperativeForFlow>(v->getSrcInfo(), start, step, end,
                                          v->getBody(), v->getVar(), std::move(sched)));
  } else if (auto *list = getListIter(iter)) {
    // convert:
    //   for a in list:
    //     body
    // into:
    //   v = list
    //   n = v.len
    //   p = v.arr.ptr
    //   imp_for i in range(0, n, 1):
    //     a = p[i]
    //     body
    auto *parent = cast<BodiedFunc>(getParentFunc());
    auto *series = M->N<SeriesFlow>(v->getSrcInfo());
    auto *listVar = util::makeVar(list, series, parent)->getVar();
    auto *lenVal = M->Nr<ExtractInstr>(M->Nr<VarValue>(listVar), "len");
    auto *lenVar = util::makeVar(lenVal, series, parent);
    auto *ptrVal = M->Nr<ExtractInstr>(
        M->Nr<ExtractInstr>(M->Nr<VarValue>(listVar), "arr"), "ptr");
    auto *ptrVar = util::makeVar(ptrVal, series, parent);

    auto *body = cast<SeriesFlow>(v->getBody());
    seqassertn(body, "loop body is not a series flow [{}]", v->getSrcInfo());
    auto *oldLoopVar = v->getVar();
    auto *newLoopVar = M->Nr<Var>(M->getIntType());
    parent->push_back(newLoopVar);
    auto *replacement = M->N<ImperativeForFlow>(
        v->getSrcInfo(), M->getInt(0), 1, lenVar, body, newLoopVar, std::move(sched));
    series->push_back(replacement);
    body->insert(
        body->begin(),
        M->Nr<AssignInstr>(oldLoopVar, (*ptrVar)[*M->Nr<VarValue>(newLoopVar)]));
    v->replaceAll(series);
  }
}

} // namespace lowering
} // namespace transform
} // namespace ir
} // namespace codon
