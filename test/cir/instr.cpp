#include "test.h"

#include "codon/cir/util/matching.h"

using namespace codon::ir;

TEST_F(CIRCoreTest, AssignInstrQueryAndReplace) {
  auto *var = module->Nr<Var>(module->getIntType());
  auto *val = module->Nr<IntConst>(1, module->getIntType());
  auto *instr = module->Nr<AssignInstr>(var, val);

  ASSERT_EQ(var, instr->getLhs());
  ASSERT_EQ(val, instr->getRhs());

  auto usedVals = instr->getUsedValues();
  ASSERT_EQ(1, usedVals.size());
  ASSERT_EQ(val, usedVals[0]);

  auto usedVars = instr->getUsedVariables();
  ASSERT_EQ(1, usedVars.size());
  ASSERT_EQ(var, usedVars[0]);

  ASSERT_EQ(
      1, instr->replaceUsedValue(val, module->Nr<IntConst>(1, module->getIntType())));
  ASSERT_EQ(1, instr->replaceUsedVariable(var, module->Nr<Var>(module->getIntType())));
}

TEST_F(CIRCoreTest, AssignInstrCloning) {
  auto *var = module->Nr<Var>(module->getIntType());
  auto *val = module->Nr<IntConst>(1, module->getIntType());
  auto *instr = module->Nr<AssignInstr>(var, val);

  ASSERT_TRUE(util::match(instr, cv->clone(instr)));
}

TEST_F(CIRCoreTest, ExtractInstrQueryAndReplace) {
  auto FIELD = "foo";
  auto *type = cast<types::RecordType>(module->unsafeGetMemberedType("**internal**"));
  type->realize({module->getIntType()}, {FIELD});
  auto *var = module->Nr<Var>(type);
  auto *val = module->Nr<VarValue>(var);
  auto *instr = module->Nr<ExtractInstr>(val, FIELD);

  ASSERT_EQ(FIELD, instr->getField());
  ASSERT_EQ(val, instr->getVal());
  ASSERT_EQ(type->getMemberType(FIELD), instr->getType());

  auto usedVals = instr->getUsedValues();
  ASSERT_EQ(1, usedVals.size());
  ASSERT_EQ(val, usedVals[0]);

  ASSERT_EQ(1, instr->replaceUsedValue(val, module->Nr<VarValue>(var)));
}

TEST_F(CIRCoreTest, ExtractInstrCloning) {
  auto FIELD = "foo";
  auto *type = cast<types::RecordType>(module->unsafeGetMemberedType("**internal**"));
  type->realize({module->getIntType()}, {FIELD});
  auto *var = module->Nr<Var>(type);
  auto *val = module->Nr<VarValue>(var);
  auto *instr = module->Nr<ExtractInstr>(val, FIELD);

  ASSERT_TRUE(util::match(instr, cv->clone(instr)));
}

TEST_F(CIRCoreTest, InsertInstrQueryAndReplace) {
  auto FIELD = "foo";
  auto *type =
      cast<types::RefType>(module->unsafeGetMemberedType("**internal**", true));
  type->realize({module->getIntType()}, {FIELD});
  auto *var = module->Nr<Var>(type);
  auto *lhs = module->Nr<VarValue>(var);
  auto *rhs = module->Nr<IntConst>(1, module->getIntType());
  auto *instr = module->Nr<InsertInstr>(lhs, FIELD, rhs);

  ASSERT_EQ(type, instr->getType());

  auto usedVals = instr->getUsedValues();
  ASSERT_EQ(2, usedVals.size());

  ASSERT_EQ(1, instr->replaceUsedValue(lhs, module->Nr<VarValue>(var)));
  ASSERT_EQ(
      1, instr->replaceUsedValue(rhs, module->Nr<IntConst>(1, module->getIntType())));
}

TEST_F(CIRCoreTest, InsertInstrCloning) {
  auto FIELD = "foo";
  auto *type =
      cast<types::RefType>(module->unsafeGetMemberedType("**internal**", true));
  type->realize({module->getIntType()}, {FIELD});
  auto *var = module->Nr<Var>(type);
  auto *lhs = module->Nr<VarValue>(var);
  auto *rhs = module->Nr<IntConst>(1, module->getIntType());
  auto *instr = module->Nr<InsertInstr>(lhs, FIELD, rhs);

  ASSERT_TRUE(util::match(instr, cv->clone(instr)));
}

