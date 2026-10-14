#include "test.h"

#include <algorithm>

#include "codon/cir/util/matching.h"

using namespace codon::ir;

TEST_F(CIRCoreTest, FuncRealizationAndVarInsertionEraseAndIterators) {
  auto *fn = module->Nr<BodiedFunc>();
  fn->realize(
      module->unsafeGetFuncType("<internal_func_type>", module->getIntType(), {}), {});

  auto *fnType = module->unsafeGetFuncType("**test_type**", module->getIntType(),
                                           {module->getIntType()});
  std::vector<std::string> names = {"foo"};
  fn->realize(cast<FuncType>(fnType), names);
  ASSERT_TRUE(fn->isGlobal());

  ASSERT_EQ(1, std::distance(fn->arg_begin(), fn->arg_end()));
  ASSERT_EQ(module->getIntType(), fn->arg_front()->getType());

  auto *var = module->Nr<Var>(module->getIntType(), false, "hi");
  fn->push_back(var);
  ASSERT_EQ(1, std::distance(fn->begin(), fn->end()));
  fn->erase(fn->begin());
  ASSERT_EQ(0, std::distance(fn->begin(), fn->end()));
  fn->insert(fn->begin(), var);
  ASSERT_EQ(1, std::distance(fn->begin(), fn->end()));
  ASSERT_EQ(module->getIntType(), fn->front()->getType());
}

TEST_F(CIRCoreTest, BodiedFuncQueryAndReplace) {
  auto *fn = module->Nr<BodiedFunc>();
  fn->realize(
      module->unsafeGetFuncType("<internal_func_type>", module->getIntType(), {}), {});
  fn->setJIT();
  ASSERT_TRUE(fn->isJIT());

  auto *body = fn->getBody();
  ASSERT_FALSE(body);
  ASSERT_EQ(0, fn->getUsedValues().size());

  body = module->Nr<SeriesFlow>();
  fn->setBody(body);
  ASSERT_EQ(body, fn->getBody());

  auto used = fn->getUsedValues();
  ASSERT_EQ(1, used.size());
  ASSERT_EQ(body, used[0]);

  ASSERT_EQ(1, fn->replaceUsedValue(body, module->Nr<SeriesFlow>()));
  ASSERT_DEATH(fn->replaceUsedValue(fn->getBody(), module->Nr<VarValue>(nullptr)), "");
  ASSERT_NE(fn->getBody(), body);
}

TEST_F(CIRCoreTest, BodiedFuncUnmangledName) {
  auto *fn = module->Nr<BodiedFunc>("Int.foo");
  fn->setUnmangledName("foo");
  fn->realize(
      module->unsafeGetFuncType("<internal_func_type>", module->getIntType(), {}), {});
  ASSERT_EQ("foo", fn->getUnmangledName());
}

TEST_F(CIRCoreTest, BodiedFuncCloning) {
  auto *fn = module->Nr<BodiedFunc>("fn");
  fn->realize(
      module->unsafeGetFuncType("<internal_func_type>", module->getIntType(), {}), {});

  fn->setJIT();
  fn->setBody(module->Nr<SeriesFlow>());
  ASSERT_TRUE(util::match(fn, cv->clone(fn)));
}

TEST_F(CIRCoreTest, ExternalFuncUnmangledNameAndCloning) {
  auto *fn = module->Nr<ExternalFunc>("fn");
  fn->realize(
      module->unsafeGetFuncType("<internal_func_type>", module->getIntType(), {}), {});

  fn->setUnmangledName("foo");
  ASSERT_EQ("foo", fn->getUnmangledName());
  ASSERT_TRUE(util::match(fn, cv->clone(fn)));
}

