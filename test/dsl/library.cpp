#include <cstdint>

#ifdef _WIN32
#define TEST_PLUGIN_EXPORT __declspec(dllexport)
#else
#define TEST_PLUGIN_EXPORT
#endif

#ifdef CODON_TEST_COMPILER_PLUGIN
#include "codon/dsl/dsl.h"

extern "C" TEST_PLUGIN_EXPORT std::unique_ptr<codon::DSL> load() {
  return std::make_unique<codon::DSL>();
}

extern "C" TEST_PLUGIN_EXPORT int64_t codon_test_compiler_value() { return 17; }
#else
extern "C" TEST_PLUGIN_EXPORT int64_t codon_test_runtime_value() { return 42; }
#endif