TEST_F(CIRCoreTest, CallInstrQueryAndReplace) {
  auto *type = cast<types::FuncType>(module->unsafeGetDummyFuncType());
  auto *func = module->Nr<BodiedFunc>();
  func->realize(type, {});
  auto *funcVal = module->Nr<VarValue>(func);
  auto *instr = module->Nr<CallInstr>(funcVal);

  ASSERT_EQ(funcVal, instr->getCallee());
  ASSERT_EQ(type->getReturnType(), instr->getType());

  auto usedVals = instr->getUsedValues();
  ASSERT_EQ(1, usedVals.size());
  ASSERT_EQ(1, instr->replaceUsedValue(funcVal, module->Nr<VarValue>(func)));
}

TEST_F(CIRCoreTest, CallInstrCloning) {
  auto *type = module->unsafeGetDummyFuncType();
  auto *func = module->Nr<BodiedFunc>();
  func->realize(type, {});
  auto *funcVal = module->Nr<VarValue>(func);
  auto *instr = module->Nr<CallInstr>(funcVal);

  ASSERT_TRUE(util::match(instr, cv->clone(instr)));
}

TEST_F(CIRCoreTest, StackAllocInstrQueryAndReplace) {
  auto COUNT = 1;

  auto *arrayType = module->unsafeGetArrayType(module->getIntType());
  auto *instr = module->Nr<StackAllocInstr>(arrayType, COUNT);

  ASSERT_EQ(COUNT, instr->getCount());
  ASSERT_EQ(arrayType, instr->getType());

  auto usedTypes = instr->getUsedTypes();
  ASSERT_EQ(1, usedTypes.size());
  ASSERT_EQ(arrayType, usedTypes[0]);

  ASSERT_EQ(1, instr->replaceUsedType(
                   arrayType, module->unsafeGetArrayType(module->getFloatType())));
}

TEST_F(CIRCoreTest, StackAllocInstrCloning) {
  auto COUNT = 1;
  auto *arrayType = module->unsafeGetArrayType(module->getIntType());
  auto *instr = module->Nr<StackAllocInstr>(arrayType, COUNT);
  ASSERT_TRUE(util::match(instr, cv->clone(instr)));
}

TEST_F(CIRCoreTest, TypePropertyInstrQueryAndReplace) {
  auto *type = module->unsafeGetArrayType(module->getIntType());
  auto *instr =
      module->Nr<TypePropertyInstr>(type, TypePropertyInstr::Property::IS_ATOMIC);

  ASSERT_EQ(type, instr->getInspectType());
  ASSERT_EQ(TypePropertyInstr::Property::IS_ATOMIC, instr->getProperty());
  ASSERT_EQ(module->getBoolType(), instr->getType());

  instr->setProperty(TypePropertyInstr::Property::SIZEOF);
  ASSERT_EQ(module->getIntType(), instr->getType());

  auto usedTypes = instr->getUsedTypes();
  ASSERT_EQ(1, usedTypes.size());
  ASSERT_EQ(type, usedTypes[0]);

  ASSERT_EQ(1, instr->replaceUsedType(
                   type, module->unsafeGetArrayType(module->getFloatType())));
}

TEST_F(CIRCoreTest, TypePropertyInstrCloning) {
  auto *type = module->unsafeGetArrayType(module->getIntType());
  auto *instr =
      module->Nr<TypePropertyInstr>(type, TypePropertyInstr::Property::IS_ATOMIC);
  ASSERT_TRUE(util::match(instr, cv->clone(instr)));
}

TEST_F(CIRCoreTest, YieldInInstrQueryAndReplace) {
  auto *type = module->unsafeGetArrayType(module->getIntType());
  auto *instr = module->Nr<YieldInInstr>(type);

  ASSERT_EQ(type, instr->getType());

  auto usedTypes = instr->getUsedTypes();
  ASSERT_EQ(1, usedTypes.size());
  ASSERT_EQ(type, usedTypes[0]);

  ASSERT_EQ(1, instr->replaceUsedType(
                   type, module->unsafeGetArrayType(module->getFloatType())));
}

TEST_F(CIRCoreTest, YieldInInstrCloning) {
  auto *type = module->unsafeGetArrayType(module->getIntType());
  auto *instr = module->Nr<YieldInInstr>(type);
  ASSERT_TRUE(util::match(instr, cv->clone(instr)));
}

