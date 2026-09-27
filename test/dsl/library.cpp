#include <cstdint>

#ifdef CODON_TEST_COMPILER_PLUGIN
#include "codon/dsl/dsl.h"

extern "C" std::unique_ptr<codon::DSL> load() { return std::make_unique<codon::DSL>(); }

extern "C" int64_t codon_test_compiler_value() { return 17; }
#else
extern "C" int64_t codon_test_runtime_value() { return 42; }
#endif
