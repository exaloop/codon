#include "test.h"

#include "codon/cir/util/matching.h"

using namespace codon::ir;

TEST_F(CIRCoreTest, ModuleNodeBuildingRemovalAndIterators) {
  {
    auto n1 = module->Nr<OptionalType>(module->getIntType());
    ASSERT_EQ(n1->getModule(), module.get());
    auto numTypes = std::distance(module->types_begin(), module->types_end());
    ASSERT_TRUE(std::find(module->types_begin(), module->types_end(), n1) !=
                module->types_end());
    module->remove(n1);
    ASSERT_EQ(numTypes - 1, std::distance(module->types_begin(), module->types_end()));
  }
  {
    auto n1 = module->N<IntConst>(codon::SrcInfo{}, 1, module->getIntType());
    ASSERT_EQ(n1->getModule(), module.get());
    auto numVals = std::distance(module->values_begin(), module->values_end());
    ASSERT_TRUE(std::find(module->values_begin(), module->values_end(), n1) !=
                module->values_end());
    module->remove(n1);
    ASSERT_EQ(numVals - 1, std::distance(module->values_begin(), module->values_end()));
  }
  {
    auto n1 = module->Nr<Var>(module->getIntType());
    ASSERT_EQ(n1->getModule(), module.get());
    auto numVars = std::distance(module->begin(), module->end());
    ASSERT_TRUE(std::find(module->begin(), module->end(), n1) != module->end());
    module->remove(n1);
    ASSERT_EQ(numVars - 1, std::distance(module->begin(), module->end()));
  }
}

TEST_F(CIRCoreTest, IntrinsicFactoryReuseAndRollback) {
  using Intrinsic = InternalFunc::Intrinsic;
  auto *integer = module->getIntType();
  auto *query = module->getIntrinsic(Intrinsic::SIZEOF, integer);
  ASSERT_TRUE(query->hasValidIntrinsicSignature());
  ASSERT_TRUE(query->isTypeQuery());
  ASSERT_TRUE(query->isGlobal());
  ASSERT_EQ(query, module->getIntrinsic(Intrinsic::SIZEOF, integer));
  auto *call = module->Nr<CallInstr>(module->Nr<VarValue>(query));
  ASSERT_EQ(call->getType(), integer);
  ASSERT_EQ(query->getIntrinsicSideEffectStatus(), util::PURE);
  ASSERT_FALSE(query->intrinsicDerivesFromArguments());

  auto *atomic = module->getIntrinsic(Intrinsic::IS_ATOMIC, integer);
  ASSERT_NE(query, atomic);
  ASSERT_EQ(cast<FuncType>(atomic->getType())->getReturnType(), module->getBoolType());
  ASSERT_TRUE(atomic->hasValidIntrinsicSignature());

  query->setIntrinsic(Intrinsic::SIZEOF, module->getBoolType());
  auto *replacement = module->getIntrinsic(Intrinsic::SIZEOF, integer);
  ASSERT_NE(query, replacement);
  auto removed = replacement->getId();
  module->remove(replacement);
  replacement = module->getIntrinsic(Intrinsic::SIZEOF, integer);
  ASSERT_NE(removed, replacement->getId());

  module->pushArena();
  auto *temporary = module->unsafeGetMemberedType("temporary.intrinsic.type");
  auto temporaryId = module->getIntrinsic(Intrinsic::SIZEOF, temporary)->getId();
  module->popArena();
  ASSERT_EQ(module->getVar(temporaryId), nullptr);
  ASSERT_EQ(module->getType("temporary.intrinsic.type"), nullptr);
  ASSERT_EQ(replacement, module->getIntrinsic(Intrinsic::SIZEOF, integer));
  temporary = module->unsafeGetMemberedType("temporary.intrinsic.type");
  ASSERT_NE(module->getIntrinsic(Intrinsic::SIZEOF, temporary)->getId(), temporaryId);
}

