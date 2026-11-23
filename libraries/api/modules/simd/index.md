# simd

Standard library Module [View source](https://github.com/exaloop/codon/blob/master/stdlib/simd.codon)

class Vec[](https://docs.exaloop.io/libraries/api/modules/simd/#class-vec "Copy/link to this API element")

@tuple

```python
class Vec[T, N]
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/simd/Vec/)

Fields 2

`T` `type`

`N` `Literal[int]`

Constructors 4

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/simd/#class-vec-__new__ "Copy/link to this API element")

```python
__new__[check: Literal[bool] = True](x) -> Vec[(T, N)]
```

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/simd/#class-vec-__new__-overload-2 "Copy/link to this API element")

```python
__new__[check: Literal[bool] = True](x: str, offset: int = 0) -> Vec[(T, N)]
```

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/simd/#class-vec-__new__-overload-3 "Copy/link to this API element")

```python
__new__[check: Literal[bool] = True](x: bytes, offset: int = 0) -> Vec[(T, N)]
```

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/simd/#class-vec-__new__-overload-4 "Copy/link to this API element")

```python
__new__[check: Literal[bool] = True](x: List[T], offset: int = 0) -> Vec[(T, N)]
```

Methods 46

load[](https://docs.exaloop.io/libraries/api/modules/simd/#class-vec-load "Copy/link to this API element")

```python
load[alignment: Literal[int] = 1](data: Ptr[T], offset: int = 0, mask = NoneType(), passthrough = NoneType()) -> Vec[(T, N)]
```

store[](https://docs.exaloop.io/libraries/api/modules/simd/#class-vec-store "Copy/link to this API element")

```python
store[alignment: Literal[int] = 1](self: Self, data: Ptr[T], offset: int = 0, mask = NoneType())
```

gather[](https://docs.exaloop.io/libraries/api/modules/simd/#class-vec-gather "Copy/link to this API element")

```python
gather[alignment: Literal[int] = 1](data: Ptr[T], indices: Vec[(int, N)], mask = NoneType(), passthrough = NoneType()) -> Vec[(T, N)]
```

scatter[](https://docs.exaloop.io/libraries/api/modules/simd/#class-vec-scatter "Copy/link to this API element")

```python
scatter[alignment: Literal[int] = 1](self: Self, data: Ptr[T], indices: Vec[(int, N)], mask = NoneType())
```

```python
scatter(self: Vec[(T, N)]) -> List[T]
```

cast[](https://docs.exaloop.io/libraries/api/modules/simd/#class-vec-cast "Copy/link to this API element")

```python
cast[T2: type](self: Self) -> Vec[(T2, N)]
```

cast\_ptr[](https://docs.exaloop.io/libraries/api/modules/simd/#class-vec-cast_ptr "Copy/link to this API element")

@pure @llvm

```python
cast_ptr[T: type, T2: type](ptr: Ptr[T]) -> Ptr[T2]
```

```llvm
ret ptr %ptr
```

extract[](https://docs.exaloop.io/libraries/api/modules/simd/#class-vec-extract "Copy/link to this API element")

```python
extract[check: Literal[bool] = True](self: Self, index: int) -> T
```

insert[](https://docs.exaloop.io/libraries/api/modules/simd/#class-vec-insert "Copy/link to this API element")

```python
insert[check: Literal[bool] = True](self: Self, index: int, value: T) -> Vec[(T, N)]
```

shuffle[](https://docs.exaloop.io/libraries/api/modules/simd/#class-vec-shuffle "Copy/link to this API element")

```python
shuffle[check: Literal[bool] = True](self: Self, indices, other = NoneType())
```

interleave[](https://docs.exaloop.io/libraries/api/modules/simd/#class-vec-interleave "Copy/link to this API element")

```python
interleave(self: Self, other: Vec[(T, N)]) -> Vec[(T, (2 * N))]
```

lshift[](https://docs.exaloop.io/libraries/api/modules/simd/#class-vec-lshift "Copy/link to this API element")

```python
lshift[check: Literal[bool] = True](self: Self, other: Vec[(T, N)]) -> Vec[(T, N)]
```

```python
lshift[check: Literal[bool] = True](self: Self, other: T) -> Vec[(T, N)]
```

```python
lshift[check: Literal[bool] = True](self: Self, other: int) -> Vec[(T, N)]
```

lsh[](https://docs.exaloop.io/libraries/api/modules/simd/#class-vec-lsh "Copy/link to this API element")

```python
lsh[other: Literal[int]](self: Self) -> Vec[(T, N)]
```

rshift[](https://docs.exaloop.io/libraries/api/modules/simd/#class-vec-rshift "Copy/link to this API element")

```python
rshift[check: Literal[bool] = True](self: Self, other: Vec[(T, N)]) -> Vec[(T, N)]
```

```python
rshift[check: Literal[bool] = True](self: Self, other: T) -> Vec[(T, N)]
```

```python
rshift[check: Literal[bool] = True](self: Self, other: int) -> Vec[(T, N)]
```

rsh[](https://docs.exaloop.io/libraries/api/modules/simd/#class-vec-rsh "Copy/link to this API element")

```python
rsh[other: Literal[int]](self: Self) -> Vec[(T, N)]
```

any[](https://docs.exaloop.io/libraries/api/modules/simd/#class-vec-any "Copy/link to this API element")

```python
any(self: Self) -> bool
```

all[](https://docs.exaloop.io/libraries/api/modules/simd/#class-vec-all "Copy/link to this API element")

```python
all(self: Self) -> bool
```

count[](https://docs.exaloop.io/libraries/api/modules/simd/#class-vec-count "Copy/link to this API element")

```python
count(self: Self) -> int
```

to\_bits[](https://docs.exaloop.io/libraries/api/modules/simd/#class-vec-to_bits "Copy/link to this API element")

```python
to_bits(self: Self) -> UInt[N]
```

from\_bits[](https://docs.exaloop.io/libraries/api/modules/simd/#class-vec-from_bits "Copy/link to this API element")

```python
from_bits(bits: UInt[N]) -> Vec[(T, N)]
```

floordiv[](https://docs.exaloop.io/libraries/api/modules/simd/#class-vec-floordiv "Copy/link to this API element")

```python
floordiv[check: Literal[bool] = True](self: Self, other: Vec[(T, N)]) -> Vec[(T, N)]
```

```python
floordiv[check: Literal[bool] = True](self: Self, other: T) -> Vec[(T, N)]
```

mod[](https://docs.exaloop.io/libraries/api/modules/simd/#class-vec-mod "Copy/link to this API element")

```python
mod[check: Literal[bool] = True](self: Self, other: Vec[(T, N)]) -> Vec[(T, N)]
```

```python
mod[check: Literal[bool] = True](self: Self, other: T) -> Vec[(T, N)]
```

sqrt[](https://docs.exaloop.io/libraries/api/modules/simd/#class-vec-sqrt "Copy/link to this API element")

```python
sqrt(self: Self)
```

log[](https://docs.exaloop.io/libraries/api/modules/simd/#class-vec-log "Copy/link to this API element")

```python
log(self: Self)
```

sin[](https://docs.exaloop.io/libraries/api/modules/simd/#class-vec-sin "Copy/link to this API element")

```python
sin(self: Self)
```

cos[](https://docs.exaloop.io/libraries/api/modules/simd/#class-vec-cos "Copy/link to this API element")

```python
cos(self: Self)
```

min[](https://docs.exaloop.io/libraries/api/modules/simd/#class-vec-min "Copy/link to this API element")

```python
min(self: Self, other: Vec[(T, N)]) -> Vec[(T, N)]
```

```python
min(self: Self, other: T) -> Vec[(T, N)]
```

```python
min(self: Self) -> T
```

max[](https://docs.exaloop.io/libraries/api/modules/simd/#class-vec-max "Copy/link to this API element")

```python
max(self: Self, other: Vec[(T, N)]) -> Vec[(T, N)]
```

```python
max(self: Self, other: T) -> Vec[(T, N)]
```

```python
max(self: Self) -> T
```

mask[](https://docs.exaloop.io/libraries/api/modules/simd/#class-vec-mask "Copy/link to this API element")

```python
mask(self: Self, other: Vec[(T, N)], mask) -> Vec[(T, N)]
```

add[](https://docs.exaloop.io/libraries/api/modules/simd/#class-vec-add "Copy/link to this API element")

```python
add[overflow: Literal[bool] = False](self: Self, other: Vec[(T, N)])
```

sub[](https://docs.exaloop.io/libraries/api/modules/simd/#class-vec-sub "Copy/link to this API element")

```python
sub[overflow: Literal[bool] = False](self: Self, other: Vec[(T, N)])
```

sum[](https://docs.exaloop.io/libraries/api/modules/simd/#class-vec-sum "Copy/link to this API element")

```python
sum(self: Self) -> T
```

prod[](https://docs.exaloop.io/libraries/api/modules/simd/#class-vec-prod "Copy/link to this API element")

```python
prod(self: Self) -> T
```

fma[](https://docs.exaloop.io/libraries/api/modules/simd/#class-vec-fma "Copy/link to this API element")

```python
fma(self: Self, other, addend) -> Vec[(T, N)]
```

exp[](https://docs.exaloop.io/libraries/api/modules/simd/#class-vec-exp "Copy/link to this API element")

```python
exp(self: Self) -> Vec[(T, N)]
```

exp2[](https://docs.exaloop.io/libraries/api/modules/simd/#class-vec-exp2 "Copy/link to this API element")

```python
exp2(self: Self) -> Vec[(T, N)]
```

log2[](https://docs.exaloop.io/libraries/api/modules/simd/#class-vec-log2 "Copy/link to this API element")

```python
log2(self: Self) -> Vec[(T, N)]
```

log10[](https://docs.exaloop.io/libraries/api/modules/simd/#class-vec-log10 "Copy/link to this API element")

```python
log10(self: Self) -> Vec[(T, N)]
```

floor[](https://docs.exaloop.io/libraries/api/modules/simd/#class-vec-floor "Copy/link to this API element")

```python
floor(self: Self) -> Vec[(T, N)]
```

ceil[](https://docs.exaloop.io/libraries/api/modules/simd/#class-vec-ceil "Copy/link to this API element")

```python
ceil(self: Self) -> Vec[(T, N)]
```

trunc[](https://docs.exaloop.io/libraries/api/modules/simd/#class-vec-trunc "Copy/link to this API element")

```python
trunc(self: Self) -> Vec[(T, N)]
```

round[](https://docs.exaloop.io/libraries/api/modules/simd/#class-vec-round "Copy/link to this API element")

```python
round(self: Self) -> Vec[(T, N)]
```

isnan[](https://docs.exaloop.io/libraries/api/modules/simd/#class-vec-isnan "Copy/link to this API element")

```python
isnan(self: Self) -> Vec[(u1, N)]
```

isinf[](https://docs.exaloop.io/libraries/api/modules/simd/#class-vec-isinf "Copy/link to this API element")

```python
isinf(self: Self) -> Vec[(u1, N)]
```

isfinite[](https://docs.exaloop.io/libraries/api/modules/simd/#class-vec-isfinite "Copy/link to this API element")

```python
isfinite(self: Self) -> Vec[(u1, N)]
```

signbit[](https://docs.exaloop.io/libraries/api/modules/simd/#class-vec-signbit "Copy/link to this API element")

```python
signbit(self: Self) -> Vec[(u1, N)]
```

bitflip[](https://docs.exaloop.io/libraries/api/modules/simd/#class-vec-bitflip "Copy/link to this API element")

```python
bitflip(self: Self) -> Vec[(T, N)]
```

Special methods 35

\_\_copy\_\_[](https://docs.exaloop.io/libraries/api/modules/simd/#class-vec-__copy__ "Copy/link to this API element")

```python
__copy__(self: Self) -> Vec[(T, N)]
```

\_\_len\_\_[](https://docs.exaloop.io/libraries/api/modules/simd/#class-vec-__len__ "Copy/link to this API element")

```python
__len__(self: Self) -> int
```

\_\_neg\_\_[](https://docs.exaloop.io/libraries/api/modules/simd/#class-vec-__neg__ "Copy/link to this API element")

```python
__neg__(self: Self) -> Vec[(T, N)]
```

\_\_getitem\_\_[](https://docs.exaloop.io/libraries/api/modules/simd/#class-vec-__getitem__ "Copy/link to this API element")

```python
__getitem__(self: Self, index: int) -> T
```

```python
__getitem__[n: Literal[int]](self: Self) -> T
```

\_\_repr\_\_[](https://docs.exaloop.io/libraries/api/modules/simd/#class-vec-__repr__ "Copy/link to this API element")

```python
__repr__(self: Self)
```

\_\_eq\_\_[](https://docs.exaloop.io/libraries/api/modules/simd/#class-vec-__eq__ "Copy/link to this API element")

```python
__eq__(self: Self, other: Vec[(T, N)]) -> Vec[(u1, N)]
```

```python
__eq__(self: Self, other: T) -> Vec[(u1, N)]
```

\_\_ne\_\_[](https://docs.exaloop.io/libraries/api/modules/simd/#class-vec-__ne__ "Copy/link to this API element")

```python
__ne__(self: Vec[(T, N)], other: Vec[(T, N)]) -> Vec[(u1, N)]
```

```python
__ne__(self: Self, other: T) -> Vec[(u1, N)]
```

\_\_and\_\_[](https://docs.exaloop.io/libraries/api/modules/simd/#class-vec-__and__ "Copy/link to this API element")

```python
__and__(self: Self, other: Vec[(T, N)]) -> Vec[(T, N)]
```

```python
__and__(self: Self, other: T) -> Vec[(T, N)]
```

\_\_or\_\_[](https://docs.exaloop.io/libraries/api/modules/simd/#class-vec-__or__ "Copy/link to this API element")

```python
__or__(self: Self, other: Vec[(T, N)]) -> Vec[(T, N)]
```

```python
__or__(self: Self, other: T) -> Vec[(T, N)]
```

\_\_xor\_\_[](https://docs.exaloop.io/libraries/api/modules/simd/#class-vec-__xor__ "Copy/link to this API element")

```python
__xor__(self: Self, other: Vec[(T, N)]) -> Vec[(T, N)]
```

```python
__xor__(self: Self, other: T) -> Vec[(T, N)]
```

\_\_rand\_\_[](https://docs.exaloop.io/libraries/api/modules/simd/#class-vec-__rand__ "Copy/link to this API element")

```python
__rand__(self: Self, other: T) -> Vec[(T, N)]
```

\_\_ror\_\_[](https://docs.exaloop.io/libraries/api/modules/simd/#class-vec-__ror__ "Copy/link to this API element")

```python
__ror__(self: Self, other: T) -> Vec[(T, N)]
```

\_\_rxor\_\_[](https://docs.exaloop.io/libraries/api/modules/simd/#class-vec-__rxor__ "Copy/link to this API element")

```python
__rxor__(self: Self, other: T) -> Vec[(T, N)]
```

\_\_invert\_\_[](https://docs.exaloop.io/libraries/api/modules/simd/#class-vec-__invert__ "Copy/link to this API element")

```python
__invert__(self: Self) -> Vec[(T, N)]
```

\_\_lshift\_\_[](https://docs.exaloop.io/libraries/api/modules/simd/#class-vec-__lshift__ "Copy/link to this API element")

```python
__lshift__(self: Self, other: Vec[(T, N)]) -> Vec[(T, N)]
```

```python
__lshift__(self: Self, other: T) -> Vec[(T, N)]
```

```python
__lshift__(self: Self, other: int) -> Vec[(T, N)]
```

\_\_rlshift\_\_[](https://docs.exaloop.io/libraries/api/modules/simd/#class-vec-__rlshift__ "Copy/link to this API element")

```python
__rlshift__(self: Self, other: T) -> Vec[(T, N)]
```

\_\_rshift\_\_[](https://docs.exaloop.io/libraries/api/modules/simd/#class-vec-__rshift__ "Copy/link to this API element")

```python
__rshift__(self: Self, other: Vec[(T, N)]) -> Vec[(T, N)]
```

```python
__rshift__(self: Self, other: T) -> Vec[(T, N)]
```

```python
__rshift__(self: Self, other: int) -> Vec[(T, N)]
```

\_\_rrshift\_\_[](https://docs.exaloop.io/libraries/api/modules/simd/#class-vec-__rrshift__ "Copy/link to this API element")

```python
__rrshift__(self: Self, other: T) -> Vec[(T, N)]
```

\_\_add\_\_[](https://docs.exaloop.io/libraries/api/modules/simd/#class-vec-__add__ "Copy/link to this API element")

```python
__add__(self: Self, other: Vec[(T, N)]) -> Vec[(T, N)]
```

```python
__add__(self: Self, other: T) -> Vec[(T, N)]
```

\_\_radd\_\_[](https://docs.exaloop.io/libraries/api/modules/simd/#class-vec-__radd__ "Copy/link to this API element")

```python
__radd__(self: Self, other: T) -> Vec[(T, N)]
```

\_\_sub\_\_[](https://docs.exaloop.io/libraries/api/modules/simd/#class-vec-__sub__ "Copy/link to this API element")

```python
__sub__(self: Self, other: Vec[(T, N)]) -> Vec[(T, N)]
```

```python
__sub__(self: Self, other: T) -> Vec[(T, N)]
```

\_\_rsub\_\_[](https://docs.exaloop.io/libraries/api/modules/simd/#class-vec-__rsub__ "Copy/link to this API element")

```python
__rsub__(self: Self, other: T) -> Vec[(T, N)]
```

\_\_mul\_\_[](https://docs.exaloop.io/libraries/api/modules/simd/#class-vec-__mul__ "Copy/link to this API element")

```python
__mul__(self: Self, other: Vec[(T, N)]) -> Vec[(T, N)]
```

```python
__mul__(self: Self, other: T) -> Vec[(T, N)]
```

\_\_rmul\_\_[](https://docs.exaloop.io/libraries/api/modules/simd/#class-vec-__rmul__ "Copy/link to this API element")

```python
__rmul__(self: Self, other: T) -> Vec[(T, N)]
```

\_\_truediv\_\_[](https://docs.exaloop.io/libraries/api/modules/simd/#class-vec-__truediv__ "Copy/link to this API element")

```python
__truediv__(self: Self, other: Vec[(T, N)]) -> Vec[(T, N)]
```

```python
__truediv__(self: Self, other: T) -> Vec[(T, N)]
```

\_\_rtruediv\_\_[](https://docs.exaloop.io/libraries/api/modules/simd/#class-vec-__rtruediv__ "Copy/link to this API element")

```python
__rtruediv__(self: Self, other: T) -> Vec[(T, N)]
```

\_\_floordiv\_\_[](https://docs.exaloop.io/libraries/api/modules/simd/#class-vec-__floordiv__ "Copy/link to this API element")

```python
__floordiv__(self: Self, other: Vec[(T, N)]) -> Vec[(T, N)]
```

```python
__floordiv__(self: Self, other: T) -> Vec[(T, N)]
```

\_\_rfloordiv\_\_[](https://docs.exaloop.io/libraries/api/modules/simd/#class-vec-__rfloordiv__ "Copy/link to this API element")

```python
__rfloordiv__(self: Self, other: T) -> Vec[(T, N)]
```

\_\_mod\_\_[](https://docs.exaloop.io/libraries/api/modules/simd/#class-vec-__mod__ "Copy/link to this API element")

```python
__mod__(self: Self, other: Vec[(T, N)]) -> Vec[(T, N)]
```

```python
__mod__(self: Self, other: T) -> Vec[(T, N)]
```

\_\_rmod\_\_[](https://docs.exaloop.io/libraries/api/modules/simd/#class-vec-__rmod__ "Copy/link to this API element")

```python
__rmod__(self: Self, other: T) -> Vec[(T, N)]
```

\_\_lt\_\_[](https://docs.exaloop.io/libraries/api/modules/simd/#class-vec-__lt__ "Copy/link to this API element")

```python
__lt__(self: Self, other: Vec[(T, N)]) -> Vec[(u1, N)]
```

```python
__lt__(self: Self, other: T) -> Vec[(u1, N)]
```

\_\_le\_\_[](https://docs.exaloop.io/libraries/api/modules/simd/#class-vec-__le__ "Copy/link to this API element")

```python
__le__(self: Self, other: Vec[(T, N)]) -> Vec[(u1, N)]
```

```python
__le__(self: Self, other: T) -> Vec[(u1, N)]
```

\_\_gt\_\_[](https://docs.exaloop.io/libraries/api/modules/simd/#class-vec-__gt__ "Copy/link to this API element")

```python
__gt__(self: Self, other: Vec[(T, N)]) -> Vec[(u1, N)]
```

```python
__gt__(self: Self, other: T) -> Vec[(u1, N)]
```

\_\_ge\_\_[](https://docs.exaloop.io/libraries/api/modules/simd/#class-vec-__ge__ "Copy/link to this API element")

```python
__ge__(self: Self, other: Vec[(T, N)]) -> Vec[(u1, N)]
```

```python
__ge__(self: Self, other: T) -> Vec[(u1, N)]
```

\_\_abs\_\_[](https://docs.exaloop.io/libraries/api/modules/simd/#class-vec-__abs__ "Copy/link to this API element")

```python
__abs__(self: Self)
```

class Ptr[](https://docs.exaloop.io/libraries/api/modules/simd/#class-ptr "Copy/link to this API element")

@extend

```python
class Ptr[T]
```

Extends [`internal.core.Ptr`](https://docs.exaloop.io/libraries/api/classes/internal/core/Ptr/) · [View complete class API →](https://docs.exaloop.io/libraries/api/classes/internal/core/Ptr/)

Special methods 2

\_\_getitem\_\_[](https://docs.exaloop.io/libraries/api/modules/simd/#class-ptr-__getitem__ "Copy/link to this API element")

@pure @derives @llvm

```python
__getitem__[S: type, M: Literal[int]](self: Ptr[Vec[(S, M)]], index: int) -> Vec[(S, M)]
```

```llvm
%address = getelementptr <{=M} x {=S}>, ptr %self, i64 %index
%result = load <{=M} x {=S}>, ptr %address, align 1
ret <{=M} x {=S}> %result
```

\_\_setitem\_\_[](https://docs.exaloop.io/libraries/api/modules/simd/#class-ptr-__setitem__ "Copy/link to this API element")

@self\_captures @llvm

```python
__setitem__[S: type, M: Literal[int]](self: Ptr[Vec[(S, M)]], index: int, what: Vec[(S, M)]) -> None
```

```llvm
%address = getelementptr <{=M} x {=S}>, ptr %self, i64 %index
store <{=M} x {=S}> %what, ptr %address, align 1
ret {} zeroinitializer
```

Source: https://docs.exaloop.io/libraries/api/modules/simd/
