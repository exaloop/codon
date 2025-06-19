// Copyright (C) 2022-2025 Exaloop Inc. <https://exaloop.io>

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
#include "codon/parser/visitors/typecheck/typecheck.h"

using codon::ir::cast;
using codon::ir::transform::parallel::OMPSched;

namespace codon::ast {

TranslateVisitor::TranslateVisitor(std::shared_ptr<TranslateContext> ctx)
    : ctx(std::move(ctx)), result(nullptr) {}

ir::Func *TranslateVisitor::apply(Cache *cache, Stmt *stmts) {
  ir::BodiedFunc *main = nullptr;
  if (cache->isJit) {
    auto fnName = fmt::format("_jit_{}", cache->jitCell);
    main = cache->module->Nr<ir::BodiedFunc>(fnName);
    main->setSrcInfo({"<jit>", 0, 0, 0});
    main->setGlobal();
    auto irType = cache->module->unsafeGetFuncType(
        fnName, cache->classes["NoneType"].realizations["NoneType"]->ir, {}, false);
    main->realize(irType, {});
    main->setJIT();
  } else {
    main = cast<ir::BodiedFunc>(cache->module->getMainFunc());
    auto path = cache->fs->get_module0();
    main->setSrcInfo({path, 0, 0, 0});
  }

  auto block = cache->module->Nr<ir::SeriesFlow>("body");
  main->setBody(block);

  if (!cache->codegenCtx)
    cache->codegenCtx = std::make_shared<TranslateContext>(cache);
  cache->codegenCtx->bases = {main};
  cache->codegenCtx->series = {block};

  TranslateVisitor(cache->codegenCtx).translateStmts(stmts);
  cache->populatePythonModule();
  return main;
}

void TranslateVisitor::translateStmts(Stmt *stmts) const {
  for (auto &[name, g] : ctx->cache->globals)
    if (/*g.first &&*/ !g.second) {
      ir::types::Type *vt = nullptr;
      if (auto t = ctx->cache->typeCtx->forceFind(name)->getType())
        vt = getType(t);
      g.second = name == VAR_ARGV ? ctx->cache->codegenCtx->getModule()->getArgVar()
                                  : ctx->cache->codegenCtx->getModule()->N<ir::Var>(
                                        SrcInfo(), vt, true, false, name);
      ctx->cache->codegenCtx->add(TranslateItem::Var, name, g.second);
    }
  TranslateVisitor(ctx->cache->codegenCtx).transform(stmts);
  for (auto &f : ctx->cache->functions | std::views::values)
    TranslateVisitor(ctx->cache->codegenCtx).transform(f.ast);
}

/************************************************************************************/

ir::Value *TranslateVisitor::transform(Expr *expr) {
  TranslateVisitor v(ctx);
  v.setSrcInfo(expr->getSrcInfo());

  types::ClassType *p = nullptr;
  if (expr->hasAttribute(Attr::ExprList) || expr->hasAttribute(Attr::ExprSet) ||
      expr->hasAttribute(Attr::ExprDict) || expr->hasAttribute(Attr::ExprPartial)) {
    ctx->seqItems.emplace_back();
  }
  if (expr->hasAttribute(Attr::ExprPartial)) {
    p = expr->getType()->getPartial();
  }

  expr->accept(v);
  ir::Value *ir = v.result;

  if (expr->hasAttribute(Attr::ExprList) || expr->hasAttribute(Attr::ExprSet)) {
    std::vector<ir::LiteralElement> le;
    for (auto &pl : ctx->seqItems.back()) {
      seqassert(pl.first == Attr::ExprSequenceItem ||
                    pl.first == Attr::ExprStarSequenceItem,
                "invalid list/set element");
      le.push_back(
          ir::LiteralElement{pl.second, pl.first == Attr::ExprStarSequenceItem});
    }
    if (expr->hasAttribute(Attr::ExprList))
      ir->setAttribute(std::make_unique<ir::ListLiteralAttribute>(le));
    else
      ir->setAttribute(std::make_unique<ir::SetLiteralAttribute>(le));
    ctx->seqItems.pop_back();
  }
  if (expr->hasAttribute(Attr::ExprDict)) {
    std::vector<ir::DictLiteralAttribute::KeyValuePair> dla;
    for (int pi = 0; pi < ctx->seqItems.back().size(); pi++) {
      auto &pl = ctx->seqItems.back()[pi];
      if (pl.first == Attr::ExprStarSequenceItem) {
        dla.push_back({pl.second, nullptr});
      } else {
        seqassert(pl.first == Attr::ExprSequenceItem &&
                      pi + 1 < ctx->seqItems.back().size() &&
                      ctx->seqItems.back()[pi + 1].first == Attr::ExprSequenceItem,
                  "invalid dict element");
        dla.push_back({pl.second, ctx->seqItems.back()[pi + 1].second});
        pi++;
      }
    }
    ir->setAttribute(std::make_unique<ir::DictLiteralAttribute>(dla));
    ctx->seqItems.pop_back();
  }
  if (expr->hasAttribute(Attr::ExprPartial)) {
    std::vector<ir::Value *> vals;
    seqassert(p, "invalid partial element");
    int j = 0;
    auto known = p->getPartialMask();
    auto func = p->getPartialFunc();
    for (int i = 0; i < known.size(); i++) {
      if (known[i] == types::ClassType::PartialFlag::Included &&
          (*func->ast)[i].isValue()) {
        seqassert(j < ctx->seqItems.back().size() &&
                      ctx->seqItems.back()[j].first == Attr::ExprSequenceItem,
                  "invalid partial element");
        vals.push_back(ctx->seqItems.back()[j++].second);
      } else if ((*func->ast)[i].isValue()) {
        vals.push_back({nullptr});
      }
    }
    ir->setAttribute(
        std::make_unique<ir::PartialFunctionAttribute>(func->ast->getName(), vals));
    ctx->seqItems.pop_back();
  }
  if (expr->hasAttribute(Attr::ExprSequenceItem)) {
    ctx->seqItems.back().emplace_back(Attr::ExprSequenceItem, ir);
  }
  if (expr->hasAttribute(Attr::ExprStarSequenceItem)) {
    ctx->seqItems.back().emplace_back(Attr::ExprStarSequenceItem, ir);
  }

  return ir;
}

void TranslateVisitor::defaultVisit(Expr *n) {
  seqassert(false, "invalid node {}", n->toString());
}

void TranslateVisitor::visit(NoneExpr *expr) {
  auto f = expr->getType()->realizedName() + ":" +
           getMangledMethod("std.internal.core", TYPE_OPTIONAL, "__new__");
  auto val = ctx->find(f);
  seqassert(val, "cannot find '{}'", f);
  result = make<ir::CallInstr>(expr, make<ir::VarValue>(expr, val->getFunc()),
                               std::vector<ir::Value *>{});
}

void TranslateVisitor::visit(BoolExpr *expr) {
  result = make<ir::BoolConst>(expr, expr->getValue(), getType(expr->getType()));
}

void TranslateVisitor::visit(IntExpr *expr) {
  result = make<ir::IntConst>(expr, expr->getValue(), getType(expr->getType()));
}

void TranslateVisitor::visit(FloatExpr *expr) {
  result = make<ir::FloatConst>(expr, expr->getValue(), getType(expr->getType()));
}

void TranslateVisitor::visit(StringExpr *expr) {
  result = make<ir::StringConst>(expr, expr->getValue(), getType(expr->getType()));
}

void TranslateVisitor::visit(IdExpr *expr) {
  auto val = ctx->find(expr->getValue());
  if (!val) {
    // ctx->find(expr->getValue());
    seqassert(val, "cannot find '{}'", expr->getValue());
  }
  if (expr->getValue() == getMangledVar("", "__vtable_size__")) {
    // LOG("[] __vtable_size__={}", ctx->cache->classRealizationCnt + 2);
    result = make<ir::IntConst>(expr, ctx->cache->classRealizationCnt + 2,
                                getType(expr->getType()));
  } else if (auto *v = val->getVar()) {
    result = make<ir::VarValue>(expr, v);
  } else if (auto *f = val->getFunc()) {
    result = make<ir::VarValue>(expr, f);
  } else {
    // Just use NoneType which is {} (same as type)
    auto ntval =
        ctx->find(getMangledMethod("std.internal.core", "NoneType", "__new__"));
    seqassert(ntval, "cannot find '{}'", "NoneType.__new__");
    result = make<ir::CallInstr>(expr, make<ir::VarValue>(expr, ntval->getFunc()),
                                 std::vector<ir::Value *>{});
  }
}

void TranslateVisitor::visit(IfExpr *expr) {
  auto cond = transform(expr->getCond());
  auto ifexpr = transform(expr->getIf());
  auto elsexpr = transform(expr->getElse());
  result = make<ir::TernaryInstr>(expr, cond, ifexpr, elsexpr);
}

// Search expression tree for an identifier
class IdVisitor : public CallbackASTVisitor<bool, bool> {
public:
  std::unordered_set<std::string> ids;

