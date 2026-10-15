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
  auto *type = cast<RecordType>(module->unsafeGetMemberedType("**internal**"));
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
  auto *type = cast<RecordType>(module->unsafeGetMemberedType("**internal**"));
  type->realize({module->getIntType()}, {FIELD});
  auto *var = module->Nr<Var>(type);
  auto *val = module->Nr<VarValue>(var);
  auto *instr = module->Nr<ExtractInstr>(val, FIELD);

  ASSERT_TRUE(util::match(instr, cv->clone(instr)));
}

TEST_F(CIRCoreTest, InsertInstrQueryAndReplace) {
  auto FIELD = "foo";
  auto *type = cast<RefType>(module->unsafeGetMemberedType("**internal**", true));
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
  auto *type = cast<RefType>(module->unsafeGetMemberedType("**internal**", true));
  type->realize({module->getIntType()}, {FIELD});
  auto *var = module->Nr<Var>(type);
  auto *lhs = module->Nr<VarValue>(var);
  auto *rhs = module->Nr<IntConst>(1, module->getIntType());
  auto *instr = module->Nr<InsertInstr>(lhs, FIELD, rhs);

  ASSERT_TRUE(util::match(instr, cv->clone(instr)));
}

TEST_F(CIRCoreTest, CallInstrQueryAndReplace) {
  auto *type = cast<FuncType>(
      module->unsafeGetFuncType("<internal_func_type>", module->getIntType(), {}));
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
  auto *type =
      module->unsafeGetFuncType("<internal_func_type>", module->getIntType(), {});
  auto *func = module->Nr<BodiedFunc>();
  func->realize(type, {});
  auto *funcVal = module->Nr<VarValue>(func);
  auto *instr = module->Nr<CallInstr>(funcVal);

  ASSERT_TRUE(util::match(instr, cv->clone(instr)));
}

TEST_F(CIRCoreTest, PolymorphicInstructionMetadata) {
  auto *type = cast<RefType>(module->unsafeGetMemberedType("polymorphic", true));
  type->realize({module->getIntType()}, {"field"});
  type->setPolymorphic();
  auto *receiver = module->Nr<VarValue>(module->Nr<Var>(type));
  auto *value = module->getInt(42);
  auto *extract = module->Nr<ExtractInstr>(receiver, "field", "extract", true);
  auto *insert = module->Nr<InsertInstr>(receiver, "field", value, "insert", true);
  auto *function = module->Nr<BodiedFunc>();
  function->realize(module->unsafeGetFuncType("method", module->getIntType(), {type}),
                    {"self"});
  auto *callee = module->Nr<VarValue>(function);
  auto *call = module->Nr<CallInstr>(callee, std::vector<Value *>{receiver}, "call", 7);

  auto *extractClone = cast<ExtractInstr>(cv->clone(extract));
  auto *insertClone = cast<InsertInstr>(cv->clone(insert));
  auto *callClone = cast<CallInstr>(cv->clone(call));
  ASSERT_TRUE(extractClone->isPolymorphic());
  ASSERT_TRUE(insertClone->isPolymorphic());
  ASSERT_EQ(callClone->getThunkID(), 7);
  ASSERT_EQ(extractClone->getName(), "extract");
  ASSERT_EQ(insertClone->getName(), "insert");
  ASSERT_EQ(callClone->getName(), "call");
  ASSERT_TRUE(util::match(extract, extractClone));
  ASSERT_TRUE(util::match(insert, insertClone));
  ASSERT_TRUE(util::match(call, callClone));
  ASSERT_FALSE(util::match(extract, module->Nr<ExtractInstr>(receiver, "field")));
  ASSERT_FALSE(util::match(insert, module->Nr<InsertInstr>(receiver, "field", value)));
  ASSERT_FALSE(
      util::match(call, module->Nr<CallInstr>(callee, std::vector<Value *>{receiver})));
  ASSERT_FALSE(util::match(
      call, module->Nr<CallInstr>(callee, std::vector<Value *>{receiver}, "", 8)));

  extract->setPolymorphic(false);
  insert->setPolymorphic(false);
  ASSERT_FALSE(extract->isPolymorphic());
  ASSERT_FALSE(insert->isPolymorphic());
  ASSERT_FALSE(util::match(extract, extractClone));
  ASSERT_FALSE(util::match(insert, insertClone));
  ASSERT_TRUE(
      util::match(extract, module->Nr<ExtractInstr>(receiver, "field", "extract")));
  ASSERT_TRUE(
      util::match(insert, module->Nr<InsertInstr>(receiver, "field", value, "insert")));
  util::CloneVisitor updatedCloner(module.get());
  ASSERT_FALSE(cast<ExtractInstr>(updatedCloner.clone(extract))->isPolymorphic());
  ASSERT_FALSE(cast<InsertInstr>(updatedCloner.clone(insert))->isPolymorphic());

  extract->setPolymorphic(true);
  insert->setPolymorphic(true);
  ASSERT_TRUE(extract->isPolymorphic());
  ASSERT_TRUE(insert->isPolymorphic());
  ASSERT_TRUE(util::match(extract, extractClone));
  ASSERT_TRUE(util::match(insert, insertClone));
}

