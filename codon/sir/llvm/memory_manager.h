#pragma once

#include <utility>
#include <vector>

#include "codon/sir/llvm/llvm.h"

namespace codon {
namespace ir {

/// Simple extension of LLVM's SectionMemoryManager which catches data section
/// allocations and registers them with the GC. This allows the GC to know not
/// to collect globals even in JIT mode.
class BoehmGCMemoryManager : public llvm::SectionMemoryManager {
private:
  /// Vector of (start, end) address pairs registered with GC.
  std::vector<std::pair<void *, void *>> roots;
  uint8_t *allocateDataSection(uintptr_t size, unsigned alignment, unsigned sectionID,
                               llvm::StringRef sectionName, bool isReadOnly) override;

public:
  BoehmGCMemoryManager();
  ~BoehmGCMemoryManager() override;
};

} // namespace ir
} // namespace codon
