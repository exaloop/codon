// Copyright (C) 2022-2023 Exaloop Inc. <https://exaloop.io>

#include <limits>
#include <map>
#include <memory>
#include <string>
#include <tuple>
#include <vector>

#include "codon/cir/types/types.h"
#include "codon/parser/ast.h"
#include "codon/parser/common.h"
#include "codon/parser/visitors/typecheck/typecheck.h"

using fmt::format;
using namespace codon::error;

const int MAX_TYPECHECK_ITER = 1000;

namespace codon::ast {

using namespace types;

/// Unify types a (passed by reference) and b.
/// Destructive operation as it modifies both a and b. If types cannot be unified, raise
/// an error.
/// @param a Type (by reference)
/// @param b Type
/// @return a
TypePtr TypecheckVisitor::unify(TypePtr &a, const TypePtr &b) {
  if (!a)
    return a = b;
  seqassert(b, "rhs is nullptr");
  types::Type::Unification undo;
  if (a->unify(b.get(), &undo) >= 0) {
    return a;
  } else {
    undo.undo();
  }
  a->unify(b.get(), &undo);
  E(Error::TYPE_UNIFY, getSrcInfo(), a->prettyString(), b->prettyString());
  return nullptr;
}

/// Infer all types within a StmtPtr. Implements the LTS-DI typechecking.
/// @param isToplevel set if typechecking the program toplevel.
StmtPtr TypecheckVisitor::inferTypes(StmtPtr result, bool isToplevel) {
  if (!result)
    return nullptr;

  for (ctx->getBase()->iteration = 1;; ctx->getBase()->iteration++) {
    LOG_TYPECHECK("[iter] {} :: {}", ctx->getBase()->name, ctx->getBase()->iteration);
    if (ctx->getBase()->iteration >= MAX_TYPECHECK_ITER) {
      LOG("[error=>] {}", result->toString(2));
      error(result, "cannot typecheck '{}' in reasonable time", ctx->getBase()->name);
    }

    // Keep iterating until:
    //   (1) success: the statement is marked as done; or
    //   (2) failure: no expression or statements were marked as done during an
    //                iteration (i.e., changedNodes is zero)
    ctx->typecheckLevel++;
    auto changedNodes = ctx->changedNodes;
    ctx->changedNodes = 0;
    auto returnEarly = ctx->returnEarly;
    ctx->returnEarly = false;
    auto tv = TypecheckVisitor(ctx, preamble);
    tv.transform(result);
    std::swap(ctx->changedNodes, changedNodes);
    std::swap(ctx->returnEarly, returnEarly);
    ctx->typecheckLevel--;

    if (ctx->getBase()->iteration == 1 && isToplevel) {
      // Realize all @force_realize functions
      for (auto &f : ctx->cache->functions) {
        auto &attr = f.second.ast->attributes;
        if (f.second.type && f.second.realizations.empty() &&
            (attr.has(Attr::ForceRealize) || attr.has(Attr::Export) ||
             (attr.has(Attr::C) && !attr.has(Attr::CVarArg)))) {
          seqassert(f.second.type->canRealize(), "cannot realize {}", f.first);
          realize(ctx->instantiate(f.second.type)->getFunc());
          seqassert(!f.second.realizations.empty(), "cannot realize {}", f.first);
        }
      }
    }

    if (result->isDone()) {
      break;
    } else if (changedNodes) {
      continue;
    } else {
      // Special case: nothing was changed, however there are unbound types that have
      // default values (e.g., generics with default values). Unify those types with
      // their default values and then run another round to see if anything changed.
      bool anotherRound = false;
      // Special case: return type might have default as well (e.g., Union)
      if (ctx->getBase()->returnType)
        ctx->pendingDefaults.insert(ctx->getBase()->returnType);
      for (auto &unbound : ctx->pendingDefaults) {
        if (auto tu = unbound->getUnion()) {
          // Seal all dynamic unions after the iteration is over
          if (!tu->isSealed()) {
            tu->seal();
            anotherRound = true;
          }
        } else if (auto u = unbound->getLink()) {
          types::Type::Unification undo;
          if (u->defaultType &&
              u->unify(ctx->getType(u->defaultType).get(), &undo) >= 0) {
            anotherRound = true;
          }
        }
      }
      ctx->pendingDefaults.clear();
      if (anotherRound)
        continue;

      // Nothing helps. Return nullptr.
      return nullptr;
    }
  }

  return result;
}

/// Realize a type and create IR type stub. If type is a function type, also realize the
/// underlying function and generate IR function stub.
/// @return realized type or nullptr if the type cannot be realized
types::TypePtr TypecheckVisitor::realize(types::TypePtr typ) {
  if (!typ || !typ->canRealize()) {
    return nullptr;
  }

  try {
    if (auto f = typ->getFunc()) {
      if (auto ret = realizeFunc(f.get())) {
        // Realize Function[..] type as well
        realizeType(ret->getClass().get());
        // Needed for return type unification
        unify(f->getRetType(), ret->getClass()->generics[1].type);
        // return unify(ret, typ); // Needed for return type unification
        return ret;
      }
    } else if (auto c = typ->getClass()) {
      return realizeType(c.get());
      // if (t) {
      //   return unify(t, typ);
      // }
    }
  } catch (exc::ParserException &e) {
    if (e.errorCode == Error::MAX_REALIZATION)
      throw;
    if (auto f = typ->getFunc()) {
      if (f->ast->attributes.has(Attr::HiddenFromUser)) {
        e.locations.back() = getSrcInfo();
      } else {
        std::vector<std::string> args;
        for (size_t i = 0, ai = 0, gi = 0; i < f->ast->args.size(); i++) {
          auto an = f->ast->args[i].name;
          auto ns = trimStars(an);
          args.push_back(fmt::format("{}{}: {}", std::string(ns, '*'),
                                     ctx->cache->rev(an),
                                     f->ast->args[i].status == Param::Generic
                                         ? f->funcGenerics[gi++].type->prettyString()
                                         : f->getArgTypes()[ai++]->prettyString()));
        }
        auto name = f->ast->name;
        std::string name_args;
        if (startswith(name, "%_import_")) {
          name = name.substr(9);
          auto p = name.rfind('_');
          if (p != std::string::npos)
            name = name.substr(0, p);
          name = "<import " + name + ">";
        } else {
          name = ctx->cache->rev(f->ast->name);
          name_args = fmt::format("({})", fmt::join(args, ", "));
        }
        e.trackRealize(fmt::format("{}{}", name, name_args), getSrcInfo());
      }

    } else {
      e.trackRealize(typ->prettyString(), getSrcInfo());
    }
    throw;
  }
  return nullptr;
}

/// Realize a type and create IR type stub.
/// @return realized type or nullptr if the type cannot be realized
types::TypePtr TypecheckVisitor::realizeType(types::ClassType *type) {
  if (!type || !type->canRealize())
    return nullptr;
  // type->_rn = type->ClassType::realizedName();

  // Check if the type fields are all initialized
  // (sometimes that's not the case: e.g., `class X: x: List[X]`)
  for (auto &field : ctx->cache->classes[type->name].fields) {
    if (!field.type)
      return nullptr;
  }

  // Check if the type was already realized
  auto rn = type->ClassType::realizedName();
  if (auto r = in(ctx->cache->classes[type->name].realizations, rn)) {
    return (*r)->type->getClass();
  }

  auto realized = type->getClass();
  if (auto s = type->getStatic())
    realized = ctx->getType(s->name)->getClass();

  if (type->getFunc()) {
    // Just realize the function stub
    realized = std::make_shared<ClassType>(realized);
  }

  // Realize generics
  for (auto &e : realized->generics) {
    if (!realize(e.type))
      return nullptr;
  }

  // LOG("[realize] T {} -> {}", realized->debugString(2), realized->realizedName());

  // Realizations should always be visible, so add them to the toplevel
  auto val = std::make_shared<TypecheckItem>(realized->realizedName(), "",
                                             ctx->getModule(), realized);
  if (!val->type->is("type"))
    val->type = ctx->instantiateGeneric(ctx->getType("type"), {realized});
  ctx->addAlwaysVisible(val);
  auto realization =
      ctx->cache->classes[realized->name].realizations[realized->realizedName()] =
          std::make_shared<Cache::Class::ClassRealization>();
  realization->type = realized;
  realization->id = ctx->cache->classRealizationCnt++;

  // Create LLVM stub
  auto lt = makeIRType(realized.get());

  // Realize fields
  std::vector<ir::types::Type *> typeArgs;   // needed for IR
  std::vector<std::string> names;            // needed for IR
  std::map<std::string, SrcInfo> memberInfo; // needed for IR

  const auto &fields = ctx->cache->classes[realized->name].fields;
  auto fTypes = getClassFieldTypes(realized);
  for (size_t i = 0; i < fTypes.size(); i++) {
    if (!realize(fTypes[i])) {
      realize(fTypes[i]);
      E(Error::TYPE_CANNOT_REALIZE_ATTR, getSrcInfo(), fields[i].name,
        realized->prettyString());
    }
    // LOG_REALIZE("- member: {} -> {}: {}", field.name, field.type, fTypes[i]);
    realization->fields.emplace_back(fields[i].name, fTypes[i]);
    names.emplace_back(fields[i].name);
    typeArgs.emplace_back(makeIRType(fTypes[i]->getClass().get()));
    memberInfo[fields[i].name] = fTypes[i]->getSrcInfo();
  }

  // Set IR attributes
  if (!names.empty()) {
    if (auto *cls = ir::cast<ir::types::RefType>(lt)) {
      cls->getContents()->realize(typeArgs, names);
      cls->setAttribute(std::make_unique<ir::MemberAttribute>(memberInfo));
      cls->getContents()->setAttribute(
          std::make_unique<ir::MemberAttribute>(memberInfo));
    }
  }

  // Fix for partial types
  // if (auto p = type->getPartial()) {
  //   auto pt = std::make_shared<PartialType>(realized->getRecord(), p->func,
  //   p->known); auto val =
  //       std::make_shared<TypecheckItem>(pt->realizedName(), "", ctx->getModule(),
  //       pt);
  //   ctx->addAlwaysVisible(val);
  //   ctx->cache->classes[pt->name].realizations[pt->realizedName()] =
  //       ctx->cache->classes[realized->name].realizations[realized->realizedName()];
  // }

  return realized;
}

types::TypePtr TypecheckVisitor::realizeFunc(types::FuncType *type, bool force) {
  auto &realizations = ctx->cache->functions[type->ast->name].realizations;
  if (auto r = in(realizations, type->realizedName())) {
    if (!force) {
      return (*r)->type;
    }
  }

  seqassert(in(ctx->cache->imports, type->ast->attributes.module) != nullptr,
            "bad module: '{}'", type->ast->attributes.module);
  auto &imp = ctx->cache->imports[type->ast->attributes.module];
  auto oldCtx = this->ctx;
  this->ctx = imp.ctx;
  // LOG("=> {}", ctx->moduleName.module, ctx->moduleName.path);

  if (ctx->getRealizationDepth() > MAX_REALIZATION_DEPTH) {
    E(Error::MAX_REALIZATION, getSrcInfo(), ctx->cache->rev(type->ast->name));
  }

  if (!startswith(type->ast->name, "%_import_")) {
    getLogger().level++;
    ctx->addBlock();
    ctx->typecheckLevel++;
  }

  // Find function parents
  if (!startswith(type->ast->name, "%_import_")) {
    ctx->bases.push_back({type->ast->name, type->getFunc(), type->getRetType()});
    // LOG("[realize] F {} -> {} : base {} ; depth = {} ; ctx-base: {}; ret = {}",
    //     type->ast->name, type->realizedName(), ctx->getRealizationStackName(),
    //     ctx->getRealizationDepth(), ctx->getBaseName(),
    //     ctx->getBase()->returnType->debugString(2));
  }

  // Clone the generic AST that is to be realized
  auto ast = generateSpecialAst(type);
  addFunctionGenerics(type);
  ctx->getBase()->attributes = &(ast->attributes);

  // Internal functions have no AST that can be realized
  bool hasAst = ast->suite && !ast->attributes.has(Attr::Internal);
  // Add function arguments
  for (auto &[c, t] : ast->attributes.captures) {
    if (t == Attr::CaptureType::Global) {
      auto cp = ctx->find(c);
      if (!cp)
        E(Error::ID_NOT_FOUND, getSrcInfo(), c);
      if (!cp->isGlobal())
        E(Error::FN_GLOBAL_NOT_FOUND, getSrcInfo(), "global", c);
    }
  }
  // Add self reference! TODO: maybe remove later when doing contexts?
  if (ast->attributes.parentClass.empty())
    ctx->addFunc(ctx->cache->rev(ast->name), ast->name, ctx->find(ast->name)->type);
  for (size_t i = 0, j = 0; hasAst && i < ast->args.size(); i++)
    if (ast->args[i].status == Param::Normal) {
      std::string varName = ast->args[i].name;
      trimStars(varName);
      auto v = ctx->addVar(ctx->cache->rev(varName), varName,
                           std::make_shared<LinkType>(type->getArgTypes()[j++]));
      // LOG("[param] {} -> {}", v->canonicalName, v->type);
    }

  // Populate realization table in advance to support recursive realizations
  auto key = type->realizedName(); // note: the key might change later
  ir::Func *oldIR = nullptr;       // Get it if it was already made (force mode)
  if (auto i = in(realizations, key))
    oldIR = (*i)->ir;
  auto r = realizations[key] = std::make_shared<Cache::Function::FunctionRealization>();
  r->type = type->getFunc();
  r->ir = oldIR;

  // Realizations should always be visible, so add them to the toplevel
  auto val =
      std::make_shared<TypecheckItem>(key, "", ctx->getModule(), type->getFunc());
  ctx->addAlwaysVisible(val);

  ctx->getBase()->suite = clean_clone(ast->suite);
  if (hasAst) {
    auto oldBlockLevel = ctx->blockLevel;
    ctx->blockLevel = 0;
    auto ret = inferTypes(ctx->getBase()->suite);
    ctx->blockLevel = oldBlockLevel;

    if (!ret) {
      realizations.erase(key);
      if (!startswith(ast->name, "%_lambda")) {
        // Lambda typecheck failures are "ignored" as they are treated as statements,
        // not functions.
        // TODO: generalize this further.
        for (size_t w = ctx->bases.size(); w-- > 0;)
          if (ctx->bases[w].suite)
            LOG("[error=> {}] {}", ctx->bases[w].type->debugString(2),
                ctx->bases[w].suite->toString(2));
        // inferTypes(ast.suite, ctx);
        error("cannot typecheck the program");
      }
      if (!startswith(type->ast->name, "%_import_")) {
        ctx->bases.pop_back();
        ctx->popBlock();
        ctx->typecheckLevel--;
        getLogger().level--;
      }
      this->ctx = oldCtx;
      return nullptr; // inference must be delayed
    } else {
      ctx->getBase()->suite = ret;
    }

    // Use NoneType as the return type when the return type is not specified and
    // function has no return statement
    if (!ast->ret && type->getRetType()->getUnbound())
      unify(type->getRetType(), ctx->getType("NoneType"));
    // LOG("-> {} {}", key, ret->toString(2));
  }
  // Realize the return type
  auto ret = realize(type->getRetType());
  seqassert(ret, "cannot realize return type '{}'", type->getRetType());

  // LOG("[realize] F {} -> {}", type->ast->name, type->debugString(2));

  std::vector<Param> args;
  for (auto &i : ast->args) {
    std::string varName = i.name;
    trimStars(varName);
    args.emplace_back(varName, nullptr, nullptr, i.status);
  }
  r->ast = N<FunctionStmt>(ast->getSrcInfo(), r->type->realizedName(), nullptr, args,
                           ctx->getBase()->suite);
  r->ast->attributes = ast->attributes;

  if (!in(ctx->cache->pendingRealizations,
          make_pair(type->ast->name, type->realizedName()))) {
    if (!r->ir)
      r->ir = makeIRFunction(r);
    realizations[type->realizedName()] = r;
  } else {
    realizations[key] = realizations[type->realizedName()];
  }
  if (force)
    realizations[type->realizedName()]->ast = r->ast;
  val = std::make_shared<TypecheckItem>(type->realizedName(), "", ctx->getModule(),
                                        type->getFunc());
  ctx->addAlwaysVisible(val);
  if (!startswith(type->ast->name, "%_import_")) {
    ctx->bases.pop_back();
    ctx->popBlock();
    ctx->typecheckLevel--;
    getLogger().level--;
  }
  this->ctx = oldCtx;

  return type->getFunc();
}

/// Generate ASTs for all __internal__ functions that deal with vtable generation.
/// Intended to be called once the typechecking is done.
/// TODO: add JIT compatibility.
StmtPtr TypecheckVisitor::prepareVTables() {
  auto rep = "__internal__.class_populate_vtables:0"; // see internal.codon
  if (!in(ctx->cache->functions, rep))
    return nullptr;
  auto &initFn = ctx->cache->functions[rep];
  auto suite = N<SuiteStmt>();
  for (auto &[_, cls] : ctx->cache->classes) {
    for (auto &[r, real] : cls.realizations) {
      size_t vtSz = 0;
      for (auto &[base, vtable] : real->vtables) {
        if (!vtable.ir)
          vtSz += vtable.table.size();
      }
      if (!vtSz)
        continue;
      // __internal__.class_set_rtti_vtable(real.ID, size, real.type)
      suite->stmts.push_back(N<ExprStmt>(
          N<CallExpr>(N<DotExpr>(N<IdExpr>("__internal__"), "class_set_rtti_vtable"),
                      N<IntExpr>(real->id), N<IntExpr>(vtSz + 2), N<IdExpr>(r))));
      // LOG("[poly] {} -> {}", r, real->id);
      vtSz = 0;
      for (auto &[base, vtable] : real->vtables) {
        if (!vtable.ir) {
          for (auto &[k, v] : vtable.table) {
            auto &[fn, id] = v;
            std::vector<ExprPtr> ids;
            for (auto &t : fn->getArgTypes())
              ids.push_back(N<IdExpr>(t->realizedName()));
            // p[real.ID].__setitem__(f.ID, Function[<TYPE_F>](f).__raw__())
            LOG_REALIZE("[poly] vtable[{}][{}] = {}", real->id, vtSz + id, fn);
            suite->stmts.push_back(N<ExprStmt>(N<CallExpr>(
                N<DotExpr>(N<IdExpr>("__internal__"), "class_set_rtti_vtable_fn"),
                N<IntExpr>(real->id), N<IntExpr>(vtSz + id),
                N<CallExpr>(N<DotExpr>(
                    N<CallExpr>(N<InstantiateExpr>(
                                    N<IdExpr>("Function"),
                                    std::vector<ExprPtr>{
                                        N<InstantiateExpr>(
                                            N<IdExpr>(generateTuple(ids.size())), ids),
                                        N<IdExpr>(fn->getRetType()->realizedName())}),
                                N<IdExpr>(fn->realizedName())),
                    "__raw__")),
                N<IdExpr>(r))));
          }
          vtSz += vtable.table.size();
        }
      }
    }
  }
  initFn.ast->suite = suite;
  auto typ = initFn.realizations.begin()->second->type;
  LOG_REALIZE("[poly] {} : {}", typ, suite->toString(2));
  typ->ast = initFn.ast.get();
  realizeFunc(typ.get(), true);

  auto &initDist = ctx->cache->functions["__internal__.class_base_derived_dist:0"];
  // def class_base_derived_dist(B, D):
  //   return Tuple[<types before B is reached in D>].__elemsize__
  auto oldAst = initDist.ast;
  for (auto &[_, real] : initDist.realizations) {
    auto t = real->type;
    auto baseTyp = t->funcGenerics[0].type->getClass();
    auto derivedTyp = t->funcGenerics[1].type->getClass();

    const auto &fields = ctx->cache->classes[derivedTyp->name].fields;
    auto types = std::vector<ExprPtr>{};
    auto found = false;
    for (auto &f : fields) {
      if (f.baseClass == baseTyp->name) {
        found = true;
        break;
      } else {
        auto ft = realize(ctx->instantiate(f.type, derivedTyp));
        types.push_back(N<IdExpr>(ft->realizedName()));
      }
    }
    seqassert(found || ctx->cache->classes[baseTyp->name].fields.empty(),
              "cannot find distance between {} and {}", derivedTyp->name,
              baseTyp->name);
    StmtPtr suite = N<ReturnStmt>(
        N<DotExpr>(N<InstantiateExpr>(N<IdExpr>(generateTuple(types.size())), types),
                   "__elemsize__"));
    LOG_REALIZE("[poly] {} : {}", t, *suite);
    initDist.ast->suite = suite;
    t->ast = initDist.ast.get();
    realizeFunc(t.get(), true);
  }
  initDist.ast = oldAst;

  return nullptr;
}

/// Generate thunks in all derived classes for a given virtual function (must be fully
/// realizable) and the corresponding base class.
/// @return unique thunk ID.
size_t TypecheckVisitor::getRealizationID(types::ClassType *cp, types::FuncType *fp) {
  seqassert(cp->canRealize() && fp->canRealize() && fp->getRetType()->canRealize(),
            "{} not realized", fp->debugString(1));

  // TODO: ugly, ugly; surely needs refactoring

  // Function signature for storing thunks
  auto sig = [](types::FuncType *fp) {
    std::vector<std::string> gs;
    for (auto &a : fp->getArgTypes())
      gs.emplace_back(a->realizedName());
    gs.emplace_back("|");
    for (auto &a : fp->funcGenerics)
      if (!a.name.empty())
        gs.push_back(a.type->realizedName());
    return join(gs, ",");
  };

  // Set up the base class information
  auto baseCls = cp->name;
  auto fnName = ctx->cache->rev(fp->ast->name);
  auto key = make_pair(fnName, sig(fp));
  auto &vt = ctx->cache->classes[baseCls]
                 .realizations[cp->realizedName()]
                 ->vtables[cp->realizedName()];

  // Add or extract thunk ID
  size_t vid = 0;
  if (auto i = in(vt.table, key)) {
    vid = i->second;
  } else {
    vid = vt.table.size() + 1;
    vt.table[key] = {fp->getFunc(), vid};
  }

  // Iterate through all derived classes and instantiate the corresponding thunk
  for (auto &[clsName, cls] : ctx->cache->classes) {
    bool inMro = false;
    for (auto &m : cls.mro)
      if (m && m->is(baseCls)) {
        inMro = true;
        break;
      }
    if (clsName != baseCls && inMro) {
      for (auto &[_, real] : cls.realizations) {
        auto &vtable = real->vtables[baseCls];

        auto ct =
            ctx->instantiate(ctx->forceFind(clsName)->type, cp->getClass())->getClass();
        std::vector<types::TypePtr> args = fp->getArgTypes();
        args[0] = ct;
        auto m = findBestMethod(ct, fnName, args);
        if (!m) {
          // Print a nice error message
          std::vector<std::string> a;
          for (auto &t : args)
            a.emplace_back(fmt::format("{}", t->prettyString()));
          std::string argsNice = fmt::format("({})", fmt::join(a, ", "));
          E(Error::DOT_NO_ATTR_ARGS, getSrcInfo(), ct->prettyString(), fnName,
            argsNice);
        }

        std::vector<std::string> ns;
        for (auto &a : args)
          ns.push_back(a->realizedName());

        // Thunk name: _thunk.<BASE>.<FN>.<ARGS>
        auto thunkName =
            format("_thunk.{}.{}.{}", baseCls, m->ast->name, fmt::join(ns, "."));
        if (in(ctx->cache->functions, thunkName))
          continue;

        // Thunk contents:
        // def _thunk.<BASE>.<FN>.<ARGS>(self, <ARGS...>):
        //   return <FN>(
        //     __internal__.class_base_to_derived(self, <BASE>, <DERIVED>),
        //     <ARGS...>)
        std::vector<Param> fnArgs;
        fnArgs.emplace_back(fp->ast->args[0].name, N<IdExpr>(cp->realizedName()),
                            nullptr);
        for (size_t i = 1; i < args.size(); i++)
          fnArgs.emplace_back(fp->ast->args[i].name, N<IdExpr>(args[i]->realizedName()),
                              nullptr);
        std::vector<ExprPtr> callArgs;
        callArgs.emplace_back(
            N<CallExpr>(N<DotExpr>(N<IdExpr>("__internal__"), "class_base_to_derived"),
                        N<IdExpr>(fp->ast->args[0].name), N<IdExpr>(cp->realizedName()),
                        N<IdExpr>(real->type->realizedName())));
        for (size_t i = 1; i < args.size(); i++)
          callArgs.emplace_back(N<IdExpr>(fp->ast->args[i].name));
        auto thunkAst = N<FunctionStmt>(
            thunkName, nullptr, fnArgs,
            N<SuiteStmt>(N<ReturnStmt>(N<CallExpr>(N<IdExpr>(m->ast->name), callArgs))),
            Attr({"std.internal.attributes.inline", Attr::ForceRealize}));
        auto &thunkFn = ctx->cache->functions[thunkAst->name];
        thunkFn.ast = clone(thunkAst);

        transform(thunkAst);
        prependStmts->push_back(thunkAst);
        auto ti = ctx->instantiate(thunkFn.type)->getFunc();
        auto tm = realizeFunc(ti.get(), true);
        seqassert(tm, "bad thunk {}", thunkFn.type);
        vtable.table[key] = {tm->getFunc(), vid};
      }
    }
  }
  return vid;
}

/// Make IR node for a realized type.
ir::types::Type *TypecheckVisitor::makeIRType(types::ClassType *t) {
  // Realize if not, and return cached value if it exists
  auto realizedName = t->ClassType::realizedName();
  if (!in(ctx->cache->classes[t->name].realizations, realizedName))
    realize(t->getClass());
  if (auto l = ctx->cache->classes[t->name].realizations[realizedName]->ir)
    return l;

  auto forceFindIRType = [&](const TypePtr &tt) {
    auto t = tt->getClass();
    auto rn = t->ClassType::realizedName();
    seqassert(t && in(ctx->cache->classes[t->name].realizations, rn), "{} not realized",
              tt);
    auto l = ctx->cache->classes[t->name].realizations[rn]->ir;
    seqassert(l, "no LLVM type for {}", t);
    return l;
  };

  // Prepare generics and statics
  std::vector<ir::types::Type *> types;
  std::vector<types::StaticTypePtr> statics;
  for (auto &m : t->generics) {
    if (auto s = m.type->getStatic())
      statics.push_back(s);
    types.push_back(forceFindIRType(m.type));
  }

  // Get the IR type
  auto *module = ctx->cache->module;
  ir::types::Type *handle = nullptr;

  if (t->name == "bool") {
    handle = module->getBoolType();
  } else if (t->name == "byte") {
    handle = module->getByteType();
  } else if (t->name == "int") {
    handle = module->getIntType();
  } else if (t->name == "float") {
    handle = module->getFloatType();
  } else if (t->name == "float32") {
    handle = module->getFloat32Type();
  } else if (t->name == "str") {
    handle = module->getStringType();
  } else if (t->name == "Int" || t->name == "UInt") {
    handle = module->Nr<ir::types::IntNType>(statics[0]->getIntStatic()->value,
                                             t->name == "Int");
  } else if (t->name == "Ptr") {
    seqassert(types.size() == 1, "bad generics/statics");
    handle = module->unsafeGetPointerType(types[0]);
  } else if (t->name == "Generator") {
    seqassert(types.size() == 1, "bad generics/statics");
    handle = module->unsafeGetGeneratorType(types[0]);
  } else if (t->name == TYPE_OPTIONAL) {
    seqassert(types.size() == 1, "bad generics/statics");
    handle = module->unsafeGetOptionalType(types[0]);
  } else if (t->name == "NoneType") {
    seqassert(types.empty() && statics.empty(), "bad generics/statics");
    auto record =
        ir::cast<ir::types::RecordType>(module->unsafeGetMemberedType(realizedName));
    record->realize({}, {});
    handle = record;
  } else if (t->name == "Union") {
    seqassert(!types.empty(), "bad union");
    auto unionTypes = t->getUnion()->getRealizationTypes();
    std::vector<ir::types::Type *> unionVec;
    unionVec.reserve(unionTypes.size());
    for (auto &u : unionTypes)
      unionVec.emplace_back(forceFindIRType(u));
    handle = module->unsafeGetUnionType(unionVec);
  } else if (t->name == "Function") {
    types.clear();
    for (auto &m : t->generics[0].type->getClass()->generics)
      types.push_back(forceFindIRType(m.type));
    auto ret = forceFindIRType(t->generics[1].type);
    handle = module->unsafeGetFuncType(realizedName, ret, types);
  } else if (t->name == "std.experimental.simd.Vec") {
    seqassert(types.size() == 2 && !statics.empty(), "bad generics/statics");
    handle = module->unsafeGetVectorType(statics[0]->getIntStatic()->value, types[0]);
  } else {
    // Type arguments will be populated afterwards to avoid infinite loop with recursive
    // reference types (e.g., `class X: x: Optional[X]`)
    if (t->isRecord()) {
      std::vector<ir::types::Type *> typeArgs;   // needed for IR
      std::vector<std::string> names;            // needed for IR
      std::map<std::string, SrcInfo> memberInfo; // needed for IR

      auto ft = getClassFieldTypes(t->getClass());
      const auto &fields = ctx->cache->classes[t->getClass()->name].fields;
      for (size_t i = 0; i < ft.size(); i++) {
        if (!realize(ft[i])) {
          realize(ft[i]);
          E(Error::TYPE_CANNOT_REALIZE_ATTR, getSrcInfo(), fields[i].name,
            t->prettyString());
        }
        names.emplace_back(fields[i].name);
        typeArgs.emplace_back(makeIRType(ft[i]->getClass().get()));
        memberInfo[fields[i].name] = ft[i]->getSrcInfo();
      }
      auto record =
          ir::cast<ir::types::RecordType>(module->unsafeGetMemberedType(realizedName));
      record->realize(typeArgs, names);
      handle = record;
      handle->setAttribute(
          std::make_unique<ir::MemberAttribute>(std::move(memberInfo)));
    } else {
      handle = module->unsafeGetMemberedType(realizedName, !t->isRecord());
      if (ctx->cache->classes[t->name].rtti)
        ir::cast<ir::types::RefType>(handle)->setPolymorphic();
    }
  }
  handle->setSrcInfo(t->getSrcInfo());
  handle->setAstType(
      std::const_pointer_cast<codon::ast::types::Type>(t->shared_from_this()));
  return ctx->cache->classes[t->name].realizations[realizedName]->ir = handle;
}

/// Make IR node for a realized function.
ir::Func *TypecheckVisitor::makeIRFunction(
    const std::shared_ptr<Cache::Function::FunctionRealization> &r) {
  ir::Func *fn = nullptr;
  // Create and store a function IR node and a realized AST for IR passes
  if (r->ast->attributes.has(Attr::Internal)) {
    // e.g., __new__, Ptr.__new__, etc.
    fn = ctx->cache->module->Nr<ir::InternalFunc>(r->type->ast->name);
  } else if (r->ast->attributes.has(Attr::LLVM)) {
    fn = ctx->cache->module->Nr<ir::LLVMFunc>(r->type->realizedName());
  } else if (r->ast->attributes.has(Attr::C)) {
    fn = ctx->cache->module->Nr<ir::ExternalFunc>(r->type->realizedName());
  } else {
    fn = ctx->cache->module->Nr<ir::BodiedFunc>(r->type->realizedName());
  }
  fn->setUnmangledName(ctx->cache->reverseIdentifierLookup[r->type->ast->name]);
  auto parent = r->type->funcParent;
  if (!r->ast->attributes.parentClass.empty() &&
      !r->ast->attributes.has(Attr::Method)) {
    // Hack for non-generic methods
    parent = ctx->find(r->ast->attributes.parentClass)->type;
  }
  if (parent && parent->isInstantiated() && parent->canRealize()) {
    parent = ctx->getType(parent);
    realize(parent);
    fn->setParentType(makeIRType(parent->getClass().get()));
  }
  fn->setGlobal();
  // Mark this realization as pending (i.e., realized but not translated)
  ctx->cache->pendingRealizations.insert({r->type->ast->name, r->type->realizedName()});

  seqassert(!r->type || r->ast->args.size() == r->type->getArgTypes().size() +
                                                   r->type->funcGenerics.size(),
            "type/AST argument mismatch");

  // Populate the IR node
  std::vector<std::string> names;
  std::vector<codon::ir::types::Type *> types;
  for (size_t i = 0, j = 0; i < r->ast->args.size(); i++) {
    if (r->ast->args[i].status == Param::Normal) {
      if (!r->type->getArgTypes()[j]->getFunc()) {
        types.push_back(makeIRType(r->type->getArgTypes()[j]->getClass().get()));
        names.push_back(ctx->cache->reverseIdentifierLookup[r->ast->args[i].name]);
      }
      j++;
    }
  }
  if (r->ast->hasAttr(Attr::CVarArg)) {
    types.pop_back();
    names.pop_back();
  }
  auto irType = ctx->cache->module->unsafeGetFuncType(
      r->type->realizedName(), makeIRType(r->type->getRetType()->getClass().get()),
      types, r->ast->hasAttr(Attr::CVarArg));
  irType->setAstType(r->type->getFunc());
  fn->realize(irType, names);
  return fn;
}

/// Generate ASTs for dynamically generated functions.
std::shared_ptr<FunctionStmt>
TypecheckVisitor::generateSpecialAst(types::FuncType *type) {
  // Clone the generic AST that is to be realized
  auto ast = std::dynamic_pointer_cast<FunctionStmt>(
      clone(ctx->cache->functions[type->ast->name].ast));

  if (ast->hasAttr("autogenerated") && endswith(ast->name, ".__iter__") &&
      type->getArgTypes()[0]->getHeterogenousTuple()) {
    // Special case: do not realize auto-generated heterogenous __iter__
    E(Error::EXPECTED_TYPE, getSrcInfo(), "iterable");
  } else if (ast->hasAttr("autogenerated") && endswith(ast->name, ".__getitem__") &&
             type->getArgTypes()[0]->getHeterogenousTuple()) {
    // Special case: do not realize auto-generated heterogenous __getitem__
    E(Error::EXPECTED_TYPE, getSrcInfo(), "iterable");
  } else if (startswith(ast->name, "Function.__call_internal__")) {
    // Special case: Function.__call_internal__
    /// TODO: move to IR one day
    std::vector<StmtPtr> items;
    items.push_back(nullptr);
    std::vector<std::string> ll;
    std::vector<std::string> lla;
    seqassert(startswith(type->getArgTypes()[1]->getClass()->name, TYPE_TUPLE),
              "bad function base");
    auto as = type->getArgTypes()[1]->getClass()->generics.size();
    auto ag = ast->args[1].name;
    trimStars(ag);
    for (int i = 0; i < as; i++) {
      ll.push_back(format("%{} = extractvalue {{}} %args, {}", i, i));
      items.push_back(N<ExprStmt>(N<IdExpr>(ag)));
    }
    items.push_back(N<ExprStmt>(N<IdExpr>("TR")));
    for (int i = 0; i < as; i++) {
      items.push_back(N<ExprStmt>(N<IndexExpr>(N<IdExpr>(ag), N<IntExpr>(i))));
      lla.push_back(format("{{}} %{}", i));
    }
    items.push_back(N<ExprStmt>(N<IdExpr>("TR")));
    ll.push_back(format("%{} = call {{}} %self({})", as, combine2(lla)));
    ll.push_back(format("ret {{}} %{}", as));
    items[0] = N<ExprStmt>(N<StringExpr>(combine2(ll, "\n")));
    ast->suite = N<SuiteStmt>(items);
  } else if (startswith(ast->name, "Union.__new__")) {
    auto unionType = type->funcParent->getUnion();
    seqassert(unionType, "expected union, got {}", type->funcParent);

    StmtPtr suite = N<ReturnStmt>(N<CallExpr>(
        N<DotExpr>(N<IdExpr>("__internal__"), "new_union"),
        N<IdExpr>(type->ast->args[0].name), N<IdExpr>(unionType->realizedName())));
    ast->suite = suite;
  } else if (startswith(ast->name, "__internal__.new_union")) {
    // Special case: __internal__.new_union
    // def __internal__.new_union(value, U[T0, ..., TN]):
    //   if isinstance(value, T0):
    //     return __internal__.union_make(0, value, U[T0, ..., TN])
    //   if isinstance(value, Union[T0]):
    //     return __internal__.union_make(
    //       0, __internal__.get_union(value, T0), U[T0, ..., TN])
    //   ... <for all T0...TN> ...
    //   compile_error("invalid union constructor")
    auto unionType = type->funcGenerics[0].type->getUnion();
    auto unionTypes = unionType->getRealizationTypes();

    auto objVar = ast->args[0].name;
    auto suite = N<SuiteStmt>();
    int tag = 0;
    for (auto &t : unionTypes) {
      suite->stmts.push_back(N<IfStmt>(
          N<CallExpr>(N<IdExpr>("isinstance"), N<IdExpr>(objVar),
                      N<IdExpr>(t->realizedName())),
          N<ReturnStmt>(N<CallExpr>(N<DotExpr>(N<IdExpr>("__internal__"), "union_make"),
                                    N<IntExpr>(tag), N<IdExpr>(objVar),
                                    N<IdExpr>(unionType->realizedName())))));
      // Check for Union[T]
      suite->stmts.push_back(N<IfStmt>(
          N<CallExpr>(
              N<IdExpr>("isinstance"), N<IdExpr>(objVar),
              N<InstantiateExpr>(N<IdExpr>("Union"),
                                 std::vector<ExprPtr>{N<IdExpr>(t->realizedName())})),
          N<ReturnStmt>(N<CallExpr>(
              N<DotExpr>(N<IdExpr>("__internal__"), "union_make"), N<IntExpr>(tag),
              N<CallExpr>(N<DotExpr>(N<IdExpr>("__internal__"), "get_union"),
                          N<IdExpr>(objVar), N<IdExpr>(t->realizedName())),
              N<IdExpr>(unionType->realizedName())))));
      tag++;
    }
    suite->stmts.push_back(N<ExprStmt>(N<CallExpr>(
        N<IdExpr>("compile_error"), N<StringExpr>("invalid union constructor"))));
    ast->suite = suite;
  } else if (startswith(ast->name, "__internal__.get_union")) {
    // Special case: __internal__.get_union
    // def __internal__.new_union(union: Union[T0,...,TN], T):
    //   if __internal__.union_get_tag(union) == 0:
    //     return __internal__.union_get_data(union, T0)
    //   ... <for all T0...TN>
    //   raise TypeError("getter")
    auto unionType = type->getArgTypes()[0]->getUnion();
    auto unionTypes = unionType->getRealizationTypes();

    auto targetType = type->funcGenerics[0].type;
    auto selfVar = ast->args[0].name;
    auto suite = N<SuiteStmt>();
    int tag = 0;
    for (const auto &t : unionTypes) {
      if (t->realizedName() == targetType->realizedName()) {
        suite->stmts.push_back(N<IfStmt>(
            N<BinaryExpr>(
                N<CallExpr>(N<DotExpr>(N<IdExpr>("__internal__"), "union_get_tag"),
                            N<IdExpr>(selfVar)),
                "==", N<IntExpr>(tag)),
            N<ReturnStmt>(
                N<CallExpr>(N<DotExpr>(N<IdExpr>("__internal__"), "union_get_data"),
                            N<IdExpr>(selfVar), N<IdExpr>(t->realizedName())))));
      }
      tag++;
    }
    suite->stmts.push_back(
        N<ThrowStmt>(N<CallExpr>(N<IdExpr>("std.internal.types.error.TypeError"),
                                 N<StringExpr>("invalid union getter"))));
    ast->suite = suite;
  } else if (startswith(ast->name, "__internal__._get_union_method")) {
    // def __internal__._get_union_method(union: Union[T0,...,TN], method, *args, **kw):
    //   if __internal__.union_get_tag(union) == 0:
    //     return __internal__.union_get_data(union, T0).method(*args, **kw)
    //   ... <for all T0...TN>
    //   raise TypeError("call")
    auto fnName = type->funcGenerics[0].type->getStrStatic()->value;
    auto unionType = type->getArgTypes()[0]->getUnion();
    auto unionTypes = unionType->getRealizationTypes();

    auto selfVar = ast->args[0].name;
    auto suite = N<SuiteStmt>();
    int tag = 0;
    for (auto &t : unionTypes) {
      auto callee = N<DotExpr>(
          N<CallExpr>(N<DotExpr>(N<IdExpr>("__internal__"), "union_get_data"),
                      N<IdExpr>(selfVar), N<IdExpr>(t->realizedName())),
          fnName);
      auto args = N<StarExpr>(N<IdExpr>(ast->args[2].name.substr(1)));
      auto kwargs = N<KeywordStarExpr>(N<IdExpr>(ast->args[3].name.substr(2)));
      std::vector<CallExpr::Arg> callArgs;
      ExprPtr check = N<CallExpr>(N<IdExpr>("hasattr"), N<IdExpr>(t->realizedName()),
                                  N<StringExpr>(fnName), clone(args), clone(kwargs));
      suite->stmts.push_back(N<IfStmt>(
          N<BinaryExpr>(check, "&&",
                        N<BinaryExpr>(N<CallExpr>(N<DotExpr>(N<IdExpr>("__internal__"),
                                                             "union_get_tag"),
                                                  N<IdExpr>(selfVar)),
                                      "==", N<IntExpr>(tag))),
          N<SuiteStmt>(N<ReturnStmt>(N<CallExpr>(callee, args, kwargs)))));
      tag++;
    }
    suite->stmts.push_back(
        N<ThrowStmt>(N<CallExpr>(N<IdExpr>("std.internal.types.error.TypeError"),
                                 N<StringExpr>("invalid union call"))));
    // suite->stmts.push_back(N<ReturnStmt>(N<NoneExpr>()));
    unify(type->getRetType(), ctx->instantiate(ctx->getType("Union")));
    ast->suite = suite;
  } else if (startswith(ast->name, "__internal__.get_union_first")) {
    // def __internal__.get_union_first(union: Union[T0]):
    //   return __internal__.union_get_data(union, T0)
    auto unionType = type->getArgTypes()[0]->getUnion();
    auto unionTypes = unionType->getRealizationTypes();

    auto selfVar = ast->args[0].name;
    auto suite = N<SuiteStmt>(N<ReturnStmt>(
        N<CallExpr>(N<DotExpr>(N<IdExpr>("__internal__"), "union_get_data"),
                    N<IdExpr>(selfVar), N<IdExpr>(unionTypes[0]->realizedName()))));
    ast->suite = suite;
  } else if (startswith(ast->name, "__internal__.namedkeys")) {
    auto n = type->funcGenerics[0].type->getIntStatic()->value;
    if (n < 0 || n >= ctx->cache->generatedTupleNames.size())
      error("bad namedkeys index");
    std::vector<ExprPtr> s;
    for (auto &k : ctx->cache->generatedTupleNames[n])
      s.push_back(N<StringExpr>(k));
    auto suite = N<SuiteStmt>(N<ReturnStmt>(N<TupleExpr>(s)));
    ast->suite = suite;
  }
  return ast;
}

} // namespace codon::ast
