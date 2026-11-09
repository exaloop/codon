# bytes

Standard library class

class bytes[](https://docs.exaloop.io/libraries/api/classes/bytes/#class-bytes "Copy/link to this API element")

@tuple

```python
class bytes
```

Constructors 5

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/classes/bytes/#class-bytes-__new__ "Copy/link to this API element")

@\_\_internal\_\_

```python
__new__(p: Ptr[u8], l: int)
```

Added by [`internal.types.bytes`](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/)

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/classes/bytes/#class-bytes-__new__-overload-2 "Copy/link to this API element")

@overload

```python
__new__()
```

Added by [`internal.types.bytes`](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/)

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/classes/bytes/#class-bytes-__new__-overload-3 "Copy/link to this API element")

@overload

```python
__new__(what)
```

Added by [`internal.types.bytes`](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/)

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/classes/bytes/#class-bytes-__new__-overload-4 "Copy/link to this API element")

@overload

```python
__new__(what: bytes)
```

Added by [`internal.types.bytes`](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/)

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/classes/bytes/#class-bytes-__new__-overload-5 "Copy/link to this API element")

```python
__new__(what: str, encoding: str, errors: str = "strict")
```

Added by [`internal.types.bytes`](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/)

Methods 53

decode[](https://docs.exaloop.io/libraries/api/classes/bytes/#class-bytes-decode "Copy/link to this API element")

```python
decode(self: Self, encoding: str = "utf-8", errors: str = "strict", Self: type)
```

Added by [`internal.types.bytes`](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/)

memcpy[](https://docs.exaloop.io/libraries/api/classes/bytes/#class-bytes-memcpy "Copy/link to this API element")

@llvm

```python
memcpy(dest: Ptr[u8], src: Ptr[u8], len: int)
```

Added by [`internal.types.bytes`](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/)

```llvm
declare void @llvm.memcpy.p0i8.p0i8.i64(ptr %dest, ptr %src, i64 %len, i32 %align, i1 %isvolatile)
call void @llvm.memcpy.p0i8.p0i8.i64(ptr %dest, ptr %src, i64 %len, i32 0, i1 false)
ret {} {}
```

memmove[](https://docs.exaloop.io/libraries/api/classes/bytes/#class-bytes-memmove "Copy/link to this API element")

@llvm

```python
memmove(dest: Ptr[u8], src: Ptr[u8], len: int)
```

Added by [`internal.types.bytes`](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/)

```llvm
declare void @llvm.memmove.p0i8.p0i8.i64(ptr %dest, ptr %src, i64 %len, i32 %align, i1 %isvolatile)
call void @llvm.memmove.p0i8.p0i8.i64(ptr %dest, ptr %src, i64 %len, i32 0, i1 false)
ret {} {}
```

memset[](https://docs.exaloop.io/libraries/api/classes/bytes/#class-bytes-memset "Copy/link to this API element")

@llvm

```python
memset(dest: Ptr[u8], val: u8, len: int)
```

Added by [`internal.types.bytes`](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/)

```llvm
declare void @llvm.memset.p0i8.i64(ptr %dest, i8 %val, i64 %len, i32 %align, i1 %isvolatile)
call void @llvm.memset.p0i8.i64(ptr %dest, i8 %val, i64 %len, i32 0, i1 false)
ret {} {}
```

c\_str[](https://docs.exaloop.io/libraries/api/classes/bytes/#class-bytes-c_str "Copy/link to this API element")

```python
c_str(self: Self, Self: type)
```

Added by [`internal.types.bytes`](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/)

from\_ptr[](https://docs.exaloop.io/libraries/api/classes/bytes/#class-bytes-from_ptr "Copy/link to this API element")

```python
from_ptr(t: cobj)
```

Added by [`internal.types.bytes`](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/)

cat[](https://docs.exaloop.io/libraries/api/classes/bytes/#class-bytes-cat "Copy/link to this API element")

```python
cat(*args)
```

Added by [`internal.types.bytes`](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/)

fromhex[](https://docs.exaloop.io/libraries/api/classes/bytes/#class-bytes-fromhex "Copy/link to this API element")

@staticmethod

```python
fromhex(value)
```

Added by [`internal.types.bytes`](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/)

```python
fromhex(value: str)
```

Added by [`internal.types.bytes`](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/)

```python
fromhex(value: bytes)
```

Added by [`internal.types.bytes`](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/)

hex[](https://docs.exaloop.io/libraries/api/classes/bytes/#class-bytes-hex "Copy/link to this API element")

```python
hex(self: Self, Self: type)
```

Added by [`internal.types.bytes`](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/)

```python
hex(self: Self, sep: str, bytes_per_sep: int = 1, Self: type)
```

Added by [`internal.types.bytes`](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/)

```python
hex(self: Self, sep: bytes, bytes_per_sep: int = 1, Self: type)
```

Added by [`internal.types.bytes`](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/)

maketrans[](https://docs.exaloop.io/libraries/api/classes/bytes/#class-bytes-maketrans "Copy/link to this API element")

@staticmethod

```python
maketrans(source, replacement)
```

Added by [`internal.types.bytes`](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/)

join[](https://docs.exaloop.io/libraries/api/classes/bytes/#class-bytes-join "Copy/link to this API element")

```python
join(self: Self, l: Generator[bytes], Self: type)
```

Added by [`internal.types.bytes`](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/)

```python
join(self: Self, l: List[bytes], Self: type)
```

Added by [`internal.types.bytes`](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/)

isdigit[](https://docs.exaloop.io/libraries/api/classes/bytes/#class-bytes-isdigit "Copy/link to this API element")

```python
isdigit(self: Self, Self: type)
```

Added by [`internal.types.bytes`](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/)

islower[](https://docs.exaloop.io/libraries/api/classes/bytes/#class-bytes-islower "Copy/link to this API element")

```python
islower(self: Self, Self: type)
```

Added by [`internal.types.bytes`](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/)

isupper[](https://docs.exaloop.io/libraries/api/classes/bytes/#class-bytes-isupper "Copy/link to this API element")

```python
isupper(self: Self, Self: type)
```

Added by [`internal.types.bytes`](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/)

isalnum[](https://docs.exaloop.io/libraries/api/classes/bytes/#class-bytes-isalnum "Copy/link to this API element")

```python
isalnum(self: Self, Self: type)
```

Added by [`internal.types.bytes`](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/)

isalpha[](https://docs.exaloop.io/libraries/api/classes/bytes/#class-bytes-isalpha "Copy/link to this API element")

```python
isalpha(self: Self, Self: type)
```

Added by [`internal.types.bytes`](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/)

isspace[](https://docs.exaloop.io/libraries/api/classes/bytes/#class-bytes-isspace "Copy/link to this API element")

```python
isspace(self: Self, Self: type)
```

Added by [`internal.types.bytes`](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/)

istitle[](https://docs.exaloop.io/libraries/api/classes/bytes/#class-bytes-istitle "Copy/link to this API element")

```python
istitle(self: Self, Self: type)
```

Added by [`internal.types.bytes`](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/)

capitalize[](https://docs.exaloop.io/libraries/api/classes/bytes/#class-bytes-capitalize "Copy/link to this API element")

```python
capitalize(self: Self, Self: type)
```

Added by [`internal.types.bytes`](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/)

isdecimal[](https://docs.exaloop.io/libraries/api/classes/bytes/#class-bytes-isdecimal "Copy/link to this API element")

```python
isdecimal(self: Self, Self: type)
```

Added by [`internal.types.bytes`](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/)

lower[](https://docs.exaloop.io/libraries/api/classes/bytes/#class-bytes-lower "Copy/link to this API element")

```python
lower(self: Self, Self: type)
```

Added by [`internal.types.bytes`](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/)

upper[](https://docs.exaloop.io/libraries/api/classes/bytes/#class-bytes-upper "Copy/link to this API element")

```python
upper(self: Self, Self: type)
```

Added by [`internal.types.bytes`](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/)

isascii[](https://docs.exaloop.io/libraries/api/classes/bytes/#class-bytes-isascii "Copy/link to this API element")

```python
isascii(self: Self, Self: type)
```

Added by [`internal.types.bytes`](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/)

casefold[](https://docs.exaloop.io/libraries/api/classes/bytes/#class-bytes-casefold "Copy/link to this API element")

```python
casefold(self: Self, Self: type)
```

Added by [`internal.types.bytes`](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/)

swapcase[](https://docs.exaloop.io/libraries/api/classes/bytes/#class-bytes-swapcase "Copy/link to this API element")

```python
swapcase(self: Self, Self: type)
```

Added by [`internal.types.bytes`](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/)

title[](https://docs.exaloop.io/libraries/api/classes/bytes/#class-bytes-title "Copy/link to this API element")

```python
title(self: Self, Self: type)
```

Added by [`internal.types.bytes`](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/)

isnumeric[](https://docs.exaloop.io/libraries/api/classes/bytes/#class-bytes-isnumeric "Copy/link to this API element")

```python
isnumeric(self: Self, Self: type)
```

Added by [`internal.types.bytes`](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/)

ljust[](https://docs.exaloop.io/libraries/api/classes/bytes/#class-bytes-ljust "Copy/link to this API element")

```python
ljust(self: Self, width: int, fillchar: bytes = " ", Self: type)
```

Added by [`internal.types.bytes`](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/)

rjust[](https://docs.exaloop.io/libraries/api/classes/bytes/#class-bytes-rjust "Copy/link to this API element")

```python
rjust(self: Self, width: int, fillchar: bytes = " ", Self: type)
```

Added by [`internal.types.bytes`](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/)

center[](https://docs.exaloop.io/libraries/api/classes/bytes/#class-bytes-center "Copy/link to this API element")

```python
center(self: Self, width: int, fillchar: bytes = " ", Self: type)
```

Added by [`internal.types.bytes`](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/)

zfill[](https://docs.exaloop.io/libraries/api/classes/bytes/#class-bytes-zfill "Copy/link to this API element")

```python
zfill(self: Self, width: int, Self: type)
```

Added by [`internal.types.bytes`](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/)

count[](https://docs.exaloop.io/libraries/api/classes/bytes/#class-bytes-count "Copy/link to this API element")

```python
count(self: Self, sub: bytes, start: int = 0, end: Optional[int] = None, Self: type)
```

Added by [`internal.types.bytes`](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/)

find[](https://docs.exaloop.io/libraries/api/classes/bytes/#class-bytes-find "Copy/link to this API element")

```python
find(self: Self, sub: bytes, start: int = 0, end: Optional[int] = None, Self: type)
```

Added by [`internal.types.bytes`](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/)

rfind[](https://docs.exaloop.io/libraries/api/classes/bytes/#class-bytes-rfind "Copy/link to this API element")

```python
rfind(self: Self, sub: bytes, start: int = 0, end: Optional[int] = None, Self: type)
```

Added by [`internal.types.bytes`](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/)

isidentifier[](https://docs.exaloop.io/libraries/api/classes/bytes/#class-bytes-isidentifier "Copy/link to this API element")

```python
isidentifier(self: Self, Self: type)
```

Added by [`internal.types.bytes`](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/)

isprintable[](https://docs.exaloop.io/libraries/api/classes/bytes/#class-bytes-isprintable "Copy/link to this API element")

```python
isprintable(self: Self, Self: type)
```

Added by [`internal.types.bytes`](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/)

lstrip[](https://docs.exaloop.io/libraries/api/classes/bytes/#class-bytes-lstrip "Copy/link to this API element")

```python
lstrip(self: Self, chars: Optional[bytes] = None, Self: type)
```

Added by [`internal.types.bytes`](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/)

rstrip[](https://docs.exaloop.io/libraries/api/classes/bytes/#class-bytes-rstrip "Copy/link to this API element")

```python
rstrip(self: Self, chars: Optional[bytes] = None, Self: type)
```

Added by [`internal.types.bytes`](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/)

strip[](https://docs.exaloop.io/libraries/api/classes/bytes/#class-bytes-strip "Copy/link to this API element")

```python
strip(self: Self, chars: Optional[bytes] = None, Self: type)
```

Added by [`internal.types.bytes`](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/)

partition[](https://docs.exaloop.io/libraries/api/classes/bytes/#class-bytes-partition "Copy/link to this API element")

```python
partition(self: Self, sep: bytes, Self: type)
```

Added by [`internal.types.bytes`](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/)

rpartition[](https://docs.exaloop.io/libraries/api/classes/bytes/#class-bytes-rpartition "Copy/link to this API element")

```python
rpartition(self: Self, sep: bytes, Self: type)
```

Added by [`internal.types.bytes`](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/)

split[](https://docs.exaloop.io/libraries/api/classes/bytes/#class-bytes-split "Copy/link to this API element")

```python
split(self: Self, sep: Optional[bytes] = None, maxsplit: int = -1, Self: type)
```

Added by [`internal.types.bytes`](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/)

rsplit[](https://docs.exaloop.io/libraries/api/classes/bytes/#class-bytes-rsplit "Copy/link to this API element")

```python
rsplit(self: Self, sep: Optional[bytes] = None, maxsplit: int = -1, Self: type)
```

Added by [`internal.types.bytes`](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/)

splitlines[](https://docs.exaloop.io/libraries/api/classes/bytes/#class-bytes-splitlines "Copy/link to this API element")

```python
splitlines(self: Self, keepends: bool = False, Self: type)
```

Added by [`internal.types.bytes`](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/)

startswith[](https://docs.exaloop.io/libraries/api/classes/bytes/#class-bytes-startswith "Copy/link to this API element")

```python
startswith(self: Self, prefix: bytes, start: int = 0, end: Optional[int] = None, Self: type)
```

Added by [`internal.types.bytes`](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/)

endswith[](https://docs.exaloop.io/libraries/api/classes/bytes/#class-bytes-endswith "Copy/link to this API element")

```python
endswith(self: Self, suffix: bytes, start: int = 0, end: Optional[int] = None, Self: type)
```

Added by [`internal.types.bytes`](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/)

removeprefix[](https://docs.exaloop.io/libraries/api/classes/bytes/#class-bytes-removeprefix "Copy/link to this API element")

```python
removeprefix(self: Self, prefix, Self: type)
```

Added by [`internal.types.bytes`](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/)

removesuffix[](https://docs.exaloop.io/libraries/api/classes/bytes/#class-bytes-removesuffix "Copy/link to this API element")

```python
removesuffix(self: Self, suffix, Self: type)
```

Added by [`internal.types.bytes`](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/)

index[](https://docs.exaloop.io/libraries/api/classes/bytes/#class-bytes-index "Copy/link to this API element")

```python
index(self: Self, sub: bytes, start: int = 0, end: Optional[int] = None, Self: type)
```

Added by [`internal.types.bytes`](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/)

rindex[](https://docs.exaloop.io/libraries/api/classes/bytes/#class-bytes-rindex "Copy/link to this API element")

```python
rindex(self: Self, sub: bytes, start: int = 0, end: Optional[int] = None, Self: type)
```

Added by [`internal.types.bytes`](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/)

replace[](https://docs.exaloop.io/libraries/api/classes/bytes/#class-bytes-replace "Copy/link to this API element")

```python
replace(self: Self, old: bytes, new: bytes, maxcount: int = -1, Self: type)
```

Added by [`internal.types.bytes`](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/)

expandtabs[](https://docs.exaloop.io/libraries/api/classes/bytes/#class-bytes-expandtabs "Copy/link to this API element")

```python
expandtabs(self: Self, tabsize: int = 8, Self: type)
```

Added by [`internal.types.bytes`](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/)

translate[](https://docs.exaloop.io/libraries/api/classes/bytes/#class-bytes-translate "Copy/link to this API element")

```python
translate(self: Self, map, Self: type)
```

Added by [`internal.types.bytes`](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/)

```python
translate(self: Self, table: bytes, delete: bytes = "", Self: type)
```

Added by [`internal.types.bytes`](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/)

Special methods 29

\_\_to\_py\_\_[](https://docs.exaloop.io/libraries/api/classes/bytes/#class-bytes-__to_py__ "Copy/link to this API element")

```python
__to_py__(self: Self, Self: type)
```

Added by [`internal.python`](https://docs.exaloop.io/libraries/api/modules/internal/python/)

\_\_from\_py\_\_[](https://docs.exaloop.io/libraries/api/classes/bytes/#class-bytes-__from_py__ "Copy/link to this API element")

```python
__from_py__(b: cobj)
```

Added by [`internal.python`](https://docs.exaloop.io/libraries/api/modules/internal/python/)

\_\_str\_\_[](https://docs.exaloop.io/libraries/api/classes/bytes/#class-bytes-__str__ "Copy/link to this API element")

```python
__str__(self: Self, Self: type)
```

Added by [`internal.types.bytes`](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/)

\_\_bytes\_\_[](https://docs.exaloop.io/libraries/api/classes/bytes/#class-bytes-__bytes__ "Copy/link to this API element")

```python
__bytes__(self: Self, Self: type)
```

Added by [`internal.types.bytes`](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/)

\_\_len\_\_[](https://docs.exaloop.io/libraries/api/classes/bytes/#class-bytes-__len__ "Copy/link to this API element")

```python
__len__(self: Self, Self: type)
```

Added by [`internal.types.bytes`](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/)

\_\_bool\_\_[](https://docs.exaloop.io/libraries/api/classes/bytes/#class-bytes-__bool__ "Copy/link to this API element")

```python
__bool__(self: Self, Self: type)
```

Added by [`internal.types.bytes`](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/)

\_\_copy\_\_[](https://docs.exaloop.io/libraries/api/classes/bytes/#class-bytes-__copy__ "Copy/link to this API element")

```python
__copy__(self: Self, Self: type)
```

Added by [`internal.types.bytes`](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/)

\_\_deepcopy\_\_[](https://docs.exaloop.io/libraries/api/classes/bytes/#class-bytes-__deepcopy__ "Copy/link to this API element")

```python
__deepcopy__(self: Self, Self: type)
```

Added by [`internal.types.bytes`](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/)

\_\_ptrcopy\_\_[](https://docs.exaloop.io/libraries/api/classes/bytes/#class-bytes-__ptrcopy__ "Copy/link to this API element")

```python
__ptrcopy__(self: Self, Self: type)
```

Added by [`internal.types.bytes`](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/)

\_\_add\_\_[](https://docs.exaloop.io/libraries/api/classes/bytes/#class-bytes-__add__ "Copy/link to this API element")

```python
__add__(self: Self, other: bytes, Self: type)
```

Added by [`internal.types.bytes`](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/)

```python
__add__(self: Self, other: bytearray, Self: type)
```

Added by [`internal.types.collections.bytearray`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/)

\_\_eq\_\_[](https://docs.exaloop.io/libraries/api/classes/bytes/#class-bytes-__eq__ "Copy/link to this API element")

```python
__eq__(self: Self, other: bytes, Self: type)
```

Added by [`internal.types.bytes`](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/)

```python
__eq__(self: Self, other: bytearray, Self: type)
```

Added by [`internal.types.collections.bytearray`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/)

\_\_match\_\_[](https://docs.exaloop.io/libraries/api/classes/bytes/#class-bytes-__match__ "Copy/link to this API element")

```python
__match__(self: Self, obj: bytes, Self: type)
```

Added by [`internal.types.bytes`](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/)

\_\_ne\_\_[](https://docs.exaloop.io/libraries/api/classes/bytes/#class-bytes-__ne__ "Copy/link to this API element")

```python
__ne__(self: Self, other: bytes, Self: type)
```

Added by [`internal.types.bytes`](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/)

```python
__ne__(self: Self, other: bytearray, Self: type)
```

Added by [`internal.types.collections.bytearray`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/)

\_\_hash\_\_[](https://docs.exaloop.io/libraries/api/classes/bytes/#class-bytes-__hash__ "Copy/link to this API element")

```python
__hash__(self: Self, Self: type)
```

Added by [`internal.types.bytes`](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/)

\_\_lt\_\_[](https://docs.exaloop.io/libraries/api/classes/bytes/#class-bytes-__lt__ "Copy/link to this API element")

```python
__lt__(self: Self, other: bytes, Self: type)
```

Added by [`internal.types.bytes`](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/)

```python
__lt__(self: Self, other: bytearray, Self: type)
```

Added by [`internal.types.collections.bytearray`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/)

\_\_le\_\_[](https://docs.exaloop.io/libraries/api/classes/bytes/#class-bytes-__le__ "Copy/link to this API element")

```python
__le__(self: Self, other: bytes, Self: type)
```

Added by [`internal.types.bytes`](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/)

```python
__le__(self: Self, other: bytearray, Self: type)
```

Added by [`internal.types.collections.bytearray`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/)

\_\_gt\_\_[](https://docs.exaloop.io/libraries/api/classes/bytes/#class-bytes-__gt__ "Copy/link to this API element")

```python
__gt__(self: Self, other: bytes, Self: type)
```

Added by [`internal.types.bytes`](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/)

```python
__gt__(self: Self, other: bytearray, Self: type)
```

Added by [`internal.types.collections.bytearray`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/)

\_\_ge\_\_[](https://docs.exaloop.io/libraries/api/classes/bytes/#class-bytes-__ge__ "Copy/link to this API element")

```python
__ge__(self: Self, other: bytes, Self: type)
```

Added by [`internal.types.bytes`](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/)

```python
__ge__(self: Self, other: bytearray, Self: type)
```

Added by [`internal.types.collections.bytearray`](https://docs.exaloop.io/libraries/api/modules/internal/types/collections/bytearray/)

\_\_repr\_\_[](https://docs.exaloop.io/libraries/api/classes/bytes/#class-bytes-__repr__ "Copy/link to this API element")

```python
__repr__(self: Self, Self: type)
```

Added by [`internal.types.bytes`](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/)

\_\_getitem\_\_[](https://docs.exaloop.io/libraries/api/classes/bytes/#class-bytes-__getitem__ "Copy/link to this API element")

```python
__getitem__(self: Self, idx: int, Self: type)
```

Added by [`internal.types.bytes`](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/)

```python
__getitem__(self: Self, s: Slice, Self: type)
```

Added by [`internal.types.bytes`](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/)

\_\_iter\_\_[](https://docs.exaloop.io/libraries/api/classes/bytes/#class-bytes-__iter__ "Copy/link to this API element")

```python
__iter__(self: Self, Self: type)
```

Added by [`internal.types.bytes`](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/)

\_\_reversed\_\_[](https://docs.exaloop.io/libraries/api/classes/bytes/#class-bytes-__reversed__ "Copy/link to this API element")

```python
__reversed__(self: Self, Self: type)
```

Added by [`internal.types.bytes`](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/)

\_\_mul\_\_[](https://docs.exaloop.io/libraries/api/classes/bytes/#class-bytes-__mul__ "Copy/link to this API element")

```python
__mul__(self: Self, x: int, Self: type)
```

Added by [`internal.types.bytes`](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/)

\_\_rmul\_\_[](https://docs.exaloop.io/libraries/api/classes/bytes/#class-bytes-__rmul__ "Copy/link to this API element")

```python
__rmul__(self: Self, x: int, Self: type)
```

Added by [`internal.types.bytes`](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/)

\_\_contains\_\_[](https://docs.exaloop.io/libraries/api/classes/bytes/#class-bytes-__contains__ "Copy/link to this API element")

```python
__contains__(self: Self, pattern: bytes, Self: type)
```

Added by [`internal.types.bytes`](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/)

\_\_mod\_\_[](https://docs.exaloop.io/libraries/api/classes/bytes/#class-bytes-__mod__ "Copy/link to this API element")

```python
__mod__(self: Self, values, Self: type)
```

Added by [`internal.format`](https://docs.exaloop.io/libraries/api/modules/internal/format/)

\_\_to\_gpu\_\_[](https://docs.exaloop.io/libraries/api/classes/bytes/#class-bytes-__to_gpu__ "Copy/link to this API element")

```python
__to_gpu__(self: Self, cache: AllocCache, Self: type)
```

Added by [`internal.gpu`](https://docs.exaloop.io/libraries/api/modules/internal/gpu/)

\_\_from\_gpu\_\_[](https://docs.exaloop.io/libraries/api/classes/bytes/#class-bytes-__from_gpu__ "Copy/link to this API element")

```python
__from_gpu__(self: Self, other: bytes, Self: type)
```

Added by [`internal.gpu`](https://docs.exaloop.io/libraries/api/modules/internal/gpu/)

\_\_from\_gpu\_new\_\_[](https://docs.exaloop.io/libraries/api/classes/bytes/#class-bytes-__from_gpu_new__ "Copy/link to this API element")

```python
__from_gpu_new__(other: bytes)
```

Added by [`internal.gpu`](https://docs.exaloop.io/libraries/api/modules/internal/gpu/)

Source: https://docs.exaloop.io/libraries/api/classes/bytes/
