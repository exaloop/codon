# internal.types.collections.bytearray

Standard library Module [View source](https://github.com/exaloop/codon/blob/master/stdlib/internal/types/collections/bytearray.codon)

class bytearray[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/#class-bytearray "Copy/link to this API element")

```python
class bytearray
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/bytearray/bytearray/)

Constructors 6

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/#class-bytearray-__init__ "Copy/link to this API element")

```python
__init__(self: Self, /)
```

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/#class-bytearray-__init__-overload-2 "Copy/link to this API element")

```python
__init__(self: Self, /, size: int)
```

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/#class-bytearray-__init__-overload-3 "Copy/link to this API element")

```python
__init__(self: Self, /, source: bytes)
```

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/#class-bytearray-__init__-overload-4 "Copy/link to this API element")

```python
__init__(self: Self, /, source: bytearray)
```

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/#class-bytearray-__init__-overload-5 "Copy/link to this API element")

```python
__init__(self: Self, /, source: Generator[int])
```

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/#class-bytearray-__init__-overload-6 "Copy/link to this API element")

```python
__init__(self: Self, /, source: str, encoding: str, errors: str = "strict")
```

Methods 56

fromhex[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/#class-bytearray-fromhex "Copy/link to this API element")

@staticmethod

```python
fromhex(value: str, /) -> bytearray
```

```python
fromhex(value: bytes, /) -> bytearray
```

```python
fromhex(value: bytearray, /) -> bytearray
```

maketrans[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/#class-bytearray-maketrans "Copy/link to this API element")

@staticmethod

```python
maketrans(source, replacement, /) -> bytes
```

copy[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/#class-bytearray-copy "Copy/link to this API element")

```python
copy(self: Self, /) -> bytearray
```

append[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/#class-bytearray-append "Copy/link to this API element")

```python
append(self: Self, value: int, /)
```

extend[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/#class-bytearray-extend "Copy/link to this API element")

```python
extend(self: Self, source: Generator[int], /)
```

```python
extend(self: Self, source: bytes, /)
```

```python
extend(self: Self, source: bytearray, /)
```

insert[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/#class-bytearray-insert "Copy/link to this API element")

```python
insert(self: Self, index: int, value: int, /)
```

pop[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/#class-bytearray-pop "Copy/link to this API element")

```python
pop(self: Self, index: int = -1, /) -> int
```

remove[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/#class-bytearray-remove "Copy/link to this API element")

```python
remove(self: Self, value: int, /)
```

clear[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/#class-bytearray-clear "Copy/link to this API element")

```python
clear(self: Self, /)
```

resize[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/#class-bytearray-resize "Copy/link to this API element")

```python
resize(self: Self, size: int, /)
```

reverse[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/#class-bytearray-reverse "Copy/link to this API element")

```python
reverse(self: Self, /)
```

count[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/#class-bytearray-count "Copy/link to this API element")

```python
count(self: Self, pattern, start: Optional[int] = 0, end: Optional[int] = None, /) -> int
```

index[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/#class-bytearray-index "Copy/link to this API element")

```python
index(self: Self, pattern, start: Optional[int] = 0, end: Optional[int] = None, /) -> int
```

find[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/#class-bytearray-find "Copy/link to this API element")

```python
find(self: Self, pattern, start: Optional[int] = 0, end: Optional[int] = None, /) -> int
```

rfind[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/#class-bytearray-rfind "Copy/link to this API element")

```python
rfind(self: Self, pattern, start: Optional[int] = 0, end: Optional[int] = None, /) -> int
```

rindex[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/#class-bytearray-rindex "Copy/link to this API element")

```python
rindex(self: Self, pattern, start: Optional[int] = 0, end: Optional[int] = None, /) -> int
```

startswith[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/#class-bytearray-startswith "Copy/link to this API element")

```python
startswith(self: Self, prefix, start: Optional[int] = 0, end: Optional[int] = None, /) -> bool
```

endswith[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/#class-bytearray-endswith "Copy/link to this API element")

```python
endswith(self: Self, suffix, start: Optional[int] = 0, end: Optional[int] = None, /) -> bool
```

removeprefix[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/#class-bytearray-removeprefix "Copy/link to this API element")

```python
removeprefix(self: Self, prefix, /) -> bytearray
```

removesuffix[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/#class-bytearray-removesuffix "Copy/link to this API element")

```python
removesuffix(self: Self, suffix, /) -> bytearray
```

isdigit[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/#class-bytearray-isdigit "Copy/link to this API element")

```python
isdigit(self: Self, /) -> bool
```

islower[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/#class-bytearray-islower "Copy/link to this API element")

```python
islower(self: Self, /) -> bool
```

isupper[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/#class-bytearray-isupper "Copy/link to this API element")

```python
isupper(self: Self, /) -> bool
```

isalnum[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/#class-bytearray-isalnum "Copy/link to this API element")

```python
isalnum(self: Self, /) -> bool
```

isalpha[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/#class-bytearray-isalpha "Copy/link to this API element")

```python
isalpha(self: Self, /) -> bool
```

isspace[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/#class-bytearray-isspace "Copy/link to this API element")

```python
isspace(self: Self, /) -> bool
```

istitle[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/#class-bytearray-istitle "Copy/link to this API element")

```python
istitle(self: Self, /) -> bool
```

isdecimal[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/#class-bytearray-isdecimal "Copy/link to this API element")

```python
isdecimal(self: Self) -> bool
```

isascii[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/#class-bytearray-isascii "Copy/link to this API element")

```python
isascii(self: Self, /) -> bool
```

isnumeric[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/#class-bytearray-isnumeric "Copy/link to this API element")

```python
isnumeric(self: Self) -> bool
```

isidentifier[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/#class-bytearray-isidentifier "Copy/link to this API element")

```python
isidentifier(self: Self) -> bool
```

isprintable[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/#class-bytearray-isprintable "Copy/link to this API element")

```python
isprintable(self: Self) -> bool
```

capitalize[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/#class-bytearray-capitalize "Copy/link to this API element")

```python
capitalize(self: Self, /) -> bytearray
```

lower[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/#class-bytearray-lower "Copy/link to this API element")

```python
lower(self: Self, /) -> bytearray
```

upper[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/#class-bytearray-upper "Copy/link to this API element")

```python
upper(self: Self, /) -> bytearray
```

casefold[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/#class-bytearray-casefold "Copy/link to this API element")

```python
casefold(self: Self) -> bytearray
```

swapcase[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/#class-bytearray-swapcase "Copy/link to this API element")

```python
swapcase(self: Self, /) -> bytearray
```

title[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/#class-bytearray-title "Copy/link to this API element")

```python
title(self: Self, /) -> bytearray
```

ljust[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/#class-bytearray-ljust "Copy/link to this API element")

```python
ljust(self: Self, width: int, fillchar: bytes = " ", /) -> bytearray
```

rjust[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/#class-bytearray-rjust "Copy/link to this API element")

```python
rjust(self: Self, width: int, fillchar: bytes = " ", /) -> bytearray
```

center[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/#class-bytearray-center "Copy/link to this API element")

```python
center(self: Self, width: int, fillchar: bytes = " ", /) -> bytearray
```

zfill[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/#class-bytearray-zfill "Copy/link to this API element")

```python
zfill(self: Self, width: int, /) -> bytearray
```

lstrip[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/#class-bytearray-lstrip "Copy/link to this API element")

```python
lstrip(self: Self, chars: Optional[bytes] = None, /) -> bytearray
```

rstrip[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/#class-bytearray-rstrip "Copy/link to this API element")

```python
rstrip(self: Self, chars: Optional[bytes] = None, /) -> bytearray
```

strip[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/#class-bytearray-strip "Copy/link to this API element")

```python
strip(self: Self, chars: Optional[bytes] = None, /) -> bytearray
```

partition[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/#class-bytearray-partition "Copy/link to this API element")

```python
partition(self: Self, separator: bytes, /) -> Tuple[(bytearray, bytearray, bytearray)]
```

rpartition[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/#class-bytearray-rpartition "Copy/link to this API element")

```python
rpartition(self: Self, separator: bytes, /) -> Tuple[(bytearray, bytearray, bytearray)]
```

split[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/#class-bytearray-split "Copy/link to this API element")

```python
split(self: Self, /, sep: Optional[bytes] = None, maxsplit: int = -1) -> List[bytearray]
```

rsplit[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/#class-bytearray-rsplit "Copy/link to this API element")

```python
rsplit(self: Self, /, sep: Optional[bytes] = None, maxsplit: int = -1) -> List[bytearray]
```

splitlines[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/#class-bytearray-splitlines "Copy/link to this API element")

```python
splitlines(self: Self, /, keepends: bool = False) -> List[bytearray]
```

replace[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/#class-bytearray-replace "Copy/link to this API element")

```python
replace(self: Self, old: bytes, new: bytes, count: int = -1, /) -> bytearray
```

expandtabs[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/#class-bytearray-expandtabs "Copy/link to this API element")

```python
expandtabs(self: Self, /, tabsize: int = 8) -> bytearray
```

translate[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/#class-bytearray-translate "Copy/link to this API element")

```python
translate(self: Self, table, /) -> bytearray
```

```python
translate(self: Self, table: bytes, /, delete: bytes = "") -> bytearray
```

```python
translate(self: Self, table: bytearray, /, delete: bytes = "") -> bytearray
```

hex[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/#class-bytearray-hex "Copy/link to this API element")

```python
hex(self: Self, /) -> str
```

```python
hex(self: Self, /, sep: str, bytes_per_sep: int = 1) -> str
```

```python
hex(self: Self, /, sep: bytes, bytes_per_sep: int = 1) -> str
```

join[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/#class-bytearray-join "Copy/link to this API element")

```python
join[T: type](self: Self, values: Generator[T], /) -> bytearray
```

```python
join[T: type](self: Self, values: List[T], /) -> bytearray
```

decode[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/#class-bytearray-decode "Copy/link to this API element")

```python
decode(self: Self, /, encoding: str = "utf-8", errors: str = "strict") -> str
```

Special methods 24

\_\_len\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/#class-bytearray-__len__ "Copy/link to this API element")

```python
__len__(self: Self, /) -> int
```

\_\_bool\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/#class-bytearray-__bool__ "Copy/link to this API element")

```python
__bool__(self: Self) -> bool
```

\_\_iter\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/#class-bytearray-__iter__ "Copy/link to this API element")

```python
__iter__(self: Self, /) -> Generator[int]
```

\_\_reversed\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/#class-bytearray-__reversed__ "Copy/link to this API element")

```python
__reversed__(self: Self) -> Generator[int]
```

\_\_getitem\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/#class-bytearray-__getitem__ "Copy/link to this API element")

```python
__getitem__(self: Self, index: int, /) -> int
```

```python
__getitem__(self: Self, slice: Slice, /) -> bytearray
```

\_\_setitem\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/#class-bytearray-__setitem__ "Copy/link to this API element")

```python
__setitem__(self: Self, slice: Slice, source: Generator[int], /)
```

```python
__setitem__(self: Self, index: int, value: int, /)
```

```python
__setitem__(self: Self, slice: Slice, source: bytes, /)
```

```python
__setitem__(self: Self, slice: Slice, source: bytearray, /)
```

\_\_delitem\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/#class-bytearray-__delitem__ "Copy/link to this API element")

```python
__delitem__(self: Self, index: int, /)
```

```python
__delitem__(self: Self, slice: Slice, /)
```

\_\_contains\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/#class-bytearray-__contains__ "Copy/link to this API element")

```python
__contains__(self: Self, value: int, /) -> bool
```

```python
__contains__(self: Self, pattern: bytes, /) -> bool
```

```python
__contains__(self: Self, pattern: bytearray, /) -> bool
```

\_\_bytes\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/#class-bytearray-__bytes__ "Copy/link to this API element")

```python
__bytes__(self: Self) -> bytes
```

\_\_str\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/#class-bytearray-__str__ "Copy/link to this API element")

```python
__str__(self: Self, /) -> str
```

\_\_repr\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/#class-bytearray-__repr__ "Copy/link to this API element")

```python
__repr__(self: Self, /) -> str
```

\_\_copy\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/#class-bytearray-__copy__ "Copy/link to this API element")

```python
__copy__(self: Self) -> bytearray
```

\_\_deepcopy\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/#class-bytearray-__deepcopy__ "Copy/link to this API element")

```python
__deepcopy__(self: Self) -> bytearray
```

\_\_eq\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/#class-bytearray-__eq__ "Copy/link to this API element")

```python
__eq__(self: Self, other: bytearray, /) -> bool
```

```python
__eq__(self: Self, other: bytes, /) -> bool
```

\_\_ne\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/#class-bytearray-__ne__ "Copy/link to this API element")

```python
__ne__(self: Self, other: bytearray, /) -> bool
```

```python
__ne__(self: Self, other: bytes, /) -> bool
```

\_\_lt\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/#class-bytearray-__lt__ "Copy/link to this API element")

```python
__lt__(self: Self, other: bytearray, /) -> bool
```

```python
__lt__(self: Self, other: bytes, /) -> bool
```

\_\_le\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/#class-bytearray-__le__ "Copy/link to this API element")

```python
__le__(self: Self, other: bytearray, /) -> bool
```

```python
__le__(self: Self, other: bytes, /) -> bool
```

\_\_gt\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/#class-bytearray-__gt__ "Copy/link to this API element")

```python
__gt__(self: Self, other: bytearray, /) -> bool
```

```python
__gt__(self: Self, other: bytes, /) -> bool
```

\_\_ge\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/#class-bytearray-__ge__ "Copy/link to this API element")

```python
__ge__(self: Self, other: bytearray, /) -> bool
```

```python
__ge__(self: Self, other: bytes, /) -> bool
```

\_\_add\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/#class-bytearray-__add__ "Copy/link to this API element")

```python
__add__(self: Self, other: bytearray, /) -> bytearray
```

```python
__add__(self: Self, other: bytes, /) -> bytearray
```

\_\_iadd\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/#class-bytearray-__iadd__ "Copy/link to this API element")

```python
__iadd__(self: Self, other: bytearray, /) -> bytearray
```

```python
__iadd__(self: Self, other: bytes, /) -> bytearray
```

\_\_mul\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/#class-bytearray-__mul__ "Copy/link to this API element")

```python
__mul__(self: Self, count: int, /) -> bytearray
```

\_\_rmul\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/#class-bytearray-__rmul__ "Copy/link to this API element")

```python
__rmul__(self: Self, count: int, /) -> bytearray
```

\_\_imul\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/#class-bytearray-__imul__ "Copy/link to this API element")

```python
__imul__(self: Self, count: int, /) -> bytearray
```

class bytes[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/#class-bytes "Copy/link to this API element")

@extend

```python
class bytes
```

Extends [`internal.core.bytes`](https://docs.exaloop.io/libraries/api/classes/internal/core/bytes/) · [View complete class API →](https://docs.exaloop.io/libraries/api/classes/internal/core/bytes/)

Methods 5

count[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/#class-bytes-count "Copy/link to this API element")

```python
count(self: Self, sub: bytearray, start: Optional[int] = 0, end: Optional[int] = None, /) -> int
```

find[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/#class-bytes-find "Copy/link to this API element")

```python
find(self: Self, sub: bytearray, start: Optional[int] = 0, end: Optional[int] = None, /) -> int
```

rfind[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/#class-bytes-rfind "Copy/link to this API element")

```python
rfind(self: Self, sub: bytearray, start: Optional[int] = 0, end: Optional[int] = None, /) -> int
```

startswith[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/#class-bytes-startswith "Copy/link to this API element")

```python
startswith(self: Self, prefix: bytearray, start: Optional[int] = 0, end: Optional[int] = None, /) -> bool
```

endswith[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/#class-bytes-endswith "Copy/link to this API element")

```python
endswith(self: Self, suffix: bytearray, start: Optional[int] = 0, end: Optional[int] = None, /) -> bool
```

Special methods 7

\_\_eq\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/#class-bytes-__eq__ "Copy/link to this API element")

```python
__eq__(self: Self, other: bytearray, /) -> bool
```

\_\_ne\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/#class-bytes-__ne__ "Copy/link to this API element")

```python
__ne__(self: Self, other: bytearray, /) -> bool
```

\_\_add\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/#class-bytes-__add__ "Copy/link to this API element")

```python
__add__(self: Self, other: bytearray, /) -> bytes
```

\_\_lt\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/#class-bytes-__lt__ "Copy/link to this API element")

```python
__lt__(self: Self, other: bytearray, /) -> bool
```

\_\_le\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/#class-bytes-__le__ "Copy/link to this API element")

```python
__le__(self: Self, other: bytearray, /) -> bool
```

\_\_gt\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/#class-bytes-__gt__ "Copy/link to this API element")

```python
__gt__(self: Self, other: bytearray, /) -> bool
```

\_\_ge\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/#class-bytes-__ge__ "Copy/link to this API element")

```python
__ge__(self: Self, other: bytearray, /) -> bool
```

Source: https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/
