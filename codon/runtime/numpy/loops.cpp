// Copyright (C) 2022-2025 Exaloop Inc. <https://exaloop.io>

#include "codon/runtime/lib.h"

// clang-format off
#undef HWY_TARGET_INCLUDE
#define HWY_TARGET_INCLUDE "codon/runtime/numpy/loops.cpp"
#include "hwy/foreach_target.h"
#include "hwy/highway.h"
#include "hwy/contrib/math/math-inl.h"
// clang-format on

#include <cmath>
#include <cstring>
#include <limits>

HWY_BEFORE_NAMESPACE();

namespace {
namespace HWY_NAMESPACE {

namespace hn = hwy::HWY_NAMESPACE;

struct AcosFunctor {
  template <typename T, typename V>
  static inline auto vector(const hn::ScalableTag<T> d, const V &v) {
    return Acos(d, v);
  }

  static inline double scalar(const double x) { return acos(x); }

  static inline float scalar(const float x) { return acosf(x); }
};

struct AcoshFunctor {
  template <typename T, typename V>
  static inline auto vector(const hn::ScalableTag<T> d, const V &v) {
    const auto nan = Set(d, std::numeric_limits<T>::quiet_NaN());
    const auto pinf = Set(d, std::numeric_limits<T>::infinity());
    const auto pone = Set(d, static_cast<T>(1.0));
    return IfThenElse(v == pinf, pinf, IfThenElse(v < pone, nan, Acosh(d, v)));
  }

  static inline double scalar(const double x) { return acosh(x); }

  static inline float scalar(const float x) { return acoshf(x); }
};

struct AsinFunctor {
  template <typename T, typename V>
  static inline auto vector(const hn::ScalableTag<T> d, const V &v) {
    return Asin(d, v);
  }

  static inline double scalar(const double x) { return asin(x); }

  static inline float scalar(const float x) { return asinf(x); }
};

struct AsinhFunctor {
  template <typename T, typename V>
  static inline auto vector(const hn::ScalableTag<T> d, const V &v) {
    const auto pinf = Set(d, std::numeric_limits<T>::infinity());
    const auto ninf = Set(d, -std::numeric_limits<T>::infinity());
    const auto zero = Set(d, static_cast<T>(0.0));
    return IfThenElse(IsNaN(v), v, IfThenElse(IsInf(v), v, Asinh(d, v)));
  }

  static inline double scalar(const double x) { return asinh(x); }

  static inline float scalar(const float x) { return asinhf(x); }
};

struct AtanFunctor {
  template <typename T, typename V>
  static inline auto vector(const hn::ScalableTag<T> d, const V &v) {
    const auto ppi2 = Set(d, static_cast<T>(+3.14159265358979323846264 / 2));
    const auto npi2 = Set(d, static_cast<T>(-3.14159265358979323846264 / 2));
    const auto pinf = Set(d, std::numeric_limits<T>::infinity());
    const auto ninf = Set(d, -std::numeric_limits<T>::infinity());
    return IfThenElse(v == pinf, ppi2, IfThenElse(v == ninf, npi2, Atan(d, v)));
  }

  static inline double scalar(const double x) { return atan(x); }

  static inline float scalar(const float x) { return atanf(x); }
};

struct AtanhFunctor {
  template <typename T, typename V>
  static inline auto vector(const hn::ScalableTag<T> d, const V &v) {
    const auto nan = Set(d, std::numeric_limits<T>::quiet_NaN());
    const auto pinf = Set(d, std::numeric_limits<T>::infinity());
    const auto ninf = Set(d, -std::numeric_limits<T>::infinity());
    const auto pone = Set(d, static_cast<T>(1.0));
    const auto none = Set(d, static_cast<T>(-1.0));
    const auto nzero = Set(d, static_cast<T>(0.0));
    return IfThenElse(
        v == pone, pinf,
        IfThenElse(v == none, ninf, IfThenElse(Abs(v) > pone, nan, Atanh(d, v))));
  }

  static inline double scalar(const double x) { return atanh(x); }

  static inline float scalar(const float x) { return atanhf(x); }
};

struct Atan2Functor {
  template <typename T, typename V>
  static inline auto vector(const hn::ScalableTag<T> d, const V &v1, const V &v2) {
    constexpr bool kIsF32 = (sizeof(T) == 4);
    using TI = hwy::MakeSigned<T>;
    const hn::Rebind<TI, hn::ScalableTag<T>> di;

    auto pzero = Set(di, kIsF32 ? static_cast<TI>(0x00000000L)
                                : static_cast<TI>(0x0000000000000000LL));
    auto nzero = Set(di, kIsF32 ? static_cast<TI>(0x80000000L)
                                : static_cast<TI>(0x8000000000000000LL));

    auto negneg = And(BitCast(di, v1) == nzero, BitCast(di, v2) == nzero);
    auto posneg = And(BitCast(di, v1) == pzero, BitCast(di, v2) == nzero);

    const auto ppi = Set(d, static_cast<T>(+3.14159265358979323846264));
    const auto npi = Set(d, static_cast<T>(-3.14159265358979323846264));
    return BitCast(d,
                   IfThenElse(negneg, BitCast(di, npi),
                              BitCast(di, IfThenElse(posneg, BitCast(di, ppi),
                                                     BitCast(di, Atan2(d, v1, v2))))));
  }

