#include "stmt.h"

#include <memory>
#include <string>
#include <vector>

#include "codon/parser/ast.h"
#include "codon/parser/visitors/visitor.h"

#define ACCEPT_IMPL(T, X)                                                              \
  StmtPtr T::clone() const { return std::make_shared<T>(*this); }                      \
  void T::accept(X &visitor) { visitor.visit(this); }

using fmt::format;

const int INDENT_SIZE = 2;

namespace codon {
namespace ast {

Stmt::Stmt() : done(false), age(-1) {}
Stmt::Stmt(const codon::SrcInfo &s) : done(false) { setSrcInfo(s); }
std::string Stmt::toString() const { return toString(-1); }

SuiteStmt::SuiteStmt(std::vector<StmtPtr> stmts, bool ownBlock)
    : Stmt(), ownBlock(ownBlock) {
  for (auto &s : stmts)
    flatten(std::move(s), this->stmts);
}
SuiteStmt::SuiteStmt(const SuiteStmt &stmt)
    : Stmt(stmt), stmts(ast::clone(stmt.stmts)), ownBlock(stmt.ownBlock) {}
std::string SuiteStmt::toString(int indent) const {
  std::string pad = indent >= 0 ? ("\n" + std::string(indent + INDENT_SIZE, ' ')) : " ";
  std::string s;
  for (int i = 0; i < stmts.size(); i++)
    if (stmts[i])
      s += (i ? pad : "") +
           stmts[i]->toString(indent >= 0 ? indent + INDENT_SIZE : -1) +
           (stmts[i]->done ? "*" : "");
  return format("(suite{}{})", ownBlock ? " #:own " : "",
                s.empty() ? s : " " + pad + s);
}
ACCEPT_IMPL(SuiteStmt, ASTVisitor);
void SuiteStmt::flatten(StmtPtr s, std::vector<StmtPtr> &stmts) {
  // WARNING: does not preserve attributes!
  if (!s)
    return;
  auto suite = const_cast<SuiteStmt *>(s->getSuite());
  if (!suite || suite->ownBlock)
    stmts.push_back(s);
  else {
    for (auto &ss : suite->stmts)
      stmts.push_back(ss);
  }
}
StmtPtr *SuiteStmt::lastInBlock() {
  if (stmts.empty())
    return nullptr;
  if (auto s = const_cast<SuiteStmt *>(stmts.back()->getSuite())) {
    auto l = s->lastInBlock();
    if (l)
      return l;
  }
  return &(stmts.back());
}

std::string BreakStmt::toString(int) const { return "(break)"; }
ACCEPT_IMPL(BreakStmt, ASTVisitor);

std::string ContinueStmt::toString(int) const { return "(continue)"; }
ACCEPT_IMPL(ContinueStmt, ASTVisitor);

ExprStmt::ExprStmt(ExprPtr expr) : Stmt(), expr(std::move(expr)) {}
ExprStmt::ExprStmt(const ExprStmt &stmt) : Stmt(stmt), expr(ast::clone(stmt.expr)) {}
std::string ExprStmt::toString(int) const {
  return format("(expr {})", expr->toString());
}
ACCEPT_IMPL(ExprStmt, ASTVisitor);

AssignStmt::AssignStmt(ExprPtr lhs, ExprPtr rhs, ExprPtr type, bool shadow)
    : Stmt(), lhs(std::move(lhs)), rhs(std::move(rhs)), type(std::move(type)),
      shadow(shadow) {}
AssignStmt::AssignStmt(const AssignStmt &stmt)
    : Stmt(stmt), lhs(ast::clone(stmt.lhs)), rhs(ast::clone(stmt.rhs)),
      type(ast::clone(stmt.type)), shadow(stmt.shadow) {}
std::string AssignStmt::toString(int) const {
  return format("(assign {}{}{})", lhs->toString(), rhs ? " " + rhs->toString() : "",
                type ? format(" #:type {}", type->toString()) : "");
}
ACCEPT_IMPL(AssignStmt, ASTVisitor);

DelStmt::DelStmt(ExprPtr expr) : Stmt(), expr(std::move(expr)) {}
DelStmt::DelStmt(const DelStmt &stmt) : Stmt(stmt), expr(ast::clone(stmt.expr)) {}
std::string DelStmt::toString(int) const {
  return format("(del {})", expr->toString());
}
ACCEPT_IMPL(DelStmt, ASTVisitor);

PrintStmt::PrintStmt(std::vector<ExprPtr> items, bool isInline)
    : Stmt(), items(std::move(items)), isInline(isInline) {}
PrintStmt::PrintStmt(const PrintStmt &stmt)
    : Stmt(stmt), items(ast::clone(stmt.items)), isInline(stmt.isInline) {}
std::string PrintStmt::toString(int) const {
  return format("(print {}{})", isInline ? "#:inline " : "", combine(items));
}
ACCEPT_IMPL(PrintStmt, ASTVisitor);

ReturnStmt::ReturnStmt(ExprPtr expr) : Stmt(), expr(std::move(expr)) {}
ReturnStmt::ReturnStmt(const ReturnStmt &stmt)
    : Stmt(stmt), expr(ast::clone(stmt.expr)) {}
std::string ReturnStmt::toString(int) const {
  return expr ? format("(return {})", expr->toString()) : "(return)";
}
ACCEPT_IMPL(ReturnStmt, ASTVisitor);

YieldStmt::YieldStmt(ExprPtr expr) : Stmt(), expr(std::move(expr)) {}
YieldStmt::YieldStmt(const YieldStmt &stmt) : Stmt(stmt), expr(ast::clone(stmt.expr)) {}
std::string YieldStmt::toString(int) const {
  return expr ? format("(yield {})", expr->toString()) : "(yield)";
}
ACCEPT_IMPL(YieldStmt, ASTVisitor);

AssertStmt::AssertStmt(ExprPtr expr, ExprPtr message)
    : Stmt(), expr(std::move(expr)), message(std::move(message)) {}
AssertStmt::AssertStmt(const AssertStmt &stmt)
    : Stmt(stmt), expr(ast::clone(stmt.expr)), message(ast::clone(stmt.message)) {}
std::string AssertStmt::toString(int) const {
  return format("(assert {}{})", expr->toString(), message ? message->toString() : "");
}
ACCEPT_IMPL(AssertStmt, ASTVisitor);

WhileStmt::WhileStmt(ExprPtr cond, StmtPtr suite, StmtPtr elseSuite)
    : Stmt(), cond(std::move(cond)), suite(std::move(suite)),
      elseSuite(std::move(elseSuite)) {}
WhileStmt::WhileStmt(const WhileStmt &stmt)
    : Stmt(stmt), cond(ast::clone(stmt.cond)), suite(ast::clone(stmt.suite)),
      elseSuite(ast::clone(stmt.elseSuite)) {}
std::string WhileStmt::toString(int indent) const {
  std::string pad = indent > 0 ? ("\n" + std::string(indent + INDENT_SIZE, ' ')) : " ";
  if (elseSuite && elseSuite->firstInBlock())
    return format("(while-else {}{}{}{}{})", cond->toString(), pad,
                  suite->toString(indent >= 0 ? indent + INDENT_SIZE : -1), pad,
                  elseSuite->toString(indent >= 0 ? indent + INDENT_SIZE : -1));
  else
    return format("(while {}{}{})", cond->toString(), pad,
                  suite->toString(indent >= 0 ? indent + INDENT_SIZE : -1));
}
ACCEPT_IMPL(WhileStmt, ASTVisitor);

ForStmt::ForStmt(ExprPtr var, ExprPtr iter, StmtPtr suite, StmtPtr elseSuite,
                 ExprPtr decorator, std::vector<CallExpr::Arg> ompArgs)
    : Stmt(), var(std::move(var)), iter(std::move(iter)), suite(std::move(suite)),
      elseSuite(std::move(elseSuite)), decorator(std::move(decorator)),
      ompArgs(std::move(ompArgs)), wrapped(false) {}
ForStmt::ForStmt(const ForStmt &stmt)
    : Stmt(stmt), var(ast::clone(stmt.var)), iter(ast::clone(stmt.iter)),
      suite(ast::clone(stmt.suite)), elseSuite(ast::clone(stmt.elseSuite)),
      decorator(ast::clone(stmt.decorator)), ompArgs(ast::clone_nop(stmt.ompArgs)),
      wrapped(stmt.wrapped) {}
std::string ForStmt::toString(int indent) const {
  std::string pad = indent > 0 ? ("\n" + std::string(indent + INDENT_SIZE, ' ')) : " ";
  std::string attr;
  if (decorator)
    attr += " " + decorator->toString();
  if (!attr.empty())
    attr = " #:attr" + attr;
  if (elseSuite && elseSuite->firstInBlock())
    return format("(for-else {} {}{}{}{}{}{})", var->toString(), iter->toString(), attr,
                  pad, suite->toString(indent >= 0 ? indent + INDENT_SIZE : -1), pad,
                  elseSuite->toString(indent >= 0 ? indent + INDENT_SIZE : -1));
  else
    return format("(for {} {}{}{}{})", var->toString(), iter->toString(), attr, pad,
                  suite->toString(indent >= 0 ? indent + INDENT_SIZE : -1));
}
ACCEPT_IMPL(ForStmt, ASTVisitor);

IfStmt::IfStmt(ExprPtr cond, StmtPtr ifSuite, StmtPtr elseSuite)
    : Stmt(), cond(std::move(cond)), ifSuite(std::move(ifSuite)),
      elseSuite(std::move(elseSuite)) {}
IfStmt::IfStmt(const IfStmt &stmt)
    : Stmt(stmt), cond(ast::clone(stmt.cond)), ifSuite(ast::clone(stmt.ifSuite)),
      elseSuite(ast::clone(stmt.elseSuite)) {}
std::string IfStmt::toString(int indent) const {
  std::string pad = indent > 0 ? ("\n" + std::string(indent + INDENT_SIZE, ' ')) : " ";
  return format("(if {}{}{}{})", cond->toString(), pad,
                ifSuite->toString(indent >= 0 ? indent + INDENT_SIZE : -1),
                elseSuite
                    ? pad + elseSuite->toString(indent >= 0 ? indent + INDENT_SIZE : -1)
                    : "");
}
ACCEPT_IMPL(IfStmt, ASTVisitor);

MatchStmt::MatchCase MatchStmt::MatchCase::clone() const {
  return {ast::clone(pattern), ast::clone(guard), ast::clone(suite)};
}

MatchStmt::MatchStmt(ExprPtr what, std::vector<MatchStmt::MatchCase> cases)
    : Stmt(), what(std::move(what)), cases(std::move(cases)) {}
MatchStmt::MatchStmt(const MatchStmt &stmt)
    : Stmt(stmt), what(ast::clone(stmt.what)), cases(ast::clone_nop(stmt.cases)) {}
std::string MatchStmt::toString(int indent) const {
  std::string pad = indent > 0 ? ("\n" + std::string(indent + INDENT_SIZE, ' ')) : " ";
  std::string padExtra = indent > 0 ? std::string(INDENT_SIZE, ' ') : "";
  std::vector<std::string> s;
  for (auto &c : cases)
    s.push_back(format("(case {}{}{}{})", c.pattern->toString(),
                       c.guard ? " #:guard " + c.guard->toString() : "", pad + padExtra,
                       c.suite->toString(indent >= 0 ? indent + INDENT_SIZE : -1 * 2)));
  return format("(match {}{}{})", what->toString(), pad, join(s, pad));
}
ACCEPT_IMPL(MatchStmt, ASTVisitor);

ImportStmt::ImportStmt(ExprPtr from, ExprPtr what, std::vector<Param> args, ExprPtr ret,
                       std::string as, int dots)
    : Stmt(), from(std::move(from)), what(std::move(what)), as(std::move(as)),
      dots(dots), args(std::move(args)), ret(std::move(ret)) {}
ImportStmt::ImportStmt(const ImportStmt &stmt)
    : Stmt(stmt), from(ast::clone(stmt.from)), what(ast::clone(stmt.what)), as(stmt.as),
      dots(stmt.dots), args(ast::clone_nop(stmt.args)), ret(ast::clone(stmt.ret)) {}
std::string ImportStmt::toString(int) const {
  std::vector<std::string> va;
  for (auto &a : args)
    va.push_back(a.toString());
  return format("(import {}{}{}{}{}{})", from->toString(),
                as.empty() ? "" : format(" #:as '{}", as),
                what ? format(" #:what {}", what->toString()) : "",
                dots ? format(" #:dots {}", dots) : "",
                va.empty() ? "" : format(" #:args ({})", join(va)),
                ret ? format(" #:ret {}", ret->toString()) : "");
}
ACCEPT_IMPL(ImportStmt, ASTVisitor);

TryStmt::Catch TryStmt::Catch::clone() const {
  return {var, ast::clone(exc), ast::clone(suite)};
}

TryStmt::TryStmt(StmtPtr suite, std::vector<Catch> catches, StmtPtr finally)
    : Stmt(), suite(std::move(suite)), catches(std::move(catches)),
      finally(std::move(finally)) {}
TryStmt::TryStmt(const TryStmt &stmt)
    : Stmt(stmt), suite(ast::clone(stmt.suite)), catches(ast::clone_nop(stmt.catches)),
      finally(ast::clone(stmt.finally)) {}
std::string TryStmt::toString(int indent) const {
  std::string pad = indent > 0 ? ("\n" + std::string(indent + INDENT_SIZE, ' ')) : " ";
  std::string padExtra = indent > 0 ? std::string(INDENT_SIZE, ' ') : "";
  std::vector<std::string> s;
  for (auto &i : catches)
    s.push_back(
        format("(catch {}{}{}{})", !i.var.empty() ? format("#:var '{}", i.var) : "",
               i.exc ? format(" #:exc {}", i.exc->toString()) : "", pad + padExtra,
               i.suite->toString(indent >= 0 ? indent + INDENT_SIZE : -1 * 2)));
  return format(
      "(try{}{}{}{}{})", pad, suite->toString(indent >= 0 ? indent + INDENT_SIZE : -1),
      pad, join(s, pad),
      finally ? format("{}{}", pad,
                       finally->toString(indent >= 0 ? indent + INDENT_SIZE : -1))
              : "");
}
ACCEPT_IMPL(TryStmt, ASTVisitor);

ThrowStmt::ThrowStmt(ExprPtr expr, bool transformed)
    : Stmt(), expr(std::move(expr)), transformed(transformed) {}
ThrowStmt::ThrowStmt(const ThrowStmt &stmt)
    : Stmt(stmt), expr(ast::clone(stmt.expr)), transformed(stmt.transformed) {}
std::string ThrowStmt::toString(int) const {
  return format("(throw{})", expr ? " " + expr->toString() : "");
}
ACCEPT_IMPL(ThrowStmt, ASTVisitor);

GlobalStmt::GlobalStmt(std::string var) : Stmt(), var(std::move(var)) {}
std::string GlobalStmt::toString(int) const { return format("(global '{})", var); }
ACCEPT_IMPL(GlobalStmt, ASTVisitor);

Attr::Attr(const std::vector<std::string> &attrs)
    : module(), parentClass(), isAttribute(false) {
  for (auto &a : attrs)
    set(a);
}
void Attr::set(const std::string &attr) { customAttr.insert(attr); }
void Attr::unset(const std::string &attr) { customAttr.erase(attr); }
bool Attr::has(const std::string &attr) const { return in(customAttr, attr); }

const std::string Attr::LLVM = "llvm";
const std::string Attr::Python = "python";
const std::string Attr::Atomic = "atomic";
const std::string Attr::Property = "property";
const std::string Attr::Internal = "__internal__";
const std::string Attr::ForceRealize = "__force__";
const std::string Attr::C = "std.internal.attributes.C";
const std::string Attr::CVarArg = ".__vararg__";
const std::string Attr::Method = ".__method__";
const std::string Attr::Capture = ".__capture__";
const std::string Attr::Extend = "extend";
const std::string Attr::Tuple = "tuple";
const std::string Attr::Test = "std.internal.attributes.test";

FunctionStmt::FunctionStmt(std::string name, ExprPtr ret, std::vector<Param> args,
                           StmtPtr suite, Attr attributes,
                           std::vector<ExprPtr> decorators)
    : Stmt(), name(std::move(name)), ret(std::move(ret)), args(std::move(args)),
      suite(std::move(suite)), attributes(std::move(attributes)),
      decorators(std::move(decorators)) {}
FunctionStmt::FunctionStmt(const FunctionStmt &stmt)
    : Stmt(stmt), name(stmt.name), ret(ast::clone(stmt.ret)),
      args(ast::clone_nop(stmt.args)), suite(ast::clone(stmt.suite)),
      attributes(stmt.attributes), decorators(ast::clone(stmt.decorators)) {}
std::string FunctionStmt::toString(int indent) const {
  std::string pad = indent > 0 ? ("\n" + std::string(indent + INDENT_SIZE, ' ')) : " ";
  std::vector<std::string> as;
  for (auto &a : args)
    as.push_back(a.toString());
  std::vector<std::string> attr;
  for (auto &a : decorators)
    attr.push_back(format("(dec {})", a->toString()));
  return format("(fn '{} ({}){}{}{}{})", name, join(as, " "),
                ret ? " #:ret " + ret->toString() : "",
                attr.empty() ? "" : format(" (attr {})", join(attr, " ")), pad,
                suite ? suite->toString(indent >= 0 ? indent + INDENT_SIZE : -1)
                      : "(suite)");
}
ACCEPT_IMPL(FunctionStmt, ASTVisitor);
std::string FunctionStmt::signature() const {
  std::vector<std::string> s;
  for (auto &a : args)
    s.push_back(a.type ? a.type->toString() : "-");
  return format("{}", join(s, ":"));
}
bool FunctionStmt::hasAttr(const std::string &attr) const {
  return attributes.has(attr);
}

ClassStmt::ClassStmt(std::string name, std::vector<Param> args, StmtPtr suite,
                     Attr attributes, std::vector<ExprPtr> decorators,
                     std::vector<ExprPtr> baseClasses)
    : Stmt(), name(std::move(name)), args(std::move(args)), suite(std::move(suite)),
      attributes(std::move(attributes)), decorators(std::move(decorators)),
      baseClasses(std::move(baseClasses)) {}
ClassStmt::ClassStmt(const ClassStmt &stmt)
    : Stmt(stmt), name(stmt.name), args(ast::clone_nop(stmt.args)),
      suite(ast::clone(stmt.suite)), attributes(stmt.attributes),
      decorators(ast::clone(stmt.decorators)),
      baseClasses(ast::clone(stmt.baseClasses)) {}
std::string ClassStmt::toString(int indent) const {
  std::string pad = indent > 0 ? ("\n" + std::string(indent + INDENT_SIZE, ' ')) : " ";
  std::vector<std::string> bases;
  for (auto &b : baseClasses)
    bases.push_back(b->toString());
  std::string as;
  for (int i = 0; i < args.size(); i++)
    as += (i ? pad : "") + args[i].toString();
  std::vector<std::string> attr;
  for (auto &a : decorators)
    attr.push_back(format("(dec {})", a->toString()));
  return format("(class '{}{}{}{}{}{})", name,
                bases.empty() ? "" : format(" (bases {})", join(bases, " ")),
                attr.empty() ? "" : format(" (attr {})", join(attr, " ")),
                as.empty() ? as : pad + as, pad,
                suite ? suite->toString(indent >= 0 ? indent + INDENT_SIZE : -1)
                      : "(suite)");
}
ACCEPT_IMPL(ClassStmt, ASTVisitor);
bool ClassStmt::isRecord() const { return hasAttr(Attr::Tuple); }
bool ClassStmt::hasAttr(const std::string &attr) const { return attributes.has(attr); }

YieldFromStmt::YieldFromStmt(ExprPtr expr) : Stmt(), expr(std::move(expr)) {}
YieldFromStmt::YieldFromStmt(const YieldFromStmt &stmt)
    : Stmt(stmt), expr(ast::clone(stmt.expr)) {}
std::string YieldFromStmt::toString(int) const {
  return format("(yield-from {})", expr->toString());
}
ACCEPT_IMPL(YieldFromStmt, ASTVisitor);

WithStmt::WithStmt(std::vector<ExprPtr> items, std::vector<std::string> vars,
                   StmtPtr suite)
    : Stmt(), items(std::move(items)), vars(std::move(vars)), suite(std::move(suite)) {
  seqassert(items.size() == vars.size(), "vector size mismatch");
}
WithStmt::WithStmt(std::vector<std::pair<ExprPtr, ExprPtr>> itemVarPairs, StmtPtr suite)
    : Stmt(), suite(std::move(suite)) {
  for (auto &i : itemVarPairs) {
    items.push_back(std::move(i.first));
    if (i.second) {
      if (!i.second->getId())
        throw;
      vars.push_back(i.second->getId()->value);
    } else {
      vars.push_back("");
    }
  }
}
WithStmt::WithStmt(const WithStmt &stmt)
    : Stmt(stmt), items(ast::clone(stmt.items)), vars(stmt.vars),
      suite(ast::clone(stmt.suite)) {}
std::string WithStmt::toString(int indent) const {
  std::string pad = indent > 0 ? ("\n" + std::string(indent + INDENT_SIZE, ' ')) : " ";
  std::vector<std::string> as;
  for (int i = 0; i < items.size(); i++) {
    as.push_back(!vars[i].empty()
                     ? format("({} #:var '{})", items[i]->toString(), vars[i])
                     : items[i]->toString());
  }
  return format("(with ({}){}{})", join(as, " "), pad,
                suite->toString(indent >= 0 ? indent + INDENT_SIZE : -1));
}
ACCEPT_IMPL(WithStmt, ASTVisitor);

CustomStmt::CustomStmt(std::string keyword, ExprPtr expr, StmtPtr suite)
    : Stmt(), keyword(std::move(keyword)), expr(std::move(expr)),
      suite(std::move(suite)) {}
CustomStmt::CustomStmt(const CustomStmt &stmt)
    : Stmt(stmt), keyword(stmt.keyword), expr(ast::clone(stmt.expr)),
      suite(ast::clone(stmt.suite)) {}
std::string CustomStmt::toString(int indent) const {
  std::string pad = indent > 0 ? ("\n" + std::string(indent + INDENT_SIZE, ' ')) : " ";
  return format("(custom-{} {}{}{})", keyword,
                expr ? format(" #:expr {}", expr->toString()) : "", pad,
                suite ? suite->toString(indent >= 0 ? indent + INDENT_SIZE : -1) : "");
}
ACCEPT_IMPL(CustomStmt, ASTVisitor);

AssignMemberStmt::AssignMemberStmt(ExprPtr lhs, std::string member, ExprPtr rhs)
    : Stmt(), lhs(std::move(lhs)), member(std::move(member)), rhs(std::move(rhs)) {}
AssignMemberStmt::AssignMemberStmt(const AssignMemberStmt &stmt)
    : Stmt(stmt), lhs(ast::clone(stmt.lhs)), member(stmt.member),
      rhs(ast::clone(stmt.rhs)) {}
std::string AssignMemberStmt::toString(int) const {
  return format("(assign-member {} {} {})", lhs->toString(), member, rhs->toString());
}
ACCEPT_IMPL(AssignMemberStmt, ASTVisitor);

UpdateStmt::UpdateStmt(ExprPtr lhs, ExprPtr rhs, bool isAtomic)
    : Stmt(), lhs(std::move(lhs)), rhs(std::move(rhs)), isAtomic(isAtomic) {}
UpdateStmt::UpdateStmt(const UpdateStmt &stmt)
    : Stmt(stmt), lhs(ast::clone(stmt.lhs)), rhs(ast::clone(stmt.rhs)),
      isAtomic(stmt.isAtomic) {}
std::string UpdateStmt::toString(int) const {
  return format("(update {} {})", lhs->toString(), rhs->toString());
}
ACCEPT_IMPL(UpdateStmt, ASTVisitor);

} // namespace ast
} // namespace codon
