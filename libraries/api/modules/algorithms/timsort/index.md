# algorithms.timsort

Standard library module [View source](https://github.com/exaloop/codon/blob/master/stdlib/algorithms/timsort.codon)

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
tim_sort_array(collection: Ptr[T], size: int, keyf: CallableTrait[T, S], T: type, S: type)
```

function tim\_sort\_inplace[](https://docs.exaloop.io/libraries/api/modules/algorithms/timsort/#function-tim_sort_inplace "Copy/link to this API element")

```python
tim_sort_inplace(collection: List[T], keyf: CallableTrait[T, S], T: type, S: type)
```

function tim\_sort[](https://docs.exaloop.io/libraries/api/modules/algorithms/timsort/#function-tim_sort "Copy/link to this API element")

```python
tim_sort(collection: List[T], keyf: CallableTrait[T, S], T: type, S: type)
```

Source: https://docs.exaloop.io/libraries/api/modules/algorithms/timsort/
