// Copyright (C) 2022-2024 Exaloop Inc. <https://exaloop.io>

#include "pipeline.h"

#include <algorithm>

#include "codon/cir/util/cloning.h"
#include "codon/cir/util/irtools.h"
#include "codon/cir/util/matching.h"

namespace codon {
namespace ir {
namespace transform {
namespace lowering {
namespace {
Value *callStage(Module *M, PipelineFlow::Stage *stage, Value *last) {
  std::vector<Value *> args;
  for (auto *arg : *stage) {
    args.push_back(arg ? arg : last);
  }
  return M->N<CallInstr>(stage->getCallee()->getSrcInfo(), stage->getCallee(), args);
}

Value *convertPipelineToForLoopsHelper(Module *M, BodiedFunc *parent,
                                       const std::vector<PipelineFlow::Stage *> &stages,
                                       unsigned idx = 0, Value *last = nullptr) {
  if (idx >= stages.size())
    return last;

  auto *stage = stages[idx];
  if (idx == 0)
    return convertPipelineToForLoopsHelper(M, parent, stages, idx + 1,
                                           stage->getCallee());

  auto *prev = stages[idx - 1];
  if (prev->isGenerator()) {
    auto *var = M->Nr<Var>(prev->getOutputElementType());
    parent->push_back(var);
    auto *body = convertPipelineToForLoopsHelper(
        M, parent, stages, idx + 1, callStage(M, stage, M->Nr<VarValue>(var)));
    auto *loop = M->N<ForFlow>(last->getSrcInfo(), last, util::series(body), var);
    if (stage->isParallel())
      loop->setParallel();
    return loop;
  } else {
    return convertPipelineToForLoopsHelper(M, parent, stages, idx + 1,
                                           callStage(M, stage, last));
  }
}

Value *convertPipelineToForLoops(PipelineFlow *p, BodiedFunc *parent) {
  std::vector<PipelineFlow::Stage *> stages;
  for (auto &stage : *p) {
    stages.push_back(&stage);
  }
  return convertPipelineToForLoopsHelper(p->getModule(), parent, stages);
}
} // namespace

const std::string PipelineLowering::KEY = "core-pipeline-lowering";

void PipelineLowering::handle(PipelineFlow *v) {
  v->replaceAll(convertPipelineToForLoops(v, cast<BodiedFunc>(getParentFunc())));
}

} // namespace lowering
} // namespace transform
} // namespace ir
} // namespace codon
