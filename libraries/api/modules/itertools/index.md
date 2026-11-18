# itertools

Standard library Module [View source](https://github.com/exaloop/codon/blob/master/stdlib/itertools.codon)

function count[](https://docs.exaloop.io/libraries/api/modules/itertools/#function-count "Copy/link to this API element")

@inline

```python
count[T: type](start: T = 0, step: T = 1) -> Generator[T]
```

function cycle[](https://docs.exaloop.io/libraries/api/modules/itertools/#function-cycle "Copy/link to this API element")

@inline

```python
cycle[T: type](iterable: Generator[T], /) -> Generator[T]
```

function repeat[](https://docs.exaloop.io/libraries/api/modules/itertools/#function-repeat "Copy/link to this API element")

@inline

```python
repeat[T: type](object: T, times: Optional[int] = None) -> Generator[T]
```

function accumulate[](https://docs.exaloop.io/libraries/api/modules/itertools/#function-accumulate "Copy/link to this API element")

@inline

```python
accumulate[T: type](iterable: Generator[T], func = lambda a, b: (a + b), *, initial = 0)
```

@overload

```python
accumulate[T: type](iterable: Generator[T], func = lambda a, b: (a + b))
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
from_iterable(iterables, /)
```

function compress[](https://docs.exaloop.io/libraries/api/modules/itertools/#function-compress "Copy/link to this API element")

@inline

```python
compress[T: type, B: type](data: Generator[T], selectors: Generator[B]) -> Generator[T]
```

function dropwhile[](https://docs.exaloop.io/libraries/api/modules/itertools/#function-dropwhile "Copy/link to this API element")

@inline

```python
dropwhile[T: type](predicate: CallableTrait[([T], bool)], iterable: Generator[T], /) -> Generator[T]
```

function filterfalse[](https://docs.exaloop.io/libraries/api/modules/itertools/#function-filterfalse "Copy/link to this API element")

@inline

```python
filterfalse[T: type](predicate: CallableTrait[([T], bool)], iterable: Generator[T], /) -> Generator[T]
```

function groupby[](https://docs.exaloop.io/libraries/api/modules/itertools/#function-groupby "Copy/link to this API element")

@inline

```python
groupby(iterable, key = None)
```

function islice[](https://docs.exaloop.io/libraries/api/modules/itertools/#function-islice "Copy/link to this API element")

```python
islice[T: type](iterable: Generator[T], stop: Optional[int], /) -> Generator[T]
```

@overload

```python
islice[T: type](iterable: Generator[T], start: Optional[int], stop: Optional[int], step: Optional[int] = None, /) -> Generator[T]
```

function starmap[](https://docs.exaloop.io/libraries/api/modules/itertools/#function-starmap "Copy/link to this API element")

@inline

```python
starmap(function, iterable, /)
```

function takewhile[](https://docs.exaloop.io/libraries/api/modules/itertools/#function-takewhile "Copy/link to this API element")

@inline

```python
takewhile[T: type](predicate: CallableTrait[([T], bool)], iterable: Generator[T], /) -> Generator[T]
```

function tee[](https://docs.exaloop.io/libraries/api/modules/itertools/#function-tee "Copy/link to this API element")

```python
tee[T: type](iterable: Generator[T], n: int = 2, /) -> List[Generator[T]]
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
product[repeat: Literal[int] = 1](*iterables)
```

function combinations[](https://docs.exaloop.io/libraries/api/modules/itertools/#function-combinations "Copy/link to this API element")

```python
combinations(pool, r: int)
```

@overload

```python
combinations[r: Literal[int]](pool)
```

function combinations\_with\_replacement[](https://docs.exaloop.io/libraries/api/modules/itertools/#function-combinations_with_replacement "Copy/link to this API element")

```python
combinations_with_replacement(pool, r: int)
```

@overload

```python
combinations_with_replacement[r: Literal[int]](pool)
```

function permutations[](https://docs.exaloop.io/libraries/api/modules/itertools/#function-permutations "Copy/link to this API element")

```python
permutations(pool, r = None) -> Generator
```

@overload

```python
permutations[r: Literal[int]](pool) -> Generator
```

Source: https://docs.exaloop.io/libraries/api/modules/itertools/
