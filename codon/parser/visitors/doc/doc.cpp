// Copyright (C) 2022-2025 Exaloop Inc. <https://exaloop.io>

#include "doc.h"

#include <memory>
#include <string>
#include <vector>

#include "codon/parser/ast.h"
#include "codon/parser/common.h"
#include "codon/parser/match.h"
#include "codon/parser/peg/peg.h"
#include "codon/parser/visitors/format/format.h"
#include "codon/parser/visitors/scoping/scoping.h"

namespace codon::ast {

using namespace error;
using namespace matcher;

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

  auto stdlib = getImportFile(cache->fs.get(), STDLIB_INTERNAL_MODULE, "", true);
  auto astOrErr = ast::parseFile(shared->cache, stdlib->path);
  if (!astOrErr)
    throw exc::ParserException(astOrErr.takeError());
  auto coreOrErr =
      ast::parseCode(shared->cache, stdlib->path, "from internal.core import *");
  if (!coreOrErr)
    throw exc::ParserException(coreOrErr.takeError());
  shared->modules[""]->setFilename(stdlib->path);
  shared->modules[""]->add("__py_numerics__", std::make_shared<int>(shared->itemID++));
  shared->modules[""]->add("__py_extension__", std::make_shared<int>(shared->itemID++));
  shared->modules[""]->add("__debug__", std::make_shared<int>(shared->itemID++));
  shared->modules[""]->add("__apple__", std::make_shared<int>(shared->itemID++));

  auto j = std::make_shared<json>(std::unordered_map<std::string, std::string>{
      {"name", "type"}, {"kind", "class"}, {"type", "type"}});
  j->set("generics", std::make_shared<json>(std::vector<std::string>{"T"}));
  shared->modules[""]->add("type", std::make_shared<int>(shared->itemID));
  shared->j->set(std::to_string(shared->itemID++), j);
  j = std::make_shared<json>(std::unordered_map<std::string, std::string>{
      {"name", "Literal"}, {"kind", "class"}, {"type", "type"}});
  j->set("generics", std::make_shared<json>(std::vector<std::string>{"T"}));
  shared->modules[""]->add("Literal", std::make_shared<int>(shared->itemID));
  shared->j->set(std::to_string(shared->itemID++), j);
  DocVisitor(shared->modules[""]).transformModule(*coreOrErr);
  DocVisitor(shared->modules[""]).transformModule(*astOrErr);

