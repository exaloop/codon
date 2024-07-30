// Copyright (C) 2022-2024 Exaloop Inc. <https://exaloop.io>

#include "translate.h"

#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "codon/cir/transform/parallel/schedule.h"
#include "codon/cir/util/cloning.h"
#include "codon/parser/ast.h"
#include "codon/parser/common.h"
#include "codon/parser/visitors/translate/translate_ctx.h"

using codon::ir::cast;
using codon::ir::transform::parallel::OMPSched;
using fmt::format;

namespace codon::ast {

TranslateVisitor::TranslateVisitor(std::shared_ptr<TranslateContext> ctx)
    : ctx(std::move(ctx)), result(nullptr) {}

ir::Func *TranslateVisitor::apply(Cache *cache, const StmtPtr &stmts) {
  ir::BodiedFunc *main = nullptr;
  if (cache->isJit) {
    auto fnName = format("_jit_{}", cache->jitCell);
    main = cache->module->Nr<ir::BodiedFunc>(fnName);
    main->setSrcInfo({"<jit>", 0, 0, 0});
    main->setGlobal();
    auto irType = cache->module->unsafeGetFuncType(
        fnName, cache->classes["NoneType"].realizations["NoneType"]->ir, {}, false);
    main->realize(irType, {});
    main->setJIT();
  } else {
    main = cast<ir::BodiedFunc>(cache->module->getMainFunc());
    auto path = getAbsolutePath(cache->module0);
    main->setSrcInfo({path, 0, 0, 0});
  }

  auto block = cache->module->Nr<ir::SeriesFlow>("body");
  main->setBody(block);

  if (!cache->codegenCtx)
    cache->codegenCtx = std::make_shared<TranslateContext>(cache);
  cache->codegenCtx->bases = {main};
  cache->codegenCtx->series = {block};

  for (auto &[name, p] : cache->globals)
    if (p.first && !p.second) {
      p.second = name == VAR_ARGV ? cache->codegenCtx->getModule()->getArgVar()
                                  : cache->codegenCtx->getModule()->N<ir::Var>(
                                        SrcInfo(), nullptr, true, false, name);
      cache->codegenCtx->add(TranslateItem::Var, name, p.second);
    }

  auto tv = TranslateVisitor(cache->codegenCtx);
  tv.transform(stmts);
  for (auto &[fn, f] : cache->functions)
    if (startswith(fn, TYPE_TUPLE)) {
      tv.transformFunctionRealizations(fn, f.ast->attributes.has(Attr::LLVM));
    }
  cache->populatePythonModule();
  return main;
}

/************************************************************************************/

ir::Value *TranslateVisitor::transform(const ExprPtr &expr) {
  TranslateVisitor v(ctx);
  v.setSrcInfo(expr->getSrcInfo());

  types::PartialType *p = nullptr;
  if (expr->attributes) {
    if (expr->hasAttr(ExprAttr::List) || expr->hasAttr(ExprAttr::Set) ||
        expr->hasAttr(ExprAttr::Dict) || expr->hasAttr(ExprAttr::Partial)) {
      ctx->seqItems.emplace_back();
    }
    if (expr->hasAttr(ExprAttr::Partial))
      p = expr->type->getPartial().get();
  }

  expr->accept(v);
  ir::Value *ir = v.result;

  if (expr->attributes) {
    if (expr->hasAttr(ExprAttr::List) || expr->hasAttr(ExprAttr::Set)) {
      std::vector<ir::LiteralElement> v;
      for (auto &p : ctx->seqItems.back()) {
        seqassert(p.first <= ExprAttr::StarSequenceItem, "invalid list/set element");
        v.push_back(
            ir::LiteralElement{p.second, p.first == ExprAttr::StarSequenceItem});
      }
      if (expr->hasAttr(ExprAttr::List))
        ir->setAttribute(std::make_unique<ir::ListLiteralAttribute>(v));
      else
        ir->setAttribute(std::make_unique<ir::SetLiteralAttribute>(v));
      ctx->seqItems.pop_back();
    }
    if (expr->hasAttr(ExprAttr::Dict)) {
      std::vector<ir::DictLiteralAttribute::KeyValuePair> v;
      for (int pi = 0; pi < ctx->seqItems.back().size(); pi++) {
        auto &p = ctx->seqItems.back()[pi];
        if (p.first == ExprAttr::StarSequenceItem) {
          v.push_back({p.second, nullptr});
        } else {
          seqassert(p.first == ExprAttr::SequenceItem &&
                        pi + 1 < ctx->seqItems.back().size() &&
                        ctx->seqItems.back()[pi + 1].first == ExprAttr::SequenceItem,
                    "invalid dict element");
          v.push_back({p.second, ctx->seqItems.back()[pi + 1].second});
          pi++;
        }
      }
      ir->setAttribute(std::make_unique<ir::DictLiteralAttribute>(v));
      ctx->seqItems.pop_back();
    }
    if (expr->hasAttr(ExprAttr::Partial)) {
      std::vector<ir::Value *> v;
      seqassert(p, "invalid partial element");
      int j = 0;
      for (int i = 0; i < p->known.size(); i++) {
        if (p->known[i] && p->func->ast->args[i].status == Param::Normal) {
          seqassert(j < ctx->seqItems.back().size() &&
                        ctx->seqItems.back()[j].first == ExprAttr::SequenceItem,
                    "invalid partial element");
          v.push_back(ctx->seqItems.back()[j++].second);
        } else if (p->func->ast->args[i].status == Param::Normal) {
          v.push_back({nullptr});
        }
      }
      ir->setAttribute(
          std::make_unique<ir::PartialFunctionAttribute>(p->func->ast->name, v));
      ctx->seqItems.pop_back();
    }
    if (expr->hasAttr(ExprAttr::SequenceItem)) {
      ctx->seqItems.back().push_back({ExprAttr::SequenceItem, ir});
    }
    if (expr->hasAttr(ExprAttr::StarSequenceItem)) {
      ctx->seqItems.back().push_back({ExprAttr::StarSequenceItem, ir});
    }
  }

  return ir;
}

void TranslateVisitor::defaultVisit(Expr *n) {
  seqassert(false, "invalid node {}", n->toString());
}

void TranslateVisitor::visit(NoneExpr *expr) {
  auto f = expr->type->realizedName() + ":Optional.__new__:0";
  auto val = ctx->find(f);
  seqassert(val, "cannot find '{}'", f);
  result = make<ir::CallInstr>(expr, make<ir::VarValue>(expr, val->getFunc()),
                               std::vector<ir::Value *>{});
}

void TranslateVisitor::visit(BoolExpr *expr) {
  result = make<ir::BoolConst>(expr, expr->value, getType(expr->getType()));
}

void TranslateVisitor::visit(IntExpr *expr) {
  result = make<ir::IntConst>(expr, *(expr->intValue), getType(expr->getType()));
}

void TranslateVisitor::visit(FloatExpr *expr) {
  result = make<ir::FloatConst>(expr, *(expr->floatValue), getType(expr->getType()));
}

void TranslateVisitor::visit(StringExpr *expr) {
  result = make<ir::StringConst>(expr, expr->getValue(), getType(expr->getType()));
}

void TranslateVisitor::visit(IdExpr *expr) {
  auto val = ctx->find(expr->value);
  seqassert(val, "cannot find '{}'", expr->value);
  if (expr->value == "__vtable_size__") {
    // LOG("[] __vtable_size__={}", ctx->cache->classRealizationCnt + 2);
    result = make<ir::IntConst>(expr, ctx->cache->classRealizationCnt + 2,
                                getType(expr->getType()));
  } else if (auto *v = val->getVar()) {
    result = make<ir::VarValue>(expr, v);
  } else if (auto *f = val->getFunc()) {
    result = make<ir::VarValue>(expr, f);
  }
}

void TranslateVisitor::visit(IfExpr *expr) {
  auto cond = transform(expr->cond);
  auto ifexpr = transform(expr->ifexpr);
  auto elsexpr = transform(expr->elsexpr);
  result = make<ir::TernaryInstr>(expr, cond, ifexpr, elsexpr);
}

void TranslateVisitor::visit(CallExpr *expr) {
  if (expr->expr->isId("__ptr__")) {
    seqassert(expr->args[0].value->getId(), "expected IdExpr, got {}",
              expr->args[0].value);
    auto val = ctx->find(expr->args[0].value->getId()->value);
    seqassert(val && val->getVar(), "{} is not a variable",
              expr->args[0].value->getId()->value);
    result = make<ir::PointerValue>(expr, val->getVar());
    return;
  } else if (expr->expr->isId("__array__.__new__:0")) {
    auto fnt = expr->expr->type->getFunc();
    auto szt = fnt->funcGenerics[0].type->getStatic();
    auto sz = szt->evaluate().getInt();
    auto typ = fnt->funcParent->getClass()->generics[0].type;

    auto *arrayType = ctx->getModule()->unsafeGetArrayType(getType(typ));
    arrayType->setAstType(expr->getType());
    result = make<ir::StackAllocInstr>(expr, arrayType, sz);
    return;
  } else if (expr->expr->getId() && startswith(expr->expr->getId()->value,
                                               "__internal__.yield_in_no_suspend:0")) {
    result = make<ir::YieldInInstr>(expr, getType(expr->getType()), false);
    return;
  }

  auto ft = expr->expr->type->getFunc();
  seqassert(ft, "not calling function: {}", ft);
  auto callee = transform(expr->expr);
  bool isVariadic = ft->ast->hasAttr(Attr::CVarArg);
  std::vector<ir::Value *> items;
  for (int i = 0; i < expr->args.size(); i++) {
    seqassert(!expr->args[i].value->getEllipsis(), "ellipsis not elided");
    if (i + 1 == expr->args.size() && isVariadic) {
      auto call = expr->args[i].value->getCall();
      seqassert(
          call && call->expr->getId() &&
              startswith(call->expr->getId()->value, std::string(TYPE_TUPLE) + "["),
          "expected *args tuple: '{}'", call->toString());
      for (auto &arg : call->args)
        items.emplace_back(transform(arg.value));
    } else {
      items.emplace_back(transform(expr->args[i].value));
    }
  }
  result = make<ir::CallInstr>(expr, callee, std::move(items));
}

void TranslateVisitor::visit(DotExpr *expr) {
  if (expr->member == "__atomic__" || expr->member == "__elemsize__" ||
      expr->member == "__contents_atomic__") {
    seqassert(expr->expr->getId(), "expected IdExpr, got {}", expr->expr);
    auto type = ctx->find(expr->expr->getId()->value)->getType();
    seqassert(type, "{} is not a type", expr->expr->getId()->value);
    result = make<ir::TypePropertyInstr>(
        expr, type,
        expr->member == "__atomic__"
            ? ir::TypePropertyInstr::Property::IS_ATOMIC
            : (expr->member == "__contents_atomic__"
                   ? ir::TypePropertyInstr::Property::IS_CONTENT_ATOMIC
                   : ir::TypePropertyInstr::Property::SIZEOF));
  } else {
    result = make<ir::ExtractInstr>(expr, transform(expr->expr), expr->member);
  }
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

  std::vector<ir::PipelineFlow::Stage> stages;
  auto *firstStage = transform(expr->items[0].expr);
  auto firstIsGen = isGen(firstStage);
  stages.emplace_back(firstStage, std::vector<ir::Value *>(), firstIsGen, false);

  // Pipeline without generators (just function call sugar)
  auto simplePipeline = !firstIsGen;
  for (auto i = 1; i < expr->items.size(); i++) {
    auto call = expr->items[i].expr->getCall();
    seqassert(call, "{} is not a call", expr->items[i].expr);

    auto fn = transform(call->expr);
    if (i + 1 != expr->items.size())
      simplePipeline &= !isGen(fn);

    std::vector<ir::Value *> args;
    args.reserve(call->args.size());
    for (auto &a : call->args)
      args.emplace_back(a.value->getEllipsis() ? nullptr : transform(a.value));
    stages.emplace_back(fn, args, isGen(fn), false);
  }

  if (simplePipeline) {
    // Transform a |> b |> c to c(b(a))
    ir::util::CloneVisitor cv(ctx->getModule());
    result = cv.clone(stages[0].getCallee());
    for (auto i = 1; i < stages.size(); ++i) {
      std::vector<ir::Value *> newArgs;
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

void TranslateVisitor::visit(ExprStmt *stmt) {
  if (stmt->expr->getCall() &&
      stmt->expr->getCall()->expr->isId("__internal__.yield_final:0")) {
    result = make<ir::YieldInstr>(stmt, transform(stmt->expr->getCall()->args[0].value),
                                  true);
    ctx->getBase()->setGenerator();
  } else {
    result = transform(stmt->expr);
  }
}

void TranslateVisitor::visit(AssignStmt *stmt) {
  if (stmt->lhs && stmt->lhs->isId(VAR_ARGV))
    return;

  if (stmt->isUpdate()) {
    seqassert(stmt->lhs->getId(), "expected IdExpr, got {}", stmt->lhs);
    auto val = ctx->find(stmt->lhs->getId()->value);
    seqassert(val && val->getVar(), "{} is not a variable", stmt->lhs->getId()->value);
    result = make<ir::AssignInstr>(stmt, val->getVar(), transform(stmt->rhs));
    return;
  }

  seqassert(stmt->lhs->getId(), "expected IdExpr, got {}", stmt->lhs);
  auto var = stmt->lhs->getId()->value;
  if (!stmt->rhs || (!stmt->rhs->isType() && stmt->rhs->type)) {
    auto isGlobal = in(ctx->cache->globals, var);
    ir::Var *v = nullptr;

    // dead declaration due to static compilation
    if (!stmt->rhs && !stmt->type && !stmt->lhs->type->getClass())
      return;

    if (isGlobal) {
      seqassert(ctx->find(var) && ctx->find(var)->getVar(), "cannot find global '{}'",
                var);
      v = ctx->find(var)->getVar();
      v->setSrcInfo(stmt->getSrcInfo());
      v->setType(getType((stmt->rhs ? stmt->rhs : stmt->lhs)->getType()));
    } else {
      v = make<ir::Var>(stmt, getType((stmt->rhs ? stmt->rhs : stmt->lhs)->getType()),
                        false, false, var);
      ctx->getBase()->push_back(v);
      ctx->add(TranslateItem::Var, var, v);
    }
    // Check if it is a C variable
    if (stmt->lhs->hasAttr(ExprAttr::ExternVar)) {
      v->setExternal();
      v->setName(ctx->cache->rev(var));
      v->setGlobal();
      return;
    }

    if (stmt->rhs)
      result = make<ir::AssignInstr>(stmt, v, transform(stmt->rhs));
  }
}

void TranslateVisitor::visit(AssignMemberStmt *stmt) {
  result = make<ir::InsertInstr>(stmt, transform(stmt->lhs), stmt->member,
                                 transform(stmt->rhs));
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
  std::unique_ptr<OMPSched> os = nullptr;
  if (stmt->decorator) {
    os = std::make_unique<OMPSched>();
    auto c = stmt->decorator->getCall();
    seqassert(c, "for par is not a call: {}", stmt->decorator);
    auto fc = c->expr->getType()->getFunc();
    seqassert(fc && fc->ast->name == "std.openmp.for_par:0",
              "for par is not a function");
    auto schedule =
        fc->funcGenerics[0].type->getStatic()->expr->staticValue.getString();
    bool ordered = fc->funcGenerics[1].type->getStatic()->expr->staticValue.getInt();
    auto threads = transform(c->args[0].value);
    auto chunk = transform(c->args[1].value);
    int64_t collapse =
        fc->funcGenerics[2].type->getStatic()->expr->staticValue.getInt();
    bool gpu = fc->funcGenerics[3].type->getStatic()->expr->staticValue.getInt();
    os = std::make_unique<OMPSched>(schedule, threads, chunk, ordered, collapse, gpu);
  }

  seqassert(stmt->var->getId(), "expected IdExpr, got {}", stmt->var);
  auto varName = stmt->var->getId()->value;
  ir::Var *var = nullptr;
  if (!ctx->find(varName) || !stmt->var->hasAttr(ExprAttr::Dominated)) {
    var = make<ir::Var>(stmt, getType(stmt->var->getType()), false, false, varName);
  } else {
    var = ctx->find(varName)->getVar();
  }
  ctx->getBase()->push_back(var);
  auto bodySeries = make<ir::SeriesFlow>(stmt, "body");

  auto loop = make<ir::ForFlow>(stmt, transform(stmt->iter), bodySeries, var);
  if (os)
    loop->setSchedule(std::move(os));
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
      if (!ctx->find(c.var) || !c.exc->hasAttr(ExprAttr::Dominated)) {
        catchVar = make<ir::Var>(stmt, excType, false, false, c.var);
      } else {
        catchVar = ctx->find(c.var)->getVar();
      }
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
  result = make<ir::ThrowInstr>(stmt, stmt->expr ? transform(stmt->expr) : nullptr);
}

void TranslateVisitor::visit(FunctionStmt *stmt) {
  // Process all realizations.
  transformFunctionRealizations(stmt->name, stmt->attributes.has(Attr::LLVM));
}

void TranslateVisitor::visit(ClassStmt *stmt) {
  // Nothing to see here, as all type handles are already generated.
  // Methods will be handled by FunctionStmt visitor.
}

/************************************************************************************/

codon::ir::types::Type *TranslateVisitor::getType(const types::TypePtr &t) {
  seqassert(t && t->getClass(), "{} is not a class", t);
  std::string name = t->getClass()->realizedTypeName();
  auto i = ctx->find(name);
  seqassert(i, "type {} not realized", t);
  return i->getType();
}

void TranslateVisitor::transformFunctionRealizations(const std::string &name,
                                                     bool isLLVM) {
  for (auto &real : ctx->cache->functions[name].realizations) {
    if (!in(ctx->cache->pendingRealizations, make_pair(name, real.first)))
      continue;
    ctx->cache->pendingRealizations.erase(make_pair(name, real.first));

    LOG_TYPECHECK("[translate] generating fn {}", real.first);
    real.second->ir->setSrcInfo(getSrcInfo());
    const auto &ast = real.second->ast;
    seqassert(ast, "AST not set for {}", real.first);
    if (!isLLVM)
      transformFunction(real.second->type.get(), ast.get(), real.second->ir);
    else
      transformLLVMFunction(real.second->type.get(), ast.get(), real.second->ir);
  }
}

void TranslateVisitor::transformFunction(types::FuncType *type, FunctionStmt *ast,
                                         ir::Func *func) {
  std::vector<std::string> names;
  std::vector<int> indices;
  for (int i = 0, j = 0; i < ast->args.size(); i++)
    if (ast->args[i].status == Param::Normal) {
      if (!type->getArgTypes()[j]->getFunc()) {
        names.push_back(ctx->cache->reverseIdentifierLookup[ast->args[i].name]);
        indices.push_back(i);
      }
      j++;
    }
  if (ast->hasAttr(Attr::CVarArg)) {
    names.pop_back();
    indices.pop_back();
  }
  // TODO: refactor IR attribute API
  std::map<std::string, std::string> attr;
  attr[".module"] = ast->attributes.module;
  for (auto &a : ast->attributes.customAttr) {
    attr[a] = "";
  }
  func->setAttribute(std::make_unique<ir::KeyValueAttribute>(attr));
  for (int i = 0; i < names.size(); i++)
    func->getArgVar(names[i])->setSrcInfo(ast->args[indices[i]].getSrcInfo());
  // func->setUnmangledName(ctx->cache->reverseIdentifierLookup[type->ast->name]);
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
  std::vector<std::string> names;
  std::vector<int> indices;
  for (int i = 0, j = 1; i < ast->args.size(); i++)
    if (ast->args[i].status == Param::Normal) {
      names.push_back(ctx->cache->reverseIdentifierLookup[ast->args[i].name]);
      indices.push_back(i);
      j++;
    }
  auto f = cast<ir::LLVMFunc>(func);
  // TODO: refactor IR attribute API
  std::map<std::string, std::string> attr;
  attr[".module"] = ast->attributes.module;
  for (auto &a : ast->attributes.customAttr)
    attr[a] = "";
  func->setAttribute(std::make_unique<ir::KeyValueAttribute>(attr));
  for (int i = 0; i < names.size(); i++)
    func->getArgVar(names[i])->setSrcInfo(ast->args[indices[i]].getSrcInfo());

  seqassert(ast->suite->firstInBlock() && ast->suite->firstInBlock()->getExpr() &&
                ast->suite->firstInBlock()->getExpr()->expr->getString(),
            "LLVM function does not begin with a string");
  std::istringstream sin(
      ast->suite->firstInBlock()->getExpr()->expr->getString()->getValue());
  std::vector<ir::types::Generic> literals;
  auto &ss = ast->suite->getSuite()->stmts;
  for (int i = 1; i < ss.size(); i++) {
    if (auto *ei = ss[i]->getExpr()->expr->getInt()) { // static integer expression
      literals.emplace_back(*(ei->intValue));
    } else if (auto *es = ss[i]->getExpr()->expr->getString()) { // static string
      literals.emplace_back(es->getValue());
    } else {
      seqassert(ss[i]->getExpr()->expr->getType(), "invalid LLVM type argument: {}",
                ss[i]->getExpr()->toString());
      literals.emplace_back(getType(ss[i]->getExpr()->expr->getType()));
    }
  }
  bool isDeclare = true;
  std::string declare;
  std::vector<std::string> lines;
  for (std::string l; getline(sin, l);) {
    std::string lp = l;
    ltrim(lp);
    rtrim(lp);
    // Extract declares and constants.
    if (isDeclare && !startswith(lp, "declare ") && !startswith(lp, "@")) {
      bool isConst = lp.find("private constant") != std::string::npos;
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
  // func->setUnmangledName(ctx->cache->reverseIdentifierLookup[type->ast->name]);
}

} // namespace codon::ast
