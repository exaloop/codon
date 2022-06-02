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

struct ReplacementVisitor : public ReplaceASTVisitor {
  const std::unordered_map<std::string, ExprPtr> *table;

  explicit ReplacementVisitor(const std::unordered_map<std::string, ExprPtr> *t)
      : table(t) {}

  void transform(ExprPtr &e) override {
    if (!e)
      return;
    ReplacementVisitor v{table};
    e->accept(v);
    if (auto i = e->getId()) {
      auto it = table->find(i->value);
      if (it != table->end())
        e = it->second->clone();
    }
  }

  void transform(StmtPtr &e) override {
    if (!e)
      return;
    ReplacementVisitor v{table};
    e->accept(v);
  }

  template <typename T>
  static T replace(const T &e, const std::unordered_map<std::string, ExprPtr> &s) {
    ReplacementVisitor v{&s};
    auto ep = clone(e);
    v.transform(ep);
    return ep;
  }
};

void SimplifyVisitor::visit(ClassStmt *stmt) {
  enum Magic { Init, Repr, Eq, Order, Hash, Pickle, Container, Python };
  Attr attr = stmt->attributes;
  std::vector<char> hasMagic(10, 2);
  hasMagic[Init] = hasMagic[Pickle] = 1;
  bool deduce = false;
  // @tuple(init=, repr=, eq=, order=, hash=, pickle=, container=, python=, add=,
  // internal=...)
  // @dataclass(...)
  // @extend
  for (auto &d : stmt->decorators) {
    if (d->isId("deduce")) {
      deduce = true;
    } else if (auto c = d->getCall()) {
      if (c->expr->isId(Attr::Tuple))
        attr.set(Attr::Tuple);
      else if (!c->expr->isId("dataclass"))
        error("invalid class attribute");
      else if (attr.has(Attr::Tuple))
        error("class already marked as tuple");
      for (auto &a : c->args) {
        auto b = CAST(a.value, BoolExpr);
        if (!b)
          error("expected static boolean");
        char val = char(b->value);
        if (a.name == "init")
          hasMagic[Init] = val;
        else if (a.name == "repr")
          hasMagic[Repr] = val;
        else if (a.name == "eq")
          hasMagic[Eq] = val;
        else if (a.name == "order")
          hasMagic[Order] = val;
        else if (a.name == "hash")
          hasMagic[Hash] = val;
        else if (a.name == "pickle")
          hasMagic[Pickle] = val;
        else if (a.name == "python")
          hasMagic[Python] = val;
        else if (a.name == "container")
          hasMagic[Container] = val;
        else
          error("invalid decorator argument");
      }
    } else if (d->isId(Attr::Tuple)) {
      if (attr.has(Attr::Tuple))
        error("class already marked as tuple");
      attr.set(Attr::Tuple);
    } else if (d->isId(Attr::Extend)) {
      attr.set(Attr::Extend);
      if (stmt->decorators.size() != 1)
        error("extend cannot be combined with other decorators");
      if (!ctx->bases.empty())
        error("extend is only allowed at the toplevel");
    } else if (d->isId(Attr::Internal)) {
      attr.set(Attr::Internal);
    }
  }
  for (int i = 1; i < hasMagic.size(); i++)
    if (hasMagic[i] == 2)
      hasMagic[i] = attr.has(Attr::Tuple) ? 1 : 0;

  // Extensions (@extend) cases are handled bit differently
  // (no auto method-generation, no arguments etc.)
  bool extension = attr.has(Attr::Extend);
  bool isRecord = attr.has(Attr::Tuple); // does it have @tuple attribute

  // Special name handling is needed because of nested classes.
  std::string name = stmt->name;
  if (!ctx->bases.empty() && ctx->bases.back().isType()) {
    const auto &a = ctx->bases.back().ast;
    std::string parentName =
        a->getId() ? a->getId()->value : a->getIndex()->expr->getId()->value;
    name = parentName + "." + name;
  }

  // Generate/find class' canonical name (unique ID) and AST
  std::string canonicalName;
  ClassStmt *originalAST = nullptr;
  auto classItem = std::make_shared<SimplifyItem>(SimplifyItem::Type, "", "",
                                                  ctx->getModule(), ctx->scope);
  classItem->setSrcInfo(stmt->getSrcInfo());
  classItem->moduleName = ctx->getModule();
  std::vector<std::pair<std::string, std::shared_ptr<SimplifyItem>>> addLater;
  if (!extension) {
    classItem->canonicalName = canonicalName =
        ctx->generateCanonicalName(name, !attr.has(Attr::Internal));
    // Reference types are added to the context at this stage.
    // Record types (tuples) are added after parsing class arguments to prevent
    // recursive record types (that are allowed for reference types).
    if (!isRecord) {
      auto v = ctx->find(name);
      if (v && v->noShadow)
        error("cannot update global/nonlocal");
      ctx->add(name, classItem);
      ctx->addAlwaysVisible(classItem);
    }
    originalAST = stmt;
  } else {
    // Find the canonical name of a class that is to be extended
    auto val = ctx->find(name);
    if (!val || !val->isType())
      error("cannot find type '{}' to extend", name);
    canonicalName = val->canonicalName;
    const auto &astIter = ctx->cache->classes.find(canonicalName);
    if (astIter == ctx->cache->classes.end())
      error("cannot extend type alias or an instantiation ({})", name);
    originalAST = astIter->second.ast.get();
    if (!stmt->args.empty())
      error("extensions cannot be generic or declare members");
  }

  // Add the class base.
  ctx->bases.emplace_back(SimplifyContext::Base(canonicalName));
  ctx->bases.back().ast = std::make_shared<IdExpr>(name);

  if (extension && !stmt->baseClasses.empty())
    error("extensions cannot inherit other classes");
  std::vector<ClassStmt *> baseASTs;
  std::vector<Param> args;
  std::vector<std::unordered_map<std::string, ExprPtr>> substitutions;
  std::vector<int> argSubstitutions;
  std::unordered_set<std::string> seenMembers;
  std::vector<int> baseASTsFields;
  for (auto &baseClass : stmt->baseClasses) {
    std::string bcName;
    std::vector<ExprPtr> subs;
    if (auto i = baseClass->getId())
      bcName = i->value;
    else if (auto e = baseClass->getIndex()) {
      if (auto ei = e->expr->getId()) {
        bcName = ei->value;
        subs = e->index->getTuple() ? e->index->getTuple()->items
                                    : std::vector<ExprPtr>{e->index};
      }
    }
    bcName = transformType(N<IdExpr>(bcName))->getId()->value;
    if (bcName.empty() || !in(ctx->cache->classes, bcName))
      error(baseClass.get(), "invalid base class");
    baseASTs.push_back(ctx->cache->classes[bcName].ast.get());
    if (!isRecord && baseASTs.back()->attributes.has(Attr::Tuple))
      error("reference classes cannot inherit by-value classes");
    if (baseASTs.back()->attributes.has(Attr::Internal))
      error("cannot inherit internal types");
    int si = 0;
    substitutions.emplace_back();
    for (auto &a : baseASTs.back()->args)
      if (a.generic) {
        if (si >= subs.size())
          error(baseClass.get(), "wrong number of generics");
        substitutions.back()[a.name] = clone(subs[si++]);
      }
    if (si != subs.size())
      error(baseClass.get(), "wrong number of generics");
    for (auto &a : baseASTs.back()->args)
      if (!a.generic) {
        if (seenMembers.find(a.name) != seenMembers.end())
          error(a.type, "'{}' declared twice", a.name);
        seenMembers.insert(a.name);
        args.emplace_back(Param{a.name, a.type, a.defaultValue});
        argSubstitutions.push_back(int(substitutions.size()) - 1);
        if (!extension)
          ctx->cache->classes[canonicalName].fields.push_back({a.name, nullptr});
      }
    baseASTsFields.push_back(int(args.size()));
  }

  // Add generics, if any, to the context.
  ctx->addBlock();
  std::vector<ExprPtr> genAst;
  substitutions.emplace_back();
  for (auto &a : (extension ? originalAST : stmt)->args) {
    seqassert(a.type, "no type provided for '{}'", a.name);
    if (a.type && (a.type->isId("type") || a.type->isId("TypeVar") ||
                   (a.type->getIndex() && a.type->getIndex()->expr->isId("Static"))))
      a.generic = true;
    if (seenMembers.find(a.name) != seenMembers.end())
      error(a.type, "'{}' declared twice", a.name);
    seenMembers.insert(a.name);
    if (a.generic) {
      auto varName = extension ? a.name : ctx->generateCanonicalName(a.name);
      auto genName = extension ? ctx->cache->reverseIdentifierLookup[a.name] : a.name;
      if (a.type->getIndex() && a.type->getIndex()->expr->isId("Static"))
        ctx->addVar(genName, varName, a.type->getSrcInfo());
      else
        ctx->addType(genName, varName, a.type->getSrcInfo());
      genAst.push_back(N<IdExpr>(varName));
      args.emplace_back(Param{varName, a.type, a.defaultValue, a.generic});
    } else {
      args.emplace_back(Param{a.name, a.type, a.defaultValue});
      if (!extension)
        ctx->cache->classes[canonicalName].fields.push_back({a.name, nullptr});
    }
    argSubstitutions.push_back(int(substitutions.size()) - 1);
  }

  // Auto-detect fields
  StmtPtr autoDeducedInit = nullptr;
  Stmt *firstInit = nullptr;
  if (deduce && args.empty() && !extension) {
    for (const auto &sp : getClassMethods(stmt->suite))
      if (sp && sp->getFunction()) {
        firstInit = sp.get();
        auto f = sp->getFunction();
        if (f->name == "__init__" && !f->args.empty() && f->args[0].name == "self") {
          ctx->bases.back().deducedMembers =
              std::make_shared<std::vector<std::string>>();
          autoDeducedInit = transform(sp);
          std::dynamic_pointer_cast<FunctionStmt>(autoDeducedInit)
              ->attributes.set(Attr::RealizeWithoutSelf);
          ctx->cache->functions[autoDeducedInit->getFunction()->name]
              .ast->attributes.set(Attr::RealizeWithoutSelf);

          int i = 0;
          for (auto &m : *(ctx->bases.back().deducedMembers)) {
            auto varName = ctx->generateCanonicalName(format("T{}", ++i));
            auto memberName = ctx->cache->reverseIdentifierLookup[varName];
            ctx->addType(memberName, varName, stmt->getSrcInfo());
            genAst.push_back(N<IdExpr>(varName));
            args.emplace_back(Param{varName, N<IdExpr>("type"), nullptr, true});
            argSubstitutions.push_back(int(substitutions.size()) - 1);

            ctx->cache->classes[canonicalName].fields.push_back({m, nullptr});
            args.emplace_back(Param{m, N<IdExpr>(varName), nullptr});
            argSubstitutions.push_back(int(substitutions.size()) - 1);
          }
          ctx->bases.back().deducedMembers = nullptr;
          break;
        }
      }
  }
  if (!genAst.empty())
    ctx->bases.back().ast =
        std::make_shared<IndexExpr>(N<IdExpr>(name), N<TupleExpr>(genAst));

  std::vector<StmtPtr> clsStmts; // Will be filled later!
  std::vector<StmtPtr> fnStmts;  // Will be filled later!
  // Parse nested classes
  for (const auto &sp : getClassMethods(stmt->suite))
    if (sp && sp->getClass()) {
      // Add dummy base to fix nested class' name.
      ctx->bases.emplace_back(SimplifyContext::Base(canonicalName));
      ctx->bases.back().ast = std::make_shared<IdExpr>(name);
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
      auto orig = ctx->find(name);
      seqassert(orig, "cannot find '{}'", name);
      ctx->add(origName, orig);
      addLater.push_back({origName, orig});
      ctx->bases.pop_back();
    }

  std::vector<Param> memberArgs;
  for (auto &s : substitutions)
    for (auto &i : s)
      i.second = transform(i.second, true);
  for (int ai = 0; ai < args.size(); ai++) {
    auto &a = args[ai];
    if (argSubstitutions[ai] == substitutions.size() - 1) {
      a.type = transformType(a.type, false);
      a.defaultValue = transform(a.defaultValue, true);
    } else {
      a.type = ReplacementVisitor::replace(a.type, substitutions[argSubstitutions[ai]]);
      a.defaultValue =
          ReplacementVisitor::replace(a.defaultValue, substitutions[argSubstitutions[ai]]);
    }
    if (!a.generic)
      memberArgs.push_back(a);
  }

  // Parse class members (arguments) and methods.
  if (!extension) {
    // Now that we are done with arguments, add record type to the context.
    if (isRecord) {
      auto v = ctx->find(stmt->name);
      if (v && v->noShadow)
        error("cannot update global/nonlocal");
      ctx->add(name, classItem);
      ctx->addAlwaysVisible(classItem);
      addLater.push_back({name, classItem});
    }
    // Create a cached AST.
    attr.module =
        format("{}{}", ctx->moduleName.status == ImportFile::STDLIB ? "std::" : "::",
               ctx->moduleName.module);
    ctx->cache->classes[canonicalName].ast =
        N<ClassStmt>(canonicalName, args, N<SuiteStmt>(), attr);
    for (int i = 0; i < baseASTs.size(); i++)
      ctx->cache->classes[canonicalName].parentClasses.emplace_back(baseASTs[i]->name,
                                                                    baseASTsFields[i]);
    std::vector<StmtPtr> fns;
    ExprPtr codeType = ctx->bases.back().ast->clone();
    std::vector<std::string> magics{};
    // Internal classes do not get any auto-generated members.
    if (!attr.has(Attr::Internal)) {
      // Prepare a list of magics that are to be auto-generated.
      if (isRecord)
        magics = {"len", "hash"};
      else
        magics = {"new", "raw"};
      if (hasMagic[Init] && !firstInit)
        magics.emplace_back(isRecord ? "new" : "init");
      if (hasMagic[Eq])
        for (auto &i : {"eq", "ne"})
          magics.emplace_back(i);
      if (hasMagic[Order])
        for (auto &i : {"lt", "gt", "le", "ge"})
          magics.emplace_back(i);
      if (hasMagic[Pickle])
        for (auto &i : {"pickle", "unpickle"})
          magics.emplace_back(i);
      if (hasMagic[Repr])
        magics.emplace_back("repr");
      if (hasMagic[Container])
        for (auto &i : {"iter", "getitem"})
          magics.emplace_back(i);
      if (hasMagic[Python])
        for (auto &i : {"to_py", "from_py"})
          magics.emplace_back(i);

      if (hasMagic[Container] && startswith(stmt->name, TYPE_TUPLE))
        magics.emplace_back("contains");
      if (!startswith(stmt->name, TYPE_TUPLE))
        magics.emplace_back("dict");
      if (startswith(stmt->name, TYPE_TUPLE))
        magics.emplace_back("add");
    }
    // Codegen default magic methods and add them to the final AST.
    for (auto &m : magics) {
      fnStmts.push_back(transform(
          codegenMagic(m, ctx->bases.back().ast.get(), memberArgs, isRecord)));
    }
  }
  for (int ai = 0; ai < baseASTs.size(); ai++) {
    for (auto &mm : ctx->cache->classes[baseASTs[ai]->name].methods)
      for (auto &mf : ctx->cache->overloads[mm.second]) {
        auto f = ctx->cache->functions[mf.name].ast;
        if (f->attributes.has("autogenerated"))
          continue;

        auto subs = substitutions[ai];

        std::string rootName;
        auto &mts = ctx->cache->classes[ctx->bases.back().name].methods;
        auto it = mts.find(ctx->cache->reverseIdentifierLookup[f->name]);
        if (it != mts.end())
          rootName = it->second;
        else
          rootName = ctx->generateCanonicalName(
              ctx->cache->reverseIdentifierLookup[f->name], true);
        auto newCanonicalName =
            format("{}:{}", rootName, ctx->cache->overloads[rootName].size());
        ctx->cache->reverseIdentifierLookup[newCanonicalName] =
            ctx->cache->reverseIdentifierLookup[f->name];
        auto nf = std::dynamic_pointer_cast<FunctionStmt>(
            ReplacementVisitor::replace(std::static_pointer_cast<Stmt>(f), subs));
        subs[nf->name] = N<IdExpr>(newCanonicalName);
        nf->name = newCanonicalName;
        fnStmts.push_back(nf);
        nf->attributes.parentClass = ctx->bases.back().name;

        // check original ast...
        if (nf->attributes.has(".changedSelf")) // replace self type with new class
          nf->args[0].type = transformType(ctx->bases.back().ast);
        // preamble->functions.push_back(clone(nf));
        ctx->cache->overloads[rootName].push_back({newCanonicalName, ctx->cache->age});
        ctx->cache->functions[newCanonicalName].ast = nf;
        ctx->cache->classes[ctx->bases.back().name]
            .methods[ctx->cache->reverseIdentifierLookup[f->name]] = rootName;
      }
  }
  if (autoDeducedInit)
    fnStmts.push_back(autoDeducedInit);
  for (const auto &sp : getClassMethods(stmt->suite))
    if (sp && !sp->getClass()) {
      if (firstInit && firstInit == sp.get())
        continue;
      fnStmts.push_back(transform(sp));
    }
  ctx->bases.pop_back();
  ctx->popBlock();
  for (auto &i : addLater)
    ctx->add(i.first, i.second); // as previous popBlock removes it

  auto c = ctx->cache->classes[canonicalName].ast;
  if (!extension) {
    // Update the cached AST.
    seqassert(c, "not a class AST for {}", canonicalName);
    clsStmts.push_back(c);
  }
  clsStmts.insert(clsStmts.end(), fnStmts.begin(), fnStmts.end());
  resultStmt = N<SuiteStmt>(clsStmts);
}

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
      fargs.emplace_back(Param{a.name, clone(a.type),
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

} // namespace codon::ast