TEST_F(CIRCoreTest, TernaryInstrQueryAndReplace) {
  auto *trueValue = module->Nr<BoolConst>(true, module->getBoolType());
  auto *falseValue = module->Nr<BoolConst>(false, module->getBoolType());
  auto *cond = module->Nr<BoolConst>(true, module->getBoolType());
  auto *instr = module->Nr<TernaryInstr>(cond, trueValue, falseValue);

  ASSERT_EQ(trueValue, instr->getTrueValue());
  ASSERT_EQ(falseValue, instr->getFalseValue());
  ASSERT_EQ(cond, instr->getCond());

  ASSERT_EQ(3, instr->getUsedValues().size());

  ASSERT_EQ(1, instr->replaceUsedValue(
                   cond, module->Nr<BoolConst>(true, module->getBoolType())));
  ASSERT_EQ(1, instr->replaceUsedValue(
                   trueValue, module->Nr<BoolConst>(true, module->getBoolType())));
  ASSERT_EQ(1, instr->replaceUsedValue(
                   falseValue, module->Nr<BoolConst>(true, module->getBoolType())));
}

TEST_F(CIRCoreTest, TernaryInstrCloning) {
  auto *trueValue = module->Nr<BoolConst>(true, module->getBoolType());
  auto *falseValue = module->Nr<BoolConst>(false, module->getBoolType());
  auto *cond = module->Nr<BoolConst>(true, module->getBoolType());
  auto *instr = module->Nr<TernaryInstr>(cond, trueValue, falseValue);

  ASSERT_TRUE(util::match(instr, cv->clone(instr)));
}

TEST_F(CIRCoreTest, ContinueInstrQueryReplaceAndCloning) {
  auto *instr = module->Nr<ContinueInstr>();
  ASSERT_TRUE(util::match(instr, cv->clone(instr)));
}

TEST_F(CIRCoreTest, BreakInstrQueryReplaceAndCloning) {
  auto *instr = module->Nr<BreakInstr>();
  ASSERT_TRUE(util::match(instr, cv->clone(instr)));
}

TEST_F(CIRCoreTest, ReturnInstrQueryReplaceAndCloning) {
  auto *val = module->Nr<IntConst>(1, module->getIntType());
  auto *instr = module->Nr<ReturnInstr>(val);
  ASSERT_EQ(val, instr->getValue());

  ASSERT_EQ(1, instr->getUsedValues().size());
  ASSERT_EQ(1, instr->replaceUsedValue(val, nullptr));
  ASSERT_EQ(0, instr->getUsedValues().size());

  ASSERT_TRUE(util::match(instr, cv->clone(instr)));
}

TEST_F(CIRCoreTest, YieldInstrQueryReplaceAndCloning) {
  auto *val = module->Nr<IntConst>(1, module->getIntType());
  auto *instr = module->Nr<YieldInstr>(val);
  ASSERT_EQ(val, instr->getValue());

  ASSERT_EQ(1, instr->getUsedValues().size());
  ASSERT_EQ(1, instr->replaceUsedValue(val, nullptr));
  ASSERT_EQ(0, instr->getUsedValues().size());

  ASSERT_TRUE(util::match(instr, cv->clone(instr)));
}

TEST_F(CIRCoreTest, ThrowInstrQueryReplaceAndCloning) {
  auto *val = module->Nr<IntConst>(1, module->getIntType());
  auto *instr = module->Nr<ThrowInstr>(val);
  ASSERT_EQ(val, instr->getValue());

  ASSERT_EQ(1, instr->getUsedValues().size());
  ASSERT_EQ(1, instr->replaceUsedValue(val, nullptr));
  ASSERT_EQ(0, instr->getUsedValues().size());

  ASSERT_TRUE(util::match(instr, cv->clone(instr)));
}

TEST_F(CIRCoreTest, FlowInstrQueryAndReplace) {
  auto *flow = module->Nr<SeriesFlow>();
  auto *val = module->Nr<IntConst>(1, module->getIntType());
  auto *instr = module->Nr<FlowInstr>(flow, val);

  ASSERT_EQ(module->getIntType(), instr->getType());
  ASSERT_EQ(val, instr->getValue());
  ASSERT_EQ(flow, instr->getFlow());

  ASSERT_EQ(2, instr->getUsedValues().size());
  ASSERT_EQ(
      1, instr->replaceUsedValue(val, module->Nr<IntConst>(2, module->getIntType())));
  ASSERT_EQ(1, instr->replaceUsedValue(flow, module->Nr<SeriesFlow>()));
}

TEST_F(CIRCoreTest, FlowInstrCloning) {
  auto *flow = module->Nr<SeriesFlow>();
  auto *val = module->Nr<IntConst>(1, module->getIntType());
  auto *instr = module->Nr<FlowInstr>(flow, val);
  ASSERT_TRUE(util::match(instr, cv->clone(instr)));
}
