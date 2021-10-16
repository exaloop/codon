#include "plugins.h"

#include "codon/util/common.h"

namespace codon {

PluginManager::Error PluginManager::load(const std::string &path) {
  std::string errMsg;
  auto handle = llvm::sys::DynamicLibrary::getPermanentLibrary(path.c_str(), &errMsg);

  if (!handle.isValid())
    return Error::NOT_FOUND;

  auto *entry = (LoadFunc *)handle.getAddressOfSymbol("load");
  if (!entry)
    return Error::NO_ENTRYPOINT;

  auto dsl = (*entry)();
  plugins.push_back(std::make_unique<Plugin>(std::move(dsl), path, handle));
  return load(plugins.back()->dsl.get());
}

PluginManager::Error PluginManager::load(DSL *dsl) {
  if (!dsl || !dsl->isVersionSupported(CODON_VERSION_MAJOR, CODON_VERSION_MINOR,
                                       CODON_VERSION_PATCH))
    return Error::UNSUPPORTED_VERSION;
  dsl->addIRPasses(pm, debug);
  return Error::NONE;
}

} // namespace codon
