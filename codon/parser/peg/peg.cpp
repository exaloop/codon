#include "peg.h"

#include <any>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "codon/parser/ast.h"
#include "codon/parser/common.h"
#include "codon/parser/peg/rules.h"
#include "codon/parser/visitors/format/format.h"
#include "codon/util/cpp-peglib/peglib.h"

extern int _ocaml_time;

using namespace std;
namespace codon {
namespace ast {

static std::shared_ptr<peg::Grammar> grammar(nullptr);
static std::shared_ptr<peg::Grammar> ompGrammar(nullptr);

std::shared_ptr<peg::Grammar> initParser() {
  auto g = std::make_shared<peg::Grammar>();
  init_codon_rules(*g);
  init_codon_actions(*g);
  ~(*g)["NLP"] <= peg::usr([](const char *s, size_t n, peg::SemanticValues &, any &dt) {
    return any_cast<ParseContext &>(dt).parens ? 0 : (n >= 1 && s[0] == '\\' ? 1 : -1);
  });
  for (auto &x : *g) {
    auto v = peg::LinkReferences(*g, x.second.params);
    x.second.accept(v);
  }
  (*g)["program"].enablePackratParsing = true;
  for (auto &rule : std::vector<std::string>{
           "arguments", "slices", "genexp", "parentheses", "star_parens", "generics",
           "with_parens_item", "params", "from_as_parens", "from_params"}) {
    (*g)[rule].enter = [](const char *, size_t, any &dt) {
      any_cast<ParseContext &>(dt).parens++;
    };
    (*g)[rule.c_str()].leave = [](const char *, size_t, size_t, any &, any &dt) {
      any_cast<ParseContext &>(dt).parens--;
    };
  }
  return g;
}

template <typename T>
T parseCode(Cache *cache, const std::string &file, std::string code, int line_offset,
            int col_offset, const std::string &rule) {
  using namespace std::chrono;
  auto t = high_resolution_clock::now();

  // Initialize
  if (!grammar)
    grammar = initParser();

  std::vector<tuple<size_t, size_t, std::string>> errors;
  auto log = [&](size_t line, size_t col, const std::string &msg) {
    errors.push_back({line, col, msg});
  };
  T result = nullptr;
  auto ctx = make_any<ParseContext>(cache, 0, line_offset, col_offset);
  auto r = (*grammar)[rule].parse_and_get_value(code.c_str(), code.size(), ctx, result,
                                                file.c_str(), log);
  auto ret = r.ret && r.len == code.size();
  if (!ret)
    r.error_info.output_log(log, code.c_str(), code.size());
  exc::ParserException ex;
  if (!errors.empty()) {
    for (auto &e : errors)
      ex.track(fmt::format("{}", get<2>(e)), SrcInfo(file, get<0>(e), get<1>(e), 0));
    throw ex;
    return nullptr;
  }
  _ocaml_time += duration_cast<milliseconds>(high_resolution_clock::now() - t).count();
  return result;
}

StmtPtr parseCode(Cache *cache, const std::string &file, const std::string &code,
                  int line_offset) {
  return parseCode<StmtPtr>(cache, file, code + "\n", line_offset, 0, "program");
}

ExprPtr parseExpr(Cache *cache, const std::string &code, const codon::SrcInfo &offset) {
  return parseCode<ExprPtr>(cache, offset.file, code, offset.line, offset.col,
                            "fstring");
}

StmtPtr parseFile(Cache *cache, const std::string &file) {
  std::vector<std::string> lines;
  std::string code;
  if (file == "-") {
    for (std::string line; getline(cin, line);) {
      lines.push_back(line);
      code += line + "\n";
    }
  } else {
    ifstream fin(file);
    if (!fin)
      error(fmt::format("cannot open {}", file).c_str());
    for (std::string line; getline(fin, line);) {
      lines.push_back(line);
      code += line + "\n";
    }
    fin.close();
  }

  cache->imports[file].content = lines;
  auto result = parseCode(cache, file, code);
  // LOG("peg/{} :=  {}", file, result ? result->toString(0) : "<nullptr>");
  // throw;
  // LOG("fmt := {}", FormatVisitor::apply(result));
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

  std::vector<tuple<size_t, size_t, std::string>> errors;
  auto log = [&](size_t line, size_t col, const std::string &msg) {
    errors.push_back({line, col, msg});
  };
  std::vector<CallExpr::Arg> result;
  auto ctx = make_any<ParseContext>(cache, 0, 0, 0);
  auto r = (*ompGrammar)["pragma"].parse_and_get_value(code.c_str(), code.size(), ctx,
                                                       result, "", log);
  auto ret = r.ret && r.len == code.size();
  if (!ret)
    r.error_info.output_log(log, code.c_str(), code.size());
  exc::ParserException ex;
  if (!errors.empty()) {
    ex.track(fmt::format("openmp {}", get<2>(errors[0])), loc);
    throw ex;
  }
  return result;
}

} // namespace ast
} // namespace codon