  static inline auto scalar(const double x, const double y) { return atan2(x, y); }

  static inline auto scalar(const float x, const float y) { return atan2f(x, y); }
};

struct CosFunctor {
  template <typename T> static inline T limit() {
    if constexpr (std::is_same_v<T, double>) {
      return 3.37e9;
    } else if constexpr (std::is_same_v<T, float>) {
      return 2.63e7f;
    } else {
      return T{};
    }
  }

  template <typename T, typename V>
  static inline auto vector(const hn::ScalableTag<T> d, const V &v) {
    // Values outside of [-LIMIT, LIMIT] are not valid for SIMD version.
    const T LIMIT = limit<T>();
    constexpr size_t L = hn::Lanes(d);
    T tmp[L];
    Store(v, d, tmp);

    for (auto i = 0; i < L; ++i) {
      const auto x = tmp[i];
      if (x < -LIMIT || x > LIMIT) {
        // Just use scalar version in this case.
        for (auto j = 0; j < L; ++j)
          tmp[j] = scalar(tmp[j]);
        return Load(d, tmp);
      }
    }

    return Cos(d, v);
  }

  static inline double scalar(const double x) { return cos(x); }

  static inline float scalar(const float x) { return cosf(x); }
};

struct ExpFunctor {
  template <typename T> static inline T limit() {
    if constexpr (std::is_same_v<T, double>) {
      return 1000.0;
    } else if constexpr (std::is_same_v<T, float>) {
      return 128.0f;
    } else {
      return T{};
    }
  }

  template <typename T, typename V>
  static inline auto vector(const hn::ScalableTag<T> d, const V &v) {
    const auto lim = Set(d, limit<T>());
    const auto pinf = Set(d, std::numeric_limits<T>::infinity());
    return IfThenElse(IsNaN(v), v, IfThenElse(v >= lim, pinf, Exp(d, v)));
  }

  static inline double scalar(const double x) { return exp(x); }

  static inline float scalar(const float x) { return expf(x); }
};

struct Exp2Functor {
  template <typename T> static inline T limit() {
    if constexpr (std::is_same_v<T, double>) {
      return 2048.0;
    } else if constexpr (std::is_same_v<T, float>) {
      return 128.0f;
    } else {
      return T{};
    }
  }

  template <typename T, typename V>
  static inline auto vector(const hn::ScalableTag<T> d, const V &v) {
    const auto lim = Set(d, limit<T>());
    const auto pinf = Set(d, std::numeric_limits<T>::infinity());
    return IfThenElse(IsNaN(v), v, IfThenElse(v >= lim, pinf, Exp2(d, v)));
  }

  static inline double scalar(const double x) { return exp2(x); }

  static inline float scalar(const float x) { return exp2f(x); }
};

struct Expm1Functor {
  template <typename T> static inline T limit() {
    if constexpr (std::is_same_v<T, double>) {
      return 1000.0;
    } else if constexpr (std::is_same_v<T, float>) {
      return 128.0f;
    } else {
      return T{};
    }
  }

  template <typename T, typename V>
  static inline auto vector(const hn::ScalableTag<T> d, const V &v) {
    const auto lim = Set(d, limit<T>());
    const auto pinf = Set(d, std::numeric_limits<T>::infinity());
    return IfThenElse(IsNaN(v), v, IfThenElse(v >= lim, pinf, Expm1(d, v)));
  }

  static inline double scalar(const double x) { return expm1(x); }

  static inline float scalar(const float x) { return expm1f(x); }
};

struct LogFunctor {
  template <typename T, typename V>
  static inline auto vector(const hn::ScalableTag<T> d, const V &v) {
    const auto nan = Set(d, std::numeric_limits<T>::quiet_NaN());
    const auto pinf = Set(d, std::numeric_limits<T>::infinity());
    const auto ninf = Set(d, -std::numeric_limits<T>::infinity());
    const auto zero = Set(d, static_cast<T>(0.0));
    return IfThenElse(
        v == zero, ninf,
        IfThenElse(v < zero, nan,
                   IfThenElse(v == pinf, pinf, IfThenElse(IsNaN(v), v, Log(d, v)))));
  }

