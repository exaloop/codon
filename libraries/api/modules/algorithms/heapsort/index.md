# algorithms.heapsort

Standard library Module [View source](https://github.com/exaloop/codon/blob/master/stdlib/algorithms/heapsort.codon)

function heap\_sort\_array[](https://docs.exaloop.io/libraries/api/modules/algorithms/heapsort/#function-heap_sort_array "Copy/link to this API element")

```python
heap_sort_array[T: type, S: type](collection: Ptr[T], size: int, keyf: CallableTrait[([T], S)])
```

function heap\_sort\_inplace[](https://docs.exaloop.io/libraries/api/modules/algorithms/heapsort/#function-heap_sort_inplace "Copy/link to this API element")

```python
heap_sort_inplace[T: type, S: type](collection: List[T], keyf: CallableTrait[([T], S)])
```

function heap\_sort[](https://docs.exaloop.io/libraries/api/modules/algorithms/heapsort/#function-heap_sort "Copy/link to this API element")

```python
heap_sort[T: type, S: type](collection: List[T], keyf: CallableTrait[([T], S)]) -> List[T]
```

Source: https://docs.exaloop.io/libraries/api/modules/algorithms/heapsort/
