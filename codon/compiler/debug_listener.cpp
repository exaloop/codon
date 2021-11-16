#include "debug_listener.h"

#include <algorithm>
#include <sstream>

#include "codon/runtime/lib.h"

namespace codon {
namespace {
std::string unmangleType(llvm::StringRef s) {
  auto p = s.rsplit('.');
  return (p.second.empty() ? p.first : p.second).str();
}

std::string unmangleFunc(llvm::StringRef s) {
  // separate type and function
  auto p = s.split(':');
  llvm::StringRef func = s;
  std::string type;
  if (!p.second.empty()) {
    type = unmangleType(p.first);
    func = p.second;
  }

  // trim off ".<id>"
  p = func.rsplit('.');
  if (!p.second.empty() && p.second.find_if([](char c) { return !std::isdigit(c); }) ==
                               llvm::StringRef::npos)
    func = p.first;

  // trim off generics
  func = func.split('[').first;

  // trim off qualified name
  p = func.rsplit('.');
  if (!p.second.empty())
    func = p.second;

  if (!type.empty())
    return type + "." + func.str();
  else
    return func.str();
}

std::string simplifyFile(llvm::StringRef s) {
  auto p = s.rsplit('/');
  return (p.second.empty() ? p.first : p.second).str();
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
  objects.emplace_back(key, &obj, start, stop);
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
      return sym.symbolizeCode(
          o.getObject(),
          {pc - o.getStart(), llvm::object::SectionedAddress::UndefSection});
    }
  }
  return llvm::DILineInfo();
}

std::string DebugListener::getPrettyBacktrace(const std::vector<uintptr_t> &backtrace) {
  auto invalid = [](const std::string &name) { return name == "<invalid>"; };
  std::ostringstream buf;
  buf << "\033[1mBacktrace:\033[0m\n";
  for (auto pc : backtrace) {
    auto src = symbolize(pc);
    if (auto err = src.takeError())
      break;
    if (invalid(src->FunctionName) || invalid(src->FileName))
      continue;
    buf << "  "
        << makeBacktraceFrameString(pc, unmangleFunc(src->FunctionName),
                                    simplifyFile(src->FileName), src->Line, src->Column)
        << "\n";
  }
  return buf.str();
}

} // namespace codon
