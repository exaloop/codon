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

private:
  std::string argv0;
  bool debug;
  std::string input;
  std::unique_ptr<PluginManager> plm;
  std::unique_ptr<ast::Cache> cache;
  std::unique_ptr<ir::Module> module;
  std::unique_ptr<ir::transform::PassManager> pm;
  std::unique_ptr<ir::LLVMVisitor> llvisitor;

  ParserError parse(bool isCode, const std::string &file, const std::string &code,
                    int startLine, int testFlags,
                    const std::unordered_map<std::string, std::string> &defines);

public:
  Compiler(const std::string &argv0, bool debug = false,
           const std::vector<std::string> &disabledPasses = {}, bool isTest = false);

  std::string getInput() const { return input; }
  PluginManager *getPluginManager() const { return plm.get(); }
  ast::Cache *getCache() const { return cache.get(); }
  ir::Module *getModule() const { return module.get(); }
  ir::transform::PassManager *getPassManager() const { return pm.get(); }
  ir::LLVMVisitor *getLLVMVisitor() const { return llvisitor.get(); }

  bool load(const std::string &plugin, std::string *errMsg);
  ParserError
  parseFile(const std::string &file, int testFlags = 0,
            const std::unordered_map<std::string, std::string> &defines = {});
  ParserError
  parseCode(const std::string &file, const std::string &code, int startLine = 0,
            int testFlags = 0,
            const std::unordered_map<std::string, std::string> &defines = {});
  void compile();
  ParserError docgen(const std::vector<std::string> &files, std::string *output);
};

} // namespace codon
