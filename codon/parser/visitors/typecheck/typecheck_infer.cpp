#include <limits>
#include <map>
#include <memory>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>

#include "codon/parser/ast.h"
#include "codon/parser/common.h"
#include "codon/parser/visitors/simplify/simplify.h"
#include "codon/parser/visitors/typecheck/typecheck.h"
#include "codon/sir/types/types.h"

using fmt::format;

namespace codon {
namespace ast {

using namespace types;

types::TypePtr TypecheckVisitor::realize(types::TypePtr typ) {
  if (!typ || !typ->canRealize()) {
    return nullptr;
  } else if (typ->getStatic()) {
    return typ;
  } else if (auto f = typ->getFunc()) {
    auto ret = realizeFunc(f.get());
    if (ret)
      realizeType(ret->getClass().get());
    return ret;
  } else if (auto c = typ->getClass()) {
    auto t = realizeType(c.get());
    if (auto p = typ->getPartial()) {
      //   if (auto rt = realize(p->func))
      //     unify(rt, p->func);
      return std::make_shared<PartialType>(t->getRecord(), p->func, p->known);
    }
    return t;
  } else {
    return nullptr;
  }
}

types::TypePtr TypecheckVisitor::realizeType(types::ClassType *type) {
  seqassert(type && type->canRealize(), "{} not realizable", type->toString());

  // We are still not done with type creation... (e.g. class X: x: List[X])
  for (auto &m : ctx->cache->classes[type->name].fields)
    if (!m.type)
      return nullptr;

  auto realizedName = type->realizedTypeName();
  try {
    ClassTypePtr realizedType = nullptr;
    auto it = ctx->cache->classes[type->name].realizations.find(realizedName);
    if (it != ctx->cache->classes[type->name].realizations.end()) {
      realizedType = it->second->type->getClass();
    } else {
      realizedType = type->getClass();
      if (type->getFunc()) { // make sure to realize function stub only and cache fn stub
        realizedType = std::make_shared<RecordType>(realizedType, type->getFunc()->args);
      }

      // Realize generics
      for (auto &e : realizedType->generics)
        if (!realize(e.type))
          return nullptr;
      realizedName = realizedType->realizedTypeName();

      LOG_REALIZE("[realize] ty {} -> {}", type->name, realizedName);
      // Realizations are stored in the top-most base.
      ctx->bases[0].visitedAsts[realizedName] = {TypecheckItem::Type, realizedType};
      auto r = ctx->cache->classes[realizedType->name].realizations[realizedName] =
          std::make_shared<Cache::Class::ClassRealization>();
      r->type = realizedType;
      // Realize arguments
      if (auto tr = realizedType->getRecord())
        for (auto &a : tr->args)
          realize(a);
      auto lt = getLLVMType(realizedType.get());
      // Realize fields.
      std::vector<ir::types::Type *> typeArgs;
      std::vector<std::string> names;
      std::map<std::string, SrcInfo> memberInfo;
      for (auto &m : ctx->cache->classes[realizedType->name].fields) {
        auto mt = ctx->instantiate(N<IdExpr>(m.name).get(), m.type, realizedType.get());
        LOG_REALIZE("- member: {} -> {}: {}", m.name, m.type->toString(),
                    mt->toString());
        auto tf = realize(mt);
        if (!tf)
          error("cannot realize {}.{} of type {}",
                ctx->cache->reverseIdentifierLookup[realizedType->name], m.name,
                mt->toString());
        // seqassert(tf, "cannot realize {}.{}: {}", realizedName, m.name,
        // mt->toString());
        r->fields.emplace_back(m.name, tf);
        names.emplace_back(m.name);
        typeArgs.emplace_back(getLLVMType(tf->getClass().get()));
        memberInfo[m.name] = m.type->getSrcInfo();
      }
      if (auto *cls = ir::cast<ir::types::RefType>(lt))
        if (!names.empty()) {
          cls->getContents()->realize(typeArgs, names);
          cls->setAttribute(std::make_unique<ir::MemberAttribute>(memberInfo));
          cls->getContents()->setAttribute(
              std::make_unique<ir::MemberAttribute>(memberInfo));
        }
    }
    return realizedType;
  } catch (exc::ParserException &e) {
    e.trackRealize(type->toString(), getSrcInfo());
    throw;
  }
}

types::TypePtr TypecheckVisitor::realizeFunc(types::FuncType *type) {
  seqassert(type->canRealize(), "{} not realizable", type->toString());

  try {
    auto it =
        ctx->cache->functions[type->ast->name].realizations.find(type->realizedName());
    if (it != ctx->cache->functions[type->ast->name].realizations.end()) {
      return it->second->type;
    }

    // Set up bases. Ensure that we have proper parent bases even during a realization
    // of mutually recursive functions.
    int depth = 1;
    for (auto p = type->funcParent; p;) {
      if (auto f = p->getFunc()) {
        depth++;
        p = f->funcParent;
      } else {
        break;
      }
    }
    auto oldBases = std::vector<TypeContext::RealizationBase>(
        ctx->bases.begin() + depth, ctx->bases.end());
    while (ctx->bases.size() > depth)
      ctx->bases.pop_back();
    if (ctx->realizationDepth > 500)
      codon::compilationError(
          "maximum realization depth exceeded (recursive static function?)",
          getSrcInfo().file, getSrcInfo().line, getSrcInfo().col);

    // Special cases: Tuple.(__iter__, __getitem__).
    if (startswith(type->ast->name, TYPE_TUPLE) &&
        (endswith(type->ast->name, ".__iter__") ||
         endswith(type->ast->name, ".__getitem__")) &&
        type->getArgTypes()[0]->getHeterogenousTuple())
      error("cannot iterate a heterogeneous tuple");

    LOG_REALIZE("[realize] fn {} -> {} : base {} ; depth = {}", type->ast->name,
                type->realizedName(), ctx->getBase(), depth);
    {
      // Timer trx(fmt::format("fn {}", type->realizedName()));
      getLogger().level++;
      ctx->realizationDepth++;
      ctx->addBlock();
      ctx->typecheckLevel++;

      // Find parents!
      ctx->bases.push_back({type->ast->name,
                            type->getFunc(),
                            type->getRetType(),
                            {},
                            findSuperMethods(type->getFunc())});
      auto clonedAst = ctx->cache->functions[type->ast->name].ast->clone();
      auto *ast = (FunctionStmt *)clonedAst.get();
      addFunctionGenerics(type);

      // There is no AST linked to internal functions, so make sure not to parse it.
      bool isInternal = ast->attributes.has(Attr::Internal);
      isInternal |= ast->suite == nullptr;
      // Add function arguments.
      if (!isInternal)
        for (int i = 0, j = 0; i < ast->args.size(); i++)
          if (ast->args[i].status == Param::Normal) {
            std::string varName = ast->args[i].name;
            trimStars(varName);
            ctx->add(TypecheckItem::Var, varName,
                     std::make_shared<LinkType>(type->getArgTypes()[j++]));
            // N.B. this used to be:
            // seqassert(type->args[j] && type->args[j]->getUnbounds().empty(),
            // "unbound argument {}", type->args[j]->toString());
            // type->args[j++]->generalize(ctx->typecheckLevel)
            // no idea why... most likely an old artefact, BUT if seq or sequre
            // fail with weird type errors try returning this and see if it works
          }

      // Need to populate realization table in advance to make recursive functions
      // work.
      auto oldKey = type->realizedName();
      auto r =
          ctx->cache->functions[type->ast->name].realizations[type->realizedName()] =
              std::make_shared<Cache::Function::FunctionRealization>();
      r->type = type->getFunc();
      // Realizations are stored in the top-most base.
      ctx->bases[0].visitedAsts[type->realizedName()] = {TypecheckItem::Func,
                                                         type->getFunc()};

      StmtPtr realized = nullptr;
      if (!isInternal) {
        auto oldBlockLevel = ctx->blockLevel;
        auto oldReturnEarly = ctx->returnEarly;
        ctx->blockLevel = 0;
        ctx->returnEarly = false;

        auto suite = ast->suite;
        if (startswith(type->ast->name, "Function.__call__")) {
          std::vector<StmtPtr> items;
          items.push_back(nullptr);
          std::vector<std::string> ll;
          std::vector<std::string> lla;
          auto &as = type->getArgTypes()[1]->getRecord()->args;
          auto ag = ast->args[1].name;
          trimStars(ag);

          for (int i = 0; i < as.size(); i++) {
            ll.push_back(format("%{} = extractvalue {{}} %args, {}", i, i));
            items.push_back(N<ExprStmt>(N<IdExpr>(ag)));
          }
          items.push_back(N<ExprStmt>(N<IdExpr>("TR")));
          for (int i = 0; i < as.size(); i++) {
            items.push_back(N<ExprStmt>(N<IndexExpr>(N<IdExpr>(ag), N<IntExpr>(i))));
            lla.push_back(format("{{}} %{}", i));
          }
          items.push_back(N<ExprStmt>(N<IdExpr>("TR")));
          if (type->getRetType()->getClass()->name == "void") {
            ll.push_back(format("call {{}} %self({})", combine2(lla)));
            ll.push_back(format("ret {{}}"));
          } else {
            ll.push_back(format("%{} = call {{}} %self({})", as.size(), combine2(lla)));
            ll.push_back(format("ret {{}} %{}", as.size()));
          }
          items[0] = N<ExprStmt>(N<StringExpr>(combine2(ll, "\n")));
          suite = N<SuiteStmt>(items);
        }

        realized = inferTypes(suite, false, type->realizedName()).second;
        ctx->blockLevel = oldBlockLevel;
        ctx->returnEarly = oldReturnEarly;
        if (ast->attributes.has(Attr::LLVM)) {
          auto s = realized->getSuite();
          for (int i = 1; i < s->stmts.size(); i++) {
            seqassert(s->stmts[i]->getExpr(), "invalid LLVM definition {}: {}",
                      type->toString(), s->stmts[i]->toString());
            auto e = s->stmts[i]->getExpr()->expr;
            // if (!e->isType() && !e->isStatic())
            //   error(e, "not a type or static expression");
          }
        }
        // Return type was not specified and the function returned nothing.
        if (!ast->ret && type->getRetType()->getUnbound()) {
          auto tt = type->getRetType();
          unify(tt, ctx->findInternal("void"));
        }
      }
      // Realize the return type.
      if (auto t = realize(type->getRetType())) {
        auto tt = type->getRetType();
        unify(tt, t);
      }
      LOG_REALIZE("[realize] done with {} / {} =>{}", type->realizedName(), oldKey,
                  time);
      // trx.log();

      // Create and store IR node and a realized AST to be used
      // during the code generation.
      if (!in(ctx->cache->pendingRealizations,
              make_pair(type->ast->name, type->realizedName()))) {
        if (ast->attributes.has(Attr::Internal)) {
          // This is either __new__, Ptr.__new__, etc.
          r->ir = ctx->cache->module->Nr<ir::InternalFunc>(type->ast->name);
        } else if (ast->attributes.has(Attr::LLVM)) {
          r->ir = ctx->cache->module->Nr<ir::LLVMFunc>(type->realizedName());
        } else if (ast->attributes.has(Attr::C)) {
          r->ir = ctx->cache->module->Nr<ir::ExternalFunc>(type->realizedName());
        } else {
          r->ir = ctx->cache->module->Nr<ir::BodiedFunc>(type->realizedName());
        }
        r->ir->setUnmangledName(ctx->cache->reverseIdentifierLookup[type->ast->name]);

        auto parent = type->funcParent;
        if (!ast->attributes.parentClass.empty() &&
            !ast->attributes.has(Attr::Method)) // hack for non-generic types
          parent = ctx->find(ast->attributes.parentClass)->type;
        if (parent && parent->canRealize()) {
          parent = realize(parent);
          r->ir->setParentType(getLLVMType(parent->getClass().get()));
        }
        r->ir->setGlobal();

        ctx->cache->pendingRealizations.insert({type->ast->name, type->realizedName()});

        seqassert(!type || ast->args.size() ==
                               type->getArgTypes().size() + type->funcGenerics.size(),
                  "type/AST argument mismatch");
        std::vector<Param> args;
        for (auto &i : ast->args) {
          std::string varName = i.name;
          trimStars(varName);
          args.emplace_back(Param{varName, nullptr, nullptr, i.status});
        }
        r->ast = N<FunctionStmt>(type->realizedName(), nullptr, args, realized);
        r->ast->setSrcInfo(ast->getSrcInfo());
        r->ast->attributes = ast->attributes; // assign later to prevent validation

        // Set up IR node
        std::vector<std::string> names;
        std::vector<codon::ir::types::Type *> types;
        for (int i = 0, j = 0; i < r->ast->args.size(); i++)
          if (r->ast->args[i].status == Param::Normal) {
            if (!type->getArgTypes()[j]->getFunc()) {
              types.push_back(getLLVMType(type->getArgTypes()[j]->getClass().get()));
              names.push_back(
                  ctx->cache->reverseIdentifierLookup[r->ast->args[i].name]);
            }
            j++;
          }
        if (r->ast->hasAttr(Attr::CVarArg)) {
          types.pop_back();
          names.pop_back();
        }
        auto irType = ctx->cache->module->unsafeGetFuncType(
            type->realizedName(), getLLVMType(type->getRetType()->getClass().get()),
            types, r->ast->hasAttr(Attr::CVarArg));
        irType->setAstType(type->getFunc());
        r->ir->realize(irType, names);

        ctx->cache->functions[type->ast->name].realizations[type->realizedName()] = r;
      } else {
        ctx->cache->functions[type->ast->name].realizations[oldKey] =
            ctx->cache->functions[type->ast->name].realizations[type->realizedName()];
      }
      ctx->bases[0].visitedAsts[type->realizedName()] = {TypecheckItem::Func,
                                                         type->getFunc()};
      ctx->bases.pop_back();
      ctx->popBlock();
      ctx->typecheckLevel--;
      ctx->realizationDepth--;
      getLogger().level--;
    }
    // Restore old bases back.
    ctx->bases.insert(ctx->bases.end(), oldBases.begin(), oldBases.end());
    return type->getFunc();
  } catch (exc::ParserException &e) {
    e.trackRealize(fmt::format("{} (arguments {})", type->ast->name, type->toString()),
                   getSrcInfo());
    throw;
  }
}

std::pair<int, StmtPtr> TypecheckVisitor::inferTypes(StmtPtr result, bool keepLast,
                                                     const std::string &name) {
  if (!result)
    return {0, nullptr};

  // We keep running typecheck transformations until there are no more unbound
  // types. It is assumed that the unbound count will decrease in each
  // iteration--- if not, the program cannot be type-checked.
  int minUnbound = ctx->cache->unboundCount;
  ctx->addBlock();
  int iteration = 0;
  for (int prevSize = std::numeric_limits<int>::max();;) {
    LOG_TYPECHECK("== iter {} ==========================================", iteration);
    ctx->typecheckLevel++;
    auto old = ctx->changedNodes;
    ctx->changedNodes = 0;
    result = TypecheckVisitor(ctx).transform(result);
    std::swap(ctx->changedNodes, old);
    iteration++;
    ctx->typecheckLevel--;

    if (iteration == 1 && name == "<top>")
      for (auto &f : ctx->cache->functions) {
        auto &attr = f.second.ast->attributes;
        if (f.second.type && f.second.realizations.empty() &&
            (attr.has(Attr::ForceRealize) ||
             (attr.has(Attr::C) && !attr.has(Attr::CVarArg)))) {
          seqassert(f.second.type->canRealize(), "cannot realize {}", f.first);
          auto e = std::make_shared<IdExpr>(f.second.type->ast->name);
          auto t = ctx->instantiate(e.get(), f.second.type, nullptr, false)->getFunc();
          realize(t);
          seqassert(!f.second.realizations.empty(), "cannot realize {}", f.first);
        }
      }
    if (result->done) {
      break;
    } else if (old) {
      continue;
    } else {
      bool fixed = false;
      for (auto &ub : ctx->pendingDefaults) {
        if (auto u = ub->getLink()) {
          types::Type::Unification undo;
          undo.realizator = this;
          if (u->unify(u->defaultType.get(), &undo) >= 0) {
            fixed = true;
          }
        }
      }
      ctx->pendingDefaults.clear();
      if (fixed)
        continue;
      // std::map<int, std::pair<codon::SrcInfo, std::string>> v;
      // for (auto &ub : ctx->activeUnbounds)
      //   if (ub.first->getLink()->id >= minUnbound) {
      //     v[ub.first->getLink()->id] = {ub.first->getSrcInfo(), ub.second};
      //     LOG_TYPECHECK("dangling ?{} ({})", ub.first->getLink()->id, minUnbound);
      //   }
      // for (auto &ub : v) {
      //   codon::compilationError(
      //       format("cannot infer the type of {}", ub.second.second),
      //       ub.second.first.file, ub.second.first.line, ub.second.first.col,
      //       /*terminate=*/false);
      //   LOG_TYPECHECK("cannot infer {} / {}", ub.first, ub.second.second);
      // }
      // LOG_TYPECHECK("-- {}", result->toString(0));
      if (codon::getLogger().flags & codon::Logger::FLAG_USER) {
        auto fo = fopen("_dump_typecheck_error.sexp", "w");
        fmt::print(fo, "{}\n", result->toString(0));
        fclose(fo);
      }
      error("cannot typecheck the program");
    }
  }
  if (!keepLast)
    ctx->popBlock();

  return {iteration, result};
}

ir::types::Type *TypecheckVisitor::getLLVMType(const types::ClassType *t) {
  auto realizedName = t->realizedTypeName();
  if (!in(ctx->cache->classes[t->name].realizations, realizedName))
    realizeType(const_cast<types::ClassType *>(t));
  if (auto l = ctx->cache->classes[t->name].realizations[realizedName]->ir)
    return l;
  auto getLLVM = [&](const TypePtr &tt) {
    auto t = tt->getClass();
    seqassert(t && in(ctx->cache->classes[t->name].realizations, t->realizedTypeName()),
              "{} not realized", tt->toString());
    auto l = ctx->cache->classes[t->name].realizations[t->realizedTypeName()]->ir;
    seqassert(l, "no LLVM type for {}", t->toString());
    return l;
  };

  ir::types::Type *handle = nullptr;
  std::vector<ir::types::Type *> types;
  std::vector<StaticValue *> statics;
  for (auto &m : t->generics)
    if (auto s = m.type->getStatic()) {
      seqassert(s->expr->staticValue.evaluated, "static not realized");
      statics.push_back(&(s->expr->staticValue));
    } else {
      types.push_back(getLLVM(m.type));
    }
  auto name = t->name;
  auto *module = ctx->cache->module;

  if (name == "void") {
    handle = module->getVoidType();
  } else if (name == "bool") {
    handle = module->getBoolType();
  } else if (name == "byte") {
    handle = module->getByteType();
  } else if (name == "int") {
    handle = module->getIntType();
  } else if (name == "float") {
    handle = module->getFloatType();
  } else if (name == "str") {
    handle = module->getStringType();
  } else if (name == "Int" || name == "UInt") {
    seqassert(statics.size() == 1 && statics[0]->type == StaticValue::INT &&
                  types.empty(),
              "bad generics/statics");
    handle = module->Nr<ir::types::IntNType>(statics[0]->getInt(), name == "Int");
  } else if (name == "Ptr") {
    seqassert(types.size() == 1 && statics.empty(), "bad generics/statics");
    handle = module->unsafeGetPointerType(types[0]);
  } else if (name == "Generator") {
    seqassert(types.size() == 1 && statics.empty(), "bad generics/statics");
    handle = module->unsafeGetGeneratorType(types[0]);
  } else if (name == TYPE_OPTIONAL) {
    seqassert(types.size() == 1 && statics.empty(), "bad generics/statics");
    handle = module->unsafeGetOptionalType(types[0]);
  } else if (name == "NoneType") {
    seqassert(types.empty() && statics.empty(), "bad generics/statics");
    auto record =
        ir::cast<ir::types::RecordType>(module->unsafeGetMemberedType(realizedName));
    record->realize({}, {});
    handle = record;
  } else if (name == "Function") {
    types.clear();
    for (auto &m : t->generics[0].type->getRecord()->args)
      types.push_back(getLLVM(m));
    auto ret = getLLVM(t->generics[1].type);
    handle = module->unsafeGetFuncType(realizedName, ret, types);
  } else if (auto tr = const_cast<ClassType *>(t)->getRecord()) {
    std::vector<ir::types::Type *> typeArgs;
    std::vector<std::string> names;
    std::map<std::string, SrcInfo> memberInfo;
    for (int ai = 0; ai < tr->args.size(); ai++) {
      names.emplace_back(ctx->cache->classes[t->name].fields[ai].name);
      typeArgs.emplace_back(getLLVM(tr->args[ai]));
      memberInfo[ctx->cache->classes[t->name].fields[ai].name] =
          ctx->cache->classes[t->name].fields[ai].type->getSrcInfo();
    }
    auto record =
        ir::cast<ir::types::RecordType>(module->unsafeGetMemberedType(realizedName));
    record->realize(typeArgs, names);
    handle = record;
    handle->setAttribute(std::make_unique<ir::MemberAttribute>(std::move(memberInfo)));
  } else {
    // Type arguments will be populated afterwards to avoid infinite loop with recursive
    // reference types.
    handle = module->unsafeGetMemberedType(realizedName, true);
  }
  handle->setSrcInfo(t->getSrcInfo());
  handle->setAstType(
      std::const_pointer_cast<codon::ast::types::Type>(t->shared_from_this()));
  // Not needed for classes, I guess
  //  if (auto &ast = ctx->cache->classes[t->name].ast)
  //    handle->setAttribute(std::make_unique<ir::KeyValueAttribute>(ast->attributes));
  return ctx->cache->classes[t->name].realizations[realizedName]->ir = handle;
}

} // namespace ast
} // namespace codon
