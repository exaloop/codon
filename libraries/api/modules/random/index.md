# random

Standard library module [View source](https://github.com/exaloop/codon/blob/master/stdlib/random.codon)

variable N[](https://docs.exaloop.io/libraries/api/modules/random/#variable-n "Copy/link to this API element")

```python
N = 624
```

variable M[](https://docs.exaloop.io/libraries/api/modules/random/#variable-m "Copy/link to this API element")

```python
M = 397
```

variable LOG4[](https://docs.exaloop.io/libraries/api/modules/random/#variable-log4 "Copy/link to this API element")

```python
LOG4 = _log(4.0)
```

variable NV\_MAGICCONST[](https://docs.exaloop.io/libraries/api/modules/random/#variable-nv_magicconst "Copy/link to this API element")

```python
NV_MAGICCONST = (4 * _exp(-0.5)).cdiv(_sqrt(2.0))
```

variable SG\_MAGICCONST[](https://docs.exaloop.io/libraries/api/modules/random/#variable-sg_magicconst "Copy/link to this API element")

```python
SG_MAGICCONST = (1.0 + _log(4.5))
```

variable TWOPI[](https://docs.exaloop.io/libraries/api/modules/random/#variable-twopi "Copy/link to this API element")

```python
TWOPI = (2.0 * _pi)
```

variable MATRIX\_A[](https://docs.exaloop.io/libraries/api/modules/random/#variable-matrix_a "Copy/link to this API element")

```python
MATRIX_A = u32(0x9908b0df)
```

variable UPPER\_MASK[](https://docs.exaloop.io/libraries/api/modules/random/#variable-upper_mask "Copy/link to this API element")

```python
UPPER_MASK = u32(0x80000000)
```

variable LOWER\_MASK[](https://docs.exaloop.io/libraries/api/modules/random/#variable-lower_mask "Copy/link to this API element")

```python
LOWER_MASK = u32(0x7fffffff)
```

class RandomGenerator[](https://docs.exaloop.io/libraries/api/modules/random/#class-randomgenerator "Copy/link to this API element")

@tuple

```python
class RandomGenerator
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/random/RandomGenerator/)

Fields 1

`data` `Ptr[u32]`

Constructors 1

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/random/#class-randomgenerator-__new__ "Copy/link to this API element")

```python
__new__()
```

Properties 2

index[](https://docs.exaloop.io/libraries/api/modules/random/#class-randomgenerator-index "Copy/link to this API element")

@property

```python
index(self: Self, Self: type)
```

state[](https://docs.exaloop.io/libraries/api/modules/random/#class-randomgenerator-state "Copy/link to this API element")

@property

```python
state(self: Self, Self: type)
```

Methods 10

getstate[](https://docs.exaloop.io/libraries/api/modules/random/#class-randomgenerator-getstate "Copy/link to this API element")

```python
getstate(self: Self, Self: type)
```

setstate[](https://docs.exaloop.io/libraries/api/modules/random/#class-randomgenerator-setstate "Copy/link to this API element")

```python
setstate(self: Self, state, Self: type)
```

genrand\_int32[](https://docs.exaloop.io/libraries/api/modules/random/#class-randomgenerator-genrand_int32 "Copy/link to this API element")

```python
genrand_int32(self: Self, Self: type)
```

genrand\_res53[](https://docs.exaloop.io/libraries/api/modules/random/#class-randomgenerator-genrand_res53 "Copy/link to this API element")

```python
genrand_res53(self: Self, Self: type)
```

random[](https://docs.exaloop.io/libraries/api/modules/random/#class-randomgenerator-random "Copy/link to this API element")

```python
random(self: Self, Self: type)
```

init\_u32[](https://docs.exaloop.io/libraries/api/modules/random/#class-randomgenerator-init_u32 "Copy/link to this API element")

```python
init_u32(self: Self, s: u32, Self: type)
```

init\_array[](https://docs.exaloop.io/libraries/api/modules/random/#class-randomgenerator-init_array "Copy/link to this API element")

```python
init_array(self: Self, init_key: Ptr[u32], key_length: int, Self: type)
```

init\_int[](https://docs.exaloop.io/libraries/api/modules/random/#class-randomgenerator-init_int "Copy/link to this API element")

```python
init_int(self: Self, s: int, Self: type)
```

random\_seed\_time\_pid[](https://docs.exaloop.io/libraries/api/modules/random/#class-randomgenerator-random_seed_time_pid "Copy/link to this API element")

```python
random_seed_time_pid(self: Self, Self: type)
```

seed[](https://docs.exaloop.io/libraries/api/modules/random/#class-randomgenerator-seed "Copy/link to this API element")

```python
seed(self: Self, s: int, Self: type)
```

```python
seed(self: Self, Self: type)
```

class Random[](https://docs.exaloop.io/libraries/api/modules/random/#class-random "Copy/link to this API element")

```python
class Random
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/random/Random/)

Fields 2

`gen` `RandomGenerator`

`gauss_next` `Optional[float]`

Constructors 1

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/random/#class-random-__init__ "Copy/link to this API element")

```python
__init__(self: Self, seed: Optional[int] = None, Self: type)
```

Methods 23

seed[](https://docs.exaloop.io/libraries/api/modules/random/#class-random-seed "Copy/link to this API element")

```python
seed(self: Self, a: Optional[int], Self: type)
```

getstate[](https://docs.exaloop.io/libraries/api/modules/random/#class-random-getstate "Copy/link to this API element")

```python
getstate(self: Self, Self: type)
```

setstate[](https://docs.exaloop.io/libraries/api/modules/random/#class-random-setstate "Copy/link to this API element")

```python
setstate(self: Self, state, Self: type)
```

getrandbits[](https://docs.exaloop.io/libraries/api/modules/random/#class-random-getrandbits "Copy/link to this API element")

```python
getrandbits(self: Self, k: int, Self: type)
```

bit\_length[](https://docs.exaloop.io/libraries/api/modules/random/#class-random-bit_length "Copy/link to this API element")

```python
bit_length(self: Self, n: int, Self: type)
```

randrange[](https://docs.exaloop.io/libraries/api/modules/random/#class-random-randrange "Copy/link to this API element")

```python
randrange(self: Self, start: int, stop: int, step: int = 1, Self: type)
```

randint[](https://docs.exaloop.io/libraries/api/modules/random/#class-random-randint "Copy/link to this API element")

```python
randint(self: Self, a: int, b: int, Self: type)
```

random[](https://docs.exaloop.io/libraries/api/modules/random/#class-random-random "Copy/link to this API element")

```python
random(self: Self, Self: type)
```

choice[](https://docs.exaloop.io/libraries/api/modules/random/#class-random-choice "Copy/link to this API element")

```python
choice(self: Self, sequence: Generator[T], T: type, Self: type)
```

@overload

```python
choice(self: Self, sequence: List[T], T: type, Self: type)
```

shuffle[](https://docs.exaloop.io/libraries/api/modules/random/#class-random-shuffle "Copy/link to this API element")

```python
shuffle(self: Self, x, Self: type)
```

uniform[](https://docs.exaloop.io/libraries/api/modules/random/#class-random-uniform "Copy/link to this API element")

```python
uniform(self: Self, a, b, Self: type)
```

triangular[](https://docs.exaloop.io/libraries/api/modules/random/#class-random-triangular "Copy/link to this API element")

```python
triangular(self: Self, low: float, high: float, mode: float, Self: type)
```

gammavariate[](https://docs.exaloop.io/libraries/api/modules/random/#class-random-gammavariate "Copy/link to this API element")

```python
gammavariate(self: Self, alpha: float, beta: float, Self: type)
```

betavariate[](https://docs.exaloop.io/libraries/api/modules/random/#class-random-betavariate "Copy/link to this API element")

```python
betavariate(self: Self, alpha: float, beta: float, Self: type)
```

expovariate[](https://docs.exaloop.io/libraries/api/modules/random/#class-random-expovariate "Copy/link to this API element")

```python
expovariate(self: Self, lambd: float, Self: type)
```

gauss[](https://docs.exaloop.io/libraries/api/modules/random/#class-random-gauss "Copy/link to this API element")

```python
gauss(self: Self, mu: float = 0.0, sigma: float = 1.0, Self: type)
```

paretovariate[](https://docs.exaloop.io/libraries/api/modules/random/#class-random-paretovariate "Copy/link to this API element")

```python
paretovariate(self: Self, alpha: float, Self: type)
```

weibullvariate[](https://docs.exaloop.io/libraries/api/modules/random/#class-random-weibullvariate "Copy/link to this API element")

```python
weibullvariate(self: Self, alpha: float, beta: float, Self: type)
```

normalvariate[](https://docs.exaloop.io/libraries/api/modules/random/#class-random-normalvariate "Copy/link to this API element")

```python
normalvariate(self: Self, mu: float = 0.0, sigma: float = 1.0, Self: type)
```

lognormvariate[](https://docs.exaloop.io/libraries/api/modules/random/#class-random-lognormvariate "Copy/link to this API element")

```python
lognormvariate(self: Self, mu: float, sigma: float, Self: type)
```

vonmisesvariate[](https://docs.exaloop.io/libraries/api/modules/random/#class-random-vonmisesvariate "Copy/link to this API element")

```python
vonmisesvariate(self: Self, mu: float, kappa: float, Self: type)
```

sample[](https://docs.exaloop.io/libraries/api/modules/random/#class-random-sample "Copy/link to this API element")

```python
sample(self: Self, population, k: int, Self: type)
```

choices[](https://docs.exaloop.io/libraries/api/modules/random/#class-random-choices "Copy/link to this API element")

```python
choices(self: Self, population, weights: Optional[List[T]], cum_weights: Optional[List[T]], k: int, T: type, Self: type)
```

function seed[](https://docs.exaloop.io/libraries/api/modules/random/#function-seed "Copy/link to this API element")

```python
seed(a: int)
```

function getrandbits[](https://docs.exaloop.io/libraries/api/modules/random/#function-getrandbits "Copy/link to this API element")

```python
getrandbits(k: int)
```

function randrange[](https://docs.exaloop.io/libraries/api/modules/random/#function-randrange "Copy/link to this API element")

```python
randrange(start: int, stop: Optional[int] = None, step: int = 1)
```

function randint[](https://docs.exaloop.io/libraries/api/modules/random/#function-randint "Copy/link to this API element")

```python
randint(a: int, b: int)
```

function choice[](https://docs.exaloop.io/libraries/api/modules/random/#function-choice "Copy/link to this API element")

```python
choice(s)
```

function choices[](https://docs.exaloop.io/libraries/api/modules/random/#function-choices "Copy/link to this API element")

```python
choices(population, weights: Optional[List[T]] = None, cum_weights: Optional[List[T]] = None, k: int = 1, T: type = int)
```

function shuffle[](https://docs.exaloop.io/libraries/api/modules/random/#function-shuffle "Copy/link to this API element")

```python
shuffle(s)
```

function sample[](https://docs.exaloop.io/libraries/api/modules/random/#function-sample "Copy/link to this API element")

```python
sample(population, k: int)
```

function random[](https://docs.exaloop.io/libraries/api/modules/random/#function-random "Copy/link to this API element")

```python
random()
```

function uniform[](https://docs.exaloop.io/libraries/api/modules/random/#function-uniform "Copy/link to this API element")

```python
uniform(a, b)
```

function triangular[](https://docs.exaloop.io/libraries/api/modules/random/#function-triangular "Copy/link to this API element")

```python
triangular(low: float = 0.0, high: float = 1.0, mode: Optional[float] = None)
```

function betavariate[](https://docs.exaloop.io/libraries/api/modules/random/#function-betavariate "Copy/link to this API element")

```python
betavariate(alpha: float, beta: float)
```

function expovariate[](https://docs.exaloop.io/libraries/api/modules/random/#function-expovariate "Copy/link to this API element")

```python
expovariate(lambd: float)
```

function gammavariate[](https://docs.exaloop.io/libraries/api/modules/random/#function-gammavariate "Copy/link to this API element")

```python
gammavariate(alpha: float, beta: float)
```

function gauss[](https://docs.exaloop.io/libraries/api/modules/random/#function-gauss "Copy/link to this API element")

```python
gauss(mu: float, sigma: float)
```

function lognormvariate[](https://docs.exaloop.io/libraries/api/modules/random/#function-lognormvariate "Copy/link to this API element")

```python
lognormvariate(mu: float, sigma: float)
```

function normalvariate[](https://docs.exaloop.io/libraries/api/modules/random/#function-normalvariate "Copy/link to this API element")

```python
normalvariate(mu: float, sigma: float)
```

function vonmisesvariate[](https://docs.exaloop.io/libraries/api/modules/random/#function-vonmisesvariate "Copy/link to this API element")

```python
vonmisesvariate(mu: float, kappa: float)
```

function paretovariate[](https://docs.exaloop.io/libraries/api/modules/random/#function-paretovariate "Copy/link to this API element")

```python
paretovariate(alpha: float)
```

function weibullvariate[](https://docs.exaloop.io/libraries/api/modules/random/#function-weibullvariate "Copy/link to this API element")

```python
weibullvariate(alpha: float, beta: float)
```

Source: https://docs.exaloop.io/libraries/api/modules/random/
