#include <limits>
#include <map>
#include <memory>
#include <string>
#include <tuple>
#include <vector>

#include "codon/parser/ast.h"
#include "codon/parser/common.h"
#include "codon/parser/visitors/simplify/simplify.h"
#include "codon/parser/visitors/typecheck/typecheck.h"
#include "codon/sir/types/types.h"

using fmt::format;
using namespace codon::error;

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

  for (size_t iteration = 1;; iteration++) {
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

    if (iteration == 1 && isToplevel) {
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
      if (ctx->getRealizationBase()->returnType) {
        if (auto tu = ctx->getRealizationBase()->returnType->getUnion()) {
          if (!tu->isSealed()) {
            tu->seal();
            anotherRound = true;
          }
        }
      }
      for (auto &unbound : ctx->pendingDefaults) {
        if (auto tu = unbound->getUnion()) {
          if (!tu->isSealed()) {
            tu->seal();
            anotherRound = true;
          }
        } else if (auto u = unbound->getLink()) {
          types::Type::Unification undo;
          if (u->unify(u->defaultType.get(), &undo) >= 0)
            anotherRound = true;
        }
      }
      ctx->pendingDefaults.clear();
      if (anotherRound)
        continue;

      // Nothing helps. Raise an error.
      /// TODO: print which expressions could not be type-checked
      if (codon::getLogger().flags & codon::Logger::FLAG_USER) {
        // Dump the problematic block
        auto fo = fopen("_dump_typecheck_error.sexp", "w");
        fmt::print(fo, "{}\n", result->toString(0));
        fclose(fo);
      }
      error("cannot typecheck the program");
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
    if (auto f = typ->getFunc()) {
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
      e.trackRealize(
          fmt::format("{}({})", ctx->cache->rev(f->ast->name), fmt::join(args, ", ")),
          getSrcInfo());
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

  // Check if the type fields are all initialized
  // (sometimes that's not the case: e.g., `class X: x: List[X]`)
  for (auto &field : ctx->cache->classes[type->name].fields) {
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

  LOG_TYPECHECK("[realize] ty {} -> {}", realized->name, realized->realizedTypeName());

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
  for (auto &field : ctx->cache->classes[realized->name].fields) {
    auto ftyp = ctx->instantiate(field.type, realized);
    if (!realize(ftyp))
      E(Error::TYPE_CANNOT_REALIZE_ATTR, getSrcInfo(), field.name,
        ftyp->prettyString());
    LOG_REALIZE("- member: {} -> {}: {}", field.name, field.type->toString(),
                ftyp->toString());
    realization->fields.emplace_back(field.name, ftyp);
    names.emplace_back(field.name);
    typeArgs.emplace_back(makeIRType(ftyp->getClass().get()));
    memberInfo[field.name] = field.type->getSrcInfo();
  }

  // Set IR attributes
  if (auto *cls = ir::cast<ir::types::RefType>(lt))
    if (!names.empty()) {
      cls->getContents()->realize(typeArgs, names);
      cls->setAttribute(std::make_unique<ir::MemberAttribute>(memberInfo));
      cls->getContents()->setAttribute(
          std::make_unique<ir::MemberAttribute>(memberInfo));
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

  if (ctx->getRealizationDepth() > MAX_REALIZATION_DEPTH)
    codon::compilationError(
        "maximum realization depth exceeded (recursive static function?)",
        getSrcInfo().file, getSrcInfo().line, getSrcInfo().col, getSrcInfo().len);

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
    inferTypes(ast->suite);
    ctx->blockLevel = oldBlockLevel;

    // Use NoneType as the return type when the return type is not specified and
    // function has no return statement
    if (!ast->ret && type->getRetType()->getUnbound())
      unify(type->getRetType(), ctx->getType("NoneType"));
  }
  // Realize the return type
  auto ret = realize(type->getRetType());
  seqassert(ret, "cannot realize return type '{}'", type->getRetType()->toString());

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

size_t TypecheckVisitor::getRealizationID(types::ClassType *cp, types::FuncType *fp) {
  seqassert(cp->canRealize() && fp->canRealize() && fp->getRetType()->canRealize(),
            "{} not realized", fp->debugString(1));

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

  auto baseCls = cp->name;
  auto fnName = ctx->cache->rev(fp->ast->name);
  auto key = make_pair(fnName, sig(fp));
  auto &vt = ctx->cache->classes[baseCls]
                 .realizations[cp->realizedName()]
                 ->vtables[cp->realizedName()];

  size_t vid;
  if (auto i = in(vt.table, key)) {
    vid = i->second;
  } else {
    vid = vt.table.size();
    vt.table[key] = {fp->getFunc(), vid};
  }

  // LOG("[virtual] realized base {} := {} :: {} :: {}", vid, baseCls, key.first,
  //     key.second);

  for (auto &[clsName, cls] : ctx->cache->classes) {
    if (clsName != baseCls && in(cls.mro, baseCls)) {
      for (auto &[_, real] : cls.realizations) {
        auto &vtable = real->vtables[baseCls];

        // TODO: fix the instantiation for geenrics!
        auto ct =
            ctx->instantiate(ctx->forceFind(clsName)->type, cp->getClass())->getClass();
        std::vector<types::TypePtr> args = fp->getArgTypes();
        args[0] = ct;
        types::TypePtr m = findBestMethod(ct, fnName, args);
        m = ctx->instantiate(m);
        for (size_t ai = 0; ai < args.size(); ai++)
          unify(m->getFunc()->getArgTypes()[ai], args[ai]);
        unify(m->getFunc()->getRetType(), fp->getRetType());
        seqassert(m->canRealize(), "cannot realize overriden method {}",
                  m->debugString(1));
        m = realize(m);
        key = make_pair(fnName, sig(m->getFunc().get()));

        std::vector<Param> fnArgs;
        fnArgs.push_back(
            Param{fp->ast->args[0].name, N<IdExpr>(cp->realizedName()), nullptr});
        for (size_t i = 1; i < args.size(); i++)
          fnArgs.push_back(Param{fp->ast->args[i].name,
                                 N<IdExpr>(args[i]->realizedName()), nullptr});
        std::vector<ExprPtr> callArgs;
        callArgs.emplace_back(N<CallExpr>(
            N<IdExpr>("__internal__.to_class_ptr:0"),
            N<BinaryExpr>(
                N<CallExpr>(N<DotExpr>(N<IdExpr>(fp->ast->args[0].name), "__raw__")),
                "-",
                N<CallExpr>(N<IdExpr>("__internal__.base_derived_dist:0"),
                            N<IdExpr>(cp->realizedName()),
                            N<IdExpr>(real->type->realizedName()))),
            NT<IdExpr>(real->type->realizedName())));
        for (size_t i = 1; i < args.size(); i++)
          callArgs.emplace_back(N<IdExpr>(fp->ast->args[i].name));
        auto thunkAst = N<FunctionStmt>(
            format("_thunk.{}.{}", baseCls, m->realizedName()), nullptr, fnArgs,
            N<ReturnStmt>(N<CallExpr>(N<IdExpr>(m->realizedName()), callArgs)),
            Attr({"std.internal.attributes.inline", Attr::ForceRealize}));
        auto &thunkFn = ctx->cache->functions[thunkAst->name];
        thunkFn.ast = std::static_pointer_cast<FunctionStmt>(thunkAst->clone());
        transform(thunkAst);
        auto tm = realize(ctx->instantiate(thunkFn.type)->getFunc());
        seqassert(tm, "bad thunk {}", thunkFn.type->debugString(2));

        // LOG("[virtual] realized child {} := {} :: {} :: {} [{} / {}]", vid, baseCls,
        // key.first, key.second, tm->debugString(1), m->debugString(1));
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
              "{} not realized", tt->toString());
    auto l = ctx->cache->classes[t->name].realizations[t->realizedTypeName()]->ir;
    seqassert(l, "no LLVM type for {}", t->toString());
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
  } else if (t->name == "std.simd.Vec") {
    seqassert(types.size() == 1 && statics.size() == 1, "bad generics/statics");
    handle = module->unsafeGetVectorType(statics[0]->getInt(), types[0]);
  } else if (auto tr = t->getRecord()) {
    std::vector<ir::types::Type *> typeArgs;
    std::vector<std::string> names;
    std::map<std::string, SrcInfo> memberInfo;
    for (int ai = 0; ai < tr->args.size(); ai++) {
      names.emplace_back(ctx->cache->classes[t->name].fields[ai].name);
      typeArgs.emplace_back(forceFindIRType(tr->args[ai]));
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
    // reference types (e.g., `class X: x: Optional[X]`)
    handle = module->unsafeGetMemberedType(realizedName, true);
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

std::shared_ptr<FunctionStmt>
TypecheckVisitor::generateSpecialAst(types::FuncType *type) {
  // Clone the generic AST that is to be realized
  auto ast = std::dynamic_pointer_cast<FunctionStmt>(
      clone(ctx->cache->functions[type->ast->name].ast));

  if (ast->hasAttr("autogenerated") && endswith(ast->name, ".__iter__:0") &&
      type->getArgTypes()[0]->getHeterogenousTuple()) {
    // Special cases: do not realize auto-generated heterogenous (__iter__, __getitem__)
    E(Error::EXPECTED_TYPE, getSrcInfo(), "iterable");
  } else if (ast->hasAttr("autogenerated") && endswith(ast->name, ".__getitem__:0") &&
             type->getArgTypes()[0]->getHeterogenousTuple()) {
    E(Error::EXPECTED_TYPE, getSrcInfo(), "iterable");
  } else if (startswith(ast->name, "Function.__call__")) {
    // Special case: Function.__call__
    /// TODO: move to IR
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
  } else if (startswith(ast->name, "__internal__.new_union:0")) {
    auto unionType = type->funcGenerics[0].type->getUnion();
    auto unionTypes = unionType->getRealizationTypes();

    auto objVar = ast->args[0].name;
    auto suite = N<SuiteStmt>();
    int tag = 0;
    for (auto &t : unionTypes) {
      suite->stmts.push_back(N<IfStmt>(
          N<CallExpr>(N<IdExpr>("isinstance"), N<IdExpr>(objVar),
                      NT<IdExpr>(t->realizedName())),
          N<ReturnStmt>(N<CallExpr>(N<IdExpr>("__internal__.union_make:0"),
                                    N<IntExpr>(tag), N<IdExpr>(objVar),
                                    N<IdExpr>(unionType->realizedTypeName())))));
      tag++;
    }
    suite->stmts.push_back(N<ExprStmt>(N<CallExpr>(
        N<IdExpr>("compile_error"), N<StringExpr>("invalid union constructor"))));
    ast->suite = suite;
    //    LOG("-> {} @ {}", unionType->debugString(1), ast->toString(1));
  } else if (startswith(ast->name, "__internal__.get_union:0")) {
    auto unionType = type->getArgTypes()[0]->getUnion();
    auto unionTypes = unionType->getRealizationTypes();

    auto targetType = type->funcGenerics[0].type;
    auto selfVar = ast->args[0].name;
    auto suite = N<SuiteStmt>();
    int tag = 0;
    for (auto t : unionTypes) {
      if (t->realizedName() == targetType->realizedName()) {
        suite->stmts.push_back(N<IfStmt>(
            N<BinaryExpr>(N<CallExpr>(N<IdExpr>("__internal__.union_get_tag:0"),
                                      N<IdExpr>(selfVar)),
                          "==", N<IntExpr>(tag)),
            N<ReturnStmt>(N<CallExpr>(N<IdExpr>("__internal__.union_get_data:0"),
                                      N<IdExpr>(selfVar),
                                      NT<IdExpr>(t->realizedName())))));
      }
      tag++;
    }
    suite->stmts.push_back(
        N<ThrowStmt>(N<CallExpr>(N<IdExpr>("std.internal.types.error.TypeError"),
                                 N<StringExpr>("invalid union getter"))));
    ast->suite = suite;
  } else if (startswith(ast->name, "__internal__.get_union_method:0")) {
    auto szt = type->funcGenerics[0].type->getStatic();
    auto fnName = szt->evaluate().getString();
    auto unionType = type->getArgTypes()[0]->getUnion();
    auto unionTypes = unionType->getRealizationTypes();

    auto selfVar = ast->args[0].name;
    auto suite = N<SuiteStmt>();
    int tag = 0;
    for (auto &t : unionTypes) {
      suite->stmts.push_back(N<IfStmt>(
          N<BinaryExpr>(N<CallExpr>(N<IdExpr>("__internal__.union_get_tag:0"),
                                    N<IdExpr>(selfVar)),
                        "==", N<IntExpr>(tag)),
          N<ReturnStmt>(N<CallExpr>(
              N<DotExpr>(N<CallExpr>(N<IdExpr>("__internal__.union_get_data:0"),
                                     N<IdExpr>(selfVar), NT<IdExpr>(t->realizedName())),
                         fnName),
              N<StarExpr>(N<IdExpr>(ast->args[2].name.substr(1))),
              N<KeywordStarExpr>(N<IdExpr>(ast->args[3].name.substr(2)))))));
      tag++;
    }
    suite->stmts.push_back(
        N<ThrowStmt>(N<CallExpr>(N<IdExpr>("std.internal.types.error.TypeError"),
                                 N<StringExpr>("invalid union call"))));
    ast->suite = suite;
  }
  return ast;
}

} // namespace codon::ast