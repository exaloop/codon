#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "codon/dsl/plugins.h"
#include "codon/parser/cache.h"
#include "codon/sir/llvm/llvisitor.h"
#include "codon/sir/module.h"
#include "codon/sir/transform/manager.h"

namespace codon {

class Compiler {
private:
  bool debug;
  std::string input;
  std::unique_ptr<PluginManager> plm;
  std::unique_ptr<ast::Cache> cache;
  std::unique_ptr<ir::Module> module;
  std::unique_ptr<ir::transform::PassManager> pm;
  std::unique_ptr<ir::LLVMVisitor> llvisitor;

public:
  struct ParserError {
    struct Message {
      std::string msg;
      std::string file;
      int line = 0;
      int col = 0;
    };

    bool error;
    std::vector<Message> messages;

    explicit ParserError(bool error) : error(error), messages() {}
    operator bool() const { return error; }

    static ParserError success() { return ParserError(false); }
    static ParserError failure() { return ParserError(true); }
  };

  Compiler(const std::string &argv0, bool debug = false,
           const std::vector<std::string> &disabledPasses = {});

  std::string getInput() const { return input; }
  PluginManager *getPluginManager() const { return plm.get(); }
  ast::Cache *getCache() const { return cache.get(); }
  ir::Module *getModule() const { return module.get(); }
  ir::transform::PassManager *getPassManager() const { return pm.get(); }
  ir::LLVMVisitor *getLLVMVisitor() const { return llvisitor.get(); }

  bool load(const std::string &plugin, std::string *errMsg);
  ParserError
  parseFile(const std::string &file, int isTest = 0,
            const std::unordered_map<std::string, std::string> &defines = {});
  ParserError
  parseCode(const std::string &code, const std::string &file, int isTest = 0,
            int startLine = 0,
            const std::unordered_map<std::string, std::string> &defines = {});
  void compile();
};

} // namespace codon
