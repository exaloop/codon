// Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>

// This file resolves ABI issues with single-precision complex
// math functions.

#include "codon/runtime/lib.h"

#include <complex>

SEQ_FUNC void cnp_cexpf(float r, float i, float *z) {
  std::complex<float> x(r, i);
  auto y = std::exp(x);
  z[0] = y.real();
  z[1] = y.imag();
}

SEQ_FUNC void cnp_clogf(float r, float i, float *z) {
  std::complex<float> x(r, i);
  auto y = std::log(x);
  z[0] = y.real();
  z[1] = y.imag();
}

SEQ_FUNC void cnp_csqrtf(float r, float i, float *z) {
  std::complex<float> x(r, i);
  auto y = std::sqrt(x);
  z[0] = y.real();
  z[1] = y.imag();
}

SEQ_FUNC void cnp_ccoshf(float r, float i, float *z) {
  std::complex<float> x(r, i);
  auto y = std::cosh(x);
  z[0] = y.real();
  z[1] = y.imag();
}

SEQ_FUNC void cnp_csinhf(float r, float i, float *z) {
  std::complex<float> x(r, i);
  auto y = std::sinh(x);
  z[0] = y.real();
  z[1] = y.imag();
}

SEQ_FUNC void cnp_ctanhf(float r, float i, float *z) {
  std::complex<float> x(r, i);
  auto y = std::tanh(x);
  z[0] = y.real();
  z[1] = y.imag();
}

SEQ_FUNC void cnp_cacoshf(float r, float i, float *z) {
  std::complex<float> x(r, i);
  auto y = std::acosh(x);
  z[0] = y.real();
  z[1] = y.imag();
}

SEQ_FUNC void cnp_casinhf(float r, float i, float *z) {
  std::complex<float> x(r, i);
  auto y = std::asinh(x);
  z[0] = y.real();
  z[1] = y.imag();
}

SEQ_FUNC void cnp_catanhf(float r, float i, float *z) {
  std::complex<float> x(r, i);
  auto y = std::atanh(x);
  z[0] = y.real();
  z[1] = y.imag();
}

SEQ_FUNC void cnp_ccosf(float r, float i, float *z) {
  std::complex<float> x(r, i);
  auto y = std::cos(x);
  z[0] = y.real();
  z[1] = y.imag();
}

SEQ_FUNC void cnp_csinf(float r, float i, float *z) {
  std::complex<float> x(r, i);
  auto y = std::sin(x);
  z[0] = y.real();
  z[1] = y.imag();
}

SEQ_FUNC void cnp_ctanf(float r, float i, float *z) {
  std::complex<float> x(r, i);
  auto y = std::tan(x);
  z[0] = y.real();
  z[1] = y.imag();
}

SEQ_FUNC void cnp_cacosf(float r, float i, float *z) {
  std::complex<float> x(r, i);
  auto y = std::acos(x);
  z[0] = y.real();
  z[1] = y.imag();
}

SEQ_FUNC void cnp_casinf(float r, float i, float *z) {
  std::complex<float> x(r, i);
  auto y = std::asin(x);
  z[0] = y.real();
  z[1] = y.imag();
}

SEQ_FUNC void cnp_catanf(float r, float i, float *z) {
  std::complex<float> x(r, i);
  auto y = std::atan(x);
  z[0] = y.real();
  z[1] = y.imag();
}
