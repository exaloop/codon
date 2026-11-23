# internal.sort

Standard library Module [View source](https://github.com/exaloop/codon/blob/master/stdlib/internal/sort.codon)

function cnp\_sort\_int16[](https://docs.exaloop.io/libraries/api/modules/internal/sort/#function-cnp_sort_int16 "Copy/link to this API element")

@nocapture @llvm\_nocapture @llvm\_nofree @C

```python
cnp_sort_int16(buffer: cobj, size: int) -> None
```

function cnp\_sort\_uint16[](https://docs.exaloop.io/libraries/api/modules/internal/sort/#function-cnp_sort_uint16 "Copy/link to this API element")

@nocapture @llvm\_nocapture @llvm\_nofree @C

```python
cnp_sort_uint16(buffer: cobj, size: int) -> None
```

function cnp\_sort\_int32[](https://docs.exaloop.io/libraries/api/modules/internal/sort/#function-cnp_sort_int32 "Copy/link to this API element")

@nocapture @llvm\_nocapture @llvm\_nofree @C

```python
cnp_sort_int32(buffer: cobj, size: int) -> None
```

function cnp\_sort\_uint32[](https://docs.exaloop.io/libraries/api/modules/internal/sort/#function-cnp_sort_uint32 "Copy/link to this API element")

@nocapture @llvm\_nocapture @llvm\_nofree @C

```python
cnp_sort_uint32(buffer: cobj, size: int) -> None
```

function cnp\_sort\_int64[](https://docs.exaloop.io/libraries/api/modules/internal/sort/#function-cnp_sort_int64 "Copy/link to this API element")

@nocapture @llvm\_nocapture @llvm\_nofree @C

```python
cnp_sort_int64(buffer: cobj, size: int) -> None
```

function cnp\_sort\_uint64[](https://docs.exaloop.io/libraries/api/modules/internal/sort/#function-cnp_sort_uint64 "Copy/link to this API element")

@nocapture @llvm\_nocapture @llvm\_nofree @C

```python
cnp_sort_uint64(buffer: cobj, size: int) -> None
```

function cnp\_sort\_uint128[](https://docs.exaloop.io/libraries/api/modules/internal/sort/#function-cnp_sort_uint128 "Copy/link to this API element")

@nocapture @llvm\_nocapture @llvm\_nofree @C

```python
cnp_sort_uint128(buffer: cobj, size: int) -> None
```

function cnp\_sort\_float32[](https://docs.exaloop.io/libraries/api/modules/internal/sort/#function-cnp_sort_float32 "Copy/link to this API element")

@nocapture @llvm\_nocapture @llvm\_nofree @C

```python
cnp_sort_float32(buffer: cobj, size: int) -> None
```

function cnp\_sort\_float64[](https://docs.exaloop.io/libraries/api/modules/internal/sort/#function-cnp_sort_float64 "Copy/link to this API element")

@nocapture @llvm\_nocapture @llvm\_nofree @C

```python
cnp_sort_float64(buffer: cobj, size: int) -> None
```

function cnp\_sort\_float32\_nan\_last[](https://docs.exaloop.io/libraries/api/modules/internal/sort/#function-cnp_sort_float32_nan_last "Copy/link to this API element")

@nocapture @llvm\_nocapture @llvm\_nofree @C

```python
cnp_sort_float32_nan_last(buffer: cobj, size: int) -> None
```

function cnp\_sort\_float64\_nan\_last[](https://docs.exaloop.io/libraries/api/modules/internal/sort/#function-cnp_sort_float64_nan_last "Copy/link to this API element")

@nocapture @llvm\_nocapture @llvm\_nofree @C

```python
cnp_sort_float64_nan_last(buffer: cobj, size: int) -> None
```

function sorted[](https://docs.exaloop.io/libraries/api/modules/internal/sort/#function-sorted "Copy/link to this API element")

```python
sorted[algorithm: Literal[str] = "auto", T: type](v: Generator[T], /, *, key = None, reverse: bool = False) -> List[T]
```

@overload

```python
sorted[algorithm: Literal[str] = "auto", T: type](v: List[T], /, *, key = None, reverse: bool = False) -> List[T]
```

class List[](https://docs.exaloop.io/libraries/api/modules/internal/sort/#class-list "Copy/link to this API element")

@extend

```python
class List[T]
```

Extends [`internal.types.collections.list.List`](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/list/List/) · [View complete class API →](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/list/List/)

Methods 1

sort[](https://docs.exaloop.io/libraries/api/modules/internal/sort/#class-list-sort "Copy/link to this API element")

```python
sort[algorithm: Literal[str] = "auto"](self: Self, /, *, key = None, reverse: bool = False)
```

Source: https://docs.exaloop.io/libraries/api/modules/internal/sort/
