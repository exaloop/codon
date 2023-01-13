#include "test.h"

#include "codon/cir/util/matching.h"

using namespace codon::ir;

TEST_F(CIRCoreTest, ModuleNodeBuildingRemovalAndIterators) {
  {
    auto n1 = module->Nr<types::OptionalType>(module->getIntType());
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

TEST_F(CIRCoreTest, ModuleMainFunctionAndArgVar) {
  auto *main = module->getMainFunc();
  ASSERT_TRUE(main);
  auto *mainType = cast<types::FuncType>(main->getType());
  ASSERT_TRUE(mainType);
  ASSERT_TRUE(util::match(mainType->getReturnType(), module->getVoidType()));
  ASSERT_EQ(0, std::distance(mainType->begin(), mainType->end()));
  ASSERT_FALSE(main->isReplaceable());

  auto *argVar = module->getArgVar();
  ASSERT_TRUE(argVar);
  ASSERT_TRUE(util::match(argVar->getType(),
                          module->unsafeGetArrayType(module->getStringType())));
  ASSERT_FALSE(argVar->isReplaceable());
}

TEST_F(CIRCoreTest, ModuleTypeGetAndLookup) {
  auto TYPE_NAME = "**test_type**";
  auto *newType = module->unsafeGetMemberedType(TYPE_NAME);
  ASSERT_TRUE(isA<types::RecordType>(newType));
  ASSERT_EQ(newType, module->getType(TYPE_NAME));
  module->remove(newType);

  newType = module->unsafeGetMemberedType(TYPE_NAME, true);
  ASSERT_TRUE(isA<types::RefType>(newType));
  ASSERT_EQ(newType, module->getType(TYPE_NAME));
}
