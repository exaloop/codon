#include <string>
#include <tuple>

#include "codon/parser/ast.h"
#include "codon/parser/cache.h"
#include "codon/parser/common.h"
#include "codon/parser/visitors/simplify/simplify.h"
#include "codon/parser/visitors/typecheck/typecheck.h"

using fmt::format;

namespace codon::ast {

using namespace types;

void TypecheckVisitor::visit(IdExpr *expr) {
  if (startswith(expr->value, TYPE_TUPLE)) {
    generateTupleStub(std::stoi(expr->value.substr(7)));
  } else if (expr->value == TYPE_CALLABLE) { // Empty Callable references
    auto typ = ctx->addUnbound(expr, ctx->typecheckLevel);
    typ->getLink()->trait = std::make_shared<CallableTrait>(std::vector<TypePtr>{});
    unify(expr->type, typ);
    expr->markType();
    return;
  }
  while (auto s = in(ctx->cache->replacements, expr->value))
    expr->value = s->first;
  auto val = ctx->find(expr->value);
  if (!val) {
    auto j = ctx->cache->globals.find(expr->value);
    if (j != ctx->cache->globals.end()) {
      auto typ = ctx->addUnbound(expr, ctx->typecheckLevel);
      unify(expr->type, typ);
      return;
    }
    auto i = ctx->cache->overloads.find(expr->value);
    if (i != ctx->cache->overloads.end()) {
      if (i->second.size() == 1) {
        val = ctx->find(i->second[0].name);
      } else {
        auto d = findDispatch(expr->value);
        val = ctx->find(d->ast->name);
      }
    }
  }
  seqassert(val, "cannot find IdExpr '{}' ({})", expr->value, expr->getSrcInfo());

  auto t = ctx->instantiate(expr, val->type);
  expr->type = unify(expr->type, t);
  if (val->type->isStaticType()) {
    expr->staticValue.type = StaticValue::Type(val->type->isStaticType());
    auto s = val->type->getStatic();
    seqassert(!expr->staticValue.evaluated, "expected unevaluated expression: {}",
              expr->toString());
    if (s && s->expr->staticValue.evaluated) {
      if (s->expr->staticValue.type == StaticValue::STRING)
        resultExpr = transform(N<StringExpr>(s->expr->staticValue.getString()));
      else
        resultExpr = transform(N<IntExpr>(s->expr->staticValue.getInt()));
    }
    return;
  } else if (val->isType()) {
    expr->markType();
  }

  // Check if we can realize the type.
  if (auto rt = realize(expr->type)) {
    unify(expr->type, rt);
    if (val->kind == TypecheckItem::Type || val->kind == TypecheckItem::Func)
      expr->value = expr->type->realizedName();
    expr->done = true;
  } else {
    expr->done = false;
  }
}

void TypecheckVisitor::visit(DotExpr *expr) { resultExpr = transformDot(expr); }

std::string TypecheckVisitor::generateTupleStub(int len, const std::string &name,
                                                std::vector<std::string> names,
                                                bool hasSuffix) {
  static std::map<std::string, int> usedNames;
  auto key = join(names, ";");
  std::string suffix;
  if (!names.empty()) {
    if (!in(usedNames, key))
      usedNames[key] = usedNames.size();
    suffix = format("_{}", usedNames[key]);
  } else {
    for (int i = 1; i <= len; i++)
      names.push_back(format("item{}", i));
  }
  auto typeName = format("{}{}", name, hasSuffix ? format(".N{}{}", len, suffix) : "");
  if (!ctx->find(typeName)) {
    std::vector<Param> args;
    for (int i = 1; i <= len; i++)
      args.emplace_back(Param(names[i - 1], N<IdExpr>(format("T{}", i)), nullptr));
    for (int i = 1; i <= len; i++)
      args.emplace_back(Param(format("T{}", i), N<IdExpr>("type"), nullptr, true));
    StmtPtr stmt = std::make_shared<ClassStmt>(
        typeName, args, nullptr, std::vector<ExprPtr>{N<IdExpr>("tuple")});
    stmt->setSrcInfo(ctx->cache->generateSrcInfo());

    stmt = SimplifyVisitor::apply(ctx->cache->imports[STDLIB_IMPORT].ctx, stmt,
                                  FILE_GENERATED, 0);
    stmt = TypecheckVisitor(ctx).transform(stmt);
    prependStmts->push_back(stmt);
  }
  return typeName;
}


ExprPtr TypecheckVisitor::transformDot(DotExpr *expr,
                                       std::vector<CallExpr::Arg> *args) {
  if (expr->member == "__class__") {
    expr->expr = transform(expr->expr, true, true, true);
    unify(expr->type, ctx->findInternal("str"));
    if (auto f = expr->expr->type->getFunc())
      return transform(N<StringExpr>(f->toString()));
    if (auto t = realize(expr->expr->type))
      return transform(N<StringExpr>(t->toString()));
    expr->done = false;
    return nullptr;
  }
  expr->expr = transform(expr->expr, true);

  // Case 1: type not yet known, so just assign an unbound type and wait until the
  // next iteration.
  if (expr->expr->getType()->getUnbound()) {
    unify(expr->type, ctx->addUnbound(expr, ctx->typecheckLevel));
    return nullptr;
  }

  auto typ = expr->expr->getType()->getClass();
  seqassert(typ, "expected formed type: {}", typ->toString());

  auto methods = ctx->findMethod(typ->name, expr->member);
  if (methods.empty()) {
    auto findGeneric = [this](ClassType *c, const std::string &m) -> TypePtr {
      for (auto &g : c->generics) {
        if (ctx->cache->reverseIdentifierLookup[g.name] == m)
          return g.type;
      }
      return nullptr;
    };
    if (auto member = ctx->findMember(typ->name, expr->member)) {
      // Case 2(a): Object member access.
      auto t = ctx->instantiate(expr, member, typ.get());
      unify(expr->type, t);
      expr->done = expr->expr->done && realize(expr->type) != nullptr;
      return nullptr;
    } else if (auto t = findGeneric(typ.get(), expr->member)) {
      // Case 2(b): Object generic access.
      unify(expr->type, t);
      if (!t->isStaticType())
        expr->markType();
      else
        expr->staticValue.type = StaticValue::Type(t->isStaticType());
      if (auto rt = realize(expr->type)) {
        unify(expr->type, rt);
        ExprPtr e;
        if (!t->isStaticType())
          e = N<IdExpr>(t->realizedName());
        else if (t->getStatic()->expr->staticValue.type == StaticValue::STRING)
          e = transform(N<StringExpr>(t->getStatic()->expr->staticValue.getString()));
        else
          e = transform(N<IntExpr>(t->getStatic()->expr->staticValue.getInt()));
        return transform(e);
      }
      return nullptr;
    } else if (typ->name == TYPE_OPTIONAL) {
      // Case 3: Transform optional.member to unwrap(optional).member.
      auto d = N<DotExpr>(transform(N<CallExpr>(N<IdExpr>(FN_UNWRAP), expr->expr)),
                          expr->member);
      if (auto dd = transformDot(d.get(), args))
        return dd;
      return d;
    } else if (typ->name == "pyobj") {
      // Case 4: Transform pyobj.member to pyobj._getattr("member").
      return transform(
          N<CallExpr>(N<DotExpr>(expr->expr, "_getattr"), N<StringExpr>(expr->member)));
    } else {
      // For debugging purposes:
      ctx->findMethod(typ->name, expr->member);
      error("cannot find '{}' in {}", expr->member, typ->toString());
    }
  }

  // Case 5: look for a method that best matches the given arguments.
  //         If it exists, return a simple IdExpr with that method's name.
  //         Append a "self" variable to the front if needed.
  if (args) {
    std::vector<CallExpr::Arg> argTypes;
    bool isType = expr->expr->isType();
    if (!isType) {
      ExprPtr expr = N<IdExpr>("self");
      expr->setType(typ);
      argTypes.emplace_back(CallExpr::Arg{"", expr});
    }
    for (const auto &a : *args)
      argTypes.emplace_back(a);
    if (auto bestMethod = findBestMethod(expr->expr.get(), expr->member, argTypes)) {
      ExprPtr e = N<IdExpr>(bestMethod->ast->name);
      auto t = ctx->instantiate(expr, bestMethod, typ.get());
      unify(e->type, t);
      unify(expr->type, e->type);
      if (!isType)
        args->insert(args->begin(), {"", expr->expr}); // self variable
      e = transform(e); // Visit IdExpr and realize it if necessary.
      return e;
    }
    // No method was found, print a nice error message.
    std::vector<std::string> nice;
    for (auto &t : argTypes)
      nice.emplace_back(format("{} = {}", t.name, t.value->type->toString()));
    findBestMethod(expr->expr.get(), expr->member, argTypes);
    error("cannot find a method '{}' in {} with arguments {}", expr->member,
          typ->toString(), join(nice, ", "));
  }

  // Case 6: multiple overloaded methods available.
  FuncTypePtr bestMethod = nullptr;
  auto oldType = expr->getType() ? expr->getType()->getClass() : nullptr;
  if (methods.size() > 1 && oldType && oldType->getFunc()) {
    // If old type is already a function, use its arguments to pick the best call.
    std::vector<TypePtr> methodArgs;
    if (!expr->expr->isType()) // self argument
      methodArgs.emplace_back(typ);
    for (auto &a : oldType->getFunc()->getArgTypes())
      methodArgs.emplace_back(a);
    bestMethod = findBestMethod(expr->expr.get(), expr->member, methodArgs);
    if (!bestMethod) {
      // Print a nice error message.
      std::vector<std::string> nice;
      for (auto &t : methodArgs)
        nice.emplace_back(format("{}", t->toString()));
      error("cannot find a method '{}' in {} with arguments {}", expr->member,
            typ->toString(), join(nice, ", "));
    }
  } else if (methods.size() > 1) {
    auto m = ctx->cache->classes.find(typ->name);
    auto t = m->second.methods.find(expr->member);
    bestMethod = findDispatch(t->second);
  } else {
    bestMethod = methods[0];
  }

  // Case 7: only one valid method remaining. Check if this is a class method or an
  // object method access and transform accordingly.
  if (expr->expr->isType()) {
    // Class method access: Type.method.
    auto name = bestMethod->ast->name;
    auto val = ctx->find(name);
    seqassert(val, "cannot find method '{}'", name);
    ExprPtr e = N<IdExpr>(name);
    auto t = ctx->instantiate(expr, bestMethod, typ.get());
    unify(e->type, t);
    unify(expr->type, e->type);
    e = transform(e); // Visit IdExpr and realize it if necessary.
    return e;
  } else {
    // Object access: y.method. Transform y.method to a partial call
    // typeof(t).foo(y, ...).
    std::vector<ExprPtr> methodArgs{expr->expr};
    methodArgs.push_back(N<EllipsisExpr>());
    // Handle @property methods.
    if (bestMethod->ast->attributes.has(Attr::Property))
      methodArgs.pop_back();
    ExprPtr e = N<CallExpr>(N<IdExpr>(bestMethod->ast->name), methodArgs);
    auto ex = transform(e, false, allowVoidExpr);
    return ex;
  }
}

}