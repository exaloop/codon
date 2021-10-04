#include "analysis.h"

#include "sir/transform/manager.h"

namespace codon {
namespace ir {
namespace analyze {

const Result *Analysis::doGetAnalysis(const std::string &key) {
  return manager ? manager->getAnalysisResult(key) : nullptr;
}

} // namespace analyze
} // namespace ir
} // namespace codon