  static inline double scalar(const double x) { return log(x); }

  static inline float scalar(const float x) { return logf(x); }
};

struct Log10Functor {
  template <typename T, typename V>
  static inline auto vector(const hn::ScalableTag<T> d, const V &v) {
    const auto nan = Set(d, std::numeric_limits<T>::quiet_NaN());
    const auto pinf = Set(d, std::numeric_limits<T>::infinity());
    const auto ninf = Set(d, -std::numeric_limits<T>::infinity());
    const auto zero = Set(d, static_cast<T>(0.0));
    return IfThenElse(
        v == zero, ninf,
        IfThenElse(v < zero, nan,
                   IfThenElse(v == pinf, pinf, IfThenElse(IsNaN(v), v, Log10(d, v)))));
  }

  static inline double scalar(const double x) { return log10(x); }

  static inline float scalar(const float x) { return log10f(x); }
};

struct Log1pFunctor {
  template <typename T, typename V>
  static inline auto vector(const hn::ScalableTag<T> d, const V &v) {
    const auto nan = Set(d, std::numeric_limits<T>::quiet_NaN());
    const auto pinf = Set(d, std::numeric_limits<T>::infinity());
    const auto ninf = Set(d, -std::numeric_limits<T>::infinity());
    const auto none = Set(d, static_cast<T>(-1.0));
    return IfThenElse(
        v == none, ninf,
        IfThenElse(v < none, nan,
                   IfThenElse(v == pinf, pinf, IfThenElse(IsNaN(v), v, Log1p(d, v)))));
  }

  static inline double scalar(const double x) { return log1p(x); }

  static inline float scalar(const float x) { return log1pf(x); }
};

struct Log2Functor {
  template <typename T, typename V>
  static inline auto vector(const hn::ScalableTag<T> d, const V &v) {
    const auto nan = Set(d, std::numeric_limits<T>::quiet_NaN());
    const auto pinf = Set(d, std::numeric_limits<T>::infinity());
    const auto ninf = Set(d, -std::numeric_limits<T>::infinity());
    const auto zero = Set(d, static_cast<T>(0.0));
    return IfThenElse(
        v == zero, ninf,
        IfThenElse(v < zero, nan,
                   IfThenElse(v == pinf, pinf, IfThenElse(IsNaN(v), v, Log2(d, v)))));
  }

  static inline double scalar(const double x) { return log2(x); }

  static inline float scalar(const float x) { return log2f(x); }
};

struct SinFunctor {
  template <typename T> static inline T limit() {
    if constexpr (std::is_same_v<T, double>) {
      return 6.74e9;
    } else if constexpr (std::is_same_v<T, float>) {
      return 5.30e8f;
    } else {
      return T{};
    }
  }

  template <typename T, typename V>
  static inline auto vector(const hn::ScalableTag<T> d, const V &v) {
    // Values outside of [-LIMIT, LIMIT] are not valid for SIMD version.
    const T LIMIT = limit<T>();
    constexpr size_t L = hn::Lanes(d);
    T tmp[L];
    Store(v, d, tmp);

    for (auto i = 0; i < L; ++i) {
      const auto x = tmp[i];
      if (x < -LIMIT || x > LIMIT) {
        // Just use scalar version in this case.
        for (auto j = 0; j < L; ++j)
          tmp[j] = scalar(tmp[j]);
        return Load(d, tmp);
      }
    }

    return Sin(d, v);
  }

  static inline double scalar(const double x) { return sin(x); }

  static inline float scalar(const float x) { return sinf(x); }
};

struct SinhFunctor {
  template <typename T> static inline T limit() {
    if constexpr (std::is_same_v<T, double>) {
      return 709.0;
    } else if constexpr (std::is_same_v<T, float>) {
      return 88.7228f;
    } else {
      return T{};
    }
  }

  template <typename T, typename V>
  static inline auto vector(const hn::ScalableTag<T> d, const V &v) {
    // Values outside of [-LIMIT, LIMIT] are not valid for SIMD version.
    const T LIMIT = limit<T>();
    constexpr size_t L = hn::Lanes(d);
    T tmp[L];
    Store(v, d, tmp);

    for (auto i = 0; i < L; ++i) {
      const auto x = tmp[i];
      if (x < -LIMIT || x > LIMIT) {
        // Just use scalar version in this case.
        for (auto j = 0; j < L; ++j)
          tmp[j] = scalar(tmp[j]);
        return Load(d, tmp);
      }
    }

    return Sinh(d, v);
  }

  static inline double scalar(const double x) { return sinh(x); }

  static inline float scalar(const float x) { return sinhf(x); }
};

struct TanhFunctor {
  template <typename T, typename V>
  static inline auto vector(const hn::ScalableTag<T> d, const V &v) {
    return Tanh(d, v);
  }

