# numpy.random

Standard library package [View source](https://github.com/exaloop/codon/blob/master/stdlib/numpy/random/__init__.codon)

function default\_rng[](https://docs.exaloop.io/libraries/api/modules/numpy/random/#function-default_rng "Copy/link to this API element")

```python
default_rng(seed = None)
```

function beta[](https://docs.exaloop.io/libraries/api/modules/numpy/random/#function-beta "Copy/link to this API element")

```python
beta(a, b, size = None)
```

function binomial[](https://docs.exaloop.io/libraries/api/modules/numpy/random/#function-binomial "Copy/link to this API element")

```python
binomial(n, p, size = None)
```

function bytes[](https://docs.exaloop.io/libraries/api/modules/numpy/random/#function-bytes "Copy/link to this API element")

```python
bytes(length: int)
```

function chisquare[](https://docs.exaloop.io/libraries/api/modules/numpy/random/#function-chisquare "Copy/link to this API element")

```python
chisquare(df, size = None)
```

function choice[](https://docs.exaloop.io/libraries/api/modules/numpy/random/#function-choice "Copy/link to this API element")

```python
choice(a, size = None, replace: bool = True, p = None)
```

function dirichlet[](https://docs.exaloop.io/libraries/api/modules/numpy/random/#function-dirichlet "Copy/link to this API element")

```python
dirichlet(alpha, size = None)
```

function exponential[](https://docs.exaloop.io/libraries/api/modules/numpy/random/#function-exponential "Copy/link to this API element")

```python
exponential(scale = 1.0, size = None)
```

function f[](https://docs.exaloop.io/libraries/api/modules/numpy/random/#function-f "Copy/link to this API element")

```python
f(dfnum, dfden, size = None)
```

function gamma[](https://docs.exaloop.io/libraries/api/modules/numpy/random/#function-gamma "Copy/link to this API element")

```python
gamma(shape, scale = 1.0, size = None)
```

function geometric[](https://docs.exaloop.io/libraries/api/modules/numpy/random/#function-geometric "Copy/link to this API element")

```python
geometric(p, size = None)
```

function get\_state[](https://docs.exaloop.io/libraries/api/modules/numpy/random/#function-get_state "Copy/link to this API element")

```python
get_state(legacy: bool)
```

function gumbel[](https://docs.exaloop.io/libraries/api/modules/numpy/random/#function-gumbel "Copy/link to this API element")

```python
gumbel(loc = 0.0, scale = 1.0, size = None)
```

function hypergeometric[](https://docs.exaloop.io/libraries/api/modules/numpy/random/#function-hypergeometric "Copy/link to this API element")

```python
hypergeometric(ngood, nbad, nsample, size = None)
```

function laplace[](https://docs.exaloop.io/libraries/api/modules/numpy/random/#function-laplace "Copy/link to this API element")

```python
laplace(loc = 0.0, scale = 1.0, size = None)
```

function logistic[](https://docs.exaloop.io/libraries/api/modules/numpy/random/#function-logistic "Copy/link to this API element")

```python
logistic(loc = 0.0, scale = 1.0, size = None)
```

function lognormal[](https://docs.exaloop.io/libraries/api/modules/numpy/random/#function-lognormal "Copy/link to this API element")

```python
lognormal(mean = 0.0, sigma = 1.0, size = None)
```

function logseries[](https://docs.exaloop.io/libraries/api/modules/numpy/random/#function-logseries "Copy/link to this API element")

```python
logseries(p, size = None)
```

function multinomial[](https://docs.exaloop.io/libraries/api/modules/numpy/random/#function-multinomial "Copy/link to this API element")

```python
multinomial(n, pvals, size = None)
```

function multivariate\_normal[](https://docs.exaloop.io/libraries/api/modules/numpy/random/#function-multivariate_normal "Copy/link to this API element")

```python
multivariate_normal(mean, cov, size = None, check_valid: Literal[str] = "warn", tol: float = 1e-8)
```

function negative\_binomial[](https://docs.exaloop.io/libraries/api/modules/numpy/random/#function-negative_binomial "Copy/link to this API element")

```python
negative_binomial(n, p, size = None)
```

function noncentral\_chisquare[](https://docs.exaloop.io/libraries/api/modules/numpy/random/#function-noncentral_chisquare "Copy/link to this API element")

```python
noncentral_chisquare(df, nonc, size = None)
```

function noncentral\_f[](https://docs.exaloop.io/libraries/api/modules/numpy/random/#function-noncentral_f "Copy/link to this API element")

```python
noncentral_f(dfnum, dfden, nonc, size = None)
```

function normal[](https://docs.exaloop.io/libraries/api/modules/numpy/random/#function-normal "Copy/link to this API element")

```python
normal(loc = 0.0, scale = 1.0, size = None)
```

function pareto[](https://docs.exaloop.io/libraries/api/modules/numpy/random/#function-pareto "Copy/link to this API element")

```python
pareto(a, size = None)
```

function permutation[](https://docs.exaloop.io/libraries/api/modules/numpy/random/#function-permutation "Copy/link to this API element")

```python
permutation(x)
```

function poisson[](https://docs.exaloop.io/libraries/api/modules/numpy/random/#function-poisson "Copy/link to this API element")

```python
poisson(lam = 1.0, size = None)
```

function power[](https://docs.exaloop.io/libraries/api/modules/numpy/random/#function-power "Copy/link to this API element")

```python
power(a, size = None)
```

function rand[](https://docs.exaloop.io/libraries/api/modules/numpy/random/#function-rand "Copy/link to this API element")

```python
rand(*d)
```

function randint[](https://docs.exaloop.io/libraries/api/modules/numpy/random/#function-randint "Copy/link to this API element")

```python
randint(low, high = None, size = None, dtype: type = int)
```

function randn[](https://docs.exaloop.io/libraries/api/modules/numpy/random/#function-randn "Copy/link to this API element")

```python
randn(*d)
```

function random[](https://docs.exaloop.io/libraries/api/modules/numpy/random/#function-random "Copy/link to this API element")

```python
random(size = None)
```

function random\_integers[](https://docs.exaloop.io/libraries/api/modules/numpy/random/#function-random_integers "Copy/link to this API element")

```python
random_integers(low, high = None, size = None)
```

function random\_sample[](https://docs.exaloop.io/libraries/api/modules/numpy/random/#function-random_sample "Copy/link to this API element")

```python
random_sample(size = None)
```

function ranf[](https://docs.exaloop.io/libraries/api/modules/numpy/random/#function-ranf "Copy/link to this API element")

```python
ranf(size = None)
```

function rayleigh[](https://docs.exaloop.io/libraries/api/modules/numpy/random/#function-rayleigh "Copy/link to this API element")

```python
rayleigh(scale = 1.0, size = None)
```

function sample[](https://docs.exaloop.io/libraries/api/modules/numpy/random/#function-sample "Copy/link to this API element")

```python
sample(size = None)
```

function seed[](https://docs.exaloop.io/libraries/api/modules/numpy/random/#function-seed "Copy/link to this API element")

```python
seed(seed = None)
```

function set\_state[](https://docs.exaloop.io/libraries/api/modules/numpy/random/#function-set_state "Copy/link to this API element")

```python
set_state(state)
```

function shuffle[](https://docs.exaloop.io/libraries/api/modules/numpy/random/#function-shuffle "Copy/link to this API element")

```python
shuffle(x)
```

function standard\_cauchy[](https://docs.exaloop.io/libraries/api/modules/numpy/random/#function-standard_cauchy "Copy/link to this API element")

```python
standard_cauchy(size = None)
```

function standard\_exponential[](https://docs.exaloop.io/libraries/api/modules/numpy/random/#function-standard_exponential "Copy/link to this API element")

```python
standard_exponential(size = None)
```

function standard\_gamma[](https://docs.exaloop.io/libraries/api/modules/numpy/random/#function-standard_gamma "Copy/link to this API element")

```python
standard_gamma(shape, size = None)
```

function standard\_normal[](https://docs.exaloop.io/libraries/api/modules/numpy/random/#function-standard_normal "Copy/link to this API element")

```python
standard_normal(size = None)
```

function standard\_t[](https://docs.exaloop.io/libraries/api/modules/numpy/random/#function-standard_t "Copy/link to this API element")

```python
standard_t(df, size = None)
```

function triangular[](https://docs.exaloop.io/libraries/api/modules/numpy/random/#function-triangular "Copy/link to this API element")

```python
triangular(left, mode, right, size = None)
```

function uniform[](https://docs.exaloop.io/libraries/api/modules/numpy/random/#function-uniform "Copy/link to this API element")

```python
uniform(low = 0.0, high = 1.0, size = None)
```

function vonmises[](https://docs.exaloop.io/libraries/api/modules/numpy/random/#function-vonmises "Copy/link to this API element")

```python
vonmises(mu, kappa, size = None)
```

function wald[](https://docs.exaloop.io/libraries/api/modules/numpy/random/#function-wald "Copy/link to this API element")

```python
wald(mean, scale, size = None)
```

function weibull[](https://docs.exaloop.io/libraries/api/modules/numpy/random/#function-weibull "Copy/link to this API element")

```python
weibull(a, size = None)
```

function zipf[](https://docs.exaloop.io/libraries/api/modules/numpy/random/#function-zipf "Copy/link to this API element")

```python
zipf(a, size = None)
```

Modules 9

[`bitgen` →](https://docs.exaloop.io/libraries/api/modules/numpy/random/bitgen/) [`logfactorial` →](https://docs.exaloop.io/libraries/api/modules/numpy/random/logfactorial/) [`mt19937` →](https://docs.exaloop.io/libraries/api/modules/numpy/random/mt19937/) [`pcg64` →](https://docs.exaloop.io/libraries/api/modules/numpy/random/pcg64/) [`philox` →](https://docs.exaloop.io/libraries/api/modules/numpy/random/philox/) [`seed` →](https://docs.exaloop.io/libraries/api/modules/numpy/random/seed/) [`sfc64` →](https://docs.exaloop.io/libraries/api/modules/numpy/random/sfc64/) [`splitmix64` →](https://docs.exaloop.io/libraries/api/modules/numpy/random/splitmix64/) [`ziggurat` →](https://docs.exaloop.io/libraries/api/modules/numpy/random/ziggurat/)

Source: https://docs.exaloop.io/libraries/api/modules/numpy/random/
