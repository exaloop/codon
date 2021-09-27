/*
 * translate.cpp --- AST-to-IR translation.
 *
 * (c) Seq project. All rights reserved.
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 */

#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "parser/ast.h"
#include "parser/common.h"
#include "parser/visitors/translate/translate.h"
#include "parser/visitors/translate/translate_ctx.h"
#include "sir/transform/parallel/schedule.h"
#include "sir/util/cloning.h"

using fmt::format;
using seq::ir::cast;
using seq::ir::transform::parallel::OMPSched;
using std::function;
using std::get;
using std::make_shared;
using std::move;
using std::shared_ptr;
using std::stack;
using std::vector;

namespace seq {
namespace ast {

TranslateVisitor::TranslateVisitor(shared_ptr<TranslateContext> ctx)
    : ctx(move(ctx)), result(nullptr) {}

ir::Module *TranslateVisitor::apply(shared_ptr<Cache> cache, StmtPtr stmts) {
  auto main = cast<ir::BodiedFunc>(cache->module->getMainFunc());

  char buf[PATH_MAX + 1];
  realpath(cache->module0.c_str(), buf);
  main->setSrcInfo({string(buf), 0, 0, 0});

  auto block = cache->module->Nr<ir::SeriesFlow>("body");
  main->setBody(block);

  cache->codegenCtx = make_shared<TranslateContext>(cache, block, main);
  TranslateVisitor(cache->codegenCtx).transform(stmts);
  return cache->module;
}

/************************************************************************************/

ir::Value *TranslateVisitor::transform(const ExprPtr &expr) {
  TranslateVisitor v(ctx);
  v.setSrcInfo(expr->getSrcInfo());
  expr->accept(v);
  return v.result;
}

void TranslateVisitor::defaultVisit(Expr *n) {
  seqassert(false, "invalid node {}", n->toString());
}

void TranslateVisitor::visit(BoolExpr *expr) {
  result = make<ir::BoolConst>(expr, expr->value, getType(expr->getType()));
}

void TranslateVisitor::visit(IntExpr *expr) {
  result = make<ir::IntConst>(expr, expr->intValue, getType(expr->getType()));
}

void TranslateVisitor::visit(FloatExpr *expr) {
  result = make<ir::FloatConst>(expr, expr->floatValue, getType(expr->getType()));
}

void TranslateVisitor::visit(StringExpr *expr) {
  result = make<ir::StringConst>(expr, expr->getValue(), getType(expr->getType()));
}

void TranslateVisitor::visit(IdExpr *expr) {
  auto val = ctx->find(expr->value);
  seqassert(val, "cannot find '{}'", expr->value);
  if (auto *v = val->getVar())
    result = make<ir::VarValue>(expr, v);
  else if (auto *f = val->getFunc())
    result = make<ir::VarValue>(expr, f);
}

void TranslateVisitor::visit(IfExpr *expr) {
  result = make<ir::TernaryInstr>(expr, transform(expr->cond), transform(expr->ifexpr),
                                  transform(expr->elsexpr));
}

void TranslateVisitor::visit(CallExpr *expr) {
  auto ft = expr->expr->type->getFunc();
  seqassert(ft, "not calling function: {}", ft->toString());
  auto callee = transform(expr->expr);
  bool isVariadic = ft->ast->hasAttr(Attr::CVarArg);
  vector<ir::Value *> items;
  for (int i = 0; i < expr->args.size(); i++) {
    seqassert(!expr->args[i].value->getEllipsis(), "ellipsis not elided");
    if (i + 1 == expr->args.size() && isVariadic) {
      auto call = expr->args[i].value->getCall();
      seqassert(call && call->expr->getId() &&
                    startswith(call->expr->getId()->value, TYPE_TUPLE),
                "expected *args tuple");
      for (auto &arg : call->args)
        items.emplace_back(transform(arg.value));
    } else {
      items.emplace_back(transform(expr->args[i].value));
    }
  }
  result = make<ir::CallInstr>(expr, callee, move(items));
}

void TranslateVisitor::visit(StackAllocExpr *expr) {
  auto *arrayType =
      ctx->getModule()->unsafeGetArrayType(getType(expr->typeExpr->getType()));
  arrayType->setAstType(expr->getType());
  seqassert(expr->expr->getInt(), "expected a static integer, got {}",
            expr->expr->toString());
  result = make<ir::StackAllocInstr>(expr, arrayType, expr->expr->getInt()->intValue);
}

void TranslateVisitor::visit(DotExpr *expr) {
  if (expr->member == "__atomic__" || expr->member == "__elemsize__") {
    seqassert(expr->expr->getId(), "expected IdExpr, got {}", expr->expr->toString());
    auto type = ctx->find(expr->expr->getId()->value)->getType();
    seqassert(type, "{} is not a type", expr->expr->getId()->value);
    result = make<ir::TypePropertyInstr>(
        expr, type,
        expr->member == "__atomic__" ? ir::TypePropertyInstr::Property::IS_ATOMIC
                                     : ir::TypePropertyInstr::Property::SIZEOF);
  } else {
    result = make<ir::ExtractInstr>(expr, transform(expr->expr), expr->member);
  }
}

void TranslateVisitor::visit(PtrExpr *expr) {
  seqassert(expr->expr->getId(), "expected IdExpr, got {}", expr->expr->toString());
  auto val = ctx->find(expr->expr->getId()->value);
  seqassert(val && val->getVar(), "{} is not a variable", expr->expr->getId()->value);
  result = make<ir::PointerValue>(expr, val->getVar());
}

void TranslateVisitor::visit(YieldExpr *expr) {
  result = make<ir::YieldInInstr>(expr, getType(expr->getType()));
}

void TranslateVisitor::visit(PipeExpr *expr) {
  auto isGen = [](const ir::Value *v) -> bool {
    auto *type = v->getType();
    if (ir::isA<ir::types::GeneratorType>(type))
      return true;
    else if (auto *fn = cast<ir::types::FuncType>(type)) {
      return ir::isA<ir::types::GeneratorType>(fn->getReturnType());
    }
    return false;
  };

  vector<ir::PipelineFlow::Stage> stages;
  auto *firstStage = transform(expr->items[0].expr);
  auto firstIsGen = isGen(firstStage);
  stages.emplace_back(firstStage, vector<ir::Value *>(), firstIsGen, false);

  // Pipeline without generators (just function call sugar)
  auto simplePipeline = !firstIsGen;
  for (auto i = 1; i < expr->items.size(); i++) {
    auto call = expr->items[i].expr->getCall();
    seqassert(call, "{} is not a call", expr->items[i].expr->toString());

    auto fn = transform(call->expr);
    if (i + 1 != expr->items.size())
      simplePipeline &= !isGen(fn);

    vector<ir::Value *> args;
    for (auto &a : call->args)
      args.emplace_back(a.value->getEllipsis() ? nullptr : transform(a.value));
    stages.emplace_back(fn, args, isGen(fn), false);
  }

  if (simplePipeline) {
    // Transform a |> b |> c to c(b(a))
    ir::util::CloneVisitor cv(ctx->getModule());
    result = cv.clone(stages[0].getCallee());
    for (auto i = 1; i < stages.size(); ++i) {
      vector<ir::Value *> newArgs;
      for (auto arg : stages[i])
        newArgs.push_back(arg ? cv.clone(arg) : result);
      result = make<ir::CallInstr>(expr, cv.clone(stages[i].getCallee()), newArgs);
    }
  } else {
    for (int i = 0; i < expr->items.size(); i++)
      if (expr->items[i].op == "||>")
        stages[i].setParallel();
    // This is a statement in IR.
    ctx->getSeries()->push_back(make<ir::PipelineFlow>(expr, stages));
  }
}

void TranslateVisitor::visit(StmtExpr *expr) {
  auto *bodySeries = make<ir::SeriesFlow>(expr, "body");
  ctx->addSeries(bodySeries);
  for (auto &s : expr->stmts)
    transform(s);
  ctx->popSeries();
  result = make<ir::FlowInstr>(expr, bodySeries, transform(expr->expr));
}

/************************************************************************************/

ir::Value *TranslateVisitor::transform(const StmtPtr &stmt) {
  TranslateVisitor v(ctx);
  v.setSrcInfo(stmt->getSrcInfo());
  stmt->accept(v);
  if (v.result)
    ctx->getSeries()->push_back(v.result);
  return v.result;
}

void TranslateVisitor::defaultVisit(Stmt *n) {
  seqassert(false, "invalid node {}", n->toString());
}

void TranslateVisitor::visit(SuiteStmt *stmt) {
  for (auto &s : stmt->stmts)
    transform(s);
}

void TranslateVisitor::visit(BreakStmt *stmt) { result = make<ir::BreakInstr>(stmt); }

void TranslateVisitor::visit(ContinueStmt *stmt) {
  result = make<ir::ContinueInstr>(stmt);
}

void TranslateVisitor::visit(ExprStmt *stmt) { result = transform(stmt->expr); }

void TranslateVisitor::visit(AssignStmt *stmt) {
  seqassert(stmt->lhs->getId(), "expected IdExpr, got {}", stmt->lhs->toString());
  auto var = stmt->lhs->getId()->value;
  if (!stmt->rhs && var == VAR_ARGV) {
    ctx->add(TranslateItem::Var, var, ctx->getModule()->getArgVar());
  } else if (!stmt->rhs || !stmt->rhs->isType()) {
    auto *newVar =
        make<ir::Var>(stmt, getType((stmt->rhs ? stmt->rhs : stmt->lhs)->getType()),
                      in(ctx->cache->globals, var), var);
    if (!in(ctx->cache->globals, var))
      ctx->getBase()->push_back(newVar);
    ctx->add(TranslateItem::Var, var, newVar);
    if (stmt->rhs)
      result = make<ir::AssignInstr>(stmt, newVar, transform(stmt->rhs));
  }
}

void TranslateVisitor::visit(AssignMemberStmt *stmt) {
  result = make<ir::InsertInstr>(stmt, transform(stmt->lhs), stmt->member,
                                 transform(stmt->rhs));
}

void TranslateVisitor::visit(UpdateStmt *stmt) {
  seqassert(stmt->lhs->getId(), "expected IdExpr, got {}", stmt->lhs->toString());
  auto val = ctx->find(stmt->lhs->getId()->value);
  seqassert(val && val->getVar(), "{} is not a variable", stmt->lhs->getId()->value);
  result = make<ir::AssignInstr>(stmt, val->getVar(), transform(stmt->rhs));
}

void TranslateVisitor::visit(ReturnStmt *stmt) {
  result = make<ir::ReturnInstr>(stmt, stmt->expr ? transform(stmt->expr) : nullptr);
}

void TranslateVisitor::visit(YieldStmt *stmt) {
  result = make<ir::YieldInstr>(stmt, stmt->expr ? transform(stmt->expr) : nullptr);
  ctx->getBase()->setGenerator();
}

void TranslateVisitor::visit(WhileStmt *stmt) {
  auto loop = make<ir::WhileFlow>(stmt, transform(stmt->cond),
                                  make<ir::SeriesFlow>(stmt, "body"));
  ctx->addSeries(cast<ir::SeriesFlow>(loop->getBody()));
  transform(stmt->suite);
  ctx->popSeries();
  result = loop;
}

void TranslateVisitor::visit(ForStmt *stmt) {
  unique_ptr<OMPSched> os = nullptr;
  if (stmt->decorator) {
    os = make_unique<OMPSched>();
    auto c = stmt->decorator->getCall();
    seqassert(c, "for par is not a call: {}", stmt->decorator->toString());
    auto fc = c->expr->getType()->getFunc();
    seqassert(fc && fc->ast->name == "std.openmp.for_par", "for par is not a function");
    auto schedule =
        fc->funcGenerics[0].type->getStatic()->expr->staticValue.getString();
    bool ordered = fc->funcGenerics[1].type->getStatic()->expr->staticValue.getInt();
    auto threads = transform(c->args[0].value);
    auto chunk = transform(c->args[1].value);
    os = make_unique<OMPSched>(schedule, threads, chunk, ordered);
    LOG_TYPECHECK("parsed {}", stmt->decorator->toString());
  }

  seqassert(stmt->var->getId(), "expected IdExpr, got {}", stmt->var->toString());
  auto varName = stmt->var->getId()->value;
  auto var = make<ir::Var>(stmt, getType(stmt->var->getType()), false, varName);
  ctx->getBase()->push_back(var);
  auto bodySeries = make<ir::SeriesFlow>(stmt, "body");

  auto loop = make<ir::ForFlow>(stmt, transform(stmt->iter), bodySeries, var);
  if (os)
    loop->setSchedule(move(os));
  ctx->add(TranslateItem::Var, varName, var);
  ctx->addSeries(cast<ir::SeriesFlow>(loop->getBody()));
  transform(stmt->suite);
  ctx->popSeries();
  result = loop;
}

void TranslateVisitor::visit(IfStmt *stmt) {
  auto cond = transform(stmt->cond);
  auto trueSeries = make<ir::SeriesFlow>(stmt, "ifstmt_true");
  ctx->addSeries(trueSeries);
  transform(stmt->ifSuite);
  ctx->popSeries();

  ir::SeriesFlow *falseSeries = nullptr;
  if (stmt->elseSuite) {
    falseSeries = make<ir::SeriesFlow>(stmt, "ifstmt_false");
    ctx->addSeries(falseSeries);
    transform(stmt->elseSuite);
    ctx->popSeries();
  }
  result = make<ir::IfFlow>(stmt, cond, trueSeries, falseSeries);
}

void TranslateVisitor::visit(TryStmt *stmt) {
  auto *bodySeries = make<ir::SeriesFlow>(stmt, "body");
  ctx->addSeries(bodySeries);
  transform(stmt->suite);
  ctx->popSeries();

  auto finallySeries = make<ir::SeriesFlow>(stmt, "finally");
  if (stmt->finally) {
    ctx->addSeries(finallySeries);
    transform(stmt->finally);
    ctx->popSeries();
  }

  auto *tc = make<ir::TryCatchFlow>(stmt, bodySeries, finallySeries);
  for (auto &c : stmt->catches) {
    auto *catchBody = make<ir::SeriesFlow>(stmt, "catch");
    auto *excType = c.exc ? getType(c.exc->getType()) : nullptr;
    ir::Var *catchVar = nullptr;
    if (!c.var.empty()) {
      catchVar = make<ir::Var>(stmt, excType, false, c.var);
      ctx->add(TranslateItem::Var, c.var, catchVar);
      ctx->getBase()->push_back(catchVar);
    }
    ctx->addSeries(catchBody);
    transform(c.suite);
    ctx->popSeries();
    tc->push_back(ir::TryCatchFlow::Catch(catchBody, excType, catchVar));
  }
  result = tc;
}

void TranslateVisitor::visit(ThrowStmt *stmt) {
  result = make<ir::ThrowInstr>(stmt, transform(stmt->expr));
}

void TranslateVisitor::visit(FunctionStmt *stmt) {
  // Process all realizations.
  for (auto &real : ctx->cache->functions[stmt->name].realizations) {
    if (!in(ctx->cache->pendingRealizations, make_pair(stmt->name, real.first)))
      continue;
    ctx->cache->pendingRealizations.erase(make_pair(stmt->name, real.first));

    LOG_TYPECHECK("[translate] generating fn {}", real.first);
    real.second->ir->setSrcInfo(getSrcInfo());
    const auto &ast = real.second->ast;
    seqassert(ast, "AST not set for {}", real.first);
    if (!stmt->attributes.has(Attr::LLVM))
      transformFunction(real.second->type.get(), ast.get(), real.second->ir);
    else
      transformLLVMFunction(real.second->type.get(), ast.get(), real.second->ir);
  }
}

void TranslateVisitor::visit(ClassStmt *stmt) {
  // Nothing to see here, as all type handles are already generated.
  // Methods will be handled by FunctionStmt visitor.
}

/************************************************************************************/

seq::ir::types::Type *TranslateVisitor::getType(const types::TypePtr &t) {
  seqassert(t && t->getClass(), "{} is not a class", t ? t->toString() : "-");
  string name = t->getClass()->realizedTypeName();
  auto i = ctx->find(name);
  seqassert(i, "type {} not realized", t->toString());
  return i->getType();
}

void TranslateVisitor::transformFunction(types::FuncType *type, FunctionStmt *ast,
                                         ir::Func *func) {
  vector<string> names;
  vector<int> indices;
  vector<SrcInfo> srcInfos;
  vector<seq::ir::types::Type *> types;
  for (int i = 0, j = 1; i < ast->args.size(); i++)
    if (!ast->args[i].generic) {
      if (!type->args[j]->getFunc()) {
        types.push_back(getType(type->args[j]));
        names.push_back(ctx->cache->reverseIdentifierLookup[ast->args[i].name]);
        indices.push_back(i);
      }
      j++;
    }
  if (ast->hasAttr(Attr::CVarArg)) {
    types.pop_back();
    names.pop_back();
    indices.pop_back();
  }
  auto irType = ctx->getModule()->unsafeGetFuncType(
      type->realizedName(), getType(type->args[0]), types, ast->hasAttr(Attr::CVarArg));
  irType->setAstType(type->getFunc());
  func->realize(irType, names);
  // TODO: refactor IR attribute API
  map<string, string> attr;
  attr[".module"] = ast->attributes.module;
  for (auto &a : ast->attributes.customAttr) {
    // LOG("{} -> {}", ast->name, a);
    attr[a] = "";
  }
  func->setAttribute(make_unique<ir::KeyValueAttribute>(attr));
  for (int i = 0; i < names.size(); i++)
    func->getArgVar(names[i])->setSrcInfo(ast->args[indices[i]].getSrcInfo());
  func->setUnmangledName(ctx->cache->reverseIdentifierLookup[type->ast->name]);
  if (!ast->attributes.has(Attr::C) && !ast->attributes.has(Attr::Internal)) {
    ctx->addBlock();
    for (auto i = 0; i < names.size(); i++)
      ctx->add(TranslateItem::Var, ast->args[indices[i]].name,
               func->getArgVar(names[i]));
    auto body = make<ir::SeriesFlow>(ast, "body");
    ctx->bases.push_back(cast<ir::BodiedFunc>(func));
    ctx->addSeries(body);
    transform(ast->suite);
    ctx->popSeries();
    ctx->bases.pop_back();
    cast<ir::BodiedFunc>(func)->setBody(body);
    ctx->popBlock();
  }
}

void TranslateVisitor::transformLLVMFunction(types::FuncType *type, FunctionStmt *ast,
                                             ir::Func *func) {
  vector<string> names;
  vector<seq::ir::types::Type *> types;
  vector<int> indices;
  for (int i = 0, j = 1; i < ast->args.size(); i++)
    if (!ast->args[i].generic) {
      types.push_back(getType(type->args[j]));
      names.push_back(ctx->cache->reverseIdentifierLookup[ast->args[i].name]);
      indices.push_back(i);
      j++;
    }
  auto irType = ctx->getModule()->unsafeGetFuncType(type->realizedName(),
                                                    getType(type->args[0]), types);
  irType->setAstType(type->getFunc());
  auto f = cast<ir::LLVMFunc>(func);
  f->realize(irType, names);
  // TODO: refactor IR attribute API
  map<string, string> attr;
  attr[".module"] = ast->attributes.module;
  for (auto &a : ast->attributes.customAttr)
    attr[a] = "";
  func->setAttribute(make_unique<ir::KeyValueAttribute>(attr));
  for (int i = 0; i < names.size(); i++)
    func->getArgVar(names[i])->setSrcInfo(ast->args[indices[i]].getSrcInfo());

  seqassert(ast->suite->firstInBlock() && ast->suite->firstInBlock()->getExpr() &&
                ast->suite->firstInBlock()->getExpr()->expr->getString(),
            "LLVM function does not begin with a string");
  std::istringstream sin(
      ast->suite->firstInBlock()->getExpr()->expr->getString()->getValue());
  vector<ir::types::Generic> literals;
  auto &ss = ast->suite->getSuite()->stmts;
  for (int i = 1; i < ss.size(); i++) {
    if (auto *ei = ss[i]->getExpr()->expr->getInt()) { // static integer expression
      literals.emplace_back(ei->intValue);
    } else {
      seqassert(ss[i]->getExpr()->expr->isType() && ss[i]->getExpr()->expr->getType(),
                "invalid LLVM type argument: {}", ss[i]->getExpr()->toString());
      literals.emplace_back(getType(ss[i]->getExpr()->expr->getType()));
    }
  }
  bool isDeclare = true;
  string declare;
  vector<string> lines;
  for (string l; getline(sin, l);) {
    string lp = l;
    ltrim(lp);
    rtrim(lp);
    // Extract declares and constants.
    if (isDeclare && !startswith(lp, "declare ")) {
      bool isConst = lp.find("private constant") != string::npos;
      if (!isConst) {
        isDeclare = false;
        if (!lp.empty() && lp.back() != ':')
          lines.emplace_back("entry:");
      }
    }
    if (isDeclare)
      declare += lp + "\n";
    else
      lines.emplace_back(l);
  }
  f->setLLVMBody(join(lines, "\n"));
  f->setLLVMDeclarations(declare);
  f->setLLVMLiterals(literals);
  func->setUnmangledName(ctx->cache->reverseIdentifierLookup[type->ast->name]);
}

} // namespace ast
} // namespace seq
