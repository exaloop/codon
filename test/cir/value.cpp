#include "test.h"

using namespace codon::ir;

TEST_F(CIRCoreTest, ValueQueryMethodsDelegate) {
  Value *original = module->Nr<IntConst>(1, module->getIntType(), "foo");
  auto originalRef = original->referenceString();

  auto *fn = module->Nr<BodiedFunc>();
  fn->realize(module->unsafeGetDummyFuncType(), {});
  Value *replacement = module->Nr<VarValue>(fn, "baz");
  original->replaceAll(replacement);

  ASSERT_NE(originalRef, original->referenceString());
  ASSERT_EQ(original->referenceString(), replacement->referenceString());

  ASSERT_EQ(1, original->getUsedVariables().size());

  replacement = module->Nr<CallInstr>(replacement);
  original->replaceAll(replacement);
  ASSERT_EQ(0, original->getUsedVariables().size());
  ASSERT_EQ(1, original->getUsedValues().size());
  ASSERT_EQ(module->getVoidType(), original->getType());

  replacement = module->Nr<TypePropertyInstr>(module->getIntType(),
                                              TypePropertyInstr::Property::SIZEOF);
  original->replaceAll(replacement);
  ASSERT_EQ(0, original->getUsedVariables().size());
  ASSERT_EQ(0, original->getUsedValues().size());
  ASSERT_EQ(1, original->getUsedTypes().size());
}

TEST_F(CIRCoreTest, ValueReplaceMethodsDelegate) {
  Value *original = module->Nr<IntConst>(1, module->getIntType(), "foo");
  auto originalRef = original->referenceString();
  auto originalId = original->getId();
  auto *var = module->Nr<BodiedFunc>();
  Value *replacement = module->Nr<VarValue>(var, "baz");
  original->replaceAll(replacement);

  ASSERT_EQ(1, original->replaceUsedVariable(var, var));

  auto *val = replacement;
  replacement = module->Nr<CallInstr>(replacement);
  original->replaceAll(replacement);
  ASSERT_EQ(1, original->replaceUsedValue(val, val));

  replacement = module->Nr<TypePropertyInstr>(module->getIntType(),
                                              TypePropertyInstr::Property::SIZEOF);
  original->replaceAll(replacement);
  ASSERT_EQ(1, original->replaceUsedType(module->getIntType(), module->getFloatType()));

  ASSERT_NE(originalId, original->getId());
  ASSERT_EQ(original->getId(), replacement->getId());
}
