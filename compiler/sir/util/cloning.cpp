#include "cloning.h"

namespace seq {
namespace ir {
namespace util {

void CloneVisitor::visit(const Var *v) {
  result = module->N<Var>(v, v->getType(), v->isGlobal(), v->getName());
}

void CloneVisitor::visit(const BodiedFunc *v) {
  auto *res = Nt(v);
  std::vector<std::string> argNames;

  for (auto it = v->arg_begin(); it != v->arg_end(); ++it)
    argNames.push_back((*it)->getName());
  for (const auto *var : *v) {
    auto *newVar = forceClone(var);
    res->push_back(newVar);
  }
  res->setUnmangledName(v->getUnmangledName());
  res->setGenerator(v->isGenerator());
  res->realize(cast<types::FuncType>(v->getType()), argNames);

  auto argIt1 = v->arg_begin();
  auto argIt2 = res->arg_begin();
  while (argIt1 != v->arg_end()) {
    forceRemap(*argIt1, *argIt2);
    ++argIt1;
    ++argIt2;
  }

  // body might reference this!
  forceRemap(v, res);

  if (v->getBody())
    res->setBody(clone(v->getBody()));
  res->setBuiltin(v->isBuiltin());
  result = res;
}

void CloneVisitor::visit(const ExternalFunc *v) {
  auto *res = Nt(v);
  std::vector<std::string> argNames;
  for (auto it = v->arg_begin(); it != v->arg_end(); ++it)
    argNames.push_back((*it)->getName());
  res->setUnmangledName(v->getUnmangledName());
  res->setGenerator(v->isGenerator());
  res->realize(cast<types::FuncType>(v->getType()), argNames);

  auto argIt1 = v->arg_begin();
  auto argIt2 = res->arg_begin();
  while (argIt1 != v->arg_end()) {
    forceRemap(*argIt1, *argIt2);
    ++argIt1;
    ++argIt2;
  }

  result = res;
}

void CloneVisitor::visit(const InternalFunc *v) {
  auto *res = Nt(v);
  std::vector<std::string> argNames;
  for (auto it = v->arg_begin(); it != v->arg_end(); ++it)
    argNames.push_back((*it)->getName());
  res->setUnmangledName(v->getUnmangledName());
  res->setGenerator(v->isGenerator());
  res->realize(cast<types::FuncType>(v->getType()), argNames);

  auto argIt1 = v->arg_begin();
  auto argIt2 = res->arg_begin();
  while (argIt1 != v->arg_end()) {
    forceRemap(*argIt1, *argIt2);
    ++argIt1;
    ++argIt2;
  }

  res->setParentType(v->getParentType());
  result = res;
}

void CloneVisitor::visit(const LLVMFunc *v) {
  auto *res = Nt(v);
  std::vector<std::string> argNames;
  for (auto it = v->arg_begin(); it != v->arg_end(); ++it)
    argNames.push_back((*it)->getName());
  res->setUnmangledName(v->getUnmangledName());
  res->setGenerator(v->isGenerator());
  res->realize(cast<types::FuncType>(v->getType()), argNames);

  auto argIt1 = v->arg_begin();
  auto argIt2 = res->arg_begin();
  while (argIt1 != v->arg_end()) {
    forceRemap(*argIt1, *argIt2);
    ++argIt1;
    ++argIt2;
  }

  res->setLLVMBody(v->getLLVMBody());
  res->setLLVMDeclarations(v->getLLVMDeclarations());
  res->setLLVMLiterals(
      std::vector<types::Generic>(v->literal_begin(), v->literal_end()));
  result = res;
}

void CloneVisitor::visit(const VarValue *v) { result = Nt(v, clone(v->getVar())); }

void CloneVisitor::visit(const PointerValue *v) { result = Nt(v, clone(v->getVar())); }

void CloneVisitor::visit(const SeriesFlow *v) {
  auto *res = Nt(v);
  for (auto *c : *v)
    res->push_back(clone(c));
  result = res;
}

void CloneVisitor::visit(const IfFlow *v) {
  result =
      Nt(v, clone(v->getCond()), clone(v->getTrueBranch()), clone(v->getFalseBranch()));
}

void CloneVisitor::visit(const WhileFlow *v) {
  auto *loop = Nt(v, nullptr, nullptr);
  forceRemap(v, loop);
  loop->setCond(clone(v->getCond()));
  loop->setBody(clone(v->getBody()));
  result = loop;
}

void CloneVisitor::visit(const ForFlow *v) {
  auto *loop = Nt(v, nullptr, nullptr, nullptr,
                  std::unique_ptr<transform::parallel::OMPSched>());
  forceRemap(v, loop);
  loop->setIter(clone(v->getIter()));
  loop->setBody(clone(v->getBody()));
  loop->setVar(clone(v->getVar()));
  if (auto *sched = v->getSchedule()) {
    auto schedCloned = std::make_unique<transform::parallel::OMPSched>(*sched);
    for (auto *val : sched->getUsedValues()) {
      schedCloned->replaceUsedValue(val->getId(), clone(val));
    }
    loop->setSchedule(std::move(schedCloned));
  }

  result = loop;
}

void CloneVisitor::visit(const ImperativeForFlow *v) {
  auto *loop = Nt(v, nullptr, v->getStep(), nullptr, nullptr, nullptr,
                  std::unique_ptr<transform::parallel::OMPSched>());
  forceRemap(v, loop);
  loop->setStart(clone(v->getStart()));
  loop->setBody(clone(v->getBody()));
  loop->setVar(clone(v->getVar()));
  loop->setEnd(clone(v->getEnd()));
  if (auto *sched = v->getSchedule()) {
    auto schedCloned = std::make_unique<transform::parallel::OMPSched>(*sched);
    for (auto *val : sched->getUsedValues()) {
      schedCloned->replaceUsedValue(val->getId(), clone(val));
    }
    loop->setSchedule(std::move(schedCloned));
  }
  result = loop;
}

void CloneVisitor::visit(const TryCatchFlow *v) {
  auto *res = Nt(v, clone(v->getBody()), clone(v->getFinally()));
  for (auto &c : *v) {
    res->emplace_back(clone(c.getHandler()), c.getType(), clone(c.getVar()));
  }
  result = res;
}

void CloneVisitor::visit(const PipelineFlow *v) {
  std::vector<PipelineFlow::Stage> cloned;
  for (const auto &s : *v) {
    cloned.push_back(clone(s));
  }
  result = Nt(v, std::move(cloned));
}

void CloneVisitor::visit(const dsl::CustomFlow *v) { result = v->doClone(*this); }

void CloneVisitor::visit(const IntConst *v) {
  result = Nt(v, v->getVal(), v->getType());
}

void CloneVisitor::visit(const FloatConst *v) {
  result = Nt(v, v->getVal(), v->getType());
}

void CloneVisitor::visit(const BoolConst *v) {
  result = Nt(v, v->getVal(), v->getType());
}

void CloneVisitor::visit(const StringConst *v) {
  result = Nt(v, v->getVal(), v->getType());
}

void CloneVisitor::visit(const dsl::CustomConst *v) { result = v->doClone(*this); }

void CloneVisitor::visit(const AssignInstr *v) {
  result = Nt(v, clone(v->getLhs()), clone(v->getRhs()));
}

void CloneVisitor::visit(const ExtractInstr *v) {
  result = Nt(v, clone(v->getVal()), v->getField());
}

void CloneVisitor::visit(const InsertInstr *v) {
  result = Nt(v, clone(v->getLhs()), v->getField(), clone(v->getRhs()));
}

void CloneVisitor::visit(const CallInstr *v) {
  std::vector<Value *> args;
  for (const auto *a : *v)
    args.push_back(clone(a));
  result = Nt(v, clone(v->getCallee()), std::move(args));
}

void CloneVisitor::visit(const StackAllocInstr *v) {
  result = Nt(v, v->getArrayType(), v->getCount());
}

void CloneVisitor::visit(const TypePropertyInstr *v) {
  result = Nt(v, v->getInspectType(), v->getProperty());
}

void CloneVisitor::visit(const YieldInInstr *v) {
  result = Nt(v, v->getType(), v->isSuspending());
}

void CloneVisitor::visit(const TernaryInstr *v) {
  result =
      Nt(v, clone(v->getCond()), clone(v->getTrueValue()), clone(v->getFalseValue()));
}

void CloneVisitor::visit(const BreakInstr *v) {
  result = Nt(v, cloneLoop ? clone(v->getLoop()) : v->getLoop());
}

void CloneVisitor::visit(const ContinueInstr *v) {
  result = Nt(v, cloneLoop ? clone(v->getLoop()) : v->getLoop());
}

void CloneVisitor::visit(const ReturnInstr *v) { result = Nt(v, clone(v->getValue())); }

void CloneVisitor::visit(const YieldInstr *v) {
  result = Nt(v, clone(v->getValue()), v->isFinal());
}

void CloneVisitor::visit(const ThrowInstr *v) { result = Nt(v, clone(v->getValue())); }

void CloneVisitor::visit(const FlowInstr *v) {
  result = Nt(v, clone(v->getFlow()), clone(v->getValue()));
}

void CloneVisitor::visit(const dsl::CustomInstr *v) { result = v->doClone(*this); }

} // namespace util
} // namespace ir
} // namespace seq
