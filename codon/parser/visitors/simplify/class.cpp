// Copyright (C) 2022-2024 Exaloop Inc. <https://exaloop.io>

#include <memory>
#include <string>
#include <tuple>
#include <vector>

#include "codon/parser/ast.h"
#include "codon/parser/cache.h"
#include "codon/parser/common.h"
#include "codon/parser/visitors/simplify/simplify.h"

using fmt::format;
using namespace codon::error;

namespace codon::ast {

/// Transform class and type definitions, as well as extensions.
/// See below for details.
void SimplifyVisitor::visit(ClassStmt *stmt) {
  // Get root name
  std::string name = stmt->name;

  // Generate/find class' canonical name (unique ID) and AST
  std::string canonicalName;
  std::vector<Param> &argsToParse = stmt->args;

  // classItem will be added later when the scope is different
  auto classItem = std::make_shared<SimplifyItem>(SimplifyItem::Type, "", "",
                                                  ctx->getModule(), ctx->scope.blocks);
  classItem->setSrcInfo(stmt->getSrcInfo());
  if (!stmt->attributes.has(Attr::Extend)) {
    classItem->canonicalName = canonicalName =
        ctx->generateCanonicalName(name, !stmt->attributes.has(Attr::Internal));
    // Reference types are added to the context here.
    // Tuple types are added after class contents are parsed to prevent
    // recursive record types (note: these are allowed for reference types)
    if (!stmt->attributes.has(Attr::Tuple)) {
      ctx->add(name, classItem);
      ctx->addAlwaysVisible(classItem);
    }
  } else {
    // Find the canonical name and AST of the class that is to be extended
    if (!ctx->isGlobal() || ctx->isConditional())
      E(Error::EXPECTED_TOPLEVEL, getSrcInfo(), "class extension");
    auto val = ctx->find(name);
    if (!val || !val->isType())
      E(Error::CLASS_ID_NOT_FOUND, getSrcInfo(), name);
    canonicalName = val->canonicalName;
    const auto &astIter = ctx->cache->classes.find(canonicalName);
    if (astIter == ctx->cache->classes.end()) {
      E(Error::CLASS_ID_NOT_FOUND, getSrcInfo(), name);
    } else {
      argsToParse = astIter->second.ast->args;
    }
  }

  std::vector<StmtPtr> clsStmts; // Will be filled later!
  std::vector<StmtPtr> varStmts; // Will be filled later!
  std::vector<StmtPtr> fnStmts;  // Will be filled later!
  std::vector<SimplifyContext::Item> addLater;
  try {
    // Add the class base
    SimplifyContext::BaseGuard br(ctx.get(), canonicalName);

    // Parse and add class generics
    std::vector<Param> args;
    std::pair<StmtPtr, FunctionStmt *> autoDeducedInit{nullptr, nullptr};
    if (stmt->attributes.has("deduce") && args.empty()) {
      // Auto-detect generics and fields
      autoDeducedInit = autoDeduceMembers(stmt, args);
    } else {
      // Add all generics before parent classes, fields and methods
      for (auto &a : argsToParse) {
        if (a.status != Param::Generic)
          continue;
        std::string genName, varName;
        if (stmt->attributes.has(Attr::Extend))
          varName = a.name, genName = ctx->cache->rev(a.name);
        else
          varName = ctx->generateCanonicalName(a.name), genName = a.name;
        if (auto st = getStaticGeneric(a.type.get())) {
          auto val = ctx->addVar(genName, varName, a.type->getSrcInfo());
          val->generic = true;
          val->staticType = st;
        } else {
          ctx->addType(genName, varName, a.type->getSrcInfo())->generic = true;
        }
        args.emplace_back(varName, transformType(clone(a.type), false),
                          transformType(clone(a.defaultValue), false), a.status);
        if (!stmt->attributes.has(Attr::Extend) && a.status == Param::Normal)
          ctx->cache->classes[canonicalName].fields.push_back(
              Cache::Class::ClassField{varName, nullptr, canonicalName});
      }
    }

    // Form class type node (e.g. `Foo`, or `Foo[T, U]` for generic classes)
    ExprPtr typeAst = N<IdExpr>(name), transformedTypeAst = NT<IdExpr>(canonicalName);
    for (auto &a : args) {
      if (a.status == Param::Generic) {
        if (!typeAst->getIndex()) {
          typeAst = N<IndexExpr>(N<IdExpr>(name), N<TupleExpr>());
          transformedTypeAst =
              NT<InstantiateExpr>(NT<IdExpr>(canonicalName), std::vector<ExprPtr>{});
        }
        typeAst->getIndex()->index->getTuple()->items.push_back(N<IdExpr>(a.name));
        CAST(transformedTypeAst, InstantiateExpr)
            ->typeParams.push_back(transform(N<IdExpr>(a.name), true));
      }
    }

    // Collect classes (and their fields) that are to be statically inherited
    std::vector<ClassStmt *> staticBaseASTs, baseASTs;
    if (!stmt->attributes.has(Attr::Extend)) {
      staticBaseASTs = parseBaseClasses(stmt->staticBaseClasses, args, stmt->attributes,
                                        canonicalName);
      if (ctx->cache->isJit && !stmt->baseClasses.empty())
        E(Error::CUSTOM, stmt->baseClasses[0],
          "inheritance is not yet supported in JIT mode");
      parseBaseClasses(stmt->baseClasses, args, stmt->attributes, canonicalName,
                       transformedTypeAst);
    }

    // A ClassStmt will be separated into class variable assignments, method-free
    // ClassStmts (that include nested classes) and method FunctionStmts
    transformNestedClasses(stmt, clsStmts, varStmts, fnStmts);

    // Collect class fields
    for (auto &a : argsToParse) {
      if (a.status == Param::Normal) {
        if (!ClassStmt::isClassVar(a)) {
          args.emplace_back(a.name, transformType(clone(a.type), false),
                            transform(clone(a.defaultValue), true));
          if (!stmt->attributes.has(Attr::Extend)) {
            ctx->cache->classes[canonicalName].fields.push_back(
                Cache::Class::ClassField{a.name, nullptr, canonicalName});
          }
        } else if (!stmt->attributes.has(Attr::Extend)) {
          // Handle class variables. Transform them later to allow self-references
          auto name = format("{}.{}", canonicalName, a.name);
          preamble->push_back(N<AssignStmt>(N<IdExpr>(name), nullptr, nullptr));
          ctx->cache->addGlobal(name);
          auto assign = N<AssignStmt>(N<IdExpr>(name), a.defaultValue,
                                      a.type ? a.type->getIndex()->index : nullptr);
          assign->setUpdate();
          varStmts.push_back(assign);
          ctx->cache->classes[canonicalName].classVars[a.name] = name;
        }
      }
    }

    // ASTs for member arguments to be used for populating magic methods
    std::vector<Param> memberArgs;
    for (auto &a : args) {
      if (a.status == Param::Normal)
        memberArgs.push_back(a.clone());
    }

    // Parse class members (arguments) and methods
    if (!stmt->attributes.has(Attr::Extend)) {
      // Now that we are done with arguments, add record type to the context
      if (stmt->attributes.has(Attr::Tuple)) {
        // Ensure that class binding does not shadow anything.
        // Class bindings cannot be dominated either
        auto v = ctx->find(name);
        if (v && v->noShadow)
          E(Error::CLASS_INVALID_BIND, stmt, name);
        ctx->add(name, classItem);
        ctx->addAlwaysVisible(classItem);
      }
      // Create a cached AST.
      stmt->attributes.module =
          format("{}{}", ctx->moduleName.status == ImportFile::STDLIB ? "std::" : "::",
                 ctx->moduleName.module);
      ctx->cache->classes[canonicalName].ast =
          N<ClassStmt>(canonicalName, args, N<SuiteStmt>(), stmt->attributes);
      ctx->cache->classes[canonicalName].ast->baseClasses = stmt->baseClasses;
      for (auto &b : staticBaseASTs)
        ctx->cache->classes[canonicalName].staticParentClasses.emplace_back(b->name);
      ctx->cache->classes[canonicalName].ast->validate();
      ctx->cache->classes[canonicalName].module = ctx->getModule();

      // Codegen default magic methods
      for (auto &m : stmt->attributes.magics) {
        fnStmts.push_back(transform(
            codegenMagic(m, typeAst, memberArgs, stmt->attributes.has(Attr::Tuple))));
      }
      // Add inherited methods
      for (auto &base : staticBaseASTs) {
        for (auto &mm : ctx->cache->classes[base->name].methods)
          for (auto &mf : ctx->cache->overloads[mm.second]) {
            auto f = ctx->cache->functions[mf.name].ast;
            if (!f->attributes.has("autogenerated")) {
              std::string rootName;
              auto &mts = ctx->cache->classes[ctx->getBase()->name].methods;
              auto it = mts.find(ctx->cache->rev(f->name));
              if (it != mts.end())
                rootName = it->second;
              else
                rootName = ctx->generateCanonicalName(ctx->cache->rev(f->name), true);
              auto newCanonicalName =
                  format("{}:{}", rootName, ctx->cache->overloads[rootName].size());
              ctx->cache->overloads[rootName].push_back(
                  {newCanonicalName, ctx->cache->age});
              ctx->cache->reverseIdentifierLookup[newCanonicalName] =
                  ctx->cache->rev(f->name);
              auto nf = std::dynamic_pointer_cast<FunctionStmt>(f->clone());
              nf->name = newCanonicalName;
              nf->attributes.parentClass = ctx->getBase()->name;
              ctx->cache->functions[newCanonicalName].ast = nf;
              ctx->cache->classes[ctx->getBase()->name]
                  .methods[ctx->cache->rev(f->name)] = rootName;
              fnStmts.push_back(nf);
            }
          }
      }
      // Add auto-deduced __init__ (if available)
      if (autoDeducedInit.first)
        fnStmts.push_back(autoDeducedInit.first);
    }
    // Add class methods
    for (const auto &sp : getClassMethods(stmt->suite))
      if (sp && sp->getFunction()) {
        if (sp.get() != autoDeducedInit.second) {
          auto &ds = sp->getFunction()->decorators;
          for (auto &dc : ds) {
            if (auto d = dc->getDot()) {
              if (d->member == "setter" and d->expr->isId(sp->getFunction()->name) &&
                  sp->getFunction()->args.size() == 2) {
                sp->getFunction()->name = format(".set_{}", sp->getFunction()->name);
                dc = nullptr;
                break;
              }
            }
          }
          fnStmts.push_back(transform(sp));
        }
      }

    // After popping context block, record types and nested classes will disappear.
    // Store their references and re-add them to the context after popping
    addLater.reserve(clsStmts.size() + 1);
    for (auto &c : clsStmts)
      addLater.push_back(ctx->find(c->getClass()->name));
    if (stmt->attributes.has(Attr::Tuple))
      addLater.push_back(ctx->forceFind(name));

    // Mark functions as virtual:
    auto banned =
        std::set<std::string>{"__init__", "__new__", "__raw__", "__tuplesize__"};
    for (auto &m : ctx->cache->classes[canonicalName].methods) {
      auto method = m.first;
      for (size_t mi = 1; mi < ctx->cache->classes[canonicalName].mro.size(); mi++) {
        // ... in the current class
        auto b = ctx->cache->classes[canonicalName].mro[mi]->getTypeName();
        if (in(ctx->cache->classes[b].methods, method) && !in(banned, method)) {
          ctx->cache->classes[canonicalName].virtuals.insert(method);
        }
      }
      for (auto &v : ctx->cache->classes[canonicalName].virtuals) {
        for (size_t mi = 1; mi < ctx->cache->classes[canonicalName].mro.size(); mi++) {
          // ... and in parent classes
          auto b = ctx->cache->classes[canonicalName].mro[mi]->getTypeName();
          ctx->cache->classes[b].virtuals.insert(v);
        }
      }
    }
  } catch (const exc::ParserException &) {
    if (!stmt->attributes.has(Attr::Tuple))
      ctx->remove(name);
    ctx->cache->classes.erase(name);
    throw;
  }
  for (auto &i : addLater)
    ctx->add(ctx->cache->rev(i->canonicalName), i);

  // Extensions are not needed as the cache is already populated
  if (!stmt->attributes.has(Attr::Extend)) {
    auto c = ctx->cache->classes[canonicalName].ast;
    seqassert(c, "not a class AST for {}", canonicalName);
    clsStmts.push_back(c);
  }

  clsStmts.insert(clsStmts.end(), fnStmts.begin(), fnStmts.end());
  for (auto &a : varStmts) {
    // Transform class variables here to allow self-references
    if (auto assign = a->getAssign()) {
      transform(assign->rhs);
      transformType(assign->type);
    }
    clsStmts.push_back(a);
  }
  resultStmt = N<SuiteStmt>(clsStmts);
}

/// Parse statically inherited classes.
/// Returns a list of their ASTs. Also updates the class fields.
/// @param args Class fields that are to be updated with base classes' fields.
/// @param typeAst Transformed AST for base class type (e.g., `A[T]`).
///                Only set when dealing with dynamic polymorphism.
std::vector<ClassStmt *> SimplifyVisitor::parseBaseClasses(
    std::vector<ExprPtr> &baseClasses, std::vector<Param> &args, const Attr &attr,
    const std::string &canonicalName, const ExprPtr &typeAst) {
  std::vector<ClassStmt *> asts;

  // MAJOR TODO: fix MRO it to work with generic classes (maybe replacements? IDK...)
  std::vector<std::vector<ExprPtr>> mro{{typeAst}};
  std::vector<ExprPtr> parentClasses;
  for (auto &cls : baseClasses) {
    std::string name;
    std::vector<ExprPtr> subs;

    // Get the base class and generic replacements (e.g., if there is Bar[T],
    // Bar in Foo(Bar[int]) will have `T = int`)
    transformType(cls);
    if (auto i = cls->getId()) {
      name = i->value;
    } else if (auto e = CAST(cls, InstantiateExpr)) {
      if (auto ei = e->typeExpr->getId()) {
        name = ei->value;
        subs = e->typeParams;
      }
    }

    auto cachedCls = const_cast<Cache::Class *>(in(ctx->cache->classes, name));
    if (!cachedCls)
      E(Error::CLASS_ID_NOT_FOUND, getSrcInfo(), ctx->cache->rev(name));
    asts.push_back(cachedCls->ast.get());
    parentClasses.push_back(clone(cls));
    mro.push_back(cachedCls->mro);

    // Sanity checks
    if (attr.has(Attr::Tuple) && typeAst)
      E(Error::CLASS_NO_INHERIT, getSrcInfo(), "tuple");
    if (!attr.has(Attr::Tuple) && asts.back()->attributes.has(Attr::Tuple))
      E(Error::CLASS_TUPLE_INHERIT, getSrcInfo());
    if (asts.back()->attributes.has(Attr::Internal))
      E(Error::CLASS_NO_INHERIT, getSrcInfo(), "internal");

    // Mark parent classes as polymorphic as well.
    if (typeAst) {
      cachedCls->rtti = true;
    }

    // Add generics first
    int nGenerics = 0;
    for (auto &a : asts.back()->args)
      nGenerics += a.status == Param::Generic;
    int si = 0;
    for (auto &a : asts.back()->args) {
      if (a.status == Param::Generic) {
        if (si == subs.size())
          E(Error::GENERICS_MISMATCH, cls, ctx->cache->rev(asts.back()->name),
            nGenerics, subs.size());
        args.emplace_back(a.name, a.type, transformType(subs[si++], false),
                          Param::HiddenGeneric);
      } else if (a.status == Param::HiddenGeneric) {
        args.emplace_back(a);
      }
      if (a.status != Param::Normal) {
        if (auto st = getStaticGeneric(a.type.get())) {
          auto val = ctx->addVar(a.name, a.name, a.type->getSrcInfo());
          val->generic = true;
          val->staticType = st;
        } else {
          ctx->addType(a.name, a.name, a.type->getSrcInfo())->generic = true;
        }
      }
    }
    if (si != subs.size())
      E(Error::GENERICS_MISMATCH, cls, ctx->cache->rev(asts.back()->name), nGenerics,
        subs.size());
  }
  // Add normal fields
  for (auto &ast : asts) {
    int ai = 0;
    for (auto &a : ast->args) {
      if (a.status == Param::Normal && !ClassStmt::isClassVar(a)) {
        auto name = a.name;
        int i = 0;
        for (auto &aa : args)
          i += aa.name == a.name || startswith(aa.name, a.name + "#");
        if (i)
          name = format("{}#{}", name, i);
        seqassert(ctx->cache->classes[ast->name].fields[ai].name == a.name,
                  "bad class fields: {} vs {}",
                  ctx->cache->classes[ast->name].fields[ai].name, a.name);
        args.emplace_back(name, a.type, a.defaultValue);
        ctx->cache->classes[canonicalName].fields.push_back(Cache::Class::ClassField{
            name, nullptr, ctx->cache->classes[ast->name].fields[ai].baseClass});
        ai++;
      }
    }
  }
  if (typeAst) {
    if (!parentClasses.empty()) {
      mro.push_back(parentClasses);
      ctx->cache->classes[canonicalName].rtti = true;
    }
    ctx->cache->classes[canonicalName].mro = Cache::mergeC3(mro);
    if (ctx->cache->classes[canonicalName].mro.empty()) {
      E(Error::CLASS_BAD_MRO, getSrcInfo());
    } else if (ctx->cache->classes[canonicalName].mro.size() > 1) {
      // LOG("[mro] {} -> {}", canonicalName, ctx->cache->classes[canonicalName].mro);
    }
  }
  return asts;
}

/// Find the first __init__ with self parameter and use it to deduce class members.
/// Each deduced member will be treated as generic.
/// @example
///   ```@deduce
///      class Foo:
///        def __init__(self):
///          self.x, self.y = 1, 2```
///   will result in
///   ```class Foo[T1, T2]:
///        x: T1
///        y: T2```
/// @return the transformed init and the pointer to the original function.
std::pair<StmtPtr, FunctionStmt *>
SimplifyVisitor::autoDeduceMembers(ClassStmt *stmt, std::vector<Param> &args) {
  std::pair<StmtPtr, FunctionStmt *> init{nullptr, nullptr};
  for (const auto &sp : getClassMethods(stmt->suite))
    if (sp && sp->getFunction()) {
      auto f = sp->getFunction();
      if (f->name == "__init__" && !f->args.empty() && f->args[0].name == "self") {
        // Set up deducedMembers that will be populated during AssignStmt evaluation
        ctx->getBase()->deducedMembers = std::make_shared<std::vector<std::string>>();
        auto transformed = transform(sp);
        transformed->getFunction()->attributes.set(Attr::RealizeWithoutSelf);
        ctx->cache->functions[transformed->getFunction()->name].ast->attributes.set(
            Attr::RealizeWithoutSelf);
        int i = 0;
        // Once done, add arguments
        for (auto &m : *(ctx->getBase()->deducedMembers)) {
          auto varName = ctx->generateCanonicalName(format("T{}", ++i));
          auto memberName = ctx->cache->rev(varName);
          ctx->addType(memberName, varName, stmt->getSrcInfo())->generic = true;
          args.emplace_back(varName, N<IdExpr>("type"), nullptr, Param::Generic);
          args.emplace_back(m, N<IdExpr>(varName));
          ctx->cache->classes[stmt->name].fields.push_back(
              Cache::Class::ClassField{m, nullptr, stmt->name});
        }
        ctx->getBase()->deducedMembers = nullptr;
        return {transformed, f};
      }
    }
  return {nullptr, nullptr};
}

/// Return a list of all statements within a given class suite.
/// Checks each suite recursively, and assumes that each statement is either
/// a function, a class or a docstring.
std::vector<StmtPtr> SimplifyVisitor::getClassMethods(const StmtPtr &s) {
  std::vector<StmtPtr> v;
  if (!s)
    return v;
  if (auto sp = s->getSuite()) {
    for (const auto &ss : sp->stmts)
      for (const auto &u : getClassMethods(ss))
        v.push_back(u);
  } else if (s->getExpr() && s->getExpr()->expr->getString()) {
    /// Those are doc-strings, ignore them.
  } else if (!s->getFunction() && !s->getClass()) {
    E(Error::CLASS_BAD_ATTR, s);
  } else {
    v.push_back(s);
  }
  return v;
}

/// Extract nested classes and transform them before the main class.
void SimplifyVisitor::transformNestedClasses(ClassStmt *stmt,
                                             std::vector<StmtPtr> &clsStmts,
                                             std::vector<StmtPtr> &varStmts,
                                             std::vector<StmtPtr> &fnStmts) {
  for (const auto &sp : getClassMethods(stmt->suite))
    if (sp && sp->getClass()) {
      auto origName = sp->getClass()->name;
      // If class B is nested within A, it's name is always A.B, never B itself.
      // Ensure that parent class name is appended
      auto parentName = stmt->name;
      sp->getClass()->name = fmt::format("{}.{}", parentName, origName);
      auto tsp = transform(sp);
      std::string name;
      if (tsp->getSuite()) {
        for (auto &s : tsp->getSuite()->stmts)
          if (auto c = s->getClass()) {
            clsStmts.push_back(s);
            name = c->name;
          } else if (auto a = s->getAssign()) {
            varStmts.push_back(s);
          } else {
            fnStmts.push_back(s);
          }
        ctx->add(origName, ctx->forceFind(name));
      }
    }
}

/// Generate a magic method `__op__` for each magic `op`
/// described by @param typExpr and its arguments.
/// Currently generate:
/// @li Constructors: __new__, __init__
/// @li Utilities: __raw__, __hash__, __repr__, __tuplesize__, __add__, __mul__, __len__
/// @li Iteration: __iter__, __getitem__, __len__, __contains__
/// @li Comparisons: __eq__, __ne__, __lt__, __le__, __gt__, __ge__
/// @li Pickling: __pickle__, __unpickle__
/// @li Python: __to_py__, __from_py__
/// @li GPU: __to_gpu__, __from_gpu__, __from_gpu_new__
/// TODO: move to Codon as much as possible
StmtPtr SimplifyVisitor::codegenMagic(const std::string &op, const ExprPtr &typExpr,
                                      const std::vector<Param> &allArgs,
                                      bool isRecord) {
#define I(s) N<IdExpr>(s)
#define NS(x) N<DotExpr>(N<IdExpr>("__magic__"), (x))
  seqassert(typExpr, "typExpr is null");
  ExprPtr ret;
  std::vector<Param> fargs;
  std::vector<StmtPtr> stmts;
  Attr attr;
  attr.set("autogenerated");

  std::vector<Param> args;
  for (auto &a : allArgs)
    args.push_back(a);

  if (op == "new") {
    ret = typExpr->clone();
    if (isRecord) {
      // Tuples: def __new__() -> T (internal)
      for (auto &a : args)
        fargs.emplace_back(a.name, clone(a.type),
                           a.defaultValue ? clone(a.defaultValue)
                                          : N<CallExpr>(clone(a.type)));
      attr.set(Attr::Internal);
    } else {
      // Classes: def __new__() -> T
      stmts.emplace_back(N<ReturnStmt>(N<CallExpr>(NS(op), typExpr->clone())));
    }
  }
  // else if (startswith(op, "new.")) {
  //   // special handle for tuple[t1, t2, ...]
  //   int sz = atoi(op.substr(4).c_str());
  //   std::vector<ExprPtr> ts;
  //   for (int i = 0; i < sz; i++) {
  //     fargs.emplace_back(format("a{}", i + 1), I(format("T{}", i + 1)));
  //     ts.emplace_back(I(format("T{}", i + 1)));
  //   }
  //   for (int i = 0; i < sz; i++) {
  //     fargs.emplace_back(format("T{}", i + 1), I("type"));
  //   }
  //   ret = N<InstantiateExpr>(I(TYPE_TUPLE), ts);
  //   ret->markType();
  //   attr.set(Attr::Internal);
  // }
  else if (op == "init") {
    // Classes: def __init__(self: T, a1: T1, ..., aN: TN) -> None:
    //            self.aI = aI ...
    ret = I("NoneType");
    fargs.emplace_back("self", typExpr->clone());
    for (auto &a : args) {
      stmts.push_back(N<AssignStmt>(N<DotExpr>(I("self"), a.name), I(a.name)));
      fargs.emplace_back(a.name, clone(a.type),
                         a.defaultValue ? clone(a.defaultValue)
                                        : N<CallExpr>(clone(a.type)));
    }
  } else if (op == "raw") {
    // Classes: def __raw__(self: T)
    fargs.emplace_back("self", typExpr->clone());
    stmts.emplace_back(N<ReturnStmt>(N<CallExpr>(NS(op), I("self"))));
  } else if (op == "tuplesize") {
    // def __tuplesize__() -> int
    ret = I("int");
    stmts.emplace_back(N<ReturnStmt>(N<CallExpr>(NS(op))));
  } else if (op == "getitem") {
    // Tuples: def __getitem__(self: T, index: int)
    fargs.emplace_back("self", typExpr->clone());
    fargs.emplace_back("index", I("int"));
    stmts.emplace_back(N<ReturnStmt>(N<CallExpr>(NS(op), I("self"), I("index"))));
  } else if (op == "iter") {
    // Tuples: def __iter__(self: T)
    fargs.emplace_back("self", typExpr->clone());
    stmts.emplace_back(N<YieldFromStmt>(N<CallExpr>(NS(op), I("self"))));
  } else if (op == "contains") {
    // Tuples: def __contains__(self: T, what) -> bool
    fargs.emplace_back("self", typExpr->clone());
    fargs.emplace_back("what", nullptr);
    ret = I("bool");
    stmts.emplace_back(N<ReturnStmt>(N<CallExpr>(NS(op), I("self"), I("what"))));
  } else if (op == "eq" || op == "ne" || op == "lt" || op == "le" || op == "gt" ||
             op == "ge") {
    // def __op__(self: T, obj: T) -> bool
    fargs.emplace_back("self", typExpr->clone());
    fargs.emplace_back("obj", typExpr->clone());
    ret = I("bool");
    stmts.emplace_back(N<ReturnStmt>(N<CallExpr>(NS(op), I("self"), I("obj"))));
  } else if (op == "hash") {
    // def __hash__(self: T) -> int
    fargs.emplace_back("self", typExpr->clone());
    ret = I("int");
    stmts.emplace_back(N<ReturnStmt>(N<CallExpr>(NS(op), I("self"))));
  } else if (op == "pickle") {
    // def __pickle__(self: T, dest: Ptr[byte])
    fargs.emplace_back("self", typExpr->clone());
    fargs.emplace_back("dest", N<IndexExpr>(I("Ptr"), I("byte")));
    stmts.emplace_back(N<ReturnStmt>(N<CallExpr>(NS(op), I("self"), I("dest"))));
  } else if (op == "unpickle") {
    // def __unpickle__(src: Ptr[byte]) -> T
    fargs.emplace_back("src", N<IndexExpr>(I("Ptr"), I("byte")));
    ret = typExpr->clone();
    stmts.emplace_back(N<ReturnStmt>(N<CallExpr>(NS(op), I("src"), typExpr->clone())));
  } else if (op == "len") {
    // def __len__(self: T) -> int
    fargs.emplace_back("self", typExpr->clone());
    ret = I("int");
    stmts.emplace_back(N<ReturnStmt>(N<CallExpr>(NS(op), I("self"))));
  } else if (op == "to_py") {
    // def __to_py__(self: T) -> Ptr[byte]
    fargs.emplace_back("self", typExpr->clone());
    ret = N<IndexExpr>(I("Ptr"), I("byte"));
    stmts.emplace_back(N<ReturnStmt>(N<CallExpr>(NS(op), I("self"))));
  } else if (op == "from_py") {
    // def __from_py__(src: Ptr[byte]) -> T
    fargs.emplace_back("src", N<IndexExpr>(I("Ptr"), I("byte")));
    ret = typExpr->clone();
    stmts.emplace_back(N<ReturnStmt>(N<CallExpr>(NS(op), I("src"), typExpr->clone())));
  } else if (op == "to_gpu") {
    // def __to_gpu__(self: T, cache) -> T
    fargs.emplace_back("self", typExpr->clone());
    fargs.emplace_back("cache");
    ret = typExpr->clone();
    stmts.emplace_back(N<ReturnStmt>(N<CallExpr>(NS(op), I("self"), I("cache"))));
  } else if (op == "from_gpu") {
    // def __from_gpu__(self: T, other: T)
    fargs.emplace_back("self", typExpr->clone());
    fargs.emplace_back("other", typExpr->clone());
    stmts.emplace_back(N<ReturnStmt>(N<CallExpr>(NS(op), I("self"), I("other"))));
  } else if (op == "from_gpu_new") {
    // def __from_gpu_new__(other: T) -> T
    fargs.emplace_back("other", typExpr->clone());
    ret = typExpr->clone();
    stmts.emplace_back(N<ReturnStmt>(N<CallExpr>(NS(op), I("other"))));
  } else if (op == "repr") {
    // def __repr__(self: T) -> str
    fargs.emplace_back("self", typExpr->clone());
    ret = I("str");
    stmts.emplace_back(N<ReturnStmt>(N<CallExpr>(NS(op), I("self"))));
  } else if (op == "dict") {
    // def __dict__(self: T)
    fargs.emplace_back("self", typExpr->clone());
    stmts.emplace_back(N<ReturnStmt>(N<CallExpr>(NS(op), I("self"))));
  } else if (op == "add") {
    // def __add__(self, obj)
    fargs.emplace_back("self", typExpr->clone());
    fargs.emplace_back("obj", nullptr);
    stmts.emplace_back(N<ReturnStmt>(N<CallExpr>(NS(op), I("self"), I("obj"))));
  } else if (op == "mul") {
    // def __mul__(self, i: Static[int])
    fargs.emplace_back("self", typExpr->clone());
    fargs.emplace_back("i", N<IndexExpr>(I("Static"), I("int")));
    stmts.emplace_back(N<ReturnStmt>(N<CallExpr>(NS(op), I("self"), I("i"))));
  } else {
    seqassert(false, "invalid magic {}", op);
  }
#undef I
#undef NS
  auto t = std::make_shared<FunctionStmt>(format("__{}__", op), ret, fargs,
                                          N<SuiteStmt>(stmts), attr);
  t->setSrcInfo(ctx->cache->generateSrcInfo());
  return t;
}

} // namespace codon::ast
