# internal.types.collections.bytearray

Standard library module [View source](https://github.com/exaloop/codon/blob/master/stdlib/internal/types/collections/bytearray.codon)

class bytearray[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/#class-bytearray "Copy/link to this API element")

```python
class bytearray
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/bytearray/bytearray/)

Constructors 6

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/#class-bytearray-__init__ "Copy/link to this API element")

```python
__init__(self: Self, Self: type)
```

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/#class-bytearray-__init__-overload-2 "Copy/link to this API element")

```python
__init__(self: Self, size: int, Self: type)
```

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/#class-bytearray-__init__-overload-3 "Copy/link to this API element")

```python
__init__(self: Self, source: bytes, Self: type)
```

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/#class-bytearray-__init__-overload-4 "Copy/link to this API element")

```python
__init__(self: Self, source: bytearray, Self: type)
```

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/#class-bytearray-__init__-overload-5 "Copy/link to this API element")

```python
__init__(self: Self, source: Generator[int], Self: type)
```

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/#class-bytearray-__init__-overload-6 "Copy/link to this API element")

```python
__init__(self: Self, source: str, encoding: str, errors: str = "strict", Self: type)
```

Methods 56

fromhex[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/#class-bytearray-fromhex "Copy/link to this API element")

@staticmethod

```python
fromhex(value: str)
```

```python
fromhex(value: bytes)
```

```python
fromhex(value: bytearray)
```

maketrans[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/#class-bytearray-maketrans "Copy/link to this API element")

@staticmethod

```python
maketrans(source, replacement)
```

copy[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/#class-bytearray-copy "Copy/link to this API element")

```python
copy(self: Self, Self: type)
```

append[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/#class-bytearray-append "Copy/link to this API element")

```python
append(self: Self, value: int, Self: type)
```

extend[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/#class-bytearray-extend "Copy/link to this API element")

```python
extend(self: Self, source: Generator[int], Self: type)
```

```python
extend(self: Self, source: bytes, Self: type)
```

```python
extend(self: Self, source: bytearray, Self: type)
```

insert[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/#class-bytearray-insert "Copy/link to this API element")

```python
insert(self: Self, index: int, value: int, Self: type)
```

pop[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/#class-bytearray-pop "Copy/link to this API element")

```python
pop(self: Self, index: int = -1, Self: type)
```

remove[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/#class-bytearray-remove "Copy/link to this API element")

```python
remove(self: Self, value: int, Self: type)
```

clear[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/#class-bytearray-clear "Copy/link to this API element")

```python
clear(self: Self, Self: type)
```

resize[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/#class-bytearray-resize "Copy/link to this API element")

```python
resize(self: Self, size: int, Self: type)
```

reverse[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/#class-bytearray-reverse "Copy/link to this API element")

```python
reverse(self: Self, Self: type)
```

count[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/#class-bytearray-count "Copy/link to this API element")

```python
count(self: Self, value: int, Self: type)
```

```python
count(self: Self, pattern: bytes, start: int = 0, end: Optional[int] = None, Self: type)
```

```python
count(self: Self, pattern: bytearray, start: int = 0, end: Optional[int] = None, Self: type)
```

index[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/#class-bytearray-index "Copy/link to this API element")

```python
index(self: Self, value: int, start: int = 0, end: Optional[int] = None, Self: type)
```

```python
index(self: Self, pattern: bytes, start: int = 0, end: Optional[int] = None, Self: type)
```

```python
index(self: Self, pattern: bytearray, start: int = 0, end: Optional[int] = None, Self: type)
```

find[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/#class-bytearray-find "Copy/link to this API element")

```python
find(self: Self, pattern: bytes, start: int = 0, end: Optional[int] = None, Self: type)
```

```python
find(self: Self, pattern: bytearray, start: int = 0, end: Optional[int] = None, Self: type)
```

rfind[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/#class-bytearray-rfind "Copy/link to this API element")

```python
rfind(self: Self, pattern: bytes, start: int = 0, end: Optional[int] = None, Self: type)
```

```python
rfind(self: Self, pattern: bytearray, start: int = 0, end: Optional[int] = None, Self: type)
```

rindex[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/#class-bytearray-rindex "Copy/link to this API element")

```python
rindex(self: Self, pattern: bytes, start: int = 0, end: Optional[int] = None, Self: type)
```

```python
rindex(self: Self, pattern: bytearray, start: int = 0, end: Optional[int] = None, Self: type)
```

startswith[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/#class-bytearray-startswith "Copy/link to this API element")

```python
startswith(self: Self, prefix: bytes, start: int = 0, end: Optional[int] = None, Self: type)
```

```python
startswith(self: Self, prefix: bytearray, start: int = 0, end: Optional[int] = None, Self: type)
```

endswith[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/#class-bytearray-endswith "Copy/link to this API element")

```python
endswith(self: Self, suffix: bytes, start: int = 0, end: Optional[int] = None, Self: type)
```

```python
endswith(self: Self, suffix: bytearray, start: int = 0, end: Optional[int] = None, Self: type)
```

removeprefix[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/#class-bytearray-removeprefix "Copy/link to this API element")

```python
removeprefix(self: Self, prefix, Self: type)
```

removesuffix[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/#class-bytearray-removesuffix "Copy/link to this API element")

```python
removesuffix(self: Self, suffix, Self: type)
```

isdigit[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/#class-bytearray-isdigit "Copy/link to this API element")

```python
isdigit(self: Self, Self: type)
```

islower[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/#class-bytearray-islower "Copy/link to this API element")

```python
islower(self: Self, Self: type)
```

isupper[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/#class-bytearray-isupper "Copy/link to this API element")

```python
isupper(self: Self, Self: type)
```

isalnum[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/#class-bytearray-isalnum "Copy/link to this API element")

```python
isalnum(self: Self, Self: type)
```

isalpha[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/#class-bytearray-isalpha "Copy/link to this API element")

```python
isalpha(self: Self, Self: type)
```

isspace[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/#class-bytearray-isspace "Copy/link to this API element")

```python
isspace(self: Self, Self: type)
```

istitle[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/#class-bytearray-istitle "Copy/link to this API element")

```python
istitle(self: Self, Self: type)
```

isdecimal[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/#class-bytearray-isdecimal "Copy/link to this API element")

```python
isdecimal(self: Self, Self: type)
```

isascii[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/#class-bytearray-isascii "Copy/link to this API element")

```python
isascii(self: Self, Self: type)
```

isnumeric[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/#class-bytearray-isnumeric "Copy/link to this API element")

```python
isnumeric(self: Self, Self: type)
```

isidentifier[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/#class-bytearray-isidentifier "Copy/link to this API element")

```python
isidentifier(self: Self, Self: type)
```

isprintable[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/#class-bytearray-isprintable "Copy/link to this API element")

```python
isprintable(self: Self, Self: type)
```

capitalize[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/#class-bytearray-capitalize "Copy/link to this API element")

```python
capitalize(self: Self, Self: type)
```

lower[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/#class-bytearray-lower "Copy/link to this API element")

```python
lower(self: Self, Self: type)
```

upper[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/#class-bytearray-upper "Copy/link to this API element")

```python
upper(self: Self, Self: type)
```

casefold[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/#class-bytearray-casefold "Copy/link to this API element")

```python
casefold(self: Self, Self: type)
```

swapcase[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/#class-bytearray-swapcase "Copy/link to this API element")

```python
swapcase(self: Self, Self: type)
```

title[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/#class-bytearray-title "Copy/link to this API element")

```python
title(self: Self, Self: type)
```

ljust[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/#class-bytearray-ljust "Copy/link to this API element")

```python
ljust(self: Self, width: int, fillchar: bytes = " ", Self: type)
```

rjust[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/#class-bytearray-rjust "Copy/link to this API element")

```python
rjust(self: Self, width: int, fillchar: bytes = " ", Self: type)
```

center[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/#class-bytearray-center "Copy/link to this API element")

```python
center(self: Self, width: int, fillchar: bytes = " ", Self: type)
```

zfill[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/#class-bytearray-zfill "Copy/link to this API element")

```python
zfill(self: Self, width: int, Self: type)
```

lstrip[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/#class-bytearray-lstrip "Copy/link to this API element")

```python
lstrip(self: Self, Self: type)
```

```python
lstrip(self: Self, chars: bytes, Self: type)
```

rstrip[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/#class-bytearray-rstrip "Copy/link to this API element")

```python
rstrip(self: Self, Self: type)
```

```python
rstrip(self: Self, chars: bytes, Self: type)
```

strip[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/#class-bytearray-strip "Copy/link to this API element")

```python
strip(self: Self, Self: type)
```

```python
strip(self: Self, chars: bytes, Self: type)
```

partition[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/#class-bytearray-partition "Copy/link to this API element")

```python
partition(self: Self, separator: bytes, Self: type)
```

rpartition[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/#class-bytearray-rpartition "Copy/link to this API element")

```python
rpartition(self: Self, separator: bytes, Self: type)
```

split[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/#class-bytearray-split "Copy/link to this API element")

```python
split(self: Self, separator: Optional[bytes] = None, maxsplit: int = -1, Self: type)
```

rsplit[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/#class-bytearray-rsplit "Copy/link to this API element")

```python
rsplit(self: Self, separator: Optional[bytes] = None, maxsplit: int = -1, Self: type)
```

splitlines[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/#class-bytearray-splitlines "Copy/link to this API element")

```python
splitlines(self: Self, keepends: bool = False, Self: type)
```

replace[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/#class-bytearray-replace "Copy/link to this API element")

```python
replace(self: Self, old: bytes, new: bytes, count: int = -1, Self: type)
```

expandtabs[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/#class-bytearray-expandtabs "Copy/link to this API element")

```python
expandtabs(self: Self, tabsize: int = 8, Self: type)
```

translate[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/#class-bytearray-translate "Copy/link to this API element")

```python
translate(self: Self, table, Self: type)
```

```python
translate(self: Self, table: bytes, delete: bytes = "", Self: type)
```

```python
translate(self: Self, table: bytearray, delete: bytes = "", Self: type)
```

hex[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/#class-bytearray-hex "Copy/link to this API element")

```python
hex(self: Self, Self: type)
```

```python
hex(self: Self, sep: str, bytes_per_sep: int = 1, Self: type)
```

```python
hex(self: Self, sep: bytes, bytes_per_sep: int = 1, Self: type)
```

join[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/#class-bytearray-join "Copy/link to this API element")

```python
join(self: Self, values: Generator[T], T: type, Self: type)
```

```python
join(self: Self, values: List[T], T: type, Self: type)
```

decode[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/#class-bytearray-decode "Copy/link to this API element")

```python
decode(self: Self, encoding: str = "utf-8", errors: str = "strict", Self: type)
```

Special methods 24

\_\_len\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/#class-bytearray-__len__ "Copy/link to this API element")

```python
__len__(self: Self, Self: type)
```

\_\_bool\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/#class-bytearray-__bool__ "Copy/link to this API element")

```python
__bool__(self: Self, Self: type)
```

\_\_iter\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/#class-bytearray-__iter__ "Copy/link to this API element")

```python
__iter__(self: Self, Self: type)
```

\_\_reversed\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/#class-bytearray-__reversed__ "Copy/link to this API element")

```python
__reversed__(self: Self, Self: type)
```

\_\_getitem\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/#class-bytearray-__getitem__ "Copy/link to this API element")

```python
__getitem__(self: Self, index: int, Self: type)
```

```python
__getitem__(self: Self, slice: Slice, Self: type)
```

\_\_setitem\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/#class-bytearray-__setitem__ "Copy/link to this API element")

```python
__setitem__(self: Self, slice: Slice, source: Generator[int], Self: type)
```

```python
__setitem__(self: Self, index: int, value: int, Self: type)
```

```python
__setitem__(self: Self, slice: Slice, source: bytes, Self: type)
```

```python
__setitem__(self: Self, slice: Slice, source: bytearray, Self: type)
```

\_\_delitem\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/#class-bytearray-__delitem__ "Copy/link to this API element")

```python
__delitem__(self: Self, index: int, Self: type)
```

```python
__delitem__(self: Self, slice: Slice, Self: type)
```

\_\_contains\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/#class-bytearray-__contains__ "Copy/link to this API element")

```python
__contains__(self: Self, value: int, Self: type)
```

```python
__contains__(self: Self, pattern: bytes, Self: type)
```

```python
__contains__(self: Self, pattern: bytearray, Self: type)
```

\_\_bytes\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/#class-bytearray-__bytes__ "Copy/link to this API element")

```python
__bytes__(self: Self, Self: type)
```

\_\_str\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/#class-bytearray-__str__ "Copy/link to this API element")

```python
__str__(self: Self, Self: type)
```

\_\_repr\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/#class-bytearray-__repr__ "Copy/link to this API element")

```python
__repr__(self: Self, Self: type)
```

\_\_copy\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/#class-bytearray-__copy__ "Copy/link to this API element")

```python
__copy__(self: Self, Self: type)
```

\_\_deepcopy\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/#class-bytearray-__deepcopy__ "Copy/link to this API element")

```python
__deepcopy__(self: Self, Self: type)
```

\_\_eq\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/#class-bytearray-__eq__ "Copy/link to this API element")

```python
__eq__(self: Self, other: bytearray, Self: type)
```

```python
__eq__(self: Self, other: bytes, Self: type)
```

\_\_ne\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/#class-bytearray-__ne__ "Copy/link to this API element")

```python
__ne__(self: Self, other: bytearray, Self: type)
```

```python
__ne__(self: Self, other: bytes, Self: type)
```

\_\_lt\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/#class-bytearray-__lt__ "Copy/link to this API element")

```python
__lt__(self: Self, other: bytearray, Self: type)
```

```python
__lt__(self: Self, other: bytes, Self: type)
```

\_\_le\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/#class-bytearray-__le__ "Copy/link to this API element")

```python
__le__(self: Self, other: bytearray, Self: type)
```

```python
__le__(self: Self, other: bytes, Self: type)
```

\_\_gt\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/#class-bytearray-__gt__ "Copy/link to this API element")

```python
__gt__(self: Self, other: bytearray, Self: type)
```

```python
__gt__(self: Self, other: bytes, Self: type)
```

\_\_ge\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/#class-bytearray-__ge__ "Copy/link to this API element")

```python
__ge__(self: Self, other: bytearray, Self: type)
```

```python
__ge__(self: Self, other: bytes, Self: type)
```

\_\_add\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/#class-bytearray-__add__ "Copy/link to this API element")

```python
__add__(self: Self, other: bytearray, Self: type)
```

```python
__add__(self: Self, other: bytes, Self: type)
```

\_\_iadd\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/#class-bytearray-__iadd__ "Copy/link to this API element")

```python
__iadd__(self: Self, other: bytearray, Self: type)
```

```python
__iadd__(self: Self, other: bytes, Self: type)
```

\_\_mul\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/#class-bytearray-__mul__ "Copy/link to this API element")

```python
__mul__(self: Self, count: int, Self: type)
```

\_\_rmul\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/#class-bytearray-__rmul__ "Copy/link to this API element")

```python
__rmul__(self: Self, count: int, Self: type)
```

\_\_imul\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/#class-bytearray-__imul__ "Copy/link to this API element")

```python
__imul__(self: Self, count: int, Self: type)
```

class bytes[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/#class-bytes "Copy/link to this API element")

@extend

```python
class bytes
```

Extends [`bytes`](https://docs.exaloop.io/libraries/api/classes/bytes/) · [View complete class API →](https://docs.exaloop.io/libraries/api/classes/bytes/)

Special methods 7

\_\_eq\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/#class-bytes-__eq__ "Copy/link to this API element")

```python
__eq__(self: Self, other: bytearray, Self: type)
```

\_\_ne\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/#class-bytes-__ne__ "Copy/link to this API element")

```python
__ne__(self: Self, other: bytearray, Self: type)
```

\_\_add\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/#class-bytes-__add__ "Copy/link to this API element")

```python
__add__(self: Self, other: bytearray, Self: type)
```

\_\_lt\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/#class-bytes-__lt__ "Copy/link to this API element")

```python
__lt__(self: Self, other: bytearray, Self: type)
```

\_\_le\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/#class-bytes-__le__ "Copy/link to this API element")

```python
__le__(self: Self, other: bytearray, Self: type)
```

\_\_gt\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/#class-bytes-__gt__ "Copy/link to this API element")

```python
__gt__(self: Self, other: bytearray, Self: type)
```

\_\_ge\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/#class-bytes-__ge__ "Copy/link to this API element")

```python
__ge__(self: Self, other: bytearray, Self: type)
```

Source: https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/
