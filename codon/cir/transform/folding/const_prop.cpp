// Copyright (C) 2022-2024 Exaloop Inc. <https://exaloop.io>

#include "const_prop.h"

#include "codon/cir/analyze/dataflow/reaching.h"
#include "codon/cir/analyze/module/global_vars.h"
#include "codon/cir/util/cloning.h"

namespace codon {
namespace ir {
namespace transform {
namespace folding {
namespace {
bool okConst(Value *v) {
  return v && (isA<IntConst>(v) || isA<FloatConst>(v) || isA<BoolConst>(v));
}
} // namespace

const std::string ConstPropPass::KEY = "core-folding-const-prop";

void ConstPropPass::handle(VarValue *v) {
  auto *M = v->getModule();

  auto *var = v->getVar();

  Value *replacement;
  if (var->isGlobal()) {
    auto *r = getAnalysisResult<analyze::module::GlobalVarsResult>(globalVarsKey);
    if (!r)
      return;

    auto it = r->assignments.find(var->getId());
    if (it == r->assignments.end())
      return;

    auto *constDef = M->getValue(it->second);
    if (!okConst(constDef))
      return;

    util::CloneVisitor cv(M);
    replacement = cv.clone(constDef);
  } else {
    auto *r = getAnalysisResult<analyze::dataflow::RDResult>(reachingDefKey);
    if (!r)
      return;
    auto *c = r->cfgResult;

    auto it = r->results.find(getParentFunc()->getId());
    auto it2 = c->graphs.find(getParentFunc()->getId());
    if (it == r->results.end() || it2 == c->graphs.end())
      return;

    auto *rd = it->second.get();
    auto *cfg = it2->second.get();

    auto reaching = rd->getReachingDefinitions(var, v);

    if (reaching.size() != 1)
      return;

    auto def = *reaching.begin();
    if (def == -1)
      return;

    auto *constDef = cfg->getValue(def);
    if (!okConst(constDef))
      return;

    util::CloneVisitor cv(M);
    replacement = cv.clone(constDef);
  }

  v->replaceAll(replacement);
}

} // namespace folding
} // namespace transform
} // namespace ir
} // namespace codon
