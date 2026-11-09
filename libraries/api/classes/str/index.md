# str

Standard library class

class str[](https://docs.exaloop.io/libraries/api/classes/str/#class-str "Copy/link to this API element")

@tuple

```python
class str
```

Constructors 4

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/classes/str/#class-str-__new__ "Copy/link to this API element")

@\_\_internal\_\_

```python
__new__(p: Ptr[u8], l: int)
```

Added by [`internal.types.str`](https://docs.exaloop.io/libraries/api/modules/internal/types/str/)

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/classes/str/#class-str-__new__-overload-2 "Copy/link to this API element")

```python
__new__()
```

Added by [`internal.types.str`](https://docs.exaloop.io/libraries/api/modules/internal/types/str/)

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/classes/str/#class-str-__new__-overload-3 "Copy/link to this API element")

```python
__new__(data: bytes, encoding: str, errors: str = "strict")
```

Added by [`internal.types.str`](https://docs.exaloop.io/libraries/api/modules/internal/types/str/)

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/classes/str/#class-str-__new__-overload-4 "Copy/link to this API element")

```python
__new__(what)
```

Added by [`internal.types.str`](https://docs.exaloop.io/libraries/api/modules/internal/types/str/)

Methods 53

format[](https://docs.exaloop.io/libraries/api/classes/str/#class-str-format "Copy/link to this API element")

```python
format(self: Self, *values, Self: type)
```

Added by [`internal.format`](https://docs.exaloop.io/libraries/api/modules/internal/format/)

format\_map[](https://docs.exaloop.io/libraries/api/classes/str/#class-str-format_map "Copy/link to this API element")

```python
format_map(self: Self, mapping, Self: type)
```

Added by [`internal.format`](https://docs.exaloop.io/libraries/api/modules/internal/format/)

from\_ptr[](https://docs.exaloop.io/libraries/api/classes/str/#class-str-from_ptr "Copy/link to this API element")

```python
from_ptr(t: cobj)
```

Added by [`internal.types.str`](https://docs.exaloop.io/libraries/api/modules/internal/types/str/)

find[](https://docs.exaloop.io/libraries/api/classes/str/#class-str-find "Copy/link to this API element")

```python
find(self: Self, sub: str, Self: type)
```

Added by [`internal.types.str`](https://docs.exaloop.io/libraries/api/modules/internal/types/str/)

```python
find(self: Self, sub: str, start: int, Self: type)
```

Added by [`internal.types.str`](https://docs.exaloop.io/libraries/api/modules/internal/types/str/)

```python
find(self: Self, sub: str, start: int, end: int, Self: type)
```

Added by [`internal.types.str`](https://docs.exaloop.io/libraries/api/modules/internal/types/str/)

rfind[](https://docs.exaloop.io/libraries/api/classes/str/#class-str-rfind "Copy/link to this API element")

```python
rfind(self: Self, sub: str, Self: type)
```

Added by [`internal.types.str`](https://docs.exaloop.io/libraries/api/modules/internal/types/str/)

```python
rfind(self: Self, sub: str, start: int, Self: type)
```

Added by [`internal.types.str`](https://docs.exaloop.io/libraries/api/modules/internal/types/str/)

```python
rfind(self: Self, sub: str, start: int, end: int, Self: type)
```

Added by [`internal.types.str`](https://docs.exaloop.io/libraries/api/modules/internal/types/str/)

index[](https://docs.exaloop.io/libraries/api/classes/str/#class-str-index "Copy/link to this API element")

```python
index(self: Self, sub: str, Self: type)
```

Added by [`internal.types.str`](https://docs.exaloop.io/libraries/api/modules/internal/types/str/)

```python
index(self: Self, sub: str, start: int, Self: type)
```

Added by [`internal.types.str`](https://docs.exaloop.io/libraries/api/modules/internal/types/str/)

```python
index(self: Self, sub: str, start: int, end: int, Self: type)
```

Added by [`internal.types.str`](https://docs.exaloop.io/libraries/api/modules/internal/types/str/)

rindex[](https://docs.exaloop.io/libraries/api/classes/str/#class-str-rindex "Copy/link to this API element")

```python
rindex(self: Self, sub: str, Self: type)
```

Added by [`internal.types.str`](https://docs.exaloop.io/libraries/api/modules/internal/types/str/)

```python
rindex(self: Self, sub: str, start: int, Self: type)
```

Added by [`internal.types.str`](https://docs.exaloop.io/libraries/api/modules/internal/types/str/)

```python
rindex(self: Self, sub: str, start: int, end: int, Self: type)
```

Added by [`internal.types.str`](https://docs.exaloop.io/libraries/api/modules/internal/types/str/)

count[](https://docs.exaloop.io/libraries/api/classes/str/#class-str-count "Copy/link to this API element")

```python
count(self: Self, sub: str, Self: type)
```

Added by [`internal.types.str`](https://docs.exaloop.io/libraries/api/modules/internal/types/str/)

```python
count(self: Self, sub: str, start: int, Self: type)
```

Added by [`internal.types.str`](https://docs.exaloop.io/libraries/api/modules/internal/types/str/)

```python
count(self: Self, sub: str, start: int, end: int, Self: type)
```

Added by [`internal.types.str`](https://docs.exaloop.io/libraries/api/modules/internal/types/str/)

startswith[](https://docs.exaloop.io/libraries/api/classes/str/#class-str-startswith "Copy/link to this API element")

```python
startswith(self: Self, prefix: str, Self: type)
```

Added by [`internal.types.str`](https://docs.exaloop.io/libraries/api/modules/internal/types/str/)

```python
startswith(self: Self, prefix: str, start: int, Self: type)
```

Added by [`internal.types.str`](https://docs.exaloop.io/libraries/api/modules/internal/types/str/)

```python
startswith(self: Self, prefix: str, start: int, end: int, Self: type)
```

Added by [`internal.types.str`](https://docs.exaloop.io/libraries/api/modules/internal/types/str/)

endswith[](https://docs.exaloop.io/libraries/api/classes/str/#class-str-endswith "Copy/link to this API element")

```python
endswith(self: Self, suffix: str, Self: type)
```

Added by [`internal.types.str`](https://docs.exaloop.io/libraries/api/modules/internal/types/str/)

```python
endswith(self: Self, suffix: str, start: int, Self: type)
```

Added by [`internal.types.str`](https://docs.exaloop.io/libraries/api/modules/internal/types/str/)

```python
endswith(self: Self, suffix: str, start: int, end: int, Self: type)
```

Added by [`internal.types.str`](https://docs.exaloop.io/libraries/api/modules/internal/types/str/)

encode[](https://docs.exaloop.io/libraries/api/classes/str/#class-str-encode "Copy/link to this API element")

```python
encode(self: Self, encoding: str = "utf-8", errors: str = "strict", Self: type)
```

Added by [`internal.types.str`](https://docs.exaloop.io/libraries/api/modules/internal/types/str/)

c\_str[](https://docs.exaloop.io/libraries/api/classes/str/#class-str-c_str "Copy/link to this API element")

```python
c_str(self: Self, Self: type)
```

Added by [`internal.types.str`](https://docs.exaloop.io/libraries/api/modules/internal/types/str/)

maketrans[](https://docs.exaloop.io/libraries/api/classes/str/#class-str-maketrans "Copy/link to this API element")

@staticmethod

```python
maketrans(x: OrderedDict[int, int])
```

Added by [`internal.types.str`](https://docs.exaloop.io/libraries/api/modules/internal/types/str/)

```python
maketrans(x: OrderedDict[int, str])
```

Added by [`internal.types.str`](https://docs.exaloop.io/libraries/api/modules/internal/types/str/)

```python
maketrans(x: OrderedDict[int, Optional[int]])
```

Added by [`internal.types.str`](https://docs.exaloop.io/libraries/api/modules/internal/types/str/)

```python
maketrans(x: OrderedDict[int, Optional[str]])
```

Added by [`internal.types.str`](https://docs.exaloop.io/libraries/api/modules/internal/types/str/)

```python
maketrans(x: str, y: str)
```

Added by [`internal.types.str`](https://docs.exaloop.io/libraries/api/modules/internal/types/str/)

```python
maketrans(x: str, y: str, z: str)
```

Added by [`internal.types.str`](https://docs.exaloop.io/libraries/api/modules/internal/types/str/)

translate[](https://docs.exaloop.io/libraries/api/classes/str/#class-str-translate "Copy/link to this API element")

```python
translate(self: Self, table: OrderedDict[int, int], Self: type)
```

Added by [`internal.types.str`](https://docs.exaloop.io/libraries/api/modules/internal/types/str/)

```python
translate(self: Self, table: OrderedDict[int, str], Self: type)
```

Added by [`internal.types.str`](https://docs.exaloop.io/libraries/api/modules/internal/types/str/)

```python
translate(self: Self, table: OrderedDict[int, Optional[int]], Self: type)
```

Added by [`internal.types.str`](https://docs.exaloop.io/libraries/api/modules/internal/types/str/)

```python
translate(self: Self, table: OrderedDict[int, Optional[str]], Self: type)
```

Added by [`internal.types.str`](https://docs.exaloop.io/libraries/api/modules/internal/types/str/)

lower[](https://docs.exaloop.io/libraries/api/classes/str/#class-str-lower "Copy/link to this API element")

```python
lower(self: Self, Self: type)
```

Added by [`internal.types.str`](https://docs.exaloop.io/libraries/api/modules/internal/types/str/)

upper[](https://docs.exaloop.io/libraries/api/classes/str/#class-str-upper "Copy/link to this API element")

```python
upper(self: Self, Self: type)
```

Added by [`internal.types.str`](https://docs.exaloop.io/libraries/api/modules/internal/types/str/)

casefold[](https://docs.exaloop.io/libraries/api/classes/str/#class-str-casefold "Copy/link to this API element")

```python
casefold(self: Self, Self: type)
```

Added by [`internal.types.str`](https://docs.exaloop.io/libraries/api/modules/internal/types/str/)

capitalize[](https://docs.exaloop.io/libraries/api/classes/str/#class-str-capitalize "Copy/link to this API element")

```python
capitalize(self: Self, Self: type)
```

Added by [`internal.types.str`](https://docs.exaloop.io/libraries/api/modules/internal/types/str/)

title[](https://docs.exaloop.io/libraries/api/classes/str/#class-str-title "Copy/link to this API element")

```python
title(self: Self, Self: type)
```

Added by [`internal.types.str`](https://docs.exaloop.io/libraries/api/modules/internal/types/str/)

swapcase[](https://docs.exaloop.io/libraries/api/classes/str/#class-str-swapcase "Copy/link to this API element")

```python
swapcase(self: Self, Self: type)
```

Added by [`internal.types.str`](https://docs.exaloop.io/libraries/api/modules/internal/types/str/)

removeprefix[](https://docs.exaloop.io/libraries/api/classes/str/#class-str-removeprefix "Copy/link to this API element")

```python
removeprefix(self: Self, prefix: str, Self: type)
```

Added by [`internal.types.str`](https://docs.exaloop.io/libraries/api/modules/internal/types/str/)

removesuffix[](https://docs.exaloop.io/libraries/api/classes/str/#class-str-removesuffix "Copy/link to this API element")

```python
removesuffix(self: Self, suffix: str, Self: type)
```

Added by [`internal.types.str`](https://docs.exaloop.io/libraries/api/modules/internal/types/str/)

strip[](https://docs.exaloop.io/libraries/api/classes/str/#class-str-strip "Copy/link to this API element")

```python
strip(self: Self, chars: Optional[str] = None, Self: type)
```

Added by [`internal.types.str`](https://docs.exaloop.io/libraries/api/modules/internal/types/str/)

lstrip[](https://docs.exaloop.io/libraries/api/classes/str/#class-str-lstrip "Copy/link to this API element")

```python
lstrip(self: Self, chars: Optional[str] = None, Self: type)
```

Added by [`internal.types.str`](https://docs.exaloop.io/libraries/api/modules/internal/types/str/)

rstrip[](https://docs.exaloop.io/libraries/api/classes/str/#class-str-rstrip "Copy/link to this API element")

```python
rstrip(self: Self, chars: Optional[str] = None, Self: type)
```

Added by [`internal.types.str`](https://docs.exaloop.io/libraries/api/modules/internal/types/str/)

split[](https://docs.exaloop.io/libraries/api/classes/str/#class-str-split "Copy/link to this API element")

```python
split(self: Self, sep: Optional[str] = None, maxsplit: int = -1, Self: type)
```

Added by [`internal.types.str`](https://docs.exaloop.io/libraries/api/modules/internal/types/str/)

rsplit[](https://docs.exaloop.io/libraries/api/classes/str/#class-str-rsplit "Copy/link to this API element")

```python
rsplit(self: Self, sep: Optional[str] = None, maxsplit: int = -1, Self: type)
```

Added by [`internal.types.str`](https://docs.exaloop.io/libraries/api/modules/internal/types/str/)

splitlines[](https://docs.exaloop.io/libraries/api/classes/str/#class-str-splitlines "Copy/link to this API element")

```python
splitlines(self: Self, keepends: bool = False, Self: type)
```

Added by [`internal.types.str`](https://docs.exaloop.io/libraries/api/modules/internal/types/str/)

join[](https://docs.exaloop.io/libraries/api/classes/str/#class-str-join "Copy/link to this API element")

```python
join(self: Self, values, Self: type)
```

Added by [`internal.types.str`](https://docs.exaloop.io/libraries/api/modules/internal/types/str/)

```python
join(self: Self, values: List[str], Self: type)
```

Added by [`internal.types.str`](https://docs.exaloop.io/libraries/api/modules/internal/types/str/)

replace[](https://docs.exaloop.io/libraries/api/classes/str/#class-str-replace "Copy/link to this API element")

```python
replace(self: Self, old: str, new: str, count: int = -1, Self: type)
```

Added by [`internal.types.str`](https://docs.exaloop.io/libraries/api/modules/internal/types/str/)

center[](https://docs.exaloop.io/libraries/api/classes/str/#class-str-center "Copy/link to this API element")

```python
center(self: Self, width: int, Self: type)
```

Added by [`internal.types.str`](https://docs.exaloop.io/libraries/api/modules/internal/types/str/)

```python
center(self: Self, width: int, fillchar: str, Self: type)
```

Added by [`internal.types.str`](https://docs.exaloop.io/libraries/api/modules/internal/types/str/)

ljust[](https://docs.exaloop.io/libraries/api/classes/str/#class-str-ljust "Copy/link to this API element")

```python
ljust(self: Self, width: int, Self: type)
```

Added by [`internal.types.str`](https://docs.exaloop.io/libraries/api/modules/internal/types/str/)

```python
ljust(self: Self, width: int, fillchar: str, Self: type)
```

Added by [`internal.types.str`](https://docs.exaloop.io/libraries/api/modules/internal/types/str/)

rjust[](https://docs.exaloop.io/libraries/api/classes/str/#class-str-rjust "Copy/link to this API element")

```python
rjust(self: Self, width: int, Self: type)
```

Added by [`internal.types.str`](https://docs.exaloop.io/libraries/api/modules/internal/types/str/)

```python
rjust(self: Self, width: int, fillchar: str, Self: type)
```

Added by [`internal.types.str`](https://docs.exaloop.io/libraries/api/modules/internal/types/str/)

zfill[](https://docs.exaloop.io/libraries/api/classes/str/#class-str-zfill "Copy/link to this API element")

```python
zfill(self: Self, width: int, Self: type)
```

Added by [`internal.types.str`](https://docs.exaloop.io/libraries/api/modules/internal/types/str/)

expandtabs[](https://docs.exaloop.io/libraries/api/classes/str/#class-str-expandtabs "Copy/link to this API element")

```python
expandtabs(self: Self, tabsize: int = 8, Self: type)
```

Added by [`internal.types.str`](https://docs.exaloop.io/libraries/api/modules/internal/types/str/)

isalnum[](https://docs.exaloop.io/libraries/api/classes/str/#class-str-isalnum "Copy/link to this API element")

```python
isalnum(self: Self, Self: type)
```

Added by [`internal.types.str`](https://docs.exaloop.io/libraries/api/modules/internal/types/str/)

isalpha[](https://docs.exaloop.io/libraries/api/classes/str/#class-str-isalpha "Copy/link to this API element")

```python
isalpha(self: Self, Self: type)
```

Added by [`internal.types.str`](https://docs.exaloop.io/libraries/api/modules/internal/types/str/)

isdigit[](https://docs.exaloop.io/libraries/api/classes/str/#class-str-isdigit "Copy/link to this API element")

```python
isdigit(self: Self, Self: type)
```

Added by [`internal.types.str`](https://docs.exaloop.io/libraries/api/modules/internal/types/str/)

isspace[](https://docs.exaloop.io/libraries/api/classes/str/#class-str-isspace "Copy/link to this API element")

```python
isspace(self: Self, Self: type)
```

Added by [`internal.types.str`](https://docs.exaloop.io/libraries/api/modules/internal/types/str/)

isascii[](https://docs.exaloop.io/libraries/api/classes/str/#class-str-isascii "Copy/link to this API element")

```python
isascii(self: Self, Self: type)
```

Added by [`internal.types.str`](https://docs.exaloop.io/libraries/api/modules/internal/types/str/)

islower[](https://docs.exaloop.io/libraries/api/classes/str/#class-str-islower "Copy/link to this API element")

```python
islower(self: Self, Self: type)
```

Added by [`internal.types.str`](https://docs.exaloop.io/libraries/api/modules/internal/types/str/)

isupper[](https://docs.exaloop.io/libraries/api/classes/str/#class-str-isupper "Copy/link to this API element")

```python
isupper(self: Self, Self: type)
```

Added by [`internal.types.str`](https://docs.exaloop.io/libraries/api/modules/internal/types/str/)

istitle[](https://docs.exaloop.io/libraries/api/classes/str/#class-str-istitle "Copy/link to this API element")

```python
istitle(self: Self, Self: type)
```

Added by [`internal.types.str`](https://docs.exaloop.io/libraries/api/modules/internal/types/str/)

isnumeric[](https://docs.exaloop.io/libraries/api/classes/str/#class-str-isnumeric "Copy/link to this API element")

```python
isnumeric(self: Self, Self: type)
```

Added by [`internal.types.str`](https://docs.exaloop.io/libraries/api/modules/internal/types/str/)

isdecimal[](https://docs.exaloop.io/libraries/api/classes/str/#class-str-isdecimal "Copy/link to this API element")

```python
isdecimal(self: Self, Self: type)
```

Added by [`internal.types.str`](https://docs.exaloop.io/libraries/api/modules/internal/types/str/)

isprintable[](https://docs.exaloop.io/libraries/api/classes/str/#class-str-isprintable "Copy/link to this API element")

```python
isprintable(self: Self, Self: type)
```

Added by [`internal.types.str`](https://docs.exaloop.io/libraries/api/modules/internal/types/str/)

isidentifier[](https://docs.exaloop.io/libraries/api/classes/str/#class-str-isidentifier "Copy/link to this API element")

```python
isidentifier(self: Self, Self: type)
```

Added by [`internal.types.str`](https://docs.exaloop.io/libraries/api/modules/internal/types/str/)

partition[](https://docs.exaloop.io/libraries/api/classes/str/#class-str-partition "Copy/link to this API element")

```python
partition(self: Self, sep: str, Self: type)
```

Added by [`internal.types.str`](https://docs.exaloop.io/libraries/api/modules/internal/types/str/)

rpartition[](https://docs.exaloop.io/libraries/api/classes/str/#class-str-rpartition "Copy/link to this API element")

```python
rpartition(self: Self, sep: str, Self: type)
```

Added by [`internal.types.str`](https://docs.exaloop.io/libraries/api/modules/internal/types/str/)

cat[](https://docs.exaloop.io/libraries/api/classes/str/#class-str-cat "Copy/link to this API element")

```python
cat(*args)
```

Added by [`internal.types.str`](https://docs.exaloop.io/libraries/api/modules/internal/types/str/)

memcpy[](https://docs.exaloop.io/libraries/api/classes/str/#class-str-memcpy "Copy/link to this API element")

@llvm

```python
memcpy(dest: Ptr[u8], src: Ptr[u8], len: int)
```

Added by [`internal.types.str`](https://docs.exaloop.io/libraries/api/modules/internal/types/str/)

```llvm
declare void @llvm.memcpy.p0i8.p0i8.i64(ptr %dest, ptr %src, i64 %len, i32 %align, i1 %isvolatile)
call void @llvm.memcpy.p0i8.p0i8.i64(ptr %dest, ptr %src, i64 %len, i32 0, i1 false)
ret {} {}
```

memmove[](https://docs.exaloop.io/libraries/api/classes/str/#class-str-memmove "Copy/link to this API element")

@llvm

```python
memmove(dest: Ptr[u8], src: Ptr[u8], len: int)
```

Added by [`internal.types.str`](https://docs.exaloop.io/libraries/api/modules/internal/types/str/)

```llvm
declare void @llvm.memmove.p0i8.p0i8.i64(ptr %dest, ptr %src, i64 %len, i32 %align, i1 %isvolatile)
call void @llvm.memmove.p0i8.p0i8.i64(ptr %dest, ptr %src, i64 %len, i32 0, i1 false)
ret {} {}
```

memset[](https://docs.exaloop.io/libraries/api/classes/str/#class-str-memset "Copy/link to this API element")

@llvm

```python
memset(dest: Ptr[u8], val: u8, len: int)
```

Added by [`internal.types.str`](https://docs.exaloop.io/libraries/api/modules/internal/types/str/)

```llvm
declare void @llvm.memset.p0i8.i64(ptr %dest, i8 %val, i64 %len, i32 %align, i1 %isvolatile)
call void @llvm.memset.p0i8.i64(ptr %dest, i8 %val, i64 %len, i32 0, i1 false)
ret {} {}
```

Special methods 30

\_\_to\_py\_\_[](https://docs.exaloop.io/libraries/api/classes/str/#class-str-__to_py__ "Copy/link to this API element")

```python
__to_py__(self: Self, Self: type)
```

Added by [`internal.python`](https://docs.exaloop.io/libraries/api/modules/internal/python/)

\_\_from\_py\_\_[](https://docs.exaloop.io/libraries/api/classes/str/#class-str-__from_py__ "Copy/link to this API element")

```python
__from_py__(s: cobj)
```

Added by [`internal.python`](https://docs.exaloop.io/libraries/api/modules/internal/python/)

\_\_format\_\_[](https://docs.exaloop.io/libraries/api/classes/str/#class-str-__format__ "Copy/link to this API element")

```python
__format__(self: Self, format_spec: str, Self: type)
```

Added by [`internal.format`](https://docs.exaloop.io/libraries/api/modules/internal/format/)

\_\_mod\_\_[](https://docs.exaloop.io/libraries/api/classes/str/#class-str-__mod__ "Copy/link to this API element")

```python
__mod__(self: Self, values, Self: type)
```

Added by [`internal.format`](https://docs.exaloop.io/libraries/api/modules/internal/format/)

\_\_pickle\_\_[](https://docs.exaloop.io/libraries/api/classes/str/#class-str-__pickle__ "Copy/link to this API element")

```python
__pickle__(self: Self, jar: Jar, Self: type)
```

Added by [`pickle`](https://docs.exaloop.io/libraries/api/modules/pickle/)

\_\_unpickle\_\_[](https://docs.exaloop.io/libraries/api/classes/str/#class-str-__unpickle__ "Copy/link to this API element")

```python
__unpickle__(jar: Jar)
```

Added by [`pickle`](https://docs.exaloop.io/libraries/api/modules/pickle/)

\_\_len\_\_[](https://docs.exaloop.io/libraries/api/classes/str/#class-str-__len__ "Copy/link to this API element")

```python
__len__(self: Self, Self: type)
```

Added by [`internal.types.str`](https://docs.exaloop.io/libraries/api/modules/internal/types/str/)

\_\_bool\_\_[](https://docs.exaloop.io/libraries/api/classes/str/#class-str-__bool__ "Copy/link to this API element")

```python
__bool__(self: Self, Self: type)
```

Added by [`internal.types.str`](https://docs.exaloop.io/libraries/api/modules/internal/types/str/)

\_\_match\_\_[](https://docs.exaloop.io/libraries/api/classes/str/#class-str-__match__ "Copy/link to this API element")

```python
__match__(self: Self, other: str, Self: type)
```

Added by [`internal.types.str`](https://docs.exaloop.io/libraries/api/modules/internal/types/str/)

\_\_getitem\_\_[](https://docs.exaloop.io/libraries/api/classes/str/#class-str-__getitem__ "Copy/link to this API element")

```python
__getitem__(self: Self, item: int, Self: type)
```

Added by [`internal.types.str`](https://docs.exaloop.io/libraries/api/modules/internal/types/str/)

```python
__getitem__(self: Self, item: Slice[int, int, int], Self: type)
```

Added by [`internal.types.str`](https://docs.exaloop.io/libraries/api/modules/internal/types/str/)

\_\_eq\_\_[](https://docs.exaloop.io/libraries/api/classes/str/#class-str-__eq__ "Copy/link to this API element")

```python
__eq__(self: Self, other: str, Self: type)
```

Added by [`internal.types.str`](https://docs.exaloop.io/libraries/api/modules/internal/types/str/)

\_\_ne\_\_[](https://docs.exaloop.io/libraries/api/classes/str/#class-str-__ne__ "Copy/link to this API element")

```python
__ne__(self: Self, other: str, Self: type)
```

Added by [`internal.types.str`](https://docs.exaloop.io/libraries/api/modules/internal/types/str/)

\_\_lt\_\_[](https://docs.exaloop.io/libraries/api/classes/str/#class-str-__lt__ "Copy/link to this API element")

```python
__lt__(self: Self, other: str, Self: type)
```

Added by [`internal.types.str`](https://docs.exaloop.io/libraries/api/modules/internal/types/str/)

\_\_le\_\_[](https://docs.exaloop.io/libraries/api/classes/str/#class-str-__le__ "Copy/link to this API element")

```python
__le__(self: Self, other: str, Self: type)
```

Added by [`internal.types.str`](https://docs.exaloop.io/libraries/api/modules/internal/types/str/)

\_\_gt\_\_[](https://docs.exaloop.io/libraries/api/classes/str/#class-str-__gt__ "Copy/link to this API element")

```python
__gt__(self: Self, other: str, Self: type)
```

Added by [`internal.types.str`](https://docs.exaloop.io/libraries/api/modules/internal/types/str/)

\_\_ge\_\_[](https://docs.exaloop.io/libraries/api/classes/str/#class-str-__ge__ "Copy/link to this API element")

```python
__ge__(self: Self, other: str, Self: type)
```

Added by [`internal.types.str`](https://docs.exaloop.io/libraries/api/modules/internal/types/str/)

\_\_add\_\_[](https://docs.exaloop.io/libraries/api/classes/str/#class-str-__add__ "Copy/link to this API element")

```python
__add__(self: Self, other: str, Self: type)
```

Added by [`internal.types.str`](https://docs.exaloop.io/libraries/api/modules/internal/types/str/)

\_\_mul\_\_[](https://docs.exaloop.io/libraries/api/classes/str/#class-str-__mul__ "Copy/link to this API element")

```python
__mul__(self: Self, count: int, Self: type)
```

Added by [`internal.types.str`](https://docs.exaloop.io/libraries/api/modules/internal/types/str/)

\_\_rmul\_\_[](https://docs.exaloop.io/libraries/api/classes/str/#class-str-__rmul__ "Copy/link to this API element")

```python
__rmul__(self: Self, count: int, Self: type)
```

Added by [`internal.types.str`](https://docs.exaloop.io/libraries/api/modules/internal/types/str/)

\_\_repr\_\_[](https://docs.exaloop.io/libraries/api/classes/str/#class-str-__repr__ "Copy/link to this API element")

```python
__repr__(self: Self, Self: type)
```

Added by [`internal.types.str`](https://docs.exaloop.io/libraries/api/modules/internal/types/str/)

\_\_str\_\_[](https://docs.exaloop.io/libraries/api/classes/str/#class-str-__str__ "Copy/link to this API element")

```python
__str__(self: Self, Self: type)
```

Added by [`internal.types.str`](https://docs.exaloop.io/libraries/api/modules/internal/types/str/)

\_\_contains\_\_[](https://docs.exaloop.io/libraries/api/classes/str/#class-str-__contains__ "Copy/link to this API element")

```python
__contains__(self: Self, sub: str, Self: type)
```

Added by [`internal.types.str`](https://docs.exaloop.io/libraries/api/modules/internal/types/str/)

\_\_copy\_\_[](https://docs.exaloop.io/libraries/api/classes/str/#class-str-__copy__ "Copy/link to this API element")

```python
__copy__(self: Self, Self: type)
```

Added by [`internal.types.str`](https://docs.exaloop.io/libraries/api/modules/internal/types/str/)

\_\_deepcopy\_\_[](https://docs.exaloop.io/libraries/api/classes/str/#class-str-__deepcopy__ "Copy/link to this API element")

```python
__deepcopy__(self: Self, Self: type)
```

Added by [`internal.types.str`](https://docs.exaloop.io/libraries/api/modules/internal/types/str/)

\_\_ptrcopy\_\_[](https://docs.exaloop.io/libraries/api/classes/str/#class-str-__ptrcopy__ "Copy/link to this API element")

```python
__ptrcopy__(self: Self, Self: type)
```

Added by [`internal.types.str`](https://docs.exaloop.io/libraries/api/modules/internal/types/str/)

\_\_hash\_\_[](https://docs.exaloop.io/libraries/api/classes/str/#class-str-__hash__ "Copy/link to this API element")

```python
__hash__(self: Self, Self: type)
```

Added by [`internal.types.str`](https://docs.exaloop.io/libraries/api/modules/internal/types/str/)

\_\_iter\_\_[](https://docs.exaloop.io/libraries/api/classes/str/#class-str-__iter__ "Copy/link to this API element")

```python
__iter__(self: Self, Self: type)
```

Added by [`internal.types.str`](https://docs.exaloop.io/libraries/api/modules/internal/types/str/)

\_\_to\_gpu\_\_[](https://docs.exaloop.io/libraries/api/classes/str/#class-str-__to_gpu__ "Copy/link to this API element")

```python
__to_gpu__(self: Self, cache: AllocCache, Self: type)
```

Added by [`internal.gpu`](https://docs.exaloop.io/libraries/api/modules/internal/gpu/)

\_\_from\_gpu\_\_[](https://docs.exaloop.io/libraries/api/classes/str/#class-str-__from_gpu__ "Copy/link to this API element")

```python
__from_gpu__(self: Self, other: str, Self: type)
```

Added by [`internal.gpu`](https://docs.exaloop.io/libraries/api/modules/internal/gpu/)

\_\_from\_gpu\_new\_\_[](https://docs.exaloop.io/libraries/api/classes/str/#class-str-__from_gpu_new__ "Copy/link to this API element")

```python
__from_gpu_new__(other: str)
```

Added by [`internal.gpu`](https://docs.exaloop.io/libraries/api/modules/internal/gpu/)

Source: https://docs.exaloop.io/libraries/api/classes/str/
