# internal.types.str

Standard library module [View source](https://github.com/exaloop/codon/blob/master/stdlib/internal/types/str.codon)

variable KIND\_SHIFT[](https://docs.exaloop.io/libraries/api/modules/internal/types/str/#variable-kind_shift "Copy/link to this API element")

```python
KIND_SHIFT: Literal[int] = 56
```

variable LEN\_MASK[](https://docs.exaloop.io/libraries/api/modules/internal/types/str/#variable-len_mask "Copy/link to this API element")

```python
LEN_MASK: Literal[int] = ((1 << KIND_SHIFT) - 1)
```

variable KIND\_MASK[](https://docs.exaloop.io/libraries/api/modules/internal/types/str/#variable-kind_mask "Copy/link to this API element")

```python
KIND_MASK: Literal[int] = (3 << KIND_SHIFT)
```

variable KIND\_ASCII[](https://docs.exaloop.io/libraries/api/modules/internal/types/str/#variable-kind_ascii "Copy/link to this API element")

```python
KIND_ASCII: Literal[int] = 0
```

variable KIND\_LATIN1[](https://docs.exaloop.io/libraries/api/modules/internal/types/str/#variable-kind_latin1 "Copy/link to this API element")

```python
KIND_LATIN1: Literal[int] = 1
```

variable KIND\_UCS2[](https://docs.exaloop.io/libraries/api/modules/internal/types/str/#variable-kind_ucs2 "Copy/link to this API element")

```python
KIND_UCS2: Literal[int] = 2
```

variable KIND\_UCS4[](https://docs.exaloop.io/libraries/api/modules/internal/types/str/#variable-kind_ucs4 "Copy/link to this API element")

```python
KIND_UCS4: Literal[int] = 3
```

variable UTF8\_CONT\_MASK[](https://docs.exaloop.io/libraries/api/modules/internal/types/str/#variable-utf8_cont_mask "Copy/link to this API element")

```python
UTF8_CONT_MASK: Literal[int] = 0xC0
```

variable UTF8\_CONT\_TAG[](https://docs.exaloop.io/libraries/api/modules/internal/types/str/#variable-utf8_cont_tag "Copy/link to this API element")

```python
UTF8_CONT_TAG: Literal[int] = 0x80
```

variable UNICODE\_MAX[](https://docs.exaloop.io/libraries/api/modules/internal/types/str/#variable-unicode_max "Copy/link to this API element")

```python
UNICODE_MAX: Literal[int] = 0x10FFFF
```

variable SURROGATE\_MIN[](https://docs.exaloop.io/libraries/api/modules/internal/types/str/#variable-surrogate_min "Copy/link to this API element")

```python
SURROGATE_MIN: Literal[int] = 0xD800
```

variable SURROGATE\_MAX[](https://docs.exaloop.io/libraries/api/modules/internal/types/str/#variable-surrogate_max "Copy/link to this API element")

```python
SURROGATE_MAX: Literal[int] = 0xDFFF
```

class str[](https://docs.exaloop.io/libraries/api/modules/internal/types/str/#class-str "Copy/link to this API element")

@extend

```python
class str
```

Extends [`str`](https://docs.exaloop.io/libraries/api/classes/str/) · [View complete class API →](https://docs.exaloop.io/libraries/api/classes/str/)

Constructors 4

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/str/#class-str-__new__ "Copy/link to this API element")

@\_\_internal\_\_

```python
__new__(p: Ptr[u8], l: int)
```

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/str/#class-str-__new__-overload-2 "Copy/link to this API element")

```python
__new__()
```

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/str/#class-str-__new__-overload-3 "Copy/link to this API element")

```python
__new__(data: bytes, encoding: str, errors: str = "strict")
```

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/str/#class-str-__new__-overload-4 "Copy/link to this API element")

```python
__new__(what)
```

Methods 51

from\_ptr[](https://docs.exaloop.io/libraries/api/modules/internal/types/str/#class-str-from_ptr "Copy/link to this API element")

```python
from_ptr(t: cobj)
```

find[](https://docs.exaloop.io/libraries/api/modules/internal/types/str/#class-str-find "Copy/link to this API element")

```python
find(self: Self, sub: str, Self: type)
```

```python
find(self: Self, sub: str, start: int, Self: type)
```

```python
find(self: Self, sub: str, start: int, end: int, Self: type)
```

rfind[](https://docs.exaloop.io/libraries/api/modules/internal/types/str/#class-str-rfind "Copy/link to this API element")

```python
rfind(self: Self, sub: str, Self: type)
```

```python
rfind(self: Self, sub: str, start: int, Self: type)
```

```python
rfind(self: Self, sub: str, start: int, end: int, Self: type)
```

index[](https://docs.exaloop.io/libraries/api/modules/internal/types/str/#class-str-index "Copy/link to this API element")

```python
index(self: Self, sub: str, Self: type)
```

```python
index(self: Self, sub: str, start: int, Self: type)
```

```python
index(self: Self, sub: str, start: int, end: int, Self: type)
```

rindex[](https://docs.exaloop.io/libraries/api/modules/internal/types/str/#class-str-rindex "Copy/link to this API element")

```python
rindex(self: Self, sub: str, Self: type)
```

```python
rindex(self: Self, sub: str, start: int, Self: type)
```

```python
rindex(self: Self, sub: str, start: int, end: int, Self: type)
```

count[](https://docs.exaloop.io/libraries/api/modules/internal/types/str/#class-str-count "Copy/link to this API element")

```python
count(self: Self, sub: str, Self: type)
```

```python
count(self: Self, sub: str, start: int, Self: type)
```

```python
count(self: Self, sub: str, start: int, end: int, Self: type)
```

startswith[](https://docs.exaloop.io/libraries/api/modules/internal/types/str/#class-str-startswith "Copy/link to this API element")

```python
startswith(self: Self, prefix: str, Self: type)
```

```python
startswith(self: Self, prefix: str, start: int, Self: type)
```

```python
startswith(self: Self, prefix: str, start: int, end: int, Self: type)
```

endswith[](https://docs.exaloop.io/libraries/api/modules/internal/types/str/#class-str-endswith "Copy/link to this API element")

```python
endswith(self: Self, suffix: str, Self: type)
```

```python
endswith(self: Self, suffix: str, start: int, Self: type)
```

```python
endswith(self: Self, suffix: str, start: int, end: int, Self: type)
```

encode[](https://docs.exaloop.io/libraries/api/modules/internal/types/str/#class-str-encode "Copy/link to this API element")

```python
encode(self: Self, encoding: str = "utf-8", errors: str = "strict", Self: type)
```

c\_str[](https://docs.exaloop.io/libraries/api/modules/internal/types/str/#class-str-c_str "Copy/link to this API element")

```python
c_str(self: Self, Self: type)
```

maketrans[](https://docs.exaloop.io/libraries/api/modules/internal/types/str/#class-str-maketrans "Copy/link to this API element")

@staticmethod

```python
maketrans(x: OrderedDict[int, int])
```

```python
maketrans(x: OrderedDict[int, str])
```

```python
maketrans(x: OrderedDict[int, Optional[int]])
```

```python
maketrans(x: OrderedDict[int, Optional[str]])
```

```python
maketrans(x: str, y: str)
```

```python
maketrans(x: str, y: str, z: str)
```

translate[](https://docs.exaloop.io/libraries/api/modules/internal/types/str/#class-str-translate "Copy/link to this API element")

```python
translate(self: Self, table: OrderedDict[int, int], Self: type)
```

```python
translate(self: Self, table: OrderedDict[int, str], Self: type)
```

```python
translate(self: Self, table: OrderedDict[int, Optional[int]], Self: type)
```

```python
translate(self: Self, table: OrderedDict[int, Optional[str]], Self: type)
```

lower[](https://docs.exaloop.io/libraries/api/modules/internal/types/str/#class-str-lower "Copy/link to this API element")

```python
lower(self: Self, Self: type)
```

upper[](https://docs.exaloop.io/libraries/api/modules/internal/types/str/#class-str-upper "Copy/link to this API element")

```python
upper(self: Self, Self: type)
```

casefold[](https://docs.exaloop.io/libraries/api/modules/internal/types/str/#class-str-casefold "Copy/link to this API element")

```python
casefold(self: Self, Self: type)
```

capitalize[](https://docs.exaloop.io/libraries/api/modules/internal/types/str/#class-str-capitalize "Copy/link to this API element")

```python
capitalize(self: Self, Self: type)
```

title[](https://docs.exaloop.io/libraries/api/modules/internal/types/str/#class-str-title "Copy/link to this API element")

```python
title(self: Self, Self: type)
```

swapcase[](https://docs.exaloop.io/libraries/api/modules/internal/types/str/#class-str-swapcase "Copy/link to this API element")

```python
swapcase(self: Self, Self: type)
```

removeprefix[](https://docs.exaloop.io/libraries/api/modules/internal/types/str/#class-str-removeprefix "Copy/link to this API element")

```python
removeprefix(self: Self, prefix: str, Self: type)
```

removesuffix[](https://docs.exaloop.io/libraries/api/modules/internal/types/str/#class-str-removesuffix "Copy/link to this API element")

```python
removesuffix(self: Self, suffix: str, Self: type)
```

strip[](https://docs.exaloop.io/libraries/api/modules/internal/types/str/#class-str-strip "Copy/link to this API element")

```python
strip(self: Self, chars: Optional[str] = None, Self: type)
```

lstrip[](https://docs.exaloop.io/libraries/api/modules/internal/types/str/#class-str-lstrip "Copy/link to this API element")

```python
lstrip(self: Self, chars: Optional[str] = None, Self: type)
```

rstrip[](https://docs.exaloop.io/libraries/api/modules/internal/types/str/#class-str-rstrip "Copy/link to this API element")

```python
rstrip(self: Self, chars: Optional[str] = None, Self: type)
```

split[](https://docs.exaloop.io/libraries/api/modules/internal/types/str/#class-str-split "Copy/link to this API element")

```python
split(self: Self, sep: Optional[str] = None, maxsplit: int = -1, Self: type)
```

rsplit[](https://docs.exaloop.io/libraries/api/modules/internal/types/str/#class-str-rsplit "Copy/link to this API element")

```python
rsplit(self: Self, sep: Optional[str] = None, maxsplit: int = -1, Self: type)
```

splitlines[](https://docs.exaloop.io/libraries/api/modules/internal/types/str/#class-str-splitlines "Copy/link to this API element")

```python
splitlines(self: Self, keepends: bool = False, Self: type)
```

join[](https://docs.exaloop.io/libraries/api/modules/internal/types/str/#class-str-join "Copy/link to this API element")

```python
join(self: Self, values, Self: type)
```

```python
join(self: Self, values: List[str], Self: type)
```

replace[](https://docs.exaloop.io/libraries/api/modules/internal/types/str/#class-str-replace "Copy/link to this API element")

```python
replace(self: Self, old: str, new: str, count: int = -1, Self: type)
```

center[](https://docs.exaloop.io/libraries/api/modules/internal/types/str/#class-str-center "Copy/link to this API element")

```python
center(self: Self, width: int, Self: type)
```

```python
center(self: Self, width: int, fillchar: str, Self: type)
```

ljust[](https://docs.exaloop.io/libraries/api/modules/internal/types/str/#class-str-ljust "Copy/link to this API element")

```python
ljust(self: Self, width: int, Self: type)
```

```python
ljust(self: Self, width: int, fillchar: str, Self: type)
```

rjust[](https://docs.exaloop.io/libraries/api/modules/internal/types/str/#class-str-rjust "Copy/link to this API element")

```python
rjust(self: Self, width: int, Self: type)
```

```python
rjust(self: Self, width: int, fillchar: str, Self: type)
```

zfill[](https://docs.exaloop.io/libraries/api/modules/internal/types/str/#class-str-zfill "Copy/link to this API element")

```python
zfill(self: Self, width: int, Self: type)
```

expandtabs[](https://docs.exaloop.io/libraries/api/modules/internal/types/str/#class-str-expandtabs "Copy/link to this API element")

```python
expandtabs(self: Self, tabsize: int = 8, Self: type)
```

isalnum[](https://docs.exaloop.io/libraries/api/modules/internal/types/str/#class-str-isalnum "Copy/link to this API element")

```python
isalnum(self: Self, Self: type)
```

isalpha[](https://docs.exaloop.io/libraries/api/modules/internal/types/str/#class-str-isalpha "Copy/link to this API element")

```python
isalpha(self: Self, Self: type)
```

isdigit[](https://docs.exaloop.io/libraries/api/modules/internal/types/str/#class-str-isdigit "Copy/link to this API element")

```python
isdigit(self: Self, Self: type)
```

isspace[](https://docs.exaloop.io/libraries/api/modules/internal/types/str/#class-str-isspace "Copy/link to this API element")

```python
isspace(self: Self, Self: type)
```

isascii[](https://docs.exaloop.io/libraries/api/modules/internal/types/str/#class-str-isascii "Copy/link to this API element")

```python
isascii(self: Self, Self: type)
```

islower[](https://docs.exaloop.io/libraries/api/modules/internal/types/str/#class-str-islower "Copy/link to this API element")

```python
islower(self: Self, Self: type)
```

isupper[](https://docs.exaloop.io/libraries/api/modules/internal/types/str/#class-str-isupper "Copy/link to this API element")

```python
isupper(self: Self, Self: type)
```

istitle[](https://docs.exaloop.io/libraries/api/modules/internal/types/str/#class-str-istitle "Copy/link to this API element")

```python
istitle(self: Self, Self: type)
```

isnumeric[](https://docs.exaloop.io/libraries/api/modules/internal/types/str/#class-str-isnumeric "Copy/link to this API element")

```python
isnumeric(self: Self, Self: type)
```

isdecimal[](https://docs.exaloop.io/libraries/api/modules/internal/types/str/#class-str-isdecimal "Copy/link to this API element")

```python
isdecimal(self: Self, Self: type)
```

isprintable[](https://docs.exaloop.io/libraries/api/modules/internal/types/str/#class-str-isprintable "Copy/link to this API element")

```python
isprintable(self: Self, Self: type)
```

isidentifier[](https://docs.exaloop.io/libraries/api/modules/internal/types/str/#class-str-isidentifier "Copy/link to this API element")

```python
isidentifier(self: Self, Self: type)
```

partition[](https://docs.exaloop.io/libraries/api/modules/internal/types/str/#class-str-partition "Copy/link to this API element")

```python
partition(self: Self, sep: str, Self: type)
```

rpartition[](https://docs.exaloop.io/libraries/api/modules/internal/types/str/#class-str-rpartition "Copy/link to this API element")

```python
rpartition(self: Self, sep: str, Self: type)
```

cat[](https://docs.exaloop.io/libraries/api/modules/internal/types/str/#class-str-cat "Copy/link to this API element")

```python
cat(*args)
```

memcpy[](https://docs.exaloop.io/libraries/api/modules/internal/types/str/#class-str-memcpy "Copy/link to this API element")

@llvm

```python
memcpy(dest: Ptr[u8], src: Ptr[u8], len: int)
```

```llvm
declare void @llvm.memcpy.p0i8.p0i8.i64(ptr %dest, ptr %src, i64 %len, i32 %align, i1 %isvolatile)
call void @llvm.memcpy.p0i8.p0i8.i64(ptr %dest, ptr %src, i64 %len, i32 0, i1 false)
ret {} {}
```

memmove[](https://docs.exaloop.io/libraries/api/modules/internal/types/str/#class-str-memmove "Copy/link to this API element")

@llvm

```python
memmove(dest: Ptr[u8], src: Ptr[u8], len: int)
```

```llvm
declare void @llvm.memmove.p0i8.p0i8.i64(ptr %dest, ptr %src, i64 %len, i32 %align, i1 %isvolatile)
call void @llvm.memmove.p0i8.p0i8.i64(ptr %dest, ptr %src, i64 %len, i32 0, i1 false)
ret {} {}
```

memset[](https://docs.exaloop.io/libraries/api/modules/internal/types/str/#class-str-memset "Copy/link to this API element")

@llvm

```python
memset(dest: Ptr[u8], val: u8, len: int)
```

```llvm
declare void @llvm.memset.p0i8.i64(ptr %dest, i8 %val, i64 %len, i32 %align, i1 %isvolatile)
call void @llvm.memset.p0i8.i64(ptr %dest, i8 %val, i64 %len, i32 0, i1 false)
ret {} {}
```

Special methods 21

\_\_len\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/str/#class-str-__len__ "Copy/link to this API element")

```python
__len__(self: Self, Self: type)
```

\_\_bool\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/str/#class-str-__bool__ "Copy/link to this API element")

```python
__bool__(self: Self, Self: type)
```

\_\_match\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/str/#class-str-__match__ "Copy/link to this API element")

```python
__match__(self: Self, other: str, Self: type)
```

\_\_getitem\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/str/#class-str-__getitem__ "Copy/link to this API element")

```python
__getitem__(self: Self, item: int, Self: type)
```

```python
__getitem__(self: Self, item: Slice[int, int, int], Self: type)
```

\_\_eq\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/str/#class-str-__eq__ "Copy/link to this API element")

```python
__eq__(self: Self, other: str, Self: type)
```

\_\_ne\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/str/#class-str-__ne__ "Copy/link to this API element")

```python
__ne__(self: Self, other: str, Self: type)
```

\_\_lt\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/str/#class-str-__lt__ "Copy/link to this API element")

```python
__lt__(self: Self, other: str, Self: type)
```

\_\_le\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/str/#class-str-__le__ "Copy/link to this API element")

```python
__le__(self: Self, other: str, Self: type)
```

\_\_gt\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/str/#class-str-__gt__ "Copy/link to this API element")

```python
__gt__(self: Self, other: str, Self: type)
```

\_\_ge\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/str/#class-str-__ge__ "Copy/link to this API element")

```python
__ge__(self: Self, other: str, Self: type)
```

\_\_add\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/str/#class-str-__add__ "Copy/link to this API element")

```python
__add__(self: Self, other: str, Self: type)
```

\_\_mul\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/str/#class-str-__mul__ "Copy/link to this API element")

```python
__mul__(self: Self, count: int, Self: type)
```

\_\_rmul\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/str/#class-str-__rmul__ "Copy/link to this API element")

```python
__rmul__(self: Self, count: int, Self: type)
```

\_\_repr\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/str/#class-str-__repr__ "Copy/link to this API element")

```python
__repr__(self: Self, Self: type)
```

\_\_str\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/str/#class-str-__str__ "Copy/link to this API element")

```python
__str__(self: Self, Self: type)
```

\_\_contains\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/str/#class-str-__contains__ "Copy/link to this API element")

```python
__contains__(self: Self, sub: str, Self: type)
```

\_\_copy\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/str/#class-str-__copy__ "Copy/link to this API element")

```python
__copy__(self: Self, Self: type)
```

\_\_deepcopy\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/str/#class-str-__deepcopy__ "Copy/link to this API element")

```python
__deepcopy__(self: Self, Self: type)
```

\_\_ptrcopy\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/str/#class-str-__ptrcopy__ "Copy/link to this API element")

```python
__ptrcopy__(self: Self, Self: type)
```

\_\_hash\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/str/#class-str-__hash__ "Copy/link to this API element")

```python
__hash__(self: Self, Self: type)
```

\_\_iter\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/str/#class-str-__iter__ "Copy/link to this API element")

```python
__iter__(self: Self, Self: type)
```

Source: https://docs.exaloop.io/libraries/api/modules/internal/types/str/
