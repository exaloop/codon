#include "test.h"

#include <fmt/format.h>
#include <sstream>

using namespace codon::ir;

TEST_F(CIRCoreTest, ConstTypeQueryAndReplace) {
  auto *node = module->Nr<IntConst>(1, module->getIntType());
  ASSERT_EQ(module->getIntType(), node->getType());

  auto usedTypes = node->getUsedTypes();
  ASSERT_EQ(1, usedTypes.size());
  ASSERT_EQ(module->getIntType(), usedTypes[0]);
  ASSERT_EQ(1, node->replaceUsedType(module->getIntType(), module->getIntType()));
}

TEST_F(CIRCoreTest, ConstCloning) {
  auto VALUE = 1;
  auto *node = module->Nr<IntConst>(VALUE, module->getIntType());
  auto *clone = cast<IntConst>(cv->clone(node));

  ASSERT_TRUE(clone);
  ASSERT_EQ(VALUE, clone->getVal());
  ASSERT_EQ(module->getIntType(), clone->getType());
}
