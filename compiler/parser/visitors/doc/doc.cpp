/*
 * doc.cpp --- Seq documentation generator.
 *
 * (c) Seq project. All rights reserved.
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 */

#include <memory>
#include <string>
#include <tuple>
#include <vector>

#include "parser/ast.h"
#include "parser/common.h"
#include "parser/peg/peg.h"
#include "parser/visitors/doc/doc.h"
#include "parser/visitors/format/format.h"

using fmt::format;
using std::get;
using std::move;
using std::ostream;
using std::stack;
using std::to_string;

namespace seq {
namespace ast {

// clang-format off
string json_escape(const string &str) {
  string r;
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
json::json(const string &s) : list(false) { values[s] = nullptr; }
json::json(const string &s, const string &v) : list(false) {
  values[s] = make_shared<json>(v);
}
json::json(const vector<shared_ptr<json>> &vs) : list(true) {
  for (int i = 0; i < vs.size(); i++)
    values[std::to_string(i)] = vs[i];
}
json::json(const vector<string> &vs) : list(true) {
  for (int i = 0; i < vs.size(); i++)
    values[std::to_string(i)] = make_shared<json>(vs[i]);
}
json::json(const unordered_map<string, string> &vs) : list(false) {
  for (auto &v : vs)
    values[v.first] = make_shared<json>(v.second);
}

string json::toString() {
  vector<string> s;
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

shared_ptr<json> json::get(const string &s) {
  auto i = values.find(s);
  seqassert(i != values.end(), "cannot find {}", s);
  return i->second;
}

shared_ptr<json> json::set(const string &s, const string &value) {
  return values[s] = make_shared<json>(value);
}
shared_ptr<json> json::set(const string &s, const shared_ptr<json> &value) {
  return values[s] = value;
}

shared_ptr<json> DocVisitor::apply(const string &argv0, const vector<string> &files) {
  auto shared = make_shared<DocShared>();
  shared->argv0 = argv0;
  shared->cache = make_shared<ast::Cache>(argv0);

  auto stdlib = getImportFile(argv0, "internal", "", true, "");
  auto ast = ast::parseFile(shared->cache, stdlib->path);
  shared->modules[""] = make_shared<DocContext>(shared);
  shared->modules[""]->setFilename(stdlib->path);
  shared->j = make_shared<json>();
  for (auto &s :
       vector<string>{"void", "byte", "float", "bool", "int", "str", "pyobj", "Ptr",
                      "Function", "Generator", "Tuple", "Int", "UInt", TYPE_OPTIONAL,
                      "Callable", "NoneType", "__internal__"}) {
    shared->j->set(to_string(shared->itemID),
                   make_shared<json>(unordered_map<string, string>{
                       {"kind", "class"}, {"name", s}, {"type", "type"}}));
    if (s == "Ptr" || s == "Generator" || s == TYPE_OPTIONAL)
      shared->generics[shared->itemID] = {"T"};
    if (s == "Int" || s == "UInt")
      shared->generics[shared->itemID] = {"N"};
    shared->modules[""]->add(s, make_shared<int>(shared->itemID++));
  }

  DocVisitor(shared->modules[""]).transformModule(move(ast));
  auto ctx = make_shared<DocContext>(shared);

  char abs[PATH_MAX];
  for (auto &f : files) {
    realpath(f.c_str(), abs);
    ctx->setFilename(abs);
    ast = ast::parseFile(shared->cache, abs);
    // LOG("parsing {}", f);
    DocVisitor(ctx).transformModule(move(ast));
  }

  return shared->j;
}

shared_ptr<int> DocContext::find(const string &s) const {
  auto i = Context<int>::find(s);
  if (!i && this != shared->modules[""].get())
    return shared->modules[""]->find(s);
  return i;
}

string getDocstr(const StmtPtr &s) {
  if (auto se = s->getExpr())
    if (auto e = se->expr->getString())
      return e->getValue();
  return "";
}

vector<StmtPtr> DocVisitor::flatten(StmtPtr stmt, string *docstr, bool deep) {
  vector<StmtPtr> stmts;
  if (auto s = const_cast<SuiteStmt *>(stmt->getSuite())) {
    for (int i = 0; i < (deep ? s->stmts.size() : 1); i++) {
      for (auto &x : flatten(move(s->stmts[i]), i ? nullptr : docstr, deep))
        stmts.push_back(move(x));
    }
  } else {
    if (docstr)
      *docstr = getDocstr(stmt);
    stmts.push_back(move(stmt));
  }
  return stmts;
}

shared_ptr<json> DocVisitor::transform(const ExprPtr &expr) {
  DocVisitor v(ctx);
  v.resultExpr = make_shared<json>();
  expr->accept(v);
  return v.resultExpr;
}

string DocVisitor::transform(const StmtPtr &stmt) {
  DocVisitor v(ctx);
  stmt->accept(v);
  return v.resultStmt;
}

void DocVisitor::transformModule(StmtPtr stmt) {
  vector<string> children;
  string docstr;

  auto flat = flatten(move(stmt), &docstr);
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

  auto id = to_string(ctx->shared->itemID++);
  auto ja =
      ctx->shared->j->set(id, make_shared<json>(unordered_map<string, string>{
                                  {"kind", "module"}, {"path", ctx->getFilename()}}));
  ja->set("children", make_shared<json>(children));
  if (!docstr.empty())
    ja->set("doc", docstr);
}

void DocVisitor::visit(IntExpr *expr) { resultExpr = make_shared<json>(expr->value); }

void DocVisitor::visit(IdExpr *expr) {
  auto i = ctx->find(expr->value);
  if (!i)
    error("unknown identifier {}", expr->value);
  resultExpr = make_shared<json>(*i ? to_string(*i) : expr->value);
}

void DocVisitor::visit(IndexExpr *expr) {
  vector<shared_ptr<json>> v;
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
  resultExpr = make_shared<json>(v);
}

bool isValidName(const string &s) {
  if (s.empty())
    return false;
  if (s.size() > 4 && s.substr(0, 2) == "__" && s.substr(s.size() - 2) == "__")
    return true;
  return s[0] != '_';
}

void DocVisitor::visit(FunctionStmt *stmt) {
  int id = ctx->shared->itemID++;
  ctx->add(stmt->name, make_shared<int>(id));
  auto j = make_shared<json>(
      unordered_map<string, string>{{"kind", "function"}, {"name", stmt->name}});
  j->set("pos", jsonify(stmt->getSrcInfo()));

  vector<shared_ptr<json>> args;
  vector<string> generics;
  for (auto &a : stmt->args)
    if (a.generic || (a.type && (a.type->isId("type") || a.type->isId("TypeVar") ||
                                 (a.type->getIndex() &&
                                  a.type->getIndex()->expr->isId("Static"))))) {
      ctx->add(a.name, make_shared<int>(0));
      generics.push_back(a.name);
      a.generic = true;
    }
  for (auto &a : stmt->args)
    if (!a.generic) {
      auto j = make_shared<json>();
      j->set("name", a.name);
      if (a.type)
        j->set("type", transform(a.type));
      if (a.deflt) {
        j->set("default", FormatVisitor::apply(a.deflt));
      }
      args.push_back(j);
    }
  j->set("generics", make_shared<json>(generics));
  bool isLLVM = false;
  for (auto &d : stmt->decorators)
    if (auto e = d->getId()) {
      j->set("attrs", make_shared<json>(e->value, ""));
      isLLVM |= (e->value == "llvm");
    }
  if (stmt->ret)
    j->set("return", transform(stmt->ret));
  j->set("args", make_shared<json>(args));
  string docstr;
  flatten(move(const_cast<FunctionStmt *>(stmt)->suite), &docstr);
  for (auto &g : generics)
    ctx->remove(g);
  if (!docstr.empty() && !isLLVM)
    j->set("doc", docstr);
  ctx->shared->j->set(to_string(id), j);
  resultStmt = to_string(id);
}

void DocVisitor::visit(ClassStmt *stmt) {
  vector<string> generics;
  auto j = make_shared<json>(
      unordered_map<string, string>{{"name", stmt->name},
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
    j->set("parent", to_string(*i));
    generics = ctx->shared->generics[*i];
  } else {
    ctx->add(stmt->name, make_shared<int>(id));
  }

  vector<shared_ptr<json>> args;
  for (auto &a : stmt->args)
    if (a.generic || (a.type && (a.type->isId("type") || a.type->isId("TypeVar") ||
                                 (a.type->getIndex() &&
                                  a.type->getIndex()->expr->isId("Static"))))) {
      a.generic = true;
      generics.push_back(a.name);
    }
  ctx->shared->generics[id] = generics;
  for (auto &g : generics)
    ctx->add(g, make_shared<int>(0));
  for (auto &a : stmt->args)
    if (!a.generic) {
      auto ja = make_shared<json>();
      ja->set("name", a.name);
      if (a.type)
        ja->set("type", transform(a.type));
      args.push_back(ja);
    }
  j->set("generics", make_shared<json>(generics));
  j->set("args", make_shared<json>(args));
  j->set("pos", jsonify(stmt->getSrcInfo()));

  string docstr;
  vector<string> members;
  for (auto &f : flatten(move(const_cast<ClassStmt *>(stmt)->suite), &docstr)) {
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
  j->set("members", make_shared<json>(members));
  if (!docstr.empty())
    j->set("doc", docstr);
  ctx->shared->j->set(to_string(id), j);
  resultStmt = to_string(id);
}

shared_ptr<json> DocVisitor::jsonify(const seq::SrcInfo &s) {
  return make_shared<json>(vector<string>{to_string(s.line), to_string(s.len)});
}

void DocVisitor::visit(ImportStmt *stmt) {
  if (stmt->from->isId("C") || stmt->from->isId("python")) {
    int id = ctx->shared->itemID++;
    string name, lib;
    if (auto i = stmt->what->getId())
      name = i->value;
    else if (auto d = stmt->what->getDot())
      name = d->member, lib = FormatVisitor::apply(d->expr);
    else
      seqassert(false, "invalid C import statement");
    ctx->add(name, make_shared<int>(id));
    name = stmt->as.empty() ? name : stmt->as;

    auto j = make_shared<json>(unordered_map<string, string>{
        {"name", name}, {"kind", "function"}, {"extern", stmt->from->getId()->value}});
    j->set("pos", jsonify(stmt->getSrcInfo()));
    vector<shared_ptr<json>> args;
    if (stmt->ret)
      j->set("return", transform(stmt->ret));
    for (auto &a : stmt->args) {
      auto ja = make_shared<json>();
      ja->set("name", a.name);
      ja->set("type", transform(a.type));
      args.push_back(ja);
    }
    j->set("dylib", lib);
    j->set("args", make_shared<json>(args));
    ctx->shared->j->set(to_string(id), j);
    resultStmt = to_string(id);
    return;
  }

  vector<string> dirs; // Path components
  Expr *e = stmt->from.get();
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
  // Handle dots (e.g. .. in from ..m import x).
  seqassert(stmt->dots >= 0, "negative dots in ImportStmt");
  for (int i = 0; i < stmt->dots - 1; i++)
    dirs.emplace_back("..");
  string path;
  for (int i = int(dirs.size()) - 1; i >= 0; i--)
    path += dirs[i] + (i ? "/" : "");
  // Fetch the import!
  auto file = getImportFile(ctx->shared->argv0, path, ctx->getFilename());
  if (!file)
    error(stmt, "cannot locate import '{}'", path);

  auto ictx = ctx;
  auto it = ctx->shared->modules.find(file->path);
  if (it == ctx->shared->modules.end()) {
    ictx = make_shared<DocContext>(ctx->shared);
    ictx->setFilename(file->path);
    auto tmp = parseFile(ctx->shared->cache, file->path);
    DocVisitor(ictx).transformModule(move(tmp));
  } else {
    ictx = it->second;
  }

  if (!stmt->what) {
    // TODO: implement this corner case
  } else if (stmt->what->isId("*")) {
    for (auto &i : *ictx)
      ctx->add(i.first, i.second[0].second);
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
  ctx->add(e->value, make_shared<int>(id));
  auto j = make_shared<json>(
      unordered_map<string, string>{{"name", e->value}, {"kind", "variable"}});
  j->set("pos", jsonify(stmt->getSrcInfo()));
  ctx->shared->j->set(to_string(id), j);
  resultStmt = to_string(id);
}

} // namespace ast
} // namespace seq
