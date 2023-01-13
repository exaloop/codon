#include "test.h"

#include "codon/cir/util/matching.h"

using namespace codon::ir;

TEST_F(CIRCoreTest, MatchingEquivalentVar) {
  auto *first = module->Nr<Var>(module->getIntType());
  auto *second = module->Nr<Var>(module->getIntType());
  auto *third = module->Nr<Var>(module->getFloatType());
  ASSERT_TRUE(util::match(first, second));
  ASSERT_FALSE(util::match(first, third));
}

TEST_F(CIRCoreTest, MatchingNonEquivalentVar) {
  auto *first = module->Nr<Var>(module->getIntType());
  auto *second = module->Nr<Var>(module->getFloatType());
  ASSERT_FALSE(util::match(first, second));
}

TEST_F(CIRCoreTest, MatchingEquivalentFunc) {
  {
    auto *first = module->Nr<BodiedFunc>();
    first->realize(module->unsafeGetDummyFuncType(), {});
    auto *second = module->Nr<BodiedFunc>();
    second->realize(module->unsafeGetDummyFuncType(), {});

    first->setJIT();
    second->setJIT();

    ASSERT_TRUE(util::match(first, second));
  }
  {
    auto *first = module->Nr<ExternalFunc>();
    first->realize(module->unsafeGetDummyFuncType(), {});
    auto *second = module->Nr<ExternalFunc>();
    second->realize(module->unsafeGetDummyFuncType(), {});

    first->setUnmangledName("baz");
    second->setUnmangledName("baz");

    ASSERT_TRUE(util::match(first, second));
  }
  {
    auto *first = module->Nr<LLVMFunc>();
    first->realize(module->unsafeGetDummyFuncType(), {});
    auto *second = module->Nr<LLVMFunc>();
    second->realize(module->unsafeGetDummyFuncType(), {});

    ASSERT_TRUE(util::match(first, second));
  }
}

TEST_F(CIRCoreTest, MatchingNonEquivalentFunc) {
  {
    auto *first = module->Nr<BodiedFunc>();
    first->realize(module->unsafeGetDummyFuncType(), {});
    auto *second = module->Nr<BodiedFunc>();
    second->realize(module->unsafeGetDummyFuncType(), {});

    first->setJIT();

    ASSERT_FALSE(util::match(first, second));
  }
  {
    auto *first = module->Nr<ExternalFunc>();
    first->realize(module->unsafeGetDummyFuncType(), {});
    auto *second = module->Nr<ExternalFunc>();
    second->realize(module->unsafeGetDummyFuncType(), {});

    first->setUnmangledName("baz");
    second->setUnmangledName("bar");

    ASSERT_FALSE(util::match(first, second));
  }
  {
    auto *first = module->Nr<LLVMFunc>();
    first->realize(module->unsafeGetDummyFuncType(), {});
    auto *second = module->Nr<LLVMFunc>();
    second->realize(module->unsafeGetDummyFuncType(), {});

    first->setLLVMLiterals({types::Generic(1)});

    ASSERT_FALSE(util::match(first, second));
  }
}

TEST_F(CIRCoreTest, MatchingAnyValue) {
  auto *first = module->Nr<VarValue>(module->Nr<Var>(module->getIntType()));
  ASSERT_TRUE(util::match(first, module->Nr<util::AnyValue>()));
}

TEST_F(CIRCoreTest, MatchingVarValue) {
  auto *first = module->Nr<VarValue>(module->Nr<Var>(module->getIntType()));
  auto *second = module->Nr<VarValue>(module->Nr<Var>(module->getIntType()));
  ASSERT_TRUE(util::match(first, second));
  first->setVar(module->Nr<Var>(module->getFloatType()));
  ASSERT_FALSE(util::match(first, second));
}

TEST_F(CIRCoreTest, MatchingPointerValue) {
  auto *first = module->Nr<PointerValue>(module->Nr<Var>(module->getIntType()));
  auto *second = module->Nr<PointerValue>(module->Nr<Var>(module->getIntType()));
  ASSERT_TRUE(util::match(first, second));
  first->setVar(module->Nr<Var>(module->getFloatType()));
  ASSERT_FALSE(util::match(first, second));
}

TEST_F(CIRCoreTest, MatchingSeriesFlow) {
  auto *first = module->Nr<SeriesFlow>();
  auto *second = module->Nr<SeriesFlow>();

  first->push_back(module->Nr<IntConst>(1, module->getIntType()));
  second->push_back(module->Nr<IntConst>(1, module->getIntType()));
  ASSERT_TRUE(util::match(first, second));
  second->push_back(module->Nr<IntConst>(1, module->getIntType()));
  ASSERT_FALSE(util::match(first, second));
}

TEST_F(CIRCoreTest, MatchingIfFlow) {
  auto *cond = module->Nr<BoolConst>(true, module->getBoolType());
  auto *tVal = module->Nr<SeriesFlow>();
  auto *first = module->Nr<IfFlow>(cond, tVal);
  auto *second = module->Nr<IfFlow>(cv->clone(cond), cast<Flow>(cv->clone(tVal)));

  ASSERT_TRUE(util::match(first, second));
  second->setFalseBranch(cast<Flow>(cv->clone(tVal)));
  ASSERT_FALSE(util::match(first, second));
}

