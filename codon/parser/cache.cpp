// Copyright (C) 2022-2025 Exaloop Inc. <https://exaloop.io>

#include "cache.h"

#include <chrono>
#include <string>
#include <vector>

#include "codon/cir/pyextension.h"
#include "codon/cir/util/irtools.h"
#include "codon/parser/common.h"
#include "codon/parser/peg/peg.h"
#include "codon/parser/visitors/translate/translate.h"
#include "codon/parser/visitors/typecheck/ctx.h"
#include "codon/parser/visitors/typecheck/typecheck.h"

namespace codon::ast {

Cache::Cache(std::string argv0) : argv0(std::move(argv0)) {
  this->_nodes = new std::vector<std::unique_ptr<ast::ASTNode>>();
  typeCtx = std::make_shared<TypeContext>(this, ".root");
}

std::string Cache::getTemporaryVar(const std::string &prefix, char sigil) {
  auto n = fmt::format("{}{}_{}", sigil ? fmt::format("{}_", sigil) : "", prefix,
                       ++varCount);
  return n;
}

std::string Cache::rev(const std::string &s) {
  auto i = reverseIdentifierLookup.find(s);
  if (i != reverseIdentifierLookup.end())
    return i->second;
  seqassertn(false, "'{}' has no non-canonical name", s);
  return "";
}

SrcInfo Cache::generateSrcInfo() {
  return {FILE_GENERATED, generatedSrcInfoCount, generatedSrcInfoCount++, 0};
}

std::string Cache::getContent(const SrcInfo &info) {
  auto i = imports.find(info.file);
  if (i == imports.end())
    return "";
  int line = info.line - 1;
  if (line < 0 || line >= i->second.content.size())
    return "";
  auto s = i->second.content[line];
  int col = info.col - 1;
  if (col < 0 || col >= s.size())
    return "";
  int len = info.len;
  return s.substr(col, len);
}

Cache::Class *Cache::getClass(types::ClassType *type) {
  auto name = type->name;
  return in(classes, name);
}

std::string Cache::getMethod(types::ClassType *typ, const std::string &member) {
  if (auto cls = getClass(typ)) {
    if (auto t = in(cls->methods, member))
      return *t;
  }
  seqassertn(false, "cannot find '{}' in '{}'", member, typ->toString());
  return "";
}

types::ClassType *Cache::findClass(const std::string &name) const {
  auto f = typeCtx->find(name);
  if (f && f->isType())
    return f->getType()->getClass()->generics[0].getType()->getClass();
  return nullptr;
}

types::FuncType *Cache::findFunction(const std::string &name) const {
  auto f = typeCtx->find(name);
  if (f && f->type && f->isFunc())
    return f->type->getFunc();
  f = typeCtx->find(name + ":0");
  if (f && f->type && f->isFunc())
    return f->type->getFunc();
  return nullptr;
}

types::FuncType *Cache::findMethod(types::ClassType *typ, const std::string &member,
                                   const std::vector<types::Type *> &args) {
  auto f = TypecheckVisitor(typeCtx).findBestMethod(typ, member, args);
  return f;
}

ir::types::Type *Cache::realizeType(types::ClassType *type,
                                    const std::vector<types::TypePtr> &generics) {
  auto tv = TypecheckVisitor(typeCtx);
  if (auto rtv = tv.realize(tv.instantiateType(type, castVectorPtr(generics)))) {
    return classes[rtv->getClass()->name]
        .realizations[rtv->getClass()->realizedName()]
        ->ir;
  }
  return nullptr;
}

ir::Func *Cache::realizeFunction(types::FuncType *type,
                                 const std::vector<types::TypePtr> &args,
                                 const std::vector<types::TypePtr> &generics,
                                 types::ClassType *parentClass) {
  auto tv = TypecheckVisitor(typeCtx);
  auto t = tv.instantiateType(type, parentClass);
  if (args.size() != t->size() + 1)
    return nullptr;
  types::Type::Unification undo;
  if (t->getRetType()->unify(args[0].get(), &undo) < 0) {
    undo.undo();
    return nullptr;
  }
  for (int gi = 1; gi < args.size(); gi++) {
    undo = types::Type::Unification();
    if ((*t)[gi - 1]->unify(args[gi].get(), &undo) < 0) {
      undo.undo();
      return nullptr;
    }
  }
  if (!generics.empty()) {
    if (generics.size() != t->funcGenerics.size())
      return nullptr;
    for (int gi = 0; gi < generics.size(); gi++) {
      undo = types::Type::Unification();
      if (t->funcGenerics[gi].type->unify(generics[gi].get(), &undo) < 0) {
        undo.undo();
        return nullptr;
      }
    }
  }
  ir::Func *f = nullptr;
  if (auto rtv = tv.realize(t.get())) {
    auto pr = pendingRealizations; // copy it as it might be modified
    for (auto &fn : pr)
      TranslateVisitor(codegenCtx).translateStmts(clone(functions[fn.first].ast));
    f = functions[rtv->getFunc()->ast->getName()].realizations[rtv->realizedName()]->ir;
  }
  return f;
}

ir::types::Type *Cache::makeTuple(const std::vector<types::TypePtr> &types) {
  auto tv = TypecheckVisitor(typeCtx);
  auto t = tv.instantiateType(tv.generateTuple(types.size()), castVectorPtr(types));
  return realizeType(t->getClass(), types);
}

ir::types::Type *Cache::makeFunction(const std::vector<types::TypePtr> &types) {
  auto tv = TypecheckVisitor(typeCtx);
  seqassertn(!types.empty(), "types must have at least one argument");

  std::vector<types::Type *> tt;
  for (size_t i = 1; i < types.size(); i++)
    tt.emplace_back(types[i].get());
  const auto &ret = types[0];
  auto argType = tv.instantiateType(tv.generateTuple(types.size() - 1), tt);
  auto ft = realizeType(tv.getStdLibType("Function")->getClass(), {argType, ret});
  return ft;
}

ir::types::Type *Cache::makeUnion(const std::vector<types::TypePtr> &types) {
  auto tv = TypecheckVisitor(typeCtx);
  auto argType =
      tv.instantiateType(tv.generateTuple(types.size()), castVectorPtr(types));
  return realizeType(tv.getStdLibType("Union")->getClass(), {argType});
}

void Cache::parseCode(const std::string &code) {
  auto nodeOrErr = ast::parseCode(this, "<internal>", code, /*startLine=*/0);
  if (nodeOrErr)
    throw exc::ParserException(nodeOrErr.takeError());
  auto sctx = imports[MAIN_IMPORT].ctx;
  auto node = ast::TypecheckVisitor::apply(sctx, *nodeOrErr);
  for (auto &[name, p] : globals)
    if (p.first && !p.second) {
      p.second = name == VAR_ARGV ? codegenCtx->getModule()->getArgVar()
                                  : codegenCtx->getModule()->N<ir::Var>(
                                        SrcInfo(), nullptr, true, false, name);
      codegenCtx->add(ast::TranslateItem::Var, name, p.second);
    }
  ast::TranslateVisitor(codegenCtx).translateStmts(node);
}

std::vector<std::shared_ptr<types::ClassType>>
Cache::mergeC3(std::vector<std::vector<types::TypePtr>> &seqs) {
  // Reference: https://www.python.org/download/releases/2.3/mro/
  std::vector<std::shared_ptr<types::ClassType>> result;
  for (size_t i = 0;; i++) {
    bool found = false;
    std::shared_ptr<types::ClassType> cand = nullptr;
    for (auto &seq : seqs) {
      if (seq.empty())
        continue;
      found = true;
      bool nothead = false;
      for (auto &s : seqs)
        if (!s.empty()) {
          bool in = false;
          for (size_t j = 1; j < s.size(); j++) {
            if ((in |= (seq[0]->is(s[j]->getClass()->name))))
              break;
          }
          if (in) {
            nothead = true;
            break;
          }
        }
      if (!nothead) {
        cand = std::dynamic_pointer_cast<types::ClassType>(seq[0]);
        break;
      }
    }
    if (!found)
      return result;
    if (!cand)
      return {};
    result.push_back(cand);
    for (auto &s : seqs)
      if (!s.empty() && cand->is(s[0]->getClass()->name)) {
        s.erase(s.begin());
      }
  }
  return result;
}

/**
 * Generate Python bindings for Cython-like access.
 */
void Cache::populatePythonModule() {
  using namespace ast;

  if (!pythonExt)
    return;
  if (!pyModule)
    pyModule = std::make_shared<ir::PyModule>();

  LOG_USER("[py] ====== module generation =======");
  auto tv = TypecheckVisitor(typeCtx);
  auto clss = classes; // needs copy as below fns can mutate this
  for (const auto &[cn, _] : clss) {
    auto py = tv.cythonizeClass(cn);
    if (!py.name.empty())
      pyModule->types.push_back(py);
  }

  // Handle __iternext__ wrappers
  for (auto &[cn, _] : classes[CYTHON_ITER].realizations) {
    auto py = tv.cythonizeIterator(cn);
    pyModule->types.push_back(py);
  }

  auto fns = functions; // needs copy as below fns can mutate this
  for (const auto &[fn, _] : fns) {
    auto py = tv.cythonizeFunction(fn);
    if (!py.name.empty())
      pyModule->functions.push_back(py);
  }

  // Handle pending realizations!
  auto pr = pendingRealizations; // copy it as it might be modified
  for (auto &fn : pr)
    TranslateVisitor(codegenCtx).translateStmts(clone(functions[fn.first].ast));
}

} // namespace codon::ast