TEST_F(CIRCoreTest, InternalFuncParentTypeUnmangledNameAndCloning) {
  auto *fn = module->Nr<InternalFunc>("fn.1");
  fn->setUnmangledName("fn");
  fn->realize(
      module->unsafeGetFuncType("<internal_func_type>", module->getIntType(), {}), {});

  fn->setParentType(module->getIntType());
  ASSERT_EQ("fn", fn->getUnmangledName());
  ASSERT_EQ(fn->getParentType(), module->getIntType());
  ASSERT_TRUE(util::match(fn, cv->clone(fn)));
  fn->setIntrinsic(InternalFunc::Intrinsic::TYPEINFO, module->getBoolType());
  auto *cloned = cast<InternalFunc>(cv->forceClone(fn));
  ASSERT_NE(cloned, fn);
  ASSERT_EQ(cloned->getIntrinsic(), InternalFunc::Intrinsic::TYPEINFO);
  ASSERT_EQ(cloned->getIntrinsicType(), module->getBoolType());
  ASSERT_TRUE(util::match(fn, cloned));
  cloned->setIntrinsic(InternalFunc::Intrinsic::ALLOCATE, module->getBoolType());
  ASSERT_FALSE(util::match(fn, cloned));
  cloned->setIntrinsic(InternalFunc::Intrinsic::TYPEINFO, module->getIntType());
  ASSERT_FALSE(util::match(fn, cloned));
  ASSERT_EQ(fn->replaceUsedType(module->getBoolType(), module->getIntType()), 1);
  ASSERT_EQ(fn->getIntrinsicType(), module->getIntType());
}

TEST_F(CIRCoreTest, RuntimeMetadataDependencies) {
  auto *type = module->unsafeGetMemberedType("runtime.dependencies", true);
  auto *method = module->Nr<BodiedFunc>("runtime.method");
  auto *replacement = module->Nr<BodiedFunc>("runtime.replacement");
  Type::RuntimeInfo info;
  info.mro = {type, module->getIntType()};
  info.parameters = {module->getBoolType()};
  info.methods[1] = method;
  type->setRuntimeInfo(std::move(info));
  auto types = type->getUsedTypes();
  ASSERT_NE(std::find(types.begin(), types.end(), module->getIntType()), types.end());
  ASSERT_NE(std::find(types.begin(), types.end(), module->getBoolType()), types.end());
  ASSERT_EQ(type->getUsedVariables(), std::vector<Var *>{method});
  ASSERT_EQ(type->replaceUsedVariable(method, replacement), 1);
  ASSERT_EQ(type->getRuntimeInfo()->methods.at(1), replacement);
  ASSERT_EQ(type->replaceUsedVariable(replacement, nullptr), 1);
  ASSERT_TRUE(type->getUsedVariables().empty());
}

TEST_F(CIRCoreTest, LLVMFuncUnmangledNameQueryAndReplace) {
  auto *fn = module->Nr<LLVMFunc>("fn");
  fn->realize(
      module->unsafeGetFuncType("<internal_func_type>", module->getIntType(), {}), {});

  fn->setLLVMBody("body");
  fn->setLLVMDeclarations("decl");
  fn->setParentType(module->getIntType());

  std::vector<Generic> literals = {Generic(1), Generic(module->getIntType())};
  fn->setLLVMLiterals(literals);

  ASSERT_EQ("body", fn->getLLVMBody());
  ASSERT_EQ("decl", fn->getLLVMDeclarations());

  std::vector<Type *> expectedTypes = {fn->getType(), module->getIntType(),
                                       module->getIntType()};
  ASSERT_EQ(expectedTypes, fn->getUsedTypes());
  ASSERT_EQ(2, fn->replaceUsedType(module->getIntType(), module->getFloatType()));
  ASSERT_EQ(module->getFloatType(), fn->getParentType());
  ASSERT_EQ(module->getFloatType(), fn->literal_back().getTypeValue());
  expectedTypes[1] = expectedTypes[2] = module->getFloatType();
  ASSERT_EQ(expectedTypes, fn->getUsedTypes());

  auto *alias = module->Nr<Var>(module->getBoolType());
  auto *forwarder = module->Nr<Var>(module->getIntType());
  alias->replaceAll(forwarder);
  forwarder->replaceAll(fn);
  const Node *replacedNode = alias;
  ASSERT_EQ(expectedTypes, replacedNode->getUsedTypes());
  ASSERT_EQ(2, alias->replaceUsedType(module->getFloatType(), module->getBoolType()));
  ASSERT_EQ(module->getBoolType(), fn->getParentType());
  ASSERT_EQ(module->getBoolType(), fn->literal_back().getTypeValue());
  expectedTypes[1] = expectedTypes[2] = module->getBoolType();
  ASSERT_EQ(expectedTypes, replacedNode->getUsedTypes());

  auto *replacementType =
      module->unsafeGetFuncType("<replacement_func_type>", module->getBoolType(), {});
  ASSERT_EQ(1, alias->replaceUsedType(fn->getType(), replacementType));
  ASSERT_EQ(replacementType, fn->getType());
  expectedTypes[0] = replacementType;
  ASSERT_EQ(expectedTypes, replacedNode->getUsedTypes());
}
