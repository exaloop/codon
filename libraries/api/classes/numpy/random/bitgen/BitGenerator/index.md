# BitGenerator

Standard library class `numpy.random.bitgen.BitGenerator` [View source](https://github.com/exaloop/codon/blob/master/stdlib/numpy/random/bitgen.codon)

class BitGenerator[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/BitGenerator/#class-bitgenerator "Copy/link to this API element")

```python
class BitGenerator[G]
```

Fields 5

`state` `G`

`binomial` `Optional[Binomial]`

`uinteger` `u32`

`has_uint32` `bool`

`G` `type`

Constructors 1

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/BitGenerator/#class-bitgenerator-__init__ "Copy/link to this API element")

```python
__init__(self: Self, state: G, Self: type)
```

Defined in [`numpy.random.bitgen`](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/)

Properties 1

seed\_seq[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/BitGenerator/#class-bitgenerator-seed_seq "Copy/link to this API element")

@property

```python
seed_seq(self: Self, Self: type)
```

Defined in [`numpy.random.bitgen`](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/)

Methods 97

next64[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/BitGenerator/#class-bitgenerator-next64 "Copy/link to this API element")

```python
next64(self: Self, Self: type)
```

Defined in [`numpy.random.bitgen`](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/)

next32[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/BitGenerator/#class-bitgenerator-next32 "Copy/link to this API element")

```python
next32(self: Self, Self: type)
```

Defined in [`numpy.random.bitgen`](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/)

next\_double[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/BitGenerator/#class-bitgenerator-next_double "Copy/link to this API element")

```python
next_double(self: Self, Self: type)
```

Defined in [`numpy.random.bitgen`](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/)

next\_float[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/BitGenerator/#class-bitgenerator-next_float "Copy/link to this API element")

```python
next_float(self: Self, Self: type)
```

Defined in [`numpy.random.bitgen`](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/)

jumped[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/BitGenerator/#class-bitgenerator-jumped "Copy/link to this API element")

```python
jumped(self: Self, jumps: int, Self: type)
```

Defined in [`numpy.random.bitgen`](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/)

standard\_uniform[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/BitGenerator/#class-bitgenerator-standard_uniform "Copy/link to this API element")

```python
standard_uniform(self: Self, Self: type)
```

Defined in [`numpy.random.bitgen`](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/)

standard\_uniform\_f[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/BitGenerator/#class-bitgenerator-standard_uniform_f "Copy/link to this API element")

```python
standard_uniform_f(self: Self, Self: type)
```

Defined in [`numpy.random.bitgen`](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/)

random\_standard\_uniform\_fill[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/BitGenerator/#class-bitgenerator-random_standard_uniform_fill "Copy/link to this API element")

```python
random_standard_uniform_fill(self: Self, cnt: int, out: Ptr[float], Self: type)
```

Defined in [`numpy.random.bitgen`](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/)

random\_standard\_uniform\_fill\_f[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/BitGenerator/#class-bitgenerator-random_standard_uniform_fill_f "Copy/link to this API element")

```python
random_standard_uniform_fill_f(self: Self, cnt: int, out: Ptr[float32], Self: type)
```

Defined in [`numpy.random.bitgen`](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/)

standard\_exponential\_unlikely[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/BitGenerator/#class-bitgenerator-standard_exponential_unlikely "Copy/link to this API element")

```python
standard_exponential_unlikely(self: Self, idx: int, x: float, Self: type)
```

Defined in [`numpy.random.bitgen`](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/)

random\_standard\_exponential[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/BitGenerator/#class-bitgenerator-random_standard_exponential "Copy/link to this API element")

```python
random_standard_exponential(self: Self, Self: type)
```

Defined in [`numpy.random.bitgen`](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/)

random\_standard\_exponential\_fill[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/BitGenerator/#class-bitgenerator-random_standard_exponential_fill "Copy/link to this API element")

```python
random_standard_exponential_fill(self: Self, cnt: int, out: Ptr[float], Self: type)
```

Defined in [`numpy.random.bitgen`](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/)

standard\_exponential\_unlikely\_f[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/BitGenerator/#class-bitgenerator-standard_exponential_unlikely_f "Copy/link to this API element")

```python
standard_exponential_unlikely_f(self: Self, idx: int, x: float32, Self: type)
```

Defined in [`numpy.random.bitgen`](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/)

random\_standard\_exponential\_f[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/BitGenerator/#class-bitgenerator-random_standard_exponential_f "Copy/link to this API element")

```python
random_standard_exponential_f(self: Self, Self: type)
```

Defined in [`numpy.random.bitgen`](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/)

random\_standard\_exponential\_fill\_f[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/BitGenerator/#class-bitgenerator-random_standard_exponential_fill_f "Copy/link to this API element")

```python
random_standard_exponential_fill_f(self: Self, cnt: int, out: Ptr[float32], Self: type)
```

Defined in [`numpy.random.bitgen`](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/)

random\_standard\_exponential\_inv\_fill[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/BitGenerator/#class-bitgenerator-random_standard_exponential_inv_fill "Copy/link to this API element")

```python
random_standard_exponential_inv_fill(self: Self, cnt: int, out: Ptr[float], Self: type)
```

Defined in [`numpy.random.bitgen`](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/)

random\_standard\_exponential\_inv\_fill\_f[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/BitGenerator/#class-bitgenerator-random_standard_exponential_inv_fill_f "Copy/link to this API element")

```python
random_standard_exponential_inv_fill_f(self: Self, cnt: int, out: Ptr[float32], Self: type)
```

Defined in [`numpy.random.bitgen`](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/)

random\_standard\_normal[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/BitGenerator/#class-bitgenerator-random_standard_normal "Copy/link to this API element")

```python
random_standard_normal(self: Self, Self: type)
```

Defined in [`numpy.random.bitgen`](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/)

random\_standard\_normal\_fill[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/BitGenerator/#class-bitgenerator-random_standard_normal_fill "Copy/link to this API element")

```python
random_standard_normal_fill(self: Self, cnt: int, out: Ptr[float], Self: type)
```

Defined in [`numpy.random.bitgen`](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/)

random\_standard\_normal\_f[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/BitGenerator/#class-bitgenerator-random_standard_normal_f "Copy/link to this API element")

```python
random_standard_normal_f(self: Self, Self: type)
```

Defined in [`numpy.random.bitgen`](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/)

random\_standard\_normal\_fill\_f[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/BitGenerator/#class-bitgenerator-random_standard_normal_fill_f "Copy/link to this API element")

```python
random_standard_normal_fill_f(self: Self, cnt: int, out: Ptr[float32], Self: type)
```

Defined in [`numpy.random.bitgen`](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/)

random\_standard\_gamma[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/BitGenerator/#class-bitgenerator-random_standard_gamma "Copy/link to this API element")

```python
random_standard_gamma(self: Self, shape: float, Self: type)
```

Defined in [`numpy.random.bitgen`](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/)

random\_standard\_gamma\_f[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/BitGenerator/#class-bitgenerator-random_standard_gamma_f "Copy/link to this API element")

```python
random_standard_gamma_f(self: Self, shape: float32, Self: type)
```

Defined in [`numpy.random.bitgen`](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/)

random\_positive\_int64[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/BitGenerator/#class-bitgenerator-random_positive_int64 "Copy/link to this API element")

```python
random_positive_int64(self: Self, Self: type)
```

Defined in [`numpy.random.bitgen`](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/)

random\_positive\_int32[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/BitGenerator/#class-bitgenerator-random_positive_int32 "Copy/link to this API element")

```python
random_positive_int32(self: Self, Self: type)
```

Defined in [`numpy.random.bitgen`](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/)

random\_positive\_int[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/BitGenerator/#class-bitgenerator-random_positive_int "Copy/link to this API element")

```python
random_positive_int(self: Self, Self: type)
```

Defined in [`numpy.random.bitgen`](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/)

random\_uint[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/BitGenerator/#class-bitgenerator-random_uint "Copy/link to this API element")

```python
random_uint(self: Self, Self: type)
```

Defined in [`numpy.random.bitgen`](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/)

random\_loggam[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/BitGenerator/#class-bitgenerator-random_loggam "Copy/link to this API element")

```python
random_loggam(x: float)
```

Defined in [`numpy.random.bitgen`](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/)

random\_normal[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/BitGenerator/#class-bitgenerator-random_normal "Copy/link to this API element")

```python
random_normal(self: Self, loc: float, scale: float, Self: type)
```

Defined in [`numpy.random.bitgen`](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/)

random\_exponential[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/BitGenerator/#class-bitgenerator-random_exponential "Copy/link to this API element")

```python
random_exponential(self: Self, scale: float, Self: type)
```

Defined in [`numpy.random.bitgen`](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/)

random\_uniform[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/BitGenerator/#class-bitgenerator-random_uniform "Copy/link to this API element")

```python
random_uniform(self: Self, off: float, rng: float, Self: type)
```

Defined in [`numpy.random.bitgen`](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/)

random\_gamma[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/BitGenerator/#class-bitgenerator-random_gamma "Copy/link to this API element")

```python
random_gamma(self: Self, shape: float, scale: float, Self: type)
```

Defined in [`numpy.random.bitgen`](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/)

random\_gamma\_f[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/BitGenerator/#class-bitgenerator-random_gamma_f "Copy/link to this API element")

```python
random_gamma_f(self: Self, shape: float32, scale: float32, Self: type)
```

Defined in [`numpy.random.bitgen`](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/)

random\_beta[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/BitGenerator/#class-bitgenerator-random_beta "Copy/link to this API element")

```python
random_beta(self: Self, a: float, b: float, Self: type)
```

Defined in [`numpy.random.bitgen`](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/)

random\_chisquare[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/BitGenerator/#class-bitgenerator-random_chisquare "Copy/link to this API element")

```python
random_chisquare(self: Self, df: float, Self: type)
```

Defined in [`numpy.random.bitgen`](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/)

random\_f[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/BitGenerator/#class-bitgenerator-random_f "Copy/link to this API element")

```python
random_f(self: Self, dfnum: float, dfden: float, Self: type)
```

Defined in [`numpy.random.bitgen`](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/)

random\_standard\_cauchy[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/BitGenerator/#class-bitgenerator-random_standard_cauchy "Copy/link to this API element")

```python
random_standard_cauchy(self: Self, Self: type)
```

Defined in [`numpy.random.bitgen`](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/)

random\_pareto[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/BitGenerator/#class-bitgenerator-random_pareto "Copy/link to this API element")

```python
random_pareto(self: Self, a: float, Self: type)
```

Defined in [`numpy.random.bitgen`](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/)

random\_weibull[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/BitGenerator/#class-bitgenerator-random_weibull "Copy/link to this API element")

```python
random_weibull(self: Self, a: float, Self: type)
```

Defined in [`numpy.random.bitgen`](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/)

random\_power[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/BitGenerator/#class-bitgenerator-random_power "Copy/link to this API element")

```python
random_power(self: Self, a: float, Self: type)
```

Defined in [`numpy.random.bitgen`](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/)

random\_laplace[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/BitGenerator/#class-bitgenerator-random_laplace "Copy/link to this API element")

```python
random_laplace(self: Self, loc: float, scale: float, Self: type)
```

Defined in [`numpy.random.bitgen`](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/)

random\_gumbel[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/BitGenerator/#class-bitgenerator-random_gumbel "Copy/link to this API element")

```python
random_gumbel(self: Self, loc: float, scale: float, Self: type)
```

Defined in [`numpy.random.bitgen`](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/)

random\_logistic[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/BitGenerator/#class-bitgenerator-random_logistic "Copy/link to this API element")

```python
random_logistic(self: Self, loc: float, scale: float, Self: type)
```

Defined in [`numpy.random.bitgen`](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/)

random\_lognormal[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/BitGenerator/#class-bitgenerator-random_lognormal "Copy/link to this API element")

```python
random_lognormal(self: Self, mean: float, sigma: float, Self: type)
```

Defined in [`numpy.random.bitgen`](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/)

random\_rayleigh[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/BitGenerator/#class-bitgenerator-random_rayleigh "Copy/link to this API element")

```python
random_rayleigh(self: Self, mode: float, Self: type)
```

Defined in [`numpy.random.bitgen`](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/)

random\_standard\_t[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/BitGenerator/#class-bitgenerator-random_standard_t "Copy/link to this API element")

```python
random_standard_t(self: Self, df: float, Self: type)
```

Defined in [`numpy.random.bitgen`](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/)

random\_poisson\_mult[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/BitGenerator/#class-bitgenerator-random_poisson_mult "Copy/link to this API element")

```python
random_poisson_mult(self: Self, lam: float, Self: type)
```

Defined in [`numpy.random.bitgen`](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/)

random\_poisson\_ptrs[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/BitGenerator/#class-bitgenerator-random_poisson_ptrs "Copy/link to this API element")

```python
random_poisson_ptrs(self: Self, lam: float, Self: type)
```

Defined in [`numpy.random.bitgen`](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/)

random\_poisson[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/BitGenerator/#class-bitgenerator-random_poisson "Copy/link to this API element")

```python
random_poisson(self: Self, lam: float, Self: type)
```

Defined in [`numpy.random.bitgen`](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/)

random\_negative\_binomial[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/BitGenerator/#class-bitgenerator-random_negative_binomial "Copy/link to this API element")

```python
random_negative_binomial(self: Self, n: float, p: float, Self: type)
```

Defined in [`numpy.random.bitgen`](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/)

random\_binomial\_btpe[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/BitGenerator/#class-bitgenerator-random_binomial_btpe "Copy/link to this API element")

```python
random_binomial_btpe(self: Self, n: int, p: float, Self: type)
```

Defined in [`numpy.random.bitgen`](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/)

random\_binomial\_inversion[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/BitGenerator/#class-bitgenerator-random_binomial_inversion "Copy/link to this API element")

```python
random_binomial_inversion(self: Self, n: int, p: float, Self: type)
```

Defined in [`numpy.random.bitgen`](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/)

random\_binomial[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/BitGenerator/#class-bitgenerator-random_binomial "Copy/link to this API element")

```python
random_binomial(self: Self, p: float, n: int, Self: type)
```

Defined in [`numpy.random.bitgen`](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/)

random\_noncentral\_chisquare[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/BitGenerator/#class-bitgenerator-random_noncentral_chisquare "Copy/link to this API element")

```python
random_noncentral_chisquare(self: Self, df: float, nonc: float, Self: type)
```

Defined in [`numpy.random.bitgen`](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/)

random\_noncentral\_f[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/BitGenerator/#class-bitgenerator-random_noncentral_f "Copy/link to this API element")

```python
random_noncentral_f(self: Self, dfnum: float, dfden: float, nonc: float, Self: type)
```

Defined in [`numpy.random.bitgen`](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/)

random\_wald[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/BitGenerator/#class-bitgenerator-random_wald "Copy/link to this API element")

```python
random_wald(self: Self, mean: float, scale: float, Self: type)
```

Defined in [`numpy.random.bitgen`](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/)

random\_vonmises[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/BitGenerator/#class-bitgenerator-random_vonmises "Copy/link to this API element")

```python
random_vonmises(self: Self, mu: float, kappa: float, Self: type)
```

Defined in [`numpy.random.bitgen`](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/)

random\_logseries[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/BitGenerator/#class-bitgenerator-random_logseries "Copy/link to this API element")

```python
random_logseries(self: Self, p: float, Self: type)
```

Defined in [`numpy.random.bitgen`](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/)

random\_geometric\_search[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/BitGenerator/#class-bitgenerator-random_geometric_search "Copy/link to this API element")

```python
random_geometric_search(self: Self, p: float, Self: type)
```

Defined in [`numpy.random.bitgen`](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/)

random\_geometric\_inversion[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/BitGenerator/#class-bitgenerator-random_geometric_inversion "Copy/link to this API element")

```python
random_geometric_inversion(self: Self, p: float, Self: type)
```

Defined in [`numpy.random.bitgen`](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/)

random\_geometric[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/BitGenerator/#class-bitgenerator-random_geometric "Copy/link to this API element")

```python
random_geometric(self: Self, p: float, Self: type)
```

Defined in [`numpy.random.bitgen`](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/)

random\_zipf[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/BitGenerator/#class-bitgenerator-random_zipf "Copy/link to this API element")

```python
random_zipf(self: Self, a: float, Self: type)
```

Defined in [`numpy.random.bitgen`](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/)

random\_triangular[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/BitGenerator/#class-bitgenerator-random_triangular "Copy/link to this API element")

```python
random_triangular(self: Self, left: float, mode: float, right: float, Self: type)
```

Defined in [`numpy.random.bitgen`](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/)

random\_interval[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/BitGenerator/#class-bitgenerator-random_interval "Copy/link to this API element")

```python
random_interval(self: Self, max: u64, Self: type)
```

Defined in [`numpy.random.bitgen`](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/)

gen\_mask[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/BitGenerator/#class-bitgenerator-gen_mask "Copy/link to this API element")

```python
gen_mask(max: u64)
```

Defined in [`numpy.random.bitgen`](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/)

buffered\_uint16[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/BitGenerator/#class-bitgenerator-buffered_uint16 "Copy/link to this API element")

```python
buffered_uint16(self: Self, bcnt: int, buf: u32, Self: type)
```

Defined in [`numpy.random.bitgen`](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/)

buffered\_uint8[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/BitGenerator/#class-bitgenerator-buffered_uint8 "Copy/link to this API element")

```python
buffered_uint8(self: Self, bcnt: int, buf: u32, Self: type)
```

Defined in [`numpy.random.bitgen`](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/)

bounded\_masked\_uint64[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/BitGenerator/#class-bitgenerator-bounded_masked_uint64 "Copy/link to this API element")

```python
bounded_masked_uint64(self: Self, rng: u64, mask: u64, Self: type)
```

Defined in [`numpy.random.bitgen`](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/)

buffered\_bounded\_masked\_uint32[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/BitGenerator/#class-bitgenerator-buffered_bounded_masked_uint32 "Copy/link to this API element")

```python
buffered_bounded_masked_uint32(self: Self, rng: u32, mask: u32, bcnt: int, buf: u32, Self: type)
```

Defined in [`numpy.random.bitgen`](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/)

buffered\_bounded\_masked\_uint16[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/BitGenerator/#class-bitgenerator-buffered_bounded_masked_uint16 "Copy/link to this API element")

```python
buffered_bounded_masked_uint16(self: Self, rng: u16, mask: u16, bcnt: int, buf: u32, Self: type)
```

Defined in [`numpy.random.bitgen`](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/)

buffered\_bounded\_masked\_uint8[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/BitGenerator/#class-bitgenerator-buffered_bounded_masked_uint8 "Copy/link to this API element")

```python
buffered_bounded_masked_uint8(self: Self, rng: u8, mask: u8, bcnt: int, buf: u32, Self: type)
```

Defined in [`numpy.random.bitgen`](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/)

buffered\_bounded\_bool[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/BitGenerator/#class-bitgenerator-buffered_bounded_bool "Copy/link to this API element")

```python
buffered_bounded_bool(self: Self, off: bool, rng: bool, mask: bool, bcnt: int, buf: u32, Self: type)
```

Defined in [`numpy.random.bitgen`](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/)

bounded\_lemire\_uint64[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/BitGenerator/#class-bitgenerator-bounded_lemire_uint64 "Copy/link to this API element")

```python
bounded_lemire_uint64(self: Self, rng: u64, Self: type)
```

Defined in [`numpy.random.bitgen`](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/)

buffered\_bounded\_lemire\_uint32[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/BitGenerator/#class-bitgenerator-buffered_bounded_lemire_uint32 "Copy/link to this API element")

```python
buffered_bounded_lemire_uint32(self: Self, rng: u32, bcnt: int, buf: u32, Self: type)
```

Defined in [`numpy.random.bitgen`](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/)

buffered\_bounded\_lemire\_uint16[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/BitGenerator/#class-bitgenerator-buffered_bounded_lemire_uint16 "Copy/link to this API element")

```python
buffered_bounded_lemire_uint16(self: Self, rng: u16, bcnt: int, buf: u32, Self: type)
```

Defined in [`numpy.random.bitgen`](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/)

buffered\_bounded\_lemire\_uint8[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/BitGenerator/#class-bitgenerator-buffered_bounded_lemire_uint8 "Copy/link to this API element")

```python
buffered_bounded_lemire_uint8(self: Self, rng: u8, bcnt: int, buf: u32, Self: type)
```

Defined in [`numpy.random.bitgen`](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/)

random\_bounded\_uint64[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/BitGenerator/#class-bitgenerator-random_bounded_uint64 "Copy/link to this API element")

```python
random_bounded_uint64(self: Self, off: u64, rng: u64, mask: u64, use_masked: bool, Self: type)
```

Defined in [`numpy.random.bitgen`](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/)

random\_buffered\_bounded\_uint32[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/BitGenerator/#class-bitgenerator-random_buffered_bounded_uint32 "Copy/link to this API element")

```python
random_buffered_bounded_uint32(self: Self, off: u32, rng: u32, mask: u32, use_masked: bool, bcnt: int, buf: u32, Self: type)
```

Defined in [`numpy.random.bitgen`](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/)

random\_buffered\_bounded\_uint16[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/BitGenerator/#class-bitgenerator-random_buffered_bounded_uint16 "Copy/link to this API element")

```python
random_buffered_bounded_uint16(self: Self, off: u16, rng: u16, mask: u16, use_masked: bool, bcnt: int, buf: u32, Self: type)
```

Defined in [`numpy.random.bitgen`](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/)

random\_buffered\_bounded\_uint8[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/BitGenerator/#class-bitgenerator-random_buffered_bounded_uint8 "Copy/link to this API element")

```python
random_buffered_bounded_uint8(self: Self, off: u8, rng: u8, mask: u8, use_masked: bool, bcnt: int, buf: u32, Self: type)
```

Defined in [`numpy.random.bitgen`](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/)

random\_buffered\_bounded\_bool[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/BitGenerator/#class-bitgenerator-random_buffered_bounded_bool "Copy/link to this API element")

```python
random_buffered_bounded_bool(self: Self, off: bool, rng: bool, mask: bool, use_masked_bool, bcnt: int, buf: u32, Self: type)
```

Defined in [`numpy.random.bitgen`](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/)

random\_bounded\_uint64\_fill[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/BitGenerator/#class-bitgenerator-random_bounded_uint64_fill "Copy/link to this API element")

```python
random_bounded_uint64_fill(self: Self, off: u64, rng: u64, cnt: int, use_masked: bool, out: Ptr[u64], Self: type)
```

Defined in [`numpy.random.bitgen`](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/)

random\_bounded\_uint32\_fill[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/BitGenerator/#class-bitgenerator-random_bounded_uint32_fill "Copy/link to this API element")

```python
random_bounded_uint32_fill(self: Self, off: u32, rng: u32, cnt: int, use_masked: bool, out: Ptr[u32], Self: type)
```

Defined in [`numpy.random.bitgen`](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/)

random\_bounded\_uint16\_fill[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/BitGenerator/#class-bitgenerator-random_bounded_uint16_fill "Copy/link to this API element")

```python
random_bounded_uint16_fill(self: Self, off: u16, rng: u16, cnt: int, use_masked: bool, out: Ptr[u16], Self: type)
```

Defined in [`numpy.random.bitgen`](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/)

random\_bounded\_uint8\_fill[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/BitGenerator/#class-bitgenerator-random_bounded_uint8_fill "Copy/link to this API element")

```python
random_bounded_uint8_fill(self: Self, off: u8, rng: u8, cnt: int, use_masked: bool, out: Ptr[u8], Self: type)
```

Defined in [`numpy.random.bitgen`](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/)

random\_bounded\_bool\_fill[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/BitGenerator/#class-bitgenerator-random_bounded_bool_fill "Copy/link to this API element")

```python
random_bounded_bool_fill(self: Self, off: bool, rng: bool, cnt: int, use_masked: bool, out: Ptr[bool], Self: type)
```

Defined in [`numpy.random.bitgen`](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/)

random\_multinomial[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/BitGenerator/#class-bitgenerator-random_multinomial "Copy/link to this API element")

```python
random_multinomial(self: Self, n: int, mnix: Ptr[int], pix: Ptr[float], d: int, Self: type)
```

Defined in [`numpy.random.bitgen`](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/)

hypergeometric\_sample[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/BitGenerator/#class-bitgenerator-hypergeometric_sample "Copy/link to this API element")

```python
hypergeometric_sample(self: Self, good: int, bad: int, sample: int, Self: type)
```

Defined in [`numpy.random.bitgen`](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/)

hypergeometric\_hrua[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/BitGenerator/#class-bitgenerator-hypergeometric_hrua "Copy/link to this API element")

```python
hypergeometric_hrua(self: Self, good: int, bad: int, sample: int, Self: type)
```

Defined in [`numpy.random.bitgen`](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/)

random\_hypergeometric[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/BitGenerator/#class-bitgenerator-random_hypergeometric "Copy/link to this API element")

```python
random_hypergeometric(self: Self, good: int, bad: int, sample: int, Self: type)
```

Defined in [`numpy.random.bitgen`](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/)

random\_multivariate\_hypergeometric\_count[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/BitGenerator/#class-bitgenerator-random_multivariate_hypergeometric_count "Copy/link to this API element")

```python
random_multivariate_hypergeometric_count(self: Self, total: int, num_colors: int, colors: Ptr[int], nsample: int, num_variates: int, variates: Ptr[int], Self: type)
```

Defined in [`numpy.random.bitgen`](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/)

random\_multivariate\_hypergeometric\_marginals[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/BitGenerator/#class-bitgenerator-random_multivariate_hypergeometric_marginals "Copy/link to this API element")

```python
random_multivariate_hypergeometric_marginals(self: Self, total: int, num_colors: int, colors: Ptr[int], nsample: int, num_variates: int, variates: Ptr[int], Self: type)
```

Defined in [`numpy.random.bitgen`](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/)

next\_raw[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/BitGenerator/#class-bitgenerator-next_raw "Copy/link to this API element")

```python
next_raw(self: Self, Self: type)
```

Defined in [`numpy.random.bitgen`](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/)

random\_raw[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/BitGenerator/#class-bitgenerator-random_raw "Copy/link to this API element")

```python
random_raw(self: Self, lock, size, output, Self: type)
```

Defined in [`numpy.random.bitgen`](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/)

```python
random_raw(self: Self, size = None, Self: type)
```

Defined in [`numpy.random.bitgen`](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/)

shuffle\_raw[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/BitGenerator/#class-bitgenerator-shuffle_raw "Copy/link to this API element")

```python
shuffle_raw(self: Self, n: int, first: int, itemsize: int, stride: int, data: cobj, buf: cobj, Self: type)
```

Defined in [`numpy.random.bitgen`](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/)

shuffle\_int[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/BitGenerator/#class-bitgenerator-shuffle_int "Copy/link to this API element")

```python
shuffle_int(self: Self, n: int, first: int, data: Ptr[int], Self: type)
```

Defined in [`numpy.random.bitgen`](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/)

shuffle\_raw\_wrap[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/BitGenerator/#class-bitgenerator-shuffle_raw_wrap "Copy/link to this API element")

```python
shuffle_raw_wrap(self: Self, n: int, first: int, itemsize: int, stride: int, data: cobj, buf: cobj, Self: type)
```

Defined in [`numpy.random.bitgen`](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/)

Source: https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/BitGenerator/