  auto ctx = std::make_shared<DocContext>(shared);
  for (auto &f : files) {
    auto path = std::string(cache->fs->canonical(f));
    ctx->setFilename(path);
    // LOG("-> parsing {}", path);
    auto fAstOrErr = ast::parseFile(shared->cache, path);
    if (!fAstOrErr)
      throw exc::ParserException(fAstOrErr.takeError());
    DocVisitor(ctx).transformModule(*fAstOrErr);
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

std::string getDocstr(Stmt *s) {
  if (auto se = cast<ExprStmt>(s))
    if (auto e = cast<StringExpr>(se->getExpr()))
      return e->getValue();
  return "";
}

std::vector<Stmt *> DocVisitor::flatten(Stmt *stmt, std::string *docstr, bool deep) {
  std::vector<Stmt *> stmts;
  if (auto s = cast<SuiteStmt>(stmt)) {
    for (int i = 0; i < (deep ? s->size() : 1); i++) {
      for (auto &x : flatten((*s)[i], i ? nullptr : docstr, deep))
        stmts.push_back(std::move(x));
    }
  } else {
    if (docstr)
      *docstr = getDocstr(stmt);
    stmts.push_back(std::move(stmt));
  }
  return stmts;
}

std::shared_ptr<json> DocVisitor::transform(Expr *expr) {
  if (!expr)
    return std::make_shared<json>();
  DocVisitor v(ctx);
  v.setSrcInfo(expr->getSrcInfo());
  v.resultExpr = std::make_shared<json>();
  expr->accept(v);
  return v.resultExpr;
}

std::string DocVisitor::transform(Stmt *stmt) {
  if (!stmt)
    return "";
  DocVisitor v(ctx);
  v.setSrcInfo(stmt->getSrcInfo());
  stmt->accept(v);
  return v.resultStmt;
}

void DocVisitor::transformModule(Stmt *stmt) {
  if (auto err = ScopingVisitor::apply(ctx->shared->cache, stmt))
    throw exc::ParserException(std::move(err));

  std::vector<std::string> children;
  std::string docstr;

  auto flat = flatten(std::move(stmt), &docstr);
  for (int i = 0; i < flat.size(); i++) {
    auto &s = flat[i];
    auto id = transform(s);
    if (id.empty())
      continue;
    if (i < (flat.size() - 1) && cast<AssignStmt>(s)) {
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
  auto [value, _] = expr->getRawData();
  resultExpr = std::make_shared<json>(value);
}

void DocVisitor::visit(IdExpr *expr) {
  auto i = ctx->find(expr->getValue());
  if (!i)
    E(Error::CUSTOM, expr->getSrcInfo(), "unknown identifier {}", expr->getValue());
  resultExpr = std::make_shared<json>(*i ? std::to_string(*i) : expr->getValue());
}

void DocVisitor::visit(IndexExpr *expr) {
  auto tr = [&](Expr *e) {
    if (match(e, MOr(M<IntExpr>(), M<BoolExpr>(), M<StringExpr>())))
      return std::make_shared<json>(FormatVisitor::apply(e));
    else
      return transform(e);
  };
  std::vector<std::shared_ptr<json>> v;
  v.push_back(transform(expr->getExpr()));
  if (auto tp = cast<TupleExpr>(expr->getIndex())) {
    if (auto l = cast<ListExpr>((*tp)[0])) {
      for (auto *e : *l)
        v.push_back(tr(e));
      v.push_back(tr((*tp)[1]));
    } else
      for (auto *e : *tp) {
        v.push_back(tr(e));
      }
  } else {
    v.push_back(tr(expr->getIndex()));
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
  ctx->add(stmt->getName(), std::make_shared<int>(id));
  auto j = std::make_shared<json>(std::unordered_map<std::string, std::string>{
      {"kind", "function"}, {"name", stmt->getName()}});
  j->set("pos", jsonify(stmt->getSrcInfo()));

  std::vector<std::shared_ptr<json>> args;
  std::vector<std::string> generics;
  for (auto &a : *stmt)
    if (!a.isValue()) {
      ctx->add(a.name, std::make_shared<int>(0));
      generics.push_back(a.name);
      a.status = Param::Generic;
    }
  for (auto &a : *stmt) {
    auto jj = std::make_shared<json>();
    jj->set("name", a.name);
    if (a.type) {
      auto tt = transform(a.type);
      if (tt->values.empty()) {
        LOG("{}: warning: cannot resolve argument {}", a.type->getSrcInfo(),
            FormatVisitor::apply(a.type));
        jj->set("type", FormatVisitor::apply(a.type));
      } else {
        jj->set("type", tt);
      }
    }
    if (a.defaultValue) {
      jj->set("default", FormatVisitor::apply(a.defaultValue));
    }
    args.push_back(jj);
  }
  j->set("generics", std::make_shared<json>(generics));
  bool isLLVM = false;
  std::vector<std::string> attrs;
  for (const auto &d : stmt->getDecorators())
    if (auto e = cast<IdExpr>(d)) {
      attrs.push_back(e->getValue());
      isLLVM |= (e->getValue() == "llvm");
    }
  if (stmt->hasAttribute(Attr::Property))
    attrs.push_back("property");
  if (stmt->hasAttribute(Attr::Attribute))
    attrs.push_back("__attribute__");
  if (stmt->hasAttribute(Attr::Python))
    attrs.push_back("python");
  if (stmt->hasAttribute(Attr::LLVM))
    attrs.push_back("llvm");
  if (stmt->hasAttribute(Attr::Internal))
    attrs.push_back("__internal__");
  if (stmt->hasAttribute(Attr::HiddenFromUser))
    attrs.push_back("__hidden__");
  if (stmt->hasAttribute(Attr::Atomic))
    attrs.push_back("atomic");
  if (stmt->hasAttribute(Attr::StaticMethod))
    attrs.push_back("staticmethod");
  if (stmt->hasAttribute(Attr::C))
    attrs.push_back("C");
  if (!attrs.empty())
    j->set("attrs", std::make_shared<json>(attrs));
  if (stmt->getReturn())
    j->set("return", transform(stmt->getReturn()));
  j->set("args", std::make_shared<json>(args));
  std::string docstr;
  flatten(stmt->getSuite(), &docstr);
  for (auto &g : generics)
    ctx->remove(g);
  if (!docstr.empty() && !isLLVM)
    j->set("doc", docstr);
  ctx->shared->j->set(std::to_string(id), j);
  resultStmt = std::to_string(id);
}

void DocVisitor::visit(ClassStmt *stmt) {
  std::vector<std::string> generics;

  bool isRecord = stmt->isRecord();
  auto j = std::make_shared<json>(std::unordered_map<std::string, std::string>{
      {"name", stmt->getName()},
      {"kind", "class"},
      {"type", isRecord ? "type" : "class"}});
  int id = ctx->shared->itemID++;

  bool isExtend = false;
  for (auto &d : stmt->getDecorators())
    if (auto e = cast<IdExpr>(d))
      isExtend |= (e->getValue() == "extend");

  if (isExtend) {
    j->set("type", "extension");
    auto i = ctx->find(stmt->getName());
    j->set("parent", std::to_string(*i));
    generics = ctx->shared->generics[*i];
  } else {
    ctx->add(stmt->getName(), std::make_shared<int>(id));
  }

  std::vector<std::shared_ptr<json>> args;
  for (const auto &a : *stmt)
    if (!a.isValue()) {
      generics.push_back(a.name);
    }
  ctx->shared->generics[id] = generics;
  for (auto &g : generics)
    ctx->add(g, std::make_shared<int>(0));
  for (const auto &a : *stmt) {
    auto ja = std::make_shared<json>();
    ja->set("name", a.name);
    if (a.type) {
      auto tt = transform(a.type);
      if (tt->values.empty()) {
        LOG("{}: warning: cannot resolve argument {}", a.type->getSrcInfo(),
            FormatVisitor::apply(a.type));
        ja->set("type", FormatVisitor::apply(a.type));
      } else {
        ja->set("type", tt);
      }
    }
    if (a.defaultValue)
      ja->set("default", FormatVisitor::apply(a.defaultValue));
    args.push_back(ja);
  }
  j->set("generics", std::make_shared<json>(generics));
  j->set("args", std::make_shared<json>(args));
  j->set("pos", jsonify(stmt->getSrcInfo()));

  std::string docstr;
  std::vector<std::string> members;
  for (auto &f : flatten(stmt->getSuite(), &docstr)) {
    if (auto ff = cast<FunctionStmt>(f)) {
      auto i = transform(f);
      if (i != "")
        members.push_back(i);
      if (isValidName(ff->getName()))
        ctx->remove(ff->getName());
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
  if (match(stmt->getFrom(), M<IdExpr>(MOr("C", "python")))) {
    int id = ctx->shared->itemID++;
    std::string name, lib;
    if (auto i = cast<IdExpr>(stmt->getWhat()))
      name = i->getValue();
    else if (auto d = cast<DotExpr>(stmt->getWhat()))
      name = d->getMember(), lib = FormatVisitor::apply(d->getExpr());
    else
      seqassert(false, "invalid C import statement");
    ctx->add(name, std::make_shared<int>(id));
    name = stmt->getAs().empty() ? name : stmt->getAs();

    auto dict = std::unordered_map<std::string, std::string>{{"name", name},
                                                             {"kind", "function"}};
    if (stmt->getFrom() && cast<IdExpr>(stmt->getFrom()))
      dict["extern"] = cast<IdExpr>(stmt->getFrom())->getValue();
    auto j = std::make_shared<json>(dict);
    j->set("pos", jsonify(stmt->getSrcInfo()));
    std::vector<std::shared_ptr<json>> args;
    if (stmt->getReturnType())
      j->set("return", transform(stmt->getReturnType()));
    for (const auto &a : stmt->getArgs()) {
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
  Expr *e = stmt->getFrom();
  while (cast<DotExpr>(e)) {
    while (auto d = cast<DotExpr>(e)) {
      dirs.push_back(d->getMember());
      e = d->getExpr();
    }
    if (!cast<IdExpr>(e) || !stmt->getArgs().empty() || stmt->getReturnType() ||
        (stmt->getWhat() && !cast<IdExpr>(stmt->getWhat())))
      E(Error::CUSTOM, stmt->getSrcInfo(), "invalid import statement");
  }
  if (auto ee = cast<IdExpr>(e)) {
    if (!stmt->getArgs().empty() || stmt->getReturnType() ||
        (stmt->getWhat() && !cast<IdExpr>(stmt->getWhat())))
      E(Error::CUSTOM, stmt->getSrcInfo(), "invalid import statement");
    // We have an empty stmt->from in "from .. import".
    if (!ee->getValue().empty())
      dirs.push_back(ee->getValue());
  }
  // Handle dots (e.g. .. in from ..m import x).
  for (size_t i = 1; i < stmt->getDots(); i++)
    dirs.emplace_back("..");
  std::string path;
  for (int i = static_cast<int>(dirs.size()) - 1; i >= 0; i--)
    path += dirs[i] + (i ? "/" : "");
  // Fetch the import!
  auto file = getImportFile(ctx->shared->cache->fs.get(), path, ctx->getFilename());
  if (!file)
    E(Error::CUSTOM, stmt->getSrcInfo(), "cannot locate import '{}'", path);

  auto ictx = ctx;
  auto it = ctx->shared->modules.find(file->path);
  if (it == ctx->shared->modules.end()) {
    ctx->shared->modules[file->path] = ictx = std::make_shared<DocContext>(ctx->shared);
    ictx->setFilename(file->path);
    // LOG("=> parsing {}", file->path);
    auto tmpOrErr = parseFile(ctx->shared->cache, file->path);
    if (!tmpOrErr)
      throw exc::ParserException(tmpOrErr.takeError());
    DocVisitor(ictx).transformModule(*tmpOrErr);
  } else {
    ictx = it->second;
  }

  if (!stmt->getWhat()) {
    // TODO: implement this corner case
    for (auto &i : dirs)
      if (!ctx->find(i))
        ctx->add(i, std::make_shared<int>(ctx->shared->itemID++));
  } else if (isId(stmt->getWhat(), "*")) {
    for (auto &i : *ictx)
      ctx->add(i.first, i.second.front());
  } else {
    auto i = cast<IdExpr>(stmt->getWhat());
    if (auto c = ictx->find(i->getValue()))
      ctx->add(stmt->getAs().empty() ? i->getValue() : stmt->getAs(), c);
    else
      E(Error::CUSTOM, stmt->getSrcInfo(), "symbol '{}' not found in {}", i->getValue(),
        file->path);
  }
}

void DocVisitor::visit(AssignStmt *stmt) {
  auto e = cast<IdExpr>(stmt->getLhs());
  if (!e)
    return;
  int id = ctx->shared->itemID++;
  ctx->add(e->getValue(), std::make_shared<int>(id));
  auto j = std::make_shared<json>(std::unordered_map<std::string, std::string>{
      {"name", e->getValue()}, {"kind", "variable"}});
  if (stmt->getTypeExpr())
    j->set("type", transform(stmt->getTypeExpr()));
  if (stmt->getRhs())
    j->set("value", FormatVisitor::apply(stmt->getRhs()));
  j->set("pos", jsonify(stmt->getSrcInfo()));
  ctx->shared->j->set(std::to_string(id), j);
  resultStmt = std::to_string(id);
}

} // namespace codon::ast
