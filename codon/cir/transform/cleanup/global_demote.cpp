// Copyright (C) 2022-2024 Exaloop Inc. <https://exaloop.io>

#include "global_demote.h"

namespace codon {
namespace ir {
namespace transform {
namespace cleanup {
namespace {
struct GetUsedGlobals : public util::Operator {
  std::vector<Var *> vars;
  void preHook(Node *v) override {
    for (auto *var : v->getUsedVariables()) {
      if (!isA<Func>(var) && var->isGlobal())
        vars.push_back(var);
    }
  }
};
} // namespace

const std::string GlobalDemotionPass::KEY = "core-cleanup-global-demote";

void GlobalDemotionPass::run(Module *M) {
  numDemotions = 0;
  std::unordered_map<Var *, Func *> localGlobals;

  std::vector<Func *> worklist = {M->getMainFunc()};
  for (auto *var : *M) {
    if (auto *func = cast<Func>(var))
      worklist.push_back(func);
  }

  for (auto *var : worklist) {
    if (auto *func = cast<Func>(var)) {
      GetUsedGlobals globals;
      func->accept(globals);

      for (auto *g : globals.vars) {
        LOG_IR("[{}] global {} used in {}", KEY, *g, func->getName());
        auto it = localGlobals.find(g);
        if (it == localGlobals.end()) {
          localGlobals.emplace(g, func);
        } else if (it->second && it->second != func) {
          it->second = nullptr;
        }
      }
    }
  }

  for (auto it : localGlobals) {
    if (!it.second || it.first->getId() == M->getArgVar()->getId() ||
        it.first->isExternal())
      continue;
    seqassertn(it.first->isGlobal(), "var was not global [{}]", it.first->getSrcInfo());
    it.first->setGlobal(false);
    if (auto *func = cast<BodiedFunc>(it.second)) {
      func->push_back(it.first);
      ++numDemotions;
      LOG_IR("[{}] demoted {} to a local of {}", KEY, *it.first, func->getName());
    }
  }
}

} // namespace cleanup
} // namespace transform
} // namespace ir
} // namespace codon
