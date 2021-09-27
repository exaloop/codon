#pragma once

#include "dsl.h"
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace seq {

/// Plugin metadata
struct Plugin {
  /// the associated DSL
  std::unique_ptr<DSL> dsl;
  /// plugin load path
  std::string path;
  /// plugin dlopen handle
  void *handle;
};

/// Manager for loading, applying and unloading plugins.
class PluginManager {
private:
  /// pass manager with which to register plugin IR passes
  ir::transform::PassManager *pm;
  /// vector of loaded plugins
  std::vector<Plugin> plugins;
  /// true if compiling in debug mode
  bool debug;

public:
  using LoadFunc = std::function<std::unique_ptr<DSL>()>;

  /// Error codes when loading plugins
  enum Error { NONE = 0, NOT_FOUND, NO_ENTRYPOINT, UNSUPPORTED_VERSION };

  /// Constructs a plugin manager from a given IR pass manager
  /// @param pm the IR pass manager to register IR passes with
  explicit PluginManager(ir::transform::PassManager *pm, bool debug = false)
      : pm(pm), plugins(), debug(debug) {}

  ~PluginManager();

  /// Loads the plugin at the given load path.
  Error load(const std::string &path);
  /// Loads the given DSL
  Error load(DSL *dsl);
};

} // namespace seq
