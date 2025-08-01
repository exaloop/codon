#include "test.h"

#include "codon/cir/analyze/dataflow/cfg.h"
#include "codon/cir/analyze/dataflow/reaching.h"

using namespace codon::ir;

namespace {
std::unordered_set<codon::ir::id_t> rdset(analyze::dataflow::RDInspector &rd, Var *var,
                                          Value *loc) {
  auto defs = rd.getReachingDefinitions(var, loc);
  std::unordered_set<codon::ir::id_t> set;
  for (auto &def : defs) {
    if (def.known())
      set.insert(def.assignee->getId());
  }
  return set;
}
} // namespace

TEST_F(CIRCoreTest, RDAnalysisSimple) {
  auto *f = module->Nr<BodiedFunc>("test_f");
  auto *b = module->Nr<SeriesFlow>();
  f->setBody(b);

  auto *v = module->Nr<Var>(module->getIntType());
  f->push_back(v);

  auto *first = module->getInt(1);
  auto *second = module->getInt(2);

  auto *start = module->getBool(false);
  auto *firstAssign = module->Nr<AssignInstr>(v, first);
  auto *secondAssign = module->Nr<AssignInstr>(v, second);
  auto *end = module->getBool(false);

  b->push_back(start);
  b->push_back(firstAssign);
  b->push_back(secondAssign);
  b->push_back(end);

  auto c = analyze::dataflow::buildCFGraph(f);
  analyze::dataflow::RDInspector rd(c.get());
  rd.analyze();

  auto startRd = rdset(rd, v, start);
  auto firstRd = rdset(rd, v, firstAssign);
  auto secondRd = rdset(rd, v, secondAssign);
  auto endRd = rdset(rd, v, end);
  auto firstRhsRd = rdset(rd, v, first);
  auto secondRhsRd = rdset(rd, v, second);

  ASSERT_EQ(0, startRd.size());
  ASSERT_EQ(1, firstRd.size());
  ASSERT_EQ(1, secondRd.size());
  ASSERT_EQ(1, endRd.size());
  ASSERT_EQ(0, firstRhsRd.size());
  ASSERT_EQ(1, secondRhsRd.size());

  ASSERT_EQ(first->getId(), *firstRd.begin());
  ASSERT_EQ(second->getId(), *secondRd.begin());
  ASSERT_EQ(second->getId(), *endRd.begin());
}

TEST_F(CIRCoreTest, RDAnalysisIfConditional) {
  auto *f = module->Nr<BodiedFunc>("test_f");

  auto *trueBranch = module->Nr<SeriesFlow>();
  auto *falseBranch = module->Nr<SeriesFlow>();
  auto *ifFlow = module->Nr<IfFlow>(module->getBool(false), trueBranch, falseBranch);
  auto *b = module->Nr<SeriesFlow>();
  f->setBody(b);

  auto *v = module->Nr<Var>(module->getIntType());
  f->push_back(v);

  auto *first = module->getInt(1);
  auto *second = module->getInt(2);

  auto *start = module->getBool(false);
  auto *firstAssign = module->Nr<AssignInstr>(v, first);
  auto *secondAssign = module->Nr<AssignInstr>(v, second);
  auto *end = module->getBool(false);

  b->push_back(start);
  b->push_back(ifFlow);
  trueBranch->push_back(firstAssign);
  falseBranch->push_back(secondAssign);
  b->push_back(end);

  auto c = analyze::dataflow::buildCFGraph(f);
  analyze::dataflow::RDInspector rd(c.get());
  rd.analyze();

  auto startRd = rdset(rd, v, start);
  auto endRd = rdset(rd, v, end);

  ASSERT_EQ(0, startRd.size());
  ASSERT_EQ(2, endRd.size());
  ASSERT_TRUE(endRd.find(first->getId()) != endRd.end());
  ASSERT_TRUE(endRd.find(second->getId()) != endRd.end());
}

TEST_F(CIRCoreTest, RDAnalysisTryCatch) {
  auto *f = module->Nr<BodiedFunc>("test_f");

  auto *body = module->Nr<SeriesFlow>();
  auto *except = module->Nr<SeriesFlow>();
  auto *finally = module->Nr<SeriesFlow>();
  auto *tc = module->Nr<TryCatchFlow>(body, finally);
  tc->emplace_back(except, nullptr);
  auto *b = module->Nr<SeriesFlow>();
  f->setBody(b);

  auto *v = module->Nr<Var>(module->getIntType());
  f->push_back(v);

  auto *first = module->getInt(1);
  auto *second = module->getInt(2);
  auto *third = module->getInt(3);

  auto *initial = module->getInt(0);
  auto *zeroAssign = module->Nr<AssignInstr>(v, initial);
  auto *start = module->getBool(false);
  auto *firstAssign = module->Nr<AssignInstr>(v, first);
  auto *secondAssign = module->Nr<AssignInstr>(v, second);
  auto *middle = module->getBool(false);
  auto *thirdAssign = module->Nr<AssignInstr>(v, third);
  auto *end = module->getBool(false);

  b->push_back(start);
  b->push_back(zeroAssign);
  b->push_back(tc);
  body->push_back(firstAssign);
  except->push_back(thirdAssign);
  finally->push_back(middle);
  finally->push_back(secondAssign);
  b->push_back(end);

  auto c = analyze::dataflow::buildCFGraph(f);
  analyze::dataflow::RDInspector rd(c.get());
  rd.analyze();

  auto startRd = rdset(rd, v, start);
  auto middleRd = rdset(rd, v, middle);
  auto endRd = rdset(rd, v, end);

  ASSERT_EQ(0, startRd.size());
  ASSERT_EQ(1, endRd.size());
  ASSERT_TRUE(endRd.find(second->getId()) != endRd.end());
  ASSERT_TRUE(middleRd.find(first->getId()) != endRd.end());
  ASSERT_TRUE(middleRd.find(third->getId()) != endRd.end());
  ASSERT_TRUE(middleRd.find(initial->getId()) != endRd.end());
}

TEST_F(CIRCoreTest, RDAnalysisWhileLoop) {
  auto *f = module->Nr<BodiedFunc>("test_f");

  auto *loopBody = module->Nr<SeriesFlow>();
  auto *whileFlow = module->Nr<WhileFlow>(module->getBool(false), loopBody);
  auto *b = module->Nr<SeriesFlow>();
  f->setBody(b);

  auto *v = module->Nr<Var>(module->getIntType());
  f->push_back(v);

  auto *first = module->getInt(1);
  auto *second = module->getInt(2);

  auto *start = module->getBool(false);
  auto *firstAssign = module->Nr<AssignInstr>(v, first);
  auto *secondAssign = module->Nr<AssignInstr>(v, second);
  auto *end = module->getBool(false);

  b->push_back(start);
  b->push_back(firstAssign);
  b->push_back(whileFlow);
  loopBody->push_back(secondAssign);
  b->push_back(end);

  auto c = analyze::dataflow::buildCFGraph(f);
  analyze::dataflow::RDInspector rd(c.get());
  rd.analyze();

  auto startRd = rdset(rd, v, start);
  auto endRd = rdset(rd, v, end);

  ASSERT_EQ(0, startRd.size());
  ASSERT_EQ(2, endRd.size());
  ASSERT_TRUE(endRd.find(first->getId()) != endRd.end());
  ASSERT_TRUE(endRd.find(second->getId()) != endRd.end());
}
