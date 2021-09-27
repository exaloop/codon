#include <algorithm>

#include "sir/sir.h"
#include "sir/util/cloning.h"
#include "gtest/gtest.h"

class SIRCoreTest : public testing::Test {
protected:
  std::unique_ptr<seq::ir::Module> module;
  std::unique_ptr<seq::ir::util::CloneVisitor> cv;

  void SetUp() override {
    seq::ir::IdMixin::resetId();
    module = std::make_unique<seq::ir::Module>("test");
    cv = std::make_unique<seq::ir::util::CloneVisitor>(module.get());
  }
};
