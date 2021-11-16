#include "debug_listener.h"

#include <algorithm>

namespace codon {

void DebugListener::notifyObjectLoaded(ObjectKey key,
                                       const llvm::object::ObjectFile &obj,
                                       const llvm::RuntimeDyld::LoadedObjectInfo &L) {
  intptr_t start = 0, stop = 0;
  for (const auto &sec : obj.sections()) {
    if (sec.isText()) {
      start = L.getSectionLoadAddress(sec);
      stop = start + sec.getSize();
      break;
    }
  }
  objects.emplace_back(key, &obj, start, stop);
}

void DebugListener::notifyFreeingObject(ObjectKey key) {
  objects.erase(
      std::remove_if(objects.begin(), objects.end(),
                     [key](const ObjectInfo &o) { return key == o.getKey(); }),
      objects.end());
}

llvm::Expected<llvm::DILineInfo> DebugListener::symbolize(intptr_t pc) {
  for (const auto &o : objects) {
    if (o.contains(pc)) {
      return sym.symbolizeCode(o.getObject(),
                               {static_cast<uint64_t>(pc - o.getStart()),
                                llvm::object::SectionedAddress::UndefSection});
    }
  }
  return llvm::DILineInfo();
}

} // namespace codon