  static inline double scalar(const double x) { return tanh(x); }

  static inline float scalar(const float x) { return tanhf(x); }
};

struct HypotFunctor {
  template <typename T, typename V>
  static inline auto vector(const hn::ScalableTag<T> d, const V &v1, const V &v2) {
    return Hypot(d, v1, v2);
  }

  static inline auto scalar(const double x, const double y) { return hypot(x, y); }

  static inline auto scalar(const float x, const float y) { return hypotf(x, y); }
};

template <typename T, typename F>
void UnaryLoop(const T *in, size_t is, T *out, size_t os, size_t n) {
  const hn::ScalableTag<T> d;
  constexpr size_t L = Lanes(d);
  T tmp[L];
  size_t i;

  if (is == sizeof(T) && os == sizeof(T)) {
    for (i = 0; i + L <= n; i += L) {
      memcpy(tmp, in + i, L * sizeof(T));
      auto vec = hn::Load(d, tmp);
      Store(F::template vector<T, decltype(vec)>(d, vec), d, tmp);
      memcpy(out + i, tmp, L * sizeof(T));
    }

    for (; i < n; ++i)
      out[i] = F::scalar(in[i]);
  } else {
    for (i = 0; i + L <= n; i += L) {
      for (size_t j = 0; j < L; ++j)
        tmp[j] = *(T *)((char *)in + (i + j) * is);

      auto vec = hn::Load(d, tmp);
      Store(F::template vector<T, decltype(vec)>(d, vec), d, tmp);

      for (size_t j = 0; j < L; ++j)
        *(T *)((char *)out + (i + j) * os) = tmp[j];
    }

    for (; i < n; ++i)
      *(T *)((char *)out + i * os) = F::scalar(*(T *)((char *)in + i * is));
  }
}

template <typename T, typename F>
void BinaryLoop(const T *in1, size_t is1, const T *in2, size_t is2, T *out, size_t os,
                size_t n) {
  const hn::ScalableTag<T> d;
  constexpr size_t L = Lanes(d);
  T tmp1[L];
  T tmp2[L];
  size_t i;

  if (is1 == sizeof(T) && is2 == sizeof(T) && os == sizeof(T)) {
    for (i = 0; i + L <= n; i += L) {
      memcpy(tmp1, in1 + i, L * sizeof(T));
      memcpy(tmp2, in2 + i, L * sizeof(T));
      auto vec1 = hn::Load(d, tmp1);
      auto vec2 = hn::Load(d, tmp2);
      Store(F::template vector<T, decltype(vec1)>(d, vec1, vec2), d, tmp1);
      memcpy(out + i, tmp1, L * sizeof(T));
    }

    for (; i < n; ++i)
      out[i] = F::scalar(in1[i], in2[i]);
  } else if (is1 == 0 && is2 == sizeof(T) && os == sizeof(T)) {
    for (size_t j = 0; j < L; ++j)
      tmp1[j] = in1[0];

    for (i = 0; i + L <= n; i += L) {
      memcpy(tmp2, in2 + i, L * sizeof(T));
      auto vec1 = hn::Load(d, tmp1);
      auto vec2 = hn::Load(d, tmp2);
      Store(F::template vector<T, decltype(vec1)>(d, vec1, vec2), d, tmp1);
      memcpy(out + i, tmp1, L * sizeof(T));
    }

    for (; i < n; ++i)
      out[i] = F::scalar(in1[0], in2[i]);
  } else if (is1 == sizeof(T) && is2 == 0 && os == sizeof(T)) {
    for (size_t j = 0; j < L; ++j)
      tmp2[j] = in2[0];

    for (i = 0; i + L <= n; i += L) {
      memcpy(tmp1, in1 + i, L * sizeof(T));
      auto vec1 = hn::Load(d, tmp1);
      auto vec2 = hn::Load(d, tmp2);
      Store(F::template vector<T, decltype(vec1)>(d, vec1, vec2), d, tmp1);
      memcpy(out + i, tmp1, L * sizeof(T));
    }

    for (; i < n; ++i)
      out[i] = F::scalar(in1[i], in2[0]);
  } else {
    for (i = 0; i + L <= n; i += L) {
      for (size_t j = 0; j < L; ++j) {
        tmp1[j] = *(T *)((char *)in1 + (i + j) * is1);
        tmp2[j] = *(T *)((char *)in2 + (i + j) * is2);
      }

      auto vec1 = hn::Load(d, tmp1);
      auto vec2 = hn::Load(d, tmp2);
      Store(F::template vector<T, decltype(vec1)>(d, vec1, vec2), d, tmp1);

      for (size_t j = 0; j < L; ++j)
        *(T *)((char *)out + (i + j) * os) = tmp1[j];
    }

    for (; i < n; ++i)
      *(T *)((char *)out + i * os) =
          F::scalar(*(T *)((char *)in1 + i * is1), *(T *)((char *)in2 + i * is2));
  }
}

void LoopAcos32(const float *in, size_t is, float *out, size_t os, size_t n) {
  UnaryLoop<float, AcosFunctor>(in, is, out, os, n);
}

void LoopAcos64(const double *in, size_t is, double *out, size_t os, size_t n) {
  UnaryLoop<double, AcosFunctor>(in, is, out, os, n);
}

void LoopAcosh32(const float *in, size_t is, float *out, size_t os, size_t n) {
  UnaryLoop<float, AcoshFunctor>(in, is, out, os, n);
}

void LoopAcosh64(const double *in, size_t is, double *out, size_t os, size_t n) {
  UnaryLoop<double, AcoshFunctor>(in, is, out, os, n);
}

void LoopAsin32(const float *in, size_t is, float *out, size_t os, size_t n) {
  UnaryLoop<float, AsinFunctor>(in, is, out, os, n);
}

void LoopAsin64(const double *in, size_t is, double *out, size_t os, size_t n) {
  UnaryLoop<double, AsinFunctor>(in, is, out, os, n);
}

void LoopAsinh32(const float *in, size_t is, float *out, size_t os, size_t n) {
  UnaryLoop<float, AsinhFunctor>(in, is, out, os, n);
}

void LoopAsinh64(const double *in, size_t is, double *out, size_t os, size_t n) {
  UnaryLoop<double, AsinhFunctor>(in, is, out, os, n);
}

void LoopAtan32(const float *in, size_t is, float *out, size_t os, size_t n) {
  UnaryLoop<float, AtanFunctor>(in, is, out, os, n);
}

void LoopAtan64(const double *in, size_t is, double *out, size_t os, size_t n) {
  UnaryLoop<double, AtanFunctor>(in, is, out, os, n);
}

void LoopAtanh32(const float *in, size_t is, float *out, size_t os, size_t n) {
  UnaryLoop<float, AtanhFunctor>(in, is, out, os, n);
}

void LoopAtanh64(const double *in, size_t is, double *out, size_t os, size_t n) {
  UnaryLoop<double, AtanhFunctor>(in, is, out, os, n);
}

void LoopAtan232(const float *in1, size_t is1, const float *in2, size_t is2, float *out,
                 size_t os, size_t n) {
  BinaryLoop<float, Atan2Functor>(in1, is1, in2, is2, out, os, n);
}

void LoopAtan264(const double *in1, size_t is1, const double *in2, size_t is2,
                 double *out, size_t os, size_t n) {
  BinaryLoop<double, Atan2Functor>(in1, is1, in2, is2, out, os, n);
}

void LoopCos32(const float *in, size_t is, float *out, size_t os, size_t n) {
  UnaryLoop<float, CosFunctor>(in, is, out, os, n);
}

void LoopCos64(const double *in, size_t is, double *out, size_t os, size_t n) {
  UnaryLoop<double, CosFunctor>(in, is, out, os, n);
}

void LoopExp32(const float *in, size_t is, float *out, size_t os, size_t n) {
  UnaryLoop<float, ExpFunctor>(in, is, out, os, n);
}

void LoopExp64(const double *in, size_t is, double *out, size_t os, size_t n) {
  UnaryLoop<double, ExpFunctor>(in, is, out, os, n);
}

void LoopExp232(const float *in, size_t is, float *out, size_t os, size_t n) {
  UnaryLoop<float, Exp2Functor>(in, is, out, os, n);
}

void LoopExp264(const double *in, size_t is, double *out, size_t os, size_t n) {
  UnaryLoop<double, Exp2Functor>(in, is, out, os, n);
}

void LoopExpm132(const float *in, size_t is, float *out, size_t os, size_t n) {
  UnaryLoop<float, Expm1Functor>(in, is, out, os, n);
}

void LoopExpm164(const double *in, size_t is, double *out, size_t os, size_t n) {
  UnaryLoop<double, Expm1Functor>(in, is, out, os, n);
}

void LoopLog32(const float *in, size_t is, float *out, size_t os, size_t n) {
  UnaryLoop<float, LogFunctor>(in, is, out, os, n);
}

void LoopLog64(const double *in, size_t is, double *out, size_t os, size_t n) {
  UnaryLoop<double, LogFunctor>(in, is, out, os, n);
}

void LoopLog1032(const float *in, size_t is, float *out, size_t os, size_t n) {
  UnaryLoop<float, Log10Functor>(in, is, out, os, n);
}

void LoopLog1064(const double *in, size_t is, double *out, size_t os, size_t n) {
  UnaryLoop<double, Log10Functor>(in, is, out, os, n);
}

void LoopLog1p32(const float *in, size_t is, float *out, size_t os, size_t n) {
  UnaryLoop<float, Log1pFunctor>(in, is, out, os, n);
}

void LoopLog1p64(const double *in, size_t is, double *out, size_t os, size_t n) {
  UnaryLoop<double, Log1pFunctor>(in, is, out, os, n);
}

void LoopLog232(const float *in, size_t is, float *out, size_t os, size_t n) {
  UnaryLoop<float, Log2Functor>(in, is, out, os, n);
}

void LoopLog264(const double *in, size_t is, double *out, size_t os, size_t n) {
  UnaryLoop<double, Log2Functor>(in, is, out, os, n);
}

void LoopSin32(const float *in, size_t is, float *out, size_t os, size_t n) {
  UnaryLoop<float, SinFunctor>(in, is, out, os, n);
}

void LoopSin64(const double *in, size_t is, double *out, size_t os, size_t n) {
  UnaryLoop<double, SinFunctor>(in, is, out, os, n);
}

void LoopSinh32(const float *in, size_t is, float *out, size_t os, size_t n) {
  UnaryLoop<float, SinhFunctor>(in, is, out, os, n);
}

void LoopSinh64(const double *in, size_t is, double *out, size_t os, size_t n) {
  UnaryLoop<double, SinhFunctor>(in, is, out, os, n);
}

void LoopTanh32(const float *in, size_t is, float *out, size_t os, size_t n) {
  UnaryLoop<float, TanhFunctor>(in, is, out, os, n);
}

void LoopTanh64(const double *in, size_t is, double *out, size_t os, size_t n) {
  UnaryLoop<double, TanhFunctor>(in, is, out, os, n);
}

void LoopHypot32(const float *in1, size_t is1, const float *in2, size_t is2, float *out,
                 size_t os, size_t n) {
  BinaryLoop<float, HypotFunctor>(in1, is1, in2, is2, out, os, n);
}

void LoopHypot64(const double *in1, size_t is1, const double *in2, size_t is2,
                 double *out, size_t os, size_t n) {
  BinaryLoop<double, HypotFunctor>(in1, is1, in2, is2, out, os, n);
}

} // namespace HWY_NAMESPACE
} // namespace
HWY_AFTER_NAMESPACE();

