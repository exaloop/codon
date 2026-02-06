// Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>

#include "peg.h"

#include <any>
#include <memory>
#include <peglib.h>
#include <string>
#include <vector>

#include "codon/parser/ast.h"
#include "codon/parser/common.h"
#include "codon/parser/peg/rules.h"
#include "codon/parser/visitors/format/format.h"

#include <ranges>

double totalPeg = 0.0;

namespace codon::ast {

static std::shared_ptr<peg::Grammar> grammar(nullptr);
static std::shared_ptr<peg::Grammar> ompGrammar(nullptr);

std::shared_ptr<peg::Grammar> initParser() {
  auto g = std::make_shared<peg::Grammar>();
  init_codon_rules(*g);
  init_codon_actions(*g);
  ~(*g)["NLP"] <=
      peg::usr([](const char *s, size_t n, peg::SemanticValues &, std::any &dt) {
        auto e = (n >= 1 && s[0] == '\\' ? 1 : -1);
        if (std::any_cast<ParseContext &>(dt).parens && e == -1)
          e = 0;
        return e;
      });
  for (auto &val : *g | std::views::values) {
    auto v = peg::LinkReferences(*g, val.params);
    val.accept(v);
  }
  (*g)["program"].enablePackratParsing = true;
  (*g)["fstring"].enablePackratParsing = true;
  for (auto &rule : std::vector<std::string>{
           "arguments", "slices", "genexp", "parentheses", "star_parens", "generics",
           "with_parens_item", "params", "from_as_parens", "from_params"}) {
    (*g)[rule].enter = [](const peg::Context &, const char *, size_t, std::any &dt) {
      std::any_cast<ParseContext &>(dt).parens++;
    };
    (*g)[rule.c_str()].leave = [](const peg::Context &, const char *, size_t, size_t,
                                  std::any &, std::any &dt) {
      std::any_cast<ParseContext &>(dt).parens--;
    };
  }
  return g;
}

template <typename T>
llvm::Expected<T> parseCode(Cache *cache, const std::string &file,
                            const std::string &code, int line_offset, int col_offset,
                            const std::string &rule) {
  Timer t("");
  t.logged = true;
  // Initialize
  if (!grammar)
    grammar = initParser();

  std::vector<ErrorMessage> errors;
  auto log = [&](size_t line, size_t col, const std::string &msg, const std::string &) {
    size_t ed = msg.size();
    if (startswith(msg, "syntax error, unexpected")) {
      auto i = msg.find(", expecting");
      if (i != std::string::npos)
        ed = i;
    }
    errors.emplace_back(msg.substr(0, ed), file, line, col);
  };
  T result;
  auto ctx = std::make_any<ParseContext>(cache, 0, line_offset, col_offset);
  auto r = (*grammar)[rule].parse_and_get_value(code.c_str(), code.size(), ctx, result,
                                                file.c_str(), log);
  auto ret = r.ret && r.len == code.size();
  if (!ret)
    r.error_info.output_log(log, code.c_str(), code.size());
  totalPeg += t.elapsed();

  if (!errors.empty())
    return llvm::make_error<error::ParserErrorInfo>(errors);
  return result;
}

llvm::Expected<Stmt *> parseCode(Cache *cache, const std::string &file,
                                 const std::string &code, int line_offset) {
  return parseCode<Stmt *>(cache, file, code + "\n", line_offset, 0, "program");
}

llvm::Expected<std::pair<Expr *, StringExpr::FormatSpec>>
parseExpr(Cache *cache, const std::string &code, const codon::SrcInfo &offset) {
  auto newCode = code;
  ltrim(newCode);
  rtrim(newCode);
  return parseCode<std::pair<Expr *, StringExpr::FormatSpec>>(
      cache, offset.file, newCode, offset.line, offset.col, "fstring");
}

llvm::Expected<Stmt *> parseFile(Cache *cache, const std::string &file) {
  auto lines = cache->fs->read_lines(file);
  cache->imports[file].content = lines;
  std::string code = join(lines, "\n");
  auto result = parseCode(cache, file, code);
  // /* For debugging purposes: */ LOG("peg/{} :=  {}", file, result);
  return result;
}

std::shared_ptr<peg::Grammar> initOpenMPParser() {
  auto g = std::make_shared<peg::Grammar>();
  init_omp_rules(*g);
  init_omp_actions(*g);
  for (auto &val : *g | std::views::values) {
    auto v = peg::LinkReferences(*g, val.params);
    val.accept(v);
  }
  (*g)["pragma"].enablePackratParsing = true;
  return g;
}

llvm::Expected<std::vector<CallArg>> parseOpenMP(Cache *cache, const std::string &code,
                                                 const codon::SrcInfo &loc) {
  if (!ompGrammar)
    ompGrammar = initOpenMPParser();

  std::vector<ErrorMessage> errors;
  auto log = [&](size_t line, size_t col, const std::string &msg, const std::string &) {
    errors.emplace_back(fmt::format("openmp: {}", msg), loc.file, loc.line, loc.col);
  };
  std::vector<CallArg> result;
  auto ctx = std::make_any<ParseContext>(cache, 0, 0, 0);
  auto r = (*ompGrammar)["pragma"].parse_and_get_value(code.c_str(), code.size(), ctx,
                                                       result, "", log);
  auto ret = r.ret && r.len == code.size();
  if (!ret)
    r.error_info.output_log(log, code.c_str(), code.size());
  if (!errors.empty())
    return llvm::make_error<error::ParserErrorInfo>(errors);
  return result;
}

} // namespace codon::ast
