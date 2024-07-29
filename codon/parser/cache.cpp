// Copyright (C) 2022-2024 Exaloop Inc. <https://exaloop.io>

#include "cache.h"

#include <chrono>
#include <string>
#include <vector>

#include "codon/cir/pyextension.h"
#include "codon/cir/util/irtools.h"
#include "codon/parser/common.h"
#include "codon/parser/peg/peg.h"
#include "codon/parser/visitors/simplify/simplify.h"
#include "codon/parser/visitors/translate/translate.h"
#include "codon/parser/visitors/typecheck/ctx.h"
#include "codon/parser/visitors/typecheck/typecheck.h"

namespace codon::ast {

Cache::Cache(std::string argv0)
    : generatedSrcInfoCount(0), unboundCount(256), varCount(0), age(0),
      argv0(std::move(argv0)), typeCtx(nullptr), codegenCtx(nullptr), isJit(false),
      jitCell(0), pythonExt(false), pyModule(nullptr) {}

std::string Cache::getTemporaryVar(const std::string &prefix, char sigil) {
  return fmt::format("{}{}_{}", sigil ? fmt::format("{}_", sigil) : "", prefix,
                     ++varCount);
}

std::string Cache::rev(const std::string &s) {
  auto i = reverseIdentifierLookup.find(s);
  if (i != reverseIdentifierLookup.end())
    return i->second;
  seqassertn(false, "'{}' has no non-canonical name", s);
  return "";
}

void Cache::addGlobal(const std::string &name, ir::Var *var) {
  if (!in(globals, name)) {
    // LOG("[global] {}", name);
    globals[name] = {false, var};
  }
}

SrcInfo Cache::generateSrcInfo() {
  return {FILE_GENERATED, generatedSrcInfoCount, generatedSrcInfoCount++, 0};
}

std::string Cache::getContent(const SrcInfo &info) {
  auto i = imports.find(info.file);
  if (i == imports.end())
    return "";
  int line = info.line - 1;
  if (line < 0 || line >= i->second.content.size())
    return "";
  auto s = i->second.content[line];
  int col = info.col - 1;
  if (col < 0 || col >= s.size())
    return "";
  int len = info.len;
  return s.substr(col, len);
}

types::ClassTypePtr Cache::findClass(const std::string &name) const {
  auto f = typeCtx->find(name);
  if (f && f->kind == TypecheckItem::Type)
    return f->type->getClass();
  return nullptr;
}

types::FuncTypePtr Cache::findFunction(const std::string &name) const {
  auto f = typeCtx->find(name);
  if (f && f->type && f->kind == TypecheckItem::Func)
    return f->type->getFunc();
  f = typeCtx->find(name + ":0");
  if (f && f->type && f->kind == TypecheckItem::Func)
    return f->type->getFunc();
  return nullptr;
}

types::FuncTypePtr Cache::findMethod(types::ClassType *typ, const std::string &member,
                                     const std::vector<types::TypePtr> &args) {
  auto e = std::make_shared<IdExpr>(typ->name);
  e->type = typ->getClass();
  seqassertn(e->type, "not a class");
  int oldAge = typeCtx->age;
  typeCtx->age = 99999;
  auto f = TypecheckVisitor(typeCtx).findBestMethod(e->type->getClass(), member, args);
  typeCtx->age = oldAge;
  return f;
}

ir::types::Type *Cache::realizeType(types::ClassTypePtr type,
                                    const std::vector<types::TypePtr> &generics) {
  auto e = std::make_shared<IdExpr>(type->name);
  e->type = type;
  type = typeCtx->instantiateGeneric(type, generics)->getClass();
  auto tv = TypecheckVisitor(typeCtx);
  if (auto rtv = tv.realize(type)) {
    return classes[rtv->getClass()->name]
        .realizations[rtv->getClass()->realizedTypeName()]
        ->ir;
  }
  return nullptr;
}

ir::Func *Cache::realizeFunction(types::FuncTypePtr type,
                                 const std::vector<types::TypePtr> &args,
                                 const std::vector<types::TypePtr> &generics,
                                 const types::ClassTypePtr &parentClass) {
  auto e = std::make_shared<IdExpr>(type->ast->name);
  e->type = type;
  type = typeCtx->instantiate(type, parentClass)->getFunc();
  if (args.size() != type->getArgTypes().size() + 1)
    return nullptr;
  types::Type::Unification undo;
  if (type->getRetType()->unify(args[0].get(), &undo) < 0) {
    undo.undo();
    return nullptr;
  }
  for (int gi = 1; gi < args.size(); gi++) {
    undo = types::Type::Unification();
    if (type->getArgTypes()[gi - 1]->unify(args[gi].get(), &undo) < 0) {
      undo.undo();
      return nullptr;
    }
  }
  if (!generics.empty()) {
    if (generics.size() != type->funcGenerics.size())
      return nullptr;
    for (int gi = 0; gi < generics.size(); gi++) {
      undo = types::Type::Unification();
      if (type->funcGenerics[gi].type->unify(generics[gi].get(), &undo) < 0) {
        undo.undo();
        return nullptr;
      }
    }
  }
  int oldAge = typeCtx->age;
  typeCtx->age = 99999;
  auto tv = TypecheckVisitor(typeCtx);
  ir::Func *f = nullptr;
  if (auto rtv = tv.realize(type)) {
    auto pr = pendingRealizations; // copy it as it might be modified
    for (auto &fn : pr)
      TranslateVisitor(codegenCtx).transform(functions[fn.first].ast->clone());
    f = functions[rtv->getFunc()->ast->name].realizations[rtv->realizedName()]->ir;
  }
  typeCtx->age = oldAge;
  return f;
}

ir::types::Type *Cache::makeTuple(const std::vector<types::TypePtr> &types) {
  auto tv = TypecheckVisitor(typeCtx);
  auto t = typeCtx->instantiateTuple(types);
  return realizeType(t, types);
}

ir::types::Type *Cache::makeFunction(const std::vector<types::TypePtr> &types) {
  auto tv = TypecheckVisitor(typeCtx);
  seqassertn(!types.empty(), "types must have at least one argument");

  const auto &ret = types[0];
  auto argType = typeCtx->instantiateTuple(
      std::vector<types::TypePtr>(types.begin() + 1, types.end()));
  auto t = typeCtx->find("Function");
  seqassertn(t && t->type, "cannot find 'Function'");
  return realizeType(t->type->getClass(), {argType, ret});
}

ir::types::Type *Cache::makeUnion(const std::vector<types::TypePtr> &types) {
  auto tv = TypecheckVisitor(typeCtx);

  auto argType = typeCtx->instantiateTuple(types);
  auto t = typeCtx->find("Union");
  seqassertn(t && t->type, "cannot find 'Union'");
  return realizeType(t->type->getClass(), {argType});
}

void Cache::parseCode(const std::string &code) {
  auto node = ast::parseCode(this, "<internal>", code, /*startLine=*/0);
  auto sctx = imports[MAIN_IMPORT].ctx;
  node = ast::SimplifyVisitor::apply(sctx, node, "<internal>", 99999);
  node = ast::TypecheckVisitor::apply(this, node);
  ast::TranslateVisitor(codegenCtx).transform(node);
}

std::vector<ExprPtr> Cache::mergeC3(std::vector<std::vector<ExprPtr>> &seqs) {
  // Reference: https://www.python.org/download/releases/2.3/mro/
  std::vector<ExprPtr> result;
  for (size_t i = 0;; i++) {
    bool found = false;
    ExprPtr cand = nullptr;
    for (auto &seq : seqs) {
      if (seq.empty())
        continue;
      found = true;
      bool nothead = false;
      for (auto &s : seqs)
        if (!s.empty()) {
          bool in = false;
          for (size_t j = 1; j < s.size(); j++) {
            if ((in |= (seq[0]->getTypeName() == s[j]->getTypeName())))
              break;
          }
          if (in) {
            nothead = true;
            break;
          }
        }
      if (!nothead) {
        cand = seq[0];
        break;
      }
    }
    if (!found)
      return result;
    if (!cand)
      return {};
    result.push_back(clone(cand));
    for (auto &s : seqs)
      if (!s.empty() && cand->getTypeName() == s[0]->getTypeName()) {
        s.erase(s.begin());
      }
  }
  return result;
}

/**
 * Generate Python bindings for Cython-like access.
 *
 * TODO: this function is total mess. Needs refactoring.
 */
void Cache::populatePythonModule() {
  if (!pythonExt)
    return;

  LOG_USER("[py] ====== module generation =======");

#define N std::make_shared

  if (!pyModule)
    pyModule = std::make_shared<ir::PyModule>();
  using namespace ast;

  int oldAge = typeCtx->age;
  typeCtx->age = 99999;

  auto realizeIR = [&](const types::FuncTypePtr &fn,
                       const std::vector<types::TypePtr> &generics = {}) -> ir::Func * {
    auto fnType = typeCtx->instantiate(fn);
    types::Type::Unification u;
    for (size_t i = 0; i < generics.size(); i++)
      fnType->getFunc()->funcGenerics[i].type->unify(generics[i].get(), &u);
    fnType = TypecheckVisitor(typeCtx).realize(fnType);
    if (!fnType)
      return nullptr;

    auto pr = pendingRealizations; // copy it as it might be modified
    for (auto &fn : pr)
      TranslateVisitor(codegenCtx).transform(functions[fn.first].ast->clone());
    return functions[fn->ast->name].realizations[fnType->realizedName()]->ir;
  };

  const std::string pyWrap = "std.internal.python._PyWrap";
  auto clss = classes; // needs copy as below fns can mutate this
  for (const auto &[cn, c] : clss) {
    if (c.module.empty()) {
      if (!in(c.methods, "__to_py__") || !in(c.methods, "__from_py__"))
        continue;

      LOG_USER("[py] Cythonizing {}", cn);
      ir::PyType py{rev(cn), c.ast->getDocstr()};

      auto tc = typeCtx->forceFind(cn)->type;
      if (!tc->canRealize())
        compilationError(fmt::format("cannot realize '{}' for Python export", rev(cn)));
      tc = TypecheckVisitor(typeCtx).realize(tc);
      seqassertn(tc, "cannot realize '{}'", cn);

      // 1. Replace to_py / from_py with _PyWrap.wrap_to_py/from_py
      if (auto ofnn = in(c.methods, "__to_py__")) {
        auto fnn = overloads[*ofnn].begin()->name; // default first overload!
        auto &fna = functions[fnn].ast;
        fna->getFunction()->suite = N<ReturnStmt>(N<CallExpr>(
            N<IdExpr>(pyWrap + ".wrap_to_py:0"), N<IdExpr>(fna->args[0].name)));
      }
      if (auto ofnn = in(c.methods, "__from_py__")) {
        auto fnn = overloads[*ofnn].begin()->name; // default first overload!
        auto &fna = functions[fnn].ast;
        fna->getFunction()->suite =
            N<ReturnStmt>(N<CallExpr>(N<IdExpr>(pyWrap + ".wrap_from_py:0"),
                                      N<IdExpr>(fna->args[0].name), N<IdExpr>(cn)));
      }
      for (auto &n : std::vector<std::string>{"__from_py__", "__to_py__"}) {
        auto fnn = overloads[*in(c.methods, n)].begin()->name;
        ir::Func *oldIR = nullptr;
        if (!functions[fnn].realizations.empty())
          oldIR = functions[fnn].realizations.begin()->second->ir;
        functions[fnn].realizations.clear();
        auto tf = TypecheckVisitor(typeCtx).realize(functions[fnn].type);
        seqassertn(tf, "cannot re-realize '{}'", fnn);
        if (oldIR) {
          std::vector<ir::Value *> args;
          for (auto it = oldIR->arg_begin(); it != oldIR->arg_end(); ++it) {
            args.push_back(module->Nr<ir::VarValue>(*it));
          }
          ir::cast<ir::BodiedFunc>(oldIR)->setBody(ir::util::series(
              ir::util::call(functions[fnn].realizations.begin()->second->ir, args)));
        }
      }
      for (auto &[rn, r] : functions[pyWrap + ".py_type:0"].realizations) {
        if (r->type->funcGenerics[0].type->unify(tc.get(), nullptr) >= 0) {
          py.typePtrHook = r->ir;
          break;
        }
      }

      // 2. Handle methods
      auto methods = c.methods;
      for (const auto &[n, ofnn] : methods) {
        auto canonicalName = overloads[ofnn].back().name;
        if (overloads[ofnn].size() == 1 &&
            functions[canonicalName].ast->hasAttr("autogenerated"))
          continue;
        auto fna = functions[canonicalName].ast;
        bool isMethod = fna->hasAttr(Attr::Method);
        bool isProperty = fna->hasAttr(Attr::Property);

        std::string call = pyWrap + ".wrap_multiple";
        bool isMagic = false;
        if (startswith(n, "__") && endswith(n, "__")) {
          auto m = n.substr(2, n.size() - 4);
          if (m == "new" && c.ast->hasAttr(Attr::Tuple))
            m = "init";
          if (auto i = in(classes[pyWrap].methods, "wrap_magic_" + m)) {
            call = *i;
            isMagic = true;
          }
        }
        if (isProperty)
          call = pyWrap + ".wrap_get";

        auto fnName = call + ":0";
        seqassertn(in(functions, fnName), "bad name");
        auto generics = std::vector<types::TypePtr>{tc};
        if (isProperty) {
          generics.push_back(
              std::make_shared<types::StaticType>(this, rev(canonicalName)));
        } else if (!isMagic) {
          generics.push_back(std::make_shared<types::StaticType>(this, n));
          generics.push_back(std::make_shared<types::StaticType>(this, (int)isMethod));
        }
        auto f = realizeIR(functions[fnName].type, generics);
        if (!f)
          continue;

        LOG_USER("[py] {} -> {} ({}; {})", n, call, isMethod, isProperty);
        if (isProperty) {
          py.getset.push_back({rev(canonicalName), "", f, nullptr});
        } else if (n == "__repr__") {
          py.repr = f;
        } else if (n == "__add__") {
          py.add = f;
        } else if (n == "__iadd__") {
          py.iadd = f;
        } else if (n == "__sub__") {
          py.sub = f;
        } else if (n == "__isub__") {
          py.isub = f;
        } else if (n == "__mul__") {
          py.mul = f;
        } else if (n == "__imul__") {
          py.imul = f;
        } else if (n == "__mod__") {
          py.mod = f;
        } else if (n == "__imod__") {
          py.imod = f;
        } else if (n == "__divmod__") {
          py.divmod = f;
        } else if (n == "__pow__") {
          py.pow = f;
        } else if (n == "__ipow__") {
          py.ipow = f;
        } else if (n == "__neg__") {
          py.neg = f;
        } else if (n == "__pos__") {
          py.pos = f;
        } else if (n == "__abs__") {
          py.abs = f;
        } else if (n == "__bool__") {
          py.bool_ = f;
        } else if (n == "__invert__") {
          py.invert = f;
        } else if (n == "__lshift__") {
          py.lshift = f;
        } else if (n == "__ilshift__") {
          py.ilshift = f;
        } else if (n == "__rshift__") {
          py.rshift = f;
        } else if (n == "__irshift__") {
          py.irshift = f;
        } else if (n == "__and__") {
          py.and_ = f;
        } else if (n == "__iand__") {
          py.iand = f;
        } else if (n == "__xor__") {
          py.xor_ = f;
        } else if (n == "__ixor__") {
          py.ixor = f;
        } else if (n == "__or__") {
          py.or_ = f;
        } else if (n == "__ior__") {
          py.ior = f;
        } else if (n == "__int__") {
          py.int_ = f;
        } else if (n == "__float__") {
          py.float_ = f;
        } else if (n == "__floordiv__") {
          py.floordiv = f;
        } else if (n == "__ifloordiv__") {
          py.ifloordiv = f;
        } else if (n == "__truediv__") {
          py.truediv = f;
        } else if (n == "__itruediv__") {
          py.itruediv = f;
        } else if (n == "__index__") {
          py.index = f;
        } else if (n == "__matmul__") {
          py.matmul = f;
        } else if (n == "__imatmul__") {
          py.imatmul = f;
        } else if (n == "__len__") {
          py.len = f;
        } else if (n == "__getitem__") {
          py.getitem = f;
        } else if (n == "__setitem__") {
          py.setitem = f;
        } else if (n == "__contains__") {
          py.contains = f;
        } else if (n == "__hash__") {
          py.hash = f;
        } else if (n == "__call__") {
          py.call = f;
        } else if (n == "__str__") {
          py.str = f;
        } else if (n == "__iter__") {
          py.iter = f;
        } else if (n == "__del__") {
          py.del = f;
        } else if (n == "__init__" || (c.ast->hasAttr(Attr::Tuple) && n == "__new__")) {
          py.init = f;
        } else {
          py.methods.push_back(ir::PyFunction{
              n, fna->getDocstr(), f,
              fna->hasAttr(Attr::Method) ? ir::PyFunction::Type::METHOD
                                         : ir::PyFunction::Type::CLASS,
              // always use FASTCALL for now; works even for 0- or 1- arg methods
              2});
          py.methods.back().keywords = true;
        }
      }

      for (auto &m : py.methods) {
        if (in(std::set<std::string>{"__lt__", "__le__", "__eq__", "__ne__", "__gt__",
                                     "__ge__"},
               m.name)) {
          py.cmp = realizeIR(
              typeCtx->forceFind(pyWrap + ".wrap_cmp:0")->type->getFunc(), {tc});
          break;
        }
      }

      if (c.realizations.size() != 1)
        compilationError(fmt::format("cannot pythonize generic class '{}'", cn));
      auto &r = c.realizations.begin()->second;
      py.type = realizeType(r->type);
      for (auto &[mn, mt] : r->fields) {
        /// TODO: handle PyMember for tuples
        // Generate getters & setters
        auto generics = std::vector<types::TypePtr>{
            tc, std::make_shared<types::StaticType>(this, mn)};
        auto gf = realizeIR(functions[pyWrap + ".wrap_get:0"].type, generics);
        ir::Func *sf = nullptr;
        if (!c.ast->hasAttr(Attr::Tuple))
          sf = realizeIR(functions[pyWrap + ".wrap_set:0"].type, generics);
        py.getset.push_back({mn, "", gf, sf});
        LOG_USER("[py] {}: {} . {}", "member", cn, mn);
      }
      pyModule->types.push_back(py);
    }
  }

  // Handle __iternext__ wrappers
  auto cin = "_PyWrap.IterWrap";
  for (auto &[cn, cr] : classes[cin].realizations) {
    LOG_USER("[py] iterfn: {}", cn);
    ir::PyType py{cn, ""};
    auto tc = cr->type;
    for (auto &[rn, r] : functions[pyWrap + ".py_type:0"].realizations) {
      if (r->type->funcGenerics[0].type->unify(tc.get(), nullptr) >= 0) {
        py.typePtrHook = r->ir;
        break;
      }
    }

    auto &methods = classes[cin].methods;
    for (auto &n : std::vector<std::string>{"_iter", "_iternext"}) {
      auto fnn = overloads[methods[n]].begin()->name;
      auto &fna = functions[fnn];
      auto ft = typeCtx->instantiate(fna.type, tc->getClass());
      auto rtv = TypecheckVisitor(typeCtx).realize(ft);
      auto f =
          functions[rtv->getFunc()->ast->name].realizations[rtv->realizedName()]->ir;
      if (n == "_iter")
        py.iter = f;
      else
        py.iternext = f;
    }
    py.type = cr->ir;
    pyModule->types.push_back(py);
  }
#undef N

  auto fns = functions; // needs copy as below fns can mutate this
  for (const auto &[fn, f] : fns) {
    if (f.isToplevel) {
      std::string call = pyWrap + ".wrap_multiple";
      auto fnName = call + ":0";
      seqassertn(in(functions, fnName), "bad name");
      auto generics = std::vector<types::TypePtr>{
          typeCtx->forceFind(".toplevel")->type,
          std::make_shared<types::StaticType>(this, rev(f.ast->name)),
          std::make_shared<types::StaticType>(this, 0)};
      if (auto ir = realizeIR(functions[fnName].type, generics)) {
        LOG_USER("[py] {}: {}", "toplevel", fn);
        pyModule->functions.push_back(ir::PyFunction{rev(fn), f.ast->getDocstr(), ir,
                                                     ir::PyFunction::Type::TOPLEVEL,
                                                     int(f.ast->args.size())});
        pyModule->functions.back().keywords = true;
      }
    }
  }

  // Handle pending realizations!
  auto pr = pendingRealizations; // copy it as it might be modified
  for (auto &fn : pr)
    TranslateVisitor(codegenCtx).transform(functions[fn.first].ast->clone());
  typeCtx->age = oldAge;
}

} // namespace codon::ast
