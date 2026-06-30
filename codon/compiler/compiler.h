// Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>

#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "codon/cir/llvm/llvisitor.h"
#include "codon/cir/module.h"
#include "codon/cir/transform/manager.h"
#include "codon/compiler/error.h"
#include "codon/compiler/options.h"
#include "codon/dsl/plugins.h"
#include "codon/parser/cache.h"

namespace codon {

class Compiler {
public:
  enum Mode {
    DEBUG,
    RELEASE,
    JIT,
  };

private:
  std::string input;
  std::unique_ptr<Options> options;
  std::unique_ptr<PluginManager> plm;
  std::unique_ptr<ast::Cache> cache;
  std::unique_ptr<ir::Module> module;
  std::unique_ptr<ir::transform::PassManager> pm;
  std::unique_ptr<ir::LLVMVisitor> llvisitor;
  std::unordered_set<std::string> loadedPlugins;

  llvm::Error parse(bool isCode, const std::string &file, const std::string &code,
                    int startLine, int testFlags,
                    const std::unordered_map<std::string, std::string> &defines);

public:
  explicit Compiler(const Options &options,
                    const std::shared_ptr<ast::IFilesystem> &fs = nullptr);
  ~Compiler();

  Options *getOptions() const { return options.get(); }
  std::string getInput() const { return input; }
  PluginManager *getPluginManager() const { return plm.get(); }
  ast::Cache *getCache() const { return cache.get(); }
  ir::Module *getModule() const { return module.get(); }
  ir::transform::PassManager *getPassManager() const { return pm.get(); }
  ir::LLVMVisitor *getLLVMVisitor() const { return llvisitor.get(); }
  bool isPluginLoaded(const std::string &) const;

  llvm::Error load(const std::string &plugin);
  llvm::Error
  parseFile(const std::string &file, int testFlags = 0,
            const std::unordered_map<std::string, std::string> &defines = {});
  llvm::Error
  parseCode(const std::string &file, const std::string &code, int startLine = 0,
            int testFlags = 0,
            const std::unordered_map<std::string, std::string> &defines = {});
  llvm::Error compile();
  llvm::Expected<std::string> docgen(const std::vector<std::string> &files);

  std::unordered_map<std::string, std::string> getEarlyDefines();
};

} // namespace codon
