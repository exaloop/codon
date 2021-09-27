/*
 * parser.cpp --- Seq AST parser.
 *
 * (c) Seq project. All rights reserved.
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 */

#include <chrono>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "parser/cache.h"
#include "parser/parser.h"
#include "parser/peg/peg.h"
#include "parser/visitors/doc/doc.h"
#include "parser/visitors/format/format.h"
#include "parser/visitors/simplify/simplify.h"
#include "parser/visitors/translate/translate.h"
#include "parser/visitors/typecheck/typecheck.h"
#include "sir/sir.h"
#include "util/fmt/format.h"

#include "sir/util/format.h"
#include <fstream>

int _ocaml_time = 0;
int _ll_time = 0;
int _level = 0;
int _dbg_level = 0;
bool _isTest = false;

namespace seq {

ir::Module *parse(const string &argv0, const string &file, const string &code,
                  bool isCode, int isTest, int startLine,
                  const std::unordered_map<std::string, std::string> &defines) {
  try {
    auto d = getenv("SEQ_DEBUG");
    if (d) {
      auto s = string(d);
      _dbg_level |= s.find('t') != string::npos ? (1 << 0) : 0; // time
      _dbg_level |= s.find('r') != string::npos ? (1 << 2) : 0; // realize
      _dbg_level |= s.find('T') != string::npos ? (1 << 4) : 0; // type-check
      _dbg_level |= s.find('L') != string::npos ? (1 << 5) : 0; // lexer
      _dbg_level |= s.find('i') != string::npos ? (1 << 6) : 0; // IR
      _dbg_level |= s.find('l') != string::npos ? (1 << 7) : 0; // User-level debugging
    }

    char abs[PATH_MAX + 1] = {'-', 0};
    if (file != "-")
      realpath(file.c_str(), abs);

    auto cache = make_shared<ast::Cache>(argv0);
    ast::StmtPtr codeStmt = isCode ? ast::parseCode(cache, abs, code, startLine)
                                   : ast::parseFile(cache, abs);
    if (_dbg_level) {
      auto fo = fopen("_dump.sexp", "w");
      fmt::print(fo, "{}\n", codeStmt->toString(0));
      fclose(fo);
    }

    using namespace std::chrono;
    cache->module0 = file;
    if (isTest)
      cache->testFlags = isTest;

    auto t = high_resolution_clock::now();

    auto transformed =
        ast::SimplifyVisitor::apply(cache, move(codeStmt), abs, defines, (isTest > 1));
    if (!isTest) {
      LOG_TIME("[T] ocaml = {:.1f}", _ocaml_time / 1000.0);
      LOG_TIME("[T] simplify = {:.1f}",
               (duration_cast<milliseconds>(high_resolution_clock::now() - t).count() -
                _ocaml_time) /
                   1000.0);
      if (_dbg_level) {
        auto fo = fopen("_dump_simplify.sexp", "w");
        fmt::print(fo, "{}\n", transformed->toString(0));
        fclose(fo);
        fo = fopen("_dump_simplify.seq", "w");
        fmt::print(fo, "{}", ast::FormatVisitor::apply(transformed, cache));
        fclose(fo);
      }
    }

    t = high_resolution_clock::now();
    auto typechecked = ast::TypecheckVisitor::apply(cache, move(transformed));
    if (!isTest) {
      LOG_TIME("[T] typecheck = {:.1f}",
               duration_cast<milliseconds>(high_resolution_clock::now() - t).count() /
                   1000.0);
      if (_dbg_level) {
        auto fo = fopen("_dump_typecheck.seq", "w");
        fmt::print(fo, "{}", ast::FormatVisitor::apply(typechecked, cache));
        fclose(fo);
        fo = fopen("_dump_typecheck.sexp", "w");
        fmt::print(fo, "{}\n", typechecked->toString(0));
        for (auto &f : cache->functions)
          for (auto &r : f.second.realizations)
            fmt::print(fo, "{}\n", r.second->ast->toString(0));
        fclose(fo);
      }
    }

    t = high_resolution_clock::now();
    auto *module = ast::TranslateVisitor::apply(cache, move(typechecked));
    module->setSrcInfo({abs, 0, 0, 0});

    if (!isTest)
      LOG_TIME("[T] translate   = {:.1f}",
               duration_cast<milliseconds>(high_resolution_clock::now() - t).count() /
                   1000.0);
    if (_dbg_level) {
      auto out = seq::ir::util::format(module);
      std::ofstream os("_dump_sir.lisp");
      os << out;
      os.close();
      os.close();
    }

    _isTest = isTest;
    return module;
  } catch (exc::ParserException &e) {
    for (int i = 0; i < e.messages.size(); i++)
      if (!e.messages[i].empty()) {
        if (isTest) {
          _level = 0;
          LOG("{}", e.messages[i]);
        } else {
          compilationError(e.messages[i], e.locations[i].file, e.locations[i].line,
                           e.locations[i].col, /*terminate=*/false);
        }
      }
    return nullptr;
  }
}

void generateDocstr(const string &argv0) {
  vector<string> files;
  string s;
  while (std::getline(std::cin, s))
    files.push_back(s);
  try {
    auto j = ast::DocVisitor::apply(argv0, files);
    fmt::print("{}\n", j->toString());
  } catch (exc::ParserException &e) {
    for (int i = 0; i < e.messages.size(); i++)
      if (!e.messages[i].empty()) {
        compilationError(e.messages[i], e.locations[i].file, e.locations[i].line,
                         e.locations[i].col, /*terminate=*/false);
      }
  }
}

} // namespace seq
