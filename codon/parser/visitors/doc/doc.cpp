// Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>

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

llvm::json::Object DocVisitor::apply(const std::string &argv0,
                                     const std::vector<std::string> &files,
                                     const std::string &root) {
  auto shared = std::make_unique<DocShared>();
  shared->argv0 = argv0;
  std::pmr::monotonic_buffer_resource nodeStorage;
  std::vector<ast::ASTNodePtr> nodes;
  auto cache = std::make_unique<ast::Cache>(nodes, argv0, nullptr, &nodeStorage);
  shared->cache = cache.get();
  for (const auto &file : files)
    shared->requested.insert(cache->fs->canonical(file).generic_string());
  auto moduleRoot = root.empty() && !files.empty()
                        ? cache->fs->canonical(files.front()).parent_path()
                        : cache->fs->canonical(root);
  if (root.empty())
    for (const auto &file : shared->requested)
      while (!moduleRoot.empty()) {
        auto relative = std::filesystem::path(file).lexically_relative(moduleRoot);
        if (!relative.empty() && *relative.begin() != "..")
          break;
        moduleRoot = moduleRoot.parent_path();
      }
  shared->root = moduleRoot.generic_string();
  if (!shared->root.empty())
    cache->fs->add_search_path(shared->root);
  shared->modules[""] = std::make_shared<DocContext>(shared.get());
  auto stdlib = getImportFile(cache.get(), STDLIB_INTERNAL_MODULE, "", true);
  if (!stdlib)
    E(Error::CUSTOM, SrcInfo(), "cannot locate Codon standard library");
  auto astOrErr = ast::parseFile(shared->cache, stdlib->path);
  if (!astOrErr)
    throw exc::ParserException(astOrErr.takeError());
  auto coreOrErr =
      ast::parseCode(shared->cache, stdlib->path, "from internal.core import *");
  if (!coreOrErr)
    throw exc::ParserException(coreOrErr.takeError());
  shared->modules[""]->setFilename(stdlib->path);
  shared->modules[stdlib->path] = shared->modules[""];
  shared->modules[""]->add("int", std::make_shared<int>(shared->itemID++));
  shared->modules[""]->add("__py_numerics__", std::make_shared<int>(shared->itemID++));
  shared->modules[""]->add("__py_extension__", std::make_shared<int>(shared->itemID++));
  shared->modules[""]->add("__debug__", std::make_shared<int>(shared->itemID++));
  shared->modules[""]->add("__apple__", std::make_shared<int>(shared->itemID++));
  shared->modules[""]->add("__windows__", std::make_shared<int>(shared->itemID++));
  shared->modules[""]->add("__dict_unordered__",
                           std::make_shared<int>(shared->itemID++));
  for (const auto *name : {"__codon_version_major__", "__codon_version_minor__",
                           "__codon_version_micro__"})
    shared->modules[""]->add(name, std::make_shared<int>(shared->itemID++));

  llvm::json::Object j{{"name", "type"},
                       {"kind", "class"},
                       {"type", "type"},
                       {"symbol_id", "builtin:type"},
                       {"generics", llvm::json::Array{"T"}}};
  shared->modules[""]->add("type", std::make_shared<int>(shared->itemID));
  shared->symbols[std::to_string(shared->itemID++)] = std::move(j);
  j = llvm::json::Object{{"name", "Literal"},
                         {"kind", "class"},
                         {"type", "type"},
                         {"symbol_id", "builtin:Literal"},
                         {"generics", llvm::json::Array{"T"}}};
  shared->modules[""]->add("Literal", std::make_shared<int>(shared->itemID));
  shared->symbols[std::to_string(shared->itemID++)] = std::move(j);
  DocVisitor(shared->modules[""]).transformModule(*astOrErr, *coreOrErr);

  for (auto &f : files) {
    auto path = cache->fs->canonical(f).generic_string();
    DocVisitor(shared->modules[""]).loadModule(path);
  }

  shared->cache = nullptr;
  auto document = llvm::json::Object{{"schema_version", 2},
                                     {"module_root", shared->root},
                                     {"symbols", std::move(shared->symbols)}};
  shared->scopes.clear();
  shared->modules.clear();
  return document;
}

