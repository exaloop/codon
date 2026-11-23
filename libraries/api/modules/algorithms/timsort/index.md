# algorithms.timsort

Standard library Module [View source](https://github.com/exaloop/codon/blob/master/stdlib/algorithms/timsort.codon)

variable BLOCK\_SIZE[](https://docs.exaloop.io/libraries/api/modules/algorithms/timsort/#variable-block_size "Copy/link to this API element")

```python
BLOCK_SIZE = 64
```

variable CACHELINE\_SIZE[](https://docs.exaloop.io/libraries/api/modules/algorithms/timsort/#variable-cacheline_size "Copy/link to this API element")

```python
CACHELINE_SIZE = 64
```

variable MIN\_GALLOP[](https://docs.exaloop.io/libraries/api/modules/algorithms/timsort/#variable-min_gallop "Copy/link to this API element")

```python
MIN_GALLOP = 7
```

function tim\_sort\_array[](https://docs.exaloop.io/libraries/api/modules/algorithms/timsort/#function-tim_sort_array "Copy/link to this API element")

```python
tim_sort_array[T: type, S: type](collection: Ptr[T], size: int, keyf: CallableTrait[([T], S)])
```

function tim\_sort\_inplace[](https://docs.exaloop.io/libraries/api/modules/algorithms/timsort/#function-tim_sort_inplace "Copy/link to this API element")

```python
tim_sort_inplace[T: type, S: type](collection: List[T], keyf: CallableTrait[([T], S)])
```

function tim\_sort[](https://docs.exaloop.io/libraries/api/modules/algorithms/timsort/#function-tim_sort "Copy/link to this API element")

```python
tim_sort[T: type, S: type](collection: List[T], keyf: CallableTrait[([T], S)]) -> List[T]
```

Source: https://docs.exaloop.io/libraries/api/modules/algorithms/timsort/
