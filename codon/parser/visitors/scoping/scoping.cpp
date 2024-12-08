// Copyright (C) 2022-2023 Exaloop Inc. <https://exaloop.io>

#include <memory>
#include <utility>
#include <vector>

#include "codon/parser/ast.h"
#include "codon/parser/common.h"
#include "codon/parser/match.h"
#include "codon/parser/peg/peg.h"
#include "codon/parser/visitors/scoping/scoping.h"
#include <fmt/format.h>

#define CHECK(x)                                                                       \
  {                                                                                    \
    if (!(x))                                                                          \
      return;                                                                          \
  }
#define STOP_ERROR(...)                                                                \
  do {                                                                                 \
    addError(__VA_ARGS__);                                                           \
    return;                                                                            \
  } while (0)

using fmt::format;
using namespace codon::error;
using namespace codon::matcher;

namespace codon::ast {

llvm::Error ScopingVisitor::apply(Cache *cache, Stmt *s) {
  auto c = std::make_shared<ScopingVisitor::Context>();
  c->cache = cache;
  c->functionScope = nullptr;
  ScopingVisitor v;
  v.ctx = c;

  ConditionalBlock cb(c.get(), s, 0);
  if (!v.transform(s))
    return llvm::make_error<ParserErrorInfo>(v.errors);
  if (v.hasErrors())
    return llvm::make_error<ParserErrorInfo>(v.errors);
  v.processChildCaptures();
  return llvm::Error::success();
}

bool ScopingVisitor::transform(Expr *expr) {
  ScopingVisitor v(*this);
  if (expr) {
    v.setSrcInfo(expr->getSrcInfo());
    expr->accept(v);
    if (v.hasErrors())
      errors.append(v.errors);
    if (!canContinue())
      return false;
  }
  return true;
}

bool ScopingVisitor::transform(Stmt *stmt) {
  ScopingVisitor v(*this);
  if (stmt) {
    v.setSrcInfo(stmt->getSrcInfo());
    stmt->accept(v);
    if (v.hasErrors())
      errors.append(v.errors);
    if (!canContinue())
      return false;
  }
  return true;
}

bool ScopingVisitor::transformScope(Expr *e) {
  if (e) {
    ConditionalBlock c(ctx.get(), nullptr);
    return transform(e);
  }
  return true;
}

bool ScopingVisitor::transformScope(Stmt *s) {
  if (s) {
    ConditionalBlock c(ctx.get(), s);
    return transform(s);
  }
  return true;
}

bool ScopingVisitor::transformAdding(Expr *e, ASTNode *root) {
  if (cast<IndexExpr>(e)) {
    return transform(e);
  } else if (auto de = cast<DotExpr>(e)) {
    if (!transform(e))
      return false;
    if (!ctx->classDeduce.first.empty() &&
        match(de->getExpr(), M<IdExpr>(ctx->classDeduce.first)))
      ctx->classDeduce.second.insert(de->getMember());
    return true;
  } else if (cast<ListExpr>(e) || cast<TupleExpr>(e) || cast<IdExpr>(e)) {
    SetInScope s1(&(ctx->adding), true);
    SetInScope s2(&(ctx->root), root);
    return transform(e);
  } else {
    seqassert(e, "bad call to transformAdding");
    addError(Error::ASSIGN_INVALID, e);
    return false;
  }
}

void ScopingVisitor::visit(IdExpr *expr) {
  if (ctx->adding)
    ctx->root = expr;
  if (ctx->adding && ctx->tempScope)
    ctx->renames.back()[expr->getValue()] =
        ctx->cache->getTemporaryVar(expr->getValue());
  for (size_t i = ctx->renames.size(); i-- > 0;)
    if (auto v = in(ctx->renames[i], expr->getValue())) {
      expr->setValue(*v);
      break;
    }
  if (visitName(expr->getValue(), ctx->adding, ctx->root, expr->getSrcInfo()))
    expr->setAttribute(Attr::ExprDominatedUndefCheck);
}

void ScopingVisitor::visit(StringExpr *expr) {
  std::vector<StringExpr::String> exprs;
  for (auto &p : *expr) {
    if (p.prefix == "f" || p.prefix == "F") {
      /// Transform an F-string
      auto fstr = unpackFString(p.value);
      if (!canContinue())
        return;
      for (auto pf : fstr) {
        if (pf.prefix.empty() && !exprs.empty() && exprs.back().prefix.empty()) {
          exprs.back().value += pf.value;
        } else {
          exprs.emplace_back(pf);
        }
      }
    } else if (!p.prefix.empty()) {
      exprs.emplace_back(p);
    } else if (!exprs.empty() && exprs.back().prefix.empty()) {
      exprs.back().value += p.value;
    } else {
      exprs.emplace_back(p);
    }
  }
  expr->strings = exprs;
}

/// Split a Python-like f-string into a list:
///   `f"foo {x+1} bar"` -> `["foo ", str(x+1), " bar"]
/// Supports "{x=}" specifier (that prints the raw expression as well):
///   `f"{x+1=}"` -> `["x+1=", str(x+1)]`
std::vector<StringExpr::String>
ScopingVisitor::unpackFString(const std::string &value) {
  // Strings to be concatenated
  std::vector<StringExpr::String> items;
  int braceCount = 0, braceStart = 0;
  for (int i = 0; i < value.size(); i++) {
    if (value[i] == '{') {
      if (braceStart < i)
        items.emplace_back(value.substr(braceStart, i - braceStart));
      if (!braceCount)
        braceStart = i + 1;
      braceCount++;
    } else if (value[i] == '}') {
      braceCount--;
      if (!braceCount) {
        std::string code = value.substr(braceStart, i - braceStart);

        auto offset = getSrcInfo();
        offset.col += i;
        items.emplace_back(code, "#f");
        items.back().setSrcInfo(offset);

        auto val = parseExpr(ctx->cache, code, offset);
        if (!val) {
          addError(val.takeError());
        } else {
          items.back().expr = val->first;
          if (!transform(items.back().expr))
            return items;
          items.back().format = val->second;
        }
      }
      braceStart = i + 1;
    }
  }
  if (braceCount > 0)
    addError(Error::STR_FSTRING_BALANCE_EXTRA, getSrcInfo());
  else if (braceCount < 0)
    addError(Error::STR_FSTRING_BALANCE_MISSING, getSrcInfo());
  if (braceStart != value.size())
    items.emplace_back(value.substr(braceStart, value.size() - braceStart));
  return items;
}

void ScopingVisitor::visit(GeneratorExpr *expr) {
  SetInScope s(&(ctx->tempScope), true);
  ctx->renames.emplace_back();
  CHECK(transform(expr->getFinalSuite()));
  ctx->renames.pop_back();
}

void ScopingVisitor::visit(IfExpr *expr) {
  CHECK(transform(expr->getCond()));
  CHECK(transformScope(expr->getIf()));
  CHECK(transformScope(expr->getElse()));
}

void ScopingVisitor::visit(BinaryExpr *expr) {
  CHECK(transform(expr->getLhs()));
  if (expr->getOp() == "&&" || expr->getOp() == "||") {
    CHECK(transformScope(expr->getRhs()));
  } else {
    CHECK(transform(expr->getRhs()));
  }
}

void ScopingVisitor::visit(AssignExpr *expr) {
  seqassert(cast<IdExpr>(expr->getVar()),
            "only simple assignment expression are supported");

  SetInScope s(&(ctx->tempScope), false);
  CHECK(transform(expr->getExpr()));
  CHECK(transformAdding(expr->getVar(), expr));
}

void ScopingVisitor::visit(LambdaExpr *expr) {
  auto c = std::make_shared<ScopingVisitor::Context>();
  c->cache = ctx->cache;
  FunctionStmt f("lambda", nullptr, {}, nullptr);
  c->functionScope = &f;
  c->renames = ctx->renames;
  ScopingVisitor v;
  c->scope.emplace_back(0, nullptr);
  v.ctx = c;
  for (const auto &a : *expr)
    v.visitName(a, true, expr, expr->getSrcInfo());
  c->scope.pop_back();

  SuiteStmt s;
  c->scope.emplace_back(0, &s);
  v.transform(expr->getExpr());
  v.processChildCaptures();
  c->scope.pop_back();
  if (v.hasErrors())
    errors.append(v.errors);
  if (!canContinue())
    return;

  auto b = std::make_unique<BindingsAttribute>();
  b->captures = c->captures;
  for (const auto &n : c->captures)
    ctx->childCaptures.insert(n);
  for (auto &[u, v] : c->map)
    b->bindings[u] = v.size();
  expr->setAttribute(Attr::Bindings, std::move(b));
}

// todo)) Globals/nonlocals cannot be shadowed in children scopes (as in Python)

void ScopingVisitor::visit(AssignStmt *stmt) {
  CHECK(transform(stmt->getRhs()));
  CHECK(transform(stmt->getTypeExpr()));
  CHECK(transformAdding(stmt->getLhs(), stmt));
}

void ScopingVisitor::visit(IfStmt *stmt) {
  CHECK(transform(stmt->getCond()));
  CHECK(transformScope(stmt->getIf()));
  CHECK(transformScope(stmt->getElse()));
}

void ScopingVisitor::visit(MatchStmt *stmt) {
  CHECK(transform(stmt->getExpr()));
  for (auto &m : *stmt) {
    CHECK(transform(m.getPattern()));
    CHECK(transform(m.getGuard()));
    CHECK(transformScope(m.getSuite()));
  }
}

void ScopingVisitor::visit(WhileStmt *stmt) {
  CHECK(transform(stmt->getCond()));

  std::unordered_set<std::string> seen;
  {
    ConditionalBlock c(ctx.get(), stmt->getSuite());
    ctx->scope.back().seenVars = std::make_unique<std::unordered_set<std::string>>();
    CHECK(transform(stmt->getSuite()));
    seen = *(ctx->scope.back().seenVars);
  }
  for (auto &var : seen)
    findDominatingBinding(var);

  CHECK(transformScope(stmt->getElse()));
}

void ScopingVisitor::visit(ForStmt *stmt) {
  CHECK(transform(stmt->getIter()));
  CHECK(transform(stmt->getDecorator()));
  for (auto &a : stmt->ompArgs)
    CHECK(transform(a.value));

  std::unordered_set<std::string> seen, seenDef;
  {
    ConditionalBlock c(ctx.get(), stmt->getSuite());

    ctx->scope.back().seenVars = std::make_unique<std::unordered_set<std::string>>();
    CHECK(transformAdding(stmt->getVar(), stmt));
    seenDef = *(ctx->scope.back().seenVars);

    ctx->scope.back().seenVars = std::make_unique<std::unordered_set<std::string>>();
    CHECK(transform(stmt->getSuite()));
    seen = *(ctx->scope.back().seenVars);
  }
  for (auto &var : seen)
    if (!in(seenDef, var))
      findDominatingBinding(var);

  CHECK(transformScope(stmt->getElse()));
}

void ScopingVisitor::visit(ImportStmt *stmt) {
  // Validate
  if (stmt->getFrom()) {
    Expr *e = stmt->getFrom();
    while (auto d = cast<DotExpr>(e))
      e = d->getExpr();
    if (!isId(stmt->getFrom(), "C") && !isId(stmt->getFrom(), "python")) {
      if (!cast<IdExpr>(e))
        STOP_ERROR(Error::IMPORT_IDENTIFIER, e);
      if (!stmt->getArgs().empty())
        STOP_ERROR(Error::IMPORT_FN, stmt->getArgs().front().getSrcInfo());
      if (stmt->getReturnType())
        STOP_ERROR(Error::IMPORT_FN, stmt->getReturnType());
      if (stmt->getWhat() && !cast<IdExpr>(stmt->getWhat()))
        STOP_ERROR(Error::IMPORT_IDENTIFIER, stmt->getWhat());
    }
    if (stmt->isCVar() && !stmt->getArgs().empty())
      STOP_ERROR(Error::IMPORT_FN, stmt->getArgs().front().getSrcInfo());
  }
  if (ctx->functionScope && stmt->getWhat() && isId(stmt->getWhat(), "*"))
    STOP_ERROR(error::Error::IMPORT_STAR, stmt);

  // dylib C imports
  if (stmt->getFrom() && isId(stmt->getFrom(), "C") && cast<DotExpr>(stmt->getWhat()))
    CHECK(transform(cast<DotExpr>(stmt->getWhat())->getExpr()));

  if (stmt->getAs().empty()) {
    if (stmt->getWhat()) {
      CHECK(transformAdding(stmt->getWhat(), stmt));
    } else {
      CHECK(transformAdding(stmt->getFrom(), stmt));
    }
  } else {
    visitName(stmt->getAs(), true, stmt, stmt->getSrcInfo());
  }
  for (const auto &a : stmt->getArgs()) {
    CHECK(transform(a.type));
    CHECK(transform(a.defaultValue));
  }
  CHECK(transform(stmt->getReturnType()));
}

void ScopingVisitor::visit(TryStmt *stmt) {
  CHECK(transformScope(stmt->getSuite()));
  for (auto *a : *stmt) {
    CHECK(transform(a->getException()));
    ConditionalBlock c(ctx.get(), a->getSuite());
    if (!a->getVar().empty()) {
      auto newName = ctx->cache->getTemporaryVar(a->getVar());
      ctx->renames.push_back({{a->getVar(), newName}});
      a->var = newName;
      visitName(a->getVar(), true, a, a->getException()->getSrcInfo());
    }
    CHECK(transform(a->getSuite()));
    if (!a->getVar().empty())
      ctx->renames.pop_back();
  }
  CHECK(transform(stmt->getFinally()));
}

void ScopingVisitor::visit(DelStmt *stmt) {
  /// TODO
  CHECK(transform(stmt->getExpr()));
}

/// Process `global` statements. Remove them upon completion.
void ScopingVisitor::visit(GlobalStmt *stmt) {
  if (!ctx->functionScope)
    STOP_ERROR(Error::FN_OUTSIDE_ERROR, stmt,
               stmt->isNonLocal() ? "nonlocal" : "global");
  if (in(ctx->map, stmt->getVar()) || in(ctx->captures, stmt->getVar()))
    STOP_ERROR(Error::FN_GLOBAL_ASSIGNED, stmt, stmt->getVar());

  visitName(stmt->getVar(), true, stmt, stmt->getSrcInfo());
  ctx->captures[stmt->getVar()] = stmt->isNonLocal()
                                      ? BindingsAttribute::CaptureType::Nonlocal
                                      : BindingsAttribute::CaptureType::Global;
}

void ScopingVisitor::visit(YieldStmt *stmt) {
  if (ctx->functionScope)
    ctx->functionScope->setAttribute(Attr::IsGenerator);
  CHECK(transform(stmt->getExpr()));
}

void ScopingVisitor::visit(YieldExpr *expr) {
  if (ctx->functionScope)
    ctx->functionScope->setAttribute(Attr::IsGenerator);
}

void ScopingVisitor::visit(FunctionStmt *stmt) {
  // Validate
  std::vector<Expr *> newDecorators;
  for (auto &d : stmt->getDecorators()) {
    if (isId(d, Attr::Attribute)) {
      if (stmt->getDecorators().size() != 1)
        STOP_ERROR(Error::FN_SINGLE_DECORATOR, stmt->getDecorators()[1],
                   Attr::Attribute);
      stmt->setAttribute(Attr::Attribute);
    } else if (isId(d, Attr::LLVM)) {
      stmt->setAttribute(Attr::LLVM);
    } else if (isId(d, Attr::Python)) {
      if (stmt->getDecorators().size() != 1)
        STOP_ERROR(Error::FN_SINGLE_DECORATOR, stmt->getDecorators()[1], Attr::Python);
      stmt->setAttribute(Attr::Python);
    } else if (isId(d, Attr::Internal)) {
      stmt->setAttribute(Attr::Internal);
    } else if (isId(d, Attr::HiddenFromUser)) {
      stmt->setAttribute(Attr::HiddenFromUser);
    } else if (isId(d, Attr::Atomic)) {
      stmt->setAttribute(Attr::Atomic);
    } else if (isId(d, Attr::Property)) {
      stmt->setAttribute(Attr::Property);
    } else if (isId(d, Attr::StaticMethod)) {
      stmt->setAttribute(Attr::StaticMethod);
    } else if (isId(d, Attr::ForceRealize)) {
      stmt->setAttribute(Attr::ForceRealize);
    } else if (isId(d, Attr::C)) {
      stmt->setAttribute(Attr::C);
    } else {
      newDecorators.emplace_back(d);
    }
  }
  if (stmt->hasAttribute(Attr::C)) {
    for (auto &a : *stmt) {
      if (a.getName().size() > 1 && a.getName()[0] == '*' && a.getName()[1] != '*')
        stmt->setAttribute(Attr::CVarArg);
    }
  }
  if (!stmt->empty() && !stmt->front().getType() && stmt->front().getName() == "self") {
    stmt->setAttribute(Attr::HasSelf);
  }
  stmt->setDecorators(newDecorators);
  if (!stmt->getReturn() &&
      (stmt->hasAttribute(Attr::LLVM) || stmt->hasAttribute(Attr::C)))
    STOP_ERROR(Error::FN_LLVM, getSrcInfo());
  // Set attributes
  std::unordered_set<std::string> seenArgs;
  bool defaultsStarted = false, hasStarArg = false, hasKwArg = false;
  for (size_t ia = 0; ia < stmt->size(); ia++) {
    auto &a = (*stmt)[ia];
    auto [stars, n] = a.getNameWithStars();
    if (stars == 2) {
      if (hasKwArg)
        STOP_ERROR(Error::FN_MULTIPLE_ARGS, a.getSrcInfo());
      if (a.getDefault())
        STOP_ERROR(Error::FN_DEFAULT_STARARG, a.getDefault());
      if (ia != stmt->size() - 1)
        STOP_ERROR(Error::FN_LAST_KWARG, a.getSrcInfo());

      hasKwArg = true;
    } else if (stars == 1) {
      if (hasStarArg)
        STOP_ERROR(Error::FN_MULTIPLE_ARGS, a.getSrcInfo());
      if (a.getDefault())
        STOP_ERROR(Error::FN_DEFAULT_STARARG, a.getDefault());
      hasStarArg = true;
    }
    if (in(seenArgs, n))
      STOP_ERROR(Error::FN_ARG_TWICE, a.getSrcInfo(), n);
    seenArgs.insert(n);
    if (!a.getDefault() && defaultsStarted && !stars && a.isValue())
      STOP_ERROR(Error::FN_DEFAULT, a.getSrcInfo(), n);
    defaultsStarted |= bool(a.getDefault());
    if (stmt->hasAttribute(Attr::C)) {
      if (a.getDefault())
        STOP_ERROR(Error::FN_C_DEFAULT, a.getDefault(), n);
      if (stars != 1 && !a.getType())
        STOP_ERROR(Error::FN_C_TYPE, a.getSrcInfo(), n);
    }
  }

  bool isOverload = false;
  for (auto &d : stmt->getDecorators())
    if (isId(d, "overload")) {
      isOverload = true;
    }
  if (!isOverload)
    visitName(stmt->getName(), true, stmt, stmt->getSrcInfo());

  auto c = std::make_shared<ScopingVisitor::Context>();
  c->cache = ctx->cache;
  c->functionScope = stmt;
  if (ctx->inClass && !stmt->empty())
    c->classDeduce = {stmt->front().getName(), {}};
  c->renames = ctx->renames;
  ScopingVisitor v;
  c->scope.emplace_back(0);
  v.ctx = c;
  v.visitName(stmt->getName(), true, stmt, stmt->getSrcInfo());
  for (const auto &a : *stmt) {
    auto [_, n] = a.getNameWithStars();
    v.visitName(n, true, stmt, a.getSrcInfo());
    if (a.defaultValue)
      CHECK(transform(a.defaultValue));
  }
  c->scope.pop_back();

  c->scope.emplace_back(0, stmt->getSuite());
  v.transform(stmt->getSuite());
  v.processChildCaptures();
  c->scope.pop_back();
  if (v.hasErrors())
    errors.append(v.errors);
  if (!canContinue())
    return;

  auto b = std::make_unique<BindingsAttribute>();
  b->captures = c->captures;
  for (const auto &n : c->captures)
    ctx->childCaptures.insert(n);
  for (auto &[u, v] : c->map)
    b->bindings[u] = v.size();
  stmt->setAttribute(Attr::Bindings, std::move(b));

  if (!c->classDeduce.second.empty()) {
    auto s = std::make_unique<ir::StringListAttribute>();
    for (const auto &n : c->classDeduce.second)
      s->values.push_back(n);
    stmt->setAttribute(Attr::ClassDeduce, std::move(s));
  }
}

void ScopingVisitor::visit(WithStmt *stmt) {
  ConditionalBlock c(ctx.get(), stmt->getSuite());
  for (size_t i = 0; i < stmt->size(); i++) {
    CHECK(transform((*stmt)[i]));
    if (!stmt->getVars()[i].empty())
      visitName(stmt->getVars()[i], true, stmt, stmt->getSrcInfo());
  }
  CHECK(transform(stmt->getSuite()));
}

void ScopingVisitor::visit(ClassStmt *stmt) {
  // @tuple(init=, repr=, eq=, order=, hash=, pickle=, container=, python=, add=,
  // internal=...)
  // @dataclass(...)
  // @extend

  std::map<std::string, bool> tupleMagics = {
      {"new", true},           {"repr", false},    {"hash", false},
      {"eq", false},           {"ne", false},      {"lt", false},
      {"le", false},           {"gt", false},      {"ge", false},
      {"pickle", true},        {"unpickle", true}, {"to_py", false},
      {"from_py", false},      {"iter", false},    {"getitem", false},
      {"len", false},          {"to_gpu", false},  {"from_gpu", false},
      {"from_gpu_new", false}, {"tuplesize", true}};

  for (auto &d : stmt->getDecorators()) {
    if (isId(d, "deduce")) {
      stmt->setAttribute(Attr::ClassDeduce);
    } else if (isId(d, "__notuple__")) {
      stmt->setAttribute(Attr::ClassNoTuple);
    } else if (isId(d, "dataclass")) {
    } else if (auto c = cast<CallExpr>(d)) {
      if (isId(c->getExpr(), Attr::Tuple)) {
        stmt->setAttribute(Attr::Tuple);
        for (auto &m : tupleMagics)
          m.second = true;
      } else if (!isId(c->getExpr(), "dataclass")) {
        STOP_ERROR(Error::CLASS_BAD_DECORATOR, c->getExpr());
      } else if (stmt->hasAttribute(Attr::Tuple)) {
        STOP_ERROR(Error::CLASS_CONFLICT_DECORATOR, c, "dataclass", Attr::Tuple);
      }
      for (const auto &a : *c) {
        auto b = cast<BoolExpr>(a);
        if (!b)
          STOP_ERROR(Error::CLASS_NONSTATIC_DECORATOR, a.getSrcInfo());
        char val = char(b->getValue());
        if (a.getName() == "init") {
          tupleMagics["new"] = val;
        } else if (a.getName() == "repr") {
          tupleMagics["repr"] = val;
        } else if (a.getName() == "eq") {
          tupleMagics["eq"] = tupleMagics["ne"] = val;
        } else if (a.getName() == "order") {
          tupleMagics["lt"] = tupleMagics["le"] = tupleMagics["gt"] =
              tupleMagics["ge"] = val;
        } else if (a.getName() == "hash") {
          tupleMagics["hash"] = val;
        } else if (a.getName() == "pickle") {
          tupleMagics["pickle"] = tupleMagics["unpickle"] = val;
        } else if (a.getName() == "python") {
          tupleMagics["to_py"] = tupleMagics["from_py"] = val;
        } else if (a.getName() == "gpu") {
          tupleMagics["to_gpu"] = tupleMagics["from_gpu"] =
              tupleMagics["from_gpu_new"] = val;
        } else if (a.getName() == "container") {
          tupleMagics["iter"] = tupleMagics["getitem"] = val;
        } else {
          STOP_ERROR(Error::CLASS_BAD_DECORATOR_ARG, a.getSrcInfo());
        }
      }
    } else if (isId(d, Attr::Tuple)) {
      if (stmt->hasAttribute(Attr::Tuple))
        STOP_ERROR(Error::CLASS_MULTIPLE_DECORATORS, d, Attr::Tuple);
      stmt->setAttribute(Attr::Tuple);
      for (auto &m : tupleMagics) {
        m.second = true;
      }
    } else if (isId(d, Attr::Extend)) {
      stmt->setAttribute(Attr::Extend);
      if (stmt->getDecorators().size() != 1) {
        STOP_ERROR(
            Error::CLASS_SINGLE_DECORATOR,
            stmt->getDecorators()[stmt->getDecorators().front() == d]->getSrcInfo(),
            Attr::Extend);
      }
    } else if (isId(d, Attr::Internal)) {
      stmt->setAttribute(Attr::Internal);
    } else {
      STOP_ERROR(Error::CLASS_BAD_DECORATOR, d);
    }
  }
  if (stmt->hasAttribute(Attr::ClassDeduce))
    tupleMagics["new"] = false;
  if (!stmt->hasAttribute(Attr::Tuple)) {
    tupleMagics["init"] = tupleMagics["new"];
    tupleMagics["new"] = tupleMagics["raw"] = true;
    tupleMagics["len"] = false;
  }
  tupleMagics["dict"] = true;
  // Internal classes do not get any auto-generated members.
  std::vector<std::string> magics;
  if (!stmt->hasAttribute(Attr::Internal)) {
    for (auto &m : tupleMagics)
      if (m.second) {
        if (m.first == "new")
          magics.insert(magics.begin(), m.first);
        else
          magics.push_back(m.first);
      }
  }
  stmt->setAttribute(Attr::ClassMagic,
                     std::make_unique<ir::StringListAttribute>(magics));
  std::unordered_set<std::string> seen;
  if (stmt->hasAttribute(Attr::Extend) && !stmt->empty())
    STOP_ERROR(Error::CLASS_EXTENSION, stmt->front().getSrcInfo());
  if (stmt->hasAttribute(Attr::Extend) &&
      !(stmt->getBaseClasses().empty() && stmt->getStaticBaseClasses().empty())) {
    STOP_ERROR(Error::CLASS_EXTENSION, stmt->getBaseClasses().empty()
                                           ? stmt->getStaticBaseClasses().front()
                                           : stmt->getBaseClasses().front());
  }
  for (auto &a : *stmt) {
    if (!a.getType() && !a.getDefault())
      STOP_ERROR(Error::CLASS_MISSING_TYPE, a.getSrcInfo(), a.getName());
    if (in(seen, a.getName()))
      STOP_ERROR(Error::CLASS_ARG_TWICE, a.getSrcInfo(), a.getName());
    seen.insert(a.getName());
  }

  if (stmt->hasAttribute(Attr::Extend))
    visitName(stmt->getName());
  else
    visitName(stmt->getName(), true, stmt, stmt->getSrcInfo());

  auto c = std::make_shared<ScopingVisitor::Context>();
  c->cache = ctx->cache;
  c->renames = ctx->renames;
  ScopingVisitor v;
  c->scope.emplace_back(0);
  c->inClass = true;
  v.ctx = c;
  for (const auto &a : *stmt) {
    v.transform(a.type);
    v.transform(a.defaultValue);
  }
  v.transform(stmt->getSuite());
  c->scope.pop_back();
  if (v.hasErrors())
    errors.append(v.errors);
  if (!canContinue())
    return;

  for (auto &d : stmt->getBaseClasses())
    CHECK(transform(d));
  for (auto &d : stmt->getStaticBaseClasses())
    CHECK(transform(d));
}

void ScopingVisitor::processChildCaptures() {
  for (auto &n : ctx->childCaptures) {
    if (auto i = in(ctx->map, n.first)) {
      if (i->back().binding && cast<ClassStmt>(i->back().binding))
        continue;
    }
    if (!findDominatingBinding(n.first)) {
      ctx->captures.insert(n); // propagate!
    }
  }
}

void ScopingVisitor::switchToUpdate(ASTNode *binding, const std::string &name,
                                    bool gotUsedVar) {
  if (binding && binding->hasAttribute(Attr::Bindings)) {
    binding->getAttribute<BindingsAttribute>(Attr::Bindings)->bindings.erase(name);
  }
  if (binding) {
    if (!gotUsedVar && binding->hasAttribute(Attr::ExprDominatedUsed))
      binding->eraseAttribute(Attr::ExprDominatedUsed);
    binding->setAttribute(gotUsedVar ? Attr::ExprDominatedUsed : Attr::ExprDominated);
  }
  if (cast<FunctionStmt>(binding))
    STOP_ERROR(error::Error::ID_INVALID_BIND, binding, name);
  else if (cast<ClassStmt>(binding))
    STOP_ERROR(error::Error::ID_INVALID_BIND, binding, name);
}

bool ScopingVisitor::visitName(const std::string &name, bool adding, ASTNode *root,
                               const SrcInfo &src) {
  if (adding && ctx->inClass)
    return false;
  if (adding) {
    if (auto p = in(ctx->captures, name)) {
      if (*p == BindingsAttribute::CaptureType::Read) {
        addError(error::Error::ASSIGN_LOCAL_REFERENCE, ctx->firstSeen[name], name, src);
        return false;
      } else if (root) { // global, nonlocal
        switchToUpdate(root, name, false);
      }
    } else {
      if (auto i = in(ctx->childCaptures, name)) {
        if (*i != BindingsAttribute::CaptureType::Global && ctx->functionScope) {
          auto newScope = std::vector<int>{ctx->scope[0].id};
          seqassert(ctx->scope.front().suite, "invalid suite");
          if (!ctx->scope.front().suite->hasAttribute(Attr::Bindings))
            ctx->scope.front().suite->setAttribute(
                Attr::Bindings, std::make_unique<BindingsAttribute>());
          ctx->scope.front()
              .suite->getAttribute<BindingsAttribute>(Attr::Bindings)
              ->bindings[name] = false;
          auto newItem = ScopingVisitor::Context::Item(src, newScope, nullptr);
          ctx->map[name].push_back(newItem);
        }
      }
      ctx->map[name].emplace_front(src, ctx->getScope(), root);
    }
  } else {
    if (!in(ctx->firstSeen, name))
      ctx->firstSeen[name] = src;
    if (!in(ctx->map, name)) {
      ctx->captures[name] = BindingsAttribute::CaptureType::Read;
    }
  }
  if (auto val = findDominatingBinding(name)) {
    // Track loop variables to dominate them later. Example:
    // x = 1
    // while True:
    //   if x > 10: break
    //   x = x + 1  # x must be dominated after the loop to ensure that it gets updated
    auto scope = ctx->getScope();
    for (size_t li = ctx->scope.size(); li-- > 0;) {
      if (ctx->scope[li].seenVars) {
        bool inside = val->scope.size() >= scope.size() &&
                      val->scope[scope.size() - 1] == scope.back();
        if (!inside)
          ctx->scope[li].seenVars->insert(name);
        else
          break;
      }
      scope.pop_back();
    }

    // Variable binding check for variables that are defined within conditional blocks
    if (!val->accessChecked.empty()) {
      bool checked = false;
      for (size_t ai = val->accessChecked.size(); ai-- > 0;) {
        auto &a = val->accessChecked[ai];
        if (a.size() <= ctx->scope.size() &&
            a[a.size() - 1] == ctx->scope[a.size() - 1].id) {
          checked = true;
          break;
        }
      }
      if (!checked) {
        seqassert(!adding, "bad branch");
        if (!(val->binding && val->binding->hasAttribute(Attr::Bindings))) {
          // If the expression is not conditional, we can just do the check once
          val->accessChecked.push_back(ctx->getScope());
        }
        return true;
      }
    }
  }
  return false;
}

/// Get an item from the context. Perform domination analysis for accessing items
/// defined in the conditional blocks (i.e., Python scoping).
ScopingVisitor::Context::Item *
ScopingVisitor::findDominatingBinding(const std::string &name, bool allowShadow) {
  auto it = in(ctx->map, name);
  if (!it || it->empty())
    return nullptr;
  auto lastGood = it->begin();
  while (lastGood != it->end() && lastGood->ignore)
    lastGood++;
  int commonScope = int(ctx->scope.size());
  // Iterate through all bindings with the given name and find the closest binding that
  // dominates the current scope.
  for (auto i = it->begin(); i != it->end(); i++) {
    if (i->ignore)
      continue;

    bool completeDomination = i->scope.size() <= ctx->scope.size() &&
                              i->scope.back() == ctx->scope[i->scope.size() - 1].id;
    if (completeDomination) {
      commonScope = i->scope.size();
      lastGood = i;
      break;
    } else {
      seqassert(i->scope[0] == 0, "bad scoping");
      seqassert(ctx->scope[0].id == 0, "bad scoping");
      // Find the longest block prefix between the binding and the current common scope.
      commonScope = std::min(commonScope, int(i->scope.size()));
      while (commonScope > 0 &&
             i->scope[commonScope - 1] != ctx->scope[commonScope - 1].id)
        commonScope--;
      // if (commonScope < int(ctx->scope.size()) && commonScope != p)
      //   break;
      lastGood = i;
    }
  }
  // if (commonScope != ctx->scope.size())
  //   LOG("==> {}: {} / {} vs {}", getSrcInfo(), name, ctx->getScope(), commonScope);
  seqassert(lastGood != it->end(), "corrupted scoping ({})", name);
  if (!allowShadow) { // go to the end
    lastGood = it->end();
    --lastGood;
    int p = std::min(commonScope, int(lastGood->scope.size()));
    while (p >= 0 && lastGood->scope[p - 1] != ctx->scope[p - 1].id)
      p--;
    commonScope = p;
  }

  bool gotUsedVar = false;
  if (lastGood->scope.size() != commonScope) {
    // The current scope is potentially reachable by multiple bindings that are
    // not dominated by a common binding. Create such binding in the scope that
    // dominates (covers) all of them.
    auto scope = ctx->getScope();
    auto newScope = std::vector<int>(scope.begin(), scope.begin() + commonScope);

    // Make sure to prepend a binding declaration: `var` and `var__used__ = False`
    // to the dominating scope.
    for (size_t si = commonScope; si-- > 0; si--) {
      if (!ctx->scope[si].suite)
        continue;
      if (!ctx->scope[si].suite->hasAttribute(Attr::Bindings))
        ctx->scope[si].suite->setAttribute(Attr::Bindings,
                                           std::make_unique<BindingsAttribute>());
      ctx->scope[si]
          .suite->getAttribute<BindingsAttribute>(Attr::Bindings)
          ->bindings[name] = true;
      auto newItem = ScopingVisitor::Context::Item(
          getSrcInfo(), newScope, ctx->scope[si].suite, {lastGood->scope});
      lastGood = it->insert(++lastGood, newItem);
      gotUsedVar = true;
      break;
    }
  } else if (lastGood->binding && lastGood->binding->hasAttribute(Attr::Bindings)) {
    gotUsedVar = lastGood->binding->getAttribute<BindingsAttribute>(Attr::Bindings)
                     ->bindings[name];
  }
  // Remove all bindings after the dominant binding.
  for (auto i = it->begin(); i != it->end(); i++) {
    if (i == lastGood)
      break;
    switchToUpdate(i->binding, name, gotUsedVar);
    i->scope = lastGood->scope;
    i->ignore = true;
  }
  if (!gotUsedVar && lastGood->binding &&
      lastGood->binding->hasAttribute(Attr::Bindings))
    lastGood->binding->getAttribute<BindingsAttribute>(Attr::Bindings)->bindings[name] =
        false;
  return &(*lastGood);
}

} // namespace codon::ast
