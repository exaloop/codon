#include "plugins.h"
#include "util/common.h"
#include <dlfcn.h>

namespace seq {

PluginManager::~PluginManager() {
  for (auto &plugin : plugins) {
    dlclose(plugin.handle);
  }
}

PluginManager::Error PluginManager::load(const std::string &path) {
  void *handle = dlopen(path.c_str(), RTLD_LAZY);
  if (!handle)
    return Error::NOT_FOUND;

  auto *entry = (LoadFunc *)dlsym(handle, "load");
  if (!entry)
    return Error::NO_ENTRYPOINT;

  auto dsl = (*entry)();
  plugins.push_back({std::move(dsl), path, handle});
  return load(plugins.back().dsl.get());
}

PluginManager::Error PluginManager::load(DSL *dsl) {
  if (!dsl ||
      !dsl->isVersionSupported(SEQ_VERSION_MAJOR, SEQ_VERSION_MINOR, SEQ_VERSION_PATCH))
    return Error::UNSUPPORTED_VERSION;

  dsl->addIRPasses(pm, debug);
  // TODO: register new keywords

  return Error::NONE;
}

} // namespace seq
