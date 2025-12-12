// Copyright (C) 2022-2025 Exaloop Inc. <https://exaloop.io>

#include <limits>
#include <map>
#include <memory>
#include <string>
#include <tuple>
#include <vector>

#include "codon/cir/attribute.h"
#include "codon/cir/types/types.h"
#include "codon/parser/ast.h"
#include "codon/parser/common.h"
#include "codon/parser/visitors/scoping/scoping.h"
#include "codon/parser/visitors/translate/translate.h"
#include "codon/parser/visitors/typecheck/typecheck.h"

using namespace codon::error;

constexpr int MAX_TYPECHECK_ITER = 1000;

namespace codon::ast {

using namespace types;

/// Unify types a (passed by reference) and b.
/// Destructive operation as it modifies both a and b. If types cannot be unified, raise
/// an error.
/// @param a Type (by reference)
/// @param b Type
/// @return a
Type *TypecheckVisitor::unify(Type *a, Type *b) const {
  seqassert(a, "lhs is nullptr");
  if (!((*a) << b)) {
    types::Type::Unification undo;
    a->unify(b, &undo);
    // log("[unify] {} {}", a->debugString(2), b->debugString(2));
    // log("[unify] {} {}", a->debugString(1), b->debugString(1));
    E(Error::TYPE_UNIFY, getSrcInfo(), a->prettyString(), b->prettyString());
    return nullptr;
  }
  return a;
}

/// Infer all types within a Stmt *. Implements the LTS-DI typechecking.
/// @param isToplevel set if typechecking the program toplevel.
Stmt *TypecheckVisitor::inferTypes(Stmt *result, bool isToplevel) {
  if (!result)
    return nullptr;

  for (ctx->getBase()->iteration = 1;; ctx->getBase()->iteration++) {
    LOG_TYPECHECK("[iter] {} :: {}", ctx->getBase()->name, ctx->getBase()->iteration);
    if (ctx->getBase()->iteration >= MAX_TYPECHECK_ITER) {
      // log("-> {}", result->toString(2));
      ParserErrors errors;
      errors.addError(
          {ErrorMessage{fmt::format("cannot typecheck '{}' in reasonable time",
                                    ctx->getBase()->name.empty()
                                        ? "toplevel"
                                        : getUnmangledName(ctx->getBase()->name)),
                        result->getSrcInfo()}});
      for (auto &error : findTypecheckErrors(result))
        errors.addError(error);
      throw exc::ParserException(errors);
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
    // if (preamble) {
    //   auto pt = tv.transform(preamble);
    //   preamble = cast<SuiteStmt>(pt);
    // }
    result = tv.transform(result);
    std::swap(ctx->changedNodes, changedNodes);
    std::swap(ctx->returnEarly, returnEarly);
    ctx->typecheckLevel--;

    if (ctx->getBase()->iteration == 1 && isToplevel) {
      // Realize all @force_realize functions
      for (auto &f : ctx->cache->functions) {
        auto ast = f.second.ast;
        if (f.second.type && f.second.realizations.empty() &&
            (ast->hasAttribute(Attr::ForceRealize) || ast->hasAttribute(Attr::Export) ||
             (ast->hasAttribute(Attr::C) && !ast->hasAttribute(Attr::CVarArg)))) {
          seqassert(f.second.type->canRealize(), "cannot realize {}", f.first);
          LOG_REALIZE("[force_realize] {}", f.second.getType()->debugString(2));
          realize(instantiateType(f.second.getType()));
          seqassert(!f.second.realizations.empty(), "cannot realize {}", f.first);
        }
      }
    }

    if (result->isDone()) {
      // Special union case: if union cannot be inferred return type is Union[NoneType]
      if (auto tr = ctx->getBase()->returnType) {
        if (auto tu = tr->getUnion()) {
          if (!tu->isSealed()) {
            if (tu->pendingTypes[0]->getLink() &&
                tu->pendingTypes[0]->getLink()->kind == LinkType::Unbound) {
              auto r = tu->addType(getStdLibType("NoneType"));
              seqassert(r, "cannot add type to union {}", tu->debugString(2));
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
      if (auto t = ctx->getBase()->returnType) {
        ctx->getBase()->pendingDefaults[0].insert(t);
      }
      // First unify "explicit" generics (whose default type is explicit),
      // then "implicit" ones (whose default type is compiler generated,
      // e.g. compiler-generated variable placeholders with default NoneType)
      for (auto &unbounds : ctx->getBase()->pendingDefaults | std::views::values) {
        if (!unbounds.empty()) {
          for (const auto &unbound : unbounds) {
            if (auto tu = unbound->getUnion()) {
              // Seal all dynamic unions after the iteration is over
              if (!tu->isSealed()) {
                tu->seal();
                anotherRound = true;
              }
            } else if (auto u = unbound->getLink()) {
              types::Type::Unification undo;
              if (u->defaultType) {
                if (u->defaultType->getClass()) { // type[...]
                  if (u->unify(extractClassType(u->defaultType.get()), &undo) >= 0) {
                    anotherRound = true;
                  }
                } else { // generic
                  if (u->unify(u->defaultType.get(), &undo) >= 0) {
                    anotherRound = true;
                  }
                }
              }
            }
          }
          unbounds.clear();
          if (anotherRound)
            break;
        }
      }
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
types::Type *TypecheckVisitor::realize(types::Type *typ) {
  if (!typ || !typ->canRealize()) {
    return nullptr;
  }

  try {
    if (auto f = typ->getFunc()) {
      // Cache::CTimer t(ctx->cache, f->realizedName());
      if (auto ret = realizeFunc(f)) {
        // Realize Function[..] type as well
        auto t = std::make_shared<ClassType>(ret->getClass());
        realizeType(t.get());
        // Needed for return type unification
        unify(f->getRetType(), extractClassGeneric(ret, 1));
        return ret;
      }
    } else if (auto c = typ->getClass()) {
      auto t = realizeType(c);
      return t;
    }
  } catch (exc::ParserException &exc) {
    seqassert(!exc.getErrors().empty(), "empty error trace");
    auto &bt = exc.getErrors().back();
    if (bt.front().getErrorCode() == Error::MAX_REALIZATION)
      throw;
    if (auto f = typ->getFunc()) {
      if (f->ast->hasAttribute(Attr::HiddenFromUser)) {
        bt.back().setSrcInfo(getSrcInfo());
      } else {
        std::vector<std::string> args;
        for (size_t i = 0, ai = 0, gi = 0; i < f->ast->size(); i++) {
          auto [ns, n] = (*f->ast)[i].getNameWithStars();
          args.push_back(fmt::format(
              "{}{}: {}", std::string(ns, '*'), getUserFacingName(n),
              (*f->ast)[i].isGeneric() ? extractFuncGeneric(f, gi++)->prettyString()
                                       : extractFuncArgType(f, ai++)->prettyString()));
        }
        auto name = f->ast->name;
        std::string name_args;
        if (startswith(name, "%_import_")) {
          for (auto &i : ctx->cache->imports | std::views::values)
            if (getMangledFunc("", i.importVar + "_call") == name) {
              name = i.name;
              break;
            }
          name = fmt::format("<import {}>", name);
        } else {
          name = getUserFacingName(f->ast->getName());
          name_args = fmt::format("({})", join(args, ", "));
        }
        bt.addMessage(fmt::format("during the realization of {}{}", name, name_args),
                      getSrcInfo());
      }
    } else {
      bt.addMessage(fmt::format("during the realization of {}", typ->prettyString()),
                    getSrcInfo());
    }
    throw;
  }
  return nullptr;
}

/// Realize a type and create IR type stub.
/// @return realized type or nullptr if the type cannot be realized
types::Type *TypecheckVisitor::realizeType(types::ClassType *type) {
  if (!type || !type->canRealize())
    return nullptr;
  // Check if the type fields are all initialized
  // (sometimes that's not the case: e.g., `class X: x: List[X]`)

  // generalize generics to ensure that they do not get unified later!
  if (type->is("unrealized_type"))
    type->generics[0].type = extractClassGeneric(type)->generalize(0);

  if (type->is("__NTuple__")) {
    auto n = std::max(static_cast<int64_t>(0), getIntLiteral(type));
    auto tt = extractClassGeneric(type, 1)->getClass();
    std::vector<ClassType::Generic> generics;
    auto t = instantiateType(generateTuple(n * tt->generics.size()));
    for (size_t i = 0, j = 0; i < n; i++)
      for (const auto &ttg : tt->generics) {
        unify(t->generics[j].getType(), ttg.getType());
        generics.push_back(t->generics[j]);
        j++;
      }
    type->name = TYPE_TUPLE;
    type->generics = generics;
    type->_rn = "";
  }

  // Check if the type was already realized
  auto rn = type->ClassType::realizedName();
  auto cls = getClass(type);
  if (auto r = in(cls->realizations, rn)) {
    return (*r)->type->getClass();
  }

  auto realized = type->getClass();
  auto fields = getClassFields(realized);
  if (!cls->ast)
    return nullptr; // not yet done!
  auto fTypes = getClassFieldTypes(realized);
  for (auto &field : fTypes) {
    if (!field)
      return nullptr;
  }

  if (auto s = type->getStatic())
    realized =
        s->getNonStaticType()->getClass(); // do not cache static but its root type!

  // Realize generics
  if (!type->is("unrealized_type"))
    for (auto &e : realized->generics) {
      if (!realize(e.getType()))
        return nullptr;
      if (e.type->getFunc() && !e.type->getFunc()->getRetType()->canRealize())
        return nullptr;
    }

  // Realizations should always be visible, so add them to the toplevel
  rn = type->ClassType::realizedName();
  auto rt = std::static_pointer_cast<ClassType>(realized->generalize(0));
  auto val = std::make_shared<TypecheckItem>(rn, "", ctx->getModule(), rt);
  if (!val->type->is(TYPE_TYPE))
    val->type = instantiateTypeVar(realized);
  ctx->addAlwaysVisible(val, true);
  auto realization = getClass(realized)->realizations[rn] =
      std::make_shared<Cache::Class::ClassRealization>();
  realization->type = rt;
  realization->id = ++ctx->cache->classRealizationCnt;

  // Create LLVM stub
  auto lt = makeIRType(realized);

  // Realize fields
  std::vector<ir::types::Type *> typeArgs;   // needed for IR
  std::vector<std::string> names;            // needed for IR
  std::map<std::string, SrcInfo> memberInfo; // needed for IR
  for (size_t i = 0; i < fTypes.size(); i++) {
    if (!realize(fTypes[i].get())) {
      // realize(fTypes[i].get());
      E(Error::TYPE_CANNOT_REALIZE_ATTR, getSrcInfo(), fields[i].name,
        realized->prettyString());
    }
    // LOG_REALIZE("- member: {} -> {}: {}", field.name, field.type, fTypes[i]);
    realization->fields.emplace_back(fields[i].name, fTypes[i]);
    names.emplace_back(fields[i].name);
    typeArgs.emplace_back(makeIRType(fTypes[i]->getClass()));
    memberInfo[fields[i].name] = fTypes[i]->getSrcInfo();
  }

  // Set IR attributes
  if (!names.empty()) {
    if (auto *ir = cast<ir::types::RefType>(lt)) {
      ir->getContents()->realize(typeArgs, names);
      ir->setAttribute(std::make_unique<ir::MemberAttribute>(memberInfo));
      ir->getContents()->setAttribute(
          std::make_unique<ir::MemberAttribute>(memberInfo));
    }
  }

  return rt.get();
}

types::Type *TypecheckVisitor::realizeFunc(types::FuncType *type, bool force) {
  auto module = type->ast->getAttribute<ir::StringValueAttribute>(Attr::Module)->value;
  auto &realizations = getFunction(type->getFuncName())->realizations;
  auto imp = getImport(module);
  if (auto r = in(realizations, type->realizedName())) {
    if (!force) {
      return (*r)->getType();
    }
  }

  // auto *_t = new Cache::CTimer(ctx->cache, ctx->getRealizationStackName() + ":" +
  //                                              type->realizedName());

  auto oldCtx = this->ctx;
  this->ctx = imp->ctx;
  if (ctx->getRealizationDepth() > MAX_REALIZATION_DEPTH) {
    E(Error::MAX_REALIZATION, getSrcInfo(), getUserFacingName(type->getFuncName()));
  }

  bool isImport = isImportFn(type->getFuncName());
  if (!isImport) {
    getLogger().level++;
    ctx->addBlock();
    ctx->typecheckLevel++;
    ctx->bases.push_back({type->getFuncName(), type->getFunc()->shared_from_this(),
                          type->getRetType()->shared_from_this()});
    for (size_t t = ctx->bases.size() - 1; t-- > 0;) {
      if (startswith(ctx->getBaseName(), ctx->bases[t].name)) {
        ctx->getBase()->parent = static_cast<int>(t);
        break;
      }
    }
    // LOG("[realize] F {} -> {} : base {} ; depth = {} ; ctx-base: {}; ret = {}; "
    //     "parent = {}",
    //     type->getFuncName(), type->realizedName(), ctx->getRealizationStackName(),
    //     ctx->getRealizationDepth(), ctx->getBaseName(),
    //     ctx->getBase()->returnType->debugString(2),
    //     ctx->bases[ctx->getBase()->parent].name);
  }

  // Types might change after realization, fix it
  for (auto &t : *type)
    realizeType(t.getType()->getClass());

  // Clone the generic AST that is to be realized
  auto ast = clean_clone(type->ast);
  if (auto s = generateSpecialAst(type))
    ast->suite = s;
  addClassGenerics(type, true);
  ctx->getBase()->func = ast;

  // Internal functions have no AST that can be realized
  bool hasAst = ast->getSuite() && !ast->hasAttribute(Attr::Internal);
  // Add function arguments
  if (auto b = ast->getAttribute<BindingsAttribute>(Attr::Bindings)) {
    for (auto &[c, t] : b->captures) {
      if (t == BindingsAttribute::CaptureType::Global) {
        auto cp = ctx->find(c);
        if (!cp)
          E(Error::ID_NOT_FOUND, getSrcInfo(), c);
        if (!cp->isGlobal())
          E(Error::FN_GLOBAL_NOT_FOUND, getSrcInfo(), "global", c);
      }
    }
    for (const auto [name, canonical] : b->localRenames) {
      auto val = ctx->forceFind(canonical);
      ctx->add(name, val);
    }
  }
  for (size_t i = 0, j = 0, gi = 0; hasAst && i < ast->size(); i++) {
    auto [_, varName] = (*ast)[i].getNameWithStars();
    auto un = getUnmangledName(varName);
    if ((*ast)[i].isValue()) {
      TypePtr at = extractFuncArgType(type, j++)->shared_from_this();
      bool isStatic = ast && getStaticGeneric((*ast)[i].getType());
      if (!isStatic && at && at->getStatic())
        at = at->getStatic()->getNonStaticType()->shared_from_this();

      if (startswith(un, "$"))
        un = un.substr(1);
      if (at->is("TypeWrap")) {
        ctx->addType(un, varName, instantiateTypeVar(extractClassGeneric(at.get())));
      } else {
        ctx->addVar(un, varName, std::make_shared<LinkType>(at));
      }
    } else {
      if (startswith(un, "$")) {
        un = un.substr(1);
        auto g = type->funcGenerics[gi];
        auto t = g.type;
        if (!g.staticKind && !t->is(TYPE_TYPE))
          t = instantiateTypeVar(t.get());
        auto v = ctx->addType(un, varName, t);
        v->generic = true;
      }
      gi++;
    }
  }

  // Populate realization table in advance to support recursive realizations
  auto key = type->realizedName(); // note: the key might change later
  ir::Func *oldIR = nullptr;       // Get it if it was already made (force mode)
  if (auto i = in(realizations, key))
    oldIR = (*i)->ir;
  auto r = realizations[key] = std::make_shared<Cache::Function::FunctionRealization>();
  r->type = std::static_pointer_cast<FuncType>(type->shared_from_this());
  r->ir = oldIR;
  if (auto b = ast->getAttribute<BindingsAttribute>(Attr::Bindings))
    for (const auto &c : b->captures | std::views::keys) {
      auto h = ctx->find(c);
      r->captures.push_back(h ? h->canonicalName : "");
    }

  // Realizations should always be visible, so add them to the toplevel
  auto val = std::make_shared<TypecheckItem>(key, "", ctx->getModule(),
                                             type->shared_from_this());
  ctx->addAlwaysVisible(val, true);

  ctx->getBase()->suite = ast->getSuite();
  if (hasAst) {
    auto oldBlockLevel = ctx->blockLevel;
    ctx->blockLevel = 0;
    auto ret = inferTypes(ctx->getBase()->suite);
    ctx->blockLevel = oldBlockLevel;

    if (!ret) {
      realizations.erase(key);
      ParserErrors errors;
      if (!startswith(ast->name, "%_lambda")) {
        // Lambda typecheck failures are "ignored" as they are treated as statements,
        // not functions.
        // TODO: generalize this further.
        errors = findTypecheckErrors(ctx->getBase()->suite);
      }
      if (!isImport) {
        ctx->bases.pop_back();
        ctx->popBlock();
        ctx->typecheckLevel--;
        getLogger().level--;
      }
      if (!errors.empty()) {
        throw exc::ParserException(errors);
      }
      this->ctx = oldCtx;
      return nullptr; // inference must be delayed
    } else {
      ctx->getBase()->suite = ret;
    }
    // Use NoneType as the return type when the return type is not specified and
    // function has no return statement
    if (!ast->getReturn() && isUnbound(type->getRetType())) {
      unify(type->getRetType(), getStdLibType("NoneType"));
    }
  }
  // Realize the return type
  auto ret = realize(type->getRetType());
  if (type->hasUnbounds(/*includeGenerics*/ false)) {
    // log("cannot realize {}; undoing...", type->debugString(2));
    realizations.erase(key);
    ctx->bases.pop_back();
    ctx->popBlock();
    ctx->typecheckLevel--;
    getLogger().level--;
    return nullptr;
  }
  seqassert(ret, "cannot realize return type '{}'", *(type->getRetType()));

  std::vector<Param> args;
  for (auto &i : *ast) {
    auto [_, varName] = i.getNameWithStars();
    args.emplace_back(varName, nullptr, nullptr, i.status);
  }
  r->ast =
      N<FunctionStmt>(r->type->realizedName(), nullptr, args, ctx->getBase()->suite,
                      std::vector<Expr *>{}, ast->isAsync());
  r->ast->setSrcInfo(ast->getSrcInfo());
  r->ast->cloneAttributesFrom(ast);

  auto newType = std::static_pointer_cast<types::FuncType>(type->generalize(0));
  auto newKey = newType->realizedName();

  if (!in(ctx->cache->pendingRealizations, make_pair(newType->getFuncName(), newKey))) {
    realizations[newKey] = r;
  } else {
    realizations[key] = realizations[newKey];
  }
  if (force)
    realizations[newKey]->ast = r->ast;
  r->type = newType;
  if (!r->ir)
    r->ir = makeIRFunction(r);
  val = std::make_shared<TypecheckItem>(newKey, "", ctx->getModule(), r->type);
  ctx->addAlwaysVisible(val, true);
  if (!isImport) {
    ctx->bases.pop_back();
    ctx->popBlock();
    ctx->typecheckLevel--;
    getLogger().level--;
  }
  this->ctx = oldCtx;

  LOG_REALIZE("[func] {}", r->getType()->debugString(2));

  return r->getType();
}

/// Make IR node for a realized type.
ir::types::Type *TypecheckVisitor::makeIRType(types::ClassType *t) {
  // Realize if not, and return cached value if it exists
  auto realizedName = t->ClassType::realizedName();
  auto cls = ctx->cache->getClass(t);
  if (!in(cls->realizations, realizedName)) {
    t = realize(t->getClass())->getClass();
    realizedName = t->ClassType::realizedName();
    cls = ctx->cache->getClass(t);
  }
  if (auto l = cls->realizations[realizedName]->ir) {
    if (cls->rtti)
      cast<ir::types::RefType>(l)->setPolymorphic();
    return l;
  }

  auto forceFindIRType = [&](Type *tt) {
    auto ttc = tt->getClass();
    auto rn = ttc->ClassType::realizedName();
    auto ttcls = ctx->cache->getClass(ttc);
    seqassert(ttc && in(ttcls->realizations, rn), "{} not realized", *tt);
    auto l = ttcls->realizations[rn]->ir;
    seqassert(l, "no LLVM type for {}", *tt);
    return l;
  };

  // Prepare generics and statics
  std::vector<ir::types::Type *> types;
  std::vector<types::StaticType *> statics;
  if (t->is("unrealized_type"))
    types.push_back(nullptr);
  else
    for (auto &m : t->generics) {
      if (auto s = m.type->getStatic())
        statics.push_back(s);
      else
        types.push_back(forceFindIRType(m.getType()));
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
    handle =
        module->Nr<ir::types::IntNType>(getIntLiteral(statics[0]), t->name == "Int");
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
        cast<ir::types::RecordType>(module->unsafeGetMemberedType(realizedName));
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
    for (auto &m : extractClassGeneric(t)->getClass()->generics)
      types.push_back(forceFindIRType(m.getType()));
    auto ret = forceFindIRType(extractClassGeneric(t, 1));
    handle = module->unsafeGetFuncType(realizedName, ret, types);
  } else if (t->name == getMangledClass("std.simd", "Vec")) {
    seqassert(types.size() == 1 && !statics.empty(), "bad generics/statics");
    handle = module->unsafeGetVectorType(getIntLiteral(statics[0]), types[0]);
  } else {
    // Type arguments will be populated afterwards to avoid infinite loop with recursive
    // reference types (e.g., `class X: x: Optional[X]`)
    if (t->isRecord()) {
      std::vector<ir::types::Type *> typeArgs;   // needed for IR
      std::vector<std::string> names;            // needed for IR
      std::map<std::string, SrcInfo> memberInfo; // needed for IR

      seqassert(!t->is("__NTuple__"), "ntuple not inlined");
      auto ft = getClassFieldTypes(t->getClass());
      const auto &fields = cls->fields;
      for (size_t i = 0; i < ft.size(); i++) {
        if (!realize(ft[i].get())) {
          E(Error::TYPE_CANNOT_REALIZE_ATTR, getSrcInfo(), fields[i].name,
            t->prettyString());
        }
        names.emplace_back(fields[i].name);
        typeArgs.emplace_back(makeIRType(ft[i]->getClass()));
        memberInfo[fields[i].name] = ft[i]->getSrcInfo();
      }
      auto record =
          cast<ir::types::RecordType>(module->unsafeGetMemberedType(realizedName));
      record->realize(typeArgs, names);
      handle = record;
      handle->setAttribute(
          std::make_unique<ir::MemberAttribute>(std::move(memberInfo)));
    } else {
      handle = module->unsafeGetMemberedType(realizedName, !t->isRecord());
      if (cls->rtti)
        cast<ir::types::RefType>(handle)->setPolymorphic();
    }
  }
  handle->setSrcInfo(t->getSrcInfo());
  handle->setAstType(
      std::const_pointer_cast<codon::ast::types::Type>(t->shared_from_this()));
  return cls->realizations[realizedName]->ir = handle;
}

/// Make IR node for a realized function.
ir::Func *TypecheckVisitor::makeIRFunction(
    const std::shared_ptr<Cache::Function::FunctionRealization> &r) {
  ir::Func *fn = nullptr;
  auto irm = ctx->cache->module;
  // Create and store a function IR node and a realized AST for IR passes
  if (r->ast->hasAttribute(Attr::Internal)) {
    // e.g., __new__, Ptr.__new__, etc.
    fn = irm->Nr<ir::InternalFunc>(r->type->ast->name);
  } else if (r->ast->hasAttribute(Attr::LLVM)) {
    fn = irm->Nr<ir::LLVMFunc>(r->type->realizedName());
  } else if (r->ast->hasAttribute(Attr::C)) {
    fn = irm->Nr<ir::ExternalFunc>(r->type->realizedName());
  } else {
    fn = irm->Nr<ir::BodiedFunc>(r->type->realizedName());
  }
  fn->setUnmangledName(ctx->cache->reverseIdentifierLookup[r->type->ast->name]);
  auto parent = r->type->funcParent;
  if (auto aa = r->ast->getAttribute<ir::StringValueAttribute>(Attr::ParentClass)) {
    if (!aa->value.empty() && !r->ast->hasAttribute(Attr::Method)) {
      // Hack for non-generic methods
      parent = ctx->find(aa->value)->type;
    }
  }
  if (parent && parent->isInstantiated() && parent->canRealize()) {
    parent = extractClassType(parent.get())->shared_from_this();
    realize(parent.get());
    fn->setParentType(makeIRType(parent->getClass()));
  }
  fn->setGlobal();
  // Mark this realization as pending (i.e., realized but not translated)
  ctx->cache->pendingRealizations.insert({r->type->ast->name, r->type->realizedName()});

  seqassert(!r->type ||
                r->ast->size() == r->type->size() + r->type->funcGenerics.size(),
            "type/AST argument mismatch");

  // Populate the IR node
  std::vector<std::string> names;
  std::vector<codon::ir::types::Type *> types;
  for (size_t i = 0, j = 0; i < r->ast->size(); i++) {
    if ((*r->ast)[i].isValue()) {
      if (!extractFuncArgType(r->getType(), j)->getFunc()) {
        types.push_back(makeIRType(extractFuncArgType(r->getType(), j)->getClass()));
        names.push_back(ctx->cache->reverseIdentifierLookup[(*r->ast)[i].getName()]);
      }
      j++;
    }
  }
  if (r->ast->hasAttribute(Attr::CVarArg)) {
    types.pop_back();
    names.pop_back();
  }
  auto irType = irm->unsafeGetFuncType(r->type->realizedName(),
                                       makeIRType(r->type->getRetType()->getClass()),
                                       types, r->ast->hasAttribute(Attr::CVarArg));
  irType->setAstType(r->type->shared_from_this());
  fn->realize(irType, names);
  return fn;
}

ir::Func *TypecheckVisitor::realizeIRFunc(types::FuncType *fn,
                                          const std::vector<types::TypePtr> &generics) {
  // TODO: used by cytonization. Probably needs refactoring.
  auto fnType = instantiateType(fn);
  types::Type::Unification u;
  for (size_t i = 0; i < generics.size(); i++)
    fnType->getFunc()->funcGenerics[i].type->unify(generics[i].get(), &u);
  if (!realize(fnType.get()))
    return nullptr;

  auto pr = ctx->cache->pendingRealizations; // copy it as it might be modified
  for (const auto &key : pr | std::views::keys)
    TranslateVisitor(ctx->cache->codegenCtx)
        .translateStmts(clone(getFunction(key)->ast));
  return getFunction(fn->ast->getName())->realizations[fnType->realizedName()]->ir;
}

} // namespace codon::ast