  bool transform(Expr *expr) override {
    IdVisitor v;
    if (expr)
      expr->accept(v);
    ids.insert(v.ids.begin(), v.ids.end());
    return true;
  }
  bool transform(Stmt *stmt) override {
    IdVisitor v;
    if (stmt)
      stmt->accept(v);
    ids.insert(v.ids.begin(), v.ids.end());
    return true;
  }
  void visit(IdExpr *expr) override { ids.insert(expr->getValue()); }
};

void TranslateVisitor::visit(GeneratorExpr *expr) {
  auto name = ctx->cache->imports[MAIN_IMPORT].ctx->generateCanonicalName("_generator");
  ir::Func *fn = ctx->cache->module->Nr<ir::BodiedFunc>(name);
  fn->setGlobal();
  fn->setGenerator();
  std::vector<std::string> names;
  std::vector<codon::ir::types::Type *> types;
  std::vector<ir::Value *> items;

  IdVisitor v;
  expr->accept(v);
  for (auto &i : v.ids) {
    auto val = ctx->find(i);
    if (val && !val->getFunc() && !val->getType() && !val->getVar()->isGlobal()) {
      types.push_back(val->getVar()->getType());
      names.push_back(i);
      items.emplace_back(make<ir::VarValue>(expr, val->getVar()));
    }
  }
  auto irType = ctx->cache->module->unsafeGetFuncType(
      name, ctx->forceFind(expr->getType()->realizedName())->getType(), types, false);
  fn->realize(irType, names);

  ctx->addBlock();
  for (auto &n : names)
    ctx->add(TranslateItem::Var, n, fn->getArgVar(n));
  auto body = make<ir::SeriesFlow>(expr, "body");
  ctx->bases.push_back(cast<ir::BodiedFunc>(fn));
  ctx->addSeries(body);

  expr->setFinalStmt(ctx->cache->N<YieldStmt>(expr->getFinalExpr()));
  auto e = expr->getFinalSuite();
  transform(e);
  ctx->popSeries();
  ctx->bases.pop_back();
  cast<ir::BodiedFunc>(fn)->setBody(body);
  ctx->popBlock();
  result = make<ir::CallInstr>(expr, make<ir::VarValue>(expr, fn), std::move(items));
}

void TranslateVisitor::visit(CallExpr *expr) {
  auto ei = cast<IdExpr>(expr->getExpr());
  if (ei && ei->getValue() == getMangledFunc("std.internal.core", "__ptr__")) {
    auto id = cast<IdExpr>(expr->begin()->getExpr());
    if (!id) {
      // Case where id is guarded by a check
      if (auto sexp = cast<StmtExpr>(expr->begin()->getExpr()))
        id = cast<IdExpr>(sexp->getExpr());
    }
    seqassert(id, "expected IdExpr, got {}", *((*expr)[0].value));
    auto key = id->getValue();
    auto val = ctx->find(key);
    seqassert(val && val->getVar(), "{} is not a variable", key);
    result = make<ir::PointerValue>(expr, val->getVar());
    return;
  } else if (ei && ei->getValue() ==
                       getMangledMethod("std.internal.core", "__array__", "__new__")) {
    auto fnt = expr->getExpr()->getType()->getFunc();
    auto sz = fnt->funcGenerics[0].type->getIntStatic()->value;
    auto typ = fnt->funcParent->getClass()->generics[0].getType();

    auto *arrayType = ctx->getModule()->unsafeGetArrayType(getType(typ));
    arrayType->setAstType(expr->getType()->shared_from_this());
    result = make<ir::StackAllocInstr>(expr, arrayType, sz);
    return;
  } else if (ei && startswith(ei->getValue(), "__internal__.yield_in_no_suspend")) {
    result = make<ir::YieldInInstr>(expr, getType(expr->getType()), false);
    return;
  }

  auto ft = expr->getExpr()->getType()->getFunc();
  seqassert(ft, "not calling function");
  auto callee = transform(expr->getExpr());
  bool isVariadic = ft->ast->hasAttribute(Attr::CVarArg);
  std::vector<ir::Value *> items;
  size_t i = 0;
  for (auto &a : *expr) {
    seqassert(!cast<EllipsisExpr>(a.value), "ellipsis not elided");
    if (i + 1 == expr->size() && isVariadic) {
      auto call = cast<CallExpr>(a.value);
      seqassert(call, "expected *args tuple: '{}'", call->toString(0));
      for (auto &arg : *call)
        items.emplace_back(transform(arg.value));
    } else {
      items.emplace_back(transform(a.value));
    }
    i++;
  }
  result = make<ir::CallInstr>(expr, callee, std::move(items));
}

void TranslateVisitor::visit(DotExpr *expr) {
  if (expr->getMember() == "__atomic__" || expr->getMember() == "__elemsize__" ||
      expr->getMember() == "__contents_atomic__") {
    auto ei = cast<IdExpr>(expr->getExpr());
    seqassert(ei, "expected IdExpr, got {}", *(expr->getExpr()));
    auto t = TypecheckVisitor(ctx->cache->typeCtx).extractType(ei->getType());
    auto type = ctx->find(t->realizedName())->getType();
    seqassert(type, "{} is not a type", ei->getValue());
    result = make<ir::TypePropertyInstr>(
        expr, type,
        expr->getMember() == "__atomic__"
            ? ir::TypePropertyInstr::Property::IS_ATOMIC
            : (expr->getMember() == "__contents_atomic__"
                   ? ir::TypePropertyInstr::Property::IS_CONTENT_ATOMIC
                   : ir::TypePropertyInstr::Property::SIZEOF));
  } else {
    result =
        make<ir::ExtractInstr>(expr, transform(expr->getExpr()), expr->getMember());
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
  auto *firstStage = transform((*expr)[0].expr);
  auto firstIsGen = isGen(firstStage);
  stages.emplace_back(firstStage, std::vector<ir::Value *>(), firstIsGen, false);

  // Pipeline without generators (just function call sugar)
  auto simplePipeline = !firstIsGen;
  for (auto i = 1; i < expr->size(); i++) {
    auto call = cast<CallExpr>((*expr)[i].expr);
    seqassert(call, "{} is not a call", *((*expr)[i].expr));

    auto fn = transform(call->getExpr());
    if (i + 1 != expr->size())
      simplePipeline &= !isGen(fn);

    std::vector<ir::Value *> args;
    args.reserve(call->size());
    for (auto &a : *call)
      args.emplace_back(cast<EllipsisExpr>(a.value) ? nullptr : transform(a.value));
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
    for (int i = 0; i < expr->size(); i++)
      if ((*expr)[i].op == "||>")
        stages[i].setParallel();
    // This is a statement in IR.
    ctx->getSeries()->push_back(make<ir::PipelineFlow>(expr, stages));
  }
}

void TranslateVisitor::visit(StmtExpr *expr) {
  auto *bodySeries = make<ir::SeriesFlow>(expr, "body");
  ctx->addSeries(bodySeries);
  for (auto &s : *expr)
    transform(s);
  ctx->popSeries();
  result = make<ir::FlowInstr>(expr, bodySeries, transform(expr->getExpr()));
}

/************************************************************************************/

ir::Value *TranslateVisitor::transform(Stmt *stmt) {
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
  for (auto *s : *stmt)
    transform(s);
}

void TranslateVisitor::visit(BreakStmt *stmt) { result = make<ir::BreakInstr>(stmt); }

void TranslateVisitor::visit(ContinueStmt *stmt) {
  result = make<ir::ContinueInstr>(stmt);
}

void TranslateVisitor::visit(ExprStmt *stmt) {
  IdExpr *ei = nullptr;
  auto ce = cast<CallExpr>(stmt->getExpr());
  if (ce && ((ei = cast<IdExpr>(ce->getExpr()))) &&
      ei->getValue() ==
          getMangledMethod("std.internal.core", "__internal__", "yield_final")) {
    result = make<ir::YieldInstr>(stmt, transform((*ce)[0].value), true);
    ctx->getBase()->setGenerator();
  } else {
    result = transform(stmt->getExpr());
  }
}

void TranslateVisitor::visit(AssignStmt *stmt) {
  if (stmt->getLhs() && cast<IdExpr>(stmt->getLhs()) &&
      cast<IdExpr>(stmt->getLhs())->getValue() == VAR_ARGV)
    return;

  auto lei = cast<IdExpr>(stmt->getLhs());
  seqassert(lei, "expected IdExpr, got {}", *(stmt->getLhs()));
  auto var = lei->getValue();

  auto isGlobal = in(ctx->cache->globals, var);
  ir::Var *v = nullptr;

  if (stmt->isUpdate()) {
    auto val = ctx->find(lei->getValue());
    seqassert(val && val->getVar(), "{} is not a variable", lei->getValue());
    v = val->getVar();

    if (!v->getType()) {
      v->setSrcInfo(stmt->getSrcInfo());
      v->setType(getType(stmt->getRhs()->getType()));
    }
    result = make<ir::AssignInstr>(stmt, v, transform(stmt->getRhs()));
    return;
  }

  if (!stmt->getLhs()->getType()->isInstantiated() ||
      (stmt->getLhs()->getType()->is(TYPE_TYPE)) ||
      stmt->getLhs()->getType()->getFunc()) {
    if (!cast<IdExpr>(stmt->getRhs())) {
      // Side effect
      result = transform(stmt->getRhs());
    }
    return; // type aliases/fn aliases etc
  }

  if (isGlobal) {
    seqassert(ctx->find(var) && ctx->find(var)->getVar(), "cannot find global '{}'",
              var);
    v = ctx->find(var)->getVar();
    v->setSrcInfo(stmt->getSrcInfo());
    v->setType(getType((stmt->getRhs() ? stmt->getRhs() : stmt->getLhs())->getType()));
  } else {
    v = make<ir::Var>(
        stmt, getType((stmt->getRhs() ? stmt->getRhs() : stmt->getLhs())->getType()),
        false, false, var);
    ctx->getBase()->push_back(v);
    ctx->add(TranslateItem::Var, var, v);
  }
  // Check if it is a C variable
  if (stmt->getLhs()->hasAttribute(Attr::ExprExternVar)) {
    v->setExternal();
    v->setName(ctx->cache->rev(var));
    v->setGlobal();
    return;
  }

  if (stmt->getRhs()) {
    result = make<ir::AssignInstr>(stmt, v, transform(stmt->getRhs()));
  }
}

void TranslateVisitor::visit(AssignMemberStmt *stmt) {
  result = make<ir::InsertInstr>(stmt, transform(stmt->getLhs()), stmt->getMember(),
                                 transform(stmt->getRhs()));
}

void TranslateVisitor::visit(ReturnStmt *stmt) {
  result = make<ir::ReturnInstr>(stmt, stmt->getExpr() ? transform(stmt->getExpr())
                                                       : nullptr);
}

void TranslateVisitor::visit(YieldStmt *stmt) {
  result = make<ir::YieldInstr>(stmt,
                                stmt->getExpr() ? transform(stmt->getExpr()) : nullptr);
  ctx->getBase()->setGenerator();
}

void TranslateVisitor::visit(WhileStmt *stmt) {
  auto loop = make<ir::WhileFlow>(stmt, transform(stmt->getCond()),
                                  make<ir::SeriesFlow>(stmt, "body"));
  ctx->addSeries(cast<ir::SeriesFlow>(loop->getBody()));
  transform(stmt->getSuite());
  ctx->popSeries();
  result = loop;
}

void TranslateVisitor::visit(ForStmt *stmt) {
  std::unique_ptr<OMPSched> os = nullptr;
  if (stmt->getDecorator()) {
    auto c = cast<CallExpr>(stmt->getDecorator());
    seqassert(c, "for par is not a call: {}", *(stmt->getDecorator()));
    auto fc = c->getExpr()->getType()->getFunc();
    seqassert(fc && fc->ast->getName() == getMangledFunc("std.openmp", "for_par"),
              "for par is not a function");
    auto schedule = fc->funcGenerics[0].type->getStrStatic()->value;
    bool ordered = fc->funcGenerics[1].type->getBoolStatic()->value;
    auto threads = transform((*c)[0].value);
    auto chunk = transform((*c)[1].value);
    auto collapse = fc->funcGenerics[2].type->getIntStatic()->value;
    bool gpu = fc->funcGenerics[3].type->getBoolStatic()->value;
    os = std::make_unique<OMPSched>(schedule, threads, chunk, ordered, collapse, gpu);
  }

  seqassert(cast<IdExpr>(stmt->getVar()), "expected IdExpr, got {}", *(stmt->getVar()));
  auto varName = cast<IdExpr>(stmt->getVar())->getValue();
  ir::Var *var = nullptr;
  if (!ctx->find(varName) || !stmt->getVar()->hasAttribute(Attr::ExprDominated)) {
    var =
        make<ir::Var>(stmt, getType(stmt->getVar()->getType()), false, false, varName);
  } else {
    var = ctx->find(varName)->getVar();
  }
  ctx->getBase()->push_back(var);
  auto bodySeries = make<ir::SeriesFlow>(stmt, "body");

  auto loop = make<ir::ForFlow>(stmt, transform(stmt->getIter()), bodySeries, var);
  if (os)
    loop->setSchedule(std::move(os));
  ctx->add(TranslateItem::Var, varName, var);
  ctx->addSeries(cast<ir::SeriesFlow>(loop->getBody()));
  transform(stmt->getSuite());
  ctx->popSeries();
  result = loop;
}

void TranslateVisitor::visit(IfStmt *stmt) {
  auto cond = transform(stmt->getCond());
  auto trueSeries = make<ir::SeriesFlow>(stmt, "ifstmt_true");
  ctx->addSeries(trueSeries);
  transform(stmt->getIf());
  ctx->popSeries();

  ir::SeriesFlow *falseSeries = nullptr;
  if (stmt->getElse()) {
    falseSeries = make<ir::SeriesFlow>(stmt, "ifstmt_false");
    ctx->addSeries(falseSeries);
    transform(stmt->getElse());
    ctx->popSeries();
  }
  result = make<ir::IfFlow>(stmt, cond, trueSeries, falseSeries);
}

void TranslateVisitor::visit(TryStmt *stmt) {
  auto *bodySeries = make<ir::SeriesFlow>(stmt, "body");
  ctx->addSeries(bodySeries);
  transform(stmt->getSuite());
  ctx->popSeries();

  ir::SeriesFlow *finallySeries = make<ir::SeriesFlow>(stmt, "finally");
  if (stmt->getFinally()) {
    ctx->addSeries(finallySeries);
    transform(stmt->getFinally());
    ctx->popSeries();
  }

  ir::SeriesFlow *elseSeries = nullptr;
  if (stmt->getElse()) {
    elseSeries = make<ir::SeriesFlow>(stmt, "else");
    ctx->addSeries(elseSeries);
    transform(stmt->getElse());
    ctx->popSeries();
  }

  auto *tc = make<ir::TryCatchFlow>(stmt, bodySeries, finallySeries, elseSeries);
  for (auto *c : *stmt) {
    auto *catchBody = make<ir::SeriesFlow>(stmt, "catch");
    auto *excType = c->getException()
                        ? getType(TypecheckVisitor(ctx->cache->typeCtx)
                                      .extractType(c->getException()->getType()))
                        : nullptr;
    ir::Var *catchVar = nullptr;
    if (!c->getVar().empty()) {
      if (!ctx->find(c->getVar()) || !c->hasAttribute(Attr::ExprDominated)) {
        catchVar = make<ir::Var>(stmt, excType, false, false, c->getVar());
      } else {
        catchVar = ctx->find(c->getVar())->getVar();
      }
      ctx->add(TranslateItem::Var, c->getVar(), catchVar);
      ctx->getBase()->push_back(catchVar);
    }
    ctx->addSeries(catchBody);
    transform(c->getSuite());
    ctx->popSeries();
    tc->push_back(ir::TryCatchFlow::Catch(catchBody, excType, catchVar));
  }
  result = tc;
}

void TranslateVisitor::visit(ThrowStmt *stmt) {
  result = make<ir::ThrowInstr>(stmt,
                                stmt->getExpr() ? transform(stmt->getExpr()) : nullptr);
}

void TranslateVisitor::visit(FunctionStmt *stmt) {
  // Process all realizations.
  transformFunctionRealizations(stmt->getName(), stmt->hasAttribute(Attr::LLVM));
}

void TranslateVisitor::visit(ClassStmt *stmt) {
  // Nothing to see here, as all type handles are already generated.
  // Methods will be handled by FunctionStmt visitor.
}

/************************************************************************************/

codon::ir::types::Type *TranslateVisitor::getType(types::Type *t) const {
  seqassert(t && t->getClass(), "not a class: {}", t ? t->debugString(2) : "-");
  std::string name = t->getClass()->ClassType::realizedName();
  auto i = ctx->find(name);
  seqassert(i, "type {} not realized: {}", t->debugString(2), name);
  seqassert(i->getType(), "type {} not IR-realized: {}", t->debugString(2), name);
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
      transformFunction(real.second->type.get(), ast, real.second->ir);
    else
      transformLLVMFunction(real.second->type.get(), ast, real.second->ir);
  }
}

void TranslateVisitor::transformFunction(const types::FuncType *type, FunctionStmt *ast,
                                         ir::Func *func) {
  std::vector<std::string> names;
  std::vector<int> indices;
  for (int i = 0, j = 0; i < ast->size(); i++)
    if ((*ast)[i].isValue()) {
      if (!(*type)[j]->getFunc()) {
        names.push_back(ctx->cache->rev((*ast)[i].name));
        indices.push_back(i);
      }
      j++;
    }
  if (ast->hasAttribute(Attr::CVarArg)) {
    names.pop_back();
    indices.pop_back();
  }
  // TODO: refactor IR attribute API
  std::unordered_map<std::string, std::string> attr;
  if (ast->hasAttribute(Attr::FunctionAttributes))
    attr =
        ast->getAttribute<ir::KeyValueAttribute>(Attr::FunctionAttributes)->attributes;
  attr[".module"] = ast->getAttribute<ir::StringValueAttribute>(Attr::Module)->value;
  func->setAttribute(std::make_unique<ir::KeyValueAttribute>(attr));
  for (int i = 0; i < names.size(); i++)
    func->getArgVar(names[i])->setSrcInfo((*ast)[indices[i]].getSrcInfo());
  // func->setUnmangledName(ctx->cache->reverseIdentifierLookup[type->ast->name]);
  if (!ast->hasAttribute(Attr::C) && !ast->hasAttribute(Attr::Internal)) {
    ctx->addBlock();
    for (auto i = 0; i < names.size(); i++)
      ctx->add(TranslateItem::Var, (*ast)[indices[i]].name, func->getArgVar(names[i]));
    auto body = make<ir::SeriesFlow>(ast, "body");
    ctx->bases.push_back(cast<ir::BodiedFunc>(func));
    ctx->addSeries(body);
    transform(ast->getSuite());
    ctx->popSeries();
    ctx->bases.pop_back();
    cast<ir::BodiedFunc>(func)->setBody(body);
    ctx->popBlock();
  }
}

void TranslateVisitor::transformLLVMFunction(types::FuncType *type, FunctionStmt *ast,
                                             ir::Func *func) const {
  std::vector<std::string> names;
  std::vector<int> indices;
  for (int i = 0, j = 1; i < ast->size(); i++)
    if ((*ast)[i].isValue()) {
      names.push_back(ctx->cache->reverseIdentifierLookup[(*ast)[i].name]);
      indices.push_back(i);
      j++;
    }
  auto f = cast<ir::LLVMFunc>(func);
  seqassert(f, "not a function");
  std::unordered_map<std::string, std::string> attr;
  if (ast->hasAttribute(Attr::FunctionAttributes))
    attr =
        ast->getAttribute<ir::KeyValueAttribute>(Attr::FunctionAttributes)->attributes;
  attr[".module"] = ast->getAttribute<ir::StringValueAttribute>(Attr::Module)->value;
  func->setAttribute(std::make_unique<ir::KeyValueAttribute>(attr));
  for (int i = 0; i < names.size(); i++)
    func->getArgVar(names[i])->setSrcInfo((*ast)[indices[i]].getSrcInfo());

  seqassert(
      ast->getSuite()->firstInBlock() &&
          cast<ExprStmt>(ast->getSuite()->firstInBlock()) &&
          cast<StringExpr>(cast<ExprStmt>(ast->getSuite()->firstInBlock())->getExpr()),
      "LLVM function does not begin with a string");
  std::istringstream sin(
      cast<StringExpr>(cast<ExprStmt>(ast->getSuite()->firstInBlock())->getExpr())
          ->getValue());
  std::vector<ir::types::Generic> literals;
  auto ss = cast<SuiteStmt>(ast->getSuite());
  for (int i = 1; i < ss->size(); i++) {
    if (auto sti = cast<ExprStmt>((*ss)[i])->getExpr()->getType()->getIntStatic()) {
      literals.emplace_back(sti->value);
    } else if (auto sts =
                   cast<ExprStmt>((*ss)[i])->getExpr()->getType()->getStrStatic()) {
      literals.emplace_back(sts->value);
    } else {
      seqassert(cast<ExprStmt>((*ss)[i])->getExpr()->getType(),
                "invalid LLVM type argument: {}", (*ss)[i]->toString(0));
      literals.emplace_back(
          getType(TypecheckVisitor(ctx->cache->typeCtx)
                      .extractType(cast<ExprStmt>((*ss)[i])->getExpr()->getType())));
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
