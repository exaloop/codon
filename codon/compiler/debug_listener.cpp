// Copyright (C) 2022-2024 Exaloop Inc. <https://exaloop.io>

#include "debug_listener.h"

#include <algorithm>
#include <functional>
#include <sstream>

#include "codon/runtime/lib.h"

namespace codon {
namespace {
std::string
makeBacktrace(const std::vector<uintptr_t> &backtrace,
              std::function<llvm::Expected<std::string>(uintptr_t)> backtraceCallback) {
  std::ostringstream buf;
  buf << "\033[1mBacktrace:\033[0m\n";
  for (auto pc : backtrace) {
    auto line = backtraceCallback(pc);
    if (!line)
      break;
    if (!line->empty())
      buf << "  " << *line << "\n";
  }
  return buf.str();
}
} // namespace

void DebugListener::notifyObjectLoaded(ObjectKey key,
                                       const llvm::object::ObjectFile &obj,
                                       const llvm::RuntimeDyld::LoadedObjectInfo &L) {
  uintptr_t start = 0, stop = 0;
  for (const auto &sec : obj.sections()) {
    if (sec.isText()) {
      start = L.getSectionLoadAddress(sec);
      stop = start + sec.getSize();
      break;
    }
  }
  auto buf = llvm::MemoryBuffer::getMemBufferCopy(obj.getData(), obj.getFileName());
  auto newObj = llvm::cantFail(
      llvm::object::ObjectFile::createObjectFile(buf->getMemBufferRef()));
  objects.emplace_back(key, std::move(newObj), std::move(buf), start, stop);
}

void DebugListener::notifyFreeingObject(ObjectKey key) {
  objects.erase(
      std::remove_if(objects.begin(), objects.end(),
                     [key](const ObjectInfo &o) { return key == o.getKey(); }),
      objects.end());
}

llvm::Expected<llvm::DILineInfo> DebugListener::symbolize(uintptr_t pc) {
  for (const auto &o : objects) {
    if (o.contains(pc)) {
      llvm::symbolize::LLVMSymbolizer sym;
      return sym.symbolizeCode(
          o.getObject(),
          {pc - o.getStart(), llvm::object::SectionedAddress::UndefSection});
    }
  }
  return llvm::DILineInfo();
}

llvm::Expected<std::string> DebugListener::getPrettyBacktrace(uintptr_t pc) {
  auto invalid = [](const std::string &name) { return name == "<invalid>"; };
  auto src = symbolize(pc);
  if (auto err = src.takeError())
    return std::move(err);
  if (invalid(src->FunctionName) || invalid(src->FileName))
    return "";
  return runtime::makeBacktraceFrameString(pc, src->FunctionName, src->FileName,
                                           src->Line, src->Column);
}

std::string DebugListener::getPrettyBacktrace(const std::vector<uintptr_t> &backtrace) {
  return makeBacktrace(backtrace, [&](uintptr_t pc) { return getPrettyBacktrace(pc); });
}

void DebugPlugin::notifyMaterializing(llvm::orc::MaterializationResponsibility &mr,
                                      llvm::jitlink::LinkGraph &graph,
                                      llvm::jitlink::JITLinkContext &ctx,
                                      llvm::MemoryBufferRef inputObject) {
  auto newBuf =
      llvm::MemoryBuffer::getMemBufferCopy(inputObject.getBuffer(), graph.getName());
  auto newObj = llvm::cantFail(
      llvm::object::ObjectFile::createObjectFile(newBuf->getMemBufferRef()));

  {
    std::lock_guard<std::mutex> lock(pluginMutex);
    assert(pendingObjs.count(&mr) == 0);
    pendingObjs[&mr] = std::unique_ptr<JITObjectInfo>(
        new JITObjectInfo{std::move(newBuf), std::move(newObj), {}});
  }
}

llvm::Error DebugPlugin::notifyEmitted(llvm::orc::MaterializationResponsibility &mr) {
  {
    std::lock_guard<std::mutex> lock(pluginMutex);
    auto it = pendingObjs.find(&mr);
    if (it == pendingObjs.end())
      return llvm::Error::success();

    auto newInfo = pendingObjs[&mr].get();
    auto getLoadAddress = [newInfo](const llvm::StringRef &name) -> uint64_t {
      auto result = newInfo->sectionLoadAddresses.find(name);
      if (result == newInfo->sectionLoadAddresses.end())
        return 0;
      return result->second;
    };

    // register(*newInfo->Object, getLoadAddress, nullptr)
  }

  llvm::cantFail(mr.withResourceKeyDo([&](llvm::orc::ResourceKey key) {
    std::lock_guard<std::mutex> lock(pluginMutex);
    registeredObjs[key].push_back(std::move(pendingObjs[&mr]));
    pendingObjs.erase(&mr);
  }));

  return llvm::Error::success();
}

llvm::Error DebugPlugin::notifyFailed(llvm::orc::MaterializationResponsibility &mr) {
  std::lock_guard<std::mutex> lock(pluginMutex);
  pendingObjs.erase(&mr);
  return llvm::Error::success();
}

llvm::Error DebugPlugin::notifyRemovingResources(llvm::orc::JITDylib &jd,
                                                 llvm::orc::ResourceKey key) {
  std::lock_guard<std::mutex> lock(pluginMutex);
  registeredObjs.erase(key);
  return llvm::Error::success();
}

void DebugPlugin::notifyTransferringResources(llvm::orc::JITDylib &jd,
                                              llvm::orc::ResourceKey dstKey,
                                              llvm::orc::ResourceKey srcKey) {
  std::lock_guard<std::mutex> lock(pluginMutex);
  auto it = registeredObjs.find(srcKey);
  if (it != registeredObjs.end()) {
    for (std::unique_ptr<JITObjectInfo> &info : it->second)
      registeredObjs[dstKey].push_back(std::move(info));
    registeredObjs.erase(it);
  }
}

void DebugPlugin::modifyPassConfig(llvm::orc::MaterializationResponsibility &mr,
                                   llvm::jitlink::LinkGraph &graph,
                                   llvm::jitlink::PassConfiguration &config) {
  std::lock_guard<std::mutex> lock(pluginMutex);
  auto it = pendingObjs.find(&mr);
  if (it == pendingObjs.end())
    return;

  JITObjectInfo &info = *it->second;
  config.PostAllocationPasses.push_back(
      [&info, this](llvm::jitlink::LinkGraph &graph) -> llvm::Error {
        std::lock_guard<std::mutex> lock(pluginMutex);
        for (const llvm::jitlink::Section &sec : graph.sections()) {
#if defined(__APPLE__) && defined(__MACH__)
          size_t secPos = sec.getName().find(',');
          if (secPos >= 16 || (sec.getName().size() - (secPos + 1) > 16))
            continue;
          auto secName = sec.getName().substr(secPos + 1);
#else
          auto secName = sec.getName();
#endif
          info.sectionLoadAddresses[secName] =
              llvm::jitlink::SectionRange(sec).getStart().getValue();
        }
        return llvm::Error::success();
      });
}

llvm::Expected<llvm::DILineInfo> DebugPlugin::symbolize(uintptr_t pc) {
  for (const auto &entry : registeredObjs) {
    for (const auto &info : entry.second) {
      const auto *o = info->object.get();
      for (const auto &sec : o->sections()) {
        if (sec.isText()) {
          uintptr_t start =
              info->sectionLoadAddresses.lookup(llvm::cantFail(sec.getName()));
          uintptr_t stop = start + sec.getSize();
          if (start <= pc && pc < stop) {
            llvm::symbolize::LLVMSymbolizer sym;
            return sym.symbolizeCode(
                *o, {pc - start, llvm::object::SectionedAddress::UndefSection});
          }
        }
      }
    }
  }
  return llvm::DILineInfo();
}

llvm::Expected<std::string> DebugPlugin::getPrettyBacktrace(uintptr_t pc) {
  auto invalid = [](const std::string &name) { return name == "<invalid>"; };
  auto src = symbolize(pc);
  if (auto err = src.takeError())
    return std::move(err);
  if (invalid(src->FunctionName) || invalid(src->FileName))
    return "";
  return runtime::makeBacktraceFrameString(pc, src->FunctionName, src->FileName,
                                           src->Line, src->Column);
}

std::string DebugPlugin::getPrettyBacktrace(const std::vector<uintptr_t> &backtrace) {
  return makeBacktrace(backtrace, [&](uintptr_t pc) { return getPrettyBacktrace(pc); });
}

} // namespace codon
