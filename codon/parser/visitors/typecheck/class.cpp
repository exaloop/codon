// Copyright (C) 2022-2025 Exaloop Inc. <https://exaloop.io>

#include <string>
#include <tuple>

#include "codon/cir/attribute.h"
#include "codon/parser/ast.h"
#include "codon/parser/cache.h"
#include "codon/parser/common.h"
#include "codon/parser/match.h"
#include "codon/parser/visitors/format/format.h"
#include "codon/parser/visitors/scoping/scoping.h"
#include "codon/parser/visitors/typecheck/typecheck.h"
using namespace codon::error;

namespace codon::ast {

using namespace types;
using namespace matcher;

/// Parse a class (type) declaration and add a (generic) type to the context.
void TypecheckVisitor::visit(ClassStmt *stmt) {
  // Get root name
  std::string name = stmt->getName();

  // Generate/find class' canonical name (unique ID) and AST
  std::string canonicalName;
  std::vector<Param> &argsToParse = stmt->items;

  // classItem will be added later when the scope is different
  auto classItem = std::make_shared<TypecheckItem>("", "", ctx->getModule(), nullptr,
                                                   ctx->getScope());
  classItem->setSrcInfo(stmt->getSrcInfo());
  std::shared_ptr<TypecheckItem> timedItem = nullptr;
  types::ClassType *typ = nullptr;
  if (!stmt->hasAttribute(Attr::Extend)) {
    classItem->canonicalName = canonicalName =
        ctx->generateCanonicalName(name, !stmt->hasAttribute(Attr::Internal),
                                   /* noSuffix*/ stmt->hasAttribute(Attr::Internal));

    if (canonicalName == "Union")
      classItem->type = std::make_shared<types::UnionType>(ctx->cache);
    else
      classItem->type =
          std::make_shared<types::ClassType>(ctx->cache, canonicalName, name);
    if (stmt->isRecord())
      classItem->type->getClass()->isTuple = true;
    classItem->type->setSrcInfo(stmt->getSrcInfo());

    typ = classItem->getType()->getClass();
    if (canonicalName != TYPE_TYPE)
      classItem->type = instantiateTypeVar(classItem->getType());

    timedItem = std::make_shared<TypecheckItem>(*classItem);
    // timedItem->time = getTime();

    // Reference types are added to the context here.
    // Tuple types are added after class contents are parsed to prevent
    // recursive record types (note: these are allowed for reference types)
    if (!stmt->hasAttribute(Attr::Tuple)) {
      ctx->add(name, timedItem);
      ctx->addAlwaysVisible(classItem);
    }
  } else {
    // Find the canonical name and AST of the class that is to be extended
    if (!ctx->isGlobal() || ctx->isConditional())
      E(Error::EXPECTED_TOPLEVEL, getSrcInfo(), "class extension");
    auto val = ctx->find(name, getTime());
    if (!val || !val->isType())
      E(Error::CLASS_ID_NOT_FOUND, getSrcInfo(), name);
    typ = val->getName() == TYPE_TYPE ? val->getType()->getClass()
                                      : extractClassType(val->getType());
    canonicalName = typ->name;
    argsToParse = getClass(typ)->ast->items;
  }
  auto &cls = ctx->cache->classes[canonicalName];

  std::vector<Stmt *> clsStmts; // Will be filled later!
  std::vector<Stmt *> varStmts; // Will be filled later!
  std::vector<Stmt *> fnStmts;  // Will be filled later!
  std::vector<TypeContext::Item> addLater;
  try {
    // Add the class base
    TypeContext::BaseGuard br(ctx.get(), canonicalName);
    ctx->getBase()->type = typ->shared_from_this();

    // Parse and add class generics
    std::vector<Param> args;
    if (stmt->hasAttribute(Attr::Extend)) {
      for (auto &a : argsToParse) {
        if (!a.isGeneric())
          continue;
        auto val = ctx->forceFind(a.name);
        val->type->getLink()->kind = LinkType::Unbound;
        ctx->add(getUnmangledName(val->canonicalName), val);
        args.emplace_back(val->canonicalName, nullptr, nullptr, a.status);
      }
    } else {
      if (stmt->hasAttribute(Attr::ClassDeduce)) {
        if (!autoDeduceMembers(stmt, argsToParse))
          stmt->eraseAttribute(Attr::ClassDeduce);
      }

      // Add all generics before parent classes, fields and methods
      for (auto &a : argsToParse) {
        if (!a.isGeneric())
          continue;

        auto varName = ctx->generateCanonicalName(a.getName()), genName = a.getName();
        auto generic = instantiateUnbound();
        auto typId = generic->id;
        generic->getLink()->genericName = genName;
        auto defType = transformType(clone(a.getDefault()));
        if (defType)
          generic->defaultType = extractType(defType)->shared_from_this();
        if (auto st = getStaticGeneric(a.getType())) {
          if (st > 3)
            a.type = transform(a.getType()); // error check
          generic->isStatic = st;
          auto val = ctx->addVar(genName, varName, generic);
          val->generic = true;
        } else {
          if (cast<IndexExpr>(a.getType())) { // Parse TraitVar
            a.type = transform(a.getType());
            auto ti = cast<InstantiateExpr>(a.getType());
            seqassert(ti && isId(ti->getExpr(), TRAIT_TYPE),
                      "not a TypeTrait instantiation: {}", *(a.getType()));
            auto l = extractType(ti->front());
            if (l->getLink() && l->getLink()->trait)
              generic->getLink()->trait = l->getLink()->trait;
            else
              generic->getLink()->trait =
                  std::make_shared<types::TypeTrait>(l->shared_from_this());
          }
          ctx->addType(genName, varName, generic)->generic = true;
        }
        typ->generics.emplace_back(varName, genName,
                                   generic->generalize(ctx->typecheckLevel), typId,
                                   generic->isStatic);
        args.emplace_back(varName, a.getType(), defType, a.status);
      }
    }

    // Form class type node (e.g. `Foo`, or `Foo[T, U]` for generic classes)
    Expr *transformedTypeAst = nullptr;
    if (!stmt->hasAttribute(Attr::Extend)) {
      transformedTypeAst = N<IdExpr>(canonicalName);
      for (auto &a : args) {
        if (a.isGeneric()) {
          if (!cast<InstantiateExpr>(transformedTypeAst)) {
            transformedTypeAst =
                N<InstantiateExpr>(N<IdExpr>(canonicalName), std::vector<Expr *>{});
          }
          cast<InstantiateExpr>(transformedTypeAst)
              ->items.push_back(transform(N<IdExpr>(a.getName()), true));
        }
      }
    }

    // Collect classes (and their fields) that are to be statically inherited
    std::vector<TypePtr> staticBaseASTs;
    if (!stmt->hasAttribute(Attr::Extend)) {
      staticBaseASTs = parseBaseClasses(stmt->staticBaseClasses, args, stmt,
                                        canonicalName, nullptr, typ);
      if (ctx->cache->isJit && !stmt->baseClasses.empty())
        E(Error::CUSTOM, stmt->baseClasses[0],
          "inheritance is not yet supported in JIT mode");
      parseBaseClasses(stmt->baseClasses, args, stmt, canonicalName, transformedTypeAst,
                       typ);
    }

    // A ClassStmt will be separated into class variable assignments, method-free
    // ClassStmts (that include nested classes) and method FunctionStmts
    transformNestedClasses(stmt, clsStmts, varStmts, fnStmts);

    // Collect class fields
    for (auto &a : argsToParse) {
      if (a.isValue()) {
        if (ClassStmt::isClassVar(a)) {
          // Handle class variables. Transform them later to allow self-references
          auto name = fmt::format("{}.{}", canonicalName, a.getName());
          auto h = transform(N<AssignStmt>(N<IdExpr>(name), nullptr, nullptr));
          preamble->addStmt(h);
          auto val = ctx->forceFind(name);
          val->baseName = "";
          val->scope = {0};
          registerGlobal(val->canonicalName);
          auto assign = N<AssignStmt>(
              N<IdExpr>(name), a.getDefault(),
              a.getType() ? cast<IndexExpr>(a.getType())->getIndex() : nullptr);
          assign->setUpdate();
          varStmts.push_back(assign);
          cls.classVars[a.getName()] = name;
        } else if (!stmt->hasAttribute(Attr::Extend)) {
          std::string varName = a.getName();
          args.emplace_back(varName, transformType(clean_clone(a.getType()), true),
                            transform(clone(a.getDefault()), true));
          cls.fields.emplace_back(varName, nullptr, canonicalName);
        }
      }
    }

    // ASTs for member arguments to be used for populating magic methods
    std::vector<Param> memberArgs;
    for (auto &a : args) {
      if (a.isValue())
        memberArgs.emplace_back(clone(a));
    }

    // Handle class members
    if (!stmt->hasAttribute(Attr::Extend)) {
      ctx->typecheckLevel++; // to avoid unifying generics early
      if (canonicalName == TYPE_TUPLE) {
        // Special tuple handling!
        for (auto aj = 0; aj < MAX_TUPLE; aj++) {
          auto genName = fmt::format("T{}", aj + 1);
          auto genCanName = ctx->generateCanonicalName(genName);
          auto generic = instantiateUnbound();
          generic->getLink()->genericName = genName;
          Expr *te = N<IdExpr>(genCanName);
          cls.fields.emplace_back(fmt::format("item{}", aj + 1),
                                  generic->generalize(ctx->typecheckLevel), "", te);
        }
      } else {
        for (auto ai = 0, aj = 0; ai < args.size(); ai++) {
          if (args[ai].isValue() && !ClassStmt::isClassVar(args[ai])) {
            cls.fields[aj].typeExpr = clean_clone(args[ai].getType());
            cls.fields[aj].type =
                extractType(args[ai].getType())->generalize(ctx->typecheckLevel - 1);
            cls.fields[aj].type->setSrcInfo(args[ai].getType()->getSrcInfo());
            aj++;
          }
        }
      }
      ctx->typecheckLevel--;
    }

    // Parse class members (arguments) and methods
    if (!stmt->hasAttribute(Attr::Extend)) {
      // Now that we are done with arguments, add record type to the context
      if (stmt->hasAttribute(Attr::Tuple)) {
        ctx->add(name, timedItem);
        ctx->addAlwaysVisible(classItem);
      }
      // Create a cached AST.
      stmt->setAttribute(Attr::Module, ctx->moduleName.status == ImportFile::STDLIB
                                           ? STDLIB_IMPORT
                                           : ctx->moduleName.path);
      cls.ast = N<ClassStmt>(canonicalName, args, N<SuiteStmt>());
      cls.ast->cloneAttributesFrom(stmt);
      cls.ast->baseClasses = stmt->baseClasses;
      for (auto &b : staticBaseASTs)
        cls.staticParentClasses.emplace_back(b->getClass()->name);
      cls.module = ctx->moduleName.path;

      // Codegen default magic methods
      // __new__ must be the first
      if (auto aa = stmt->getAttribute<ir::StringListAttribute>(Attr::ClassMagic))
        for (auto &m : aa->values) {
          fnStmts.push_back(transform(codegenMagic(m, transformedTypeAst, memberArgs,
                                                   stmt->hasAttribute(Attr::Tuple))));
        }
      // Add inherited methods
      for (auto &base : staticBaseASTs) {
        for (auto &mm : getClass(base->getClass())->methods)
          for (auto &mf : getOverloads(mm.second)) {
            const auto &fp = getFunction(mf);
            auto f = fp->origAst;
            if (f && !f->hasAttribute(Attr::AutoGenerated)) {
              fnStmts.push_back(
                  cast<FunctionStmt>(withClassGenerics(base->getClass(), [&]() {
                    auto cf = clean_clone(f);
                    // since functions can come from other modules
                    // make sure to transform them in their respective module
                    // however makle sure to add/pop generics :/
                    if (!ctx->isStdlibLoading && fp->module != ctx->moduleName.path) {
                      auto ictx = getImport(fp->module)->ctx;
                      TypeContext::BaseGuard br(ictx.get(), canonicalName);
                      ictx->getBase()->type = typ->shared_from_this();
                      auto tv = TypecheckVisitor(ictx);
                      auto e = tv.withClassGenerics(
                          typ, [&]() { return tv.transform(clean_clone(f)); }, false,
                          false,
                          /*instantiate*/ true);
                      return e;
                    } else {
                      return transform(clean_clone(f));
                    }
                  })));
            }
          }
      }
    }

    // Add class methods
    for (const auto &sp : getClassMethods(stmt->getSuite()))
      if (auto fp = cast<FunctionStmt>(sp)) {
        for (auto *&dc : fp->decorators) {
          // Handle @setter setters
          if (match(dc, M<DotExpr>(M<IdExpr>(fp->getName()), "setter")) &&
              fp->size() == 2) {
            fp->name = fmt::format("{}{}", FN_SETTER_SUFFIX, fp->getName());
            dc = nullptr;
            break;
          }
        }
        fnStmts.emplace_back(transform(sp));
      }

    // After popping context block, record types and nested classes will disappear.
    // Store their references and re-add them to the context after popping
    addLater.reserve(clsStmts.size() + 1);
    for (auto &c : clsStmts)
      addLater.emplace_back(ctx->find(cast<ClassStmt>(c)->getName()));
    if (stmt->hasAttribute(Attr::Tuple))
      addLater.emplace_back(ctx->forceFind(name));

    // Mark functions as virtual:
    auto banned =
        std::set<std::string>{"__init__", "__new__", "__raw__", "__tuplesize__"};
    for (auto &m : cls.methods) {
      auto method = m.first;
      for (size_t mi = 1; mi < cls.mro.size(); mi++) {
        // ... in the current class
        auto b = cls.mro[mi]->name;
        if (in(getClass(b)->methods, method) && !in(banned, method)) {
          cls.virtuals.insert(method);
        }
      }
      for (auto &v : cls.virtuals) {
        for (size_t mi = 1; mi < cls.mro.size(); mi++) {
          // ... and in parent classes
          auto b = cls.mro[mi]->name;
          getClass(b)->virtuals.insert(v);
        }
      }
    }

    // Generalize generics and remove them from the context
    for (const auto &g : args)
      if (!g.isValue()) {
        auto generic = ctx->forceFind(g.name)->type;
        if (g.status == Param::Generic) {
          // Generalize generics. Hidden generics are linked to the class generics so
          // ignore them
          seqassert(generic && generic->getLink() &&
                        generic->getLink()->kind != types::LinkType::Link,
                    "generic has been unified");
          generic->getLink()->kind = LinkType::Generic;
        }
        ctx->remove(g.name);
      }

    // Debug information
    LOG_REALIZE("[class] {} -> {:c} / {}", canonicalName, *typ, cls.fields.size());
    for (auto &m : cls.fields)
      LOG_REALIZE("       - member: {}: {:c}", m.name, *(m.type));
    for (auto &m : cls.methods)
      LOG_REALIZE("       - method: {}: {}", m.first, m.second);
    for (auto &m : cls.mro)
      LOG_REALIZE("       - mro: {:c}", *m);
  } catch (const exc::ParserException &) {
    if (!stmt->hasAttribute(Attr::Tuple))
      ctx->remove(name);
    ctx->cache->classes.erase(name);
    throw;
  }
  for (auto &i : addLater)
    ctx->add(getUnmangledName(i->canonicalName), i);

  // Extensions are not needed as the cache is already populated
  if (!stmt->hasAttribute(Attr::Extend)) {
    auto c = cls.ast;
    seqassert(c, "not a class AST for {}", canonicalName);
    c->setDone();
    clsStmts.push_back(c);
  }

  clsStmts.insert(clsStmts.end(), fnStmts.begin(), fnStmts.end());
  for (auto &a : varStmts) {
    // Transform class variables here to allow self-references
    clsStmts.push_back(transform(a));
  }
  resultStmt = N<SuiteStmt>(clsStmts);
}

/// Parse statically inherited classes.
/// Returns a list of their ASTs. Also updates the class fields.
/// @param args Class fields that are to be updated with base classes' fields.
/// @param typeAst Transformed AST for base class type (e.g., `A[T]`).
///                Only set when dealing with dynamic polymorphism.
std::vector<TypePtr> TypecheckVisitor::parseBaseClasses(
    std::vector<Expr *> &baseClasses, std::vector<Param> &args, Stmt *attr,
    const std::string &canonicalName, Expr *typeAst, types::ClassType *typ) {
  std::vector<TypePtr> asts;

  // TODO)) fix MRO it to work with generic classes (maybe replacements? IDK...)
  std::vector<std::vector<TypePtr>> mro{{typ->shared_from_this()}};
  for (auto &cls : baseClasses) {
    std::vector<Expr *> subs;

    // Get the base class and generic replacements (e.g., if there is Bar[T],
    // Bar in Foo(Bar[int]) will have `T = int`)
    cls = transformType(cls, true);
    if (!cls->getClassType())
      E(Error::CLASS_ID_NOT_FOUND, getSrcInfo(), FormatVisitor::apply(cls));

    auto clsTyp = extractClassType(cls);
    asts.push_back(clsTyp->shared_from_this());
    auto cachedCls = getClass(clsTyp);
    if (!cachedCls->ast)
      E(Error::CLASS_NO_INHERIT, getSrcInfo(), "nested", "surrounding");
    std::vector<TypePtr> rootMro;
    for (auto &t : cachedCls->mro)
      rootMro.push_back(instantiateType(t.get(), clsTyp));
    mro.push_back(rootMro);

    // Sanity checks
    if (attr->hasAttribute(Attr::Tuple) && typeAst)
      E(Error::CLASS_NO_INHERIT, getSrcInfo(), "tuple", "other");
    if (!attr->hasAttribute(Attr::Tuple) && cachedCls->ast->hasAttribute(Attr::Tuple))
      E(Error::CLASS_TUPLE_INHERIT, getSrcInfo());
    if (cachedCls->ast->hasAttribute(Attr::Internal))
      E(Error::CLASS_NO_INHERIT, getSrcInfo(), "internal", "other");

    // Mark parent classes as polymorphic as well.
    if (typeAst)
      cachedCls->rtti = true;

    // Add hidden generics
    addClassGenerics(clsTyp);
    for (auto &g : clsTyp->generics)
      typ->hiddenGenerics.push_back(g);
    for (auto &g : clsTyp->hiddenGenerics)
      typ->hiddenGenerics.push_back(g);
  }
  // Add normal fields
  auto cls = getClass(canonicalName);
  for (auto &clsTyp : asts) {
    withClassGenerics(clsTyp->getClass(), [&]() {
      int ai = 0;
      auto ast = getClass(clsTyp->getClass())->ast;
      for (auto &a : *ast) {
        auto acls = getClass(ast->name);
        if (a.isValue() && !ClassStmt::isClassVar(a)) {
          auto name = a.getName();
          int i = 0;
          for (auto &aa : args)
            i += aa.getName() == a.getName() ||
                 startswith(aa.getName(), a.getName() + "#");
          if (i)
            name = fmt::format("{}#{}", name, i);
          seqassert(acls->fields[ai].name == a.getName(), "bad class fields: {} vs {}",
                    acls->fields[ai].name, a.getName());
          args.emplace_back(name, transformType(clean_clone(a.getType()), true),
                            transform(clean_clone(a.getDefault())));
          cls->fields.emplace_back(
              name, extractType(args.back().getType())->shared_from_this(),
              acls->fields[ai].baseClass);
          ai++;
        }
      }
      return true;
    });
  }
  if (typeAst) {
    if (!asts.empty()) {
      mro.push_back(asts);
      cls->rtti = true;
    }
    cls->mro = Cache::mergeC3(mro);
    if (cls->mro.empty()) {
      E(Error::CLASS_BAD_MRO, getSrcInfo());
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
bool TypecheckVisitor::autoDeduceMembers(ClassStmt *stmt, std::vector<Param> &args) {
  std::set<std::string> members;
  for (const auto &sp : getClassMethods(stmt->suite))
    if (auto f = cast<FunctionStmt>(sp)) {
      if (f->name == "__init__")
        if (auto b = f->getAttribute<ir::StringListAttribute>(Attr::ClassDeduce)) {
          for (auto m : b->values)
            members.insert(m);
        }
    }
  if (!members.empty()) {
    // log("auto-deducing {}: {}", stmt->name, members);
    if (auto aa = stmt->getAttribute<ir::StringListAttribute>(Attr::ClassMagic))
      aa->values.erase(std::remove(aa->values.begin(), aa->values.end(), "init"),
                       aa->values.end());
    for (auto m : members) {
      auto genericName = fmt::format("T_{}", m);
      args.emplace_back(genericName, N<IdExpr>(TYPE_TYPE), N<IdExpr>("NoneType"),
                        Param::Generic);
      args.emplace_back(m, N<IdExpr>(genericName));
    }
    return true;
  }
  return false;
}

/// Return a list of all statements within a given class suite.
/// Checks each suite recursively, and assumes that each statement is either
/// a function, a class or a docstring.
std::vector<Stmt *> TypecheckVisitor::getClassMethods(Stmt *s) {
  std::vector<Stmt *> v;
  if (!s)
    return v;
  if (auto sp = cast<SuiteStmt>(s)) {
    for (auto *ss : *sp)
      for (auto *u : getClassMethods(ss))
        v.push_back(u);
  } else if (cast<FunctionStmt>(s) || cast<ClassStmt>(s)) {
    v.push_back(s);
  } else if (!match(s, M<ExprStmt>(M<StringExpr>()))) {
    E(Error::CLASS_BAD_ATTR, s);
  }
  return v;
}

/// Extract nested classes and transform them before the main class.
void TypecheckVisitor::transformNestedClasses(ClassStmt *stmt,
                                              std::vector<Stmt *> &clsStmts,
                                              std::vector<Stmt *> &varStmts,
                                              std::vector<Stmt *> &fnStmts) {
  for (const auto &sp : getClassMethods(stmt->suite))
    if (auto cp = cast<ClassStmt>(sp)) {
      auto origName = cp->getName();
      // If class B is nested within A, it's name is always A.B, never B itself.
      // Ensure that parent class name is appended
      auto parentName = stmt->getName();
      cp->name = fmt::format("{}.{}", parentName, origName);
      auto tsp = transform(cp);
      std::string name;
      if (auto tss = cast<SuiteStmt>(tsp)) {
        for (auto &s : *tss)
          if (auto c = cast<ClassStmt>(s)) {
            clsStmts.push_back(s);
            name = c->getName();
          } else if (auto a = cast<AssignStmt>(s)) {
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
Stmt *TypecheckVisitor::codegenMagic(const std::string &op, Expr *typExpr,
                                     const std::vector<Param> &allArgs, bool isRecord) {
#define I(s) N<IdExpr>(s)
#define NS(x) N<DotExpr>(N<IdExpr>("__magic__"), (x))
  seqassert(typExpr, "typExpr is null");
  Expr *ret = nullptr;
  std::vector<Param> fargs;
  std::vector<Stmt *> stmts;
  std::vector<int> attrs{Attr::AutoGenerated};

  std::vector<Param> args;
  args.reserve(allArgs.size());
  for (auto &a : allArgs)
    args.push_back(clone(a));

  if (op == "new") {
    ret = clone(typExpr);
    if (isRecord) {
      // Tuples: def __new__() -> T (internal)
      for (auto &a : args)
        fargs.emplace_back(a.getName(), clone(a.getType()), clone(a.getDefault()));
      attrs.push_back(Attr::Internal);
    } else {
      // Classes: def __new__() -> T
      stmts.emplace_back(N<ReturnStmt>(N<CallExpr>(NS(op), clone(typExpr))));
    }
  } else if (op == "init") {
    // Classes: def __init__(self: T, a1: T1, ..., aN: TN) -> None:
    //            self.aI = aI ...
    ret = I("NoneType");
    fargs.emplace_back("self", clone(typExpr));
    for (auto &a : args) {
      fargs.emplace_back(a.getName(), clean_clone(a.getType()), clone(a.getDefault()));
      stmts.push_back(
          N<AssignStmt>(N<DotExpr>(I("self"), a.getName()), I(a.getName())));
    }
  } else if (op == "raw" || op == "dict") {
    // Classes: def __raw__(self: T)
    fargs.emplace_back("self", clone(typExpr));
    stmts.emplace_back(N<ReturnStmt>(N<CallExpr>(NS(op), I("self"))));
  } else if (op == "tuplesize") {
    // def __tuplesize__() -> int
    ret = I("int");
    stmts.emplace_back(N<ReturnStmt>(N<CallExpr>(NS(op))));
  } else if (op == "getitem") {
    // Tuples: def __getitem__(self: T, index: int)
    fargs.emplace_back("self", clone(typExpr));
    fargs.emplace_back("index", I("int"));
    stmts.emplace_back(N<ReturnStmt>(N<CallExpr>(NS(op), I("self"), I("index"))));
  } else if (op == "iter") {
    // Tuples: def __iter__(self: T)
    fargs.emplace_back("self", clone(typExpr));
    stmts.emplace_back(N<YieldFromStmt>(N<CallExpr>(NS(op), I("self"))));
  } else if (op == "contains") {
    // Tuples: def __contains__(self: T, what) -> bool
    fargs.emplace_back("self", clone(typExpr));
    fargs.emplace_back("what", nullptr);
    ret = I("bool");
    stmts.emplace_back(N<ReturnStmt>(N<CallExpr>(NS(op), I("self"), I("what"))));
  } else if (op == "eq" || op == "ne" || op == "lt" || op == "le" || op == "gt" ||
             op == "ge") {
    // def __op__(self: T, obj: T) -> bool
    fargs.emplace_back("self", clone(typExpr));
    fargs.emplace_back("obj", clone(typExpr));
    ret = I("bool");
    stmts.emplace_back(N<ReturnStmt>(N<CallExpr>(NS(op), I("self"), I("obj"))));
  } else if (op == "hash" || op == "len") {
    // def __hash__(self: T) -> int
    fargs.emplace_back("self", clone(typExpr));
    ret = I("int");
    stmts.emplace_back(N<ReturnStmt>(N<CallExpr>(NS(op), I("self"))));
  } else if (op == "pickle") {
    // def __pickle__(self: T, dest: Ptr[byte])
    fargs.emplace_back("self", clone(typExpr));
    fargs.emplace_back("dest", N<IndexExpr>(I("Ptr"), I("byte")));
    stmts.emplace_back(N<ReturnStmt>(N<CallExpr>(NS(op), I("self"), I("dest"))));
  } else if (op == "unpickle" || op == "from_py") {
    // def __unpickle__(src: Ptr[byte]) -> T
    fargs.emplace_back("src", N<IndexExpr>(I("Ptr"), I("byte")));
    ret = clone(typExpr);
    stmts.emplace_back(N<ReturnStmt>(N<CallExpr>(NS(op), I("src"), clone(typExpr))));
  } else if (op == "to_py") {
    // def __to_py__(self: T) -> Ptr[byte]
    fargs.emplace_back("self", clone(typExpr));
    ret = N<IndexExpr>(I("Ptr"), I("byte"));
    stmts.emplace_back(N<ReturnStmt>(N<CallExpr>(NS(op), I("self"))));
  } else if (op == "to_gpu") {
    // def __to_gpu__(self: T, cache) -> T
    fargs.emplace_back("self", clone(typExpr));
    fargs.emplace_back("cache");
    ret = clone(typExpr);
    stmts.emplace_back(N<ReturnStmt>(N<CallExpr>(NS(op), I("self"), I("cache"))));
  } else if (op == "from_gpu") {
    // def __from_gpu__(self: T, other: T)
    fargs.emplace_back("self", clone(typExpr));
    fargs.emplace_back("other", clone(typExpr));
    stmts.emplace_back(N<ReturnStmt>(N<CallExpr>(NS(op), I("self"), I("other"))));
  } else if (op == "from_gpu_new") {
    // def __from_gpu_new__(other: T) -> T
    fargs.emplace_back("other", clone(typExpr));
    ret = clone(typExpr);
    stmts.emplace_back(N<ReturnStmt>(N<CallExpr>(NS(op), I("other"))));
  } else if (op == "repr") {
    // def __repr__(self: T) -> str
    fargs.emplace_back("self", clone(typExpr));
    ret = I("str");
    stmts.emplace_back(N<ReturnStmt>(N<CallExpr>(NS(op), I("self"))));
  } else if (op == "add") {
    // def __add__(self, obj)
    fargs.emplace_back("self", clone(typExpr));
    fargs.emplace_back("obj", nullptr);
    stmts.emplace_back(N<ReturnStmt>(N<CallExpr>(NS(op), I("self"), I("obj"))));
  } else if (op == "mul") {
    // def __mul__(self, i: Literal[int])
    fargs.emplace_back("self", clone(typExpr));
    fargs.emplace_back("i", N<IndexExpr>(I("Literal"), I("int")));
    stmts.emplace_back(N<ReturnStmt>(N<CallExpr>(NS(op), I("self"), I("i"))));
  } else {
    seqassert(false, "invalid magic {}", op);
  }
#undef I
#undef NS
  auto t =
      NC<FunctionStmt>(fmt::format("__{}__", op), ret, fargs, NC<SuiteStmt>(stmts));
  for (auto &a : attrs)
    t->setAttribute(a);
  t->setSrcInfo(ctx->cache->generateSrcInfo());
  return t;
}

int TypecheckVisitor::generateKwId(const std::vector<std::string> &names) {
  auto key = join(names, ";");
  std::string suffix;
  if (!names.empty()) {
    // Each set of names generates different tuple (i.e., `KwArgs[foo, bar]` is not the
    // same as `KwArgs[bar, baz]`). Cache the names and use an integer for each name
    // combination.
    if (!in(ctx->cache->generatedTuples, key)) {
      ctx->cache->generatedTupleNames.push_back(names);
      ctx->cache->generatedTuples[key] = int(ctx->cache->generatedTuples.size()) + 1;
    }
    return ctx->cache->generatedTuples[key];
  } else {
    return 0;
  }
}

types::ClassType *TypecheckVisitor::generateTuple(size_t n, bool generateNew) {
  static std::unordered_set<size_t> funcArgTypes;

  if (n > MAX_TUPLE)
    E(Error::CUSTOM, getSrcInfo(), "tuple too large ({})", n);

  auto key = fmt::format("{}.{}", TYPE_TUPLE, n);
  auto val = getImport(STDLIB_IMPORT)->ctx->find(key);
  if (!val) {
    auto t = std::make_shared<types::ClassType>(ctx->cache, TYPE_TUPLE, TYPE_TUPLE);
    t->isTuple = true;
    auto cls = getClass(t.get());
    seqassert(n <= cls->fields.size(), "tuple too large");
    for (size_t i = 0; i < n; i++) {
      const auto &f = cls->fields[i];
      auto gt = f.getType()->getLink();
      t->generics.emplace_back(cast<IdExpr>(f.typeExpr)->getValue(), gt->genericName,
                               f.type, gt->id, 0);
    }
    val = getImport(STDLIB_IMPORT)->ctx->addType(key, key, t);
  }
  auto t = val->getType()->getClass();
  if (generateNew && !in(funcArgTypes, n)) {
    funcArgTypes.insert(n);
    std::vector<Param> newFnArgs;
    std::vector<Expr *> typeArgs;
    for (size_t i = 0; i < n; i++) {
      newFnArgs.emplace_back(fmt::format("item{}", i + 1),
                             N<IdExpr>(fmt::format("T{}", i + 1)));
      typeArgs.emplace_back(N<IdExpr>(fmt::format("T{}", i + 1)));
    }
    for (size_t i = 0; i < n; i++) {
      newFnArgs.emplace_back(fmt::format("T{}", i + 1), N<IdExpr>(TYPE_TYPE));
    }
    Stmt *fn = N<FunctionStmt>(
        "__new__", N<IndexExpr>(N<IdExpr>(TYPE_TUPLE), N<TupleExpr>(typeArgs)),
        newFnArgs, nullptr);
    fn->setAttribute(Attr::Internal);
    Stmt *ext = N<ClassStmt>(TYPE_TUPLE, std::vector<Param>{}, fn);
    ext->setAttribute(Attr::Extend);
    ext = N<SuiteStmt>(ext);

    llvm::cantFail(ScopingVisitor::apply(ctx->cache, ext));
    auto rctx = getImport(STDLIB_IMPORT)->ctx;
    auto oldBases = rctx->bases;
    rctx->bases.clear();
    rctx->bases.push_back(oldBases[0]);
    ext = TypecheckVisitor::apply(rctx, ext);
    rctx->bases = oldBases;
    preamble->addStmt(ext);
  }
  return t;
}

} // namespace codon::ast
