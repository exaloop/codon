# bytes

Standard library class `internal.core.bytes` [View source](https://github.com/exaloop/codon/blob/master/stdlib/internal/core.codon#L145)

class bytes[](https://docs.exaloop.io/libraries/api/classes/internal/core/bytes/#class-bytes "Copy/link to this API element")

@tuple

```python
class bytes
```

Constructors 5

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/core/bytes/#class-bytes-__new__ "Copy/link to this API element")

@\_\_internal\_\_

```python
__new__(p: Ptr[u8], l: int) -> bytes
```

Added by [`internal.types.bytes`](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/)

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/core/bytes/#class-bytes-__new__-overload-2 "Copy/link to this API element")

@overload

```python
__new__() -> bytes
```

Added by [`internal.types.bytes`](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/)

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/core/bytes/#class-bytes-__new__-overload-3 "Copy/link to this API element")

@overload

```python
__new__(what) -> bytes
```

Added by [`internal.types.bytes`](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/)

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/core/bytes/#class-bytes-__new__-overload-4 "Copy/link to this API element")

@overload

```python
__new__(what: bytes) -> bytes
```

Added by [`internal.types.bytes`](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/)

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/core/bytes/#class-bytes-__new__-overload-5 "Copy/link to this API element")

```python
__new__(what: str, encoding: str, errors: str = "strict") -> bytes
```

Added by [`internal.types.bytes`](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/)

Methods 53

decode[](https://docs.exaloop.io/libraries/api/classes/internal/core/bytes/#class-bytes-decode "Copy/link to this API element")

```python
decode(self: Self, /, encoding: str = "utf-8", errors: str = "strict") -> str
```

Added by [`internal.types.bytes`](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/)

memcpy[](https://docs.exaloop.io/libraries/api/classes/internal/core/bytes/#class-bytes-memcpy "Copy/link to this API element")

@llvm

```python
memcpy(dest: Ptr[u8], src: Ptr[u8], len: int) -> None
```

Added by [`internal.types.bytes`](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/)

```llvm
declare void @llvm.memcpy.p0i8.p0i8.i64(ptr %dest, ptr %src, i64 %len, i32 %align, i1 %isvolatile)
call void @llvm.memcpy.p0i8.p0i8.i64(ptr %dest, ptr %src, i64 %len, i32 0, i1 false)
ret {} {}
```

memmove[](https://docs.exaloop.io/libraries/api/classes/internal/core/bytes/#class-bytes-memmove "Copy/link to this API element")

@llvm

```python
memmove(dest: Ptr[u8], src: Ptr[u8], len: int) -> None
```

Added by [`internal.types.bytes`](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/)

```llvm
declare void @llvm.memmove.p0i8.p0i8.i64(ptr %dest, ptr %src, i64 %len, i32 %align, i1 %isvolatile)
call void @llvm.memmove.p0i8.p0i8.i64(ptr %dest, ptr %src, i64 %len, i32 0, i1 false)
ret {} {}
```

memset[](https://docs.exaloop.io/libraries/api/classes/internal/core/bytes/#class-bytes-memset "Copy/link to this API element")

@llvm

```python
memset(dest: Ptr[u8], val: u8, len: int) -> None
```

Added by [`internal.types.bytes`](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/)

```llvm
declare void @llvm.memset.p0i8.i64(ptr %dest, i8 %val, i64 %len, i32 %align, i1 %isvolatile)
call void @llvm.memset.p0i8.i64(ptr %dest, i8 %val, i64 %len, i32 0, i1 false)
ret {} {}
```

c\_str[](https://docs.exaloop.io/libraries/api/classes/internal/core/bytes/#class-bytes-c_str "Copy/link to this API element")

```python
c_str(self: Self) -> cobj
```

Added by [`internal.types.bytes`](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/)

from\_ptr[](https://docs.exaloop.io/libraries/api/classes/internal/core/bytes/#class-bytes-from_ptr "Copy/link to this API element")

```python
from_ptr(t: cobj) -> bytes
```

Added by [`internal.types.bytes`](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/)

cat[](https://docs.exaloop.io/libraries/api/classes/internal/core/bytes/#class-bytes-cat "Copy/link to this API element")

```python
cat(*args) -> bytes
```

Added by [`internal.types.bytes`](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/)

fromhex[](https://docs.exaloop.io/libraries/api/classes/internal/core/bytes/#class-bytes-fromhex "Copy/link to this API element")

@staticmethod

```python
fromhex(value, /) -> bytes
```

Added by [`internal.types.bytes`](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/)

```python
fromhex(value: str, /) -> bytes
```

Added by [`internal.types.bytes`](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/)

```python
fromhex(value: bytes, /) -> bytes
```

Added by [`internal.types.bytes`](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/)

hex[](https://docs.exaloop.io/libraries/api/classes/internal/core/bytes/#class-bytes-hex "Copy/link to this API element")

```python
hex(self: Self, /) -> str
```

Added by [`internal.types.bytes`](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/)

```python
hex(self: Self, /, sep: str, bytes_per_sep: int = 1) -> str
```

Added by [`internal.types.bytes`](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/)

```python
hex(self: Self, /, sep: bytes, bytes_per_sep: int = 1) -> str
```

Added by [`internal.types.bytes`](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/)

maketrans[](https://docs.exaloop.io/libraries/api/classes/internal/core/bytes/#class-bytes-maketrans "Copy/link to this API element")

@staticmethod

```python
maketrans(source, replacement, /) -> bytes
```

Added by [`internal.types.bytes`](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/)

join[](https://docs.exaloop.io/libraries/api/classes/internal/core/bytes/#class-bytes-join "Copy/link to this API element")

```python
join(self: Self, l: Generator[bytes], /) -> bytes
```

Added by [`internal.types.bytes`](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/)

```python
join(self: Self, l: List[bytes], /) -> bytes
```

Added by [`internal.types.bytes`](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/)

isdigit[](https://docs.exaloop.io/libraries/api/classes/internal/core/bytes/#class-bytes-isdigit "Copy/link to this API element")

```python
isdigit(self: Self, /) -> bool
```

Added by [`internal.types.bytes`](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/)

islower[](https://docs.exaloop.io/libraries/api/classes/internal/core/bytes/#class-bytes-islower "Copy/link to this API element")

```python
islower(self: Self, /) -> bool
```

Added by [`internal.types.bytes`](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/)

isupper[](https://docs.exaloop.io/libraries/api/classes/internal/core/bytes/#class-bytes-isupper "Copy/link to this API element")

```python
isupper(self: Self, /) -> bool
```

Added by [`internal.types.bytes`](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/)

isalnum[](https://docs.exaloop.io/libraries/api/classes/internal/core/bytes/#class-bytes-isalnum "Copy/link to this API element")

```python
isalnum(self: Self, /) -> bool
```

Added by [`internal.types.bytes`](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/)

isalpha[](https://docs.exaloop.io/libraries/api/classes/internal/core/bytes/#class-bytes-isalpha "Copy/link to this API element")

```python
isalpha(self: Self, /) -> bool
```

Added by [`internal.types.bytes`](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/)

isspace[](https://docs.exaloop.io/libraries/api/classes/internal/core/bytes/#class-bytes-isspace "Copy/link to this API element")

```python
isspace(self: Self, /) -> bool
```

Added by [`internal.types.bytes`](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/)

istitle[](https://docs.exaloop.io/libraries/api/classes/internal/core/bytes/#class-bytes-istitle "Copy/link to this API element")

```python
istitle(self: Self, /) -> bool
```

Added by [`internal.types.bytes`](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/)

capitalize[](https://docs.exaloop.io/libraries/api/classes/internal/core/bytes/#class-bytes-capitalize "Copy/link to this API element")

```python
capitalize(self: Self, /) -> bytes
```

Added by [`internal.types.bytes`](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/)

isdecimal[](https://docs.exaloop.io/libraries/api/classes/internal/core/bytes/#class-bytes-isdecimal "Copy/link to this API element")

```python
isdecimal(self: Self) -> bool
```

Added by [`internal.types.bytes`](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/)

lower[](https://docs.exaloop.io/libraries/api/classes/internal/core/bytes/#class-bytes-lower "Copy/link to this API element")

```python
lower(self: Self, /) -> bytes
```

Added by [`internal.types.bytes`](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/)

upper[](https://docs.exaloop.io/libraries/api/classes/internal/core/bytes/#class-bytes-upper "Copy/link to this API element")

```python
upper(self: Self, /) -> bytes
```

Added by [`internal.types.bytes`](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/)

isascii[](https://docs.exaloop.io/libraries/api/classes/internal/core/bytes/#class-bytes-isascii "Copy/link to this API element")

```python
isascii(self: Self, /) -> bool
```

Added by [`internal.types.bytes`](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/)

casefold[](https://docs.exaloop.io/libraries/api/classes/internal/core/bytes/#class-bytes-casefold "Copy/link to this API element")

```python
casefold(self: Self) -> bytes
```

Added by [`internal.types.bytes`](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/)

swapcase[](https://docs.exaloop.io/libraries/api/classes/internal/core/bytes/#class-bytes-swapcase "Copy/link to this API element")

```python
swapcase(self: Self, /) -> bytes
```

Added by [`internal.types.bytes`](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/)

title[](https://docs.exaloop.io/libraries/api/classes/internal/core/bytes/#class-bytes-title "Copy/link to this API element")

```python
title(self: Self, /) -> bytes
```

Added by [`internal.types.bytes`](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/)

isnumeric[](https://docs.exaloop.io/libraries/api/classes/internal/core/bytes/#class-bytes-isnumeric "Copy/link to this API element")

```python
isnumeric(self: Self) -> bool
```

Added by [`internal.types.bytes`](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/)

ljust[](https://docs.exaloop.io/libraries/api/classes/internal/core/bytes/#class-bytes-ljust "Copy/link to this API element")

```python
ljust(self: Self, width: int, fillchar: bytes = " ", /) -> bytes
```

Added by [`internal.types.bytes`](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/)

rjust[](https://docs.exaloop.io/libraries/api/classes/internal/core/bytes/#class-bytes-rjust "Copy/link to this API element")

```python
rjust(self: Self, width: int, fillchar: bytes = " ", /) -> bytes
```

Added by [`internal.types.bytes`](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/)

center[](https://docs.exaloop.io/libraries/api/classes/internal/core/bytes/#class-bytes-center "Copy/link to this API element")

```python
center(self: Self, width: int, fillchar: bytes = " ", /) -> bytes
```

Added by [`internal.types.bytes`](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/)

zfill[](https://docs.exaloop.io/libraries/api/classes/internal/core/bytes/#class-bytes-zfill "Copy/link to this API element")

```python
zfill(self: Self, width: int, /) -> bytes
```

Added by [`internal.types.bytes`](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/)

count[](https://docs.exaloop.io/libraries/api/classes/internal/core/bytes/#class-bytes-count "Copy/link to this API element")

```python
count(self: Self, sub: int, start: Optional[int] = 0, end: Optional[int] = None, /) -> int
```

Added by [`internal.types.bytes`](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/)

```python
count(self: Self, sub: bytes, start: Optional[int] = 0, end: Optional[int] = None, /) -> int
```

Added by [`internal.types.bytes`](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/)

```python
count(self: Self, sub: bytearray, start: Optional[int] = 0, end: Optional[int] = None, /) -> int
```

Added by [`internal.types.collections.bytearray`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/)

find[](https://docs.exaloop.io/libraries/api/classes/internal/core/bytes/#class-bytes-find "Copy/link to this API element")

```python
find(self: Self, sub: int, start: Optional[int] = 0, end: Optional[int] = None, /) -> int
```

Added by [`internal.types.bytes`](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/)

```python
find(self: Self, sub: bytes, start: Optional[int] = 0, end: Optional[int] = None, /) -> int
```

Added by [`internal.types.bytes`](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/)

```python
find(self: Self, sub: bytearray, start: Optional[int] = 0, end: Optional[int] = None, /) -> int
```

Added by [`internal.types.collections.bytearray`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/)

rfind[](https://docs.exaloop.io/libraries/api/classes/internal/core/bytes/#class-bytes-rfind "Copy/link to this API element")

```python
rfind(self: Self, sub: int, start: Optional[int] = 0, end: Optional[int] = None, /) -> int
```

Added by [`internal.types.bytes`](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/)

```python
rfind(self: Self, sub: bytes, start: Optional[int] = 0, end: Optional[int] = None, /) -> int
```

Added by [`internal.types.bytes`](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/)

```python
rfind(self: Self, sub: bytearray, start: Optional[int] = 0, end: Optional[int] = None, /) -> int
```

Added by [`internal.types.collections.bytearray`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/)

isidentifier[](https://docs.exaloop.io/libraries/api/classes/internal/core/bytes/#class-bytes-isidentifier "Copy/link to this API element")

```python
isidentifier(self: Self) -> bool
```

Added by [`internal.types.bytes`](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/)

isprintable[](https://docs.exaloop.io/libraries/api/classes/internal/core/bytes/#class-bytes-isprintable "Copy/link to this API element")

```python
isprintable(self: Self) -> bool
```

Added by [`internal.types.bytes`](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/)

lstrip[](https://docs.exaloop.io/libraries/api/classes/internal/core/bytes/#class-bytes-lstrip "Copy/link to this API element")

```python
lstrip(self: Self, chars: Optional[bytes] = None, /) -> bytes
```

Added by [`internal.types.bytes`](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/)

rstrip[](https://docs.exaloop.io/libraries/api/classes/internal/core/bytes/#class-bytes-rstrip "Copy/link to this API element")

```python
rstrip(self: Self, chars: Optional[bytes] = None, /) -> bytes
```

Added by [`internal.types.bytes`](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/)

strip[](https://docs.exaloop.io/libraries/api/classes/internal/core/bytes/#class-bytes-strip "Copy/link to this API element")

```python
strip(self: Self, chars: Optional[bytes] = None, /) -> bytes
```

Added by [`internal.types.bytes`](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/)

partition[](https://docs.exaloop.io/libraries/api/classes/internal/core/bytes/#class-bytes-partition "Copy/link to this API element")

```python
partition(self: Self, sep: bytes, /) -> Tuple[(bytes, bytes, bytes)]
```

Added by [`internal.types.bytes`](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/)

rpartition[](https://docs.exaloop.io/libraries/api/classes/internal/core/bytes/#class-bytes-rpartition "Copy/link to this API element")

```python
rpartition(self: Self, sep: bytes, /) -> Tuple[(bytes, bytes, bytes)]
```

Added by [`internal.types.bytes`](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/)

split[](https://docs.exaloop.io/libraries/api/classes/internal/core/bytes/#class-bytes-split "Copy/link to this API element")

```python
split(self: Self, /, sep: Optional[bytes] = None, maxsplit: int = -1) -> List[bytes]
```

Added by [`internal.types.bytes`](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/)

rsplit[](https://docs.exaloop.io/libraries/api/classes/internal/core/bytes/#class-bytes-rsplit "Copy/link to this API element")

```python
rsplit(self: Self, /, sep: Optional[bytes] = None, maxsplit: int = -1) -> List[bytes]
```

Added by [`internal.types.bytes`](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/)

splitlines[](https://docs.exaloop.io/libraries/api/classes/internal/core/bytes/#class-bytes-splitlines "Copy/link to this API element")

```python
splitlines(self: Self, /, keepends: bool = False) -> List[bytes]
```

Added by [`internal.types.bytes`](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/)

startswith[](https://docs.exaloop.io/libraries/api/classes/internal/core/bytes/#class-bytes-startswith "Copy/link to this API element")

```python
startswith(self: Self, prefix, start: Optional[int] = 0, end: Optional[int] = None, /) -> bool
```

Added by [`internal.types.bytes`](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/)

```python
startswith(self: Self, prefix: bytes, start: Optional[int] = 0, end: Optional[int] = None, /) -> bool
```

Added by [`internal.types.bytes`](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/)

```python
startswith(self: Self, prefix: bytearray, start: Optional[int] = 0, end: Optional[int] = None, /) -> bool
```

Added by [`internal.types.collections.bytearray`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/)

endswith[](https://docs.exaloop.io/libraries/api/classes/internal/core/bytes/#class-bytes-endswith "Copy/link to this API element")

```python
endswith(self: Self, suffix, start: Optional[int] = 0, end: Optional[int] = None, /) -> bool
```

Added by [`internal.types.bytes`](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/)

```python
endswith(self: Self, suffix: bytes, start: Optional[int] = 0, end: Optional[int] = None, /) -> bool
```

Added by [`internal.types.bytes`](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/)

```python
endswith(self: Self, suffix: bytearray, start: Optional[int] = 0, end: Optional[int] = None, /) -> bool
```

Added by [`internal.types.collections.bytearray`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/)

removeprefix[](https://docs.exaloop.io/libraries/api/classes/internal/core/bytes/#class-bytes-removeprefix "Copy/link to this API element")

```python
removeprefix(self: Self, prefix, /) -> bytes
```

Added by [`internal.types.bytes`](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/)

removesuffix[](https://docs.exaloop.io/libraries/api/classes/internal/core/bytes/#class-bytes-removesuffix "Copy/link to this API element")

```python
removesuffix(self: Self, suffix, /) -> bytes
```

Added by [`internal.types.bytes`](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/)

index[](https://docs.exaloop.io/libraries/api/classes/internal/core/bytes/#class-bytes-index "Copy/link to this API element")

```python
index(self: Self, sub, start: Optional[int] = 0, end: Optional[int] = None, /) -> int
```

Added by [`internal.types.bytes`](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/)

rindex[](https://docs.exaloop.io/libraries/api/classes/internal/core/bytes/#class-bytes-rindex "Copy/link to this API element")

```python
rindex(self: Self, sub, start: Optional[int] = 0, end: Optional[int] = None, /) -> int
```

Added by [`internal.types.bytes`](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/)

replace[](https://docs.exaloop.io/libraries/api/classes/internal/core/bytes/#class-bytes-replace "Copy/link to this API element")

```python
replace(self: Self, old: bytes, new: bytes, maxcount: int = -1, /) -> bytes
```

Added by [`internal.types.bytes`](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/)

expandtabs[](https://docs.exaloop.io/libraries/api/classes/internal/core/bytes/#class-bytes-expandtabs "Copy/link to this API element")

```python
expandtabs(self: Self, /, tabsize: int = 8) -> bytes
```

Added by [`internal.types.bytes`](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/)

translate[](https://docs.exaloop.io/libraries/api/classes/internal/core/bytes/#class-bytes-translate "Copy/link to this API element")

```python
translate(self: Self, map, /) -> bytes
```

Added by [`internal.types.bytes`](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/)

```python
translate(self: Self, table: bytes, /, delete: bytes = "") -> bytes
```

Added by [`internal.types.bytes`](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/)

Special methods 31

\_\_str\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/core/bytes/#class-bytes-__str__ "Copy/link to this API element")

```python
__str__(self: Self, /) -> str
```

Added by [`internal.types.bytes`](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/)

\_\_bytes\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/core/bytes/#class-bytes-__bytes__ "Copy/link to this API element")

```python
__bytes__(self: Self, /) -> bytes
```

Added by [`internal.types.bytes`](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/)

\_\_len\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/core/bytes/#class-bytes-__len__ "Copy/link to this API element")

```python
__len__(self: Self, /) -> int
```

Added by [`internal.types.bytes`](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/)

\_\_bool\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/core/bytes/#class-bytes-__bool__ "Copy/link to this API element")

```python
__bool__(self: Self) -> bool
```

Added by [`internal.types.bytes`](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/)

\_\_copy\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/core/bytes/#class-bytes-__copy__ "Copy/link to this API element")

```python
__copy__(self: Self) -> bytes
```

Added by [`internal.types.bytes`](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/)

\_\_deepcopy\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/core/bytes/#class-bytes-__deepcopy__ "Copy/link to this API element")

```python
__deepcopy__(self: Self) -> bytes
```

Added by [`internal.types.bytes`](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/)

\_\_ptrcopy\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/core/bytes/#class-bytes-__ptrcopy__ "Copy/link to this API element")

```python
__ptrcopy__(self: Self) -> bytes
```

Added by [`internal.types.bytes`](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/)

\_\_add\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/core/bytes/#class-bytes-__add__ "Copy/link to this API element")

```python
__add__(self: Self, other: bytes, /) -> bytes
```

Added by [`internal.types.bytes`](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/)

```python
__add__(self: Self, other: bytearray, /) -> bytes
```

Added by [`internal.types.collections.bytearray`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/)

\_\_eq\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/core/bytes/#class-bytes-__eq__ "Copy/link to this API element")

```python
__eq__(self: Self, other: bytes, /) -> bool
```

Added by [`internal.types.bytes`](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/)

```python
__eq__(self: Self, other: bytearray, /) -> bool
```

Added by [`internal.types.collections.bytearray`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/)

\_\_match\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/core/bytes/#class-bytes-__match__ "Copy/link to this API element")

```python
__match__(self: Self, obj: bytes) -> bool
```

Added by [`internal.types.bytes`](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/)

\_\_ne\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/core/bytes/#class-bytes-__ne__ "Copy/link to this API element")

```python
__ne__(self: Self, other: bytes, /) -> bool
```

Added by [`internal.types.bytes`](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/)

```python
__ne__(self: Self, other: bytearray, /) -> bool
```

Added by [`internal.types.collections.bytearray`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/)

\_\_hash\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/core/bytes/#class-bytes-__hash__ "Copy/link to this API element")

```python
__hash__(self: Self, /) -> int
```

Added by [`internal.types.bytes`](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/)

\_\_lt\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/core/bytes/#class-bytes-__lt__ "Copy/link to this API element")

```python
__lt__(self: Self, other: bytes, /) -> bool
```

Added by [`internal.types.bytes`](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/)

```python
__lt__(self: Self, other: bytearray, /) -> bool
```

Added by [`internal.types.collections.bytearray`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/)

\_\_le\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/core/bytes/#class-bytes-__le__ "Copy/link to this API element")

```python
__le__(self: Self, other: bytes, /) -> bool
```

Added by [`internal.types.bytes`](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/)

```python
__le__(self: Self, other: bytearray, /) -> bool
```

Added by [`internal.types.collections.bytearray`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/)

\_\_gt\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/core/bytes/#class-bytes-__gt__ "Copy/link to this API element")

```python
__gt__(self: Self, other: bytes, /) -> bool
```

Added by [`internal.types.bytes`](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/)

```python
__gt__(self: Self, other: bytearray, /) -> bool
```

Added by [`internal.types.collections.bytearray`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/)

\_\_ge\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/core/bytes/#class-bytes-__ge__ "Copy/link to this API element")

```python
__ge__(self: Self, other: bytes, /) -> bool
```

Added by [`internal.types.bytes`](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/)

```python
__ge__(self: Self, other: bytearray, /) -> bool
```

Added by [`internal.types.collections.bytearray`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/)

\_\_repr\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/core/bytes/#class-bytes-__repr__ "Copy/link to this API element")

```python
__repr__(self: Self, /) -> str
```

Added by [`internal.types.bytes`](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/)

\_\_getitem\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/core/bytes/#class-bytes-__getitem__ "Copy/link to this API element")

```python
__getitem__(self: Self, idx: int, /) -> int
```

Added by [`internal.types.bytes`](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/)

```python
__getitem__(self: Self, s: Slice, /) -> bytes
```

Added by [`internal.types.bytes`](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/)

\_\_iter\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/core/bytes/#class-bytes-__iter__ "Copy/link to this API element")

```python
__iter__(self: Self, /) -> Generator[int]
```

Added by [`internal.types.bytes`](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/)

\_\_reversed\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/core/bytes/#class-bytes-__reversed__ "Copy/link to this API element")

```python
__reversed__(self: Self) -> Generator[int]
```

Added by [`internal.types.bytes`](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/)

\_\_mul\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/core/bytes/#class-bytes-__mul__ "Copy/link to this API element")

```python
__mul__(self: Self, x: int, /) -> bytes
```

Added by [`internal.types.bytes`](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/)

\_\_rmul\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/core/bytes/#class-bytes-__rmul__ "Copy/link to this API element")

```python
__rmul__(self: Self, x: int, /) -> bytes
```

Added by [`internal.types.bytes`](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/)

\_\_contains\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/core/bytes/#class-bytes-__contains__ "Copy/link to this API element")

```python
__contains__(self: Self, pattern: bytes, /) -> bool
```

Added by [`internal.types.bytes`](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/)

\_\_mod\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/core/bytes/#class-bytes-__mod__ "Copy/link to this API element")

```python
__mod__(self: Self, values, /) -> bytes
```

Added by [`internal.format`](https://docs.exaloop.io/libraries/api/modules/internal/format/)

\_\_to\_gpu\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/core/bytes/#class-bytes-__to_gpu__ "Copy/link to this API element")

```python
__to_gpu__(self: Self, cache: AllocCache)
```

Added by [`internal.gpu`](https://docs.exaloop.io/libraries/api/modules/internal/gpu/)

\_\_from\_gpu\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/core/bytes/#class-bytes-__from_gpu__ "Copy/link to this API element")

```python
__from_gpu__(self: Self, other: bytes)
```

Added by [`internal.gpu`](https://docs.exaloop.io/libraries/api/modules/internal/gpu/)

\_\_from\_gpu\_new\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/core/bytes/#class-bytes-__from_gpu_new__ "Copy/link to this API element")

```python
__from_gpu_new__(other: bytes)
```

Added by [`internal.gpu`](https://docs.exaloop.io/libraries/api/modules/internal/gpu/)

\_\_pickle\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/core/bytes/#class-bytes-__pickle__ "Copy/link to this API element")

```python
__pickle__(self: Self, jar: Jar)
```

Added by [`pickle`](https://docs.exaloop.io/libraries/api/modules/pickle/)

\_\_unpickle\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/core/bytes/#class-bytes-__unpickle__ "Copy/link to this API element")

```python
__unpickle__(jar: Jar) -> bytes
```

Added by [`pickle`](https://docs.exaloop.io/libraries/api/modules/pickle/)

\_\_to\_py\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/core/bytes/#class-bytes-__to_py__ "Copy/link to this API element")

```python
__to_py__(self: Self) -> cobj
```

Added by [`internal.python`](https://docs.exaloop.io/libraries/api/modules/internal/python/)

\_\_from\_py\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/core/bytes/#class-bytes-__from_py__ "Copy/link to this API element")

```python
__from_py__(b: cobj) -> bytes
```

Added by [`internal.python`](https://docs.exaloop.io/libraries/api/modules/internal/python/)

Source: https://docs.exaloop.io/libraries/api/classes/internal/core/bytes/
