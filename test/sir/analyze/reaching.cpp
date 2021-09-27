#include "test.h"

#include "sir/analyze/dataflow/cfg.h"
#include "sir/analyze/dataflow/reaching.h"

using namespace seq::ir;

TEST_F(SIRCoreTest, RDAnalysisSimple) {
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

  auto startRd = rd.getReachingDefinitions(v, start);
  auto firstRd = rd.getReachingDefinitions(v, firstAssign);
  auto secondRd = rd.getReachingDefinitions(v, secondAssign);
  auto endRd = rd.getReachingDefinitions(v, end);

  ASSERT_EQ(0, startRd.size());
  ASSERT_EQ(1, firstRd.size());
  ASSERT_EQ(1, secondRd.size());
  ASSERT_EQ(1, endRd.size());

  ASSERT_EQ(first->getId(), *firstRd.begin());
  ASSERT_EQ(second->getId(), *secondRd.begin());
  ASSERT_EQ(second->getId(), *endRd.begin());
}

TEST_F(SIRCoreTest, RDAnalysisIfConditional) {
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

  auto startRd = rd.getReachingDefinitions(v, start);
  auto endRd = rd.getReachingDefinitions(v, end);

  ASSERT_EQ(0, startRd.size());
  ASSERT_EQ(2, endRd.size());
  ASSERT_TRUE(endRd.find(first->getId()) != endRd.end());
  ASSERT_TRUE(endRd.find(second->getId()) != endRd.end());
}

TEST_F(SIRCoreTest, RDAnalysisTryCatch) {
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

  auto startRd = rd.getReachingDefinitions(v, start);
  auto middleRd = rd.getReachingDefinitions(v, middle);
  auto endRd = rd.getReachingDefinitions(v, end);

  ASSERT_EQ(0, startRd.size());
  ASSERT_EQ(1, endRd.size());
  ASSERT_TRUE(endRd.find(second->getId()) != endRd.end());
  ASSERT_TRUE(middleRd.find(first->getId()) != endRd.end());
  ASSERT_TRUE(middleRd.find(third->getId()) != endRd.end());
  ASSERT_TRUE(middleRd.find(initial->getId()) != endRd.end());
}

TEST_F(SIRCoreTest, RDAnalysisWhileLoop) {
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

  auto startRd = rd.getReachingDefinitions(v, start);
  auto endRd = rd.getReachingDefinitions(v, end);

  ASSERT_EQ(0, startRd.size());
  ASSERT_EQ(2, endRd.size());
  ASSERT_TRUE(endRd.find(first->getId()) != endRd.end());
  ASSERT_TRUE(endRd.find(second->getId()) != endRd.end());
}
