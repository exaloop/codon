#pragma once

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "codon/dsl/dsl.h"
#include "codon/sir/util/iterators.h"
#include "llvm/Support/DynamicLibrary.h"

namespace codon {

/// Plugin metadata
struct Plugin {
  /// the associated DSL
  std::unique_ptr<DSL> dsl;
  /// plugin load path
  std::string path;
  /// library handle
  llvm::sys::DynamicLibrary handle;

  Plugin(std::unique_ptr<DSL> dsl, const std::string &path,
         const llvm::sys::DynamicLibrary &handle)
      : dsl(std::move(dsl)), path(path), handle(handle) {}
};

/// Manager for loading, applying and unloading plugins.
class PluginManager {
private:
  /// pass manager with which to register plugin IR passes
  ir::transform::PassManager *pm;
  /// vector of loaded plugins
  std::vector<std::unique_ptr<Plugin>> plugins;
  /// true if compiling in debug mode
  bool debug;

public:
  using LoadFunc = std::function<std::unique_ptr<DSL>()>;

  /// Error codes when loading plugins
  enum Error { NONE = 0, NOT_FOUND, NO_ENTRYPOINT, UNSUPPORTED_VERSION };

  /// Constructs a plugin manager from a given IR pass manager
  /// @param pm the IR pass manager to register IR passes with
  /// @param debug true if compining in debug mode
  explicit PluginManager(ir::transform::PassManager *pm, bool debug = false)
      : pm(pm), plugins(), debug(debug) {}

  /// @return iterator to the first plugin
  auto begin() { return ir::util::raw_ptr_adaptor(plugins.begin()); }
  /// @return iterator beyond the last plugin
  auto end() { return ir::util::raw_ptr_adaptor(plugins.end()); }
  /// @return const iterator to the first plugin
  auto begin() const { return ir::util::const_raw_ptr_adaptor(plugins.begin()); }
  /// @return const iterator beyond the last plugin
  auto end() const { return ir::util::const_raw_ptr_adaptor(plugins.end()); }

  /// Loads the plugin at the given load path.
  Error load(const std::string &path);
  /// Loads the given DSL
  Error load(DSL *dsl);
};

} // namespace codon
