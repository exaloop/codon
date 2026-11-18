# Random

Standard library class `random.Random` [View source](https://github.com/exaloop/codon/blob/master/stdlib/random.codon#L141)

class Random[](https://docs.exaloop.io/libraries/api/classes/random/Random/#class-random "Copy/link to this API element")

```python
class Random
```

Fields 2

`gen` `RandomGenerator`

`gauss_next` `Optional[float]`

Constructors 1

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/classes/random/Random/#class-random-__init__ "Copy/link to this API element")

```python
__init__(self: Self, seed: Optional[int] = None)
```

Defined in [`random`](https://docs.exaloop.io/libraries/api/modules/random/)

Methods 23

seed[](https://docs.exaloop.io/libraries/api/classes/random/Random/#class-random-seed "Copy/link to this API element")

```python
seed(self: Self, a: Optional[int])
```

Defined in [`random`](https://docs.exaloop.io/libraries/api/modules/random/)

getstate[](https://docs.exaloop.io/libraries/api/classes/random/Random/#class-random-getstate "Copy/link to this API element")

```python
getstate(self: Self)
```

Defined in [`random`](https://docs.exaloop.io/libraries/api/modules/random/)

setstate[](https://docs.exaloop.io/libraries/api/classes/random/Random/#class-random-setstate "Copy/link to this API element")

```python
setstate(self: Self, state)
```

Defined in [`random`](https://docs.exaloop.io/libraries/api/modules/random/)

getrandbits[](https://docs.exaloop.io/libraries/api/classes/random/Random/#class-random-getrandbits "Copy/link to this API element")

```python
getrandbits(self: Self, k: int, /) -> int
```

Defined in [`random`](https://docs.exaloop.io/libraries/api/modules/random/)

bit\_length[](https://docs.exaloop.io/libraries/api/classes/random/Random/#class-random-bit_length "Copy/link to this API element")

```python
bit_length(self: Self, n: int) -> int
```

Defined in [`random`](https://docs.exaloop.io/libraries/api/modules/random/)

randrange[](https://docs.exaloop.io/libraries/api/classes/random/Random/#class-random-randrange "Copy/link to this API element")

```python
randrange(self: Self, start: int, stop: int, step: int = 1) -> int
```

Defined in [`random`](https://docs.exaloop.io/libraries/api/modules/random/)

randint[](https://docs.exaloop.io/libraries/api/classes/random/Random/#class-random-randint "Copy/link to this API element")

```python
randint(self: Self, a: int, b: int)
```

Defined in [`random`](https://docs.exaloop.io/libraries/api/modules/random/)

random[](https://docs.exaloop.io/libraries/api/classes/random/Random/#class-random-random "Copy/link to this API element")

```python
random(self: Self, /) -> float
```

Defined in [`random`](https://docs.exaloop.io/libraries/api/modules/random/)

choice[](https://docs.exaloop.io/libraries/api/classes/random/Random/#class-random-choice "Copy/link to this API element")

```python
choice[T: type](self: Self, sequence: Generator[T]) -> T
```

Defined in [`random`](https://docs.exaloop.io/libraries/api/modules/random/)

@overload

```python
choice[T: type](self: Self, sequence: List[T]) -> T
```

Defined in [`random`](https://docs.exaloop.io/libraries/api/modules/random/)

shuffle[](https://docs.exaloop.io/libraries/api/classes/random/Random/#class-random-shuffle "Copy/link to this API element")

```python
shuffle(self: Self, x)
```

Defined in [`random`](https://docs.exaloop.io/libraries/api/modules/random/)

uniform[](https://docs.exaloop.io/libraries/api/classes/random/Random/#class-random-uniform "Copy/link to this API element")

```python
uniform(self: Self, a, b) -> float
```

Defined in [`random`](https://docs.exaloop.io/libraries/api/modules/random/)

triangular[](https://docs.exaloop.io/libraries/api/classes/random/Random/#class-random-triangular "Copy/link to this API element")

```python
triangular(self: Self, low: float, high: float, mode: float) -> float
```

Defined in [`random`](https://docs.exaloop.io/libraries/api/modules/random/)

gammavariate[](https://docs.exaloop.io/libraries/api/classes/random/Random/#class-random-gammavariate "Copy/link to this API element")

```python
gammavariate(self: Self, alpha: float, beta: float) -> float
```

Defined in [`random`](https://docs.exaloop.io/libraries/api/modules/random/)

betavariate[](https://docs.exaloop.io/libraries/api/classes/random/Random/#class-random-betavariate "Copy/link to this API element")

```python
betavariate(self: Self, alpha: float, beta: float) -> float
```

Defined in [`random`](https://docs.exaloop.io/libraries/api/modules/random/)

expovariate[](https://docs.exaloop.io/libraries/api/classes/random/Random/#class-random-expovariate "Copy/link to this API element")

```python
expovariate(self: Self, lambd: float) -> float
```

Defined in [`random`](https://docs.exaloop.io/libraries/api/modules/random/)

gauss[](https://docs.exaloop.io/libraries/api/classes/random/Random/#class-random-gauss "Copy/link to this API element")

```python
gauss(self: Self, mu: float = 0.0, sigma: float = 1.0) -> float
```

Defined in [`random`](https://docs.exaloop.io/libraries/api/modules/random/)

paretovariate[](https://docs.exaloop.io/libraries/api/classes/random/Random/#class-random-paretovariate "Copy/link to this API element")

```python
paretovariate(self: Self, alpha: float) -> float
```

Defined in [`random`](https://docs.exaloop.io/libraries/api/modules/random/)

weibullvariate[](https://docs.exaloop.io/libraries/api/classes/random/Random/#class-random-weibullvariate "Copy/link to this API element")

```python
weibullvariate(self: Self, alpha: float, beta: float) -> float
```

Defined in [`random`](https://docs.exaloop.io/libraries/api/modules/random/)

normalvariate[](https://docs.exaloop.io/libraries/api/classes/random/Random/#class-random-normalvariate "Copy/link to this API element")

```python
normalvariate(self: Self, mu: float = 0.0, sigma: float = 1.0) -> float
```

Defined in [`random`](https://docs.exaloop.io/libraries/api/modules/random/)

lognormvariate[](https://docs.exaloop.io/libraries/api/classes/random/Random/#class-random-lognormvariate "Copy/link to this API element")

```python
lognormvariate(self: Self, mu: float, sigma: float) -> float
```

Defined in [`random`](https://docs.exaloop.io/libraries/api/modules/random/)

vonmisesvariate[](https://docs.exaloop.io/libraries/api/classes/random/Random/#class-random-vonmisesvariate "Copy/link to this API element")

```python
vonmisesvariate(self: Self, mu: float, kappa: float) -> float
```

Defined in [`random`](https://docs.exaloop.io/libraries/api/modules/random/)

sample[](https://docs.exaloop.io/libraries/api/classes/random/Random/#class-random-sample "Copy/link to this API element")

```python
sample(self: Self, population, k: int)
```

Defined in [`random`](https://docs.exaloop.io/libraries/api/modules/random/)

choices[](https://docs.exaloop.io/libraries/api/classes/random/Random/#class-random-choices "Copy/link to this API element")

```python
choices[T: type](self: Self, population, weights: Optional[List[T]], *, cum_weights: Optional[List[T]], k: int)
```

Defined in [`random`](https://docs.exaloop.io/libraries/api/modules/random/)

Source: https://docs.exaloop.io/libraries/api/classes/random/Random/
