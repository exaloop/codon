# statistics

Standard library module [View source](https://github.com/exaloop/codon/blob/master/stdlib/statistics.codon)

Descriptive statistics and distributions for native numeric data.

The API follows CPython 3.14, including weighted means, ranked correlation, regression, and kernel density estimation. Numeric aggregates return `float`; `median_low`, `median_high`, and modes preserve the input element type. Compensated floating-point arithmetic and widened integer intermediates avoid common cancellation and overflow errors, but do not provide Decimal/Fraction arithmetic. Random sampling accepts integer seeds. Remove NaNs before computing order statistics; native floats do not retain Python object identity for NaNs. Finite product overflow uses a fixed-point recovery accumulator. Supplied-center dispersion is scaled, and integer relational statistics use shifted origins; these can retain finite results and precision lost by CPython's float paths.

class StatisticsError[](https://docs.exaloop.io/libraries/api/modules/statistics/#class-statisticserror "Copy/link to this API element")

```python
class StatisticsError
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/statistics/StatisticsError/)

Constructors 1

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/statistics/#class-statisticserror-__init__ "Copy/link to this API element")

```python
__init__(self: Self, message: str = "", Self: type)
```

function median[](https://docs.exaloop.io/libraries/api/modules/statistics/#function-median "Copy/link to this API element")

```python
median(data)
```

function median\_low[](https://docs.exaloop.io/libraries/api/modules/statistics/#function-median_low "Copy/link to this API element")

```python
median_low(data)
```

function median\_high[](https://docs.exaloop.io/libraries/api/modules/statistics/#function-median_high "Copy/link to this API element")

```python
median_high(data)
```

function median\_grouped[](https://docs.exaloop.io/libraries/api/modules/statistics/#function-median_grouped "Copy/link to this API element")

```python
median_grouped(data, interval = 1.0)
```

function mode[](https://docs.exaloop.io/libraries/api/modules/statistics/#function-mode "Copy/link to this API element")

```python
mode(data)
```

function multimode[](https://docs.exaloop.io/libraries/api/modules/statistics/#function-multimode "Copy/link to this API element")

```python
multimode(data)
```

function quantiles[](https://docs.exaloop.io/libraries/api/modules/statistics/#function-quantiles "Copy/link to this API element")

```python
quantiles(data, n: int = 4, method: str = "exclusive")
```

function mean[](https://docs.exaloop.io/libraries/api/modules/statistics/#function-mean "Copy/link to this API element")

```python
mean(data)
```

function fmean[](https://docs.exaloop.io/libraries/api/modules/statistics/#function-fmean "Copy/link to this API element")

```python
fmean(data, weights = None)
```

function geometric\_mean[](https://docs.exaloop.io/libraries/api/modules/statistics/#function-geometric_mean "Copy/link to this API element")

```python
geometric_mean(data)
```

function harmonic\_mean[](https://docs.exaloop.io/libraries/api/modules/statistics/#function-harmonic_mean "Copy/link to this API element")

```python
harmonic_mean(data, weights = None)
```

function pvariance[](https://docs.exaloop.io/libraries/api/modules/statistics/#function-pvariance "Copy/link to this API element")

```python
pvariance(data, mu = None)
```

function pstdev[](https://docs.exaloop.io/libraries/api/modules/statistics/#function-pstdev "Copy/link to this API element")

```python
pstdev(data, mu = None)
```

function variance[](https://docs.exaloop.io/libraries/api/modules/statistics/#function-variance "Copy/link to this API element")

```python
variance(data, xbar = None)
```

function stdev[](https://docs.exaloop.io/libraries/api/modules/statistics/#function-stdev "Copy/link to this API element")

```python
stdev(data, xbar = None)
```

function covariance[](https://docs.exaloop.io/libraries/api/modules/statistics/#function-covariance "Copy/link to this API element")

```python
covariance(xdata, ydata)
```

function correlation[](https://docs.exaloop.io/libraries/api/modules/statistics/#function-correlation "Copy/link to this API element")

```python
correlation(xdata, ydata, method: str = "linear")
```

class LinearRegression[](https://docs.exaloop.io/libraries/api/modules/statistics/#class-linearregression "Copy/link to this API element")

@tuple

```python
class LinearRegression
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/statistics/LinearRegression/)

Fields 2

`slope` `float`

`intercept` `float`

function linear\_regression[](https://docs.exaloop.io/libraries/api/modules/statistics/#function-linear_regression "Copy/link to this API element")

```python
linear_regression(xdata, ydata, proportional: bool = False)
```

function kde[](https://docs.exaloop.io/libraries/api/modules/statistics/#function-kde "Copy/link to this API element")

```python
kde(data, h, kernel: str = "normal", cumulative: bool = False)
```

Return a kernel density or cumulative distribution estimator.

function kde\_random[](https://docs.exaloop.io/libraries/api/modules/statistics/#function-kde_random "Copy/link to this API element")

```python
kde_random(data, h, kernel: str = "normal", seed: Optional[int] = None)
```

Return a sampler with a private random generator and optional integer seed.

class NormalDist[](https://docs.exaloop.io/libraries/api/modules/statistics/#class-normaldist "Copy/link to this API element")

```python
class NormalDist
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/statistics/NormalDist/)

Constructors 1

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/statistics/#class-normaldist-__init__ "Copy/link to this API element")

```python
__init__(self: Self, mu = 0.0, sigma = 1.0, Self: type)
```

Properties 5

mean[](https://docs.exaloop.io/libraries/api/modules/statistics/#class-normaldist-mean "Copy/link to this API element")

@property

```python
mean(self: Self, Self: type)
```

median[](https://docs.exaloop.io/libraries/api/modules/statistics/#class-normaldist-median "Copy/link to this API element")

@property

```python
median(self: Self, Self: type)
```

mode[](https://docs.exaloop.io/libraries/api/modules/statistics/#class-normaldist-mode "Copy/link to this API element")

@property

```python
mode(self: Self, Self: type)
```

stdev[](https://docs.exaloop.io/libraries/api/modules/statistics/#class-normaldist-stdev "Copy/link to this API element")

@property

```python
stdev(self: Self, Self: type)
```

variance[](https://docs.exaloop.io/libraries/api/modules/statistics/#class-normaldist-variance "Copy/link to this API element")

@property

```python
variance(self: Self, Self: type)
```

Methods 8

pdf[](https://docs.exaloop.io/libraries/api/modules/statistics/#class-normaldist-pdf "Copy/link to this API element")

```python
pdf(self: Self, x, Self: type)
```

cdf[](https://docs.exaloop.io/libraries/api/modules/statistics/#class-normaldist-cdf "Copy/link to this API element")

```python
cdf(self: Self, x, Self: type)
```

zscore[](https://docs.exaloop.io/libraries/api/modules/statistics/#class-normaldist-zscore "Copy/link to this API element")

```python
zscore(self: Self, x, Self: type)
```

inv\_cdf[](https://docs.exaloop.io/libraries/api/modules/statistics/#class-normaldist-inv_cdf "Copy/link to this API element")

```python
inv_cdf(self: Self, p: float, Self: type)
```

quantiles[](https://docs.exaloop.io/libraries/api/modules/statistics/#class-normaldist-quantiles "Copy/link to this API element")

```python
quantiles(self: Self, n: int = 4, Self: type)
```

overlap[](https://docs.exaloop.io/libraries/api/modules/statistics/#class-normaldist-overlap "Copy/link to this API element")

```python
overlap(self: Self, other: NormalDist, Self: type)
```

samples[](https://docs.exaloop.io/libraries/api/modules/statistics/#class-normaldist-samples "Copy/link to this API element")

```python
samples(self: Self, n: int, seed: Optional[int] = None, Self: type)
```

from\_samples[](https://docs.exaloop.io/libraries/api/modules/statistics/#class-normaldist-from_samples "Copy/link to this API element")

```python
from_samples(data)
```

Special methods 13

\_\_add\_\_[](https://docs.exaloop.io/libraries/api/modules/statistics/#class-normaldist-__add__ "Copy/link to this API element")

```python
__add__(x1: NormalDist, x2: NormalDist)
```

```python
__add__(x1: NormalDist, x2: float)
```

\_\_sub\_\_[](https://docs.exaloop.io/libraries/api/modules/statistics/#class-normaldist-__sub__ "Copy/link to this API element")

```python
__sub__(x1: NormalDist, x2: NormalDist)
```

```python
__sub__(x1: NormalDist, x2: float)
```

\_\_mul\_\_[](https://docs.exaloop.io/libraries/api/modules/statistics/#class-normaldist-__mul__ "Copy/link to this API element")

```python
__mul__(x1: NormalDist, x2: float)
```

\_\_truediv\_\_[](https://docs.exaloop.io/libraries/api/modules/statistics/#class-normaldist-__truediv__ "Copy/link to this API element")

```python
__truediv__(x1: NormalDist, x2: float)
```

\_\_pos\_\_[](https://docs.exaloop.io/libraries/api/modules/statistics/#class-normaldist-__pos__ "Copy/link to this API element")

```python
__pos__(x1: NormalDist)
```

\_\_neg\_\_[](https://docs.exaloop.io/libraries/api/modules/statistics/#class-normaldist-__neg__ "Copy/link to this API element")

```python
__neg__(x1: NormalDist)
```

\_\_radd\_\_[](https://docs.exaloop.io/libraries/api/modules/statistics/#class-normaldist-__radd__ "Copy/link to this API element")

```python
__radd__(x1: NormalDist, x2: float)
```

\_\_rsub\_\_[](https://docs.exaloop.io/libraries/api/modules/statistics/#class-normaldist-__rsub__ "Copy/link to this API element")

```python
__rsub__(x1: NormalDist, x2: float)
```

\_\_rmul\_\_[](https://docs.exaloop.io/libraries/api/modules/statistics/#class-normaldist-__rmul__ "Copy/link to this API element")

```python
__rmul__(x1: NormalDist, x2: float)
```

\_\_eq\_\_[](https://docs.exaloop.io/libraries/api/modules/statistics/#class-normaldist-__eq__ "Copy/link to this API element")

```python
__eq__(x1: NormalDist, x2: NormalDist)
```

\_\_ne\_\_[](https://docs.exaloop.io/libraries/api/modules/statistics/#class-normaldist-__ne__ "Copy/link to this API element")

```python
__ne__(self: Self, other: NormalDist, Self: type)
```

\_\_hash\_\_[](https://docs.exaloop.io/libraries/api/modules/statistics/#class-normaldist-__hash__ "Copy/link to this API element")

```python
__hash__(self: Self, Self: type)
```

\_\_repr\_\_[](https://docs.exaloop.io/libraries/api/modules/statistics/#class-normaldist-__repr__ "Copy/link to this API element")

```python
__repr__(self: Self, Self: type)
```

Source: https://docs.exaloop.io/libraries/api/modules/statistics/
