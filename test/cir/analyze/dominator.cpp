#include "test.h"

#include "codon/cir/analyze/dataflow/cfg.h"
#include "codon/cir/analyze/dataflow/dominator.h"

using namespace codon::ir;

TEST_F(CIRCoreTest, DominatorAnalysisSimple) {
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

TEST_F(CIRCoreTest, DominatorAnalysisTernary) {
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

TEST_F(CIRCoreTest, DominatorAnalysisImperativeForUpdate) {
  for (bool useContinue : {false, true}) {
    auto *function = module->Nr<BodiedFunc>("loop_dominance");
    auto *body = module->Nr<SeriesFlow>();
    function->setBody(body);
    auto *index = module->Nr<Var>(module->getIntType());
    function->push_back(index);
    auto *seed = module->getBool(false);
    auto *read = module->Nr<VarValue>(index);
    auto *after = module->getBool(true);
    auto *loopBody = module->Nr<SeriesFlow>();
    loopBody->push_back(read);
    if (useContinue)
      loopBody->push_back(module->Nr<ContinueInstr>());
    body->push_back(seed);
    body->push_back(module->Nr<ImperativeForFlow>(module->getInt(0), 1,
                                                  module->getInt(3), loopBody, index));
    body->push_back(after);

    auto graph = analyze::dataflow::buildCFGraph(function);
    analyze::dataflow::DominatorInspector dominators(graph.get());
    dominators.analyze();
    EXPECT_TRUE(dominators.isDominated(read, seed));
    EXPECT_TRUE(dominators.isDominated(after, seed));
    analyze::dataflow::CFBlock *update = nullptr;
    for (auto *block : *graph)
      if (block->getName() == "forUpdate")
        update = block;
    ASSERT_NE(update, nullptr);
    auto *readBlock = graph->getBlock(read);
    EXPECT_NE(
        std::find(readBlock->successors_begin(), readBlock->successors_end(), update),
        readBlock->successors_end());
  }
}