#if HWY_ONCE

HWY_EXPORT(LoopAcos32);
HWY_EXPORT(LoopAcos64);
HWY_EXPORT(LoopAcosh32);
HWY_EXPORT(LoopAcosh64);
HWY_EXPORT(LoopAsin32);
HWY_EXPORT(LoopAsin64);
HWY_EXPORT(LoopAsinh32);
HWY_EXPORT(LoopAsinh64);
HWY_EXPORT(LoopAtan32);
HWY_EXPORT(LoopAtan64);
HWY_EXPORT(LoopAtanh32);
HWY_EXPORT(LoopAtanh64);
HWY_EXPORT(LoopAtan232);
HWY_EXPORT(LoopAtan264);
HWY_EXPORT(LoopCos32);
HWY_EXPORT(LoopCos64);
HWY_EXPORT(LoopExp32);
HWY_EXPORT(LoopExp64);
HWY_EXPORT(LoopExp232);
HWY_EXPORT(LoopExp264);
HWY_EXPORT(LoopExpm132);
HWY_EXPORT(LoopExpm164);
HWY_EXPORT(LoopLog32);
HWY_EXPORT(LoopLog64);
HWY_EXPORT(LoopLog1032);
HWY_EXPORT(LoopLog1064);
HWY_EXPORT(LoopLog1p32);
HWY_EXPORT(LoopLog1p64);
HWY_EXPORT(LoopLog232);
HWY_EXPORT(LoopLog264);
HWY_EXPORT(LoopSin32);
HWY_EXPORT(LoopSin64);
HWY_EXPORT(LoopSinh32);
HWY_EXPORT(LoopSinh64);
HWY_EXPORT(LoopTanh32);
HWY_EXPORT(LoopTanh64);
HWY_EXPORT(LoopHypot32);
HWY_EXPORT(LoopHypot64);

