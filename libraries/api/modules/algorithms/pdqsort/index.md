# algorithms.pdqsort

Standard library Module [View source](https://github.com/exaloop/codon/blob/master/stdlib/algorithms/pdqsort.codon)

variable INSERTION\_SORT\_THRESHOLD[](https://docs.exaloop.io/libraries/api/modules/algorithms/pdqsort/#variable-insertion_sort_threshold "Copy/link to this API element")

```python
INSERTION_SORT_THRESHOLD = 24
```

variable NINTHER\_THRESHOLD[](https://docs.exaloop.io/libraries/api/modules/algorithms/pdqsort/#variable-ninther_threshold "Copy/link to this API element")

```python
NINTHER_THRESHOLD = 128
```

variable PARTIAL\_INSERTION\_SORT\_LIMIT[](https://docs.exaloop.io/libraries/api/modules/algorithms/pdqsort/#variable-partial_insertion_sort_limit "Copy/link to this API element")

```python
PARTIAL_INSERTION_SORT_LIMIT = 8
```

function pdq\_sort\_array[](https://docs.exaloop.io/libraries/api/modules/algorithms/pdqsort/#function-pdq_sort_array "Copy/link to this API element")

```python
pdq_sort_array[T: type, S: type](collection: Ptr[T], size: int, keyf: CallableTrait[([T], S)])
```

function pdq\_sort\_inplace[](https://docs.exaloop.io/libraries/api/modules/algorithms/pdqsort/#function-pdq_sort_inplace "Copy/link to this API element")

```python
pdq_sort_inplace[T: type, S: type](collection: List[T], keyf: CallableTrait[([T], S)])
```

function pdq\_sort[](https://docs.exaloop.io/libraries/api/modules/algorithms/pdqsort/#function-pdq_sort "Copy/link to this API element")

```python
pdq_sort[T: type, S: type](collection: List[T], keyf: CallableTrait[([T], S)]) -> List[T]
```

Source: https://docs.exaloop.io/libraries/api/modules/algorithms/pdqsort/
