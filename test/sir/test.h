#include <algorithm>

#include "codon/sir/sir.h"
#include "codon/sir/util/cloning.h"
#include "gtest/gtest.h"

class SIRCoreTest : public testing::Test {
protected:
  std::unique_ptr<codon::ir::Module> module;
  std::unique_ptr<codon::ir::util::CloneVisitor> cv;

  void SetUp() override {
    codon::ir::IdMixin::resetId();
    module = std::make_unique<codon::ir::Module>("test");
    cv = std::make_unique<codon::ir::util::CloneVisitor>(module.get());
  }
};
