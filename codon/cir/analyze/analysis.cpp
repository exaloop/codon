// Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>

#include "analysis.h"

#include "codon/cir/transform/manager.h"

namespace codon {
namespace ir {
namespace analyze {

Options *Analysis::getOptions() const {
  return manager ? manager->getOptions() : nullptr;
}

Result *Analysis::doGetAnalysis(const std::string &key) {
  return manager ? manager->getAnalysisResult(key) : nullptr;
}

} // namespace analyze
} // namespace ir
} // namespace codon
