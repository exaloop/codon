// Copyright (C) 2022-2024 Exaloop Inc. <https://exaloop.io>

#include "analysis.h"

#include "codon/cir/transform/manager.h"

namespace codon {
namespace ir {
namespace analyze {

Result *Analysis::doGetAnalysis(const std::string &key) {
  return manager ? manager->getAnalysisResult(key) : nullptr;
}

} // namespace analyze
} // namespace ir
} // namespace codon
