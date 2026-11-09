# bytearray

Standard library class `internal.types.collections.bytearray.bytearray` [View source](https://github.com/exaloop/codon/blob/master/stdlib/internal/types/collections/bytearray.codon)

class bytearray[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/bytearray/bytearray/#class-bytearray "Copy/link to this API element")

```python
class bytearray
```

Constructors 6

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/bytearray/bytearray/#class-bytearray-__init__ "Copy/link to this API element")

```python
__init__(self: Self, Self: type)
```

Defined in [`internal.types.collections.bytearray`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/)

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/bytearray/bytearray/#class-bytearray-__init__-overload-2 "Copy/link to this API element")

```python
__init__(self: Self, size: int, Self: type)
```

Defined in [`internal.types.collections.bytearray`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/)

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/bytearray/bytearray/#class-bytearray-__init__-overload-3 "Copy/link to this API element")

```python
__init__(self: Self, source: bytes, Self: type)
```

Defined in [`internal.types.collections.bytearray`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/)

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/bytearray/bytearray/#class-bytearray-__init__-overload-4 "Copy/link to this API element")

```python
__init__(self: Self, source: bytearray, Self: type)
```

Defined in [`internal.types.collections.bytearray`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/)

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/bytearray/bytearray/#class-bytearray-__init__-overload-5 "Copy/link to this API element")

```python
__init__(self: Self, source: Generator[int], Self: type)
```

Defined in [`internal.types.collections.bytearray`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/)

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/bytearray/bytearray/#class-bytearray-__init__-overload-6 "Copy/link to this API element")

```python
__init__(self: Self, source: str, encoding: str, errors: str = "strict", Self: type)
```

Defined in [`internal.types.collections.bytearray`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/)

Methods 56

fromhex[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/bytearray/bytearray/#class-bytearray-fromhex "Copy/link to this API element")

@staticmethod

```python
fromhex(value: str)
```

Defined in [`internal.types.collections.bytearray`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/)

```python
fromhex(value: bytes)
```

Defined in [`internal.types.collections.bytearray`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/)

```python
fromhex(value: bytearray)
```

Defined in [`internal.types.collections.bytearray`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/)

maketrans[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/bytearray/bytearray/#class-bytearray-maketrans "Copy/link to this API element")

@staticmethod

```python
maketrans(source, replacement)
```

Defined in [`internal.types.collections.bytearray`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/)

copy[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/bytearray/bytearray/#class-bytearray-copy "Copy/link to this API element")

```python
copy(self: Self, Self: type)
```

Defined in [`internal.types.collections.bytearray`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/)

append[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/bytearray/bytearray/#class-bytearray-append "Copy/link to this API element")

```python
append(self: Self, value: int, Self: type)
```

Defined in [`internal.types.collections.bytearray`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/)

extend[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/bytearray/bytearray/#class-bytearray-extend "Copy/link to this API element")

```python
extend(self: Self, source: Generator[int], Self: type)
```

Defined in [`internal.types.collections.bytearray`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/)

```python
extend(self: Self, source: bytes, Self: type)
```

Defined in [`internal.types.collections.bytearray`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/)

```python
extend(self: Self, source: bytearray, Self: type)
```

Defined in [`internal.types.collections.bytearray`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/)

insert[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/bytearray/bytearray/#class-bytearray-insert "Copy/link to this API element")

```python
insert(self: Self, index: int, value: int, Self: type)
```

Defined in [`internal.types.collections.bytearray`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/)

pop[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/bytearray/bytearray/#class-bytearray-pop "Copy/link to this API element")

```python
pop(self: Self, index: int = -1, Self: type)
```

Defined in [`internal.types.collections.bytearray`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/)

remove[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/bytearray/bytearray/#class-bytearray-remove "Copy/link to this API element")

```python
remove(self: Self, value: int, Self: type)
```

Defined in [`internal.types.collections.bytearray`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/)

clear[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/bytearray/bytearray/#class-bytearray-clear "Copy/link to this API element")

```python
clear(self: Self, Self: type)
```

Defined in [`internal.types.collections.bytearray`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/)

resize[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/bytearray/bytearray/#class-bytearray-resize "Copy/link to this API element")

```python
resize(self: Self, size: int, Self: type)
```

Defined in [`internal.types.collections.bytearray`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/)

reverse[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/bytearray/bytearray/#class-bytearray-reverse "Copy/link to this API element")

```python
reverse(self: Self, Self: type)
```

Defined in [`internal.types.collections.bytearray`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/)

count[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/bytearray/bytearray/#class-bytearray-count "Copy/link to this API element")

```python
count(self: Self, value: int, Self: type)
```

Defined in [`internal.types.collections.bytearray`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/)

```python
count(self: Self, pattern: bytes, start: int = 0, end: Optional[int] = None, Self: type)
```

Defined in [`internal.types.collections.bytearray`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/)

```python
count(self: Self, pattern: bytearray, start: int = 0, end: Optional[int] = None, Self: type)
```

Defined in [`internal.types.collections.bytearray`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/)

index[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/bytearray/bytearray/#class-bytearray-index "Copy/link to this API element")

```python
index(self: Self, value: int, start: int = 0, end: Optional[int] = None, Self: type)
```

Defined in [`internal.types.collections.bytearray`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/)

```python
index(self: Self, pattern: bytes, start: int = 0, end: Optional[int] = None, Self: type)
```

Defined in [`internal.types.collections.bytearray`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/)

```python
index(self: Self, pattern: bytearray, start: int = 0, end: Optional[int] = None, Self: type)
```

Defined in [`internal.types.collections.bytearray`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/)

find[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/bytearray/bytearray/#class-bytearray-find "Copy/link to this API element")

```python
find(self: Self, pattern: bytes, start: int = 0, end: Optional[int] = None, Self: type)
```

Defined in [`internal.types.collections.bytearray`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/)

```python
find(self: Self, pattern: bytearray, start: int = 0, end: Optional[int] = None, Self: type)
```

Defined in [`internal.types.collections.bytearray`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/)

rfind[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/bytearray/bytearray/#class-bytearray-rfind "Copy/link to this API element")

```python
rfind(self: Self, pattern: bytes, start: int = 0, end: Optional[int] = None, Self: type)
```

Defined in [`internal.types.collections.bytearray`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/)

```python
rfind(self: Self, pattern: bytearray, start: int = 0, end: Optional[int] = None, Self: type)
```

Defined in [`internal.types.collections.bytearray`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/)

rindex[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/bytearray/bytearray/#class-bytearray-rindex "Copy/link to this API element")

```python
rindex(self: Self, pattern: bytes, start: int = 0, end: Optional[int] = None, Self: type)
```

Defined in [`internal.types.collections.bytearray`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/)

```python
rindex(self: Self, pattern: bytearray, start: int = 0, end: Optional[int] = None, Self: type)
```

Defined in [`internal.types.collections.bytearray`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/)

startswith[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/bytearray/bytearray/#class-bytearray-startswith "Copy/link to this API element")

```python
startswith(self: Self, prefix: bytes, start: int = 0, end: Optional[int] = None, Self: type)
```

Defined in [`internal.types.collections.bytearray`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/)

```python
startswith(self: Self, prefix: bytearray, start: int = 0, end: Optional[int] = None, Self: type)
```

Defined in [`internal.types.collections.bytearray`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/)

endswith[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/bytearray/bytearray/#class-bytearray-endswith "Copy/link to this API element")

```python
endswith(self: Self, suffix: bytes, start: int = 0, end: Optional[int] = None, Self: type)
```

Defined in [`internal.types.collections.bytearray`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/)

```python
endswith(self: Self, suffix: bytearray, start: int = 0, end: Optional[int] = None, Self: type)
```

Defined in [`internal.types.collections.bytearray`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/)

removeprefix[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/bytearray/bytearray/#class-bytearray-removeprefix "Copy/link to this API element")

```python
removeprefix(self: Self, prefix, Self: type)
```

Defined in [`internal.types.collections.bytearray`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/)

removesuffix[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/bytearray/bytearray/#class-bytearray-removesuffix "Copy/link to this API element")

```python
removesuffix(self: Self, suffix, Self: type)
```

Defined in [`internal.types.collections.bytearray`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/)

isdigit[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/bytearray/bytearray/#class-bytearray-isdigit "Copy/link to this API element")

```python
isdigit(self: Self, Self: type)
```

Defined in [`internal.types.collections.bytearray`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/)

islower[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/bytearray/bytearray/#class-bytearray-islower "Copy/link to this API element")

```python
islower(self: Self, Self: type)
```

Defined in [`internal.types.collections.bytearray`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/)

isupper[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/bytearray/bytearray/#class-bytearray-isupper "Copy/link to this API element")

```python
isupper(self: Self, Self: type)
```

Defined in [`internal.types.collections.bytearray`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/)

isalnum[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/bytearray/bytearray/#class-bytearray-isalnum "Copy/link to this API element")

```python
isalnum(self: Self, Self: type)
```

Defined in [`internal.types.collections.bytearray`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/)

isalpha[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/bytearray/bytearray/#class-bytearray-isalpha "Copy/link to this API element")

```python
isalpha(self: Self, Self: type)
```

Defined in [`internal.types.collections.bytearray`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/)

isspace[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/bytearray/bytearray/#class-bytearray-isspace "Copy/link to this API element")

```python
isspace(self: Self, Self: type)
```

Defined in [`internal.types.collections.bytearray`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/)

istitle[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/bytearray/bytearray/#class-bytearray-istitle "Copy/link to this API element")

```python
istitle(self: Self, Self: type)
```

Defined in [`internal.types.collections.bytearray`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/)

isdecimal[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/bytearray/bytearray/#class-bytearray-isdecimal "Copy/link to this API element")

```python
isdecimal(self: Self, Self: type)
```

Defined in [`internal.types.collections.bytearray`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/)

isascii[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/bytearray/bytearray/#class-bytearray-isascii "Copy/link to this API element")

```python
isascii(self: Self, Self: type)
```

Defined in [`internal.types.collections.bytearray`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/)

isnumeric[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/bytearray/bytearray/#class-bytearray-isnumeric "Copy/link to this API element")

```python
isnumeric(self: Self, Self: type)
```

Defined in [`internal.types.collections.bytearray`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/)

isidentifier[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/bytearray/bytearray/#class-bytearray-isidentifier "Copy/link to this API element")

```python
isidentifier(self: Self, Self: type)
```

Defined in [`internal.types.collections.bytearray`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/)

isprintable[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/bytearray/bytearray/#class-bytearray-isprintable "Copy/link to this API element")

```python
isprintable(self: Self, Self: type)
```

Defined in [`internal.types.collections.bytearray`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/)

capitalize[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/bytearray/bytearray/#class-bytearray-capitalize "Copy/link to this API element")

```python
capitalize(self: Self, Self: type)
```

Defined in [`internal.types.collections.bytearray`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/)

lower[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/bytearray/bytearray/#class-bytearray-lower "Copy/link to this API element")

```python
lower(self: Self, Self: type)
```

Defined in [`internal.types.collections.bytearray`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/)

upper[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/bytearray/bytearray/#class-bytearray-upper "Copy/link to this API element")

```python
upper(self: Self, Self: type)
```

Defined in [`internal.types.collections.bytearray`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/)

casefold[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/bytearray/bytearray/#class-bytearray-casefold "Copy/link to this API element")

```python
casefold(self: Self, Self: type)
```

Defined in [`internal.types.collections.bytearray`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/)

swapcase[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/bytearray/bytearray/#class-bytearray-swapcase "Copy/link to this API element")

```python
swapcase(self: Self, Self: type)
```

Defined in [`internal.types.collections.bytearray`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/)

title[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/bytearray/bytearray/#class-bytearray-title "Copy/link to this API element")

```python
title(self: Self, Self: type)
```

Defined in [`internal.types.collections.bytearray`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/)

ljust[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/bytearray/bytearray/#class-bytearray-ljust "Copy/link to this API element")

```python
ljust(self: Self, width: int, fillchar: bytes = " ", Self: type)
```

Defined in [`internal.types.collections.bytearray`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/)

rjust[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/bytearray/bytearray/#class-bytearray-rjust "Copy/link to this API element")

```python
rjust(self: Self, width: int, fillchar: bytes = " ", Self: type)
```

Defined in [`internal.types.collections.bytearray`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/)

center[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/bytearray/bytearray/#class-bytearray-center "Copy/link to this API element")

```python
center(self: Self, width: int, fillchar: bytes = " ", Self: type)
```

Defined in [`internal.types.collections.bytearray`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/)

zfill[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/bytearray/bytearray/#class-bytearray-zfill "Copy/link to this API element")

```python
zfill(self: Self, width: int, Self: type)
```

Defined in [`internal.types.collections.bytearray`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/)

lstrip[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/bytearray/bytearray/#class-bytearray-lstrip "Copy/link to this API element")

```python
lstrip(self: Self, Self: type)
```

Defined in [`internal.types.collections.bytearray`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/)

```python
lstrip(self: Self, chars: bytes, Self: type)
```

Defined in [`internal.types.collections.bytearray`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/)

rstrip[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/bytearray/bytearray/#class-bytearray-rstrip "Copy/link to this API element")

```python
rstrip(self: Self, Self: type)
```

Defined in [`internal.types.collections.bytearray`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/)

```python
rstrip(self: Self, chars: bytes, Self: type)
```

Defined in [`internal.types.collections.bytearray`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/)

strip[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/bytearray/bytearray/#class-bytearray-strip "Copy/link to this API element")

```python
strip(self: Self, Self: type)
```

Defined in [`internal.types.collections.bytearray`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/)

```python
strip(self: Self, chars: bytes, Self: type)
```

Defined in [`internal.types.collections.bytearray`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/)

partition[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/bytearray/bytearray/#class-bytearray-partition "Copy/link to this API element")

```python
partition(self: Self, separator: bytes, Self: type)
```

Defined in [`internal.types.collections.bytearray`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/)

rpartition[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/bytearray/bytearray/#class-bytearray-rpartition "Copy/link to this API element")

```python
rpartition(self: Self, separator: bytes, Self: type)
```

Defined in [`internal.types.collections.bytearray`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/)

split[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/bytearray/bytearray/#class-bytearray-split "Copy/link to this API element")

```python
split(self: Self, separator: Optional[bytes] = None, maxsplit: int = -1, Self: type)
```

Defined in [`internal.types.collections.bytearray`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/)

rsplit[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/bytearray/bytearray/#class-bytearray-rsplit "Copy/link to this API element")

```python
rsplit(self: Self, separator: Optional[bytes] = None, maxsplit: int = -1, Self: type)
```

Defined in [`internal.types.collections.bytearray`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/)

splitlines[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/bytearray/bytearray/#class-bytearray-splitlines "Copy/link to this API element")

```python
splitlines(self: Self, keepends: bool = False, Self: type)
```

Defined in [`internal.types.collections.bytearray`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/)

replace[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/bytearray/bytearray/#class-bytearray-replace "Copy/link to this API element")

```python
replace(self: Self, old: bytes, new: bytes, count: int = -1, Self: type)
```

Defined in [`internal.types.collections.bytearray`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/)

expandtabs[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/bytearray/bytearray/#class-bytearray-expandtabs "Copy/link to this API element")

```python
expandtabs(self: Self, tabsize: int = 8, Self: type)
```

Defined in [`internal.types.collections.bytearray`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/)

translate[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/bytearray/bytearray/#class-bytearray-translate "Copy/link to this API element")

```python
translate(self: Self, table, Self: type)
```

Defined in [`internal.types.collections.bytearray`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/)

```python
translate(self: Self, table: bytes, delete: bytes = "", Self: type)
```

Defined in [`internal.types.collections.bytearray`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/)

```python
translate(self: Self, table: bytearray, delete: bytes = "", Self: type)
```

Defined in [`internal.types.collections.bytearray`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/)

hex[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/bytearray/bytearray/#class-bytearray-hex "Copy/link to this API element")

```python
hex(self: Self, Self: type)
```

Defined in [`internal.types.collections.bytearray`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/)

```python
hex(self: Self, sep: str, bytes_per_sep: int = 1, Self: type)
```

Defined in [`internal.types.collections.bytearray`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/)

```python
hex(self: Self, sep: bytes, bytes_per_sep: int = 1, Self: type)
```

Defined in [`internal.types.collections.bytearray`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/)

join[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/bytearray/bytearray/#class-bytearray-join "Copy/link to this API element")

```python
join(self: Self, values: Generator[T], T: type, Self: type)
```

Defined in [`internal.types.collections.bytearray`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/)

```python
join(self: Self, values: List[T], T: type, Self: type)
```

Defined in [`internal.types.collections.bytearray`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/)

decode[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/bytearray/bytearray/#class-bytearray-decode "Copy/link to this API element")

```python
decode(self: Self, encoding: str = "utf-8", errors: str = "strict", Self: type)
```

Defined in [`internal.types.collections.bytearray`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/)

Special methods 24

\_\_len\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/bytearray/bytearray/#class-bytearray-__len__ "Copy/link to this API element")

```python
__len__(self: Self, Self: type)
```

Defined in [`internal.types.collections.bytearray`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/)

\_\_bool\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/bytearray/bytearray/#class-bytearray-__bool__ "Copy/link to this API element")

```python
__bool__(self: Self, Self: type)
```

Defined in [`internal.types.collections.bytearray`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/)

\_\_iter\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/bytearray/bytearray/#class-bytearray-__iter__ "Copy/link to this API element")

```python
__iter__(self: Self, Self: type)
```

Defined in [`internal.types.collections.bytearray`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/)

\_\_reversed\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/bytearray/bytearray/#class-bytearray-__reversed__ "Copy/link to this API element")

```python
__reversed__(self: Self, Self: type)
```

Defined in [`internal.types.collections.bytearray`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/)

\_\_getitem\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/bytearray/bytearray/#class-bytearray-__getitem__ "Copy/link to this API element")

```python
__getitem__(self: Self, index: int, Self: type)
```

Defined in [`internal.types.collections.bytearray`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/)

```python
__getitem__(self: Self, slice: Slice, Self: type)
```

Defined in [`internal.types.collections.bytearray`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/)

\_\_setitem\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/bytearray/bytearray/#class-bytearray-__setitem__ "Copy/link to this API element")

```python
__setitem__(self: Self, slice: Slice, source: Generator[int], Self: type)
```

Defined in [`internal.types.collections.bytearray`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/)

```python
__setitem__(self: Self, index: int, value: int, Self: type)
```

Defined in [`internal.types.collections.bytearray`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/)

```python
__setitem__(self: Self, slice: Slice, source: bytes, Self: type)
```

Defined in [`internal.types.collections.bytearray`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/)

```python
__setitem__(self: Self, slice: Slice, source: bytearray, Self: type)
```

Defined in [`internal.types.collections.bytearray`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/)

\_\_delitem\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/bytearray/bytearray/#class-bytearray-__delitem__ "Copy/link to this API element")

```python
__delitem__(self: Self, index: int, Self: type)
```

Defined in [`internal.types.collections.bytearray`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/)

```python
__delitem__(self: Self, slice: Slice, Self: type)
```

Defined in [`internal.types.collections.bytearray`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/)

\_\_contains\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/bytearray/bytearray/#class-bytearray-__contains__ "Copy/link to this API element")

```python
__contains__(self: Self, value: int, Self: type)
```

Defined in [`internal.types.collections.bytearray`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/)

```python
__contains__(self: Self, pattern: bytes, Self: type)
```

Defined in [`internal.types.collections.bytearray`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/)

```python
__contains__(self: Self, pattern: bytearray, Self: type)
```

Defined in [`internal.types.collections.bytearray`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/)

\_\_bytes\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/bytearray/bytearray/#class-bytearray-__bytes__ "Copy/link to this API element")

```python
__bytes__(self: Self, Self: type)
```

Defined in [`internal.types.collections.bytearray`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/)

\_\_str\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/bytearray/bytearray/#class-bytearray-__str__ "Copy/link to this API element")

```python
__str__(self: Self, Self: type)
```

Defined in [`internal.types.collections.bytearray`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/)

\_\_repr\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/bytearray/bytearray/#class-bytearray-__repr__ "Copy/link to this API element")

```python
__repr__(self: Self, Self: type)
```

Defined in [`internal.types.collections.bytearray`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/)

\_\_copy\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/bytearray/bytearray/#class-bytearray-__copy__ "Copy/link to this API element")

```python
__copy__(self: Self, Self: type)
```

Defined in [`internal.types.collections.bytearray`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/)

\_\_deepcopy\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/bytearray/bytearray/#class-bytearray-__deepcopy__ "Copy/link to this API element")

```python
__deepcopy__(self: Self, Self: type)
```

Defined in [`internal.types.collections.bytearray`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/)

\_\_eq\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/bytearray/bytearray/#class-bytearray-__eq__ "Copy/link to this API element")

```python
__eq__(self: Self, other: bytearray, Self: type)
```

Defined in [`internal.types.collections.bytearray`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/)

```python
__eq__(self: Self, other: bytes, Self: type)
```

Defined in [`internal.types.collections.bytearray`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/)

\_\_ne\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/bytearray/bytearray/#class-bytearray-__ne__ "Copy/link to this API element")

```python
__ne__(self: Self, other: bytearray, Self: type)
```

Defined in [`internal.types.collections.bytearray`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/)

```python
__ne__(self: Self, other: bytes, Self: type)
```

Defined in [`internal.types.collections.bytearray`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/)

\_\_lt\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/bytearray/bytearray/#class-bytearray-__lt__ "Copy/link to this API element")

```python
__lt__(self: Self, other: bytearray, Self: type)
```

Defined in [`internal.types.collections.bytearray`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/)

```python
__lt__(self: Self, other: bytes, Self: type)
```

Defined in [`internal.types.collections.bytearray`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/)

\_\_le\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/bytearray/bytearray/#class-bytearray-__le__ "Copy/link to this API element")

```python
__le__(self: Self, other: bytearray, Self: type)
```

Defined in [`internal.types.collections.bytearray`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/)

```python
__le__(self: Self, other: bytes, Self: type)
```

Defined in [`internal.types.collections.bytearray`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/)

\_\_gt\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/bytearray/bytearray/#class-bytearray-__gt__ "Copy/link to this API element")

```python
__gt__(self: Self, other: bytearray, Self: type)
```

Defined in [`internal.types.collections.bytearray`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/)

```python
__gt__(self: Self, other: bytes, Self: type)
```

Defined in [`internal.types.collections.bytearray`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/)

\_\_ge\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/bytearray/bytearray/#class-bytearray-__ge__ "Copy/link to this API element")

```python
__ge__(self: Self, other: bytearray, Self: type)
```

Defined in [`internal.types.collections.bytearray`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/)

```python
__ge__(self: Self, other: bytes, Self: type)
```

Defined in [`internal.types.collections.bytearray`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/)

\_\_add\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/bytearray/bytearray/#class-bytearray-__add__ "Copy/link to this API element")

```python
__add__(self: Self, other: bytearray, Self: type)
```

Defined in [`internal.types.collections.bytearray`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/)

```python
__add__(self: Self, other: bytes, Self: type)
```

Defined in [`internal.types.collections.bytearray`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/)

\_\_iadd\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/bytearray/bytearray/#class-bytearray-__iadd__ "Copy/link to this API element")

```python
__iadd__(self: Self, other: bytearray, Self: type)
```

Defined in [`internal.types.collections.bytearray`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/)

```python
__iadd__(self: Self, other: bytes, Self: type)
```

Defined in [`internal.types.collections.bytearray`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/)

\_\_mul\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/bytearray/bytearray/#class-bytearray-__mul__ "Copy/link to this API element")

```python
__mul__(self: Self, count: int, Self: type)
```

Defined in [`internal.types.collections.bytearray`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/)

\_\_rmul\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/bytearray/bytearray/#class-bytearray-__rmul__ "Copy/link to this API element")

```python
__rmul__(self: Self, count: int, Self: type)
```

Defined in [`internal.types.collections.bytearray`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/)

\_\_imul\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/types/collections/bytearray/bytearray/#class-bytearray-__imul__ "Copy/link to this API element")

```python
__imul__(self: Self, count: int, Self: type)
```

Defined in [`internal.types.collections.bytearray`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/)

Source: https://docs.exaloop.io/libraries/api/classes/internal/types/collections/bytearray/bytearray/
