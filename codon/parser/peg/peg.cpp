// Copyright (C) 2022-2024 Exaloop Inc. <https://exaloop.io>

#include "peg.h"

#include <any>
#include <iostream>
#include <memory>
#include <peglib.h>
#include <string>
#include <vector>

#include "codon/parser/ast.h"
#include "codon/parser/common.h"
#include "codon/parser/peg/rules.h"
#include "codon/parser/visitors/format/format.h"

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
  for (auto &x : *g) {
    auto v = peg::LinkReferences(*g, x.second.params);
    x.second.accept(v);
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
T parseCode(Cache *cache, const std::string &file, const std::string &code,
            int line_offset, int col_offset, const std::string &rule) {
  Timer t("");
  t.logged = true;
  // Initialize
  if (!grammar)
    grammar = initParser();

  std::vector<std::tuple<size_t, size_t, std::string>> errors;
  auto log = [&](size_t line, size_t col, const std::string &msg, const std::string &) {
    size_t ed = msg.size();
    if (startswith(msg, "syntax error, unexpected")) {
      auto i = msg.find(", expecting");
      if (i != std::string::npos)
        ed = i;
    }
    errors.emplace_back(line, col, msg.substr(0, ed));
  };
  T result;
  auto ctx = std::make_any<ParseContext>(cache, 0, line_offset, col_offset);
  auto r = (*grammar)[rule].parse_and_get_value(code.c_str(), code.size(), ctx, result,
                                                file.c_str(), log);
  auto ret = r.ret && r.len == code.size();
  if (!ret)
    r.error_info.output_log(log, code.c_str(), code.size());
  totalPeg += t.elapsed();
  exc::ParserException ex;
  if (!errors.empty()) {
    for (auto &e : errors)
      ex.track(fmt::format("{}", std::get<2>(e)),
               SrcInfo(file, std::get<0>(e), std::get<1>(e), 0));
    throw ex;
    return T();
  }
  return result;
}

StmtPtr parseCode(Cache *cache, const std::string &file, const std::string &code,
                  int line_offset) {
  return parseCode<StmtPtr>(cache, file, code + "\n", line_offset, 0, "program");
}

std::pair<ExprPtr, std::string> parseExpr(Cache *cache, const std::string &code,
                                          const codon::SrcInfo &offset) {
  auto newCode = code;
  ltrim(newCode);
  rtrim(newCode);
  auto e = parseCode<std::pair<ExprPtr, std::string>>(
      cache, offset.file, newCode, offset.line, offset.col, "fstring");
  return e;
}

StmtPtr parseFile(Cache *cache, const std::string &file) {
  std::vector<std::string> lines;
  std::string code;
  if (file == "-") {
    for (std::string line; getline(std::cin, line);) {
      lines.push_back(line);
      code += line + "\n";
    }
  } else {
    std::ifstream fin(file);
    if (!fin)
      E(error::Error::COMPILER_NO_FILE, SrcInfo(), file);
    for (std::string line; getline(fin, line);) {
      lines.push_back(line);
      code += line + "\n";
    }
    fin.close();
  }

  cache->imports[file].content = lines;
  auto result = parseCode(cache, file, code);
  // For debugging purposes:
  // LOG("peg/{} :=  {}", file, result);
  return result;
}

std::shared_ptr<peg::Grammar> initOpenMPParser() {
  auto g = std::make_shared<peg::Grammar>();
  init_omp_rules(*g);
  init_omp_actions(*g);
  for (auto &x : *g) {
    auto v = peg::LinkReferences(*g, x.second.params);
    x.second.accept(v);
  }
  (*g)["pragma"].enablePackratParsing = true;
  return g;
}

std::vector<CallExpr::Arg> parseOpenMP(Cache *cache, const std::string &code,
                                       const codon::SrcInfo &loc) {
  if (!ompGrammar)
    ompGrammar = initOpenMPParser();

  std::vector<std::tuple<size_t, size_t, std::string>> errors;
  auto log = [&](size_t line, size_t col, const std::string &msg, const std::string &) {
    errors.emplace_back(line, col, msg);
  };
  std::vector<CallExpr::Arg> result;
  auto ctx = std::make_any<ParseContext>(cache, 0, 0, 0);
  auto r = (*ompGrammar)["pragma"].parse_and_get_value(code.c_str(), code.size(), ctx,
                                                       result, "", log);
  auto ret = r.ret && r.len == code.size();
  if (!ret)
    r.error_info.output_log(log, code.c_str(), code.size());
  exc::ParserException ex;
  if (!errors.empty()) {
    ex.track(fmt::format("openmp {}", std::get<2>(errors[0])), loc);
    throw ex;
  }
  return result;
}

} // namespace codon::ast
