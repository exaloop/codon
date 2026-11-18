# Generator

Standard library class `numpy.random.bitgen.Generator` [View source](https://github.com/exaloop/codon/blob/master/stdlib/numpy/random/bitgen.codon#L1912)

class Generator[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/Generator/#class-generator "Copy/link to this API element")

@tuple

```python
class Generator[G]
```

Fields 3

`lock` `Lock`

`bit_generator` `BitGenerator[G]`

`G` `type`

Constructors 1

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/Generator/#class-generator-__new__ "Copy/link to this API element")

```python
__new__(g: G)
```

Defined in [`numpy.random.bitgen`](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/)

Methods 44

spawn[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/Generator/#class-generator-spawn "Copy/link to this API element")

```python
spawn(self: Self, n_children: int)
```

Defined in [`numpy.random.bitgen`](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/)

random[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/Generator/#class-generator-random "Copy/link to this API element")

```python
random[dtype: type = float](self: Self, size = None, out = None)
```

Defined in [`numpy.random.bitgen`](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/)

beta[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/Generator/#class-generator-beta "Copy/link to this API element")

```python
beta(self: Self, a, b, size = None)
```

Defined in [`numpy.random.bitgen`](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/)

exponential[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/Generator/#class-generator-exponential "Copy/link to this API element")

```python
exponential(self: Self, scale = 1.0, size = None)
```

Defined in [`numpy.random.bitgen`](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/)

standard\_exponential[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/Generator/#class-generator-standard_exponential "Copy/link to this API element")

```python
standard_exponential[dtype: type = float](self: Self, size = None, method: str = "zig", out = None)
```

Defined in [`numpy.random.bitgen`](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/)

integers[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/Generator/#class-generator-integers "Copy/link to this API element")

```python
integers[dtype: type = int](self: Self, low, high = None, size = None, endpoint: bool = False)
```

Defined in [`numpy.random.bitgen`](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/)

bytes[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/Generator/#class-generator-bytes "Copy/link to this API element")

```python
bytes(self: Self, length: int)
```

Defined in [`numpy.random.bitgen`](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/)

choice[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/Generator/#class-generator-choice "Copy/link to this API element")

```python
choice(self: Self, a, size = None, replace: bool = True, p = None, axis: int = 0, shuffle: bool = True)
```

Defined in [`numpy.random.bitgen`](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/)

uniform[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/Generator/#class-generator-uniform "Copy/link to this API element")

```python
uniform(self: Self, low = 0.0, high = 1.0, size = None)
```

Defined in [`numpy.random.bitgen`](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/)

standard\_normal[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/Generator/#class-generator-standard_normal "Copy/link to this API element")

```python
standard_normal[dtype: type = float](self: Self, size = None, out = None)
```

Defined in [`numpy.random.bitgen`](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/)

normal[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/Generator/#class-generator-normal "Copy/link to this API element")

```python
normal(self: Self, loc = 0.0, scale = 1.0, size = None)
```

Defined in [`numpy.random.bitgen`](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/)

standard\_gamma[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/Generator/#class-generator-standard_gamma "Copy/link to this API element")

```python
standard_gamma[dtype: type = float](self: Self, shape, size = None, out = None)
```

Defined in [`numpy.random.bitgen`](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/)

gamma[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/Generator/#class-generator-gamma "Copy/link to this API element")

```python
gamma(self: Self, shape, scale = 1.0, size = None)
```

Defined in [`numpy.random.bitgen`](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/)

f[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/Generator/#class-generator-f "Copy/link to this API element")

```python
f(self: Self, dfnum, dfden, size = None)
```

Defined in [`numpy.random.bitgen`](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/)

noncentral\_f[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/Generator/#class-generator-noncentral_f "Copy/link to this API element")

```python
noncentral_f(self: Self, dfnum, dfden, nonc, size = None)
```

Defined in [`numpy.random.bitgen`](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/)

chisquare[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/Generator/#class-generator-chisquare "Copy/link to this API element")

```python
chisquare(self: Self, df, size = None)
```

Defined in [`numpy.random.bitgen`](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/)

noncentral\_chisquare[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/Generator/#class-generator-noncentral_chisquare "Copy/link to this API element")

```python
noncentral_chisquare(self: Self, df, nonc, size = None)
```

Defined in [`numpy.random.bitgen`](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/)

standard\_cauchy[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/Generator/#class-generator-standard_cauchy "Copy/link to this API element")

```python
standard_cauchy(self: Self, size = None)
```

Defined in [`numpy.random.bitgen`](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/)

standard\_t[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/Generator/#class-generator-standard_t "Copy/link to this API element")

```python
standard_t(self: Self, df, size = None)
```

Defined in [`numpy.random.bitgen`](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/)

vonmises[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/Generator/#class-generator-vonmises "Copy/link to this API element")

```python
vonmises(self: Self, mu, kappa, size = None)
```

Defined in [`numpy.random.bitgen`](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/)

pareto[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/Generator/#class-generator-pareto "Copy/link to this API element")

```python
pareto(self: Self, a, size = None)
```

Defined in [`numpy.random.bitgen`](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/)

weibull[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/Generator/#class-generator-weibull "Copy/link to this API element")

```python
weibull(self: Self, a, size = None)
```

Defined in [`numpy.random.bitgen`](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/)

power[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/Generator/#class-generator-power "Copy/link to this API element")

```python
power(self: Self, a, size = None)
```

Defined in [`numpy.random.bitgen`](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/)

laplace[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/Generator/#class-generator-laplace "Copy/link to this API element")

```python
laplace(self: Self, loc = 0.0, scale = 1.0, size = None)
```

Defined in [`numpy.random.bitgen`](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/)

gumbel[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/Generator/#class-generator-gumbel "Copy/link to this API element")

```python
gumbel(self: Self, loc = 0.0, scale = 1.0, size = None)
```

Defined in [`numpy.random.bitgen`](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/)

logistic[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/Generator/#class-generator-logistic "Copy/link to this API element")

```python
logistic(self: Self, loc = 0.0, scale = 1.0, size = None)
```

Defined in [`numpy.random.bitgen`](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/)

lognormal[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/Generator/#class-generator-lognormal "Copy/link to this API element")

```python
lognormal(self: Self, mean = 0.0, sigma = 1.0, size = None)
```

Defined in [`numpy.random.bitgen`](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/)

rayleigh[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/Generator/#class-generator-rayleigh "Copy/link to this API element")

```python
rayleigh(self: Self, scale = 1.0, size = None)
```

Defined in [`numpy.random.bitgen`](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/)

wald[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/Generator/#class-generator-wald "Copy/link to this API element")

```python
wald(self: Self, mean, scale, size = None)
```

Defined in [`numpy.random.bitgen`](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/)

triangular[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/Generator/#class-generator-triangular "Copy/link to this API element")

```python
triangular(self: Self, left, mode, right, size = None)
```

Defined in [`numpy.random.bitgen`](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/)

binomial[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/Generator/#class-generator-binomial "Copy/link to this API element")

```python
binomial(self: Self, n, p, size = None)
```

Defined in [`numpy.random.bitgen`](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/)

negative\_binomial[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/Generator/#class-generator-negative_binomial "Copy/link to this API element")

```python
negative_binomial(self: Self, n, p, size = None)
```

Defined in [`numpy.random.bitgen`](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/)

poisson[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/Generator/#class-generator-poisson "Copy/link to this API element")

```python
poisson(self: Self, lam = 1.0, size = None)
```

Defined in [`numpy.random.bitgen`](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/)

zipf[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/Generator/#class-generator-zipf "Copy/link to this API element")

```python
zipf(self: Self, a, size = None)
```

Defined in [`numpy.random.bitgen`](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/)

geometric[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/Generator/#class-generator-geometric "Copy/link to this API element")

```python
geometric(self: Self, p, size = None)
```

Defined in [`numpy.random.bitgen`](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/)

hypergeometric[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/Generator/#class-generator-hypergeometric "Copy/link to this API element")

```python
hypergeometric(self: Self, ngood, nbad, nsample, size = None)
```

Defined in [`numpy.random.bitgen`](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/)

logseries[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/Generator/#class-generator-logseries "Copy/link to this API element")

```python
logseries(self: Self, p, size = None)
```

Defined in [`numpy.random.bitgen`](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/)

multivariate\_normal[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/Generator/#class-generator-multivariate_normal "Copy/link to this API element")

```python
multivariate_normal[check_valid: Literal[str] = "warn", method: Literal[str] = "svd"](self: Self, mean, cov, size = None, tol: float = 1e-8)
```

Defined in [`numpy.random.bitgen`](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/)

multinomial[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/Generator/#class-generator-multinomial "Copy/link to this API element")

```python
multinomial(self: Self, n, pvals, size = None)
```

Defined in [`numpy.random.bitgen`](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/)

multivariate\_hypergeometric[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/Generator/#class-generator-multivariate_hypergeometric "Copy/link to this API element")

```python
multivariate_hypergeometric(self: Self, colors, nsample: int, size = None, method: str = "marginals")
```

Defined in [`numpy.random.bitgen`](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/)

dirichlet[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/Generator/#class-generator-dirichlet "Copy/link to this API element")

```python
dirichlet(self: Self, alpha, size = None)
```

Defined in [`numpy.random.bitgen`](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/)

permuted[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/Generator/#class-generator-permuted "Copy/link to this API element")

```python
permuted(self: Self, x, axis = None, out = None)
```

Defined in [`numpy.random.bitgen`](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/)

shuffle[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/Generator/#class-generator-shuffle "Copy/link to this API element")

```python
shuffle(self: Self, x, axis: int = 0)
```

Defined in [`numpy.random.bitgen`](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/)

permutation[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/Generator/#class-generator-permutation "Copy/link to this API element")

```python
permutation(self: Self, x, axis: int = 0)
```

Defined in [`numpy.random.bitgen`](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/)

Special methods 1

\_\_str\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/Generator/#class-generator-__str__ "Copy/link to this API element")

```python
__str__(self: Self)
```

Defined in [`numpy.random.bitgen`](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/)

Source: https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/Generator/
