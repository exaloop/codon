// Copyright (C) 2022-2024 Exaloop Inc. <https://exaloop.io>

#include "cfg.h"

#include <vector>

#include "codon/cir/dsl/codegen.h"
#include "codon/cir/dsl/nodes.h"

namespace codon {
namespace ir {
namespace analyze {
namespace dataflow {
namespace {
// TODO: this logic is very similar to lowering/pipeline -- unify somehow?
Value *callStage(analyze::dataflow::CFGraph *cfg, PipelineFlow::Stage *stage,
                 Value *last) {
  std::vector<Value *> args;
  for (auto *arg : *stage) {
    args.push_back(arg ? arg : last);
  }
  return cfg->N<CallInstr>(stage->getCallee(), args);
}

Value *convertPipelineToForLoopsHelper(analyze::dataflow::CFGraph *cfg,
                                       std::vector<PipelineFlow::Stage *> &stages,
                                       unsigned idx = 0, Value *last = nullptr) {
  if (idx >= stages.size())
    return last;

  auto *stage = stages[idx];
  if (idx == 0)
    return convertPipelineToForLoopsHelper(cfg, stages, idx + 1, stage->getCallee());

  auto *prev = stages[idx - 1];
  if (prev->isGenerator()) {
    auto *var = cfg->N<Var>(prev->getOutputElementType());
    auto *body = convertPipelineToForLoopsHelper(
        cfg, stages, idx + 1, callStage(cfg, stage, cfg->N<VarValue>(var)));
    auto *series = cfg->N<SeriesFlow>();
    series->push_back(body);
    return cfg->N<ForFlow>(last, series, var);
  } else {
    return convertPipelineToForLoopsHelper(cfg, stages, idx + 1,
                                           callStage(cfg, stage, last));
  }
}
} // namespace

const Value *convertPipelineToForLoops(analyze::dataflow::CFGraph *cfg,
                                       const PipelineFlow *p) {
  std::vector<PipelineFlow::Stage *> stages;
  for (const auto &stage : *p) {
    stages.push_back(const_cast<PipelineFlow::Stage *>(&stage));
  }
  return convertPipelineToForLoopsHelper(cfg, stages);
}

void CFBlock::reg(const Value *v) { graph->valueLocations[v->getId()] = this; }

const char SyntheticAssignInstr::NodeId = 0;

int SyntheticAssignInstr::doReplaceUsedValue(id_t id, Value *newValue) {
  if (arg && arg->getId() == id) {
    arg = newValue;
    return 1;
  }
  return 0;
}

int SyntheticAssignInstr::doReplaceUsedVariable(id_t id, Var *newVar) {
  if (lhs->getId() == id) {
    lhs = newVar;
    return 1;
  }
  return 0;
}

const char SyntheticPhiInstr::NodeId = 0;

std::vector<Value *> SyntheticPhiInstr::doGetUsedValues() const {
  std::vector<Value *> ret;
  for (auto &p : *this) {
    ret.push_back(const_cast<Value *>(p.getResult()));
  }
  return ret;
}

int SyntheticPhiInstr::doReplaceUsedValue(id_t id, Value *newValue) {
  auto res = 0;
  for (auto &p : *this) {
    if (p.getResult()->getId() == id) {
      p.setResult(newValue);
      ++res;
    }
  }
  return res;
}

CFGraph::CFGraph(const BodiedFunc *f) : func(f) { newBlock("entry", true); }

std::ostream &operator<<(std::ostream &os, const CFGraph &cfg) {
  os << "digraph \"" << cfg.func->getName() << "\" {\n";
  for (auto *block : cfg) {
    os << "  ";
    os << block->getName() << "_" << reinterpret_cast<uintptr_t>(block);
    os << " [ label=\"" << block->getName() << "\"";
    if (block == cfg.getEntryBlock()) {
      os << " shape=square";
    }
    os << " ];\n";
  }
  for (auto *block : cfg) {
    for (auto next = block->successors_begin(); next != block->successors_end();
         ++next) {
      CFBlock *succ = *next;
      os << "  ";
      os << block->getName() << "_" << reinterpret_cast<uintptr_t>(block);
      os << " -> ";
      os << succ->getName() << "_" << reinterpret_cast<uintptr_t>(succ);
      os << ";\n";
    }
  }
  os << "}";
  return os;
}

std::unique_ptr<CFGraph> buildCFGraph(const BodiedFunc *f) {
  auto ret = std::make_unique<CFGraph>(f);
  CFVisitor v(ret.get());
  v.process(f);
  return ret;
}

const std::string CFAnalysis::KEY = "core-analyses-cfg";

std::unique_ptr<Result> CFAnalysis::run(const Module *m) {
  auto res = std::make_unique<CFResult>();
  if (const auto *main = cast<BodiedFunc>(m->getMainFunc())) {
    res->graphs.insert(std::make_pair(main->getId(), buildCFGraph(main)));
  }

  for (const auto *var : *m) {
    if (const auto *f = cast<BodiedFunc>(var)) {
      res->graphs.insert(std::make_pair(f->getId(), buildCFGraph(f)));
    }
  }
  return res;
}

void CFVisitor::visit(const BodiedFunc *f) {
  auto *blk = graph->getCurrentBlock();
  for (auto it = f->arg_begin(); it != f->arg_end(); it++) {
    blk->push_back(graph->N<analyze::dataflow::SyntheticAssignInstr>(
        const_cast<Var *>(*it), const_cast<VarValue *>(graph->N<VarValue>(*it))));
  }
  process(f->getBody());
}

void CFVisitor::visit(const SeriesFlow *v) {
  for (auto *c : *v) {
    process(c);
  }
}

void CFVisitor::visit(const IfFlow *v) {
  process(v->getCond());
  auto *original = graph->getCurrentBlock();
  auto *end = graph->newBlock("endIf");

  auto *tBranch = graph->newBlock("trueBranch", true);
  process(v->getTrueBranch());
  graph->getCurrentBlock()->successors_insert(end);

  analyze::dataflow::CFBlock *fBranch = nullptr;
  if (v->getFalseBranch()) {
    fBranch = graph->newBlock("falseBranch", true);
    process(v->getFalseBranch());
    graph->getCurrentBlock()->successors_insert(end);
  }

  original->successors_insert(tBranch);
  if (fBranch)
    original->successors_insert(fBranch);
  else
    original->successors_insert(end);

  graph->setCurrentBlock(end);
}

void CFVisitor::visit(const WhileFlow *v) {
  auto *original = graph->getCurrentBlock();
  auto *end = graph->newBlock("endWhile");

  auto *loopBegin = graph->newBlock("whileBegin", true);
  original->successors_insert(loopBegin);
  process(v->getCond());
  graph->getCurrentBlock()->successors_insert(end);

  loopStack.emplace_back(loopBegin, end, v->getId(), tryCatchStack.size() - 1);
  auto *body = graph->newBlock("whileBody", true);
  loopBegin->successors_insert(body);
  process(v->getBody());
  loopStack.pop_back();
  graph->getCurrentBlock()->successors_insert(loopBegin);

  graph->setCurrentBlock(end);
}

void CFVisitor::visit(const ForFlow *v) {
  if (v->isParallel()) {
    for (auto *v : v->getSchedule()->getUsedValues()) {
      process(v);
    }
  }
  auto *original = graph->getCurrentBlock();
  auto *end = graph->newBlock("endFor");

  auto *loopBegin = graph->newBlock("forBegin", true);
  original->successors_insert(loopBegin);
  process(v->getIter());

  auto *loopCheck = graph->newBlock("forCheck");
  graph->getCurrentBlock()->successors_insert(loopCheck);
  loopCheck->successors_insert(end);

  auto *loopNext = graph->newBlock("forNext");
  loopCheck->successors_insert(loopNext);
  loopNext->push_back(graph->N<analyze::dataflow::SyntheticAssignInstr>(
      const_cast<Var *>(v->getVar()), const_cast<Value *>(v->getIter()),
      analyze::dataflow::SyntheticAssignInstr::NEXT_VALUE));

  loopStack.emplace_back(loopCheck, end, v->getId(), tryCatchStack.size() - 1);
  auto *loopBody = graph->newBlock("forBody", true);
  loopNext->successors_insert(loopBody);
  process(v->getBody());
  graph->getCurrentBlock()->successors_insert(loopCheck);
  loopStack.pop_back();

  graph->setCurrentBlock(end);
}

void CFVisitor::visit(const ImperativeForFlow *v) {
  if (v->isParallel()) {
    for (auto *v : v->getSchedule()->getUsedValues()) {
      process(v);
    }
  }
  auto *original = graph->getCurrentBlock();
  auto *end = graph->newBlock("endFor");

  auto *loopBegin = graph->newBlock("forBegin", true);
  original->successors_insert(loopBegin);
  loopBegin->push_back(graph->N<analyze::dataflow::SyntheticAssignInstr>(
      const_cast<Var *>(v->getVar()), const_cast<Value *>(v->getStart()),
      analyze::dataflow::SyntheticAssignInstr::KNOWN));
  process(v->getStart());
  process(v->getEnd());

  auto *loopCheck = graph->newBlock("forCheck");
  graph->getCurrentBlock()->successors_insert(loopCheck);
  loopCheck->successors_insert(end);

  auto *loopNext = graph->newBlock("forUpdate");
  loopNext->push_back(graph->N<analyze::dataflow::SyntheticAssignInstr>(
      const_cast<Var *>(v->getVar()), v->getStep()));
  loopNext->successors_insert(loopCheck);

  loopStack.emplace_back(loopCheck, end, v->getId(), tryCatchStack.size() - 1);
  auto *loopBody = graph->newBlock("forBody", true);
  loopCheck->successors_insert(loopBody);
  process(v->getBody());
  graph->getCurrentBlock()->successors_insert(loopCheck);
  loopStack.pop_back();

  graph->setCurrentBlock(end);
}

void CFVisitor::visit(const TryCatchFlow *v) {
  auto *routeBlock = graph->newBlock("tcRoute");
  auto *end = graph->newBlock("tcEnd");
  analyze::dataflow::CFBlock *finally = nullptr;
  if (v->getFinally())
    finally = graph->newBlock("tcFinally");

  auto *dst = finally ? finally : end;

  tryCatchStack.emplace_back(routeBlock, finally);
  process(v->getBody());
  graph->getCurrentBlock()->successors_insert(dst);

  for (auto &c : *v) {
    auto *cBlock = graph->newBlock("catch", true);
    if (c.getVar())
      cBlock->push_back(graph->N<analyze::dataflow::SyntheticAssignInstr>(
          const_cast<Var *>(c.getVar())));
    process(c.getHandler());
    routeBlock->successors_insert(cBlock);
    graph->getCurrentBlock()->successors_insert(dst);
  }

  tryCatchStack.pop_back();

  if (v->getFinally()) {
    graph->setCurrentBlock(finally);
    process(v->getFinally());
    graph->getCurrentBlock()->successors_insert(end);
    routeBlock->successors_insert(finally);
  }

  if (!tryCatchStack.empty()) {
    if (finally)
      finally->successors_insert(tryCatchStack.back().first);
    else
      routeBlock->successors_insert(tryCatchStack.back().first);
  }

  graph->setCurrentBlock(end);
}

void CFVisitor::visit(const PipelineFlow *v) {
  if (auto *loops = convertPipelineToForLoops(graph, v)) {
    process(loops);
  } else {
    // pipeline is empty
  }
}

void CFVisitor::visit(const dsl::CustomFlow *v) {
  v->getCFBuilder()->buildCFNodes(this);
}

void CFVisitor::visit(const AssignInstr *v) {
  process(v->getRhs());
  defaultInsert(v);
}

void CFVisitor::visit(const ExtractInstr *v) {
  process(v->getVal());
  defaultInsert(v);
}

void CFVisitor::visit(const InsertInstr *v) {
  process(v->getLhs());
  process(v->getRhs());
  defaultInsert(v);
}

void CFVisitor::visit(const CallInstr *v) {
  process(v->getCallee());
  for (auto *a : *v)
    process(a);
  defaultInsert(v);
}

void CFVisitor::visit(const TernaryInstr *v) {
  auto *end = graph->newBlock("ternaryDone");
  auto *tBranch = graph->newBlock("ternaryTrue");
  auto *fBranch = graph->newBlock("ternaryFalse");

  process(v->getCond());
  graph->getCurrentBlock()->successors_insert(tBranch);
  graph->getCurrentBlock()->successors_insert(fBranch);

  graph->setCurrentBlock(tBranch);
  process(v->getTrueValue());
  graph->getCurrentBlock()->successors_insert(end);

  graph->setCurrentBlock(fBranch);
  process(v->getFalseValue());
  graph->getCurrentBlock()->successors_insert(end);

  auto *phi = graph->N<analyze::dataflow::SyntheticPhiInstr>();
  phi->emplace_back(tBranch, const_cast<Value *>(v->getTrueValue()));
  phi->emplace_back(fBranch, const_cast<Value *>(v->getFalseValue()));

  end->push_back(phi);
  graph->remapValue(v, phi);
  graph->setCurrentBlock(end);
}

void CFVisitor::visit(const BreakInstr *v) {
  auto &loop = v->getLoop() ? findLoop(v->getLoop()->getId()) : loopStack.back();
  defaultJump(loop.end, loop.tcIndex);
  defaultInsert(v);
}

void CFVisitor::visit(const ContinueInstr *v) {
  auto &loop = v->getLoop() ? findLoop(v->getLoop()->getId()) : loopStack.back();
  defaultJump(loop.nextIt, loop.tcIndex);
  defaultInsert(v);
}

void CFVisitor::visit(const ReturnInstr *v) {
  if (v->getValue())
    process(v->getValue());
  defaultJump(nullptr, -1);
  defaultInsert(v);
}

void CFVisitor::visit(const YieldInstr *v) {
  if (v->getValue())
    process(v->getValue());
  defaultInsert(v);
}

void CFVisitor::visit(const ThrowInstr *v) {
  if (v->getValue())
    process(v->getValue());
  defaultInsert(v);
}

void CFVisitor::visit(const FlowInstr *v) {
  process(v->getFlow());
  if (v->getValue())
    process(v->getValue());
  defaultInsert(v);
}

void CFVisitor::visit(const dsl::CustomInstr *v) {
  v->getCFBuilder()->buildCFNodes(this);
}

void CFVisitor::defaultInsert(const Value *v) {
  if (tryCatchStack.empty()) {
    graph->getCurrentBlock()->push_back(v);
  } else {
    auto *original = graph->getCurrentBlock();
    auto *newBlock = graph->newBlock("default", true);
    original->successors_insert(newBlock);
    newBlock->successors_insert(tryCatchStack.back().first);
    graph->getCurrentBlock()->push_back(v);
  }
  seenIds.insert(v->getId());
}

void CFVisitor::defaultJump(const CFBlock *cf, int newTcLevel) {
  int curTc = tryCatchStack.size() - 1;

  if (curTc == -1 || curTc <= newTcLevel) {
    if (cf)
      graph->getCurrentBlock()->successors_insert(const_cast<CFBlock *>(cf));
  } else {
    CFBlock *nearestFinally = nullptr;
    for (auto i = newTcLevel + 1; i <= curTc; ++i) {
      if (auto *n = tryCatchStack[i].second) {
        nearestFinally = n;
        break;
      }
    }
    if (nearestFinally) {
      graph->getCurrentBlock()->successors_insert(tryCatchStack.back().first);
      if (cf)
        nearestFinally->successors_insert(const_cast<CFBlock *>(cf));
    } else {
      if (cf)
        graph->getCurrentBlock()->successors_insert(const_cast<CFBlock *>(cf));
    }
  }
}

CFVisitor::Loop &CFVisitor::findLoop(id_t id) {
  return *std::find_if(loopStack.begin(), loopStack.end(),
                       [=](auto &it) { return it.loopId == id; });
}

} // namespace dataflow
} // namespace analyze
} // namespace ir
} // namespace codon

#undef DEFAULT_VISIT
