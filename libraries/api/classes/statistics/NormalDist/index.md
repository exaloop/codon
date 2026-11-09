# NormalDist

Standard library class `statistics.NormalDist` [View source](https://github.com/exaloop/codon/blob/master/stdlib/statistics.codon)

class NormalDist[](https://docs.exaloop.io/libraries/api/classes/statistics/NormalDist/#class-normaldist "Copy/link to this API element")

```python
class NormalDist
```

Constructors 1

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/classes/statistics/NormalDist/#class-normaldist-__init__ "Copy/link to this API element")

```python
__init__(self: Self, mu = 0.0, sigma = 1.0, Self: type)
```

Defined in [`statistics`](https://docs.exaloop.io/libraries/api/modules/statistics/)

Properties 5

mean[](https://docs.exaloop.io/libraries/api/classes/statistics/NormalDist/#class-normaldist-mean "Copy/link to this API element")

@property

```python
mean(self: Self, Self: type)
```

Defined in [`statistics`](https://docs.exaloop.io/libraries/api/modules/statistics/)

median[](https://docs.exaloop.io/libraries/api/classes/statistics/NormalDist/#class-normaldist-median "Copy/link to this API element")

@property

```python
median(self: Self, Self: type)
```

Defined in [`statistics`](https://docs.exaloop.io/libraries/api/modules/statistics/)

mode[](https://docs.exaloop.io/libraries/api/classes/statistics/NormalDist/#class-normaldist-mode "Copy/link to this API element")

@property

```python
mode(self: Self, Self: type)
```

Defined in [`statistics`](https://docs.exaloop.io/libraries/api/modules/statistics/)

stdev[](https://docs.exaloop.io/libraries/api/classes/statistics/NormalDist/#class-normaldist-stdev "Copy/link to this API element")

@property

```python
stdev(self: Self, Self: type)
```

Defined in [`statistics`](https://docs.exaloop.io/libraries/api/modules/statistics/)

variance[](https://docs.exaloop.io/libraries/api/classes/statistics/NormalDist/#class-normaldist-variance "Copy/link to this API element")

@property

```python
variance(self: Self, Self: type)
```

Defined in [`statistics`](https://docs.exaloop.io/libraries/api/modules/statistics/)

Methods 8

pdf[](https://docs.exaloop.io/libraries/api/classes/statistics/NormalDist/#class-normaldist-pdf "Copy/link to this API element")

```python
pdf(self: Self, x, Self: type)
```

Defined in [`statistics`](https://docs.exaloop.io/libraries/api/modules/statistics/)

cdf[](https://docs.exaloop.io/libraries/api/classes/statistics/NormalDist/#class-normaldist-cdf "Copy/link to this API element")

```python
cdf(self: Self, x, Self: type)
```

Defined in [`statistics`](https://docs.exaloop.io/libraries/api/modules/statistics/)

zscore[](https://docs.exaloop.io/libraries/api/classes/statistics/NormalDist/#class-normaldist-zscore "Copy/link to this API element")

```python
zscore(self: Self, x, Self: type)
```

Defined in [`statistics`](https://docs.exaloop.io/libraries/api/modules/statistics/)

inv\_cdf[](https://docs.exaloop.io/libraries/api/classes/statistics/NormalDist/#class-normaldist-inv_cdf "Copy/link to this API element")

```python
inv_cdf(self: Self, p: float, Self: type)
```

Defined in [`statistics`](https://docs.exaloop.io/libraries/api/modules/statistics/)

quantiles[](https://docs.exaloop.io/libraries/api/classes/statistics/NormalDist/#class-normaldist-quantiles "Copy/link to this API element")

```python
quantiles(self: Self, n: int = 4, Self: type)
```

Defined in [`statistics`](https://docs.exaloop.io/libraries/api/modules/statistics/)

overlap[](https://docs.exaloop.io/libraries/api/classes/statistics/NormalDist/#class-normaldist-overlap "Copy/link to this API element")

```python
overlap(self: Self, other: NormalDist, Self: type)
```

Defined in [`statistics`](https://docs.exaloop.io/libraries/api/modules/statistics/)

samples[](https://docs.exaloop.io/libraries/api/classes/statistics/NormalDist/#class-normaldist-samples "Copy/link to this API element")

```python
samples(self: Self, n: int, seed: Optional[int] = None, Self: type)
```

Defined in [`statistics`](https://docs.exaloop.io/libraries/api/modules/statistics/)

from\_samples[](https://docs.exaloop.io/libraries/api/classes/statistics/NormalDist/#class-normaldist-from_samples "Copy/link to this API element")

```python
from_samples(data)
```

Defined in [`statistics`](https://docs.exaloop.io/libraries/api/modules/statistics/)

Special methods 13

\_\_add\_\_[](https://docs.exaloop.io/libraries/api/classes/statistics/NormalDist/#class-normaldist-__add__ "Copy/link to this API element")

```python
__add__(x1: NormalDist, x2: NormalDist)
```

Defined in [`statistics`](https://docs.exaloop.io/libraries/api/modules/statistics/)

```python
__add__(x1: NormalDist, x2: float)
```

Defined in [`statistics`](https://docs.exaloop.io/libraries/api/modules/statistics/)

\_\_sub\_\_[](https://docs.exaloop.io/libraries/api/classes/statistics/NormalDist/#class-normaldist-__sub__ "Copy/link to this API element")

```python
__sub__(x1: NormalDist, x2: NormalDist)
```

Defined in [`statistics`](https://docs.exaloop.io/libraries/api/modules/statistics/)

```python
__sub__(x1: NormalDist, x2: float)
```

Defined in [`statistics`](https://docs.exaloop.io/libraries/api/modules/statistics/)

\_\_mul\_\_[](https://docs.exaloop.io/libraries/api/classes/statistics/NormalDist/#class-normaldist-__mul__ "Copy/link to this API element")

```python
__mul__(x1: NormalDist, x2: float)
```

Defined in [`statistics`](https://docs.exaloop.io/libraries/api/modules/statistics/)

\_\_truediv\_\_[](https://docs.exaloop.io/libraries/api/classes/statistics/NormalDist/#class-normaldist-__truediv__ "Copy/link to this API element")

```python
__truediv__(x1: NormalDist, x2: float)
```

Defined in [`statistics`](https://docs.exaloop.io/libraries/api/modules/statistics/)

\_\_pos\_\_[](https://docs.exaloop.io/libraries/api/classes/statistics/NormalDist/#class-normaldist-__pos__ "Copy/link to this API element")

```python
__pos__(x1: NormalDist)
```

Defined in [`statistics`](https://docs.exaloop.io/libraries/api/modules/statistics/)

\_\_neg\_\_[](https://docs.exaloop.io/libraries/api/classes/statistics/NormalDist/#class-normaldist-__neg__ "Copy/link to this API element")

```python
__neg__(x1: NormalDist)
```

Defined in [`statistics`](https://docs.exaloop.io/libraries/api/modules/statistics/)

\_\_radd\_\_[](https://docs.exaloop.io/libraries/api/classes/statistics/NormalDist/#class-normaldist-__radd__ "Copy/link to this API element")

```python
__radd__(x1: NormalDist, x2: float)
```

Defined in [`statistics`](https://docs.exaloop.io/libraries/api/modules/statistics/)

\_\_rsub\_\_[](https://docs.exaloop.io/libraries/api/classes/statistics/NormalDist/#class-normaldist-__rsub__ "Copy/link to this API element")

```python
__rsub__(x1: NormalDist, x2: float)
```

Defined in [`statistics`](https://docs.exaloop.io/libraries/api/modules/statistics/)

\_\_rmul\_\_[](https://docs.exaloop.io/libraries/api/classes/statistics/NormalDist/#class-normaldist-__rmul__ "Copy/link to this API element")

```python
__rmul__(x1: NormalDist, x2: float)
```

Defined in [`statistics`](https://docs.exaloop.io/libraries/api/modules/statistics/)

\_\_eq\_\_[](https://docs.exaloop.io/libraries/api/classes/statistics/NormalDist/#class-normaldist-__eq__ "Copy/link to this API element")

```python
__eq__(x1: NormalDist, x2: NormalDist)
```

Defined in [`statistics`](https://docs.exaloop.io/libraries/api/modules/statistics/)

\_\_ne\_\_[](https://docs.exaloop.io/libraries/api/classes/statistics/NormalDist/#class-normaldist-__ne__ "Copy/link to this API element")

```python
__ne__(self: Self, other: NormalDist, Self: type)
```

Defined in [`statistics`](https://docs.exaloop.io/libraries/api/modules/statistics/)

\_\_hash\_\_[](https://docs.exaloop.io/libraries/api/classes/statistics/NormalDist/#class-normaldist-__hash__ "Copy/link to this API element")

```python
__hash__(self: Self, Self: type)
```

Defined in [`statistics`](https://docs.exaloop.io/libraries/api/modules/statistics/)

\_\_repr\_\_[](https://docs.exaloop.io/libraries/api/classes/statistics/NormalDist/#class-normaldist-__repr__ "Copy/link to this API element")

```python
__repr__(self: Self, Self: type)
```

Defined in [`statistics`](https://docs.exaloop.io/libraries/api/modules/statistics/)

Source: https://docs.exaloop.io/libraries/api/classes/statistics/NormalDist/
