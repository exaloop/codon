# itertools

Standard library module [View source](https://github.com/exaloop/codon/blob/master/stdlib/itertools.codon)

function count[](https://docs.exaloop.io/libraries/api/modules/itertools/#function-count "Copy/link to this API element")

@inline

```python
count(start: T = 0, step: T = 1, T: type)
```

function cycle[](https://docs.exaloop.io/libraries/api/modules/itertools/#function-cycle "Copy/link to this API element")

@inline

```python
cycle(iterable: Generator[T], T: type)
```

function repeat[](https://docs.exaloop.io/libraries/api/modules/itertools/#function-repeat "Copy/link to this API element")

@inline

```python
repeat(object: T, times: Optional[int] = None, T: type)
```

function accumulate[](https://docs.exaloop.io/libraries/api/modules/itertools/#function-accumulate "Copy/link to this API element")

@inline

```python
accumulate(iterable: Generator[T], func = lambda a, b: (a + b), initial = 0, T: type)
```

@overload

```python
accumulate(iterable: Generator[T], func = lambda a, b: (a + b), T: type)
```

class chain[](https://docs.exaloop.io/libraries/api/modules/itertools/#class-chain "Copy/link to this API element")

@tuple

```python
class chain
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/itertools/chain/)

Constructors 1

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/itertools/#class-chain-__new__ "Copy/link to this API element")

@inline

```python
__new__(*iterables)
```

Methods 1

from\_iterable[](https://docs.exaloop.io/libraries/api/modules/itertools/#class-chain-from_iterable "Copy/link to this API element")

@inline

```python
from_iterable(iterables)
```

function compress[](https://docs.exaloop.io/libraries/api/modules/itertools/#function-compress "Copy/link to this API element")

@inline

```python
compress(data: Generator[T], selectors: Generator[B], T: type, B: type)
```

function dropwhile[](https://docs.exaloop.io/libraries/api/modules/itertools/#function-dropwhile "Copy/link to this API element")

@inline

```python
dropwhile(predicate: CallableTrait[T, bool], iterable: Generator[T], T: type)
```

function filterfalse[](https://docs.exaloop.io/libraries/api/modules/itertools/#function-filterfalse "Copy/link to this API element")

@inline

```python
filterfalse(predicate: CallableTrait[T, bool], iterable: Generator[T], T: type)
```

function groupby[](https://docs.exaloop.io/libraries/api/modules/itertools/#function-groupby "Copy/link to this API element")

@inline

```python
groupby(iterable, key = None)
```

function islice[](https://docs.exaloop.io/libraries/api/modules/itertools/#function-islice "Copy/link to this API element")

```python
islice(iterable: Generator[T], stop: Optional[int], T: type)
```

@overload

```python
islice(iterable: Generator[T], start: Optional[int], stop: Optional[int], step: Optional[int] = None, T: type)
```

function starmap[](https://docs.exaloop.io/libraries/api/modules/itertools/#function-starmap "Copy/link to this API element")

@inline

```python
starmap(function, iterable)
```

function takewhile[](https://docs.exaloop.io/libraries/api/modules/itertools/#function-takewhile "Copy/link to this API element")

@inline

```python
takewhile(predicate: CallableTrait[T, bool], iterable: Generator[T], T: type)
```

function tee[](https://docs.exaloop.io/libraries/api/modules/itertools/#function-tee "Copy/link to this API element")

```python
tee(iterable: Generator[T], n: int = 2, T: type)
```

function zip\_longest[](https://docs.exaloop.io/libraries/api/modules/itertools/#function-zip_longest "Copy/link to this API element")

@inline

```python
zip_longest(*iterables, fillvalue)
```

@overload

```python
zip_longest(*args)
```

function product[](https://docs.exaloop.io/libraries/api/modules/itertools/#function-product "Copy/link to this API element")

```python
product(*iterables, repeat: int)
```

@overload

```python
product(*iterables, repeat: Literal[int] = 1)
```

function combinations[](https://docs.exaloop.io/libraries/api/modules/itertools/#function-combinations "Copy/link to this API element")

```python
combinations(pool, r: int)
```

@overload

```python
combinations(pool, r: Literal[int])
```

function combinations\_with\_replacement[](https://docs.exaloop.io/libraries/api/modules/itertools/#function-combinations_with_replacement "Copy/link to this API element")

```python
combinations_with_replacement(pool, r: int)
```

@overload

```python
combinations_with_replacement(pool, r: Literal[int])
```

function permutations[](https://docs.exaloop.io/libraries/api/modules/itertools/#function-permutations "Copy/link to this API element")

```python
permutations(pool, r = None)
```

@overload

```python
permutations(pool, r: Literal[int])
```

Source: https://docs.exaloop.io/libraries/api/modules/itertools/
