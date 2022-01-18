#include "cache.h"

#include <chrono>
#include <string>
#include <vector>

#include "codon/parser/common.h"
#include "codon/parser/visitors/translate/translate.h"
#include "codon/parser/visitors/typecheck/typecheck.h"
#include "codon/parser/visitors/typecheck/typecheck_ctx.h"

namespace codon {
namespace ast {

Cache::Cache(std::string argv0)
    : generatedSrcInfoCount(0), unboundCount(0), varCount(0), age(0), testFlags(0),
      argv0(move(argv0)), module(nullptr), typeCtx(nullptr), codegenCtx(nullptr),
      isJit(false), jitCell(0) {}

std::string Cache::getTemporaryVar(const std::string &prefix, char sigil) {
  return fmt::format("{}{}_{}", sigil ? fmt::format("{}_", sigil) : "", prefix,
                     ++varCount);
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

types::ClassTypePtr Cache::findClass(const std::string &name) const {
  auto f = typeCtx->find(name);
  if (f && f->kind == TypecheckItem::Type)
    return f->type->getClass();
  return nullptr;
}

types::FuncTypePtr Cache::findFunction(const std::string &name) const {
  auto f = typeCtx->find(name);
  if (f && f->type && f->kind == TypecheckItem::Func)
    return f->type->getFunc();
  f = typeCtx->find(name + ":0");
  if (f && f->type && f->kind == TypecheckItem::Func)
    return f->type->getFunc();
  return nullptr;
}

types::FuncTypePtr Cache::findMethod(types::ClassType *typ, const std::string &member,
                                     const std::vector<types::TypePtr> &args) {
  auto e = std::make_shared<IdExpr>(typ->name);
  e->type = typ->getClass();
  seqassert(e->type, "not a class");
  int oldAge = typeCtx->age;
  typeCtx->age = 99999;
  auto f = TypecheckVisitor(typeCtx).findBestMethod(e.get(), member, args);
  typeCtx->age = oldAge;
  return f;
}

ir::types::Type *Cache::realizeType(types::ClassTypePtr type,
                                    const std::vector<types::TypePtr> &generics) {
  auto e = std::make_shared<IdExpr>(type->name);
  e->type = type;
  type = typeCtx->instantiateGeneric(e.get(), type, generics)->getClass();
  auto tv = TypecheckVisitor(typeCtx);
  if (auto rtv = tv.realize(type)) {
    return classes[rtv->getClass()->name]
        .realizations[rtv->getClass()->realizedTypeName()]
        ->ir;
  }
  return nullptr;
}

ir::Func *Cache::realizeFunction(types::FuncTypePtr type,
                                 const std::vector<types::TypePtr> &args,
                                 const std::vector<types::TypePtr> &generics,
                                 types::ClassTypePtr parentClass) {
  auto e = std::make_shared<IdExpr>(type->ast->name);
  e->type = type;
  type = typeCtx->instantiate(e.get(), type, parentClass.get(), false)->getFunc();
  if (args.size() != type->args.size())
    return nullptr;
  for (int gi = 0; gi < args.size(); gi++) {
    types::Type::Unification undo;
    if (type->args[gi]->unify(args[gi].get(), &undo) < 0) {
      undo.undo();
      return nullptr;
    }
  }
  if (!generics.empty()) {
    if (generics.size() != type->funcGenerics.size())
      return nullptr;
    for (int gi = 0; gi < generics.size(); gi++) {
      types::Type::Unification undo;
      if (type->funcGenerics[gi].type->unify(generics[gi].get(), &undo) < 0) {
        undo.undo();
        return nullptr;
      }
    }
  }
  auto tv = TypecheckVisitor(typeCtx);
  if (auto rtv = tv.realize(type)) {
    auto pr = pendingRealizations; // copy it as it might be modified
    for (auto &fn : pr)
      TranslateVisitor(codegenCtx).transform(functions[fn.first].ast->clone());
    return functions[rtv->getFunc()->ast->name].realizations[rtv->realizedName()]->ir;
  }
  return nullptr;
}

ir::types::Type *Cache::makeTuple(const std::vector<types::TypePtr> &types) {
  auto tv = TypecheckVisitor(typeCtx);
  auto name = tv.generateTupleStub(types.size());
  auto t = typeCtx->find(name);
  seqassert(t && t->type, "cannot find {}", name);
  return realizeType(t->type->getClass(), types);
}

ir::types::Type *Cache::makeFunction(const std::vector<types::TypePtr> &types) {
  auto tv = TypecheckVisitor(typeCtx);
  seqassert(!types.empty(), "types must have at least one argument");
  auto name = tv.generateFunctionStub(types.size() - 1);
  auto t = typeCtx->find(name);
  seqassert(t && t->type, "cannot find {}", name);
  return realizeType(t->type->getClass(), types);
}

} // namespace ast
} // namespace codon
