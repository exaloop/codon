#include "test.h"

using namespace codon::ir;

TEST_F(CIRCoreTest, VarQueryMethodsDelegate) {
  Var *original = module->Nr<Var>(module->getIntType());
  Var *replacement = module->Nr<Var>(module->getFloatType());
  original->replaceAll(replacement);

  ASSERT_EQ(module->getFloatType(), original->getType());
  ASSERT_EQ(module->getFloatType(), original->getUsedTypes().back());
}

TEST_F(CIRCoreTest, VarReplaceMethodsDelegate) {
  Var *original = module->Nr<Var>(module->getIntType());
  Var *replacement = module->Nr<Var>(module->getFloatType());

  auto originalId = original->getId();
  original->replaceAll(replacement);

  ASSERT_EQ(1, original->replaceUsedType(module->getFloatType(), module->getIntType()));
  ASSERT_NE(originalId, original->getId());
  ASSERT_EQ(original->getId(), replacement->getId());
}
