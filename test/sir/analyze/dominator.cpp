#include "test.h"

#include "sir/analyze/dataflow/cfg.h"
#include "sir/analyze/dataflow/dominator.h"

using namespace seq::ir;

TEST_F(SIRCoreTest, DominatorAnalysisSimple) {
  auto *f = module->Nr<BodiedFunc>("test_f");
  auto *b = module->Nr<SeriesFlow>();
  f->setBody(b);

  auto *v = module->Nr<Var>(module->getIntType());
  f->push_back(v);

  auto *start = module->getBool(false);
  auto *end = module->getBool(false);

  b->push_back(start);
  b->push_back(end);

  auto c = analyze::dataflow::buildCFGraph(f);
  analyze::dataflow::DominatorInspector dom(c.get());
  dom.analyze();

  ASSERT_TRUE(dom.isDominated(end, start));
  ASSERT_FALSE(dom.isDominated(start, end));
}

TEST_F(SIRCoreTest, DominatorAnalysisTernary) {
  auto *f = module->Nr<BodiedFunc>("test_f");
  auto *b = module->Nr<SeriesFlow>();
  f->setBody(b);

  auto *v = module->Nr<Var>(module->getIntType());
  f->push_back(v);

  auto *start = module->getBool(false);
  auto *middle = module->getBool(false);
  auto *end =
      module->Nr<TernaryInstr>(module->getBool(true), middle, module->getBool(true));

  b->push_back(start);
  b->push_back(end);

  auto c = analyze::dataflow::buildCFGraph(f);
  analyze::dataflow::DominatorInspector dom(c.get());
  dom.analyze();

  ASSERT_TRUE(dom.isDominated(end, start));
  ASSERT_TRUE(dom.isDominated(middle, start));
  ASSERT_FALSE(dom.isDominated(start, end));
  ASSERT_FALSE(dom.isDominated(end, middle));
}
