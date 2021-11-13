#include "memory_manager.h"

#include "codon/runtime/lib.h"

namespace codon {

BoehmGCMemoryManager::BoehmGCMemoryManager() : SectionMemoryManager(), roots() {}

uint8_t *BoehmGCMemoryManager::allocateDataSection(uintptr_t size, unsigned alignment,
                                                   unsigned sectionID,
                                                   llvm::StringRef sectionName,
                                                   bool isReadOnly) {
  auto *result = SectionMemoryManager::allocateDataSection(size, alignment, sectionID,
                                                           sectionName, isReadOnly);
  void *start = result;
  void *end = result + size;
  seq_gc_add_roots(start, end);
  roots.emplace_back(start, end);
  return result;
}

BoehmGCMemoryManager::~BoehmGCMemoryManager() {
  for (const auto &root : roots) {
    seq_gc_remove_roots(root.first, root.second);
  }
}

} // namespace codon
