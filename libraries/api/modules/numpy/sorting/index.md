# numpy.sorting

Standard library module [View source](https://github.com/exaloop/codon/blob/master/stdlib/numpy/sorting.codon)

function insertionsort[](https://docs.exaloop.io/libraries/api/modules/numpy/sorting/#function-insertionsort "Copy/link to this API element")

```python
insertionsort(start: Ptr[T], n: int, T: type)
```

function ainsertionsort[](https://docs.exaloop.io/libraries/api/modules/numpy/sorting/#function-ainsertionsort "Copy/link to this API element")

```python
ainsertionsort(v: Ptr[T], tosort: Ptr[int], n: int, T: type)
```

function heapsort[](https://docs.exaloop.io/libraries/api/modules/numpy/sorting/#function-heapsort "Copy/link to this API element")

```python
heapsort(start: Ptr[T], n: int, T: type)
```

function aheapsort[](https://docs.exaloop.io/libraries/api/modules/numpy/sorting/#function-aheapsort "Copy/link to this API element")

```python
aheapsort(vv: Ptr[T], tosort: Ptr[int], n: int, T: type)
```

variable PYA\_QS\_STACK[](https://docs.exaloop.io/libraries/api/modules/numpy/sorting/#variable-pya_qs_stack "Copy/link to this API element")

```python
PYA_QS_STACK: Literal[int] = 100
```

variable SMALL\_QUICKSORT[](https://docs.exaloop.io/libraries/api/modules/numpy/sorting/#variable-small_quicksort "Copy/link to this API element")

```python
SMALL_QUICKSORT: Literal[int] = 15
```

variable SMALL\_MERGESORT[](https://docs.exaloop.io/libraries/api/modules/numpy/sorting/#variable-small_mergesort "Copy/link to this API element")

```python
SMALL_MERGESORT: Literal[int] = 20
```

variable SMALL\_STRING[](https://docs.exaloop.io/libraries/api/modules/numpy/sorting/#variable-small_string "Copy/link to this API element")

```python
SMALL_STRING: Literal[int] = 16
```

function mergesort[](https://docs.exaloop.io/libraries/api/modules/numpy/sorting/#function-mergesort "Copy/link to this API element")

```python
mergesort(start: Ptr[T], num: int, T: type)
```

function amergesort[](https://docs.exaloop.io/libraries/api/modules/numpy/sorting/#function-amergesort "Copy/link to this API element")

```python
amergesort(v: Ptr[T], tosort: Ptr[int], num: int, T: type)
```

variable INSERTION\_SORT\_THRESHOLD[](https://docs.exaloop.io/libraries/api/modules/numpy/sorting/#variable-insertion_sort_threshold "Copy/link to this API element")

```python
INSERTION_SORT_THRESHOLD: Literal[int] = 24
```

variable NINTHER\_THRESHOLD[](https://docs.exaloop.io/libraries/api/modules/numpy/sorting/#variable-ninther_threshold "Copy/link to this API element")

```python
NINTHER_THRESHOLD: Literal[int] = 128
```

variable PARTIAL\_INSERTION\_SORT\_LIMIT[](https://docs.exaloop.io/libraries/api/modules/numpy/sorting/#variable-partial_insertion_sort_limit "Copy/link to this API element")

```python
PARTIAL_INSERTION_SORT_LIMIT: Literal[int] = 8
```

function quicksort[](https://docs.exaloop.io/libraries/api/modules/numpy/sorting/#function-quicksort "Copy/link to this API element")

```python
quicksort(start: Ptr[T], n: int, T: type)
```

function aquicksort[](https://docs.exaloop.io/libraries/api/modules/numpy/sorting/#function-aquicksort "Copy/link to this API element")

```python
aquicksort(start: Ptr[T], tosort: Ptr[int], n: int, T: type)
```

function key\_of[](https://docs.exaloop.io/libraries/api/modules/numpy/sorting/#function-key_of "Copy/link to this API element")

```python
key_of(x: UT, T: type, UT: type)
```

function nth\_byte[](https://docs.exaloop.io/libraries/api/modules/numpy/sorting/#function-nth_byte "Copy/link to this API element")

```python
nth_byte(key: T, l: int, T: type)
```

function radixsort[](https://docs.exaloop.io/libraries/api/modules/numpy/sorting/#function-radixsort "Copy/link to this API element")

```python
radixsort(start: Ptr[T], num: int, T: type)
```

function aradixsort[](https://docs.exaloop.io/libraries/api/modules/numpy/sorting/#function-aradixsort "Copy/link to this API element")

```python
aradixsort(start: Ptr[T], tosort: Ptr[int], num: int, T: type)
```

variable TIMSORT\_STACK\_SIZE[](https://docs.exaloop.io/libraries/api/modules/numpy/sorting/#variable-timsort_stack_size "Copy/link to this API element")

```python
TIMSORT_STACK_SIZE: Literal[int] = 128
```

class Buffer[](https://docs.exaloop.io/libraries/api/modules/numpy/sorting/#class-buffer "Copy/link to this API element")

```python
class Buffer[T]
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/numpy/sorting/Buffer/)

Fields 3

`pw` `Ptr[T]`

`size` `int`

`T` `type`

Constructors 1

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/sorting/#class-buffer-__init__ "Copy/link to this API element")

```python
__init__(self: Self, Self: type)
```

Methods 2

resize[](https://docs.exaloop.io/libraries/api/modules/numpy/sorting/#class-buffer-resize "Copy/link to this API element")

```python
resize(self: Self, new_size: int, Self: type)
```

free[](https://docs.exaloop.io/libraries/api/modules/numpy/sorting/#class-buffer-free "Copy/link to this API element")

```python
free(self: Self, Self: type)
```

function timsort[](https://docs.exaloop.io/libraries/api/modules/numpy/sorting/#function-timsort "Copy/link to this API element")

```python
timsort(start: Ptr[T], num: int, T: type)
```

function atimsort[](https://docs.exaloop.io/libraries/api/modules/numpy/sorting/#function-atimsort "Copy/link to this API element")

```python
atimsort(start: Ptr[T], tosort: Ptr[int], num: int, T: type)
```

function stablesort[](https://docs.exaloop.io/libraries/api/modules/numpy/sorting/#function-stablesort "Copy/link to this API element")

```python
stablesort(start: Ptr[T], num: int, T: type)
```

function astablesort[](https://docs.exaloop.io/libraries/api/modules/numpy/sorting/#function-astablesort "Copy/link to this API element")

```python
astablesort(start: Ptr[T], tosort: Ptr[int], num: int, T: type)
```

variable MAX\_PIVOT\_STACK[](https://docs.exaloop.io/libraries/api/modules/numpy/sorting/#variable-max_pivot_stack "Copy/link to this API element")

```python
MAX_PIVOT_STACK: Literal[int] = 50
```

class Sortee[](https://docs.exaloop.io/libraries/api/modules/numpy/sorting/#class-sortee "Copy/link to this API element")

@tuple

```python
class Sortee[T]
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/numpy/sorting/Sortee/)

Fields 2

`v` `Ptr[T]`

`T` `type`

Methods 1

swap[](https://docs.exaloop.io/libraries/api/modules/numpy/sorting/#class-sortee-swap "Copy/link to this API element")

```python
swap(self: Self, i: int, j: int, Self: type)
```

Special methods 1

\_\_call\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/sorting/#class-sortee-__call__ "Copy/link to this API element")

```python
__call__(self: Self, i: int, Self: type)
```

class Idx[](https://docs.exaloop.io/libraries/api/modules/numpy/sorting/#class-idx "Copy/link to this API element")

@tuple

```python
class Idx
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/numpy/sorting/Idx/)

Special methods 1

\_\_call\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/sorting/#class-idx-__call__ "Copy/link to this API element")

```python
__call__(self: Self, i: int, Self: type)
```

class ArgSortee[](https://docs.exaloop.io/libraries/api/modules/numpy/sorting/#class-argsortee "Copy/link to this API element")

@tuple

```python
class ArgSortee
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/numpy/sorting/ArgSortee/)

Fields 1

`tosort` `Ptr[int]`

Methods 1

swap[](https://docs.exaloop.io/libraries/api/modules/numpy/sorting/#class-argsortee-swap "Copy/link to this API element")

```python
swap(self: Self, i: int, j: int, Self: type)
```

Special methods 1

\_\_call\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/sorting/#class-argsortee-__call__ "Copy/link to this API element")

```python
__call__(self: Self, i: int, Self: type)
```

class ArgIdx[](https://docs.exaloop.io/libraries/api/modules/numpy/sorting/#class-argidx "Copy/link to this API element")

@tuple

```python
class ArgIdx
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/numpy/sorting/ArgIdx/)

Fields 1

`tosort` `Ptr[int]`

Special methods 1

\_\_call\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/sorting/#class-argidx-__call__ "Copy/link to this API element")

```python
__call__(self: Self, i: int, Self: type)
```

function partition[](https://docs.exaloop.io/libraries/api/modules/numpy/sorting/#function-partition "Copy/link to this API element")

```python
partition(a, kth, axis = -1, kind: str = "introselect")
```

function argpartition[](https://docs.exaloop.io/libraries/api/modules/numpy/sorting/#function-argpartition "Copy/link to this API element")

```python
argpartition(a: ndarray, kth, axis = -1, kind: str = "introselect")
```

function lexsort[](https://docs.exaloop.io/libraries/api/modules/numpy/sorting/#function-lexsort "Copy/link to this API element")

```python
lexsort(keys, axis: int = -1)
```

function sort[](https://docs.exaloop.io/libraries/api/modules/numpy/sorting/#function-sort "Copy/link to this API element")

```python
sort(a, axis = -1, kind: Optional[str] = None)
```

function argsort[](https://docs.exaloop.io/libraries/api/modules/numpy/sorting/#function-argsort "Copy/link to this API element")

```python
argsort(a, axis = -1, kind: Optional[str] = None)
```

function sort\_complex[](https://docs.exaloop.io/libraries/api/modules/numpy/sorting/#function-sort_complex "Copy/link to this API element")

```python
sort_complex(a)
```

class ndarray[](https://docs.exaloop.io/libraries/api/modules/numpy/sorting/#class-ndarray "Copy/link to this API element")

@extend

```python
class ndarray[dtype, ndim]
```

Extends [`numpy.ndarray.ndarray`](https://docs.exaloop.io/libraries/api/classes/numpy/ndarray/ndarray/) · [View complete class API →](https://docs.exaloop.io/libraries/api/classes/numpy/ndarray/ndarray/)

Methods 4

sort[](https://docs.exaloop.io/libraries/api/modules/numpy/sorting/#class-ndarray-sort "Copy/link to this API element")

```python
sort(self: Self, axis: int = -1, kind: Optional[str] = None, Self: type)
```

argsort[](https://docs.exaloop.io/libraries/api/modules/numpy/sorting/#class-ndarray-argsort "Copy/link to this API element")

```python
argsort(self: Self, axis: int = -1, kind: Optional[str] = None, Self: type)
```

partition[](https://docs.exaloop.io/libraries/api/modules/numpy/sorting/#class-ndarray-partition "Copy/link to this API element")

```python
partition(self: Self, kth, axis: int = -1, kind: str = "introselect", Self: type)
```

argpartition[](https://docs.exaloop.io/libraries/api/modules/numpy/sorting/#class-ndarray-argpartition "Copy/link to this API element")

```python
argpartition(self: Self, kth, axis: int = -1, kind: str = "introselect", Self: type)
```

Source: https://docs.exaloop.io/libraries/api/modules/numpy/sorting/
