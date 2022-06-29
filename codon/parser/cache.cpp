#include "cache.h"

#include <chrono>
#include <string>
#include <vector>

#include "codon/parser/common.h"
#include "codon/parser/visitors/translate/translate.h"
#include "codon/parser/visitors/typecheck/ctx.h"
#include "codon/parser/visitors/typecheck/typecheck.h"

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
  seqassertn(e->type, "not a class");
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
  if (args.size() != type->getArgTypes().size() + 1)
    return nullptr;
  types::Type::Unification undo;
  if (type->getRetType()->unify(args[0].get(), &undo) < 0) {
    undo.undo();
    return nullptr;
  }
  for (int gi = 1; gi < args.size(); gi++) {
    types::Type::Unification undo;
    if (type->getArgTypes()[gi - 1]->unify(args[gi].get(), &undo) < 0) {
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
  int oldAge = typeCtx->age;
  typeCtx->age = 99999;
  auto tv = TypecheckVisitor(typeCtx);
  ir::Func *f = nullptr;
  if (auto rtv = tv.realize(type)) {
    auto pr = pendingRealizations; // copy it as it might be modified
    for (auto &fn : pr)
      TranslateVisitor(codegenCtx).transform(functions[fn.first].ast->clone());
    f = functions[rtv->getFunc()->ast->name].realizations[rtv->realizedName()]->ir;
  }
  typeCtx->age = oldAge;
  return f;
}

ir::types::Type *Cache::makeTuple(const std::vector<types::TypePtr> &types) {
  auto tv = TypecheckVisitor(typeCtx);
  auto name = tv.generateTuple(types.size());
  auto t = typeCtx->find(name);
  seqassertn(t && t->type, "cannot find {}", name);
  return realizeType(t->type->getClass(), types);
}

ir::types::Type *Cache::makeFunction(const std::vector<types::TypePtr> &types) {
  auto tv = TypecheckVisitor(typeCtx);
  seqassertn(!types.empty(), "types must have at least one argument");

  auto tup = tv.generateTuple(types.size() - 1);
  auto ret = types[0];
  auto argType = typeCtx->instantiateGeneric(
      nullptr, typeCtx->find(tup)->type,
      std::vector<types::TypePtr>(types.begin() + 1, types.end()));
  auto t = typeCtx->find("Function");
  seqassertn(t && t->type, "cannot find 'Function'");
  return realizeType(t->type->getClass(), {argType, ret});
}

} // namespace ast
} // namespace codon