SEQ_FUNC void cnp_acos_float32(const float *in, size_t is, float *out, size_t os,
                               size_t n) {
  const auto ptr = HWY_DYNAMIC_POINTER(LoopAcos32);
  return ptr(in, is, out, os, n);
}

SEQ_FUNC void cnp_acos_float64(const double *in, size_t is, double *out, size_t os,
                               size_t n) {
  const auto ptr = HWY_DYNAMIC_POINTER(LoopAcos64);
  return ptr(in, is, out, os, n);
}

SEQ_FUNC void cnp_acosh_float32(const float *in, size_t is, float *out, size_t os,
                                size_t n) {
  const auto ptr = HWY_DYNAMIC_POINTER(LoopAcosh32);
  return ptr(in, is, out, os, n);
}

SEQ_FUNC void cnp_acosh_float64(const double *in, size_t is, double *out, size_t os,
                                size_t n) {
  const auto ptr = HWY_DYNAMIC_POINTER(LoopAcosh64);
  return ptr(in, is, out, os, n);
}

SEQ_FUNC void cnp_asin_float32(const float *in, size_t is, float *out, size_t os,
                               size_t n) {
  const auto ptr = HWY_DYNAMIC_POINTER(LoopAsin32);
  return ptr(in, is, out, os, n);
}

