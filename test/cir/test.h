#include <algorithm>
#include <fstream>
#include <iterator>
#include <stdexcept>

#include "codon/cir/cir.h"
#include "codon/cir/util/cloning.h"
#include "gtest/gtest.h"

inline std::string readCIRSource(const char *file) {
  auto path = std::string(TEST_DIR) + "/cir/fixtures/" + file;
  std::ifstream input(path);
  if (!input)
    throw std::runtime_error("cannot open CIR fixture " + path);
  return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
}

class CIRCoreTest : public testing::Test {
protected:
  std::unique_ptr<codon::ir::Module> module;
  std::unique_ptr<codon::ir::util::CloneVisitor> cv;

  void SetUp() override {
    codon::ir::IdMixin::resetId();
    module = std::make_unique<codon::ir::Module>(/*cache=*/nullptr, "test");
    cv = std::make_unique<codon::ir::util::CloneVisitor>(module.get());
  }
};
