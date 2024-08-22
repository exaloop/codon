// Copyright (C) 2022-2024 Exaloop Inc. <https://exaloop.io>

#include <limits>
#include <map>
#include <memory>
#include <string>
#include <tuple>
#include <vector>

#include "codon/cir/types/types.h"
#include "codon/parser/ast.h"
#include "codon/parser/common.h"
#include "codon/parser/visitors/simplify/simplify.h"
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

  for (ctx->getRealizationBase()->iteration = 1;;
       ctx->getRealizationBase()->iteration++) {
    LOG_TYPECHECK("[iter] {} :: {}", ctx->getRealizationBase()->name,
                  ctx->getRealizationBase()->iteration);
    if (ctx->getRealizationBase()->iteration >= MAX_TYPECHECK_ITER) {
      error(result, "cannot typecheck '{}' in reasonable time",
            ctx->getRealizationBase()->name.empty()
                ? "toplevel"
                : ctx->cache->rev(ctx->getRealizationBase()->name));
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
    TypecheckVisitor(ctx).transform(result);
    std::swap(ctx->changedNodes, changedNodes);
    std::swap(ctx->returnEarly, returnEarly);
    ctx->typecheckLevel--;

    if (ctx->getRealizationBase()->iteration == 1 && isToplevel) {
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
      // Special union case: if union cannot be inferred return type is Union[NoneType]
      if (auto tr = ctx->getRealizationBase()->returnType) {
        if (auto tu = tr->getUnion()) {
          if (!tu->isSealed()) {
            if (tu->pendingTypes[0]->getLink() &&
                tu->pendingTypes[0]->getLink()->kind == LinkType::Unbound) {
              tu->addType(ctx->forceFind("NoneType")->type);
              tu->seal();
            }
          }
        }
      }
      break;
    } else if (changedNodes) {
      continue;
    } else {
      // Special case: nothing was changed, however there are unbound types that have
      // default values (e.g., generics with default values). Unify those types with
      // their default values and then run another round to see if anything changed.
      bool anotherRound = false;
      // Special case: return type might have default as well (e.g., Union)
      if (ctx->getRealizationBase()->returnType)
        ctx->getRealizationBase()->pendingDefaults.insert(
            ctx->getRealizationBase()->returnType);
      for (auto &unbound : ctx->getRealizationBase()->pendingDefaults) {
        if (auto tu = unbound->getUnion()) {
          // Seal all dynamic unions after the iteration is over
          if (!tu->isSealed()) {
            tu->seal();
            anotherRound = true;
          }
        } else if (auto u = unbound->getLink()) {
          types::Type::Unification undo;
          if (u->defaultType && u->unify(u->defaultType.get(), &undo) >= 0)
            anotherRound = true;
        }
      }
      ctx->getRealizationBase()->pendingDefaults.clear();
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

  if (typ->getStatic()) {
    // Nothing to realize here
    return typ;
  }

  try {
    if (auto f = typ->getFunc()) {
      if (auto ret = realizeFunc(f.get())) {
        // Realize Function[..] type as well
        realizeType(ret->getClass().get());
        return unify(ret, typ); // Needed for return type unification
      }
    } else if (auto c = typ->getClass()) {
      auto t = realizeType(c.get());
      if (auto p = typ->getPartial()) {
        // Ensure that the partial type is preserved
        t = std::make_shared<PartialType>(t->getRecord(), p->func, p->known);
      }
      if (t) {
        return unify(t, typ);
      }
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
        if (startswith(name, "._import_")) {
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

  if (auto tr = type->getRecord())
    tr->flatten();

  // Check if the type fields are all initialized
  // (sometimes that's not the case: e.g., `class X: x: List[X]`)
  for (auto field : getClassFields(type)) {
    if (!field.type)
      return nullptr;
  }

  // Check if the type was already realized
  if (auto r =
          in(ctx->cache->classes[type->name].realizations, type->realizedTypeName())) {
    return (*r)->type->getClass();
  }

  auto realized = type->getClass();
  if (type->getFunc()) {
    // Just realize the function stub
    realized = std::make_shared<RecordType>(realized, type->getFunc()->args);
  }

  // Realize generics
  for (auto &e : realized->generics) {
    if (!realize(e.type))
      return nullptr;
  }

  LOG_REALIZE("[realize] ty {} -> {}", realized->name, realized->realizedTypeName());

  // Realizations should always be visible, so add them to the toplevel
  ctx->addToplevel(realized->realizedTypeName(),
                   std::make_shared<TypecheckItem>(TypecheckItem::Type, realized));
  auto realization =
      ctx->cache->classes[realized->name].realizations[realized->realizedTypeName()] =
          std::make_shared<Cache::Class::ClassRealization>();
  realization->type = realized;
  realization->id = ctx->cache->classRealizationCnt++;

  // Realize tuple arguments
  if (auto tr = realized->getRecord()) {
    for (auto &a : tr->args)
      realize(a);
  }

  // Create LLVM stub
  auto lt = makeIRType(realized.get());

  // Realize fields
  std::vector<ir::types::Type *> typeArgs;   // needed for IR
  std::vector<std::string> names;            // needed for IR
  std::map<std::string, SrcInfo> memberInfo; // needed for IR
  if (realized->is(TYPE_TUPLE))
    realized->getRecord()->flatten();
  int i = 0;
  for (auto &field : getClassFields(realized.get())) {
    auto ftyp = ctx->instantiate(field.type, realized);
    // HACK: repeated tuples have no generics so this is needed to fix the instantiation
    // above
    if (realized->is(TYPE_TUPLE))
      unify(ftyp, realized->getRecord()->args[i]);

    if (!realize(ftyp))
      E(Error::TYPE_CANNOT_REALIZE_ATTR, getSrcInfo(), field.name,
        ftyp->prettyString());
    realization->fields.emplace_back(field.name, ftyp);
    names.emplace_back(field.name);
    typeArgs.emplace_back(makeIRType(ftyp->getClass().get()));
    memberInfo[field.name] = field.type->getSrcInfo();
    i++;
  }

  // Set IR attributes
  if (auto *cls = ir::cast<ir::types::RefType>(lt))
    if (!names.empty()) {
      cls->getContents()->realize(typeArgs, names);
      cls->setAttribute(std::make_unique<ir::MemberAttribute>(memberInfo));
      cls->getContents()->setAttribute(
          std::make_unique<ir::MemberAttribute>(memberInfo));
    }

  // Fix for partial types
  if (auto p = type->getPartial()) {
    auto pt = std::make_shared<PartialType>(realized->getRecord(), p->func, p->known);
    ctx->addToplevel(pt->realizedName(),
                     std::make_shared<TypecheckItem>(TypecheckItem::Type, pt));
    ctx->cache->classes[pt->name].realizations[pt->realizedName()] =
        ctx->cache->classes[realized->name].realizations[realized->realizedTypeName()];
  }

  return realized;
}

types::TypePtr TypecheckVisitor::realizeFunc(types::FuncType *type, bool force) {
  auto &realizations = ctx->cache->functions[type->ast->name].realizations;
  if (auto r = in(realizations, type->realizedName())) {
    if (!force) {
      return (*r)->type;
    }
  }

  if (ctx->getRealizationDepth() > MAX_REALIZATION_DEPTH) {
    E(Error::MAX_REALIZATION, getSrcInfo(), ctx->cache->rev(type->ast->name));
  }

  LOG_REALIZE("[realize] fn {} -> {} : base {} ; depth = {}", type->ast->name,
              type->realizedName(), ctx->getRealizationStackName(),
              ctx->getRealizationDepth());
  getLogger().level++;
  ctx->addBlock();
  ctx->typecheckLevel++;

  // Find function parents
  ctx->realizationBases.push_back(
      {type->ast->name, type->getFunc(), type->getRetType()});

  // Clone the generic AST that is to be realized
  auto ast = generateSpecialAst(type);
  addFunctionGenerics(type);

  // Internal functions have no AST that can be realized
  bool hasAst = ast->suite && !ast->attributes.has(Attr::Internal);
  // Add function arguments
  for (size_t i = 0, j = 0; hasAst && i < ast->args.size(); i++)
    if (ast->args[i].status == Param::Normal) {
      std::string varName = ast->args[i].name;
      trimStars(varName);
      ctx->add(TypecheckItem::Var, varName,
               std::make_shared<LinkType>(type->getArgTypes()[j++]));
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
  ctx->addToplevel(
      key, std::make_shared<TypecheckItem>(TypecheckItem::Func, type->getFunc()));

  if (hasAst) {
    auto oldBlockLevel = ctx->blockLevel;
    ctx->blockLevel = 0;
    auto ret = inferTypes(ast->suite);
    ctx->blockLevel = oldBlockLevel;

    if (!ret) {
      realizations.erase(key);
      ctx->realizationBases.pop_back();
      ctx->popBlock();
      ctx->typecheckLevel--;
      getLogger().level--;
      if (!startswith(ast->name, "._lambda")) {
        // Lambda typecheck failures are "ignored" as they are treated as statements,
        // not functions.
        // TODO: generalize this further.
        LOG_REALIZE("[error] {}", ast->suite->toString(2));
        error("cannot typecheck the program");
      }
      return nullptr; // inference must be delayed
    }

    // Use NoneType as the return type when the return type is not specified and
    // function has no return statement
    if (!ast->ret && type->getRetType()->getUnbound())
      unify(type->getRetType(), ctx->getType("NoneType"));
  }
  // Realize the return type
  auto ret = realize(type->getRetType());
  seqassert(ret, "cannot realize return type '{}'", type->getRetType());

  std::vector<Param> args;
  for (auto &i : ast->args) {
    std::string varName = i.name;
    trimStars(varName);
    args.emplace_back(Param{varName, nullptr, nullptr, i.status});
  }
  r->ast = N<FunctionStmt>(ast->getSrcInfo(), r->type->realizedName(), nullptr, args,
                           ast->suite);
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
  ctx->addToplevel(type->realizedName(), std::make_shared<TypecheckItem>(
                                             TypecheckItem::Func, type->getFunc()));
  ctx->realizationBases.pop_back();
  ctx->popBlock();
  ctx->typecheckLevel--;
  getLogger().level--;

  return type->getFunc();
}

/// Generate ASTs for all __internal__ functions that deal with vtable generation.
/// Intended to be called once the typechecking is done.
/// TODO: add JIT compatibility.
StmtPtr TypecheckVisitor::prepareVTables() {
  auto rep = "__internal__.class_populate_vtables:0"; // see internal.codon
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
          N<CallExpr>(N<IdExpr>("__internal__.class_set_rtti_vtable:0"),
                      N<IntExpr>(real->id), N<IntExpr>(vtSz + 2), NT<IdExpr>(r))));
      // LOG("[poly] {} -> {}", r, real->id);
      vtSz = 0;
      for (auto &[base, vtable] : real->vtables) {
        if (!vtable.ir) {
          for (auto &[k, v] : vtable.table) {
            auto &[fn, id] = v;
            std::vector<ExprPtr> ids;
            for (auto &t : fn->getArgTypes())
              ids.push_back(NT<IdExpr>(t->realizedName()));
            // p[real.ID].__setitem__(f.ID, Function[<TYPE_F>](f).__raw__())
            LOG_REALIZE("[poly] vtable[{}][{}] = {}", real->id, vtSz + id, fn);
            suite->stmts.push_back(N<ExprStmt>(N<CallExpr>(
                N<IdExpr>("__internal__.class_set_rtti_vtable_fn:0"),
                N<IntExpr>(real->id), N<IntExpr>(vtSz + id),
                N<CallExpr>(N<DotExpr>(
                    N<CallExpr>(
                        NT<InstantiateExpr>(
                            NT<IdExpr>("Function"),
                            std::vector<ExprPtr>{
                                NT<InstantiateExpr>(NT<IdExpr>(TYPE_TUPLE), ids),
                                NT<IdExpr>(fn->getRetType()->realizedName())}),
                        N<IdExpr>(fn->realizedName())),
                    "__raw__")),
                NT<IdExpr>(r))));
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

    const auto &fields = getClassFields(derivedTyp.get());
    auto types = std::vector<ExprPtr>{};
    auto found = false;
    for (auto &f : fields) {
      if (f.baseClass == baseTyp->name) {
        found = true;
        break;
      } else {
        auto ft = realize(ctx->instantiate(f.type, derivedTyp));
        types.push_back(NT<IdExpr>(ft->realizedName()));
      }
    }
    seqassert(found || getClassFields(baseTyp.get()).empty(),
              "cannot find distance between {} and {}", derivedTyp->name,
              baseTyp->name);
    StmtPtr suite = N<ReturnStmt>(
        N<DotExpr>(NT<InstantiateExpr>(NT<IdExpr>(TYPE_TUPLE), types), "__elemsize__"));
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
      gs.push_back(a->realizedName());
    gs.push_back("|");
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
      if (m->type && m->type->getClass() && m->type->getClass()->name == baseCls) {
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
            N<CallExpr>(N<IdExpr>("__internal__.class_base_to_derived:0"),
                        N<IdExpr>(fp->ast->args[0].name), N<IdExpr>(cp->realizedName()),
                        N<IdExpr>(real->type->realizedName())));
        for (size_t i = 1; i < args.size(); i++)
          callArgs.emplace_back(N<IdExpr>(fp->ast->args[i].name));
        auto thunkAst = N<FunctionStmt>(
            thunkName, nullptr, fnArgs,
            N<SuiteStmt>(N<ReturnStmt>(N<CallExpr>(N<IdExpr>(m->ast->name), callArgs))),
            Attr({"std.internal.attributes.inline", Attr::ForceRealize}));
        auto &thunkFn = ctx->cache->functions[thunkAst->name];
        thunkFn.ast = std::static_pointer_cast<FunctionStmt>(thunkAst->clone());

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
  auto realizedName = t->realizedTypeName();
  if (!in(ctx->cache->classes[t->name].realizations, realizedName))
    realize(t->getClass());
  if (auto l = ctx->cache->classes[t->name].realizations[realizedName]->ir)
    return l;

  auto forceFindIRType = [&](const TypePtr &tt) {
    auto t = tt->getClass();
    seqassert(t && in(ctx->cache->classes[t->name].realizations, t->realizedTypeName()),
              "{} not realized", tt);
    auto l = ctx->cache->classes[t->name].realizations[t->realizedTypeName()]->ir;
    seqassert(l, "no LLVM type for {}", t);
    return l;
  };

  // Prepare generics and statics
  std::vector<ir::types::Type *> types;
  std::vector<StaticValue *> statics;
  for (auto &m : t->generics) {
    if (auto s = m.type->getStatic()) {
      seqassert(s->expr->staticValue.evaluated, "static not realized");
      statics.push_back(&(s->expr->staticValue));
    } else {
      types.push_back(forceFindIRType(m.type));
    }
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
  } else if (t->name == "float16") {
    handle = module->getFloat16Type();
  } else if (t->name == "bfloat16") {
    handle = module->getBFloat16Type();
  } else if (t->name == "float128") {
    handle = module->getFloat128Type();
  } else if (t->name == "str") {
    handle = module->getStringType();
  } else if (t->name == "Int" || t->name == "UInt") {
    handle = module->Nr<ir::types::IntNType>(statics[0]->getInt(), t->name == "Int");
  } else if (t->name == "Ptr") {
    seqassert(types.size() == 1 && statics.empty(), "bad generics/statics");
    handle = module->unsafeGetPointerType(types[0]);
  } else if (t->name == "Generator") {
    seqassert(types.size() == 1 && statics.empty(), "bad generics/statics");
    handle = module->unsafeGetGeneratorType(types[0]);
  } else if (t->name == TYPE_OPTIONAL) {
    seqassert(types.size() == 1 && statics.empty(), "bad generics/statics");
    handle = module->unsafeGetOptionalType(types[0]);
  } else if (t->name == "NoneType") {
    seqassert(types.empty() && statics.empty(), "bad generics/statics");
    auto record =
        ir::cast<ir::types::RecordType>(module->unsafeGetMemberedType(realizedName));
    record->realize({}, {});
    handle = record;
  } else if (t->name == "Union") {
    seqassert(!types.empty() && statics.empty(), "bad union");
    auto unionTypes = t->getUnion()->getRealizationTypes();
    std::vector<ir::types::Type *> unionVec;
    unionVec.reserve(unionTypes.size());
    for (auto &u : unionTypes)
      unionVec.emplace_back(forceFindIRType(u));
    handle = module->unsafeGetUnionType(unionVec);
  } else if (t->name == "Function") {
    types.clear();
    for (auto &m : t->generics[0].type->getRecord()->args)
      types.push_back(forceFindIRType(m));
    auto ret = forceFindIRType(t->generics[1].type);
    handle = module->unsafeGetFuncType(realizedName, ret, types);
  } else if (t->name == "std.experimental.simd.Vec") {
    seqassert(types.size() == 1 && statics.size() == 1, "bad generics/statics");
    handle = module->unsafeGetVectorType(statics[0]->getInt(), types[0]);
  } else if (auto tr = t->getRecord()) {
    seqassert(tr->getRepeats() >= 0, "repeats not resolved: '{}'", tr->debugString(2));
    tr->flatten();
    std::vector<ir::types::Type *> typeArgs;
    std::vector<std::string> names;
    std::map<std::string, SrcInfo> memberInfo;
    for (int ai = 0; ai < tr->args.size(); ai++) {
      auto n = t->name == TYPE_TUPLE ? format("item{}", ai + 1)
                                     : ctx->cache->classes[t->name].fields[ai].name;
      names.emplace_back(n);
      typeArgs.emplace_back(forceFindIRType(tr->args[ai]));
      memberInfo[n] = t->name == TYPE_TUPLE
                          ? tr->getSrcInfo()
                          : ctx->cache->classes[t->name].fields[ai].type->getSrcInfo();
    }
    auto record =
        ir::cast<ir::types::RecordType>(module->unsafeGetMemberedType(realizedName));
    record->realize(typeArgs, names);
    handle = record;
    handle->setAttribute(std::make_unique<ir::MemberAttribute>(std::move(memberInfo)));
  } else {
    // Type arguments will be populated afterwards to avoid infinite loop with recursive
    // reference types (e.g., `class X: x: Optional[X]`)
    handle = module->unsafeGetMemberedType(realizedName, true);
    if (ctx->cache->classes[t->name].rtti) {
      // LOG("RTTI: {}", t->name);
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
  if (parent && parent->canRealize()) {
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

  if (ast->hasAttr("autogenerated") && endswith(ast->name, ".__iter__:0") &&
      type->getArgTypes()[0]->getHeterogenousTuple()) {
    // Special case: do not realize auto-generated heterogenous __iter__
    E(Error::EXPECTED_TYPE, getSrcInfo(), "iterable");
  } else if (ast->hasAttr("autogenerated") && endswith(ast->name, ".__getitem__:0") &&
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
    ll.push_back(format("%{} = call {{}} %self({})", as.size(), combine2(lla)));
    ll.push_back(format("ret {{}} %{}", as.size()));
    items[0] = N<ExprStmt>(N<StringExpr>(combine2(ll, "\n")));
    ast->suite = N<SuiteStmt>(items);
  } else if (startswith(ast->name, "Union.__new__:0")) {
    auto unionType = type->funcParent->getUnion();
    seqassert(unionType, "expected union, got {}", type->funcParent);

    StmtPtr suite = N<ReturnStmt>(N<CallExpr>(
        N<IdExpr>("__internal__.new_union:0"), N<IdExpr>(type->ast->args[0].name),
        N<IdExpr>(unionType->realizedTypeName())));
    ast->suite = suite;
  } else if (startswith(ast->name, "__internal__.get_union_tag:0")) {
    // def __internal__.get_union_tag(union: Union, tag: Static[int]):
    //   return __internal__.union_get_data(union, T0)
    auto szt = type->funcGenerics[0].type->getStatic();
    auto tag = szt->evaluate().getInt();
    auto unionType = type->getArgTypes()[0]->getUnion();
    auto unionTypes = unionType->getRealizationTypes();
    if (tag < 0 || tag >= unionTypes.size())
      E(Error::CUSTOM, getSrcInfo(), "bad union tag");
    auto selfVar = ast->args[0].name;
    auto suite = N<SuiteStmt>(N<ReturnStmt>(
        N<CallExpr>(N<IdExpr>("__internal__.union_get_data:0"), N<IdExpr>(selfVar),
                    NT<IdExpr>(unionTypes[tag]->realizedName()))));
    ast->suite = suite;
  }
  return ast;
}

} // namespace codon::ast