TEST_F(CIRCoreTest, StackAllocInstrQueryAndReplace) {
  auto COUNT = 1;

  auto *arrayType = module->unsafeGetPointerType(module->getIntType());
  auto *instr = module->Nr<StackAllocInstr>(arrayType, COUNT);

  ASSERT_EQ(COUNT, instr->getCount());
  ASSERT_EQ(arrayType, instr->getType());

  auto usedTypes = instr->getUsedTypes();
  ASSERT_EQ(1, usedTypes.size());
  ASSERT_EQ(arrayType, usedTypes[0]);

  ASSERT_EQ(1, instr->replaceUsedType(
                   arrayType, module->unsafeGetPointerType(module->getFloatType())));
}

TEST_F(CIRCoreTest, StackAllocInstrCloning) {
  auto COUNT = 1;
  auto *arrayType = module->unsafeGetPointerType(module->getIntType());
  auto *instr = module->Nr<StackAllocInstr>(arrayType, COUNT);
  ASSERT_TRUE(util::match(instr, cv->clone(instr)));
}

TEST_F(CIRCoreTest, IntrinsicQueryCallTypesAndReplacement) {
  auto *type = module->unsafeGetPointerType(module->getIntType());
  auto *atomic = module->getIntrinsic(InternalFunc::Intrinsic::IS_ATOMIC, type);
  auto *callee = module->Nr<VarValue>(atomic);
  auto *instr = module->Nr<CallInstr>(callee);
  ASSERT_EQ(module->getBoolType(), instr->getType());
  ASSERT_EQ(0, instr->numArgs());
  auto *size = module->getIntrinsic(InternalFunc::Intrinsic::SIZEOF, type);
  ASSERT_EQ(1, instr->replaceUsedValue(callee, module->Nr<VarValue>(size)));
  ASSERT_EQ(module->getIntType(), instr->getType());
}

TEST_F(CIRCoreTest, IntrinsicQueryCloningAndTypeDependencies) {
  auto *type = module->unsafeGetPointerType(module->getIntType());
  auto *query = module->getIntrinsic(InternalFunc::Intrinsic::IS_ATOMIC, type);
  auto *instr = module->Nr<CallInstr>(module->Nr<VarValue>(query));
  ASSERT_TRUE(util::match(instr, cv->clone(instr)));
  auto *cloned = cast<InternalFunc>(cv->forceClone(query));
  ASSERT_NE(query, cloned);
  ASSERT_TRUE(util::match(query, cloned));
  ASSERT_TRUE(cloned->hasValidIntrinsicSignature());
  auto usedTypes = cloned->getUsedTypes();
  ASSERT_NE(std::find(usedTypes.begin(), usedTypes.end(), type), usedTypes.end());
  auto *replacement = module->unsafeGetPointerType(module->getFloatType());
  ASSERT_EQ(1, cloned->replaceUsedType(type, replacement));
  ASSERT_EQ(replacement, cloned->getIntrinsicType());
  ASSERT_FALSE(util::match(query, cloned));
  ASSERT_TRUE(cloned->hasValidIntrinsicSignature());
}

TEST_F(CIRCoreTest, YieldInInstrQueryAndReplace) {
  auto *type = module->unsafeGetPointerType(module->getIntType());
  auto *instr = module->Nr<YieldInInstr>(type);

  ASSERT_EQ(type, instr->getType());

  auto usedTypes = instr->getUsedTypes();
  ASSERT_EQ(1, usedTypes.size());
  ASSERT_EQ(type, usedTypes[0]);

  ASSERT_EQ(1, instr->replaceUsedType(
                   type, module->unsafeGetPointerType(module->getFloatType())));
}

TEST_F(CIRCoreTest, YieldInInstrCloning) {
  auto *type = module->unsafeGetPointerType(module->getIntType());
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
  ASSERT_EQ(nullptr, instr->getFallback());

  ASSERT_EQ(1, instr->getUsedValues().size());
  ASSERT_EQ(1, instr->replaceUsedValue(val, nullptr));
  ASSERT_EQ(0, instr->getUsedValues().size());

  ASSERT_TRUE(util::match(instr, cv->clone(instr)));

  auto *bare = module->Nr<ThrowInstr>();
  bare->setFallback(val);
  ASSERT_EQ(val, bare->getFallback());
  ASSERT_EQ(1, bare->getUsedValues().size());
  ASSERT_FALSE(util::match(instr, bare));
  auto *cloned = cast<ThrowInstr>(cv->clone(bare));
  ASSERT_NE(nullptr, cloned);
  ASSERT_TRUE(util::match(bare, cloned));
  ASSERT_TRUE(util::match(val, cloned->getFallback()));
  ASSERT_EQ(1, bare->replaceUsedValue(val, nullptr));
  ASSERT_EQ(nullptr, bare->getFallback());
  ASSERT_EQ(0, bare->getUsedValues().size());
  ASSERT_TRUE(util::match(instr, bare));
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
