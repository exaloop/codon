#include <memory>
#include <string>
#include <tuple>
#include <vector>

#include "codon/parser/ast.h"
#include "codon/parser/cache.h"
#include "codon/parser/common.h"
#include "codon/parser/visitors/simplify/simplify.h"

using fmt::format;

namespace codon::ast {

/// Transform class and type definitions, as well as extensions.
/// See below for details.
void SimplifyVisitor::visit(ClassStmt *stmt) {
  // Get root name
  std::string name = stmt->name;
  if (ctx->getBase()->isType()) {
    // If class B is nested within A, it's name is always A.B, never B itself.
    // Ensure that parent class name is appended
    const auto &a = ctx->getBase()->ast;
    std::string parentName =
        a->getId() ? a->getId()->value : a->getIndex()->expr->getId()->value;
    name = parentName + "." + name;
  }

  // Generate/find class' canonical name (unique ID) and AST
  std::string canonicalName;
  ClassStmt *originalAST = nullptr;
  // classItem will be added later when the scope is different
  auto classItem = std::make_shared<SimplifyItem>(SimplifyItem::Type, "", "",
                                                  ctx->getModule(), ctx->scope);
  classItem->setSrcInfo(stmt->getSrcInfo());
  if (!stmt->attributes.has(Attr::Extend)) {
    classItem->canonicalName = canonicalName =
        ctx->generateCanonicalName(name, !stmt->attributes.has(Attr::Internal));
    // Reference types are added to the context here.
    // Tuple types are added after class contents are parsed to prevent
    // recursive record types (note: these are allowed for reference types).
    if (!stmt->attributes.has(Attr::Tuple)) {
      ctx->add(name, classItem);
      ctx->addAlwaysVisible(classItem);
    }
    originalAST = stmt;
  } else {
    // Find the canonical name and AST of the class that is to be extended
    if (!ctx->isGlobal() || ctx->isConditional())
      error("extend is only allowed at the toplevel");
    auto val = ctx->find(name);
    if (!val || !val->isType())
      error("cannot find type '{}' to extend", name);
    canonicalName = val->canonicalName;
    const auto &astIter = ctx->cache->classes.find(canonicalName);
    if (astIter == ctx->cache->classes.end())
      error("cannot extend type alias or an instantiation ({})", name);
    originalAST = astIter->second.ast.get();
  }

  // Add the class base.
  ctx->bases.emplace_back(SimplifyContext::Base(canonicalName));
  ctx->getBase()->ast = std::make_shared<IdExpr>(name);
  ctx->getBase()->scope = ctx->scope;
  ctx->addBlock();

  // Parse and add class generics
  std::vector<Param> args;
  std::pair<StmtPtr, FunctionStmt *> autoDeducedInit{nullptr, nullptr};
  if (stmt->attributes.has("deduce") && args.empty()) {
    // Auto-detect generics and fields
    autoDeducedInit = autoDeduceMembers(stmt, args);
  } else {
    // Add all generics before parent classes, fields and methods
    for (auto &a : originalAST->args) {
      if (a.status != Param::Generic)
        continue;
      std::string name, varName;
      if (stmt->attributes.has(Attr::Extend))
        varName = a.name, name = ctx->rev(a.name);
      else
        varName = ctx->generateCanonicalName(a.name), name = a.name;
      if (isStaticGeneric(a.type))
        ctx->addVar(name, varName, a.type->getSrcInfo())->generic = true;
      else
        ctx->addType(name, varName, a.type->getSrcInfo())->generic = true;
      args.emplace_back(Param{varName, transformType(a.type, false),
                              transformType(a.defaultValue, false), a.status});
    }
  }
  // Form class reference AST (e.g. `Foo`, or `Foo[T, U]` for generic classes)
  for (auto &a : args) {
    if (a.status == Param::Generic) {
      if (!ctx->getBase()->ast->getIndex())
        ctx->getBase()->ast = N<IndexExpr>(N<IdExpr>(name), N<TupleExpr>());
      ctx->getBase()->ast->getIndex()->index->getTuple()->items.push_back(
          N<IdExpr>(a.name));
    }
  }

  // Collect classes (and their fields) that are to be statically inherited
  auto baseASTs = parseBaseClasses(stmt->baseClasses, args, stmt->attributes);

  // A ClassStmt will be separated into method-free ClassStmts (that include nested
  // classes) and method FunctionStmts
  std::vector<StmtPtr> clsStmts; // Will be filled later!
  std::vector<StmtPtr> fnStmts;  // Will be filled later!
  transformNestedClasses(stmt, clsStmts, fnStmts);

  // Collect class fields
  for (auto &a : originalAST->args) {
    if (a.status == Param::Normal) {
      args.emplace_back(
          Param{a.name, transformType(a.type, false), transform(a.defaultValue, true)});
    }
  }
  // ASTs for member arguments to be used for populating magic methods
  std::vector<Param> memberArgs;
  for (auto &a : args) {
    if (a.status == Param::Normal)
      memberArgs.push_back(a);
  }

  if (!stmt->attributes.has(Attr::Extend)) {
    // Ensure that all fields are registered
    for (auto &a : args)
      if (a.status == Param::Normal) {
        ctx->cache->classes[canonicalName].fields.push_back({a.name, nullptr});
      }
  }

  // Parse class members (arguments) and methods
  if (!stmt->attributes.has(Attr::Extend)) {
    // Now that we are done with arguments, add record type to the context
    if (stmt->attributes.has(Attr::Tuple)) {
      // Ensure that class binding does not shadow anything.
      // Class bindings cannot be dominated either
      auto v = ctx->find(name);
      if (v && v->noShadow)
        error("cannot update global/nonlocal");
      ctx->add(name, classItem);
      ctx->addAlwaysVisible(classItem);
    }
    // Create a cached AST.
    Attr attr = stmt->attributes;
    attr.module =
        format("{}{}", ctx->moduleName.status == ImportFile::STDLIB ? "std::" : "::",
               ctx->moduleName.module);
    ctx->cache->classes[canonicalName].ast =
        N<ClassStmt>(canonicalName, args, N<SuiteStmt>(), attr);
    for (auto &b : baseASTs)
      ctx->cache->classes[canonicalName].parentClasses.emplace_back(b->name);
    ctx->cache->classes[canonicalName].ast->validate();

    // Codegen default magic methods
    for (auto &m : attr.magics) {
      fnStmts.push_back(transform(codegenMagic(m, ctx->getBase()->ast.get(), memberArgs,
                                               stmt->attributes.has(Attr::Tuple))));
    }
    // Add inherited methods
    for (int ai = 0; ai < baseASTs.size(); ai++) {
      for (auto &mm : ctx->cache->classes[baseASTs[ai]->name].methods)
        for (auto &mf : ctx->cache->overloads[mm.second]) {
          auto f = ctx->cache->functions[mf.name].ast;
          if (!f->attributes.has("autogenerated")) {
            std::string rootName;
            auto &mts = ctx->cache->classes[ctx->getBase()->name].methods;
            auto it = mts.find(ctx->rev(f->name));
            if (it != mts.end())
              rootName = it->second;
            else
              rootName = ctx->generateCanonicalName(ctx->rev(f->name), true);
            auto newCanonicalName =
                format("{}:{}", rootName, ctx->cache->overloads[rootName].size());
            ctx->cache->overloads[rootName].push_back(
                {newCanonicalName, ctx->cache->age});
            ctx->cache->reverseIdentifierLookup[newCanonicalName] = ctx->rev(f->name);
            auto nf = std::dynamic_pointer_cast<FunctionStmt>(f->clone());
            nf->name = newCanonicalName;
            nf->attributes.parentClass = ctx->getBase()->name;
            ctx->cache->functions[newCanonicalName].ast = nf;
            ctx->cache->classes[ctx->getBase()->name].methods[ctx->rev(f->name)] =
                rootName;
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
      if (sp.get() != autoDeducedInit.second)
        fnStmts.push_back(transform(sp));
    }

  // After popping context block, record types and nested classes will dissapear.
  // Store their references and re-add them to the context after popping
  std::vector<SimplifyContext::Item> addLater;
  for (auto &c : clsStmts)
    addLater.push_back(ctx->find(c->getClass()->name));
  if (stmt->attributes.has(Attr::Tuple))
    addLater.push_back(ctx->forceFind(name));
  ctx->bases.pop_back();
  ctx->popBlock();
  for (auto &i : addLater)
    ctx->add(ctx->rev(i->canonicalName), i);

  // Extensions are not needed as the cache is already populated
  if (!stmt->attributes.has(Attr::Extend)) {
    auto c = ctx->cache->classes[canonicalName].ast;
    seqassert(c, "not a class AST for {}", canonicalName);
    clsStmts.push_back(c);
  }
  clsStmts.insert(clsStmts.end(), fnStmts.begin(), fnStmts.end());
  resultStmt = N<SuiteStmt>(clsStmts);
}

/// Parse statically inherited classes.
/// Returns a list of their ASTs. Also updates the class fields.
/// @param args Class fields that are to be updated with base classes' fields.
std::vector<ClassStmt *>
SimplifyVisitor::parseBaseClasses(const std::vector<ExprPtr> &baseClasses,
                                  std::vector<Param> &args, const Attr &attr) {
  std::vector<ClassStmt *> asts;
  for (auto &cls : baseClasses) {
    std::string name;
    std::vector<ExprPtr> subs;
    // Get the base class and generic replacements (e.g., if there is Bar[T],
    // Bar in Foo(Bar[int]) will have `T = int`)
    if (auto i = cls->getId()) {
      name = i->value;
    } else if (auto e = cls->getIndex()) {
      if (auto ei = e->expr->getId()) {
        name = ei->value;
        subs = e->index->getTuple() ? e->index->getTuple()->items
                                    : std::vector<ExprPtr>{e->index};
      }
    }
    name = transformType(N<IdExpr>(name))->getId()->value;
    if (name.empty() || !in(ctx->cache->classes, name))
      error(cls.get(), "invalid base class");
    asts.push_back(ctx->cache->classes[name].ast.get());

    // Sanity checks
    if (!attr.has(Attr::Tuple) && asts.back()->attributes.has(Attr::Tuple))
      error("reference classes cannot inherit by-value classes");
    if (asts.back()->attributes.has(Attr::Internal))
      error("cannot inherit internal types");

    // Add generics first
    int si = 0;
    for (auto &a : asts.back()->args) {
      if (a.status == Param::Generic) {
        if (si == subs.size())
          error(cls.get(), "wrong number of generics");
        args.push_back(Param{a.name, a.type, transformType(subs[si++], false),
                             Param::HiddenGeneric});
      } else if (a.status == Param::HiddenGeneric) {
        args.push_back(a);
      }
      if (a.status != Param::Normal) {
        if (isStaticGeneric(a.type))
          ctx->addVar(a.name, a.name, a.type->getSrcInfo())->generic = true;
        else
          ctx->addType(a.name, a.name, a.type->getSrcInfo())->generic = true;
      }
    }
    if (si != subs.size())
      error(cls.get(), "wrong number of generics");
  }
  // Add normal fields
  for (auto &ast : asts) {
    for (auto &a : ast->args) {
      if (a.status == Param::Normal)
        args.emplace_back(Param{a.name, a.type, a.defaultValue});
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
          auto memberName = ctx->rev(varName);
          ctx->addType(memberName, varName, stmt->getSrcInfo())->generic = true;
          args.emplace_back(Param{varName, N<IdExpr>("type"), nullptr, Param::Generic});
          args.emplace_back(Param{m, N<IdExpr>(varName)});
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
    error("only function and class definitions are allowed within classes");
  } else {
    v.push_back(s);
  }
  return v;
}

/// Extract nested classes and transform them before the main class.
void SimplifyVisitor::transformNestedClasses(ClassStmt *stmt,
                                             std::vector<StmtPtr> &clsStmts,
                                             std::vector<StmtPtr> &fnStmts) {
  for (const auto &sp : getClassMethods(stmt->suite))
    if (sp && sp->getClass()) {
      auto origName = sp->getClass()->name;
      auto tsp = transform(sp);
      std::string name;
      for (auto &s : tsp->getSuite()->stmts)
        if (auto c = s->getClass()) {
          clsStmts.push_back(s);
          name = c->name;
        } else {
          fnStmts.push_back(s);
        }
      ctx->add(origName, ctx->forceFind(name));
    }
}

/// Generate a magic method `__op__` for each magic `op`
/// described by @param typExpr and its arguments.
/// Currently generate:
/// @li Constructors: __new__, __init__
/// @li Utilities: __raw__, __hash__, __repr__
/// @li Iteration: __iter__, __getitem__, __len__, __contains__
/// @li Comparisons: __eq__, __ne__, __lt__, __le__, __gt__, __ge__
/// @li Pickling: __pickle__, __unpickle__
/// @li Python: __to_py__, __from_py__
/// TODO: move to Codon as much as possible
StmtPtr SimplifyVisitor::codegenMagic(const std::string &op, const Expr *typExpr,
                                      const std::vector<Param> &args, bool isRecord) {
#define I(s) N<IdExpr>(s)
  seqassert(typExpr, "typExpr is null");
  ExprPtr ret;
  std::vector<Param> fargs;
  std::vector<StmtPtr> stmts;
  Attr attr;
  attr.set("autogenerated");
  if (op == "new") {
    // Classes: @internal def __new__() -> T
    // Tuples: @internal def __new__(a1: T1, ..., aN: TN) -> T
    ret = typExpr->clone();
    if (isRecord)
      for (auto &a : args)
        fargs.emplace_back(
            Param{a.name, clone(a.type),
                  a.defaultValue ? clone(a.defaultValue) : N<CallExpr>(clone(a.type))});
    attr.set(Attr::Internal);
  } else if (op == "init") {
    // Classes: def __init__(self: T, a1: T1, ..., aN: TN) -> void:
    //            self.aI = aI ...
    ret = I("void");
    fargs.emplace_back(Param{"self", typExpr->clone()});
    for (auto &a : args) {
      stmts.push_back(N<AssignStmt>(N<DotExpr>(I("self"), a.name), I(a.name)));
      fargs.emplace_back(
          Param{a.name, clone(a.type),
                a.defaultValue ? clone(a.defaultValue) : N<CallExpr>(clone(a.type))});
    }
  } else if (op == "raw") {
    // Classes: def __raw__(self: T) -> Ptr[byte]:
    //            return __internal__.class_raw(self)
    fargs.emplace_back(Param{"self", typExpr->clone()});
    ret = N<IndexExpr>(I("Ptr"), I("byte"));
    stmts.emplace_back(N<ReturnStmt>(
        N<CallExpr>(N<DotExpr>(I("__internal__"), "class_raw"), I("self"))));
  } else if (op == "getitem") {
    // Tuples: def __getitem__(self: T, index: int) -> T1:
    //           return __internal__.tuple_getitem[T, T1](self, index)
    //         (error during a realizeFunc() method if T is a heterogeneous tuple)
    fargs.emplace_back(Param{"self", typExpr->clone()});
    fargs.emplace_back(Param{"index", I("int")});
    ret = !args.empty() ? clone(args[0].type) : I("void");
    stmts.emplace_back(N<ReturnStmt>(
        N<CallExpr>(N<DotExpr>(I("__internal__"), "tuple_getitem"), I("self"),
                    I("index"), typExpr->clone(), ret->clone())));
  } else if (op == "iter") {
    // Tuples: def __iter__(self: T) -> Generator[T]:
    //           yield self.aI ...
    //         (error during a realizeFunc() method if T is a heterogeneous tuple)
    fargs.emplace_back(Param{"self", typExpr->clone()});
    ret = N<IndexExpr>(I("Generator"), !args.empty() ? clone(args[0].type) : I("int"));
    for (auto &a : args)
      stmts.emplace_back(N<YieldStmt>(N<DotExpr>("self", a.name)));
    if (args.empty()) // Hack for empty tuple: yield from List[int]()
      stmts.emplace_back(
          N<YieldFromStmt>(N<CallExpr>(N<IndexExpr>(I("List"), I("int")))));
  } else if (op == "contains") {
    // Tuples: def __contains__(self: T, what) -> bool:
    //            if isinstance(what, T1): if what == self.a1: return True ...
    //            return False
    fargs.emplace_back(Param{"self", typExpr->clone()});
    fargs.emplace_back(Param{"what", nullptr});
    ret = I("bool");
    for (auto &a : args)
      stmts.push_back(N<IfStmt>(N<CallExpr>(I("isinstance"), I("what"), clone(a.type)),
                                N<IfStmt>(N<CallExpr>(N<DotExpr>(I("what"), "__eq__"),
                                                      N<DotExpr>(I("self"), a.name)),
                                          N<ReturnStmt>(N<BoolExpr>(true)))));
    stmts.emplace_back(N<ReturnStmt>(N<BoolExpr>(false)));
  } else if (op == "eq") {
    // def __eq__(self: T, other: T) -> bool:
    //   if not self.arg1.__eq__(other.arg1): return False ...
    //   return True
    fargs.emplace_back(Param{"self", typExpr->clone()});
    fargs.emplace_back(Param{"other", typExpr->clone()});
    ret = I("bool");
    for (auto &a : args)
      stmts.push_back(N<IfStmt>(
          N<UnaryExpr>("!",
                       N<CallExpr>(N<DotExpr>(N<DotExpr>(I("self"), a.name), "__eq__"),
                                   N<DotExpr>(I("other"), a.name))),
          N<ReturnStmt>(N<BoolExpr>(false))));
    stmts.emplace_back(N<ReturnStmt>(N<BoolExpr>(true)));
  } else if (op == "ne") {
    // def __ne__(self: T, other: T) -> bool:
    //   if self.arg1.__ne__(other.arg1): return True ...
    //   return False
    fargs.emplace_back(Param{"self", typExpr->clone()});
    fargs.emplace_back(Param{"other", typExpr->clone()});
    ret = I("bool");
    for (auto &a : args)
      stmts.emplace_back(
          N<IfStmt>(N<CallExpr>(N<DotExpr>(N<DotExpr>(I("self"), a.name), "__ne__"),
                                N<DotExpr>(I("other"), a.name)),
                    N<ReturnStmt>(N<BoolExpr>(true))));
    stmts.push_back(N<ReturnStmt>(N<BoolExpr>(false)));
  } else if (op == "lt" || op == "gt") {
    // def __lt__(self: T, other: T) -> bool:  (same for __gt__)
    //   if self.arg1.__lt__(other.arg1): return True
    //   elif self.arg1.__eq__(other.arg1):
    //      ... (arg2, ...) ...
    //   return False
    fargs.emplace_back(Param{"self", typExpr->clone()});
    fargs.emplace_back(Param{"other", typExpr->clone()});
    ret = I("bool");
    std::vector<StmtPtr> *v = &stmts;
    for (int i = 0; i < (int)args.size() - 1; i++) {
      v->emplace_back(N<IfStmt>(
          N<CallExpr>(
              N<DotExpr>(N<DotExpr>(I("self"), args[i].name), format("__{}__", op)),
              N<DotExpr>(I("other"), args[i].name)),
          N<ReturnStmt>(N<BoolExpr>(true)),
          N<IfStmt>(
              N<CallExpr>(N<DotExpr>(N<DotExpr>(I("self"), args[i].name), "__eq__"),
                          N<DotExpr>(I("other"), args[i].name)),
              N<SuiteStmt>())));
      v = &((SuiteStmt *)(((IfStmt *)(((IfStmt *)(v->back().get()))->elseSuite.get()))
                              ->ifSuite)
                .get())
               ->stmts;
    }
    if (!args.empty())
      v->emplace_back(N<ReturnStmt>(N<CallExpr>(
          N<DotExpr>(N<DotExpr>(I("self"), args.back().name), format("__{}__", op)),
          N<DotExpr>(I("other"), args.back().name))));
    stmts.emplace_back(N<ReturnStmt>(N<BoolExpr>(false)));
  } else if (op == "le" || op == "ge") {
    // def __le__(self: T, other: T) -> bool:  (same for __ge__)
    //   if not self.arg1.__le__(other.arg1): return False
    //   elif self.arg1.__eq__(other.arg1):
    //      ... (arg2, ...) ...
    //   return True
    fargs.emplace_back(Param{"self", typExpr->clone()});
    fargs.emplace_back(Param{"other", typExpr->clone()});
    ret = I("bool");
    std::vector<StmtPtr> *v = &stmts;
    for (int i = 0; i < (int)args.size() - 1; i++) {
      v->emplace_back(N<IfStmt>(
          N<UnaryExpr>("!", N<CallExpr>(N<DotExpr>(N<DotExpr>(I("self"), args[i].name),
                                                   format("__{}__", op)),
                                        N<DotExpr>(I("other"), args[i].name))),
          N<ReturnStmt>(N<BoolExpr>(false)),
          N<IfStmt>(
              N<CallExpr>(N<DotExpr>(N<DotExpr>(I("self"), args[i].name), "__eq__"),
                          N<DotExpr>(I("other"), args[i].name)),
              N<SuiteStmt>())));
      v = &((SuiteStmt *)(((IfStmt *)(((IfStmt *)(v->back().get()))->elseSuite.get()))
                              ->ifSuite)
                .get())
               ->stmts;
    }
    if (!args.empty())
      v->emplace_back(N<ReturnStmt>(N<CallExpr>(
          N<DotExpr>(N<DotExpr>(I("self"), args.back().name), format("__{}__", op)),
          N<DotExpr>(I("other"), args.back().name))));
    stmts.emplace_back(N<ReturnStmt>(N<BoolExpr>(true)));
  } else if (op == "hash") {
    // def __hash__(self: T) -> int:
    //   seed = 0
    //   seed = (
    //     seed ^ ((self.arg1.__hash__() + 2654435769) + ((seed << 6) + (seed >> 2)))
    //   ) ...
    //   return seed
    fargs.emplace_back(Param{"self", typExpr->clone()});
    ret = I("int");
    stmts.emplace_back(N<AssignStmt>(I("seed"), N<IntExpr>(0)));
    for (auto &a : args)
      stmts.push_back(N<AssignStmt>(
          I("seed"),
          N<BinaryExpr>(
              I("seed"), "^",
              N<BinaryExpr>(
                  N<BinaryExpr>(N<CallExpr>(N<DotExpr>(N<DotExpr>(I("self"), a.name),
                                                       "__hash__")),
                                "+", N<IntExpr>(0x9e3779b9)),
                  "+",
                  N<BinaryExpr>(N<BinaryExpr>(I("seed"), "<<", N<IntExpr>(6)), "+",
                                N<BinaryExpr>(I("seed"), ">>", N<IntExpr>(2)))))));
    stmts.emplace_back(N<ReturnStmt>(I("seed")));
  } else if (op == "pickle") {
    // def __pickle__(self: T, dest: Ptr[byte]) -> void:
    //   self.arg1.__pickle__(dest) ...
    fargs.emplace_back(Param{"self", typExpr->clone()});
    fargs.emplace_back(Param{"dest", N<IndexExpr>(I("Ptr"), I("byte"))});
    ret = I("void");
    for (auto &a : args)
      stmts.emplace_back(N<ExprStmt>(N<CallExpr>(
          N<DotExpr>(N<DotExpr>(I("self"), a.name), "__pickle__"), I("dest"))));
  } else if (op == "unpickle") {
    // def __unpickle__(src: Ptr[byte]) -> T:
    //   return T(T1.__unpickle__(src),...)
    fargs.emplace_back(Param{"src", N<IndexExpr>(I("Ptr"), I("byte"))});
    ret = typExpr->clone();
    std::vector<ExprPtr> ar;
    for (auto &a : args)
      ar.emplace_back(N<CallExpr>(N<DotExpr>(clone(a.type), "__unpickle__"), I("src")));
    stmts.emplace_back(N<ReturnStmt>(N<CallExpr>(typExpr->clone(), ar)));
  } else if (op == "len") {
    // def __len__(self: T) -> int:
    //   return N (number of args)
    fargs.emplace_back(Param{"self", typExpr->clone()});
    ret = I("int");
    stmts.emplace_back(N<ReturnStmt>(N<IntExpr>(args.size())));
  } else if (op == "to_py") {
    // def __to_py__(self: T) -> cobj:
    //   o = pyobj._tuple_new(N)  (number of args)
    //   pyobj._tuple_set(o, 1, self.arg1.__to_py__()) ...
    //   return o
    fargs.emplace_back(Param{"self", typExpr->clone()});
    ret = I("cobj");
    stmts.emplace_back(
        N<AssignStmt>(I("o"), N<CallExpr>(N<DotExpr>(I("pyobj"), "_tuple_new"),
                                          N<IntExpr>(args.size()))));
    for (int i = 0; i < args.size(); i++)
      stmts.push_back(N<ExprStmt>(N<CallExpr>(
          N<DotExpr>(I("pyobj"), "_tuple_set"), I("o"), N<IntExpr>(i),
          N<CallExpr>(N<DotExpr>(N<DotExpr>(I("self"), args[i].name), "__to_py__")))));
    stmts.emplace_back(N<ReturnStmt>(I("o")));
  } else if (op == "from_py") {
    // def __from_py__(src: cobj) -> T:
    //   return T(T1.__from_py__(pyobj._tuple_get(src, 1)), ...)
    fargs.emplace_back(Param{"src", I("cobj")});
    ret = typExpr->clone();
    std::vector<ExprPtr> ar;
    for (int i = 0; i < args.size(); i++)
      ar.push_back(N<CallExpr>(
          N<DotExpr>(clone(args[i].type), "__from_py__"),
          N<CallExpr>(N<DotExpr>(I("pyobj"), "_tuple_get"), I("src"), N<IntExpr>(i))));
    stmts.emplace_back(N<ReturnStmt>(N<CallExpr>(typExpr->clone(), ar)));
  } else if (op == "repr") {
    // def __repr__(self: T) -> str:
    //   a = __array__[str](N)  (number of args)
    //   n = __array__[str](N)  (number of args)
    //   a.__setitem__(0, self.arg1.__repr__()) ...
    //   n.__setitem__(0, "arg1") ...  (if not a Tuple.N; otherwise "")
    //   return __internal__.tuple_str(a.ptr, n.ptr, N)
    fargs.emplace_back(Param{"self", typExpr->clone()});
    ret = I("str");
    if (!args.empty()) {
      stmts.emplace_back(
          N<AssignStmt>(I("a"), N<CallExpr>(N<IndexExpr>(I("__array__"), I("str")),
                                            N<IntExpr>(args.size()))));
      stmts.emplace_back(
          N<AssignStmt>(I("n"), N<CallExpr>(N<IndexExpr>(I("__array__"), I("str")),
                                            N<IntExpr>(args.size()))));
      for (int i = 0; i < args.size(); i++) {
        stmts.push_back(N<ExprStmt>(N<CallExpr>(
            N<DotExpr>(I("a"), "__setitem__"), N<IntExpr>(i),
            N<CallExpr>(N<DotExpr>(N<DotExpr>(I("self"), args[i].name), "__repr__")))));

        auto name = const_cast<Expr *>(typExpr)->getIndex()
                        ? const_cast<Expr *>(typExpr)->getIndex()->expr->getId()
                        : nullptr;
        stmts.push_back(N<ExprStmt>(N<CallExpr>(
            N<DotExpr>(I("n"), "__setitem__"), N<IntExpr>(i),
            N<StringExpr>(
                name && startswith(name->value, TYPE_TUPLE) ? "" : args[i].name))));
      }
      stmts.emplace_back(N<ReturnStmt>(N<CallExpr>(
          N<DotExpr>(I("__internal__"), "tuple_str"), N<DotExpr>(I("a"), "ptr"),
          N<DotExpr>(I("n"), "ptr"), N<IntExpr>(args.size()))));
    } else {
      stmts.emplace_back(N<ReturnStmt>(N<StringExpr>("()")));
    }
  } else if (op == "dict") {
    // def __dict__(self: T):
    //   d = List[str](N)
    //   d.append('arg1')  ...
    //   return d
    fargs.emplace_back(Param{"self", typExpr->clone()});
    stmts.emplace_back(
        N<AssignStmt>(I("d"), N<CallExpr>(N<IndexExpr>(I("List"), I("str")),
                                          N<IntExpr>(args.size()))));
    for (auto &a : args)
      stmts.push_back(N<ExprStmt>(
          N<CallExpr>(N<DotExpr>(I("d"), "append"), N<StringExpr>(a.name))));
    stmts.emplace_back(N<ReturnStmt>(I("d")));
  } else if (op == "add") {
    // def __add__(self, tup):
    //   return (*self, *t)
    fargs.emplace_back(Param{"self", typExpr->clone()});
    fargs.emplace_back(Param{"tup", nullptr});
    stmts.emplace_back(N<ReturnStmt>(N<TupleExpr>(
        std::vector<ExprPtr>{N<StarExpr>(I("self")), N<StarExpr>(I("tup"))})));
  } else {
    seqassert(false, "invalid magic {}", op);
  }
#undef I
  auto t = std::make_shared<FunctionStmt>(format("__{}__", op), ret, fargs,
                                          N<SuiteStmt>(stmts), attr);
  t->setSrcInfo(ctx->cache->generateSrcInfo());
  return t;
}

} // namespace codon::ast