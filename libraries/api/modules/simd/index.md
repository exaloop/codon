# simd

Standard library module [View source](https://github.com/exaloop/codon/blob/master/stdlib/simd.codon)

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
__new__(x, check: Literal[bool] = True)
```

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/simd/#class-vec-__new__-overload-2 "Copy/link to this API element")

```python
__new__(x: str, offset: int = 0, check: Literal[bool] = True)
```

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/simd/#class-vec-__new__-overload-3 "Copy/link to this API element")

```python
__new__(x: bytes, offset: int = 0, check: Literal[bool] = True)
```

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/simd/#class-vec-__new__-overload-4 "Copy/link to this API element")

```python
__new__(x: List[T], offset: int = 0, check: Literal[bool] = True)
```

Methods 46

load[](https://docs.exaloop.io/libraries/api/modules/simd/#class-vec-load "Copy/link to this API element")

```python
load(data: Ptr[T], offset: int = 0, alignment: Literal[int] = 1, mask = NoneType(), passthrough = NoneType())
```

store[](https://docs.exaloop.io/libraries/api/modules/simd/#class-vec-store "Copy/link to this API element")

```python
store(self: Self, data: Ptr[T], offset: int = 0, alignment: Literal[int] = 1, mask = NoneType(), Self: type)
```

gather[](https://docs.exaloop.io/libraries/api/modules/simd/#class-vec-gather "Copy/link to this API element")

```python
gather(data: Ptr[T], indices: Vec[int, N], alignment: Literal[int] = 1, mask = NoneType(), passthrough = NoneType())
```

scatter[](https://docs.exaloop.io/libraries/api/modules/simd/#class-vec-scatter "Copy/link to this API element")

```python
scatter(self: Self, data: Ptr[T], indices: Vec[int, N], alignment: Literal[int] = 1, mask = NoneType(), Self: type)
```

```python
scatter(self: Vec[T, N])
```

cast[](https://docs.exaloop.io/libraries/api/modules/simd/#class-vec-cast "Copy/link to this API element")

```python
cast(self: Self, T2: type, Self: type)
```

cast\_ptr[](https://docs.exaloop.io/libraries/api/modules/simd/#class-vec-cast_ptr "Copy/link to this API element")

@pure @llvm

```python
cast_ptr(ptr: Ptr[T], T: type, T2: type)
```

```llvm
ret ptr %ptr
```

extract[](https://docs.exaloop.io/libraries/api/modules/simd/#class-vec-extract "Copy/link to this API element")

```python
extract(self: Self, index: int, check: Literal[bool] = True, Self: type)
```

insert[](https://docs.exaloop.io/libraries/api/modules/simd/#class-vec-insert "Copy/link to this API element")

```python
insert(self: Self, index: int, value: T, check: Literal[bool] = True, Self: type)
```

shuffle[](https://docs.exaloop.io/libraries/api/modules/simd/#class-vec-shuffle "Copy/link to this API element")

```python
shuffle(self: Self, indices, other = NoneType(), check: Literal[bool] = True, Self: type)
```

interleave[](https://docs.exaloop.io/libraries/api/modules/simd/#class-vec-interleave "Copy/link to this API element")

```python
interleave(self: Self, other: Vec[T, N], Self: type)
```

lshift[](https://docs.exaloop.io/libraries/api/modules/simd/#class-vec-lshift "Copy/link to this API element")

```python
lshift(self: Self, other: Vec[T, N], check: Literal[bool] = True, Self: type)
```

```python
lshift(self: Self, other: T, check: Literal[bool] = True, Self: type)
```

```python
lshift(self: Self, other: int, check: Literal[bool] = True, Self: type)
```

lsh[](https://docs.exaloop.io/libraries/api/modules/simd/#class-vec-lsh "Copy/link to this API element")

```python
lsh(self: Self, other: Literal[int], Self: type)
```

rshift[](https://docs.exaloop.io/libraries/api/modules/simd/#class-vec-rshift "Copy/link to this API element")

```python
rshift(self: Self, other: Vec[T, N], check: Literal[bool] = True, Self: type)
```

```python
rshift(self: Self, other: T, check: Literal[bool] = True, Self: type)
```

```python
rshift(self: Self, other: int, check: Literal[bool] = True, Self: type)
```

rsh[](https://docs.exaloop.io/libraries/api/modules/simd/#class-vec-rsh "Copy/link to this API element")

```python
rsh(self: Self, other: Literal[int], Self: type)
```

any[](https://docs.exaloop.io/libraries/api/modules/simd/#class-vec-any "Copy/link to this API element")

```python
any(self: Self, Self: type)
```

all[](https://docs.exaloop.io/libraries/api/modules/simd/#class-vec-all "Copy/link to this API element")

```python
all(self: Self, Self: type)
```

count[](https://docs.exaloop.io/libraries/api/modules/simd/#class-vec-count "Copy/link to this API element")

```python
count(self: Self, Self: type)
```

to\_bits[](https://docs.exaloop.io/libraries/api/modules/simd/#class-vec-to_bits "Copy/link to this API element")

```python
to_bits(self: Self, Self: type)
```

from\_bits[](https://docs.exaloop.io/libraries/api/modules/simd/#class-vec-from_bits "Copy/link to this API element")

```python
from_bits(bits: UInt[N])
```

floordiv[](https://docs.exaloop.io/libraries/api/modules/simd/#class-vec-floordiv "Copy/link to this API element")

```python
floordiv(self: Self, other: Vec[T, N], check: Literal[bool] = True, Self: type)
```

```python
floordiv(self: Self, other: T, check: Literal[bool] = True, Self: type)
```

mod[](https://docs.exaloop.io/libraries/api/modules/simd/#class-vec-mod "Copy/link to this API element")

```python
mod(self: Self, other: Vec[T, N], check: Literal[bool] = True, Self: type)
```

```python
mod(self: Self, other: T, check: Literal[bool] = True, Self: type)
```

sqrt[](https://docs.exaloop.io/libraries/api/modules/simd/#class-vec-sqrt "Copy/link to this API element")

```python
sqrt(self: Self, Self: type)
```

log[](https://docs.exaloop.io/libraries/api/modules/simd/#class-vec-log "Copy/link to this API element")

```python
log(self: Self, Self: type)
```

sin[](https://docs.exaloop.io/libraries/api/modules/simd/#class-vec-sin "Copy/link to this API element")

```python
sin(self: Self, Self: type)
```

cos[](https://docs.exaloop.io/libraries/api/modules/simd/#class-vec-cos "Copy/link to this API element")

```python
cos(self: Self, Self: type)
```

min[](https://docs.exaloop.io/libraries/api/modules/simd/#class-vec-min "Copy/link to this API element")

```python
min(self: Self, other: Vec[T, N], Self: type)
```

```python
min(self: Self, other: T, Self: type)
```

```python
min(self: Self, Self: type)
```

max[](https://docs.exaloop.io/libraries/api/modules/simd/#class-vec-max "Copy/link to this API element")

```python
max(self: Self, other: Vec[T, N], Self: type)
```

```python
max(self: Self, other: T, Self: type)
```

```python
max(self: Self, Self: type)
```

mask[](https://docs.exaloop.io/libraries/api/modules/simd/#class-vec-mask "Copy/link to this API element")

```python
mask(self: Self, other: Vec[T, N], mask, Self: type)
```

add[](https://docs.exaloop.io/libraries/api/modules/simd/#class-vec-add "Copy/link to this API element")

```python
add(self: Self, other: Vec[T, N], overflow: Literal[bool] = False, Self: type)
```

sub[](https://docs.exaloop.io/libraries/api/modules/simd/#class-vec-sub "Copy/link to this API element")

```python
sub(self: Self, other: Vec[T, N], overflow: Literal[bool] = False, Self: type)
```

sum[](https://docs.exaloop.io/libraries/api/modules/simd/#class-vec-sum "Copy/link to this API element")

```python
sum(self: Self, Self: type)
```

prod[](https://docs.exaloop.io/libraries/api/modules/simd/#class-vec-prod "Copy/link to this API element")

```python
prod(self: Self, Self: type)
```

fma[](https://docs.exaloop.io/libraries/api/modules/simd/#class-vec-fma "Copy/link to this API element")

```python
fma(self: Self, other, addend, Self: type)
```

exp[](https://docs.exaloop.io/libraries/api/modules/simd/#class-vec-exp "Copy/link to this API element")

```python
exp(self: Self, Self: type)
```

exp2[](https://docs.exaloop.io/libraries/api/modules/simd/#class-vec-exp2 "Copy/link to this API element")

```python
exp2(self: Self, Self: type)
```

log2[](https://docs.exaloop.io/libraries/api/modules/simd/#class-vec-log2 "Copy/link to this API element")

```python
log2(self: Self, Self: type)
```

log10[](https://docs.exaloop.io/libraries/api/modules/simd/#class-vec-log10 "Copy/link to this API element")

```python
log10(self: Self, Self: type)
```

floor[](https://docs.exaloop.io/libraries/api/modules/simd/#class-vec-floor "Copy/link to this API element")

```python
floor(self: Self, Self: type)
```

ceil[](https://docs.exaloop.io/libraries/api/modules/simd/#class-vec-ceil "Copy/link to this API element")

```python
ceil(self: Self, Self: type)
```

trunc[](https://docs.exaloop.io/libraries/api/modules/simd/#class-vec-trunc "Copy/link to this API element")

```python
trunc(self: Self, Self: type)
```

round[](https://docs.exaloop.io/libraries/api/modules/simd/#class-vec-round "Copy/link to this API element")

```python
round(self: Self, Self: type)
```

isnan[](https://docs.exaloop.io/libraries/api/modules/simd/#class-vec-isnan "Copy/link to this API element")

```python
isnan(self: Self, Self: type)
```

isinf[](https://docs.exaloop.io/libraries/api/modules/simd/#class-vec-isinf "Copy/link to this API element")

```python
isinf(self: Self, Self: type)
```

isfinite[](https://docs.exaloop.io/libraries/api/modules/simd/#class-vec-isfinite "Copy/link to this API element")

```python
isfinite(self: Self, Self: type)
```

signbit[](https://docs.exaloop.io/libraries/api/modules/simd/#class-vec-signbit "Copy/link to this API element")

```python
signbit(self: Self, Self: type)
```

bitflip[](https://docs.exaloop.io/libraries/api/modules/simd/#class-vec-bitflip "Copy/link to this API element")

```python
bitflip(self: Self, Self: type)
```

Special methods 35

\_\_copy\_\_[](https://docs.exaloop.io/libraries/api/modules/simd/#class-vec-__copy__ "Copy/link to this API element")

```python
__copy__(self: Self, Self: type)
```

\_\_len\_\_[](https://docs.exaloop.io/libraries/api/modules/simd/#class-vec-__len__ "Copy/link to this API element")

```python
__len__(self: Self, Self: type)
```

\_\_neg\_\_[](https://docs.exaloop.io/libraries/api/modules/simd/#class-vec-__neg__ "Copy/link to this API element")

```python
__neg__(self: Self, Self: type)
```

\_\_getitem\_\_[](https://docs.exaloop.io/libraries/api/modules/simd/#class-vec-__getitem__ "Copy/link to this API element")

```python
__getitem__(self: Self, index: int, Self: type)
```

```python
__getitem__(self: Self, n: Literal[int], Self: type)
```

\_\_repr\_\_[](https://docs.exaloop.io/libraries/api/modules/simd/#class-vec-__repr__ "Copy/link to this API element")

```python
__repr__(self: Self, Self: type)
```

\_\_eq\_\_[](https://docs.exaloop.io/libraries/api/modules/simd/#class-vec-__eq__ "Copy/link to this API element")

```python
__eq__(self: Self, other: Vec[T, N], Self: type)
```

```python
__eq__(self: Self, other: T, Self: type)
```

\_\_ne\_\_[](https://docs.exaloop.io/libraries/api/modules/simd/#class-vec-__ne__ "Copy/link to this API element")

```python
__ne__(self: Vec[T, N], other: Vec[T, N])
```

```python
__ne__(self: Self, other: T, Self: type)
```

\_\_and\_\_[](https://docs.exaloop.io/libraries/api/modules/simd/#class-vec-__and__ "Copy/link to this API element")

```python
__and__(self: Self, other: Vec[T, N], Self: type)
```

```python
__and__(self: Self, other: T, Self: type)
```

\_\_or\_\_[](https://docs.exaloop.io/libraries/api/modules/simd/#class-vec-__or__ "Copy/link to this API element")

```python
__or__(self: Self, other: Vec[T, N], Self: type)
```

```python
__or__(self: Self, other: T, Self: type)
```

\_\_xor\_\_[](https://docs.exaloop.io/libraries/api/modules/simd/#class-vec-__xor__ "Copy/link to this API element")

```python
__xor__(self: Self, other: Vec[T, N], Self: type)
```

```python
__xor__(self: Self, other: T, Self: type)
```

\_\_rand\_\_[](https://docs.exaloop.io/libraries/api/modules/simd/#class-vec-__rand__ "Copy/link to this API element")

```python
__rand__(self: Self, other: T, Self: type)
```

\_\_ror\_\_[](https://docs.exaloop.io/libraries/api/modules/simd/#class-vec-__ror__ "Copy/link to this API element")

```python
__ror__(self: Self, other: T, Self: type)
```

\_\_rxor\_\_[](https://docs.exaloop.io/libraries/api/modules/simd/#class-vec-__rxor__ "Copy/link to this API element")

```python
__rxor__(self: Self, other: T, Self: type)
```

\_\_invert\_\_[](https://docs.exaloop.io/libraries/api/modules/simd/#class-vec-__invert__ "Copy/link to this API element")

```python
__invert__(self: Self, Self: type)
```

\_\_lshift\_\_[](https://docs.exaloop.io/libraries/api/modules/simd/#class-vec-__lshift__ "Copy/link to this API element")

```python
__lshift__(self: Self, other: Vec[T, N], Self: type)
```

```python
__lshift__(self: Self, other: T, Self: type)
```

```python
__lshift__(self: Self, other: int, Self: type)
```

\_\_rlshift\_\_[](https://docs.exaloop.io/libraries/api/modules/simd/#class-vec-__rlshift__ "Copy/link to this API element")

```python
__rlshift__(self: Self, other: T, Self: type)
```

\_\_rshift\_\_[](https://docs.exaloop.io/libraries/api/modules/simd/#class-vec-__rshift__ "Copy/link to this API element")

```python
__rshift__(self: Self, other: Vec[T, N], Self: type)
```

```python
__rshift__(self: Self, other: T, Self: type)
```

```python
__rshift__(self: Self, other: int, Self: type)
```

\_\_rrshift\_\_[](https://docs.exaloop.io/libraries/api/modules/simd/#class-vec-__rrshift__ "Copy/link to this API element")

```python
__rrshift__(self: Self, other: T, Self: type)
```

\_\_add\_\_[](https://docs.exaloop.io/libraries/api/modules/simd/#class-vec-__add__ "Copy/link to this API element")

```python
__add__(self: Self, other: Vec[T, N], Self: type)
```

```python
__add__(self: Self, other: T, Self: type)
```

\_\_radd\_\_[](https://docs.exaloop.io/libraries/api/modules/simd/#class-vec-__radd__ "Copy/link to this API element")

```python
__radd__(self: Self, other: T, Self: type)
```

\_\_sub\_\_[](https://docs.exaloop.io/libraries/api/modules/simd/#class-vec-__sub__ "Copy/link to this API element")

```python
__sub__(self: Self, other: Vec[T, N], Self: type)
```

```python
__sub__(self: Self, other: T, Self: type)
```

\_\_rsub\_\_[](https://docs.exaloop.io/libraries/api/modules/simd/#class-vec-__rsub__ "Copy/link to this API element")

```python
__rsub__(self: Self, other: T, Self: type)
```

\_\_mul\_\_[](https://docs.exaloop.io/libraries/api/modules/simd/#class-vec-__mul__ "Copy/link to this API element")

```python
__mul__(self: Self, other: Vec[T, N], Self: type)
```

```python
__mul__(self: Self, other: T, Self: type)
```

\_\_rmul\_\_[](https://docs.exaloop.io/libraries/api/modules/simd/#class-vec-__rmul__ "Copy/link to this API element")

```python
__rmul__(self: Self, other: T, Self: type)
```

\_\_truediv\_\_[](https://docs.exaloop.io/libraries/api/modules/simd/#class-vec-__truediv__ "Copy/link to this API element")

```python
__truediv__(self: Self, other: Vec[T, N], Self: type)
```

```python
__truediv__(self: Self, other: T, Self: type)
```

\_\_rtruediv\_\_[](https://docs.exaloop.io/libraries/api/modules/simd/#class-vec-__rtruediv__ "Copy/link to this API element")

```python
__rtruediv__(self: Self, other: T, Self: type)
```

\_\_floordiv\_\_[](https://docs.exaloop.io/libraries/api/modules/simd/#class-vec-__floordiv__ "Copy/link to this API element")

```python
__floordiv__(self: Self, other: Vec[T, N], Self: type)
```

```python
__floordiv__(self: Self, other: T, Self: type)
```

\_\_rfloordiv\_\_[](https://docs.exaloop.io/libraries/api/modules/simd/#class-vec-__rfloordiv__ "Copy/link to this API element")

```python
__rfloordiv__(self: Self, other: T, Self: type)
```

\_\_mod\_\_[](https://docs.exaloop.io/libraries/api/modules/simd/#class-vec-__mod__ "Copy/link to this API element")

```python
__mod__(self: Self, other: Vec[T, N], Self: type)
```

```python
__mod__(self: Self, other: T, Self: type)
```

\_\_rmod\_\_[](https://docs.exaloop.io/libraries/api/modules/simd/#class-vec-__rmod__ "Copy/link to this API element")

```python
__rmod__(self: Self, other: T, Self: type)
```

\_\_lt\_\_[](https://docs.exaloop.io/libraries/api/modules/simd/#class-vec-__lt__ "Copy/link to this API element")

```python
__lt__(self: Self, other: Vec[T, N], Self: type)
```

```python
__lt__(self: Self, other: T, Self: type)
```

\_\_le\_\_[](https://docs.exaloop.io/libraries/api/modules/simd/#class-vec-__le__ "Copy/link to this API element")

```python
__le__(self: Self, other: Vec[T, N], Self: type)
```

```python
__le__(self: Self, other: T, Self: type)
```

\_\_gt\_\_[](https://docs.exaloop.io/libraries/api/modules/simd/#class-vec-__gt__ "Copy/link to this API element")

```python
__gt__(self: Self, other: Vec[T, N], Self: type)
```

```python
__gt__(self: Self, other: T, Self: type)
```

\_\_ge\_\_[](https://docs.exaloop.io/libraries/api/modules/simd/#class-vec-__ge__ "Copy/link to this API element")

```python
__ge__(self: Self, other: Vec[T, N], Self: type)
```

```python
__ge__(self: Self, other: T, Self: type)
```

\_\_abs\_\_[](https://docs.exaloop.io/libraries/api/modules/simd/#class-vec-__abs__ "Copy/link to this API element")

```python
__abs__(self: Self, Self: type)
```

class Ptr[](https://docs.exaloop.io/libraries/api/modules/simd/#class-ptr "Copy/link to this API element")

@extend

```python
class Ptr[T]
```

Extends [`Ptr`](https://docs.exaloop.io/libraries/api/classes/Ptr/) · [View complete class API →](https://docs.exaloop.io/libraries/api/classes/Ptr/)

Special methods 2

\_\_getitem\_\_[](https://docs.exaloop.io/libraries/api/modules/simd/#class-ptr-__getitem__ "Copy/link to this API element")

@pure @derives @llvm

```python
__getitem__(self: Ptr[Vec[S, M]], index: int, S: type, M: Literal[int])
```

```llvm
%address = getelementptr <{=M} x {=S}>, ptr %self, i64 %index
%result = load <{=M} x {=S}>, ptr %address, align 1
ret <{=M} x {=S}> %result
```

\_\_setitem\_\_[](https://docs.exaloop.io/libraries/api/modules/simd/#class-ptr-__setitem__ "Copy/link to this API element")

@self\_captures @llvm

```python
__setitem__(self: Ptr[Vec[S, M]], index: int, what: Vec[S, M], S: type, M: Literal[int])
```

```llvm
%address = getelementptr <{=M} x {=S}>, ptr %self, i64 %index
store <{=M} x {=S}> %what, ptr %address, align 1
ret {} zeroinitializer
```

Source: https://docs.exaloop.io/libraries/api/modules/simd/
