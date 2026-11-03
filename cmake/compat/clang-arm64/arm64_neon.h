// Compatibility header for Clang releases without an arm64_neon.h wrapper.
// MSVC's header declares neon_* intrinsics that those releases cannot lower.
#ifndef CODON_CLANG_ARM64_NEON_H
#define CODON_CLANG_ARM64_NEON_H

#include <arm_neon.h>

#endif