std::shared_ptr<int> DocContext::find(const std::string &s) const {
  auto i = Context<int>::find(s);
  if (!i && parent)
    return parent->find(s);
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

llvm::json::Value DocVisitor::transform(Expr *expr) {
  if (!expr)
    return nullptr;
  DocVisitor v(ctx);
  v.setSrcInfo(expr->getSrcInfo());
  v.resultExpr =
      llvm::json::Object{{"kind", "expression"}, {"text", FormatVisitor::apply(expr)}};
  expr->accept(v);
  return std::move(v.resultExpr);
}

std::string DocVisitor::transform(Stmt *stmt) {
  if (!stmt)
    return "";
  DocVisitor v(ctx);
  v.setSrcInfo(stmt->getSrcInfo());
  stmt->accept(v);
  return v.resultStmt;
}

void DocVisitor::transformModule(Stmt *stmt, Stmt *prelude) {
  if (auto err = ScopingVisitor::apply(ctx->shared->cache, stmt))
    throw exc::ParserException(std::move(err));

  std::vector<std::string> children;
  std::string docstr;

  auto flat = flatten(std::move(stmt), &docstr);
  auto relative =
      std::filesystem::path(ctx->getFilename()).lexically_relative(ctx->shared->root);
  if (!relative.empty() && *relative.begin() != "..") {
    relative.replace_extension();
    if (relative.filename() == "__init__")
      relative = relative.parent_path();
    std::vector<std::string> parts;
    for (const auto &part : relative)
      if (part != ".")
        parts.push_back(part.string());
    ctx->module = join(parts, ".");
  } else {
    ctx->module = ctx->shared->cache->fs->get_root(ctx->getFilename()).module;
  }
  ctx->moduleID = ctx->shared->itemID++;
  ctx->shared->scopes[ctx->moduleID] = ctx;
  ctx->shared->symbols[std::to_string(ctx->moduleID)] = llvm::json::Object{
      {"kind", "module"},
      {"name", ctx->module},
      {"symbol_id", "module:" + ctx->module},
      {"path", ctx->getFilename()},
      {"owned", ctx->shared->requested.count(ctx->getFilename()) != 0},
      {"children", llvm::json::Array{}}};
  for (auto *declaration : flat) {
    if (auto *function = cast<FunctionStmt>(declaration)) {
      declare(function, "function", function->getName());
    } else if (auto *cls = cast<ClassStmt>(declaration)) {
      bool extension = false;
      for (auto *decorator : cls->getDecorators())
        extension |= isId(decorator, "extend");
      if (!extension) {
        auto id = declare(cls, "class", cls->getName());
        for (const auto &parameter : *cls)
          if (!parameter.isValue())
            ctx->shared->generics[id].push_back(parameter.name);
      }
    } else if (auto *assignment = cast<AssignStmt>(declaration)) {
      if (auto *name = cast<IdExpr>(assignment->getLhs()))
        declare(assignment, "variable", name->getValue());
    }
  }
  if (prelude)
    transform(prelude);
  for (int i = 0; i < flat.size(); i++) {
    auto &s = flat[i];
    auto id = transform(s);
    if (id.empty())
      continue;
    if (i < (flat.size() - 1) && cast<AssignStmt>(s)) {
      auto ds = getDocstr(flat[i + 1]);
      if (!ds.empty())
        (*ctx->shared->symbols.getObject(id))["doc"] = ds;
    }
    children.push_back(id);
  }

  children.insert(children.end(), ctx->extraChildren.begin(), ctx->extraChildren.end());
  llvm::json::Object ja{{"children", llvm::json::Array(children)}};
  if (!docstr.empty())
    ja["doc"] = docstr;
  publish(ctx->moduleID, std::move(ja));
}

std::shared_ptr<DocContext> DocVisitor::loadModule(const std::string &path) {
  if (auto found = ctx->shared->modules.find(path); found != ctx->shared->modules.end())
    return found->second;
  auto module = std::make_shared<DocContext>(ctx->shared);
  module->setFilename(path);
  ctx->shared->modules[path] = module;
  auto parsed = parseFile(ctx->shared->cache, path);
  if (!parsed)
    throw exc::ParserException(parsed.takeError());
  DocVisitor(module).transformModule(*parsed);
  return module;
}

int DocVisitor::declare(Stmt *stmt, const std::string &kind, const std::string &name,
                        bool bind) {
  auto [entry, inserted] =
      ctx->shared->declarations[stmt].emplace(name, ctx->shared->itemID);
  if (inserted) {
    ++ctx->shared->itemID;
    auto qualified = ctx->module;
    if (!ctx->owner.empty())
      qualified += (qualified.empty() ? "" : ".") + ctx->owner;
    qualified += (qualified.empty() ? "" : ".") + name;
    auto identity = kind + ":" + qualified;
    auto index = ctx->shared->overloads[identity]++;
    if (kind == "function" || index)
      identity += ":" + std::to_string(index);
    ctx->shared->symbols[std::to_string(entry->second)] =
        llvm::json::Object{{"kind", kind},
                           {"name", name},
                           {"qualified_name", qualified},
                           {"symbol_id", identity},
                           {"module", std::to_string(ctx->moduleID)},
                           {"source", jsonify(stmt->getSrcInfo())}};
  }
  if (bind)
    ctx->add(name, std::make_shared<int>(entry->second));
  return entry->second;
}

void DocVisitor::publish(int id, llvm::json::Object value) {
  auto *target = ctx->shared->symbols.getObject(std::to_string(id));
  if (!target) {
    ctx->shared->symbols[std::to_string(id)] = std::move(value);
    return;
  }
  for (auto &entry : value)
    (*target)[entry.first] = std::move(entry.second);
}

std::shared_ptr<int> DocVisitor::resolve(Expr *expr) const {
  if (auto *name = cast<IdExpr>(expr))
    return ctx->find(name->getValue());
  if (auto *member = cast<DotExpr>(expr)) {
    auto parent = resolve(member->getExpr());
    if (parent) {
      auto scope = ctx->shared->scopes.find(*parent);
      if (scope != ctx->shared->scopes.end())
        return scope->second->Context<int>::find(member->getMember());
    }
  }
  return nullptr;
}

std::string DocVisitor::alias(Stmt *stmt, const std::string &name, int target) {
  auto id = declare(stmt, "alias", name, false);
  ctx->add(name, std::make_shared<int>(target));
  auto identity =
      ctx->shared->symbols.getObject(std::to_string(id))->getString("symbol_id")->str();
  identity.replace(0, identity.find(':'), "alias");
  publish(id, llvm::json::Object{{"kind", "alias"},
                                 {"target", std::to_string(target)},
                                 {"symbol_id", identity}});
  return std::to_string(id);
}

void DocVisitor::visit(IntExpr *expr) {
  resultExpr =
      llvm::json::Object{{"kind", "literal"}, {"text", FormatVisitor::apply(expr)}};
}

void DocVisitor::visit(IdExpr *expr) {
  auto i = ctx->find(expr->getValue());
  if (i && *i && ctx->shared->symbols.get(std::to_string(*i)))
    resultExpr = llvm::json::Object{{"kind", "reference"},
                                    {"target", std::to_string(*i)},
                                    {"text", expr->getValue()}};
}

void DocVisitor::visit(DotExpr *expr) {
  auto target = resolve(expr);
  if (target)
    resultExpr = llvm::json::Object{{"kind", "reference"},
                                    {"target", std::to_string(*target)},
                                    {"text", FormatVisitor::apply(expr)}};
}

void DocVisitor::visit(IndexExpr *expr) {
  auto tr = [&](Expr *e) -> llvm::json::Value {
    if (match(e, MOr(M<IntExpr>(), M<BoolExpr>(), M<StringExpr>())))
      return llvm::json::Object{{"kind", "literal"}, {"text", FormatVisitor::apply(e)}};
    else
      return transform(e);
  };
  llvm::json::Array v;
  if (auto tp = cast<TupleExpr>(expr->getIndex())) {
    if (auto l = tp->size() == 2 ? cast<ListExpr>((*tp)[0]) : nullptr) {
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
  resultExpr = llvm::json::Object{{"kind", "application"},
                                  {"base", transform(expr->getExpr())},
                                  {"args", std::move(v)},
                                  {"text", FormatVisitor::apply(expr)}};
}

void DocVisitor::visit(FunctionStmt *stmt) {
  int id = declare(stmt, "function", stmt->getName());
  llvm::json::Object j{
      {"kind", "function"},
      {"name", stmt->getName()},
      {"async", stmt->isAsync()},
      {"source", jsonify(stmt->getSrcInfo())},
      {"pos", llvm::json::Array{stmt->getSrcInfo().line, stmt->getSrcInfo().len}}};

  llvm::json::Array args;
  std::vector<std::string> generics;
  for (auto &a : *stmt)
    if (!a.isValue()) {
      ctx->add(a.name, std::make_shared<int>(0));
      generics.push_back(a.name);
    }
  for (auto &a : *stmt) {
    auto stars = a.getNameWithStars().first;
    const char *kind = stars == 2                        ? "var_keyword"
                       : stars == 1                      ? "var_positional"
                       : a.kind == Param::PositionalOnly ? "positional_only"
                       : a.kind == Param::KeywordOnly    ? "keyword_only"
                                                         : "positional_or_keyword";
    llvm::json::Object jj{
        {"name", a.name},
        {"kind", kind},
        {"generic", !a.isValue()},
        {"hidden", a.isHiddenGeneric() || (ctx->parent && a.name == "Self")}};
    if (a.type)
      jj["type"] = transform(a.type);
    if (a.defaultValue) {
      jj["default"] = FormatVisitor::apply(a.defaultValue);
    }
    args.push_back(std::move(jj));
  }
  j["generics"] = llvm::json::Array(generics);
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
    j["attrs"] = llvm::json::Array(attrs);
  if (stmt->getReturn())
    j["return"] = transform(stmt->getReturn());
  j["args"] = std::move(args);
  std::string docstr;
  flatten(stmt->getSuite(), &docstr);
  for (auto &g : generics)
    ctx->remove(g);
  if (!docstr.empty() && !isLLVM)
    j["doc"] = docstr;
  publish(id, std::move(j));
  resultStmt = std::to_string(id);
}

void DocVisitor::visit(ClassStmt *stmt) {
  std::vector<std::string> generics;

  bool isRecord = stmt->isRecord();
  llvm::json::Object j{{"name", stmt->getName()},
                       {"kind", "class"},
                       {"type", isRecord ? "type" : "class"}};
  bool isExtend = false;
  for (auto &d : stmt->getDecorators())
    if (auto e = cast<IdExpr>(d))
      isExtend |= (e->getValue() == "extend");

  int id = declare(stmt, "class", stmt->getName(), !isExtend);
  if (isExtend) {
    j["type"] = "extension";
    auto i = ctx->find(stmt->getName());
    if (i) {
      j["parent"] = std::to_string(*i);
      generics = ctx->shared->generics[*i];
    }
  }

  auto parent = ctx;
  ctx = std::make_shared<DocContext>(parent->shared);
  ctx->parent = parent.get();
  ctx->module = parent->module;
  ctx->moduleID = parent->moduleID;
  ctx->owner =
      parent->owner.empty() ? stmt->getName() : parent->owner + "." + stmt->getName();
  ctx->setFilename(parent->getFilename());
  ctx->shared->scopes[id] = ctx;
  llvm::json::Array args;
  for (const auto &a : *stmt)
    if (!a.isValue()) {
      generics.push_back(a.name);
    }
  ctx->shared->generics[id] = generics;
  for (auto &g : generics)
    ctx->add(g, std::make_shared<int>(0));
  for (const auto &a : *stmt) {
    llvm::json::Object ja{{"name", a.name}};
    if (a.type)
      ja["type"] = transform(a.type);
    if (a.defaultValue)
      ja["default"] = FormatVisitor::apply(a.defaultValue);
    args.push_back(std::move(ja));
  }
  j["generics"] = llvm::json::Array(generics);
  j["args"] = std::move(args);
  j["source"] = jsonify(stmt->getSrcInfo());
  j["pos"] = llvm::json::Array{stmt->getSrcInfo().line, stmt->getSrcInfo().len};
  llvm::json::Array bases;
  for (auto *base : stmt->getBaseClasses())
    bases.push_back(transform(base));
  j["bases"] = std::move(bases);

  std::string docstr;
  std::vector<std::string> members;
  for (auto &f : flatten(stmt->getSuite(), &docstr)) {
    if (auto ff = cast<FunctionStmt>(f)) {
      auto i = transform(f);
      if (i != "")
        members.push_back(i);
    }
  }
  for (auto &g : generics)
    ctx->remove(g);
  j["members"] = llvm::json::Array(members);
  if (!docstr.empty())
    j["doc"] = docstr;
  ctx = parent;
  publish(id, std::move(j));
  resultStmt = std::to_string(id);
}

llvm::json::Object DocVisitor::jsonify(const codon::SrcInfo &s) {
  return llvm::json::Object{
      {"path", s.file.str()}, {"line", s.line}, {"column", s.col}, {"length", s.len}};
}

void DocVisitor::visit(ImportStmt *stmt) {
  if (match(stmt->getFrom(), M<IdExpr>(MOr("C", "python")))) {
    std::string name, lib;
    if (auto i = cast<IdExpr>(stmt->getWhat()))
      name = i->getValue();
    else if (auto d = cast<DotExpr>(stmt->getWhat()))
      name = d->getMember(), lib = FormatVisitor::apply(d->getExpr());
    else
      seqassert(false, "invalid C import statement");
    name = stmt->getAs().empty() ? name : stmt->getAs();
    int id = declare(stmt, "function", name);

    llvm::json::Object dict{{"name", name}, {"kind", "function"}};
    if (stmt->getFrom() && cast<IdExpr>(stmt->getFrom()))
      dict["extern"] = cast<IdExpr>(stmt->getFrom())->getValue();
    auto j = std::move(dict);
    j["source"] = jsonify(stmt->getSrcInfo());
    j["pos"] = llvm::json::Array{stmt->getSrcInfo().line, stmt->getSrcInfo().len};
    llvm::json::Array args;
    if (stmt->getReturnType())
      j["return"] = transform(stmt->getReturnType());
    for (const auto &a : stmt->getArgs()) {
      args.push_back(llvm::json::Object{{"name", a.name}, {"type", transform(a.type)}});
    }
    j["dylib"] = lib;
    j["args"] = std::move(args);
    publish(id, std::move(j));
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
  auto locate = [&](const std::string &name) {
    auto file = getImportFile(ctx->shared->cache, name, ctx->getFilename());
    if (!file && !ctx->shared->root.empty())
      file =
          getImportFile(ctx->shared->cache, name, ctx->shared->root + "/__doc__.codon");
    return file;
  };
  auto file = locate(path);
  if (!file)
    E(Error::CUSTOM, stmt->getSrcInfo(), "cannot locate import '{}'", path);

  auto ictx = loadModule(file->path);

  if (!stmt->getWhat()) {
    if (!stmt->getAs().empty()) {
      resultStmt = alias(stmt, stmt->getAs(), ictx->moduleID);
    } else if (dirs.size() == 1) {
      resultStmt = alias(stmt, dirs.front(), ictx->moduleID);
    } else if (!dirs.empty()) {
      auto topFile = locate(dirs.back());
      if (topFile) {
        auto top = loadModule(topFile->path);
        resultStmt = alias(stmt, dirs.back(), top->moduleID);
        for (size_t index = dirs.size() - 1; index > 0; --index) {
          std::string prefix;
          for (size_t part = dirs.size(); part > index - 1; --part)
            prefix += (prefix.empty() ? "" : "/") + dirs[part - 1];
          auto childFile = locate(prefix);
          if (!childFile)
            break;
          auto child = loadModule(childFile->path);
          top->add(dirs[index - 1], std::make_shared<int>(child->moduleID));
          top = child;
        }
      }
    }
  } else if (isId(stmt->getWhat(), "*")) {
    std::vector<std::pair<std::string, int>> bindings;
    for (auto &binding : *ictx)
      bindings.emplace_back(binding.first, *binding.second.front());
    std::sort(bindings.begin(), bindings.end());
    for (const auto &[name, target] : bindings) {
      ctx->add(name, std::make_shared<int>(target));
      if (!name.empty() && name.front() != '_' && target)
        ctx->extraChildren.push_back(alias(stmt, name, target));
    }
  } else {
    auto i = cast<IdExpr>(stmt->getWhat());
    auto imported = stmt->getDots() > 0 && !stmt->getFrom()
                        ? ictx->Context<int>::find(i->getValue())
                        : ictx->find(i->getValue());
    if (auto c = imported)
      resultStmt =
          alias(stmt, stmt->getAs().empty() ? i->getValue() : stmt->getAs(), *c);
    else {
      auto child = locate(path.empty() ? i->getValue() : path + "/" + i->getValue());
      if (!child)
        E(Error::CUSTOM, stmt->getSrcInfo(), "symbol '{}' not found in {}",
          i->getValue(), file->path);
      auto module = loadModule(child->path);
      resultStmt = alias(stmt, stmt->getAs().empty() ? i->getValue() : stmt->getAs(),
                         module->moduleID);
    }
  }
}

void DocVisitor::visit(AssignStmt *stmt) {
  auto e = cast<IdExpr>(stmt->getLhs());
  if (!e)
    return;

  if (cast<IdExpr>(stmt->getRhs()) || cast<DotExpr>(stmt->getRhs()) ||
      cast<IndexExpr>(stmt->getRhs())) {
    auto *application = cast<IndexExpr>(stmt->getRhs());
    auto i = resolve(application ? application->getExpr() : stmt->getRhs());
    auto *target = i ? ctx->shared->symbols.getObject(std::to_string(*i)) : nullptr;
    if (target && target->getString("kind") == "class") {
      resultStmt = alias(stmt, e->getValue(), *i);
      (*ctx->shared->symbols.getObject(resultStmt))["type"] = transform(stmt->getRhs());
      return;
    }
  }

  int id = declare(stmt, "variable", e->getValue());
  llvm::json::Object j{{"name", e->getValue()}, {"kind", "variable"}};
  if (stmt->getTypeExpr())
    j["type"] = transform(stmt->getTypeExpr());
  if (stmt->getRhs())
    j["value"] = FormatVisitor::apply(stmt->getRhs());
  j["source"] = jsonify(stmt->getSrcInfo());
  j["pos"] = llvm::json::Array{stmt->getSrcInfo().line, stmt->getSrcInfo().len};
  publish(id, std::move(j));
  resultStmt = std::to_string(id);
}

} // namespace codon::ast
