# Vec

Standard library class `simd.Vec` [View source](https://github.com/exaloop/codon/blob/master/stdlib/simd.codon)

class Vec[](https://docs.exaloop.io/libraries/api/classes/simd/Vec/#class-vec "Copy/link to this API element")

@tuple

```python
class Vec[T, N]
```

Fields 2

`T` `type`

`N` `Literal[int]`

Constructors 4

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/classes/simd/Vec/#class-vec-__new__ "Copy/link to this API element")

```python
__new__(x, check: Literal[bool] = True)
```

Defined in [`simd`](https://docs.exaloop.io/libraries/api/modules/simd/)

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/classes/simd/Vec/#class-vec-__new__-overload-2 "Copy/link to this API element")

```python
__new__(x: str, offset: int = 0, check: Literal[bool] = True)
```

Defined in [`simd`](https://docs.exaloop.io/libraries/api/modules/simd/)

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/classes/simd/Vec/#class-vec-__new__-overload-3 "Copy/link to this API element")

```python
__new__(x: bytes, offset: int = 0, check: Literal[bool] = True)
```

Defined in [`simd`](https://docs.exaloop.io/libraries/api/modules/simd/)

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/classes/simd/Vec/#class-vec-__new__-overload-4 "Copy/link to this API element")

```python
__new__(x: List[T], offset: int = 0, check: Literal[bool] = True)
```

Defined in [`simd`](https://docs.exaloop.io/libraries/api/modules/simd/)

Methods 46

load[](https://docs.exaloop.io/libraries/api/classes/simd/Vec/#class-vec-load "Copy/link to this API element")

```python
load(data: Ptr[T], offset: int = 0, alignment: Literal[int] = 1, mask = NoneType(), passthrough = NoneType())
```

Defined in [`simd`](https://docs.exaloop.io/libraries/api/modules/simd/)

store[](https://docs.exaloop.io/libraries/api/classes/simd/Vec/#class-vec-store "Copy/link to this API element")

```python
store(self: Self, data: Ptr[T], offset: int = 0, alignment: Literal[int] = 1, mask = NoneType(), Self: type)
```

Defined in [`simd`](https://docs.exaloop.io/libraries/api/modules/simd/)

gather[](https://docs.exaloop.io/libraries/api/classes/simd/Vec/#class-vec-gather "Copy/link to this API element")

```python
gather(data: Ptr[T], indices: Vec[int, N], alignment: Literal[int] = 1, mask = NoneType(), passthrough = NoneType())
```

Defined in [`simd`](https://docs.exaloop.io/libraries/api/modules/simd/)

scatter[](https://docs.exaloop.io/libraries/api/classes/simd/Vec/#class-vec-scatter "Copy/link to this API element")

```python
scatter(self: Self, data: Ptr[T], indices: Vec[int, N], alignment: Literal[int] = 1, mask = NoneType(), Self: type)
```

Defined in [`simd`](https://docs.exaloop.io/libraries/api/modules/simd/)

```python
scatter(self: Vec[T, N])
```

Defined in [`simd`](https://docs.exaloop.io/libraries/api/modules/simd/)

cast[](https://docs.exaloop.io/libraries/api/classes/simd/Vec/#class-vec-cast "Copy/link to this API element")

```python
cast(self: Self, T2: type, Self: type)
```

Defined in [`simd`](https://docs.exaloop.io/libraries/api/modules/simd/)

cast\_ptr[](https://docs.exaloop.io/libraries/api/classes/simd/Vec/#class-vec-cast_ptr "Copy/link to this API element")

@pure @llvm

```python
cast_ptr(ptr: Ptr[T], T: type, T2: type)
```

Defined in [`simd`](https://docs.exaloop.io/libraries/api/modules/simd/)

```llvm
ret ptr %ptr
```

extract[](https://docs.exaloop.io/libraries/api/classes/simd/Vec/#class-vec-extract "Copy/link to this API element")

```python
extract(self: Self, index: int, check: Literal[bool] = True, Self: type)
```

Defined in [`simd`](https://docs.exaloop.io/libraries/api/modules/simd/)

insert[](https://docs.exaloop.io/libraries/api/classes/simd/Vec/#class-vec-insert "Copy/link to this API element")

```python
insert(self: Self, index: int, value: T, check: Literal[bool] = True, Self: type)
```

Defined in [`simd`](https://docs.exaloop.io/libraries/api/modules/simd/)

shuffle[](https://docs.exaloop.io/libraries/api/classes/simd/Vec/#class-vec-shuffle "Copy/link to this API element")

```python
shuffle(self: Self, indices, other = NoneType(), check: Literal[bool] = True, Self: type)
```

Defined in [`simd`](https://docs.exaloop.io/libraries/api/modules/simd/)

interleave[](https://docs.exaloop.io/libraries/api/classes/simd/Vec/#class-vec-interleave "Copy/link to this API element")

```python
interleave(self: Self, other: Vec[T, N], Self: type)
```

Defined in [`simd`](https://docs.exaloop.io/libraries/api/modules/simd/)

lshift[](https://docs.exaloop.io/libraries/api/classes/simd/Vec/#class-vec-lshift "Copy/link to this API element")

```python
lshift(self: Self, other: Vec[T, N], check: Literal[bool] = True, Self: type)
```

Defined in [`simd`](https://docs.exaloop.io/libraries/api/modules/simd/)

```python
lshift(self: Self, other: T, check: Literal[bool] = True, Self: type)
```

Defined in [`simd`](https://docs.exaloop.io/libraries/api/modules/simd/)

```python
lshift(self: Self, other: int, check: Literal[bool] = True, Self: type)
```

Defined in [`simd`](https://docs.exaloop.io/libraries/api/modules/simd/)

lsh[](https://docs.exaloop.io/libraries/api/classes/simd/Vec/#class-vec-lsh "Copy/link to this API element")

```python
lsh(self: Self, other: Literal[int], Self: type)
```

Defined in [`simd`](https://docs.exaloop.io/libraries/api/modules/simd/)

rshift[](https://docs.exaloop.io/libraries/api/classes/simd/Vec/#class-vec-rshift "Copy/link to this API element")

```python
rshift(self: Self, other: Vec[T, N], check: Literal[bool] = True, Self: type)
```

Defined in [`simd`](https://docs.exaloop.io/libraries/api/modules/simd/)

```python
rshift(self: Self, other: T, check: Literal[bool] = True, Self: type)
```

Defined in [`simd`](https://docs.exaloop.io/libraries/api/modules/simd/)

```python
rshift(self: Self, other: int, check: Literal[bool] = True, Self: type)
```

Defined in [`simd`](https://docs.exaloop.io/libraries/api/modules/simd/)

rsh[](https://docs.exaloop.io/libraries/api/classes/simd/Vec/#class-vec-rsh "Copy/link to this API element")

```python
rsh(self: Self, other: Literal[int], Self: type)
```

Defined in [`simd`](https://docs.exaloop.io/libraries/api/modules/simd/)

any[](https://docs.exaloop.io/libraries/api/classes/simd/Vec/#class-vec-any "Copy/link to this API element")

```python
any(self: Self, Self: type)
```

Defined in [`simd`](https://docs.exaloop.io/libraries/api/modules/simd/)

all[](https://docs.exaloop.io/libraries/api/classes/simd/Vec/#class-vec-all "Copy/link to this API element")

```python
all(self: Self, Self: type)
```

Defined in [`simd`](https://docs.exaloop.io/libraries/api/modules/simd/)

count[](https://docs.exaloop.io/libraries/api/classes/simd/Vec/#class-vec-count "Copy/link to this API element")

```python
count(self: Self, Self: type)
```

Defined in [`simd`](https://docs.exaloop.io/libraries/api/modules/simd/)

to\_bits[](https://docs.exaloop.io/libraries/api/classes/simd/Vec/#class-vec-to_bits "Copy/link to this API element")

```python
to_bits(self: Self, Self: type)
```

Defined in [`simd`](https://docs.exaloop.io/libraries/api/modules/simd/)

from\_bits[](https://docs.exaloop.io/libraries/api/classes/simd/Vec/#class-vec-from_bits "Copy/link to this API element")

```python
from_bits(bits: UInt[N])
```

Defined in [`simd`](https://docs.exaloop.io/libraries/api/modules/simd/)

floordiv[](https://docs.exaloop.io/libraries/api/classes/simd/Vec/#class-vec-floordiv "Copy/link to this API element")

```python
floordiv(self: Self, other: Vec[T, N], check: Literal[bool] = True, Self: type)
```

Defined in [`simd`](https://docs.exaloop.io/libraries/api/modules/simd/)

```python
floordiv(self: Self, other: T, check: Literal[bool] = True, Self: type)
```

Defined in [`simd`](https://docs.exaloop.io/libraries/api/modules/simd/)

mod[](https://docs.exaloop.io/libraries/api/classes/simd/Vec/#class-vec-mod "Copy/link to this API element")

```python
mod(self: Self, other: Vec[T, N], check: Literal[bool] = True, Self: type)
```

Defined in [`simd`](https://docs.exaloop.io/libraries/api/modules/simd/)

```python
mod(self: Self, other: T, check: Literal[bool] = True, Self: type)
```

Defined in [`simd`](https://docs.exaloop.io/libraries/api/modules/simd/)

sqrt[](https://docs.exaloop.io/libraries/api/classes/simd/Vec/#class-vec-sqrt "Copy/link to this API element")

```python
sqrt(self: Self, Self: type)
```

Defined in [`simd`](https://docs.exaloop.io/libraries/api/modules/simd/)

log[](https://docs.exaloop.io/libraries/api/classes/simd/Vec/#class-vec-log "Copy/link to this API element")

```python
log(self: Self, Self: type)
```

Defined in [`simd`](https://docs.exaloop.io/libraries/api/modules/simd/)

sin[](https://docs.exaloop.io/libraries/api/classes/simd/Vec/#class-vec-sin "Copy/link to this API element")

```python
sin(self: Self, Self: type)
```

Defined in [`simd`](https://docs.exaloop.io/libraries/api/modules/simd/)

cos[](https://docs.exaloop.io/libraries/api/classes/simd/Vec/#class-vec-cos "Copy/link to this API element")

```python
cos(self: Self, Self: type)
```

Defined in [`simd`](https://docs.exaloop.io/libraries/api/modules/simd/)

min[](https://docs.exaloop.io/libraries/api/classes/simd/Vec/#class-vec-min "Copy/link to this API element")

```python
min(self: Self, other: Vec[T, N], Self: type)
```

Defined in [`simd`](https://docs.exaloop.io/libraries/api/modules/simd/)

```python
min(self: Self, other: T, Self: type)
```

Defined in [`simd`](https://docs.exaloop.io/libraries/api/modules/simd/)

```python
min(self: Self, Self: type)
```

Defined in [`simd`](https://docs.exaloop.io/libraries/api/modules/simd/)

max[](https://docs.exaloop.io/libraries/api/classes/simd/Vec/#class-vec-max "Copy/link to this API element")

```python
max(self: Self, other: Vec[T, N], Self: type)
```

Defined in [`simd`](https://docs.exaloop.io/libraries/api/modules/simd/)

```python
max(self: Self, other: T, Self: type)
```

Defined in [`simd`](https://docs.exaloop.io/libraries/api/modules/simd/)

```python
max(self: Self, Self: type)
```

Defined in [`simd`](https://docs.exaloop.io/libraries/api/modules/simd/)

mask[](https://docs.exaloop.io/libraries/api/classes/simd/Vec/#class-vec-mask "Copy/link to this API element")

```python
mask(self: Self, other: Vec[T, N], mask, Self: type)
```

Defined in [`simd`](https://docs.exaloop.io/libraries/api/modules/simd/)

add[](https://docs.exaloop.io/libraries/api/classes/simd/Vec/#class-vec-add "Copy/link to this API element")

```python
add(self: Self, other: Vec[T, N], overflow: Literal[bool] = False, Self: type)
```

Defined in [`simd`](https://docs.exaloop.io/libraries/api/modules/simd/)

sub[](https://docs.exaloop.io/libraries/api/classes/simd/Vec/#class-vec-sub "Copy/link to this API element")

```python
sub(self: Self, other: Vec[T, N], overflow: Literal[bool] = False, Self: type)
```

Defined in [`simd`](https://docs.exaloop.io/libraries/api/modules/simd/)

sum[](https://docs.exaloop.io/libraries/api/classes/simd/Vec/#class-vec-sum "Copy/link to this API element")

```python
sum(self: Self, Self: type)
```

Defined in [`simd`](https://docs.exaloop.io/libraries/api/modules/simd/)

prod[](https://docs.exaloop.io/libraries/api/classes/simd/Vec/#class-vec-prod "Copy/link to this API element")

```python
prod(self: Self, Self: type)
```

Defined in [`simd`](https://docs.exaloop.io/libraries/api/modules/simd/)

fma[](https://docs.exaloop.io/libraries/api/classes/simd/Vec/#class-vec-fma "Copy/link to this API element")

```python
fma(self: Self, other, addend, Self: type)
```

Defined in [`simd`](https://docs.exaloop.io/libraries/api/modules/simd/)

exp[](https://docs.exaloop.io/libraries/api/classes/simd/Vec/#class-vec-exp "Copy/link to this API element")

```python
exp(self: Self, Self: type)
```

Defined in [`simd`](https://docs.exaloop.io/libraries/api/modules/simd/)

exp2[](https://docs.exaloop.io/libraries/api/classes/simd/Vec/#class-vec-exp2 "Copy/link to this API element")

```python
exp2(self: Self, Self: type)
```

Defined in [`simd`](https://docs.exaloop.io/libraries/api/modules/simd/)

log2[](https://docs.exaloop.io/libraries/api/classes/simd/Vec/#class-vec-log2 "Copy/link to this API element")

```python
log2(self: Self, Self: type)
```

Defined in [`simd`](https://docs.exaloop.io/libraries/api/modules/simd/)

log10[](https://docs.exaloop.io/libraries/api/classes/simd/Vec/#class-vec-log10 "Copy/link to this API element")

```python
log10(self: Self, Self: type)
```

Defined in [`simd`](https://docs.exaloop.io/libraries/api/modules/simd/)

floor[](https://docs.exaloop.io/libraries/api/classes/simd/Vec/#class-vec-floor "Copy/link to this API element")

```python
floor(self: Self, Self: type)
```

Defined in [`simd`](https://docs.exaloop.io/libraries/api/modules/simd/)

ceil[](https://docs.exaloop.io/libraries/api/classes/simd/Vec/#class-vec-ceil "Copy/link to this API element")

```python
ceil(self: Self, Self: type)
```

Defined in [`simd`](https://docs.exaloop.io/libraries/api/modules/simd/)

trunc[](https://docs.exaloop.io/libraries/api/classes/simd/Vec/#class-vec-trunc "Copy/link to this API element")

```python
trunc(self: Self, Self: type)
```

Defined in [`simd`](https://docs.exaloop.io/libraries/api/modules/simd/)

round[](https://docs.exaloop.io/libraries/api/classes/simd/Vec/#class-vec-round "Copy/link to this API element")

```python
round(self: Self, Self: type)
```

Defined in [`simd`](https://docs.exaloop.io/libraries/api/modules/simd/)

isnan[](https://docs.exaloop.io/libraries/api/classes/simd/Vec/#class-vec-isnan "Copy/link to this API element")

```python
isnan(self: Self, Self: type)
```

Defined in [`simd`](https://docs.exaloop.io/libraries/api/modules/simd/)

isinf[](https://docs.exaloop.io/libraries/api/classes/simd/Vec/#class-vec-isinf "Copy/link to this API element")

```python
isinf(self: Self, Self: type)
```

Defined in [`simd`](https://docs.exaloop.io/libraries/api/modules/simd/)

isfinite[](https://docs.exaloop.io/libraries/api/classes/simd/Vec/#class-vec-isfinite "Copy/link to this API element")

```python
isfinite(self: Self, Self: type)
```

Defined in [`simd`](https://docs.exaloop.io/libraries/api/modules/simd/)

signbit[](https://docs.exaloop.io/libraries/api/classes/simd/Vec/#class-vec-signbit "Copy/link to this API element")

```python
signbit(self: Self, Self: type)
```

Defined in [`simd`](https://docs.exaloop.io/libraries/api/modules/simd/)

bitflip[](https://docs.exaloop.io/libraries/api/classes/simd/Vec/#class-vec-bitflip "Copy/link to this API element")

```python
bitflip(self: Self, Self: type)
```

Defined in [`simd`](https://docs.exaloop.io/libraries/api/modules/simd/)

Special methods 35

\_\_copy\_\_[](https://docs.exaloop.io/libraries/api/classes/simd/Vec/#class-vec-__copy__ "Copy/link to this API element")

```python
__copy__(self: Self, Self: type)
```

Defined in [`simd`](https://docs.exaloop.io/libraries/api/modules/simd/)

\_\_len\_\_[](https://docs.exaloop.io/libraries/api/classes/simd/Vec/#class-vec-__len__ "Copy/link to this API element")

```python
__len__(self: Self, Self: type)
```

Defined in [`simd`](https://docs.exaloop.io/libraries/api/modules/simd/)

\_\_neg\_\_[](https://docs.exaloop.io/libraries/api/classes/simd/Vec/#class-vec-__neg__ "Copy/link to this API element")

```python
__neg__(self: Self, Self: type)
```

Defined in [`simd`](https://docs.exaloop.io/libraries/api/modules/simd/)

\_\_getitem\_\_[](https://docs.exaloop.io/libraries/api/classes/simd/Vec/#class-vec-__getitem__ "Copy/link to this API element")

```python
__getitem__(self: Self, index: int, Self: type)
```

Defined in [`simd`](https://docs.exaloop.io/libraries/api/modules/simd/)

```python
__getitem__(self: Self, n: Literal[int], Self: type)
```

Defined in [`simd`](https://docs.exaloop.io/libraries/api/modules/simd/)

\_\_repr\_\_[](https://docs.exaloop.io/libraries/api/classes/simd/Vec/#class-vec-__repr__ "Copy/link to this API element")

```python
__repr__(self: Self, Self: type)
```

Defined in [`simd`](https://docs.exaloop.io/libraries/api/modules/simd/)

\_\_eq\_\_[](https://docs.exaloop.io/libraries/api/classes/simd/Vec/#class-vec-__eq__ "Copy/link to this API element")

```python
__eq__(self: Self, other: Vec[T, N], Self: type)
```

Defined in [`simd`](https://docs.exaloop.io/libraries/api/modules/simd/)

```python
__eq__(self: Self, other: T, Self: type)
```

Defined in [`simd`](https://docs.exaloop.io/libraries/api/modules/simd/)

\_\_ne\_\_[](https://docs.exaloop.io/libraries/api/classes/simd/Vec/#class-vec-__ne__ "Copy/link to this API element")

```python
__ne__(self: Vec[T, N], other: Vec[T, N])
```

Defined in [`simd`](https://docs.exaloop.io/libraries/api/modules/simd/)

```python
__ne__(self: Self, other: T, Self: type)
```

Defined in [`simd`](https://docs.exaloop.io/libraries/api/modules/simd/)

\_\_and\_\_[](https://docs.exaloop.io/libraries/api/classes/simd/Vec/#class-vec-__and__ "Copy/link to this API element")

```python
__and__(self: Self, other: Vec[T, N], Self: type)
```

Defined in [`simd`](https://docs.exaloop.io/libraries/api/modules/simd/)

```python
__and__(self: Self, other: T, Self: type)
```

Defined in [`simd`](https://docs.exaloop.io/libraries/api/modules/simd/)

\_\_or\_\_[](https://docs.exaloop.io/libraries/api/classes/simd/Vec/#class-vec-__or__ "Copy/link to this API element")

```python
__or__(self: Self, other: Vec[T, N], Self: type)
```

Defined in [`simd`](https://docs.exaloop.io/libraries/api/modules/simd/)

```python
__or__(self: Self, other: T, Self: type)
```

Defined in [`simd`](https://docs.exaloop.io/libraries/api/modules/simd/)

\_\_xor\_\_[](https://docs.exaloop.io/libraries/api/classes/simd/Vec/#class-vec-__xor__ "Copy/link to this API element")

```python
__xor__(self: Self, other: Vec[T, N], Self: type)
```

Defined in [`simd`](https://docs.exaloop.io/libraries/api/modules/simd/)

```python
__xor__(self: Self, other: T, Self: type)
```

Defined in [`simd`](https://docs.exaloop.io/libraries/api/modules/simd/)

\_\_rand\_\_[](https://docs.exaloop.io/libraries/api/classes/simd/Vec/#class-vec-__rand__ "Copy/link to this API element")

```python
__rand__(self: Self, other: T, Self: type)
```

Defined in [`simd`](https://docs.exaloop.io/libraries/api/modules/simd/)

\_\_ror\_\_[](https://docs.exaloop.io/libraries/api/classes/simd/Vec/#class-vec-__ror__ "Copy/link to this API element")

```python
__ror__(self: Self, other: T, Self: type)
```

Defined in [`simd`](https://docs.exaloop.io/libraries/api/modules/simd/)

\_\_rxor\_\_[](https://docs.exaloop.io/libraries/api/classes/simd/Vec/#class-vec-__rxor__ "Copy/link to this API element")

```python
__rxor__(self: Self, other: T, Self: type)
```

Defined in [`simd`](https://docs.exaloop.io/libraries/api/modules/simd/)

\_\_invert\_\_[](https://docs.exaloop.io/libraries/api/classes/simd/Vec/#class-vec-__invert__ "Copy/link to this API element")

```python
__invert__(self: Self, Self: type)
```

Defined in [`simd`](https://docs.exaloop.io/libraries/api/modules/simd/)

\_\_lshift\_\_[](https://docs.exaloop.io/libraries/api/classes/simd/Vec/#class-vec-__lshift__ "Copy/link to this API element")

```python
__lshift__(self: Self, other: Vec[T, N], Self: type)
```

Defined in [`simd`](https://docs.exaloop.io/libraries/api/modules/simd/)

```python
__lshift__(self: Self, other: T, Self: type)
```

Defined in [`simd`](https://docs.exaloop.io/libraries/api/modules/simd/)

```python
__lshift__(self: Self, other: int, Self: type)
```

Defined in [`simd`](https://docs.exaloop.io/libraries/api/modules/simd/)

\_\_rlshift\_\_[](https://docs.exaloop.io/libraries/api/classes/simd/Vec/#class-vec-__rlshift__ "Copy/link to this API element")

```python
__rlshift__(self: Self, other: T, Self: type)
```

Defined in [`simd`](https://docs.exaloop.io/libraries/api/modules/simd/)

\_\_rshift\_\_[](https://docs.exaloop.io/libraries/api/classes/simd/Vec/#class-vec-__rshift__ "Copy/link to this API element")

```python
__rshift__(self: Self, other: Vec[T, N], Self: type)
```

Defined in [`simd`](https://docs.exaloop.io/libraries/api/modules/simd/)

```python
__rshift__(self: Self, other: T, Self: type)
```

Defined in [`simd`](https://docs.exaloop.io/libraries/api/modules/simd/)

```python
__rshift__(self: Self, other: int, Self: type)
```

Defined in [`simd`](https://docs.exaloop.io/libraries/api/modules/simd/)

\_\_rrshift\_\_[](https://docs.exaloop.io/libraries/api/classes/simd/Vec/#class-vec-__rrshift__ "Copy/link to this API element")

```python
__rrshift__(self: Self, other: T, Self: type)
```

Defined in [`simd`](https://docs.exaloop.io/libraries/api/modules/simd/)

\_\_add\_\_[](https://docs.exaloop.io/libraries/api/classes/simd/Vec/#class-vec-__add__ "Copy/link to this API element")

```python
__add__(self: Self, other: Vec[T, N], Self: type)
```

Defined in [`simd`](https://docs.exaloop.io/libraries/api/modules/simd/)

```python
__add__(self: Self, other: T, Self: type)
```

Defined in [`simd`](https://docs.exaloop.io/libraries/api/modules/simd/)

\_\_radd\_\_[](https://docs.exaloop.io/libraries/api/classes/simd/Vec/#class-vec-__radd__ "Copy/link to this API element")

```python
__radd__(self: Self, other: T, Self: type)
```

Defined in [`simd`](https://docs.exaloop.io/libraries/api/modules/simd/)

\_\_sub\_\_[](https://docs.exaloop.io/libraries/api/classes/simd/Vec/#class-vec-__sub__ "Copy/link to this API element")

```python
__sub__(self: Self, other: Vec[T, N], Self: type)
```

Defined in [`simd`](https://docs.exaloop.io/libraries/api/modules/simd/)

```python
__sub__(self: Self, other: T, Self: type)
```

Defined in [`simd`](https://docs.exaloop.io/libraries/api/modules/simd/)

\_\_rsub\_\_[](https://docs.exaloop.io/libraries/api/classes/simd/Vec/#class-vec-__rsub__ "Copy/link to this API element")

```python
__rsub__(self: Self, other: T, Self: type)
```

Defined in [`simd`](https://docs.exaloop.io/libraries/api/modules/simd/)

\_\_mul\_\_[](https://docs.exaloop.io/libraries/api/classes/simd/Vec/#class-vec-__mul__ "Copy/link to this API element")

```python
__mul__(self: Self, other: Vec[T, N], Self: type)
```

Defined in [`simd`](https://docs.exaloop.io/libraries/api/modules/simd/)

```python
__mul__(self: Self, other: T, Self: type)
```

Defined in [`simd`](https://docs.exaloop.io/libraries/api/modules/simd/)

\_\_rmul\_\_[](https://docs.exaloop.io/libraries/api/classes/simd/Vec/#class-vec-__rmul__ "Copy/link to this API element")

```python
__rmul__(self: Self, other: T, Self: type)
```

Defined in [`simd`](https://docs.exaloop.io/libraries/api/modules/simd/)

\_\_truediv\_\_[](https://docs.exaloop.io/libraries/api/classes/simd/Vec/#class-vec-__truediv__ "Copy/link to this API element")

```python
__truediv__(self: Self, other: Vec[T, N], Self: type)
```

Defined in [`simd`](https://docs.exaloop.io/libraries/api/modules/simd/)

```python
__truediv__(self: Self, other: T, Self: type)
```

Defined in [`simd`](https://docs.exaloop.io/libraries/api/modules/simd/)

\_\_rtruediv\_\_[](https://docs.exaloop.io/libraries/api/classes/simd/Vec/#class-vec-__rtruediv__ "Copy/link to this API element")

```python
__rtruediv__(self: Self, other: T, Self: type)
```

Defined in [`simd`](https://docs.exaloop.io/libraries/api/modules/simd/)

\_\_floordiv\_\_[](https://docs.exaloop.io/libraries/api/classes/simd/Vec/#class-vec-__floordiv__ "Copy/link to this API element")

```python
__floordiv__(self: Self, other: Vec[T, N], Self: type)
```

Defined in [`simd`](https://docs.exaloop.io/libraries/api/modules/simd/)

```python
__floordiv__(self: Self, other: T, Self: type)
```

Defined in [`simd`](https://docs.exaloop.io/libraries/api/modules/simd/)

\_\_rfloordiv\_\_[](https://docs.exaloop.io/libraries/api/classes/simd/Vec/#class-vec-__rfloordiv__ "Copy/link to this API element")

```python
__rfloordiv__(self: Self, other: T, Self: type)
```

Defined in [`simd`](https://docs.exaloop.io/libraries/api/modules/simd/)

\_\_mod\_\_[](https://docs.exaloop.io/libraries/api/classes/simd/Vec/#class-vec-__mod__ "Copy/link to this API element")

```python
__mod__(self: Self, other: Vec[T, N], Self: type)
```

Defined in [`simd`](https://docs.exaloop.io/libraries/api/modules/simd/)

```python
__mod__(self: Self, other: T, Self: type)
```

Defined in [`simd`](https://docs.exaloop.io/libraries/api/modules/simd/)

\_\_rmod\_\_[](https://docs.exaloop.io/libraries/api/classes/simd/Vec/#class-vec-__rmod__ "Copy/link to this API element")

```python
__rmod__(self: Self, other: T, Self: type)
```

Defined in [`simd`](https://docs.exaloop.io/libraries/api/modules/simd/)

\_\_lt\_\_[](https://docs.exaloop.io/libraries/api/classes/simd/Vec/#class-vec-__lt__ "Copy/link to this API element")

```python
__lt__(self: Self, other: Vec[T, N], Self: type)
```

Defined in [`simd`](https://docs.exaloop.io/libraries/api/modules/simd/)

```python
__lt__(self: Self, other: T, Self: type)
```

Defined in [`simd`](https://docs.exaloop.io/libraries/api/modules/simd/)

\_\_le\_\_[](https://docs.exaloop.io/libraries/api/classes/simd/Vec/#class-vec-__le__ "Copy/link to this API element")

```python
__le__(self: Self, other: Vec[T, N], Self: type)
```

Defined in [`simd`](https://docs.exaloop.io/libraries/api/modules/simd/)

```python
__le__(self: Self, other: T, Self: type)
```

Defined in [`simd`](https://docs.exaloop.io/libraries/api/modules/simd/)

\_\_gt\_\_[](https://docs.exaloop.io/libraries/api/classes/simd/Vec/#class-vec-__gt__ "Copy/link to this API element")

```python
__gt__(self: Self, other: Vec[T, N], Self: type)
```

Defined in [`simd`](https://docs.exaloop.io/libraries/api/modules/simd/)

```python
__gt__(self: Self, other: T, Self: type)
```

Defined in [`simd`](https://docs.exaloop.io/libraries/api/modules/simd/)

\_\_ge\_\_[](https://docs.exaloop.io/libraries/api/classes/simd/Vec/#class-vec-__ge__ "Copy/link to this API element")

```python
__ge__(self: Self, other: Vec[T, N], Self: type)
```

Defined in [`simd`](https://docs.exaloop.io/libraries/api/modules/simd/)

```python
__ge__(self: Self, other: T, Self: type)
```

Defined in [`simd`](https://docs.exaloop.io/libraries/api/modules/simd/)

\_\_abs\_\_[](https://docs.exaloop.io/libraries/api/classes/simd/Vec/#class-vec-__abs__ "Copy/link to this API element")

```python
__abs__(self: Self, Self: type)
```

Defined in [`simd`](https://docs.exaloop.io/libraries/api/modules/simd/)

Source: https://docs.exaloop.io/libraries/api/classes/simd/Vec/
