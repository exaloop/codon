#include "debug_listener.h"

#include <algorithm>
#include <sstream>

#include "codon/runtime/lib.h"

namespace codon {

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
      llvm::symbolize::LLVMSymbolizer::Options opt;
      opt.PrintFunctions = llvm::DILineInfoSpecifier::FunctionNameKind::ShortName;
      opt.PathStyle = llvm::DILineInfoSpecifier::FileLineInfoKind::BaseNameOnly;
      llvm::symbolize::LLVMSymbolizer sym(opt);
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
  return makeBacktraceFrameString(pc, src->FunctionName, src->FileName, src->Line,
                                  src->Column);
}

std::string DebugListener::getPrettyBacktrace(const std::vector<uintptr_t> &backtrace) {
  std::ostringstream buf;
  buf << "\033[1mBacktrace:\033[0m\n";
  for (auto pc : backtrace) {
    auto line = getPrettyBacktrace(pc);
    if (!line)
      break;
    if (!line->empty())
      buf << "  " << *line << "\n";
  }
  return buf.str();
}

} // namespace codon
