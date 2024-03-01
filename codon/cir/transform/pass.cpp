// Copyright (C) 2022-2024 Exaloop Inc. <https://exaloop.io>

#include "pass.h"

#include "codon/cir/transform/manager.h"

namespace codon {
namespace ir {
namespace transform {

analyze::Result *Pass::doGetAnalysis(const std::string &key) {
  return manager ? manager->getAnalysisResult(key) : nullptr;
}

void PassGroup::run(Module *module) {
  for (auto &p : passes)
    p->run(module);
}

void PassGroup::setManager(PassManager *mng) {
  Pass::setManager(mng);
  for (auto &p : passes)
    p->setManager(mng);
}

} // namespace transform
} // namespace ir
} // namespace codon
