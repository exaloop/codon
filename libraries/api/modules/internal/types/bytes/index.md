# internal.types.bytes

Standard library module [View source](https://github.com/exaloop/codon/blob/master/stdlib/internal/types/bytes.codon)

class bytes[](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/#class-bytes "Copy/link to this API element")

@extend

```python
class bytes
```

Extends [`bytes`](https://docs.exaloop.io/libraries/api/classes/bytes/) · [View complete class API →](https://docs.exaloop.io/libraries/api/classes/bytes/)

Constructors 5

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/#class-bytes-__new__ "Copy/link to this API element")

@\_\_internal\_\_

```python
__new__(p: Ptr[u8], l: int)
```

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/#class-bytes-__new__-overload-2 "Copy/link to this API element")

@overload

```python
__new__()
```

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/#class-bytes-__new__-overload-3 "Copy/link to this API element")

@overload

```python
__new__(what)
```

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/#class-bytes-__new__-overload-4 "Copy/link to this API element")

@overload

```python
__new__(what: bytes)
```

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/#class-bytes-__new__-overload-5 "Copy/link to this API element")

```python
__new__(what: str, encoding: str, errors: str = "strict")
```

Methods 7

decode[](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/#class-bytes-decode "Copy/link to this API element")

```python
decode(self: Self, encoding: str = "utf-8", errors: str = "strict", Self: type)
```

memcpy[](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/#class-bytes-memcpy "Copy/link to this API element")

@llvm

```python
memcpy(dest: Ptr[u8], src: Ptr[u8], len: int)
```

```llvm
declare void @llvm.memcpy.p0i8.p0i8.i64(ptr %dest, ptr %src, i64 %len, i32 %align, i1 %isvolatile)
call void @llvm.memcpy.p0i8.p0i8.i64(ptr %dest, ptr %src, i64 %len, i32 0, i1 false)
ret {} {}
```

memmove[](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/#class-bytes-memmove "Copy/link to this API element")

@llvm

```python
memmove(dest: Ptr[u8], src: Ptr[u8], len: int)
```

```llvm
declare void @llvm.memmove.p0i8.p0i8.i64(ptr %dest, ptr %src, i64 %len, i32 %align, i1 %isvolatile)
call void @llvm.memmove.p0i8.p0i8.i64(ptr %dest, ptr %src, i64 %len, i32 0, i1 false)
ret {} {}
```

memset[](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/#class-bytes-memset "Copy/link to this API element")

@llvm

```python
memset(dest: Ptr[u8], val: u8, len: int)
```

```llvm
declare void @llvm.memset.p0i8.i64(ptr %dest, i8 %val, i64 %len, i32 %align, i1 %isvolatile)
call void @llvm.memset.p0i8.i64(ptr %dest, i8 %val, i64 %len, i32 0, i1 false)
ret {} {}
```

c\_str[](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/#class-bytes-c_str "Copy/link to this API element")

```python
c_str(self: Self, Self: type)
```

from\_ptr[](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/#class-bytes-from_ptr "Copy/link to this API element")

```python
from_ptr(t: cobj)
```

cat[](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/#class-bytes-cat "Copy/link to this API element")

```python
cat(*args)
```

Special methods 22

\_\_str\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/#class-bytes-__str__ "Copy/link to this API element")

```python
__str__(self: Self, Self: type)
```

\_\_bytes\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/#class-bytes-__bytes__ "Copy/link to this API element")

```python
__bytes__(self: Self, Self: type)
```

\_\_len\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/#class-bytes-__len__ "Copy/link to this API element")

```python
__len__(self: Self, Self: type)
```

\_\_bool\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/#class-bytes-__bool__ "Copy/link to this API element")

```python
__bool__(self: Self, Self: type)
```

\_\_copy\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/#class-bytes-__copy__ "Copy/link to this API element")

```python
__copy__(self: Self, Self: type)
```

\_\_deepcopy\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/#class-bytes-__deepcopy__ "Copy/link to this API element")

```python
__deepcopy__(self: Self, Self: type)
```

\_\_ptrcopy\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/#class-bytes-__ptrcopy__ "Copy/link to this API element")

```python
__ptrcopy__(self: Self, Self: type)
```

\_\_add\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/#class-bytes-__add__ "Copy/link to this API element")

```python
__add__(self: Self, other: bytes, Self: type)
```

\_\_eq\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/#class-bytes-__eq__ "Copy/link to this API element")

```python
__eq__(self: Self, other: bytes, Self: type)
```

\_\_match\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/#class-bytes-__match__ "Copy/link to this API element")

```python
__match__(self: Self, obj: bytes, Self: type)
```

\_\_ne\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/#class-bytes-__ne__ "Copy/link to this API element")

```python
__ne__(self: Self, other: bytes, Self: type)
```

\_\_hash\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/#class-bytes-__hash__ "Copy/link to this API element")

```python
__hash__(self: Self, Self: type)
```

\_\_lt\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/#class-bytes-__lt__ "Copy/link to this API element")

```python
__lt__(self: Self, other: bytes, Self: type)
```

\_\_le\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/#class-bytes-__le__ "Copy/link to this API element")

```python
__le__(self: Self, other: bytes, Self: type)
```

\_\_gt\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/#class-bytes-__gt__ "Copy/link to this API element")

```python
__gt__(self: Self, other: bytes, Self: type)
```

\_\_ge\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/#class-bytes-__ge__ "Copy/link to this API element")

```python
__ge__(self: Self, other: bytes, Self: type)
```

\_\_repr\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/#class-bytes-__repr__ "Copy/link to this API element")

```python
__repr__(self: Self, Self: type)
```

\_\_getitem\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/#class-bytes-__getitem__ "Copy/link to this API element")

```python
__getitem__(self: Self, idx: int, Self: type)
```

```python
__getitem__(self: Self, s: Slice, Self: type)
```

\_\_iter\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/#class-bytes-__iter__ "Copy/link to this API element")

```python
__iter__(self: Self, Self: type)
```

\_\_reversed\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/#class-bytes-__reversed__ "Copy/link to this API element")

```python
__reversed__(self: Self, Self: type)
```

\_\_mul\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/#class-bytes-__mul__ "Copy/link to this API element")

```python
__mul__(self: Self, x: int, Self: type)
```

\_\_rmul\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/#class-bytes-__rmul__ "Copy/link to this API element")

```python
__rmul__(self: Self, x: int, Self: type)
```

class bytes[](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/#class-bytes "Copy/link to this API element")

@extend

```python
class bytes
```

Extends [`bytes`](https://docs.exaloop.io/libraries/api/classes/bytes/) · [View complete class API →](https://docs.exaloop.io/libraries/api/classes/bytes/)

Methods 46

fromhex[](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/#class-bytes-fromhex "Copy/link to this API element")

@staticmethod

```python
fromhex(value)
```

```python
fromhex(value: str)
```

```python
fromhex(value: bytes)
```

hex[](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/#class-bytes-hex "Copy/link to this API element")

```python
hex(self: Self, Self: type)
```

```python
hex(self: Self, sep: str, bytes_per_sep: int = 1, Self: type)
```

```python
hex(self: Self, sep: bytes, bytes_per_sep: int = 1, Self: type)
```

maketrans[](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/#class-bytes-maketrans "Copy/link to this API element")

@staticmethod

```python
maketrans(source, replacement)
```

join[](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/#class-bytes-join "Copy/link to this API element")

```python
join(self: Self, l: Generator[bytes], Self: type)
```

```python
join(self: Self, l: List[bytes], Self: type)
```

isdigit[](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/#class-bytes-isdigit "Copy/link to this API element")

```python
isdigit(self: Self, Self: type)
```

islower[](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/#class-bytes-islower "Copy/link to this API element")

```python
islower(self: Self, Self: type)
```

isupper[](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/#class-bytes-isupper "Copy/link to this API element")

```python
isupper(self: Self, Self: type)
```

isalnum[](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/#class-bytes-isalnum "Copy/link to this API element")

```python
isalnum(self: Self, Self: type)
```

isalpha[](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/#class-bytes-isalpha "Copy/link to this API element")

```python
isalpha(self: Self, Self: type)
```

isspace[](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/#class-bytes-isspace "Copy/link to this API element")

```python
isspace(self: Self, Self: type)
```

istitle[](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/#class-bytes-istitle "Copy/link to this API element")

```python
istitle(self: Self, Self: type)
```

capitalize[](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/#class-bytes-capitalize "Copy/link to this API element")

```python
capitalize(self: Self, Self: type)
```

isdecimal[](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/#class-bytes-isdecimal "Copy/link to this API element")

```python
isdecimal(self: Self, Self: type)
```

lower[](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/#class-bytes-lower "Copy/link to this API element")

```python
lower(self: Self, Self: type)
```

upper[](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/#class-bytes-upper "Copy/link to this API element")

```python
upper(self: Self, Self: type)
```

isascii[](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/#class-bytes-isascii "Copy/link to this API element")

```python
isascii(self: Self, Self: type)
```

casefold[](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/#class-bytes-casefold "Copy/link to this API element")

```python
casefold(self: Self, Self: type)
```

swapcase[](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/#class-bytes-swapcase "Copy/link to this API element")

```python
swapcase(self: Self, Self: type)
```

title[](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/#class-bytes-title "Copy/link to this API element")

```python
title(self: Self, Self: type)
```

isnumeric[](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/#class-bytes-isnumeric "Copy/link to this API element")

```python
isnumeric(self: Self, Self: type)
```

ljust[](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/#class-bytes-ljust "Copy/link to this API element")

```python
ljust(self: Self, width: int, fillchar: bytes = " ", Self: type)
```

rjust[](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/#class-bytes-rjust "Copy/link to this API element")

```python
rjust(self: Self, width: int, fillchar: bytes = " ", Self: type)
```

center[](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/#class-bytes-center "Copy/link to this API element")

```python
center(self: Self, width: int, fillchar: bytes = " ", Self: type)
```

zfill[](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/#class-bytes-zfill "Copy/link to this API element")

```python
zfill(self: Self, width: int, Self: type)
```

count[](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/#class-bytes-count "Copy/link to this API element")

```python
count(self: Self, sub: bytes, start: int = 0, end: Optional[int] = None, Self: type)
```

find[](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/#class-bytes-find "Copy/link to this API element")

```python
find(self: Self, sub: bytes, start: int = 0, end: Optional[int] = None, Self: type)
```

rfind[](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/#class-bytes-rfind "Copy/link to this API element")

```python
rfind(self: Self, sub: bytes, start: int = 0, end: Optional[int] = None, Self: type)
```

isidentifier[](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/#class-bytes-isidentifier "Copy/link to this API element")

```python
isidentifier(self: Self, Self: type)
```

isprintable[](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/#class-bytes-isprintable "Copy/link to this API element")

```python
isprintable(self: Self, Self: type)
```

lstrip[](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/#class-bytes-lstrip "Copy/link to this API element")

```python
lstrip(self: Self, chars: Optional[bytes] = None, Self: type)
```

rstrip[](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/#class-bytes-rstrip "Copy/link to this API element")

```python
rstrip(self: Self, chars: Optional[bytes] = None, Self: type)
```

strip[](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/#class-bytes-strip "Copy/link to this API element")

```python
strip(self: Self, chars: Optional[bytes] = None, Self: type)
```

partition[](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/#class-bytes-partition "Copy/link to this API element")

```python
partition(self: Self, sep: bytes, Self: type)
```

rpartition[](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/#class-bytes-rpartition "Copy/link to this API element")

```python
rpartition(self: Self, sep: bytes, Self: type)
```

split[](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/#class-bytes-split "Copy/link to this API element")

```python
split(self: Self, sep: Optional[bytes] = None, maxsplit: int = -1, Self: type)
```

rsplit[](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/#class-bytes-rsplit "Copy/link to this API element")

```python
rsplit(self: Self, sep: Optional[bytes] = None, maxsplit: int = -1, Self: type)
```

splitlines[](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/#class-bytes-splitlines "Copy/link to this API element")

```python
splitlines(self: Self, keepends: bool = False, Self: type)
```

startswith[](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/#class-bytes-startswith "Copy/link to this API element")

```python
startswith(self: Self, prefix: bytes, start: int = 0, end: Optional[int] = None, Self: type)
```

endswith[](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/#class-bytes-endswith "Copy/link to this API element")

```python
endswith(self: Self, suffix: bytes, start: int = 0, end: Optional[int] = None, Self: type)
```

removeprefix[](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/#class-bytes-removeprefix "Copy/link to this API element")

```python
removeprefix(self: Self, prefix, Self: type)
```

removesuffix[](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/#class-bytes-removesuffix "Copy/link to this API element")

```python
removesuffix(self: Self, suffix, Self: type)
```

index[](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/#class-bytes-index "Copy/link to this API element")

```python
index(self: Self, sub: bytes, start: int = 0, end: Optional[int] = None, Self: type)
```

rindex[](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/#class-bytes-rindex "Copy/link to this API element")

```python
rindex(self: Self, sub: bytes, start: int = 0, end: Optional[int] = None, Self: type)
```

replace[](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/#class-bytes-replace "Copy/link to this API element")

```python
replace(self: Self, old: bytes, new: bytes, maxcount: int = -1, Self: type)
```

expandtabs[](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/#class-bytes-expandtabs "Copy/link to this API element")

```python
expandtabs(self: Self, tabsize: int = 8, Self: type)
```

translate[](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/#class-bytes-translate "Copy/link to this API element")

```python
translate(self: Self, map, Self: type)
```

```python
translate(self: Self, table: bytes, delete: bytes = "", Self: type)
```

Special methods 1

\_\_contains\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/#class-bytes-__contains__ "Copy/link to this API element")

```python
__contains__(self: Self, pattern: bytes, Self: type)
```

Source: https://docs.exaloop.io/libraries/api/modules/internal/types/bytes/