TEST_F(CIRCoreTest, IntrinsicSignaturesAndEffects) {
  using Intrinsic = InternalFunc::Intrinsic;
  auto *reference = module->unsafeGetMemberedType("intrinsic.reference", true);
  auto *signature =
      module->unsafeGetFuncType("intrinsic.alloc.signature", reference, {});
  auto *allocate = module->getIntrinsic(Intrinsic::ALLOCATE, reference, signature);
  ASSERT_TRUE(allocate->hasValidIntrinsicSignature());
  ASSERT_EQ(allocate->getIntrinsicSideEffectStatus(), util::NO_SIDE_EFFECT);
  ASSERT_FALSE(allocate->intrinsicDerivesFromArguments());

  auto *record = module->unsafeGetMemberedType("intrinsic.record");
  signature = module->unsafeGetFuncType("intrinsic.record.signature", record, {});
  auto *construct = module->getIntrinsic(Intrinsic::RECORD_NEW, record, signature);
  ASSERT_TRUE(construct->intrinsicDerivesFromArguments());
  ASSERT_TRUE(construct->hasValidIntrinsicSignature());

  auto *generator = module->unsafeGetGeneratorType(module->getIntType());
  signature = module->unsafeGetFuncType(
      "intrinsic.promise.signature", module->unsafeGetPointerType(module->getIntType()),
      {generator});
  auto *promise =
      module->getIntrinsic(Intrinsic::GENERATOR_PROMISE, generator, signature);
  ASSERT_TRUE(promise->intrinsicDerivesFromArguments());
  ASSERT_TRUE(promise->hasValidIntrinsicSignature());

  signature = module->unsafeGetFuncType("intrinsic.lookup.signature", reference,
                                        {module->getIntType()});
  auto *lookup = module->getIntrinsic(Intrinsic::TYPEINFO_LOOKUP, nullptr, signature);
  ASSERT_EQ(lookup->getIntrinsicSideEffectStatus(), util::UNKNOWN);
  ASSERT_TRUE(lookup->hasValidIntrinsicSignature());
  lookup->setIntrinsic(Intrinsic::NONE);
  ASSERT_FALSE(lookup->hasValidIntrinsicSignature());
  lookup->setIntrinsic(Intrinsic::SIZEOF, reference);
  ASSERT_FALSE(lookup->hasValidIntrinsicSignature());
}

TEST_F(CIRCoreTest, ModuleMainFunctionAndArgVar) {
  auto *main = module->getMainFunc();
  ASSERT_TRUE(main);
  main->realize(
      module->unsafeGetFuncType("my_func_type", module->getIntType(), {}, false), {});
  auto *mainType = cast<FuncType>(main->getType());
  ASSERT_TRUE(mainType);
  ASSERT_EQ(0, std::distance(mainType->begin(), mainType->end()));
  ASSERT_FALSE(main->isReplaceable());

  auto *argvVar = module->getArgvVar();
  ASSERT_TRUE(argvVar);
  ASSERT_TRUE(util::match(argvVar->getType(),
                          module->unsafeGetPointerType(module->getStringType())));
  ASSERT_FALSE(argvVar->isReplaceable());

  auto *argcVar = module->getArgcVar();
  ASSERT_TRUE(argcVar);
  ASSERT_TRUE(util::match(argcVar->getType(), module->getIntType()));
  ASSERT_FALSE(argcVar->isReplaceable());
}

TEST_F(CIRCoreTest, ModuleTypeGetAndLookup) {
  auto TYPE_NAME = "**test_type**";
  auto *newType = module->unsafeGetMemberedType(TYPE_NAME);
  ASSERT_TRUE(isA<RecordType>(newType));
  ASSERT_EQ(newType, module->getType(TYPE_NAME));
  module->remove(newType);

  newType = module->unsafeGetMemberedType(TYPE_NAME, true);
  ASSERT_TRUE(isA<RefType>(newType));
  ASSERT_EQ(newType, module->getType(TYPE_NAME));
}
