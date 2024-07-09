// Copyright (C) 2022-2024 Exaloop Inc. <https://exaloop.io>

#include "doc.h"

#include <memory>
#include <string>
#include <tuple>
#include <vector>

#include "codon/parser/ast.h"
#include "codon/parser/common.h"
#include "codon/parser/peg/peg.h"
#include "codon/parser/visitors/format/format.h"

using fmt::format;

namespace codon::ast {

// clang-format off
std::string json_escape(const std::string &str) {
  std::string r;
  r.reserve(str.size());
  for (unsigned char c : str) {
    switch (c) {
    case '\b': r += "\\b"; break;
    case '\f': r += "\\f"; break;
    case '\n': r += "\\n"; break;
    case '\r': r += "\\r"; break;
    case '\t': r += "\\t"; break;
    case '\\': r += "\\\\"; break;
    case '"': r += "\\\""; break;
    default: r += c;
    }
  }
  return r;
}
// clang-format on

json::json() : list(false) {}
json::json(const std::string &s) : list(false) { values[s] = nullptr; }
json::json(const std::string &s, const std::string &v) : list(false) {
  values[s] = std::make_shared<json>(v);
}
json::json(const std::vector<std::shared_ptr<json>> &vs) : list(true) {
  for (int i = 0; i < vs.size(); i++)
    values[std::to_string(i)] = vs[i];
}
json::json(const std::vector<std::string> &vs) : list(true) {
  for (int i = 0; i < vs.size(); i++)
    values[std::to_string(i)] = std::make_shared<json>(vs[i]);
}
json::json(const std::unordered_map<std::string, std::string> &vs) : list(false) {
  for (auto &v : vs)
    values[v.first] = std::make_shared<json>(v.second);
}

std::string json::toString() {
  std::vector<std::string> s;
  if (values.empty()) {
    return "{}";
  } else if (values.size() == 1 && !values.begin()->second) {
    return fmt::format("\"{}\"", json_escape(values.begin()->first));
  } else if (list) {
    for (int i = 0; i < values.size(); i++)
      s.push_back(values[std::to_string(i)]->toString());
    return fmt::format("[ {} ]", join(s, ", "));
  } else {
    for (auto &v : values)
      s.push_back(
          fmt::format("\"{}\": {}", json_escape(v.first), v.second->toString()));
    return fmt::format("{{ {} }}", join(s, ", "));
  }
}

std::shared_ptr<json> json::get(const std::string &s) {
  auto i = values.find(s);
  seqassertn(i != values.end(), "cannot find {}", s);
  return i->second;
}

std::shared_ptr<json> json::set(const std::string &s, const std::string &value) {
  return values[s] = std::make_shared<json>(value);
}
std::shared_ptr<json> json::set(const std::string &s,
                                const std::shared_ptr<json> &value) {
  return values[s] = value;
}

std::shared_ptr<json> DocVisitor::apply(const std::string &argv0,
                                        const std::vector<std::string> &files) {
  auto shared = std::make_shared<DocShared>();
  shared->argv0 = argv0;
  auto cache = std::make_unique<ast::Cache>(argv0);
  shared->cache = cache.get();
  shared->modules[""] = std::make_shared<DocContext>(shared);
  shared->j = std::make_shared<json>();

  auto stdlib = getImportFile(argv0, STDLIB_INTERNAL_MODULE, "", true, "");
  auto ast = ast::parseFile(shared->cache, stdlib->path);
  auto core =
      ast::parseCode(shared->cache, stdlib->path, "from internal.core import *");
  shared->modules[""]->setFilename(stdlib->path);
  shared->modules[""]->add("__py_numerics__", std::make_shared<int>(shared->itemID++));
  shared->modules[""]->add("__py_extension__", std::make_shared<int>(shared->itemID++));
  shared->modules[""]->add("__debug__", std::make_shared<int>(shared->itemID++));
  shared->modules[""]->add("__apple__", std::make_shared<int>(shared->itemID++));
  DocVisitor(shared->modules[""]).transformModule(std::move(core));
  DocVisitor(shared->modules[""]).transformModule(std::move(ast));

  auto ctx = std::make_shared<DocContext>(shared);
  for (auto &f : files) {
    auto path = getAbsolutePath(f);
    ctx->setFilename(path);
    LOG("-> parsing {}", path);
    auto ast = ast::parseFile(shared->cache, path);
    DocVisitor(ctx).transformModule(std::move(ast));
  }

  shared->cache = nullptr;
  return shared->j;
}

std::shared_ptr<int> DocContext::find(const std::string &s) const {
  auto i = Context<int>::find(s);
  if (!i && this != shared->modules[""].get())
    return shared->modules[""]->find(s);
  return i;
}

std::string getDocstr(const StmtPtr &s) {
  if (auto se = s->getExpr())
    if (auto e = se->expr->getString())
      return e->getValue();
  return "";
}

std::vector<StmtPtr> DocVisitor::flatten(StmtPtr stmt, std::string *docstr, bool deep) {
  std::vector<StmtPtr> stmts;
  if (auto s = stmt->getSuite()) {
    for (int i = 0; i < (deep ? s->stmts.size() : 1); i++) {
      for (auto &x : flatten(std::move(s->stmts[i]), i ? nullptr : docstr, deep))
        stmts.push_back(std::move(x));
    }
  } else {
    if (docstr)
      *docstr = getDocstr(stmt);
    stmts.push_back(std::move(stmt));
  }
  return stmts;
}

std::shared_ptr<json> DocVisitor::transform(const ExprPtr &expr) {
  if (!expr)
    return std::make_shared<json>();
  DocVisitor v(ctx);
  v.setSrcInfo(expr->getSrcInfo());
  v.resultExpr = std::make_shared<json>();
  expr->accept(v);
  return v.resultExpr;
}

std::string DocVisitor::transform(const StmtPtr &stmt) {
  if (!stmt)
    return "";
  DocVisitor v(ctx);
  v.setSrcInfo(stmt->getSrcInfo());
  stmt->accept(v);
  return v.resultStmt;
}

void DocVisitor::transformModule(StmtPtr stmt) {
  std::vector<std::string> children;
  std::string docstr;

  auto flat = flatten(std::move(stmt), &docstr);
  for (int i = 0; i < flat.size(); i++) {
    auto &s = flat[i];
    auto id = transform(s);
    if (id.empty())
      continue;
    if (i < (flat.size() - 1) && CAST(s, AssignStmt)) {
      auto ds = getDocstr(flat[i + 1]);
      if (!ds.empty())
        ctx->shared->j->get(id)->set("doc", ds);
    }
    children.push_back(id);
  }

  auto id = std::to_string(ctx->shared->itemID++);
  auto ja = ctx->shared->j->set(
      id, std::make_shared<json>(std::unordered_map<std::string, std::string>{
              {"kind", "module"}, {"path", ctx->getFilename()}}));
  ja->set("children", std::make_shared<json>(children));
  if (!docstr.empty())
    ja->set("doc", docstr);
}

void DocVisitor::visit(IntExpr *expr) {
  resultExpr = std::make_shared<json>(expr->value);
}

void DocVisitor::visit(IdExpr *expr) {
  auto i = ctx->find(expr->value);
  if (!i)
    error("unknown identifier {}", expr->value);
  resultExpr = std::make_shared<json>(*i ? std::to_string(*i) : expr->value);
}

void DocVisitor::visit(IndexExpr *expr) {
  std::vector<std::shared_ptr<json>> v;
  v.push_back(transform(expr->expr));
  if (auto tp = CAST(expr->index, TupleExpr)) {
    if (auto l = tp->items[0]->getList()) {
      for (auto &e : l->items)
        v.push_back(transform(e));
      v.push_back(transform(tp->items[1]));
    } else
      for (auto &e : tp->items)
        v.push_back(transform(e));
  } else {
    v.push_back(transform(expr->index));
  }
  resultExpr = std::make_shared<json>(v);
}

bool isValidName(const std::string &s) {
  if (s.empty())
    return false;
  if (s.size() > 4 && s.substr(0, 2) == "__" && s.substr(s.size() - 2) == "__")
    return true;
  return s[0] != '_';
}

void DocVisitor::visit(FunctionStmt *stmt) {
  int id = ctx->shared->itemID++;
  ctx->add(stmt->name, std::make_shared<int>(id));
  auto j = std::make_shared<json>(std::unordered_map<std::string, std::string>{
      {"kind", "function"}, {"name", stmt->name}});
  j->set("pos", jsonify(stmt->getSrcInfo()));

  std::vector<std::shared_ptr<json>> args;
  std::vector<std::string> generics;
  for (auto &a : stmt->args)
    if (a.status != Param::Normal) {
      ctx->add(a.name, std::make_shared<int>(0));
      generics.push_back(a.name);
      a.status = Param::Generic;
    }
  for (auto &a : stmt->args)
    if (a.status == Param::Normal) {
      auto j = std::make_shared<json>();
      j->set("name", a.name);
      if (a.type)
        j->set("type", transform(a.type));
      if (a.defaultValue) {
        j->set("default", FormatVisitor::apply(a.defaultValue));
      }
      args.push_back(j);
    }
  j->set("generics", std::make_shared<json>(generics));
  bool isLLVM = false;
  for (auto &d : stmt->decorators)
    if (auto e = d->getId()) {
      j->set("attrs", std::make_shared<json>(e->value, ""));
      isLLVM |= (e->value == "llvm");
    }
  if (stmt->ret)
    j->set("return", transform(stmt->ret));
  j->set("args", std::make_shared<json>(args));
  std::string docstr;
  flatten(std::move(stmt->suite), &docstr);
  for (auto &g : generics)
    ctx->remove(g);
  if (!docstr.empty() && !isLLVM)
    j->set("doc", docstr);
  ctx->shared->j->set(std::to_string(id), j);
  resultStmt = std::to_string(id);
}

void DocVisitor::visit(ClassStmt *stmt) {
  std::vector<std::string> generics;
  auto j = std::make_shared<json>(std::unordered_map<std::string, std::string>{
      {"name", stmt->name},
      {"kind", "class"},
      {"type", stmt->isRecord() ? "type" : "class"}});
  int id = ctx->shared->itemID++;

  bool isExtend = false;
  for (auto &d : stmt->decorators)
    if (auto e = d->getId())
      isExtend |= (e->value == "extend");

  if (isExtend) {
    j->set("type", "extension");
    auto i = ctx->find(stmt->name);
    j->set("parent", std::to_string(*i));
    generics = ctx->shared->generics[*i];
  } else {
    ctx->add(stmt->name, std::make_shared<int>(id));
  }

  std::vector<std::shared_ptr<json>> args;
  for (auto &a : stmt->args)
    if (a.status != Param::Normal) {
      a.status = Param::Generic;
      generics.push_back(a.name);
    }
  ctx->shared->generics[id] = generics;
  for (auto &g : generics)
    ctx->add(g, std::make_shared<int>(0));
  for (auto &a : stmt->args)
    if (a.status == Param::Normal) {
      auto ja = std::make_shared<json>();
      ja->set("name", a.name);
      if (a.type)
        ja->set("type", transform(a.type));
      args.push_back(ja);
    }
  j->set("generics", std::make_shared<json>(generics));
  j->set("args", std::make_shared<json>(args));
  j->set("pos", jsonify(stmt->getSrcInfo()));

  std::string docstr;
  std::vector<std::string> members;
  for (auto &f : flatten(std::move(stmt->suite), &docstr)) {
    if (auto ff = CAST(f, FunctionStmt)) {
      auto i = transform(f);
      if (i != "")
        members.push_back(i);
      if (isValidName(ff->name))
        ctx->remove(ff->name);
    }
  }
  for (auto &g : generics)
    ctx->remove(g);
  j->set("members", std::make_shared<json>(members));
  if (!docstr.empty())
    j->set("doc", docstr);
  ctx->shared->j->set(std::to_string(id), j);
  resultStmt = std::to_string(id);
}

std::shared_ptr<json> DocVisitor::jsonify(const codon::SrcInfo &s) {
  return std::make_shared<json>(
      std::vector<std::string>{std::to_string(s.line), std::to_string(s.len)});
}

void DocVisitor::visit(ImportStmt *stmt) {
  if (stmt->from && (stmt->from->isId("C") || stmt->from->isId("python"))) {
    int id = ctx->shared->itemID++;
    std::string name, lib;
    if (auto i = stmt->what->getId())
      name = i->value;
    else if (auto d = stmt->what->getDot())
      name = d->member, lib = FormatVisitor::apply(d->expr);
    else
      seqassert(false, "invalid C import statement");
    ctx->add(name, std::make_shared<int>(id));
    name = stmt->as.empty() ? name : stmt->as;

    auto j = std::make_shared<json>(std::unordered_map<std::string, std::string>{
        {"name", name}, {"kind", "function"}, {"extern", stmt->from->getId()->value}});
    j->set("pos", jsonify(stmt->getSrcInfo()));
    std::vector<std::shared_ptr<json>> args;
    if (stmt->ret)
      j->set("return", transform(stmt->ret));
    for (auto &a : stmt->args) {
      auto ja = std::make_shared<json>();
      ja->set("name", a.name);
      ja->set("type", transform(a.type));
      args.push_back(ja);
    }
    j->set("dylib", lib);
    j->set("args", std::make_shared<json>(args));
    ctx->shared->j->set(std::to_string(id), j);
    resultStmt = std::to_string(id);
    return;
  }

  std::vector<std::string> dirs; // Path components
  Expr *e = stmt->from.get();
  if (e) {
    while (auto d = e->getDot()) {
      dirs.push_back(d->member);
      e = d->expr.get();
    }
    if (!e->getId() || !stmt->args.empty() || stmt->ret ||
        (stmt->what && !stmt->what->getId()))
      error("invalid import statement");
    // We have an empty stmt->from in "from .. import".
    if (!e->getId()->value.empty())
      dirs.push_back(e->getId()->value);
  }
  // Handle dots (e.g. .. in from ..m import x).
  seqassert(stmt->dots >= 0, "negative dots in ImportStmt");
  for (size_t i = 1; i < stmt->dots; i++)
    dirs.emplace_back("..");
  std::string path;
  for (int i = int(dirs.size()) - 1; i >= 0; i--)
    path += dirs[i] + (i ? "/" : "");
  // Fetch the import!
  auto file = getImportFile(ctx->shared->argv0, path, ctx->getFilename());
  if (!file)
    error(stmt, "cannot locate import '{}'", path);

  auto ictx = ctx;
  auto it = ctx->shared->modules.find(file->path);
  if (it == ctx->shared->modules.end()) {
    ctx->shared->modules[file->path] = ictx = std::make_shared<DocContext>(ctx->shared);
    ictx->setFilename(file->path);
    LOG("=> parsing {}", file->path);
    auto tmp = parseFile(ctx->shared->cache, file->path);
    DocVisitor(ictx).transformModule(std::move(tmp));
  } else {
    ictx = it->second;
  }

  if (!stmt->what) {
    // TODO: implement this corner case
    for (auto &i : dirs)
      if (!ctx->find(i))
        ctx->add(i, std::make_shared<int>(ctx->shared->itemID++));
  } else if (stmt->what->isId("*")) {
    for (auto &i : *ictx)
      ctx->add(i.first, i.second.front());
  } else {
    auto i = stmt->what->getId();
    if (auto c = ictx->find(i->value))
      ctx->add(stmt->as.empty() ? i->value : stmt->as, c);
    else
      error(stmt, "symbol '{}' not found in {}", i->value, file->path);
  }
}

void DocVisitor::visit(AssignStmt *stmt) {
  auto e = CAST(stmt->lhs, IdExpr);
  if (!e)
    return;
  int id = ctx->shared->itemID++;
  ctx->add(e->value, std::make_shared<int>(id));
  auto j = std::make_shared<json>(std::unordered_map<std::string, std::string>{
      {"name", e->value}, {"kind", "variable"}});
  j->set("pos", jsonify(stmt->getSrcInfo()));
  ctx->shared->j->set(std::to_string(id), j);
  resultStmt = std::to_string(id);
}

} // namespace codon::ast