TEST_F(CIRCoreTest, MatchingForFlow) {
  auto *body = module->Nr<SeriesFlow>();
  auto *var = module->Nr<Var>(module->getIntType());
  auto *iter = module->Nr<StringConst>("hello", module->getStringType());
  auto *first = module->Nr<ForFlow>(iter, body, var);
  auto *second =
      module->Nr<ForFlow>(cv->clone(iter), cast<Flow>(cv->clone(body)), cv->clone(var));

  ASSERT_TRUE(util::match(first, second));
  second->setIter(module->Nr<StringConst>("foo", module->getStringType()));
  ASSERT_FALSE(util::match(first, second));
}

TEST_F(CIRCoreTest, MatchingIntConst) {
  auto *first = module->Nr<IntConst>(0, module->getIntType());
  auto *second = module->Nr<IntConst>(0, module->getIntType());
  ASSERT_TRUE(util::match(first, second));
  first->setVal(2);
  ASSERT_FALSE(util::match(first, second));
}

TEST_F(CIRCoreTest, MatchingFloatConst) {
  auto *first = module->Nr<FloatConst>(0.0, module->getFloatType());
  auto *second = module->Nr<FloatConst>(0.0, module->getFloatType());
  ASSERT_TRUE(util::match(first, second));
  first->setVal(2.0);
  ASSERT_FALSE(util::match(first, second));
}

TEST_F(CIRCoreTest, MatchingBoolConst) {
  auto *first = module->Nr<BoolConst>(false, module->getBoolType());
  auto *second = module->Nr<BoolConst>(false, module->getBoolType());
  ASSERT_TRUE(util::match(first, second));
  first->setVal(true);
  ASSERT_FALSE(util::match(first, second));
}

TEST_F(CIRCoreTest, MatchingStringConst) {
  auto *first = module->Nr<StringConst>("hi", module->getStringType());
  auto *second = module->Nr<StringConst>("hi", module->getStringType());
  ASSERT_TRUE(util::match(first, second));
  first->setVal("bye");
  ASSERT_FALSE(util::match(first, second));
}

TEST_F(CIRCoreTest, MatchingAssignInstr) {
  auto *var = module->Nr<Var>(module->getIntType());
  auto *val = module->Nr<IntConst>(1, module->getIntType());
  auto *first = module->Nr<AssignInstr>(var, val);
  auto *second = module->Nr<AssignInstr>(cv->clone(var), cv->clone(val));

  ASSERT_TRUE(util::match(first, second));
  second->setRhs(module->Nr<IntConst>(5, module->getIntType()));
  ASSERT_FALSE(util::match(first, second));
}

TEST_F(CIRCoreTest, MatchingExtractInstr) {
  auto FIELD = "foo";
  auto *type = cast<types::RecordType>(module->unsafeGetMemberedType("**internal**"));
  type->realize({module->getIntType()}, {FIELD});
  auto *var = module->Nr<Var>(type);
  auto *val = module->Nr<VarValue>(var);
  auto *first = module->Nr<ExtractInstr>(val, FIELD);
  auto *second = module->Nr<ExtractInstr>(cv->clone(val), FIELD);

  ASSERT_TRUE(util::match(first, second));
  second->setField("");
  ASSERT_FALSE(util::match(first, second));
}

TEST_F(CIRCoreTest, MatchingInsertInstr) {
  auto FIELD = "foo";
  auto *type = cast<types::RecordType>(module->unsafeGetMemberedType("**internal**"));
  type->realize({module->getIntType()}, {FIELD});
  auto *var = module->Nr<Var>(type);
  auto *val = module->Nr<VarValue>(var);
  auto *toInsert = module->Nr<IntConst>(1, module->getIntType());
  auto *first = module->Nr<InsertInstr>(val, FIELD, toInsert);
  auto *second = module->Nr<InsertInstr>(cv->clone(val), FIELD, cv->clone(toInsert));

  ASSERT_TRUE(util::match(first, second));
  second->setField("");
  ASSERT_FALSE(util::match(first, second));
}

TEST_F(CIRCoreTest, MatchingCallInstr) {
  auto *type = module->unsafeGetDummyFuncType();
  auto *func = module->Nr<BodiedFunc>();
  func->realize(type, {});
  auto *func2 = module->Nr<BodiedFunc>();
  func2->realize(module->unsafeGetFuncType("baz", module->getIntType(), {}), {});

  auto *funcVal = module->Nr<VarValue>(func);
  auto *first = module->Nr<CallInstr>(funcVal);
  auto *second = module->Nr<CallInstr>(cv->clone(funcVal));

  ASSERT_TRUE(util::match(first, second));
  second->setCallee(module->Nr<VarValue>(func2));
  ASSERT_FALSE(util::match(first, second));
}

TEST_F(CIRCoreTest, MatchingTernaryInstr) {
  auto *trueValue = module->Nr<BoolConst>(true, module->getBoolType());
  auto *falseValue = module->Nr<BoolConst>(false, module->getBoolType());
  auto *cond = module->Nr<BoolConst>(true, module->getBoolType());

  auto *first = module->Nr<TernaryInstr>(cond, trueValue, falseValue);
  auto *second = module->Nr<TernaryInstr>(cv->clone(cond), cv->clone(trueValue),
                                          cv->clone(falseValue));

  ASSERT_TRUE(util::match(first, second));
  second->setFalseValue(module->Nr<BoolConst>(true, module->getBoolType()));
  ASSERT_FALSE(util::match(first, second));
}
