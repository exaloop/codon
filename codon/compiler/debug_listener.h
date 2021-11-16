#pragma once

#include <memory>
#include <vector>

#include "codon/sir/llvm/llvm.h"

namespace codon {

class DebugListener : public llvm::JITEventListener {
public:
  class ObjectInfo {
  private:
    ObjectKey key;
    const llvm::object::ObjectFile *object;
    intptr_t start;
    intptr_t stop;

  public:
    ObjectInfo(ObjectKey key, const llvm::object::ObjectFile *object, intptr_t start,
               intptr_t stop)
        : key(key), object(object), start(start), stop(stop) {}

    ObjectKey getKey() const { return key; }
    const llvm::object::ObjectFile &getObject() const { return *object; }
    intptr_t getStart() const { return start; }
    intptr_t getStop() const { return stop; }
    bool contains(intptr_t pc) const { return start <= pc && pc < stop; }
  };

private:
  llvm::symbolize::LLVMSymbolizer sym;
  std::vector<ObjectInfo> objects;

  void notifyObjectLoaded(ObjectKey key, const llvm::object::ObjectFile &obj,
                          const llvm::RuntimeDyld::LoadedObjectInfo &L) override;
  void notifyFreeingObject(ObjectKey key) override;

public:
  DebugListener() : llvm::JITEventListener(), sym(), objects() {}

  llvm::Expected<llvm::DILineInfo> symbolize(intptr_t pc);
};

} // namespace codon
