# numpy.random.bitgen

Standard library module [View source](https://github.com/exaloop/codon/blob/master/stdlib/numpy/random/bitgen.codon)

variable u128[](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/#variable-u128 "Copy/link to this API element")

```python
u128 = UInt[128]
```

class Binomial[](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/#class-binomial "Copy/link to this API element")

```python
class Binomial
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/Binomial/)

Fields 16

`psave` `float`

`nsave` `int`

`r` `float`

`q` `float`

`fm` `float`

`m` `int`

`p1` `float`

`xm` `float`

`xl` `float`

`xr` `float`

`c` `float`

`laml` `float`

`lamr` `float`

`p2` `float`

`p3` `float`

`p4` `float`

Constructors 1

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/#class-binomial-__init__ "Copy/link to this API element")

```python
__init__(self: Self, Self: type)
```

class BitGenerator[](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/#class-bitgenerator "Copy/link to this API element")

```python
class BitGenerator[G]
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/BitGenerator/)

Fields 5

`state` `G`

`binomial` `Optional[Binomial]`

`uinteger` `u32`

`has_uint32` `bool`

`G` `type`

Constructors 1

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/#class-bitgenerator-__init__ "Copy/link to this API element")

```python
__init__(self: Self, state: G, Self: type)
```

Properties 1

seed\_seq[](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/#class-bitgenerator-seed_seq "Copy/link to this API element")

@property

```python
seed_seq(self: Self, Self: type)
```

Methods 97

next64[](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/#class-bitgenerator-next64 "Copy/link to this API element")

```python
next64(self: Self, Self: type)
```

next32[](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/#class-bitgenerator-next32 "Copy/link to this API element")

```python
next32(self: Self, Self: type)
```

next\_double[](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/#class-bitgenerator-next_double "Copy/link to this API element")

```python
next_double(self: Self, Self: type)
```

next\_float[](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/#class-bitgenerator-next_float "Copy/link to this API element")

```python
next_float(self: Self, Self: type)
```

jumped[](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/#class-bitgenerator-jumped "Copy/link to this API element")

```python
jumped(self: Self, jumps: int, Self: type)
```

standard\_uniform[](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/#class-bitgenerator-standard_uniform "Copy/link to this API element")

```python
standard_uniform(self: Self, Self: type)
```

standard\_uniform\_f[](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/#class-bitgenerator-standard_uniform_f "Copy/link to this API element")

```python
standard_uniform_f(self: Self, Self: type)
```

random\_standard\_uniform\_fill[](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/#class-bitgenerator-random_standard_uniform_fill "Copy/link to this API element")

```python
random_standard_uniform_fill(self: Self, cnt: int, out: Ptr[float], Self: type)
```

random\_standard\_uniform\_fill\_f[](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/#class-bitgenerator-random_standard_uniform_fill_f "Copy/link to this API element")

```python
random_standard_uniform_fill_f(self: Self, cnt: int, out: Ptr[float32], Self: type)
```

standard\_exponential\_unlikely[](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/#class-bitgenerator-standard_exponential_unlikely "Copy/link to this API element")

```python
standard_exponential_unlikely(self: Self, idx: int, x: float, Self: type)
```

random\_standard\_exponential[](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/#class-bitgenerator-random_standard_exponential "Copy/link to this API element")

```python
random_standard_exponential(self: Self, Self: type)
```

random\_standard\_exponential\_fill[](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/#class-bitgenerator-random_standard_exponential_fill "Copy/link to this API element")

```python
random_standard_exponential_fill(self: Self, cnt: int, out: Ptr[float], Self: type)
```

standard\_exponential\_unlikely\_f[](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/#class-bitgenerator-standard_exponential_unlikely_f "Copy/link to this API element")

```python
standard_exponential_unlikely_f(self: Self, idx: int, x: float32, Self: type)
```

random\_standard\_exponential\_f[](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/#class-bitgenerator-random_standard_exponential_f "Copy/link to this API element")

```python
random_standard_exponential_f(self: Self, Self: type)
```

random\_standard\_exponential\_fill\_f[](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/#class-bitgenerator-random_standard_exponential_fill_f "Copy/link to this API element")

```python
random_standard_exponential_fill_f(self: Self, cnt: int, out: Ptr[float32], Self: type)
```

random\_standard\_exponential\_inv\_fill[](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/#class-bitgenerator-random_standard_exponential_inv_fill "Copy/link to this API element")

```python
random_standard_exponential_inv_fill(self: Self, cnt: int, out: Ptr[float], Self: type)
```

random\_standard\_exponential\_inv\_fill\_f[](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/#class-bitgenerator-random_standard_exponential_inv_fill_f "Copy/link to this API element")

```python
random_standard_exponential_inv_fill_f(self: Self, cnt: int, out: Ptr[float32], Self: type)
```

random\_standard\_normal[](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/#class-bitgenerator-random_standard_normal "Copy/link to this API element")

```python
random_standard_normal(self: Self, Self: type)
```

random\_standard\_normal\_fill[](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/#class-bitgenerator-random_standard_normal_fill "Copy/link to this API element")

```python
random_standard_normal_fill(self: Self, cnt: int, out: Ptr[float], Self: type)
```

random\_standard\_normal\_f[](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/#class-bitgenerator-random_standard_normal_f "Copy/link to this API element")

```python
random_standard_normal_f(self: Self, Self: type)
```

random\_standard\_normal\_fill\_f[](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/#class-bitgenerator-random_standard_normal_fill_f "Copy/link to this API element")

```python
random_standard_normal_fill_f(self: Self, cnt: int, out: Ptr[float32], Self: type)
```

random\_standard\_gamma[](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/#class-bitgenerator-random_standard_gamma "Copy/link to this API element")

```python
random_standard_gamma(self: Self, shape: float, Self: type)
```

random\_standard\_gamma\_f[](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/#class-bitgenerator-random_standard_gamma_f "Copy/link to this API element")

```python
random_standard_gamma_f(self: Self, shape: float32, Self: type)
```

random\_positive\_int64[](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/#class-bitgenerator-random_positive_int64 "Copy/link to this API element")

```python
random_positive_int64(self: Self, Self: type)
```

random\_positive\_int32[](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/#class-bitgenerator-random_positive_int32 "Copy/link to this API element")

```python
random_positive_int32(self: Self, Self: type)
```

random\_positive\_int[](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/#class-bitgenerator-random_positive_int "Copy/link to this API element")

```python
random_positive_int(self: Self, Self: type)
```

random\_uint[](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/#class-bitgenerator-random_uint "Copy/link to this API element")

```python
random_uint(self: Self, Self: type)
```

random\_loggam[](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/#class-bitgenerator-random_loggam "Copy/link to this API element")

```python
random_loggam(x: float)
```

random\_normal[](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/#class-bitgenerator-random_normal "Copy/link to this API element")

```python
random_normal(self: Self, loc: float, scale: float, Self: type)
```

random\_exponential[](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/#class-bitgenerator-random_exponential "Copy/link to this API element")

```python
random_exponential(self: Self, scale: float, Self: type)
```

random\_uniform[](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/#class-bitgenerator-random_uniform "Copy/link to this API element")

```python
random_uniform(self: Self, off: float, rng: float, Self: type)
```

random\_gamma[](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/#class-bitgenerator-random_gamma "Copy/link to this API element")

```python
random_gamma(self: Self, shape: float, scale: float, Self: type)
```

random\_gamma\_f[](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/#class-bitgenerator-random_gamma_f "Copy/link to this API element")

```python
random_gamma_f(self: Self, shape: float32, scale: float32, Self: type)
```

random\_beta[](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/#class-bitgenerator-random_beta "Copy/link to this API element")

```python
random_beta(self: Self, a: float, b: float, Self: type)
```

random\_chisquare[](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/#class-bitgenerator-random_chisquare "Copy/link to this API element")

```python
random_chisquare(self: Self, df: float, Self: type)
```

random\_f[](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/#class-bitgenerator-random_f "Copy/link to this API element")

```python
random_f(self: Self, dfnum: float, dfden: float, Self: type)
```

random\_standard\_cauchy[](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/#class-bitgenerator-random_standard_cauchy "Copy/link to this API element")

```python
random_standard_cauchy(self: Self, Self: type)
```

random\_pareto[](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/#class-bitgenerator-random_pareto "Copy/link to this API element")

```python
random_pareto(self: Self, a: float, Self: type)
```

random\_weibull[](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/#class-bitgenerator-random_weibull "Copy/link to this API element")

```python
random_weibull(self: Self, a: float, Self: type)
```

random\_power[](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/#class-bitgenerator-random_power "Copy/link to this API element")

```python
random_power(self: Self, a: float, Self: type)
```

random\_laplace[](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/#class-bitgenerator-random_laplace "Copy/link to this API element")

```python
random_laplace(self: Self, loc: float, scale: float, Self: type)
```

random\_gumbel[](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/#class-bitgenerator-random_gumbel "Copy/link to this API element")

```python
random_gumbel(self: Self, loc: float, scale: float, Self: type)
```

random\_logistic[](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/#class-bitgenerator-random_logistic "Copy/link to this API element")

```python
random_logistic(self: Self, loc: float, scale: float, Self: type)
```

random\_lognormal[](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/#class-bitgenerator-random_lognormal "Copy/link to this API element")

```python
random_lognormal(self: Self, mean: float, sigma: float, Self: type)
```

random\_rayleigh[](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/#class-bitgenerator-random_rayleigh "Copy/link to this API element")

```python
random_rayleigh(self: Self, mode: float, Self: type)
```

random\_standard\_t[](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/#class-bitgenerator-random_standard_t "Copy/link to this API element")

```python
random_standard_t(self: Self, df: float, Self: type)
```

random\_poisson\_mult[](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/#class-bitgenerator-random_poisson_mult "Copy/link to this API element")

```python
random_poisson_mult(self: Self, lam: float, Self: type)
```

random\_poisson\_ptrs[](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/#class-bitgenerator-random_poisson_ptrs "Copy/link to this API element")

```python
random_poisson_ptrs(self: Self, lam: float, Self: type)
```

random\_poisson[](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/#class-bitgenerator-random_poisson "Copy/link to this API element")

```python
random_poisson(self: Self, lam: float, Self: type)
```

random\_negative\_binomial[](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/#class-bitgenerator-random_negative_binomial "Copy/link to this API element")

```python
random_negative_binomial(self: Self, n: float, p: float, Self: type)
```

random\_binomial\_btpe[](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/#class-bitgenerator-random_binomial_btpe "Copy/link to this API element")

```python
random_binomial_btpe(self: Self, n: int, p: float, Self: type)
```

random\_binomial\_inversion[](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/#class-bitgenerator-random_binomial_inversion "Copy/link to this API element")

```python
random_binomial_inversion(self: Self, n: int, p: float, Self: type)
```

random\_binomial[](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/#class-bitgenerator-random_binomial "Copy/link to this API element")

```python
random_binomial(self: Self, p: float, n: int, Self: type)
```

random\_noncentral\_chisquare[](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/#class-bitgenerator-random_noncentral_chisquare "Copy/link to this API element")

```python
random_noncentral_chisquare(self: Self, df: float, nonc: float, Self: type)
```

random\_noncentral\_f[](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/#class-bitgenerator-random_noncentral_f "Copy/link to this API element")

```python
random_noncentral_f(self: Self, dfnum: float, dfden: float, nonc: float, Self: type)
```

random\_wald[](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/#class-bitgenerator-random_wald "Copy/link to this API element")

```python
random_wald(self: Self, mean: float, scale: float, Self: type)
```

random\_vonmises[](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/#class-bitgenerator-random_vonmises "Copy/link to this API element")

```python
random_vonmises(self: Self, mu: float, kappa: float, Self: type)
```

random\_logseries[](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/#class-bitgenerator-random_logseries "Copy/link to this API element")

```python
random_logseries(self: Self, p: float, Self: type)
```

random\_geometric\_search[](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/#class-bitgenerator-random_geometric_search "Copy/link to this API element")

```python
random_geometric_search(self: Self, p: float, Self: type)
```

random\_geometric\_inversion[](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/#class-bitgenerator-random_geometric_inversion "Copy/link to this API element")

```python
random_geometric_inversion(self: Self, p: float, Self: type)
```

random\_geometric[](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/#class-bitgenerator-random_geometric "Copy/link to this API element")

```python
random_geometric(self: Self, p: float, Self: type)
```

random\_zipf[](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/#class-bitgenerator-random_zipf "Copy/link to this API element")

```python
random_zipf(self: Self, a: float, Self: type)
```

random\_triangular[](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/#class-bitgenerator-random_triangular "Copy/link to this API element")

```python
random_triangular(self: Self, left: float, mode: float, right: float, Self: type)
```

random\_interval[](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/#class-bitgenerator-random_interval "Copy/link to this API element")

```python
random_interval(self: Self, max: u64, Self: type)
```

gen\_mask[](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/#class-bitgenerator-gen_mask "Copy/link to this API element")

```python
gen_mask(max: u64)
```

buffered\_uint16[](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/#class-bitgenerator-buffered_uint16 "Copy/link to this API element")

```python
buffered_uint16(self: Self, bcnt: int, buf: u32, Self: type)
```

buffered\_uint8[](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/#class-bitgenerator-buffered_uint8 "Copy/link to this API element")

```python
buffered_uint8(self: Self, bcnt: int, buf: u32, Self: type)
```

bounded\_masked\_uint64[](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/#class-bitgenerator-bounded_masked_uint64 "Copy/link to this API element")

```python
bounded_masked_uint64(self: Self, rng: u64, mask: u64, Self: type)
```

buffered\_bounded\_masked\_uint32[](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/#class-bitgenerator-buffered_bounded_masked_uint32 "Copy/link to this API element")

```python
buffered_bounded_masked_uint32(self: Self, rng: u32, mask: u32, bcnt: int, buf: u32, Self: type)
```

buffered\_bounded\_masked\_uint16[](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/#class-bitgenerator-buffered_bounded_masked_uint16 "Copy/link to this API element")

```python
buffered_bounded_masked_uint16(self: Self, rng: u16, mask: u16, bcnt: int, buf: u32, Self: type)
```

buffered\_bounded\_masked\_uint8[](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/#class-bitgenerator-buffered_bounded_masked_uint8 "Copy/link to this API element")

```python
buffered_bounded_masked_uint8(self: Self, rng: u8, mask: u8, bcnt: int, buf: u32, Self: type)
```

buffered\_bounded\_bool[](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/#class-bitgenerator-buffered_bounded_bool "Copy/link to this API element")

```python
buffered_bounded_bool(self: Self, off: bool, rng: bool, mask: bool, bcnt: int, buf: u32, Self: type)
```

bounded\_lemire\_uint64[](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/#class-bitgenerator-bounded_lemire_uint64 "Copy/link to this API element")

```python
bounded_lemire_uint64(self: Self, rng: u64, Self: type)
```

buffered\_bounded\_lemire\_uint32[](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/#class-bitgenerator-buffered_bounded_lemire_uint32 "Copy/link to this API element")

```python
buffered_bounded_lemire_uint32(self: Self, rng: u32, bcnt: int, buf: u32, Self: type)
```

buffered\_bounded\_lemire\_uint16[](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/#class-bitgenerator-buffered_bounded_lemire_uint16 "Copy/link to this API element")

```python
buffered_bounded_lemire_uint16(self: Self, rng: u16, bcnt: int, buf: u32, Self: type)
```

buffered\_bounded\_lemire\_uint8[](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/#class-bitgenerator-buffered_bounded_lemire_uint8 "Copy/link to this API element")

```python
buffered_bounded_lemire_uint8(self: Self, rng: u8, bcnt: int, buf: u32, Self: type)
```

random\_bounded\_uint64[](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/#class-bitgenerator-random_bounded_uint64 "Copy/link to this API element")

```python
random_bounded_uint64(self: Self, off: u64, rng: u64, mask: u64, use_masked: bool, Self: type)
```

random\_buffered\_bounded\_uint32[](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/#class-bitgenerator-random_buffered_bounded_uint32 "Copy/link to this API element")

```python
random_buffered_bounded_uint32(self: Self, off: u32, rng: u32, mask: u32, use_masked: bool, bcnt: int, buf: u32, Self: type)
```

random\_buffered\_bounded\_uint16[](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/#class-bitgenerator-random_buffered_bounded_uint16 "Copy/link to this API element")

```python
random_buffered_bounded_uint16(self: Self, off: u16, rng: u16, mask: u16, use_masked: bool, bcnt: int, buf: u32, Self: type)
```

random\_buffered\_bounded\_uint8[](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/#class-bitgenerator-random_buffered_bounded_uint8 "Copy/link to this API element")

```python
random_buffered_bounded_uint8(self: Self, off: u8, rng: u8, mask: u8, use_masked: bool, bcnt: int, buf: u32, Self: type)
```

random\_buffered\_bounded\_bool[](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/#class-bitgenerator-random_buffered_bounded_bool "Copy/link to this API element")

```python
random_buffered_bounded_bool(self: Self, off: bool, rng: bool, mask: bool, use_masked_bool, bcnt: int, buf: u32, Self: type)
```

random\_bounded\_uint64\_fill[](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/#class-bitgenerator-random_bounded_uint64_fill "Copy/link to this API element")

```python
random_bounded_uint64_fill(self: Self, off: u64, rng: u64, cnt: int, use_masked: bool, out: Ptr[u64], Self: type)
```

random\_bounded\_uint32\_fill[](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/#class-bitgenerator-random_bounded_uint32_fill "Copy/link to this API element")

```python
random_bounded_uint32_fill(self: Self, off: u32, rng: u32, cnt: int, use_masked: bool, out: Ptr[u32], Self: type)
```

random\_bounded\_uint16\_fill[](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/#class-bitgenerator-random_bounded_uint16_fill "Copy/link to this API element")

```python
random_bounded_uint16_fill(self: Self, off: u16, rng: u16, cnt: int, use_masked: bool, out: Ptr[u16], Self: type)
```

random\_bounded\_uint8\_fill[](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/#class-bitgenerator-random_bounded_uint8_fill "Copy/link to this API element")

```python
random_bounded_uint8_fill(self: Self, off: u8, rng: u8, cnt: int, use_masked: bool, out: Ptr[u8], Self: type)
```

random\_bounded\_bool\_fill[](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/#class-bitgenerator-random_bounded_bool_fill "Copy/link to this API element")

```python
random_bounded_bool_fill(self: Self, off: bool, rng: bool, cnt: int, use_masked: bool, out: Ptr[bool], Self: type)
```

random\_multinomial[](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/#class-bitgenerator-random_multinomial "Copy/link to this API element")

```python
random_multinomial(self: Self, n: int, mnix: Ptr[int], pix: Ptr[float], d: int, Self: type)
```

hypergeometric\_sample[](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/#class-bitgenerator-hypergeometric_sample "Copy/link to this API element")

```python
hypergeometric_sample(self: Self, good: int, bad: int, sample: int, Self: type)
```

hypergeometric\_hrua[](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/#class-bitgenerator-hypergeometric_hrua "Copy/link to this API element")

```python
hypergeometric_hrua(self: Self, good: int, bad: int, sample: int, Self: type)
```

random\_hypergeometric[](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/#class-bitgenerator-random_hypergeometric "Copy/link to this API element")

```python
random_hypergeometric(self: Self, good: int, bad: int, sample: int, Self: type)
```

random\_multivariate\_hypergeometric\_count[](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/#class-bitgenerator-random_multivariate_hypergeometric_count "Copy/link to this API element")

```python
random_multivariate_hypergeometric_count(self: Self, total: int, num_colors: int, colors: Ptr[int], nsample: int, num_variates: int, variates: Ptr[int], Self: type)
```

random\_multivariate\_hypergeometric\_marginals[](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/#class-bitgenerator-random_multivariate_hypergeometric_marginals "Copy/link to this API element")

```python
random_multivariate_hypergeometric_marginals(self: Self, total: int, num_colors: int, colors: Ptr[int], nsample: int, num_variates: int, variates: Ptr[int], Self: type)
```

next\_raw[](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/#class-bitgenerator-next_raw "Copy/link to this API element")

```python
next_raw(self: Self, Self: type)
```

random\_raw[](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/#class-bitgenerator-random_raw "Copy/link to this API element")

```python
random_raw(self: Self, lock, size, output, Self: type)
```

```python
random_raw(self: Self, size = None, Self: type)
```

shuffle\_raw[](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/#class-bitgenerator-shuffle_raw "Copy/link to this API element")

```python
shuffle_raw(self: Self, n: int, first: int, itemsize: int, stride: int, data: cobj, buf: cobj, Self: type)
```

shuffle\_int[](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/#class-bitgenerator-shuffle_int "Copy/link to this API element")

```python
shuffle_int(self: Self, n: int, first: int, data: Ptr[int], Self: type)
```

shuffle\_raw\_wrap[](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/#class-bitgenerator-shuffle_raw_wrap "Copy/link to this API element")

```python
shuffle_raw_wrap(self: Self, n: int, first: int, itemsize: int, stride: int, data: cobj, buf: cobj, Self: type)
```

function validate\_output\_shape[](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/#function-validate_output_shape "Copy/link to this API element")

```python
validate_output_shape(iter_shape, output: ndarray)
```

function check\_output[](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/#function-check_output "Copy/link to this API element")

```python
check_output(out, dtype: type, size, require_c_array: bool)
```

function double\_fill[](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/#function-double_fill "Copy/link to this API element")

```python
double_fill(func, size, lock, out)
```

function float\_fill[](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/#function-float_fill "Copy/link to this API element")

```python
float_fill(func, size, lock, out)
```

function float\_fill\_from\_double[](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/#function-float_fill_from_double "Copy/link to this API element")

```python
float_fill_from_double(func, size, lock, out)
```

variable CONS\_NONE[](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/#variable-cons_none "Copy/link to this API element")

```python
CONS_NONE: Literal[int] = 0
```

variable CONS\_NON\_NEGATIVE[](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/#variable-cons_non_negative "Copy/link to this API element")

```python
CONS_NON_NEGATIVE: Literal[int] = 1
```

variable CONS\_POSITIVE[](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/#variable-cons_positive "Copy/link to this API element")

```python
CONS_POSITIVE: Literal[int] = 2
```

variable CONS\_POSITIVE\_NOT\_NAN[](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/#variable-cons_positive_not_nan "Copy/link to this API element")

```python
CONS_POSITIVE_NOT_NAN: Literal[int] = 3
```

variable CONS\_BOUNDED\_0\_1[](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/#variable-cons_bounded_0_1 "Copy/link to this API element")

```python
CONS_BOUNDED_0_1: Literal[int] = 4
```

variable CONS\_BOUNDED\_GT\_0\_1[](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/#variable-cons_bounded_gt_0_1 "Copy/link to this API element")

```python
CONS_BOUNDED_GT_0_1: Literal[int] = 5
```

variable CONS\_BOUNDED\_LT\_0\_1[](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/#variable-cons_bounded_lt_0_1 "Copy/link to this API element")

```python
CONS_BOUNDED_LT_0_1: Literal[int] = 6
```

variable CONS\_GT\_1[](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/#variable-cons_gt_1 "Copy/link to this API element")

```python
CONS_GT_1: Literal[int] = 7
```

variable CONS\_GTE\_1[](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/#variable-cons_gte_1 "Copy/link to this API element")

```python
CONS_GTE_1: Literal[int] = 8
```

variable CONS\_POISSON[](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/#variable-cons_poisson "Copy/link to this API element")

```python
CONS_POISSON: Literal[int] = 9
```

variable MAX\_INT[](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/#variable-max_int "Copy/link to this API element")

```python
MAX_INT: Literal[int] = 0x7FFFFFFFFFFFFFFF
```

variable MAXSIZE[](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/#variable-maxsize "Copy/link to this API element")

```python
MAXSIZE: Literal[int] = MAX_INT
```

variable POISSON\_LAM\_MAX[](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/#variable-poisson_lam_max "Copy/link to this API element")

```python
POISSON_LAM_MAX = (MAX_INT - (sqrt(float(MAX_INT)) * 10))
```

function check\_array\_constraint[](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/#function-check_array_constraint "Copy/link to this API element")

```python
check_array_constraint(val, name: str, const: int)
```

function convert\_array\_like[](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/#function-convert_array_like "Copy/link to this API element")

```python
convert_array_like(a)
```

function gather\_arrays[](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/#function-gather_arrays "Copy/link to this API element")

```python
gather_arrays(t)
```

function cont[](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/#function-cont "Copy/link to this API element")

```python
cont(fn, size, lock, arrays, names, constraints, out = None, dtype: type = float)
```

function kahan\_sum[](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/#function-kahan_sum "Copy/link to this API element")

```python
kahan_sum(darr: Ptr[float], n: int)
```

class Generator[](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/#class-generator "Copy/link to this API element")

@tuple

```python
class Generator[G]
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/Generator/)

Fields 3

`lock` `Lock`

`bit_generator` `BitGenerator[G]`

`G` `type`

Constructors 1

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/#class-generator-__new__ "Copy/link to this API element")

```python
__new__(g: G)
```

Methods 44

spawn[](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/#class-generator-spawn "Copy/link to this API element")

```python
spawn(self: Self, n_children: int, Self: type)
```

random[](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/#class-generator-random "Copy/link to this API element")

```python
random(self: Self, size = None, dtype: type = float, out = None, Self: type)
```

beta[](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/#class-generator-beta "Copy/link to this API element")

```python
beta(self: Self, a, b, size = None, Self: type)
```

exponential[](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/#class-generator-exponential "Copy/link to this API element")

```python
exponential(self: Self, scale = 1.0, size = None, Self: type)
```

standard\_exponential[](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/#class-generator-standard_exponential "Copy/link to this API element")

```python
standard_exponential(self: Self, size = None, dtype: type = float, method: str = "zig", out = None, Self: type)
```

integers[](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/#class-generator-integers "Copy/link to this API element")

```python
integers(self: Self, low, high = None, size = None, dtype: type = int, endpoint: bool = False, Self: type)
```

bytes[](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/#class-generator-bytes "Copy/link to this API element")

```python
bytes(self: Self, length: int, Self: type)
```

choice[](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/#class-generator-choice "Copy/link to this API element")

```python
choice(self: Self, a, size = None, replace: bool = True, p = None, axis: int = 0, shuffle: bool = True, Self: type)
```

uniform[](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/#class-generator-uniform "Copy/link to this API element")

```python
uniform(self: Self, low = 0.0, high = 1.0, size = None, Self: type)
```

standard\_normal[](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/#class-generator-standard_normal "Copy/link to this API element")

```python
standard_normal(self: Self, size = None, dtype: type = float, out = None, Self: type)
```

normal[](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/#class-generator-normal "Copy/link to this API element")

```python
normal(self: Self, loc = 0.0, scale = 1.0, size = None, Self: type)
```

standard\_gamma[](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/#class-generator-standard_gamma "Copy/link to this API element")

```python
standard_gamma(self: Self, shape, size = None, dtype: type = float, out = None, Self: type)
```

gamma[](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/#class-generator-gamma "Copy/link to this API element")

```python
gamma(self: Self, shape, scale = 1.0, size = None, Self: type)
```

f[](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/#class-generator-f "Copy/link to this API element")

```python
f(self: Self, dfnum, dfden, size = None, Self: type)
```

noncentral\_f[](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/#class-generator-noncentral_f "Copy/link to this API element")

```python
noncentral_f(self: Self, dfnum, dfden, nonc, size = None, Self: type)
```

chisquare[](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/#class-generator-chisquare "Copy/link to this API element")

```python
chisquare(self: Self, df, size = None, Self: type)
```

noncentral\_chisquare[](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/#class-generator-noncentral_chisquare "Copy/link to this API element")

```python
noncentral_chisquare(self: Self, df, nonc, size = None, Self: type)
```

standard\_cauchy[](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/#class-generator-standard_cauchy "Copy/link to this API element")

```python
standard_cauchy(self: Self, size = None, Self: type)
```

standard\_t[](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/#class-generator-standard_t "Copy/link to this API element")

```python
standard_t(self: Self, df, size = None, Self: type)
```

vonmises[](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/#class-generator-vonmises "Copy/link to this API element")

```python
vonmises(self: Self, mu, kappa, size = None, Self: type)
```

pareto[](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/#class-generator-pareto "Copy/link to this API element")

```python
pareto(self: Self, a, size = None, Self: type)
```

weibull[](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/#class-generator-weibull "Copy/link to this API element")

```python
weibull(self: Self, a, size = None, Self: type)
```

power[](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/#class-generator-power "Copy/link to this API element")

```python
power(self: Self, a, size = None, Self: type)
```

laplace[](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/#class-generator-laplace "Copy/link to this API element")

```python
laplace(self: Self, loc = 0.0, scale = 1.0, size = None, Self: type)
```

gumbel[](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/#class-generator-gumbel "Copy/link to this API element")

```python
gumbel(self: Self, loc = 0.0, scale = 1.0, size = None, Self: type)
```

logistic[](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/#class-generator-logistic "Copy/link to this API element")

```python
logistic(self: Self, loc = 0.0, scale = 1.0, size = None, Self: type)
```

lognormal[](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/#class-generator-lognormal "Copy/link to this API element")

```python
lognormal(self: Self, mean = 0.0, sigma = 1.0, size = None, Self: type)
```

rayleigh[](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/#class-generator-rayleigh "Copy/link to this API element")

```python
rayleigh(self: Self, scale = 1.0, size = None, Self: type)
```

wald[](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/#class-generator-wald "Copy/link to this API element")

```python
wald(self: Self, mean, scale, size = None, Self: type)
```

triangular[](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/#class-generator-triangular "Copy/link to this API element")

```python
triangular(self: Self, left, mode, right, size = None, Self: type)
```

binomial[](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/#class-generator-binomial "Copy/link to this API element")

```python
binomial(self: Self, n, p, size = None, Self: type)
```

negative\_binomial[](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/#class-generator-negative_binomial "Copy/link to this API element")

```python
negative_binomial(self: Self, n, p, size = None, Self: type)
```

poisson[](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/#class-generator-poisson "Copy/link to this API element")

```python
poisson(self: Self, lam = 1.0, size = None, Self: type)
```

zipf[](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/#class-generator-zipf "Copy/link to this API element")

```python
zipf(self: Self, a, size = None, Self: type)
```

geometric[](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/#class-generator-geometric "Copy/link to this API element")

```python
geometric(self: Self, p, size = None, Self: type)
```

hypergeometric[](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/#class-generator-hypergeometric "Copy/link to this API element")

```python
hypergeometric(self: Self, ngood, nbad, nsample, size = None, Self: type)
```

logseries[](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/#class-generator-logseries "Copy/link to this API element")

```python
logseries(self: Self, p, size = None, Self: type)
```

multivariate\_normal[](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/#class-generator-multivariate_normal "Copy/link to this API element")

```python
multivariate_normal(self: Self, mean, cov, size = None, check_valid: Literal[str] = "warn", tol: float = 1e-8, method: Literal[str] = "svd", Self: type)
```

multinomial[](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/#class-generator-multinomial "Copy/link to this API element")

```python
multinomial(self: Self, n, pvals, size = None, Self: type)
```

multivariate\_hypergeometric[](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/#class-generator-multivariate_hypergeometric "Copy/link to this API element")

```python
multivariate_hypergeometric(self: Self, colors, nsample: int, size = None, method: str = "marginals", Self: type)
```

dirichlet[](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/#class-generator-dirichlet "Copy/link to this API element")

```python
dirichlet(self: Self, alpha, size = None, Self: type)
```

permuted[](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/#class-generator-permuted "Copy/link to this API element")

```python
permuted(self: Self, x, axis = None, out = None, Self: type)
```

shuffle[](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/#class-generator-shuffle "Copy/link to this API element")

```python
shuffle(self: Self, x, axis: int = 0, Self: type)
```

permutation[](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/#class-generator-permutation "Copy/link to this API element")

```python
permutation(self: Self, x, axis: int = 0, Self: type)
```

Special methods 1

\_\_str\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/#class-generator-__str__ "Copy/link to this API element")

```python
__str__(self: Self, Self: type)
```

Source: https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/
