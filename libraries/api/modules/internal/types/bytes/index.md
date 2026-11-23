# internal.types.bytes

Standard library Module [View source](https://github.com/exaloop/codon/blob/master/stdlib/internal/types/bytes.codon)

class bytes[](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/#class-bytes "Copy/link to this API element")

@extend

```python
class bytes
```

Extends [`internal.core.bytes`](https://docs.exaloop.io/libraries/api/classes/internal/core/bytes/) · [View complete class API →](https://docs.exaloop.io/libraries/api/classes/internal/core/bytes/)

Constructors 5

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/#class-bytes-__new__ "Copy/link to this API element")

@\_\_internal\_\_

```python
__new__(p: Ptr[u8], l: int) -> bytes
```

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/#class-bytes-__new__-overload-2 "Copy/link to this API element")

@overload

```python
__new__() -> bytes
```

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/#class-bytes-__new__-overload-3 "Copy/link to this API element")

@overload

```python
__new__(what) -> bytes
```

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/#class-bytes-__new__-overload-4 "Copy/link to this API element")

@overload

```python
__new__(what: bytes) -> bytes
```

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/#class-bytes-__new__-overload-5 "Copy/link to this API element")

```python
__new__(what: str, encoding: str, errors: str = "strict") -> bytes
```

Methods 7

decode[](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/#class-bytes-decode "Copy/link to this API element")

```python
decode(self: Self, /, encoding: str = "utf-8", errors: str = "strict") -> str
```

memcpy[](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/#class-bytes-memcpy "Copy/link to this API element")

@llvm

```python
memcpy(dest: Ptr[u8], src: Ptr[u8], len: int) -> None
```

```llvm
declare void @llvm.memcpy.p0i8.p0i8.i64(ptr %dest, ptr %src, i64 %len, i32 %align, i1 %isvolatile)
call void @llvm.memcpy.p0i8.p0i8.i64(ptr %dest, ptr %src, i64 %len, i32 0, i1 false)
ret {} {}
```

memmove[](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/#class-bytes-memmove "Copy/link to this API element")

@llvm

```python
memmove(dest: Ptr[u8], src: Ptr[u8], len: int) -> None
```

```llvm
declare void @llvm.memmove.p0i8.p0i8.i64(ptr %dest, ptr %src, i64 %len, i32 %align, i1 %isvolatile)
call void @llvm.memmove.p0i8.p0i8.i64(ptr %dest, ptr %src, i64 %len, i32 0, i1 false)
ret {} {}
```

memset[](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/#class-bytes-memset "Copy/link to this API element")

@llvm

```python
memset(dest: Ptr[u8], val: u8, len: int) -> None
```

```llvm
declare void @llvm.memset.p0i8.i64(ptr %dest, i8 %val, i64 %len, i32 %align, i1 %isvolatile)
call void @llvm.memset.p0i8.i64(ptr %dest, i8 %val, i64 %len, i32 0, i1 false)
ret {} {}
```

c\_str[](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/#class-bytes-c_str "Copy/link to this API element")

```python
c_str(self: Self) -> cobj
```

from\_ptr[](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/#class-bytes-from_ptr "Copy/link to this API element")

```python
from_ptr(t: cobj) -> bytes
```

cat[](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/#class-bytes-cat "Copy/link to this API element")

```python
cat(*args) -> bytes
```

Special methods 22

\_\_str\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/#class-bytes-__str__ "Copy/link to this API element")

```python
__str__(self: Self, /) -> str
```

\_\_bytes\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/#class-bytes-__bytes__ "Copy/link to this API element")

```python
__bytes__(self: Self, /) -> bytes
```

\_\_len\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/#class-bytes-__len__ "Copy/link to this API element")

```python
__len__(self: Self, /) -> int
```

\_\_bool\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/#class-bytes-__bool__ "Copy/link to this API element")

```python
__bool__(self: Self) -> bool
```

\_\_copy\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/#class-bytes-__copy__ "Copy/link to this API element")

```python
__copy__(self: Self) -> bytes
```

\_\_deepcopy\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/#class-bytes-__deepcopy__ "Copy/link to this API element")

```python
__deepcopy__(self: Self) -> bytes
```

\_\_ptrcopy\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/#class-bytes-__ptrcopy__ "Copy/link to this API element")

```python
__ptrcopy__(self: Self) -> bytes
```

\_\_add\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/#class-bytes-__add__ "Copy/link to this API element")

```python
__add__(self: Self, other: bytes, /) -> bytes
```

\_\_eq\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/#class-bytes-__eq__ "Copy/link to this API element")

```python
__eq__(self: Self, other: bytes, /) -> bool
```

\_\_match\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/#class-bytes-__match__ "Copy/link to this API element")

```python
__match__(self: Self, obj: bytes) -> bool
```

\_\_ne\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/#class-bytes-__ne__ "Copy/link to this API element")

```python
__ne__(self: Self, other: bytes, /) -> bool
```

\_\_hash\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/#class-bytes-__hash__ "Copy/link to this API element")

```python
__hash__(self: Self, /) -> int
```

\_\_lt\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/#class-bytes-__lt__ "Copy/link to this API element")

```python
__lt__(self: Self, other: bytes, /) -> bool
```

\_\_le\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/#class-bytes-__le__ "Copy/link to this API element")

```python
__le__(self: Self, other: bytes, /) -> bool
```

\_\_gt\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/#class-bytes-__gt__ "Copy/link to this API element")

```python
__gt__(self: Self, other: bytes, /) -> bool
```

\_\_ge\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/#class-bytes-__ge__ "Copy/link to this API element")

```python
__ge__(self: Self, other: bytes, /) -> bool
```

\_\_repr\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/#class-bytes-__repr__ "Copy/link to this API element")

```python
__repr__(self: Self, /) -> str
```

\_\_getitem\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/#class-bytes-__getitem__ "Copy/link to this API element")

```python
__getitem__(self: Self, idx: int, /) -> int
```

```python
__getitem__(self: Self, s: Slice, /) -> bytes
```

\_\_iter\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/#class-bytes-__iter__ "Copy/link to this API element")

```python
__iter__(self: Self, /) -> Generator[int]
```

\_\_reversed\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/#class-bytes-__reversed__ "Copy/link to this API element")

```python
__reversed__(self: Self) -> Generator[int]
```

\_\_mul\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/#class-bytes-__mul__ "Copy/link to this API element")

```python
__mul__(self: Self, x: int, /) -> bytes
```

\_\_rmul\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/#class-bytes-__rmul__ "Copy/link to this API element")

```python
__rmul__(self: Self, x: int, /) -> bytes
```

class bytes[](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/#class-bytes "Copy/link to this API element")

@extend

```python
class bytes
```

Extends [`internal.core.bytes`](https://docs.exaloop.io/libraries/api/classes/internal/core/bytes/) · [View complete class API →](https://docs.exaloop.io/libraries/api/classes/internal/core/bytes/)

Methods 46

fromhex[](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/#class-bytes-fromhex "Copy/link to this API element")

@staticmethod

```python
fromhex(value, /) -> bytes
```

```python
fromhex(value: str, /) -> bytes
```

```python
fromhex(value: bytes, /) -> bytes
```

hex[](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/#class-bytes-hex "Copy/link to this API element")

```python
hex(self: Self, /) -> str
```

```python
hex(self: Self, /, sep: str, bytes_per_sep: int = 1) -> str
```

```python
hex(self: Self, /, sep: bytes, bytes_per_sep: int = 1) -> str
```

maketrans[](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/#class-bytes-maketrans "Copy/link to this API element")

@staticmethod

```python
maketrans(source, replacement, /) -> bytes
```

join[](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/#class-bytes-join "Copy/link to this API element")

```python
join(self: Self, l: Generator[bytes], /) -> bytes
```

```python
join(self: Self, l: List[bytes], /) -> bytes
```

isdigit[](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/#class-bytes-isdigit "Copy/link to this API element")

```python
isdigit(self: Self, /) -> bool
```

islower[](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/#class-bytes-islower "Copy/link to this API element")

```python
islower(self: Self, /) -> bool
```

isupper[](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/#class-bytes-isupper "Copy/link to this API element")

```python
isupper(self: Self, /) -> bool
```

isalnum[](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/#class-bytes-isalnum "Copy/link to this API element")

```python
isalnum(self: Self, /) -> bool
```

isalpha[](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/#class-bytes-isalpha "Copy/link to this API element")

```python
isalpha(self: Self, /) -> bool
```

isspace[](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/#class-bytes-isspace "Copy/link to this API element")

```python
isspace(self: Self, /) -> bool
```

istitle[](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/#class-bytes-istitle "Copy/link to this API element")

```python
istitle(self: Self, /) -> bool
```

capitalize[](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/#class-bytes-capitalize "Copy/link to this API element")

```python
capitalize(self: Self, /) -> bytes
```

isdecimal[](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/#class-bytes-isdecimal "Copy/link to this API element")

```python
isdecimal(self: Self) -> bool
```

lower[](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/#class-bytes-lower "Copy/link to this API element")

```python
lower(self: Self, /) -> bytes
```

upper[](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/#class-bytes-upper "Copy/link to this API element")

```python
upper(self: Self, /) -> bytes
```

isascii[](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/#class-bytes-isascii "Copy/link to this API element")

```python
isascii(self: Self, /) -> bool
```

casefold[](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/#class-bytes-casefold "Copy/link to this API element")

```python
casefold(self: Self) -> bytes
```

swapcase[](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/#class-bytes-swapcase "Copy/link to this API element")

```python
swapcase(self: Self, /) -> bytes
```

title[](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/#class-bytes-title "Copy/link to this API element")

```python
title(self: Self, /) -> bytes
```

isnumeric[](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/#class-bytes-isnumeric "Copy/link to this API element")

```python
isnumeric(self: Self) -> bool
```

ljust[](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/#class-bytes-ljust "Copy/link to this API element")

```python
ljust(self: Self, width: int, fillchar: bytes = " ", /) -> bytes
```

rjust[](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/#class-bytes-rjust "Copy/link to this API element")

```python
rjust(self: Self, width: int, fillchar: bytes = " ", /) -> bytes
```

center[](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/#class-bytes-center "Copy/link to this API element")

```python
center(self: Self, width: int, fillchar: bytes = " ", /) -> bytes
```

zfill[](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/#class-bytes-zfill "Copy/link to this API element")

```python
zfill(self: Self, width: int, /) -> bytes
```

count[](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/#class-bytes-count "Copy/link to this API element")

```python
count(self: Self, sub: int, start: Optional[int] = 0, end: Optional[int] = None, /) -> int
```

```python
count(self: Self, sub: bytes, start: Optional[int] = 0, end: Optional[int] = None, /) -> int
```

find[](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/#class-bytes-find "Copy/link to this API element")

```python
find(self: Self, sub: int, start: Optional[int] = 0, end: Optional[int] = None, /) -> int
```

```python
find(self: Self, sub: bytes, start: Optional[int] = 0, end: Optional[int] = None, /) -> int
```

rfind[](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/#class-bytes-rfind "Copy/link to this API element")

```python
rfind(self: Self, sub: int, start: Optional[int] = 0, end: Optional[int] = None, /) -> int
```

```python
rfind(self: Self, sub: bytes, start: Optional[int] = 0, end: Optional[int] = None, /) -> int
```

isidentifier[](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/#class-bytes-isidentifier "Copy/link to this API element")

```python
isidentifier(self: Self) -> bool
```

isprintable[](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/#class-bytes-isprintable "Copy/link to this API element")

```python
isprintable(self: Self) -> bool
```

lstrip[](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/#class-bytes-lstrip "Copy/link to this API element")

```python
lstrip(self: Self, chars: Optional[bytes] = None, /) -> bytes
```

rstrip[](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/#class-bytes-rstrip "Copy/link to this API element")

```python
rstrip(self: Self, chars: Optional[bytes] = None, /) -> bytes
```

strip[](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/#class-bytes-strip "Copy/link to this API element")

```python
strip(self: Self, chars: Optional[bytes] = None, /) -> bytes
```

partition[](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/#class-bytes-partition "Copy/link to this API element")

```python
partition(self: Self, sep: bytes, /) -> Tuple[(bytes, bytes, bytes)]
```

rpartition[](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/#class-bytes-rpartition "Copy/link to this API element")

```python
rpartition(self: Self, sep: bytes, /) -> Tuple[(bytes, bytes, bytes)]
```

split[](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/#class-bytes-split "Copy/link to this API element")

```python
split(self: Self, /, sep: Optional[bytes] = None, maxsplit: int = -1) -> List[bytes]
```

rsplit[](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/#class-bytes-rsplit "Copy/link to this API element")

```python
rsplit(self: Self, /, sep: Optional[bytes] = None, maxsplit: int = -1) -> List[bytes]
```

splitlines[](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/#class-bytes-splitlines "Copy/link to this API element")

```python
splitlines(self: Self, /, keepends: bool = False) -> List[bytes]
```

startswith[](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/#class-bytes-startswith "Copy/link to this API element")

```python
startswith(self: Self, prefix, start: Optional[int] = 0, end: Optional[int] = None, /) -> bool
```

```python
startswith(self: Self, prefix: bytes, start: Optional[int] = 0, end: Optional[int] = None, /) -> bool
```

endswith[](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/#class-bytes-endswith "Copy/link to this API element")

```python
endswith(self: Self, suffix, start: Optional[int] = 0, end: Optional[int] = None, /) -> bool
```

```python
endswith(self: Self, suffix: bytes, start: Optional[int] = 0, end: Optional[int] = None, /) -> bool
```

removeprefix[](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/#class-bytes-removeprefix "Copy/link to this API element")

```python
removeprefix(self: Self, prefix, /) -> bytes
```

removesuffix[](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/#class-bytes-removesuffix "Copy/link to this API element")

```python
removesuffix(self: Self, suffix, /) -> bytes
```

index[](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/#class-bytes-index "Copy/link to this API element")

```python
index(self: Self, sub, start: Optional[int] = 0, end: Optional[int] = None, /) -> int
```

rindex[](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/#class-bytes-rindex "Copy/link to this API element")

```python
rindex(self: Self, sub, start: Optional[int] = 0, end: Optional[int] = None, /) -> int
```

replace[](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/#class-bytes-replace "Copy/link to this API element")

```python
replace(self: Self, old: bytes, new: bytes, maxcount: int = -1, /) -> bytes
```

expandtabs[](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/#class-bytes-expandtabs "Copy/link to this API element")

```python
expandtabs(self: Self, /, tabsize: int = 8) -> bytes
```

translate[](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/#class-bytes-translate "Copy/link to this API element")

```python
translate(self: Self, map, /) -> bytes
```

```python
translate(self: Self, table: bytes, /, delete: bytes = "") -> bytes
```

Special methods 1

\_\_contains\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/#class-bytes-__contains__ "Copy/link to this API element")

```python
__contains__(self: Self, pattern: bytes, /) -> bool
```

Source: https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/
