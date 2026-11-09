# Generator

Standard library class `numpy.random.bitgen.Generator` [View source](https://github.com/exaloop/codon/blob/master/stdlib/numpy/random/bitgen.codon)

class Generator[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/Generator/#class-generator "Copy/link to this API element")

@tuple

```python
class Generator[G]
```

Fields 4

`lock` `Lock`

`bit_generator` `BitGenerator[G]`

`G` `type`

`T` `type`

Constructors 3

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/Generator/#class-generator-__new__ "Copy/link to this API element")

```python
__new__(g: G)
```

Defined in [`numpy.random.bitgen`](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/)

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/Generator/#class-generator-__new__-overload-2 "Copy/link to this API element")

@pure @derives @llvm

```python
__new__(ptr: cobj)
```

Added by [`internal.types.generator`](https://docs.exaloop.io/libraries/api/modules/internal/types/generator/)

```llvm
ret ptr %ptr
```

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/Generator/#class-generator-__new__-overload-3 "Copy/link to this API element")

@overload

```python
__new__()
```

Added by [`internal.types.generator`](https://docs.exaloop.io/libraries/api/modules/internal/types/generator/)

Methods 45

spawn[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/Generator/#class-generator-spawn "Copy/link to this API element")

```python
spawn(self: Self, n_children: int, Self: type)
```

Defined in [`numpy.random.bitgen`](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/)

random[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/Generator/#class-generator-random "Copy/link to this API element")

```python
random(self: Self, size = None, dtype: type = float, out = None, Self: type)
```

Defined in [`numpy.random.bitgen`](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/)

beta[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/Generator/#class-generator-beta "Copy/link to this API element")

```python
beta(self: Self, a, b, size = None, Self: type)
```

Defined in [`numpy.random.bitgen`](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/)

exponential[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/Generator/#class-generator-exponential "Copy/link to this API element")

```python
exponential(self: Self, scale = 1.0, size = None, Self: type)
```

Defined in [`numpy.random.bitgen`](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/)

standard\_exponential[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/Generator/#class-generator-standard_exponential "Copy/link to this API element")

```python
standard_exponential(self: Self, size = None, dtype: type = float, method: str = "zig", out = None, Self: type)
```

Defined in [`numpy.random.bitgen`](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/)

integers[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/Generator/#class-generator-integers "Copy/link to this API element")

```python
integers(self: Self, low, high = None, size = None, dtype: type = int, endpoint: bool = False, Self: type)
```

Defined in [`numpy.random.bitgen`](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/)

bytes[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/Generator/#class-generator-bytes "Copy/link to this API element")

```python
bytes(self: Self, length: int, Self: type)
```

Defined in [`numpy.random.bitgen`](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/)

choice[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/Generator/#class-generator-choice "Copy/link to this API element")

```python
choice(self: Self, a, size = None, replace: bool = True, p = None, axis: int = 0, shuffle: bool = True, Self: type)
```

Defined in [`numpy.random.bitgen`](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/)

uniform[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/Generator/#class-generator-uniform "Copy/link to this API element")

```python
uniform(self: Self, low = 0.0, high = 1.0, size = None, Self: type)
```

Defined in [`numpy.random.bitgen`](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/)

standard\_normal[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/Generator/#class-generator-standard_normal "Copy/link to this API element")

```python
standard_normal(self: Self, size = None, dtype: type = float, out = None, Self: type)
```

Defined in [`numpy.random.bitgen`](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/)

normal[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/Generator/#class-generator-normal "Copy/link to this API element")

```python
normal(self: Self, loc = 0.0, scale = 1.0, size = None, Self: type)
```

Defined in [`numpy.random.bitgen`](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/)

standard\_gamma[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/Generator/#class-generator-standard_gamma "Copy/link to this API element")

```python
standard_gamma(self: Self, shape, size = None, dtype: type = float, out = None, Self: type)
```

Defined in [`numpy.random.bitgen`](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/)

gamma[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/Generator/#class-generator-gamma "Copy/link to this API element")

```python
gamma(self: Self, shape, scale = 1.0, size = None, Self: type)
```

Defined in [`numpy.random.bitgen`](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/)

f[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/Generator/#class-generator-f "Copy/link to this API element")

```python
f(self: Self, dfnum, dfden, size = None, Self: type)
```

Defined in [`numpy.random.bitgen`](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/)

noncentral\_f[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/Generator/#class-generator-noncentral_f "Copy/link to this API element")

```python
noncentral_f(self: Self, dfnum, dfden, nonc, size = None, Self: type)
```

Defined in [`numpy.random.bitgen`](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/)

chisquare[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/Generator/#class-generator-chisquare "Copy/link to this API element")

```python
chisquare(self: Self, df, size = None, Self: type)
```

Defined in [`numpy.random.bitgen`](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/)

noncentral\_chisquare[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/Generator/#class-generator-noncentral_chisquare "Copy/link to this API element")

```python
noncentral_chisquare(self: Self, df, nonc, size = None, Self: type)
```

Defined in [`numpy.random.bitgen`](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/)

standard\_cauchy[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/Generator/#class-generator-standard_cauchy "Copy/link to this API element")

```python
standard_cauchy(self: Self, size = None, Self: type)
```

Defined in [`numpy.random.bitgen`](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/)

standard\_t[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/Generator/#class-generator-standard_t "Copy/link to this API element")

```python
standard_t(self: Self, df, size = None, Self: type)
```

Defined in [`numpy.random.bitgen`](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/)

vonmises[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/Generator/#class-generator-vonmises "Copy/link to this API element")

```python
vonmises(self: Self, mu, kappa, size = None, Self: type)
```

Defined in [`numpy.random.bitgen`](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/)

pareto[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/Generator/#class-generator-pareto "Copy/link to this API element")

```python
pareto(self: Self, a, size = None, Self: type)
```

Defined in [`numpy.random.bitgen`](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/)

weibull[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/Generator/#class-generator-weibull "Copy/link to this API element")

```python
weibull(self: Self, a, size = None, Self: type)
```

Defined in [`numpy.random.bitgen`](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/)

power[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/Generator/#class-generator-power "Copy/link to this API element")

```python
power(self: Self, a, size = None, Self: type)
```

Defined in [`numpy.random.bitgen`](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/)

laplace[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/Generator/#class-generator-laplace "Copy/link to this API element")

```python
laplace(self: Self, loc = 0.0, scale = 1.0, size = None, Self: type)
```

Defined in [`numpy.random.bitgen`](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/)

gumbel[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/Generator/#class-generator-gumbel "Copy/link to this API element")

```python
gumbel(self: Self, loc = 0.0, scale = 1.0, size = None, Self: type)
```

Defined in [`numpy.random.bitgen`](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/)

logistic[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/Generator/#class-generator-logistic "Copy/link to this API element")

```python
logistic(self: Self, loc = 0.0, scale = 1.0, size = None, Self: type)
```

Defined in [`numpy.random.bitgen`](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/)

lognormal[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/Generator/#class-generator-lognormal "Copy/link to this API element")

```python
lognormal(self: Self, mean = 0.0, sigma = 1.0, size = None, Self: type)
```

Defined in [`numpy.random.bitgen`](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/)

rayleigh[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/Generator/#class-generator-rayleigh "Copy/link to this API element")

```python
rayleigh(self: Self, scale = 1.0, size = None, Self: type)
```

Defined in [`numpy.random.bitgen`](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/)

wald[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/Generator/#class-generator-wald "Copy/link to this API element")

```python
wald(self: Self, mean, scale, size = None, Self: type)
```

Defined in [`numpy.random.bitgen`](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/)

triangular[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/Generator/#class-generator-triangular "Copy/link to this API element")

```python
triangular(self: Self, left, mode, right, size = None, Self: type)
```

Defined in [`numpy.random.bitgen`](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/)

binomial[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/Generator/#class-generator-binomial "Copy/link to this API element")

```python
binomial(self: Self, n, p, size = None, Self: type)
```

Defined in [`numpy.random.bitgen`](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/)

negative\_binomial[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/Generator/#class-generator-negative_binomial "Copy/link to this API element")

```python
negative_binomial(self: Self, n, p, size = None, Self: type)
```

Defined in [`numpy.random.bitgen`](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/)

poisson[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/Generator/#class-generator-poisson "Copy/link to this API element")

```python
poisson(self: Self, lam = 1.0, size = None, Self: type)
```

Defined in [`numpy.random.bitgen`](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/)

zipf[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/Generator/#class-generator-zipf "Copy/link to this API element")

```python
zipf(self: Self, a, size = None, Self: type)
```

Defined in [`numpy.random.bitgen`](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/)

geometric[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/Generator/#class-generator-geometric "Copy/link to this API element")

```python
geometric(self: Self, p, size = None, Self: type)
```

Defined in [`numpy.random.bitgen`](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/)

hypergeometric[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/Generator/#class-generator-hypergeometric "Copy/link to this API element")

```python
hypergeometric(self: Self, ngood, nbad, nsample, size = None, Self: type)
```

Defined in [`numpy.random.bitgen`](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/)

logseries[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/Generator/#class-generator-logseries "Copy/link to this API element")

```python
logseries(self: Self, p, size = None, Self: type)
```

Defined in [`numpy.random.bitgen`](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/)

multivariate\_normal[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/Generator/#class-generator-multivariate_normal "Copy/link to this API element")

```python
multivariate_normal(self: Self, mean, cov, size = None, check_valid: Literal[str] = "warn", tol: float = 1e-8, method: Literal[str] = "svd", Self: type)
```

Defined in [`numpy.random.bitgen`](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/)

multinomial[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/Generator/#class-generator-multinomial "Copy/link to this API element")

```python
multinomial(self: Self, n, pvals, size = None, Self: type)
```

Defined in [`numpy.random.bitgen`](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/)

multivariate\_hypergeometric[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/Generator/#class-generator-multivariate_hypergeometric "Copy/link to this API element")

```python
multivariate_hypergeometric(self: Self, colors, nsample: int, size = None, method: str = "marginals", Self: type)
```

Defined in [`numpy.random.bitgen`](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/)

dirichlet[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/Generator/#class-generator-dirichlet "Copy/link to this API element")

```python
dirichlet(self: Self, alpha, size = None, Self: type)
```

Defined in [`numpy.random.bitgen`](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/)

permuted[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/Generator/#class-generator-permuted "Copy/link to this API element")

```python
permuted(self: Self, x, axis = None, out = None, Self: type)
```

Defined in [`numpy.random.bitgen`](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/)

shuffle[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/Generator/#class-generator-shuffle "Copy/link to this API element")

```python
shuffle(self: Self, x, axis: int = 0, Self: type)
```

Defined in [`numpy.random.bitgen`](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/)

permutation[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/Generator/#class-generator-permutation "Copy/link to this API element")

```python
permutation(self: Self, x, axis: int = 0, Self: type)
```

Defined in [`numpy.random.bitgen`](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/)

send[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/Generator/#class-generator-send "Copy/link to this API element")

```python
send(self: Self, what: T, Self: type)
```

Added by [`internal.types.generator`](https://docs.exaloop.io/libraries/api/modules/internal/types/generator/)

Special methods 8

\_\_str\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/Generator/#class-generator-__str__ "Copy/link to this API element")

```python
__str__(self: Self, Self: type)
```

Defined in [`numpy.random.bitgen`](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/)

\_\_promise\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/Generator/#class-generator-__promise__ "Copy/link to this API element")

@\_\_internal\_\_

```python
__promise__(self: Self, Self: type)
```

Added by [`internal.types.generator`](https://docs.exaloop.io/libraries/api/modules/internal/types/generator/)

\_\_next\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/Generator/#class-generator-__next__ "Copy/link to this API element")

```python
__next__(self: Generator[T])
```

Added by [`internal.types.generator`](https://docs.exaloop.io/libraries/api/modules/internal/types/generator/)

\_\_iter\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/Generator/#class-generator-__iter__ "Copy/link to this API element")

```python
__iter__(self: Self, Self: type)
```

Added by [`internal.types.generator`](https://docs.exaloop.io/libraries/api/modules/internal/types/generator/)

\_\_raw\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/Generator/#class-generator-__raw__ "Copy/link to this API element")

@pure @derives @llvm

```python
__raw__(self: Self, Self: type)
```

Added by [`internal.types.generator`](https://docs.exaloop.io/libraries/api/modules/internal/types/generator/)

```llvm
ret ptr %self
```

\_\_done\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/Generator/#class-generator-__done__ "Copy/link to this API element")

@pure @llvm

```python
__done__(self: Self, Self: type)
```

Added by [`internal.types.generator`](https://docs.exaloop.io/libraries/api/modules/internal/types/generator/)

```llvm
declare i1 @llvm.coro.done(ptr nocapture readonly)
%0 = call i1 @llvm.coro.done(ptr %self)
%1 = zext i1 %0 to i8
ret i8 %1
```

\_\_resume\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/Generator/#class-generator-__resume__ "Copy/link to this API element")

@nocapture @llvm

```python
__resume__(self: Self, Self: type)
```

Added by [`internal.types.generator`](https://docs.exaloop.io/libraries/api/modules/internal/types/generator/)

```llvm
declare void @llvm.coro.resume(ptr)
call void @llvm.coro.resume(ptr %self)
ret {} {}
```

\_\_repr\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/Generator/#class-generator-__repr__ "Copy/link to this API element")

```python
__repr__(self: Self, Self: type)
```

Added by [`internal.types.generator`](https://docs.exaloop.io/libraries/api/modules/internal/types/generator/)

Source: https://docs.exaloop.io/libraries/api/classes/numpy/random/bitgen/Generator/
