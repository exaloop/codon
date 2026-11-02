// Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>

#ifndef CODON_QUAD_ABI_H
#define CODON_QUAD_ABI_H

// LLVM's x86-64 quad libcalls pass binary128 values in XMM registers. Clang's
// Windows C ABI passes them indirectly. Vectorcall supplies the register ABI
// while preserving Windows callee-saved registers; asm names avoid decoration.
// The LLVM assembler-name escape suppresses vectorcall name decoration.
#define QUAD_ABI __attribute__((vectorcall))
QUAD_ABI fp_t __addtf3(fp_t, fp_t) __asm__("\01__addtf3");
QUAD_ABI fp_t __subtf3(fp_t, fp_t) __asm__("\01__subtf3");
QUAD_ABI fp_t __multf3(fp_t, fp_t) __asm__("\01__multf3");
QUAD_ABI fp_t __divtf3(fp_t, fp_t) __asm__("\01__divtf3");
QUAD_ABI long long __letf2(fp_t, fp_t) __asm__("\01__letf2");
QUAD_ABI long long __getf2(fp_t, fp_t) __asm__("\01__getf2");
QUAD_ABI long long __eqtf2(fp_t, fp_t) __asm__("\01__eqtf2");
QUAD_ABI long long __netf2(fp_t, fp_t) __asm__("\01__netf2");
QUAD_ABI long long __lttf2(fp_t, fp_t) __asm__("\01__lttf2");
QUAD_ABI long long __gttf2(fp_t, fp_t) __asm__("\01__gttf2");
QUAD_ABI long long __unordtf2(fp_t, fp_t) __asm__("\01__unordtf2");
QUAD_ABI fp_t __extenddftf2(double) __asm__("\01__extenddftf2");
QUAD_ABI fp_t __extendsftf2(float) __asm__("\01__extendsftf2");
QUAD_ABI fp_t __extendhftf2(_Float16) __asm__("\01__extendhftf2");
QUAD_ABI double __trunctfdf2(fp_t) __asm__("\01__trunctfdf2");
QUAD_ABI float __trunctfsf2(fp_t) __asm__("\01__trunctfsf2");
QUAD_ABI _Float16 __trunctfhf2(fp_t) __asm__("\01__trunctfhf2");
#undef QUAD_ABI

#endif