SEQ_FUNC void cnp_asin_float64(const double *in, size_t is, double *out, size_t os,
                               size_t n) {
  const auto ptr = HWY_DYNAMIC_POINTER(LoopAsin64);
  return ptr(in, is, out, os, n);
}

SEQ_FUNC void cnp_asinh_float32(const float *in, size_t is, float *out, size_t os,
                                size_t n) {
  const auto ptr = HWY_DYNAMIC_POINTER(LoopAsinh32);
  return ptr(in, is, out, os, n);
}

SEQ_FUNC void cnp_asinh_float64(const double *in, size_t is, double *out, size_t os,
                                size_t n) {
  const auto ptr = HWY_DYNAMIC_POINTER(LoopAsinh64);
  return ptr(in, is, out, os, n);
}

SEQ_FUNC void cnp_atan_float32(const float *in, size_t is, float *out, size_t os,
                               size_t n) {
  const auto ptr = HWY_DYNAMIC_POINTER(LoopAtan32);
  return ptr(in, is, out, os, n);
}

SEQ_FUNC void cnp_atan_float64(const double *in, size_t is, double *out, size_t os,
                               size_t n) {
  const auto ptr = HWY_DYNAMIC_POINTER(LoopAtan64);
  return ptr(in, is, out, os, n);
}

SEQ_FUNC void cnp_atanh_float32(const float *in, size_t is, float *out, size_t os,
                                size_t n) {
  const auto ptr = HWY_DYNAMIC_POINTER(LoopAtanh32);
  return ptr(in, is, out, os, n);
}

SEQ_FUNC void cnp_atanh_float64(const double *in, size_t is, double *out, size_t os,
                                size_t n) {
  const auto ptr = HWY_DYNAMIC_POINTER(LoopAtanh64);
  return ptr(in, is, out, os, n);
}

SEQ_FUNC void cnp_atan2_float32(const float *in1, size_t is1, const float *in2,
                                size_t is2, float *out, size_t os, size_t n) {
  const auto ptr = HWY_DYNAMIC_POINTER(LoopAtan232);
  return ptr(in1, is1, in2, is2, out, os, n);
}

SEQ_FUNC void cnp_atan2_float64(const double *in1, size_t is1, const double *in2,
                                size_t is2, double *out, size_t os, size_t n) {
  const auto ptr = HWY_DYNAMIC_POINTER(LoopAtan264);
  return ptr(in1, is1, in2, is2, out, os, n);
}

SEQ_FUNC void cnp_cos_float32(const float *in, size_t is, float *out, size_t os,
                              size_t n) {
  const auto ptr = HWY_DYNAMIC_POINTER(LoopCos32);
  return ptr(in, is, out, os, n);
}

SEQ_FUNC void cnp_cos_float64(const double *in, size_t is, double *out, size_t os,
                              size_t n) {
  const auto ptr = HWY_DYNAMIC_POINTER(LoopCos64);
  return ptr(in, is, out, os, n);
}

SEQ_FUNC void cnp_exp_float32(const float *in, size_t is, float *out, size_t os,
                              size_t n) {
  const auto ptr = HWY_DYNAMIC_POINTER(LoopExp32);
  return ptr(in, is, out, os, n);
}

SEQ_FUNC void cnp_exp_float64(const double *in, size_t is, double *out, size_t os,
                              size_t n) {
  const auto ptr = HWY_DYNAMIC_POINTER(LoopExp64);
  return ptr(in, is, out, os, n);
}

SEQ_FUNC void cnp_exp2_float32(const float *in, size_t is, float *out, size_t os,
                               size_t n) {
  const auto ptr = HWY_DYNAMIC_POINTER(LoopExp232);
  return ptr(in, is, out, os, n);
}

SEQ_FUNC void cnp_exp2_float64(const double *in, size_t is, double *out, size_t os,
                               size_t n) {
  const auto ptr = HWY_DYNAMIC_POINTER(LoopExp264);
  return ptr(in, is, out, os, n);
}

