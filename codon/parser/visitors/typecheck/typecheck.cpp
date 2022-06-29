#include "typecheck.h"

#include <memory>
#include <utility>
#include <vector>

#include "codon/parser/ast.h"
#include "codon/parser/common.h"
#include "codon/parser/visitors/simplify/ctx.h"
#include "codon/parser/visitors/typecheck/ctx.h"
#include "codon/util/fmt/format.h"

using fmt::format;

namespace codon {
namespace ast {

using namespace types;

TypecheckVisitor::TypecheckVisitor(std::shared_ptr<TypeContext> ctx,
                                   const std::shared_ptr<std::vector<StmtPtr>> &stmts)
    : ctx(std::move(ctx)) {
  prependStmts = stmts ? stmts : std::make_shared<std::vector<StmtPtr>>();
}

StmtPtr TypecheckVisitor::apply(Cache *cache, StmtPtr stmts) {
  if (!cache->typeCtx) {
    auto ctx = std::make_shared<TypeContext>(cache);
    cache->typeCtx = ctx;
  }
  TypecheckVisitor v(cache->typeCtx);
  auto infer = v.inferTypes(stmts->clone(), true, "<top>");
  return std::move(infer.second);
}

TypePtr TypecheckVisitor::unify(TypePtr &a, const TypePtr &b, bool undoOnSuccess) {
  if (!a)
    return a = b;
  seqassert(b, "rhs is nullptr");
  types::Type::Unification undo;
  undo.realizator = this;
  if (a->unify(b.get(), &undo) >= 0) {
    if (undoOnSuccess)
      undo.undo();
    return a;
  } else {
    undo.undo();
  }
  if (!undoOnSuccess)
    a->unify(b.get(), &undo);
  error("cannot unify {} and {}", a->toString(), b->toString());
  return nullptr;
}

/**************************************************************************************/

ExprPtr TypecheckVisitor::transform(ExprPtr &expr) { return transform(expr, false); }

ExprPtr TypecheckVisitor::transform(ExprPtr &expr, bool allowTypes) {
  if (!expr)
    return nullptr;
  auto typ = expr->type;
  if (!expr->done) {
    TypecheckVisitor v(ctx, prependStmts);
    v.setSrcInfo(expr->getSrcInfo());
    ctx->pushSrcInfo(expr->getSrcInfo());
    expr->accept(v);
    ctx->popSrcInfo();
    if (v.resultExpr) {
      v.resultExpr->attributes |= expr->attributes;
      expr = v.resultExpr;
    }
    seqassert(expr->type, "type not set for {}", expr->toString());
    unify(typ, expr->type);
    if (expr->done)
      ctx->changedNodes++;
  }
  if (auto rt = realize(typ))
    unify(typ, rt);
  return expr;
}

ExprPtr TypecheckVisitor::transformType(ExprPtr &expr) {
  if (expr && expr->getNone()) {
    expr = N<IdExpr>(expr->getSrcInfo(), "NoneType");
    expr->markType();
  }
  expr = transform(expr, true);
  if (expr) {
    TypePtr t = nullptr;
    if (!expr->isType()) {
      if (expr->isStatic())
        t = std::make_shared<StaticType>(expr, ctx);
      else
        error("expected type expression");
    } else {
      t = ctx->instantiate(expr.get(), expr->getType());
    }
    expr->setType(t);
  }
  return expr;
}

void TypecheckVisitor::defaultVisit(Expr *e) {
  seqassert(false, "unexpected AST node {}", e->toString());
}

StmtPtr TypecheckVisitor::transform(StmtPtr &stmt) {
  if (!stmt || stmt->done)
    return stmt;
  TypecheckVisitor v(ctx);
  v.setSrcInfo(stmt->getSrcInfo());
  auto oldAge = ctx->age;
  stmt->age = ctx->age = std::max(stmt->age, oldAge);
  ctx->pushSrcInfo(stmt->getSrcInfo());
  stmt->accept(v);
  ctx->popSrcInfo();
  ctx->age = oldAge;
  if (v.resultStmt)
    stmt = v.resultStmt;
  if (!v.prependStmts->empty()) {
    if (stmt)
      v.prependStmts->push_back(stmt);
    bool done = true;
    for (auto &s : *(v.prependStmts))
      done &= s->done;
    stmt = N<SuiteStmt>(*v.prependStmts);
    stmt->done = done;
  }
  if (stmt->done)
    ctx->changedNodes++;
  return stmt;
}

void TypecheckVisitor::defaultVisit(Stmt *s) {
  seqassert(false, "unexpected AST node {}", s->toString());
}

/**************************************************************************************/

void TypecheckVisitor::visit(StarExpr *) { error("cannot use star-expression"); }

void TypecheckVisitor::visit(KeywordStarExpr *) { error("cannot use star-expression"); }

void TypecheckVisitor::visit(EllipsisExpr *expr) {
  unify(expr->type, ctx->addUnbound(expr, ctx->typecheckLevel));
}

void TypecheckVisitor::visit(StmtExpr *expr) {
  expr->done = true;
  for (auto &s : expr->stmts) {
    s = transform(s);
    expr->done &= s->done;
  }
  expr->expr = transform(expr->expr);
  unify(expr->type, expr->expr->type);
  expr->done &= expr->expr->done;
}

void TypecheckVisitor::visit(SuiteStmt *stmt) {
  std::vector<StmtPtr> stmts;
  stmt->done = true;
  for (auto &s : stmt->stmts) {
    if (ctx->returnEarly)
      break;
    if (auto t = transform(s)) {
      stmts.push_back(t);
      stmt->done &= stmts.back()->done;
    }
  }
  stmt->stmts = stmts;
}

void TypecheckVisitor::visit(ExprStmt *stmt) {
  stmt->expr = transform(stmt->expr, false);
  stmt->done = stmt->expr->done;
}

/**************************************************************************************/

void TypecheckVisitor::wrapOptionalIfNeeded(const TypePtr &targetType, ExprPtr &e) {
  if (!targetType)
    return;
  auto t1 = targetType->getClass();
  auto t2 = e->getType()->getClass();
  if (t1 && t2 && t1->name == TYPE_OPTIONAL && t1->name != t2->name)
    e = transform(N<CallExpr>(N<IdExpr>(TYPE_OPTIONAL), e));
}

void TypecheckVisitor::addFunctionGenerics(const FuncType *t) {
  for (auto p = t->funcParent; p;) {
    if (auto f = p->getFunc()) {
      for (auto &g : f->funcGenerics)
        ctx->add(TypecheckItem::Type, g.name, g.type);
      p = f->funcParent;
    } else {
      auto c = p->getClass();
      seqassert(c, "not a class: {}", p->toString());
      for (auto &g : c->generics)
        ctx->add(TypecheckItem::Type, g.name, g.type);
      for (auto &g : c->hiddenGenerics)
        ctx->add(TypecheckItem::Type, g.name, g.type);
      break;
    }
  }
  for (auto &g : t->funcGenerics)
    ctx->add(TypecheckItem::Type, g.name, g.type);
}

std::string TypecheckVisitor::generatePartialStub(const std::vector<char> &mask,
                                                  types::FuncType *fn) {
  std::string strMask(mask.size(), '1');
  int tupleSize = 0, genericSize = 0;
  for (int i = 0; i < mask.size(); i++)
    if (!mask[i])
      strMask[i] = '0';
    else if (fn->ast->args[i].status == Param::Normal)
      tupleSize++;
    else
      genericSize++;
  auto typeName = format(TYPE_PARTIAL "{}.{}", strMask, fn->toString());
  if (!ctx->find(typeName)) {
    ctx->cache->partials[typeName] = {fn->generalize(0)->getFunc(), mask};
    generateTuple(tupleSize + 2, typeName, {}, false);
  }
  return typeName;
}

ExprPtr TypecheckVisitor::partializeFunction(ExprPtr expr) {
  auto fn = expr->getType()->getFunc();
  seqassert(fn, "not a function: {}", expr->getType()->toString());
  std::vector<char> mask(fn->ast->args.size(), 0);
  for (int i = 0, j = 0; i < fn->ast->args.size(); i++)
    if (fn->ast->args[i].status == Param::Generic) {
      // TODO: better detection of user-provided args...?
      if (!fn->funcGenerics[j].type->getUnbound())
        mask[i] = 1;
      j++;
    }
  auto partialTypeName = generatePartialStub(mask, fn.get());
  std::string var = ctx->cache->getTemporaryVar("partial");
  auto kwName = generateTuple(0, "KwTuple", {});
  ExprPtr call =
      N<StmtExpr>(N<AssignStmt>(N<IdExpr>(var),
                                N<CallExpr>(N<IdExpr>(partialTypeName), N<TupleExpr>(),
                                            N<CallExpr>(N<IdExpr>(kwName)))),
                  N<IdExpr>(var));
  call->setAttr(ExprAttr::Partial);
  call = transform(call, false);
  seqassert(call->type->getPartial(), "expected partial type");
  return call;
}

types::FuncTypePtr
TypecheckVisitor::findBestMethod(const Expr *expr, const std::string &member,
                                 const std::vector<types::TypePtr> &args) {
  std::vector<CallExpr::Arg> callArgs;
  for (auto &a : args) {
    callArgs.push_back({"", std::make_shared<NoneExpr>()}); // dummy expression
    callArgs.back().value->setType(a);
  }
  return findBestMethod(expr, member, callArgs);
}

types::FuncTypePtr
TypecheckVisitor::findBestMethod(const Expr *expr, const std::string &member,
                                 const std::vector<CallExpr::Arg> &args) {
  auto typ = expr->getType()->getClass();
  seqassert(typ, "not a class");
  auto methods = ctx->findMethod(typ->name, member, false);
  auto m = findMatchingMethods(typ.get(), methods, args);
  return m.empty() ? nullptr : m[0];
}

types::FuncTypePtr
TypecheckVisitor::findBestMethod(const std::string &fn,
                                 const std::vector<CallExpr::Arg> &args) {
  std::vector<types::FuncTypePtr> methods;
  for (auto &m : ctx->cache->overloads[fn])
    if (!endswith(m.name, ":dispatch"))
      methods.push_back(ctx->cache->functions[m.name].type);
  std::reverse(methods.begin(), methods.end());
  auto m = findMatchingMethods(nullptr, methods, args);
  return m.empty() ? nullptr : m[0];
}

std::vector<types::FuncTypePtr>
TypecheckVisitor::findSuperMethods(const types::FuncTypePtr &func) {
  if (func->ast->attributes.parentClass.empty() ||
      endswith(func->ast->name, ":dispatch"))
    return {};
  auto p = ctx->find(func->ast->attributes.parentClass)->type;
  if (!p || !p->getClass())
    return {};

  auto methodName = ctx->cache->reverseIdentifierLookup[func->ast->name];
  auto m = ctx->cache->classes.find(p->getClass()->name);
  std::vector<types::FuncTypePtr> result;
  if (m != ctx->cache->classes.end()) {
    auto t = m->second.methods.find(methodName);
    if (t != m->second.methods.end()) {
      for (auto &m : ctx->cache->overloads[t->second]) {
        if (endswith(m.name, ":dispatch"))
          continue;
        if (m.name == func->ast->name)
          break;
        result.emplace_back(ctx->cache->functions[m.name].type);
      }
    }
  }
  std::reverse(result.begin(), result.end());
  return result;
}

std::vector<types::FuncTypePtr>
TypecheckVisitor::findMatchingMethods(types::ClassType *typ,
                                      const std::vector<types::FuncTypePtr> &methods,
                                      const std::vector<CallExpr::Arg> &args) {
  // Pick the last method that accepts the given arguments.
  std::vector<types::FuncTypePtr> results;
  for (int mi = 0; mi < methods.size(); mi++) {
    auto m = ctx->instantiate(nullptr, methods[mi], typ, false)->getFunc();
    std::vector<types::TypePtr> reordered;
    auto score = ctx->reorderNamedArgs(
        m.get(), args,
        [&](int s, int k, const std::vector<std::vector<int>> &slots, bool _) {
          for (int si = 0; si < slots.size(); si++) {
            if (m->ast->args[si].status == Param::Generic) {
              // Ignore type arguments
            } else if (si == s || si == k || slots[si].size() != 1) {
              // Ignore *args, *kwargs and default arguments
              reordered.emplace_back(nullptr);
            } else {
              reordered.emplace_back(args[slots[si][0]].value->type);
            }
          }
          return 0;
        },
        [](const std::string &) { return -1; });
    for (int ai = 0, mi = 0, gi = 0; score != -1 && ai < reordered.size(); ai++) {
      auto expectTyp = m->ast->args[ai].status == Param::Normal
                           ? m->getArgTypes()[mi++]
                           : m->funcGenerics[gi++].type;
      auto argType = reordered[ai];
      if (!argType)
        continue;
      try {
        ExprPtr dummy = std::make_shared<IdExpr>("");
        dummy->type = argType;
        dummy->done = true;
        wrapExpr(dummy, expectTyp, m, /*undoOnSuccess*/ true);
      } catch (const exc::ParserException &) {
        score = -1;
      }
    }
    if (score != -1) {
      results.push_back(methods[mi]);
    }
  }
  return results;
}

bool TypecheckVisitor::wrapExpr(ExprPtr &expr, TypePtr expectedType,
                                const FuncTypePtr &callee, bool undoOnSuccess) {
  auto expectedClass = expectedType->getClass();
  auto exprClass = expr->getType()->getClass();
  if (callee && expr->isType())
    expr = transform(N<CallExpr>(expr, N<EllipsisExpr>()));

  std::unordered_set<std::string> hints = {"Generator", "float", TYPE_OPTIONAL};
  if (!exprClass && expectedClass && in(hints, expectedClass->name)) {
    return false; // argument type not yet known.
  } else if (expectedClass && expectedClass->name == "Generator" &&
             exprClass->name != expectedClass->name && !expr->getEllipsis()) {
    // Note: do not do this in pipelines (TODO: why?).
    expr = transform(N<CallExpr>(N<DotExpr>(expr, "__iter__")));
  } else if (expectedClass && expectedClass->name == "float" &&
             exprClass->name == "int") {
    expr = transform(N<CallExpr>(N<IdExpr>("float"), expr));
  } else if (expectedClass && expectedClass->name == TYPE_OPTIONAL &&
             exprClass->name != expectedClass->name) {
    expr = transform(N<CallExpr>(N<IdExpr>(TYPE_OPTIONAL), expr));
  } else if (expectedClass && exprClass && exprClass->name == TYPE_OPTIONAL &&
             exprClass->name != expectedClass->name) { // unwrap optional
    expr = transform(N<CallExpr>(N<IdExpr>(FN_UNWRAP), expr));
  } else if (callee && exprClass && expr->type->getFunc() &&
             !(expectedClass && expectedClass->name == "Function")) {
    // Case 7: wrap raw Seq functions into Partial(...) call for easy realization.
    expr = partializeFunction(expr);
  }

  // Special case:
  unify(expr->type, expectedType, undoOnSuccess);
  return true;
}

int64_t TypecheckVisitor::translateIndex(int64_t idx, int64_t len, bool clamp) {
  if (idx < 0)
    idx += len;
  if (clamp) {
    if (idx < 0)
      idx = 0;
    if (idx > len)
      idx = len;
  } else if (idx < 0 || idx >= len) {
    error("tuple index {} out of bounds (len: {})", idx, len);
  }
  return idx;
}

int64_t TypecheckVisitor::sliceAdjustIndices(int64_t length, int64_t *start,
                                             int64_t *stop, int64_t step) {
  if (step == 0)
    error("slice step cannot be 0");

  if (*start < 0) {
    *start += length;
    if (*start < 0) {
      *start = (step < 0) ? -1 : 0;
    }
  } else if (*start >= length) {
    *start = (step < 0) ? length - 1 : length;
  }

  if (*stop < 0) {
    *stop += length;
    if (*stop < 0) {
      *stop = (step < 0) ? -1 : 0;
    }
  } else if (*stop >= length) {
    *stop = (step < 0) ? length - 1 : length;
  }

  if (step < 0) {
    if (*stop < *start) {
      return (*start - *stop - 1) / (-step) + 1;
    }
  } else {
    if (*start < *stop) {
      return (*stop - *start - 1) / step + 1;
    }
  }
  return 0;
}

std::shared_ptr<RecordType> TypecheckVisitor::getFuncTypeBase(int nargs) {
  auto baseType = ctx->instantiate(nullptr, ctx->find("Function")->type)->getRecord();
  auto argType =
      ctx->instantiate(nullptr, ctx->find(generateTuple(nargs))->type)->getRecord();
  unify(baseType->generics[0].type, argType);
  return baseType;
}

} // namespace ast
} // namespace codon
