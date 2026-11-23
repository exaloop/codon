# internal.types.str

Standard library Module [View source](https://github.com/exaloop/codon/blob/master/stdlib/internal/types/str.codon)

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

Extends [`internal.core.str`](https://docs.exaloop.io/libraries/api/classes/internal/core/str/) · [View complete class API →](https://docs.exaloop.io/libraries/api/classes/internal/core/str/)

Constructors 4

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/str/#class-str-__new__ "Copy/link to this API element")

@\_\_internal\_\_

```python
__new__(p: Ptr[u8], l: int) -> str
```

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/str/#class-str-__new__-overload-2 "Copy/link to this API element")

```python
__new__() -> str
```

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/str/#class-str-__new__-overload-3 "Copy/link to this API element")

```python
__new__(data: bytes, encoding: str, errors: str = "strict") -> str
```

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/str/#class-str-__new__-overload-4 "Copy/link to this API element")

```python
__new__(what) -> str
```

Methods 51

from\_ptr[](https://docs.exaloop.io/libraries/api/modules/internal/types/str/#class-str-from_ptr "Copy/link to this API element")

```python
from_ptr(t: cobj) -> str
```

find[](https://docs.exaloop.io/libraries/api/modules/internal/types/str/#class-str-find "Copy/link to this API element")

```python
find(self: Self, sub: str, start: Optional[int] = 0, end: Optional[int] = None, /) -> int
```

rfind[](https://docs.exaloop.io/libraries/api/modules/internal/types/str/#class-str-rfind "Copy/link to this API element")

```python
rfind(self: Self, sub: str, start: Optional[int] = 0, end: Optional[int] = None, /) -> int
```

index[](https://docs.exaloop.io/libraries/api/modules/internal/types/str/#class-str-index "Copy/link to this API element")

```python
index(self: Self, sub: str, start: Optional[int] = 0, end: Optional[int] = None, /) -> int
```

rindex[](https://docs.exaloop.io/libraries/api/modules/internal/types/str/#class-str-rindex "Copy/link to this API element")

```python
rindex(self: Self, sub: str, start: Optional[int] = 0, end: Optional[int] = None, /) -> int
```

count[](https://docs.exaloop.io/libraries/api/modules/internal/types/str/#class-str-count "Copy/link to this API element")

```python
count(self: Self, sub: str, start: Optional[int] = 0, end: Optional[int] = None, /) -> int
```

startswith[](https://docs.exaloop.io/libraries/api/modules/internal/types/str/#class-str-startswith "Copy/link to this API element")

```python
startswith(self: Self, prefix, start: Optional[int] = 0, end: Optional[int] = None, /) -> bool
```

```python
startswith(self: Self, prefix: str, start: Optional[int] = 0, end: Optional[int] = None, /) -> bool
```

endswith[](https://docs.exaloop.io/libraries/api/modules/internal/types/str/#class-str-endswith "Copy/link to this API element")

```python
endswith(self: Self, suffix, start: Optional[int] = 0, end: Optional[int] = None, /) -> bool
```

```python
endswith(self: Self, suffix: str, start: Optional[int] = 0, end: Optional[int] = None, /) -> bool
```

encode[](https://docs.exaloop.io/libraries/api/modules/internal/types/str/#class-str-encode "Copy/link to this API element")

```python
encode(self: Self, /, encoding: str = "utf-8", errors: str = "strict") -> bytes
```

c\_str[](https://docs.exaloop.io/libraries/api/modules/internal/types/str/#class-str-c_str "Copy/link to this API element")

```python
c_str(self: Self) -> cobj
```

maketrans[](https://docs.exaloop.io/libraries/api/modules/internal/types/str/#class-str-maketrans "Copy/link to this API element")

@staticmethod

```python
maketrans(x: dict[(int, int)], /) -> dict[(int, int)]
```

```python
maketrans(x: dict[(int, str)], /) -> dict[(int, str)]
```

```python
maketrans(x: dict[(int, Optional[int])], /) -> dict[(int, Optional[int])]
```

```python
maketrans(x: dict[(int, Optional[str])], /) -> dict[(int, Optional[str])]
```

```python
maketrans(x: str, y: str, /) -> dict[(int, int)]
```

```python
maketrans(x: str, y: str, z: str, /) -> dict[(int, Optional[int])]
```

translate[](https://docs.exaloop.io/libraries/api/modules/internal/types/str/#class-str-translate "Copy/link to this API element")

```python
translate(self: Self, table: dict[(int, int)], /) -> str
```

```python
translate(self: Self, table: dict[(int, str)], /) -> str
```

```python
translate(self: Self, table: dict[(int, Optional[int])], /) -> str
```

```python
translate(self: Self, table: dict[(int, Optional[str])], /) -> str
```

lower[](https://docs.exaloop.io/libraries/api/modules/internal/types/str/#class-str-lower "Copy/link to this API element")

```python
lower(self: Self, /) -> str
```

upper[](https://docs.exaloop.io/libraries/api/modules/internal/types/str/#class-str-upper "Copy/link to this API element")

```python
upper(self: Self, /) -> str
```

casefold[](https://docs.exaloop.io/libraries/api/modules/internal/types/str/#class-str-casefold "Copy/link to this API element")

```python
casefold(self: Self, /) -> str
```

capitalize[](https://docs.exaloop.io/libraries/api/modules/internal/types/str/#class-str-capitalize "Copy/link to this API element")

```python
capitalize(self: Self, /) -> str
```

title[](https://docs.exaloop.io/libraries/api/modules/internal/types/str/#class-str-title "Copy/link to this API element")

```python
title(self: Self, /) -> str
```

swapcase[](https://docs.exaloop.io/libraries/api/modules/internal/types/str/#class-str-swapcase "Copy/link to this API element")

```python
swapcase(self: Self, /) -> str
```

removeprefix[](https://docs.exaloop.io/libraries/api/modules/internal/types/str/#class-str-removeprefix "Copy/link to this API element")

```python
removeprefix(self: Self, prefix: str, /) -> str
```

removesuffix[](https://docs.exaloop.io/libraries/api/modules/internal/types/str/#class-str-removesuffix "Copy/link to this API element")

```python
removesuffix(self: Self, suffix: str, /) -> str
```

strip[](https://docs.exaloop.io/libraries/api/modules/internal/types/str/#class-str-strip "Copy/link to this API element")

```python
strip(self: Self, chars: Optional[str] = None, /) -> str
```

lstrip[](https://docs.exaloop.io/libraries/api/modules/internal/types/str/#class-str-lstrip "Copy/link to this API element")

```python
lstrip(self: Self, chars: Optional[str] = None, /) -> str
```

rstrip[](https://docs.exaloop.io/libraries/api/modules/internal/types/str/#class-str-rstrip "Copy/link to this API element")

```python
rstrip(self: Self, chars: Optional[str] = None, /) -> str
```

split[](https://docs.exaloop.io/libraries/api/modules/internal/types/str/#class-str-split "Copy/link to this API element")

```python
split(self: Self, /, sep: Optional[str] = None, maxsplit: int = -1) -> List[str]
```

rsplit[](https://docs.exaloop.io/libraries/api/modules/internal/types/str/#class-str-rsplit "Copy/link to this API element")

```python
rsplit(self: Self, /, sep: Optional[str] = None, maxsplit: int = -1) -> List[str]
```

splitlines[](https://docs.exaloop.io/libraries/api/modules/internal/types/str/#class-str-splitlines "Copy/link to this API element")

```python
splitlines(self: Self, /, keepends: bool = False) -> List[str]
```

join[](https://docs.exaloop.io/libraries/api/modules/internal/types/str/#class-str-join "Copy/link to this API element")

```python
join(self: Self, values, /) -> str
```

```python
join(self: Self, values: List[str], /) -> str
```

replace[](https://docs.exaloop.io/libraries/api/modules/internal/types/str/#class-str-replace "Copy/link to this API element")

```python
replace(self: Self, old: str, new: str, /, count: int = -1) -> str
```

ljust[](https://docs.exaloop.io/libraries/api/modules/internal/types/str/#class-str-ljust "Copy/link to this API element")

```python
ljust(self: Self, width: int, /) -> str
```

```python
ljust(self: Self, width: int, fillchar: str, /) -> str
```

rjust[](https://docs.exaloop.io/libraries/api/modules/internal/types/str/#class-str-rjust "Copy/link to this API element")

```python
rjust(self: Self, width: int, /) -> str
```

```python
rjust(self: Self, width: int, fillchar: str, /) -> str
```

center[](https://docs.exaloop.io/libraries/api/modules/internal/types/str/#class-str-center "Copy/link to this API element")

```python
center(self: Self, width: int, fillchar: str = " ", /) -> str
```

zfill[](https://docs.exaloop.io/libraries/api/modules/internal/types/str/#class-str-zfill "Copy/link to this API element")

```python
zfill(self: Self, width: int, /) -> str
```

expandtabs[](https://docs.exaloop.io/libraries/api/modules/internal/types/str/#class-str-expandtabs "Copy/link to this API element")

```python
expandtabs(self: Self, /, tabsize: int = 8) -> str
```

isalnum[](https://docs.exaloop.io/libraries/api/modules/internal/types/str/#class-str-isalnum "Copy/link to this API element")

```python
isalnum(self: Self, /) -> bool
```

isalpha[](https://docs.exaloop.io/libraries/api/modules/internal/types/str/#class-str-isalpha "Copy/link to this API element")

```python
isalpha(self: Self, /) -> bool
```

isdigit[](https://docs.exaloop.io/libraries/api/modules/internal/types/str/#class-str-isdigit "Copy/link to this API element")

```python
isdigit(self: Self, /) -> bool
```

isspace[](https://docs.exaloop.io/libraries/api/modules/internal/types/str/#class-str-isspace "Copy/link to this API element")

```python
isspace(self: Self, /) -> bool
```

isascii[](https://docs.exaloop.io/libraries/api/modules/internal/types/str/#class-str-isascii "Copy/link to this API element")

```python
isascii(self: Self, /) -> bool
```

islower[](https://docs.exaloop.io/libraries/api/modules/internal/types/str/#class-str-islower "Copy/link to this API element")

```python
islower(self: Self, /) -> bool
```

isupper[](https://docs.exaloop.io/libraries/api/modules/internal/types/str/#class-str-isupper "Copy/link to this API element")

```python
isupper(self: Self, /) -> bool
```

istitle[](https://docs.exaloop.io/libraries/api/modules/internal/types/str/#class-str-istitle "Copy/link to this API element")

```python
istitle(self: Self, /) -> bool
```

isnumeric[](https://docs.exaloop.io/libraries/api/modules/internal/types/str/#class-str-isnumeric "Copy/link to this API element")

```python
isnumeric(self: Self, /) -> bool
```

isdecimal[](https://docs.exaloop.io/libraries/api/modules/internal/types/str/#class-str-isdecimal "Copy/link to this API element")

```python
isdecimal(self: Self, /) -> bool
```

isprintable[](https://docs.exaloop.io/libraries/api/modules/internal/types/str/#class-str-isprintable "Copy/link to this API element")

```python
isprintable(self: Self, /) -> bool
```

isidentifier[](https://docs.exaloop.io/libraries/api/modules/internal/types/str/#class-str-isidentifier "Copy/link to this API element")

```python
isidentifier(self: Self, /) -> bool
```

partition[](https://docs.exaloop.io/libraries/api/modules/internal/types/str/#class-str-partition "Copy/link to this API element")

```python
partition(self: Self, sep: str, /) -> Tuple[(str, str, str)]
```

rpartition[](https://docs.exaloop.io/libraries/api/modules/internal/types/str/#class-str-rpartition "Copy/link to this API element")

```python
rpartition(self: Self, sep: str, /) -> Tuple[(str, str, str)]
```

cat[](https://docs.exaloop.io/libraries/api/modules/internal/types/str/#class-str-cat "Copy/link to this API element")

```python
cat(*args) -> str
```

memcpy[](https://docs.exaloop.io/libraries/api/modules/internal/types/str/#class-str-memcpy "Copy/link to this API element")

@llvm

```python
memcpy(dest: Ptr[u8], src: Ptr[u8], len: int) -> None
```

```llvm
declare void @llvm.memcpy.p0i8.p0i8.i64(ptr %dest, ptr %src, i64 %len, i32 %align, i1 %isvolatile)
call void @llvm.memcpy.p0i8.p0i8.i64(ptr %dest, ptr %src, i64 %len, i32 0, i1 false)
ret {} {}
```

memmove[](https://docs.exaloop.io/libraries/api/modules/internal/types/str/#class-str-memmove "Copy/link to this API element")

@llvm

```python
memmove(dest: Ptr[u8], src: Ptr[u8], len: int) -> None
```

```llvm
declare void @llvm.memmove.p0i8.p0i8.i64(ptr %dest, ptr %src, i64 %len, i32 %align, i1 %isvolatile)
call void @llvm.memmove.p0i8.p0i8.i64(ptr %dest, ptr %src, i64 %len, i32 0, i1 false)
ret {} {}
```

memset[](https://docs.exaloop.io/libraries/api/modules/internal/types/str/#class-str-memset "Copy/link to this API element")

@llvm

```python
memset(dest: Ptr[u8], val: u8, len: int) -> None
```

```llvm
declare void @llvm.memset.p0i8.i64(ptr %dest, i8 %val, i64 %len, i32 %align, i1 %isvolatile)
call void @llvm.memset.p0i8.i64(ptr %dest, i8 %val, i64 %len, i32 0, i1 false)
ret {} {}
```

Special methods 21

\_\_len\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/str/#class-str-__len__ "Copy/link to this API element")

```python
__len__(self: Self, /) -> int
```

\_\_bool\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/str/#class-str-__bool__ "Copy/link to this API element")

```python
__bool__(self: Self) -> bool
```

\_\_match\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/str/#class-str-__match__ "Copy/link to this API element")

```python
__match__(self: Self, other: str) -> bool
```

\_\_getitem\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/str/#class-str-__getitem__ "Copy/link to this API element")

```python
__getitem__(self: Self, item: int, /)
```

```python
__getitem__(self: Self, item: slice[(int, int, int)], /) -> str
```

\_\_eq\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/str/#class-str-__eq__ "Copy/link to this API element")

```python
__eq__(self: Self, other: str, /) -> bool
```

\_\_ne\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/str/#class-str-__ne__ "Copy/link to this API element")

```python
__ne__(self: Self, other: str, /) -> bool
```

\_\_lt\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/str/#class-str-__lt__ "Copy/link to this API element")

```python
__lt__(self: Self, other: str, /) -> bool
```

\_\_le\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/str/#class-str-__le__ "Copy/link to this API element")

```python
__le__(self: Self, other: str, /) -> bool
```

\_\_gt\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/str/#class-str-__gt__ "Copy/link to this API element")

```python
__gt__(self: Self, other: str, /) -> bool
```

\_\_ge\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/str/#class-str-__ge__ "Copy/link to this API element")

```python
__ge__(self: Self, other: str, /) -> bool
```

\_\_add\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/str/#class-str-__add__ "Copy/link to this API element")

```python
__add__(self: Self, other: str, /) -> str
```

\_\_mul\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/str/#class-str-__mul__ "Copy/link to this API element")

```python
__mul__(self: Self, count: int, /) -> str
```

\_\_rmul\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/str/#class-str-__rmul__ "Copy/link to this API element")

```python
__rmul__(self: Self, count: int, /) -> str
```

\_\_repr\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/str/#class-str-__repr__ "Copy/link to this API element")

```python
__repr__(self: Self, /) -> str
```

\_\_str\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/str/#class-str-__str__ "Copy/link to this API element")

```python
__str__(self: Self, /) -> str
```

\_\_contains\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/str/#class-str-__contains__ "Copy/link to this API element")

```python
__contains__(self: Self, sub: str, /) -> bool
```

\_\_copy\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/str/#class-str-__copy__ "Copy/link to this API element")

```python
__copy__(self: Self) -> str
```

\_\_deepcopy\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/str/#class-str-__deepcopy__ "Copy/link to this API element")

```python
__deepcopy__(self: Self) -> str
```

\_\_ptrcopy\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/str/#class-str-__ptrcopy__ "Copy/link to this API element")

```python
__ptrcopy__(self: Self) -> str
```

\_\_hash\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/str/#class-str-__hash__ "Copy/link to this API element")

```python
__hash__(self: Self, /) -> int
```

\_\_iter\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/str/#class-str-__iter__ "Copy/link to this API element")

```python
__iter__(self: Self, /)
```

Source: https://docs.exaloop.io/libraries/api/modules/internal/types/str/