SEQ_FUNC void cnp_expm1_float32(const float *in, size_t is, float *out, size_t os,
                                size_t n) {
  const auto ptr = HWY_DYNAMIC_POINTER(LoopExpm132);
  return ptr(in, is, out, os, n);
}

SEQ_FUNC void cnp_expm1_float64(const double *in, size_t is, double *out, size_t os,
                                size_t n) {
  const auto ptr = HWY_DYNAMIC_POINTER(LoopExpm164);
  return ptr(in, is, out, os, n);
}

SEQ_FUNC void cnp_log_float32(const float *in, size_t is, float *out, size_t os,
                              size_t n) {
  const auto ptr = HWY_DYNAMIC_POINTER(LoopLog32);
  return ptr(in, is, out, os, n);
}

SEQ_FUNC void cnp_log_float64(const double *in, size_t is, double *out, size_t os,
                              size_t n) {
  const auto ptr = HWY_DYNAMIC_POINTER(LoopLog64);
  return ptr(in, is, out, os, n);
}

SEQ_FUNC void cnp_log10_float32(const float *in, size_t is, float *out, size_t os,
                                size_t n) {
  const auto ptr = HWY_DYNAMIC_POINTER(LoopLog1032);
  return ptr(in, is, out, os, n);
}

SEQ_FUNC void cnp_log10_float64(const double *in, size_t is, double *out, size_t os,
                                size_t n) {
  const auto ptr = HWY_DYNAMIC_POINTER(LoopLog1064);
  return ptr(in, is, out, os, n);
}

SEQ_FUNC void cnp_log1p_float32(const float *in, size_t is, float *out, size_t os,
                                size_t n) {
  const auto ptr = HWY_DYNAMIC_POINTER(LoopLog1p32);
  return ptr(in, is, out, os, n);
}

SEQ_FUNC void cnp_log1p_float64(const double *in, size_t is, double *out, size_t os,
                                size_t n) {
  const auto ptr = HWY_DYNAMIC_POINTER(LoopLog1p64);
  return ptr(in, is, out, os, n);
}

SEQ_FUNC void cnp_log2_float32(const float *in, size_t is, float *out, size_t os,
                               size_t n) {
  const auto ptr = HWY_DYNAMIC_POINTER(LoopLog232);
  return ptr(in, is, out, os, n);
}

SEQ_FUNC void cnp_log2_float64(const double *in, size_t is, double *out, size_t os,
                               size_t n) {
  const auto ptr = HWY_DYNAMIC_POINTER(LoopLog264);
  return ptr(in, is, out, os, n);
}

SEQ_FUNC void cnp_sin_float32(const float *in, size_t is, float *out, size_t os,
                              size_t n) {
  const auto ptr = HWY_DYNAMIC_POINTER(LoopSin32);
  return ptr(in, is, out, os, n);
}

SEQ_FUNC void cnp_sin_float64(const double *in, size_t is, double *out, size_t os,
                              size_t n) {
  const auto ptr = HWY_DYNAMIC_POINTER(LoopSin64);
  return ptr(in, is, out, os, n);
}

SEQ_FUNC void cnp_sinh_float32(const float *in, size_t is, float *out, size_t os,
                               size_t n) {
  const auto ptr = HWY_DYNAMIC_POINTER(LoopSinh32);
  return ptr(in, is, out, os, n);
}

SEQ_FUNC void cnp_sinh_float64(const double *in, size_t is, double *out, size_t os,
                               size_t n) {
  const auto ptr = HWY_DYNAMIC_POINTER(LoopSinh64);
  return ptr(in, is, out, os, n);
}

SEQ_FUNC void cnp_tanh_float32(const float *in, size_t is, float *out, size_t os,
                               size_t n) {
  const auto ptr = HWY_DYNAMIC_POINTER(LoopTanh32);
  return ptr(in, is, out, os, n);
}

SEQ_FUNC void cnp_tanh_float64(const double *in, size_t is, double *out, size_t os,
                               size_t n) {
  const auto ptr = HWY_DYNAMIC_POINTER(LoopTanh64);
  return ptr(in, is, out, os, n);
}

SEQ_FUNC void cnp_hypot_float32(const float *in1, size_t is1, const float *in2,
                                size_t is2, float *out, size_t os, size_t n) {
  const auto ptr = HWY_DYNAMIC_POINTER(LoopHypot32);
  return ptr(in1, is1, in2, is2, out, os, n);
}

SEQ_FUNC void cnp_hypot_float64(const double *in1, size_t is1, const double *in2,
                                size_t is2, double *out, size_t os, size_t n) {
  const auto ptr = HWY_DYNAMIC_POINTER(LoopHypot64);
  return ptr(in1, is1, in2, is2, out, os, n);
}

#endif // HWY_ONCE
