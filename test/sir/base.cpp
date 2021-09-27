#include "test.h"

#include <algorithm>

namespace {
class TestVisitor : public seq::ir::util::Visitor {
public:
  void visit(seq::ir::IntConst *) override { FAIL(); }
  void visit(seq::ir::BoolConst *) override {}
};

class ConstTestVisitor : public seq::ir::util::ConstVisitor {
public:
  void visit(const seq::ir::IntConst *) override { FAIL(); }
  void visit(const seq::ir::BoolConst *) override {}
};

} // namespace

using namespace seq::ir;

TEST_F(SIRCoreTest, NodeNoReplacementRTTI) {
  auto *derived = module->Nr<IntConst>(1, module->getIntType());
  ASSERT_TRUE(derived);
  ASSERT_FALSE(derived->hasReplacement());
  auto *base = cast<Value>(derived);
  ASSERT_TRUE(base);
  ASSERT_TRUE(isA<IntConst>(base));
  ASSERT_TRUE(isA<Const>(base));
  ASSERT_TRUE(isA<Value>(base));
  ASSERT_FALSE(isA<Flow>(base));

  const auto *constBase = base;
  ASSERT_TRUE(isA<Const>(constBase));
  ASSERT_TRUE(cast<Const>(constBase));
}

TEST_F(SIRCoreTest, NodeNoReplacementAttributes) {
  auto *node = module->Nr<IntConst>(1, module->getIntType());
  ASSERT_FALSE(node->hasReplacement());
  ASSERT_FALSE(node->hasAttribute<KeyValueAttribute>());

  ASSERT_TRUE(node->hasAttribute<SrcInfoAttribute>());
  ASSERT_TRUE(node->getAttribute<SrcInfoAttribute>());
  ASSERT_EQ(1, std::distance(node->attributes_begin(), node->attributes_end()));
}

TEST_F(SIRCoreTest, NodeReplacementRTTI) {
  Value *node = module->Nr<IntConst>(1, module->getIntType());
  ASSERT_TRUE(node);
  ASSERT_FALSE(node->hasReplacement());
  ASSERT_TRUE(isA<IntConst>(node));

  node->replaceAll(module->Nr<BoolConst>(false, module->getBoolType()));
  ASSERT_TRUE(node->hasReplacement());
  ASSERT_FALSE(isA<IntConst>(node));
  ASSERT_TRUE(isA<BoolConst>(node));
  ASSERT_TRUE(cast<BoolConst>(node));
}

TEST_F(SIRCoreTest, NodeReplacementDelegates) {
  auto NODE_NAME = "foo";

  Value *originalNode = module->Nr<IntConst>(1, module->getIntType());
  Value *newNode = module->Nr<BoolConst>(false, module->getBoolType(), NODE_NAME);
  newNode->setAttribute(std::make_unique<KeyValueAttribute>());

  ASSERT_EQ(0, originalNode->getName().size());
  ASSERT_EQ(1, std::distance(originalNode->attributes_begin(),
                             originalNode->attributes_end()));

  originalNode->replaceAll(newNode);
  ASSERT_EQ(NODE_NAME, originalNode->getName());
  ASSERT_EQ(2, std::distance(originalNode->attributes_begin(),
                             originalNode->attributes_end()));

  TestVisitor v;
  originalNode->accept(v);
  newNode->accept(v);

  ConstTestVisitor v2;
  originalNode->accept(v2);
  newNode->accept(v2);
}

TEST_F(SIRCoreTest, NodeNonReplaceableFails) {
  Value *originalNode = module->Nr<IntConst>(1, module->getIntType());
  originalNode->setReplaceable(false);
  ASSERT_DEATH(originalNode->replaceAll(originalNode), "");
}
