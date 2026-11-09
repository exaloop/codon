# numpy.util

Standard library module [View source](https://github.com/exaloop/codon/blob/master/stdlib/numpy/util.codon)

class AxisError[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#class-axiserror "Copy/link to this API element")

```python
class AxisError
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/numpy/util/AxisError/)

Constructors 1

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#class-axiserror-__init__ "Copy/link to this API element")

```python
__init__(self: Self, message: str = "", Self: type)
```

class TooHardError[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#class-tooharderror "Copy/link to this API element")

```python
class TooHardError
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/numpy/util/TooHardError/)

Constructors 1

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#class-tooharderror-__init__ "Copy/link to this API element")

```python
__init__(self: Self, message: str = "", Self: type)
```

function tuple\_get[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-tuple_get "Copy/link to this API element")

```python
tuple_get(tup, idx)
```

function tuple\_set[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-tuple_set "Copy/link to this API element")

```python
tuple_set(tup, idx, elm, init = 0)
```

function tuple\_add[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-tuple_add "Copy/link to this API element")

```python
tuple_add(tup, idx, inc, init = 0)
```

function tuple\_delete[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-tuple_delete "Copy/link to this API element")

```python
tuple_delete(tup, idx, init = 0)
```

function tuple\_insert[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-tuple_insert "Copy/link to this API element")

```python
tuple_insert(tup, idx, elm, init = 0)
```

function tuple\_swap[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-tuple_swap "Copy/link to this API element")

```python
tuple_swap(tup, idx1, idx2, init = 0)
```

function tuple\_perm[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-tuple_perm "Copy/link to this API element")

```python
tuple_perm(tup, perm)
```

function tuple\_perm\_inv[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-tuple_perm_inv "Copy/link to this API element")

```python
tuple_perm_inv(perm)
```

function tuple\_apply[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-tuple_apply "Copy/link to this API element")

```python
tuple_apply(fn, tup1, tup2)
```

function tuple\_insert\_tuple[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-tuple_insert_tuple "Copy/link to this API element")

```python
tuple_insert_tuple(tup, idx, ins, init = 0)
```

function count[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-count "Copy/link to this API element")

```python
count(shape)
```

function tuple\_range[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-tuple_range "Copy/link to this API element")

```python
tuple_range(n: Literal[int])
```

function tuple\_equal[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-tuple_equal "Copy/link to this API element")

```python
tuple_equal(t1, t2)
```

function broadcast[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-broadcast "Copy/link to this API element")

```python
broadcast(shape1, shape2)
```

function normalize\_index[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-normalize_index "Copy/link to this API element")

```python
normalize_index(idx: int, axis: int, n: int)
```

function normalize\_axis\_index[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-normalize_axis_index "Copy/link to this API element")

```python
normalize_axis_index(axis: int, ndim: int, prefix: str = "")
```

function has\_duplicate[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-has_duplicate "Copy/link to this API element")

```python
has_duplicate(t)
```

function normalize\_axis\_tuple[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-normalize_axis_tuple "Copy/link to this API element")

```python
normalize_axis_tuple(axis, ndim: int, argname: str = "", allow_duplicates: bool = False)
```

function reconstruct\_index[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-reconstruct_index "Copy/link to this API element")

```python
reconstruct_index(t1, t2, mask)
```

function index\_to\_coords[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-index_to_coords "Copy/link to this API element")

```python
index_to_coords(index: int, limits)
```

function index\_to\_fcoords[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-index_to_fcoords "Copy/link to this API element")

```python
index_to_fcoords(index: int, limits)
```

function coords\_to\_index[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-coords_to_index "Copy/link to this API element")

```python
coords_to_index(coords: S, limits: S, S: type)
```

function coords\_to\_findex[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-coords_to_findex "Copy/link to this API element")

```python
coords_to_findex(coords: S, limits: S, S: type)
```

function sort[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-sort "Copy/link to this API element")

```python
sort(p: Ptr[T], n: int, key, T: type)
```

function sort\_by\_stride[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-sort_by_stride "Copy/link to this API element")

```python
sort_by_stride(shape, strides)
```

function median[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-median "Copy/link to this API element")

```python
median(arr, n: int)
```

function multirange[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-multirange "Copy/link to this API element")

```python
multirange(limits)
```

function fmultirange[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-fmultirange "Copy/link to this API element")

```python
fmultirange(limits)
```

function noop[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-noop "Copy/link to this API element")

@pure @llvm

```python
noop(x: T, D: type, T: type)
```

```llvm
ret {=D} %x
```

function zero[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-zero "Copy/link to this API element")

@pure @llvm

```python
zero(T: type)
```

```llvm
ret {=T} zeroinitializer
```

function inf64[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-inf64 "Copy/link to this API element")

@pure @llvm

```python
inf64()
```

```llvm
ret double 0x7FF0000000000000
```

function inf32[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-inf32 "Copy/link to this API element")

@pure @llvm

```python
inf32()
```

```llvm
ret float 0x7FF0000000000000
```

function inf16[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-inf16 "Copy/link to this API element")

@pure @llvm

```python
inf16()
```

```llvm
ret half 0x7FF0000000000000
```

function inf[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-inf "Copy/link to this API element")

```python
inf(T: type)
```

function nan64[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-nan64 "Copy/link to this API element")

@pure @llvm

```python
nan64()
```

```llvm
ret double 0x7FF8000000000000
```

function nan32[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-nan32 "Copy/link to this API element")

@pure @llvm

```python
nan32()
```

```llvm
ret float 0x7FF8000000000000
```

function nan16[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-nan16 "Copy/link to this API element")

@pure @llvm

```python
nan16()
```

```llvm
ret half 0x7FF8000000000000
```

function nan[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-nan "Copy/link to this API element")

```python
nan(T: type)
```

function minnum64[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-minnum64 "Copy/link to this API element")

@pure @llvm

```python
minnum64()
```

```llvm
ret double 0x10000000000000
```

function minnum32[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-minnum32 "Copy/link to this API element")

@pure @llvm

```python
minnum32()
```

```llvm
ret float 0x3810000000000000
```

function minnum16[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-minnum16 "Copy/link to this API element")

@pure @llvm

```python
minnum16()
```

```llvm
ret half 0xHFBFF
```

function minnum[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-minnum "Copy/link to this API element")

```python
minnum(T: type)
```

function maxnum64[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-maxnum64 "Copy/link to this API element")

@pure @llvm

```python
maxnum64()
```

```llvm
ret double 0x7FEFFFFFFFFFFFFF
```

function maxnum32[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-maxnum32 "Copy/link to this API element")

@pure @llvm

```python
maxnum32()
```

```llvm
ret float 0x47EFFFFFE0000000
```

function maxnum16[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-maxnum16 "Copy/link to this API element")

@pure @llvm

```python
maxnum16()
```

```llvm
ret half 0xH7BFF
```

function maxnum[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-maxnum "Copy/link to this API element")

```python
maxnum(T: type)
```

function eps64[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-eps64 "Copy/link to this API element")

@pure @llvm

```python
eps64()
```

```llvm
ret double 0x3CB0000000000000
```

function eps32[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-eps32 "Copy/link to this API element")

@pure @llvm

```python
eps32()
```

```llvm
ret float 0x3E80000000000000
```

function eps16[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-eps16 "Copy/link to this API element")

@pure @llvm

```python
eps16()
```

```llvm
ret half 0xH1400
```

function eps[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-eps "Copy/link to this API element")

```python
eps(T: type)
```

function mantdig64[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-mantdig64 "Copy/link to this API element")

```python
mantdig64()
```

function mantdig32[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-mantdig32 "Copy/link to this API element")

```python
mantdig32()
```

function mantdig16[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-mantdig16 "Copy/link to this API element")

```python
mantdig16()
```

function mantdig[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-mantdig "Copy/link to this API element")

```python
mantdig(T: type)
```

function maxexp64[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-maxexp64 "Copy/link to this API element")

```python
maxexp64()
```

function maxexp32[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-maxexp32 "Copy/link to this API element")

```python
maxexp32()
```

function maxexp16[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-maxexp16 "Copy/link to this API element")

```python
maxexp16()
```

function maxexp[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-maxexp "Copy/link to this API element")

```python
maxexp(T: type)
```

function bitcast[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-bitcast "Copy/link to this API element")

@pure @llvm

```python
bitcast(x: T, D: type, T: type)
```

```llvm
%y = bitcast {=T} %x to {=D}
ret {=D} %y
```

function uitofp[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-uitofp "Copy/link to this API element")

@pure @llvm

```python
uitofp(x: T, D: type, T: type)
```

```llvm
%y = uitofp {=T} %x to {=D}
ret {=D} %y
```

function sitofp[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-sitofp "Copy/link to this API element")

@pure @llvm

```python
sitofp(x: T, D: type, T: type)
```

```llvm
%y = sitofp {=T} %x to {=D}
ret {=D} %y
```

function fptoui[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-fptoui "Copy/link to this API element")

@pure @llvm

```python
fptoui(x: T, D: type, T: type)
```

```llvm
%y = fptoui {=T} %x to {=D}
ret {=D} %y
```

function fptosi[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-fptosi "Copy/link to this API element")

@pure @llvm

```python
fptosi(x: T, D: type, T: type)
```

```llvm
%y = fptosi {=T} %x to {=D}
ret {=D} %y
```

function zext[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-zext "Copy/link to this API element")

@pure @llvm

```python
zext(x: T, D: type, T: type)
```

```llvm
%y = zext {=T} %x to {=D}
ret {=D} %y
```

function sext[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-sext "Copy/link to this API element")

@pure @llvm

```python
sext(x: T, D: type, T: type)
```

```llvm
%y = sext {=T} %x to {=D}
ret {=D} %y
```

function itrunc[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-itrunc "Copy/link to this API element")

@pure @llvm

```python
itrunc(x: T, D: type, T: type)
```

```llvm
%y = trunc {=T} %x to {=D}
ret {=D} %y
```

function fpext[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-fpext "Copy/link to this API element")

@pure @llvm

```python
fpext(x: T, D: type, T: type)
```

```llvm
%y = fpext {=T} %x to {=D}
ret {=D} %y
```

function fptrunc[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-fptrunc "Copy/link to this API element")

@pure @llvm

```python
fptrunc(x: T, D: type, T: type)
```

```llvm
%y = fptrunc {=T} %x to {=D}
ret {=D} %y
```

function fmin64[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-fmin64 "Copy/link to this API element")

@pure @llvm

```python
fmin64(x: float, y: float)
```

```llvm
declare double @llvm.minimum.f64(double, double)
%z = call double @llvm.minimum.f64(double %x, double %y)
ret double %z
```

function fmin32[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-fmin32 "Copy/link to this API element")

@pure @llvm

```python
fmin32(x: float32, y: float32)
```

```llvm
declare float @llvm.minimum.f32(float, float)
%z = call float @llvm.minimum.f32(float %x, float %y)
ret float %z
```

function fmin16[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-fmin16 "Copy/link to this API element")

@pure @llvm

```python
fmin16(x: float16, y: float16)
```

```llvm
declare half @llvm.minimum.f16(half, half)
%z = call half @llvm.minimum.f16(half %x, half %y)
ret half %z
```

function fmin[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-fmin "Copy/link to this API element")

```python
fmin(x, y)
```

function fmax64[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-fmax64 "Copy/link to this API element")

@pure @llvm

```python
fmax64(x: float, y: float)
```

```llvm
declare double @llvm.maximum.f64(double, double)
%z = call double @llvm.maximum.f64(double %x, double %y)
ret double %z
```

function fmax32[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-fmax32 "Copy/link to this API element")

@pure @llvm

```python
fmax32(x: float32, y: float32)
```

```llvm
declare float @llvm.maximum.f32(float, float)
%z = call float @llvm.maximum.f32(float %x, float %y)
ret float %z
```

function fmax16[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-fmax16 "Copy/link to this API element")

@pure @llvm

```python
fmax16(x: float16, y: float16)
```

```llvm
declare half @llvm.maximum.f16(half, half)
%z = call half @llvm.maximum.f16(half %x, half %y)
ret half %z
```

function fmax[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-fmax "Copy/link to this API element")

```python
fmax(x, y)
```

function fminnum64[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-fminnum64 "Copy/link to this API element")

@pure @llvm

```python
fminnum64(x: float, y: float)
```

```llvm
declare double @llvm.minimumnum.f64(double, double)
%z = call double @llvm.minimumnum.f64(double %x, double %y)
ret double %z
```

function fminnum32[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-fminnum32 "Copy/link to this API element")

@pure @llvm

```python
fminnum32(x: float32, y: float32)
```

```llvm
declare float @llvm.minimumnum.f32(float, float)
%z = call float @llvm.minimumnum.f32(float %x, float %y)
ret float %z
```

function fminnum16[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-fminnum16 "Copy/link to this API element")

@pure @llvm

```python
fminnum16(x: float16, y: float16)
```

```llvm
declare half @llvm.minimumnum.f16(half, half)
%z = call half @llvm.minimumnum.f16(half %x, half %y)
ret half %z
```

function fminnum[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-fminnum "Copy/link to this API element")

```python
fminnum(x, y)
```

function fmaxnum64[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-fmaxnum64 "Copy/link to this API element")

@pure @llvm

```python
fmaxnum64(x: float, y: float)
```

```llvm
declare double @llvm.maximumnum.f64(double, double)
%z = call double @llvm.maximumnum.f64(double %x, double %y)
ret double %z
```

function fmaxnum32[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-fmaxnum32 "Copy/link to this API element")

@pure @llvm

```python
fmaxnum32(x: float32, y: float32)
```

```llvm
declare float @llvm.maximumnum.f32(float, float)
%z = call float @llvm.maximumnum.f32(float %x, float %y)
ret float %z
```

function fmaxnum16[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-fmaxnum16 "Copy/link to this API element")

@pure @llvm

```python
fmaxnum16(x: float16, y: float16)
```

```llvm
declare half @llvm.maximumnum.f16(half, half)
%z = call half @llvm.maximumnum.f16(half %x, half %y)
ret half %z
```

function fmaxnum[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-fmaxnum "Copy/link to this API element")

```python
fmaxnum(x, y)
```

function bswap[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-bswap "Copy/link to this API element")

@pure @llvm

```python
bswap(x: T, T: type)
```

```llvm
declare {=T} @llvm.bswap.{=T}({=T})
%y = call {=T} @llvm.bswap.{=T}({=T} %x)
ret {=T} %y
```

function isinf64[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-isinf64 "Copy/link to this API element")

@pure @llvm

```python
isinf64(x: float)
```

```llvm
declare double @llvm.fabs.f64(double)
%a = call double @llvm.fabs.f64(double %x)
%y = fcmp oeq double %a, 0x7FF0000000000000
%z = zext i1 %y to i8
ret i8 %z
```

function isinf32[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-isinf32 "Copy/link to this API element")

@pure @llvm

```python
isinf32(x: float32)
```

```llvm
declare float @llvm.fabs.f32(float)
%a = call float @llvm.fabs.f32(float %x)
%y = fcmp oeq float %a, 0x7FF0000000000000
%z = zext i1 %y to i8
ret i8 %z
```

function isinf16[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-isinf16 "Copy/link to this API element")

@pure @llvm

```python
isinf16(x: float16)
```

```llvm
declare half @llvm.fabs.f16(half)
%a = call half @llvm.fabs.f16(half %x)
%y = fcmp oeq half %a, 0x7FF0000000000000
%z = zext i1 %y to i8
ret i8 %z
```

function isinf[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-isinf "Copy/link to this API element")

```python
isinf(x)
```

function isnan64[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-isnan64 "Copy/link to this API element")

@pure @llvm

```python
isnan64(x: float)
```

```llvm
%y = fcmp uno double %x, 0.000000e+00
%z = zext i1 %y to i8
ret i8 %z
```

function isnan32[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-isnan32 "Copy/link to this API element")

@pure @llvm

```python
isnan32(x: float32)
```

```llvm
%y = fcmp uno float %x, 0.000000e+00
%z = zext i1 %y to i8
ret i8 %z
```

function isnan16[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-isnan16 "Copy/link to this API element")

@pure @llvm

```python
isnan16(x: float16)
```

```llvm
%y = fcmp uno half %x, 0.000000e+00
%z = zext i1 %y to i8
ret i8 %z
```

function isnan[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-isnan "Copy/link to this API element")

```python
isnan(x)
```

function signbit64[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-signbit64 "Copy/link to this API element")

@pure @llvm

```python
signbit64(x: float)
```

```llvm
%n = bitcast double %x to i64
%s = icmp slt i64 %n, 0
%b = zext i1 %s to i8
ret i8 %b
```

function signbit32[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-signbit32 "Copy/link to this API element")

@pure @llvm

```python
signbit32(x: float32)
```

```llvm
%n = bitcast float %x to i32
%s = icmp slt i32 %n, 0
%b = zext i1 %s to i8
ret i8 %b
```

function signbit16[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-signbit16 "Copy/link to this API element")

@pure @llvm

```python
signbit16(x: float16)
```

```llvm
%n = bitcast half %x to i16
%s = icmp slt i16 %n, 0
%b = zext i1 %s to i8
ret i8 %b
```

function signbit[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-signbit "Copy/link to this API element")

```python
signbit(x)
```

function fabs64[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-fabs64 "Copy/link to this API element")

@pure @llvm

```python
fabs64(x: float)
```

```llvm
declare double @llvm.fabs.f64(double)
%y = call double @llvm.fabs.f64(double %x)
ret double %y
```

function fabs32[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-fabs32 "Copy/link to this API element")

@pure @llvm

```python
fabs32(x: float32)
```

```llvm
declare float @llvm.fabs.f32(float)
%y = call float @llvm.fabs.f32(float %x)
ret float %y
```

function fabs16[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-fabs16 "Copy/link to this API element")

@pure @llvm

```python
fabs16(x: float16)
```

```llvm
declare half @llvm.fabs.f16(half)
%y = call half @llvm.fabs.f16(half %x)
ret half %y
```

function fabs[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-fabs "Copy/link to this API element")

```python
fabs(x)
```

function isfinite64[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-isfinite64 "Copy/link to this API element")

```python
isfinite64(x: float)
```

function isfinite32[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-isfinite32 "Copy/link to this API element")

```python
isfinite32(x: float32)
```

function isfinite16[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-isfinite16 "Copy/link to this API element")

```python
isfinite16(x: float16)
```

function isfinite[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-isfinite "Copy/link to this API element")

```python
isfinite(x)
```

function rint64[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-rint64 "Copy/link to this API element")

@pure @llvm

```python
rint64(x: float)
```

```llvm
declare double @llvm.rint.f64(double)
%y = call double @llvm.rint.f64(double %x)
ret double %y
```

function rint32[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-rint32 "Copy/link to this API element")

@pure @llvm

```python
rint32(x: float32)
```

```llvm
declare float @llvm.rint.f32(float)
%y = call float @llvm.rint.f32(float %x)
ret float %y
```

function rint16[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-rint16 "Copy/link to this API element")

@pure @llvm

```python
rint16(x: float16)
```

```llvm
declare half @llvm.rint.f16(half)
%y = call half @llvm.rint.f16(half %x)
ret half %y
```

function rint[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-rint "Copy/link to this API element")

```python
rint(x)
```

function floor64[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-floor64 "Copy/link to this API element")

@pure @llvm

```python
floor64(x: float)
```

```llvm
declare double @llvm.floor.f64(double)
%y = call double @llvm.floor.f64(double %x)
ret double %y
```

function floor32[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-floor32 "Copy/link to this API element")

@pure @llvm

```python
floor32(x: float32)
```

```llvm
declare float @llvm.floor.f32(float)
%y = call float @llvm.floor.f32(float %x)
ret float %y
```

function floor16[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-floor16 "Copy/link to this API element")

@pure @llvm

```python
floor16(x: float16)
```

```llvm
declare half @llvm.floor.f16(half)
%y = call half @llvm.floor.f16(half %x)
ret half %y
```

function floor[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-floor "Copy/link to this API element")

```python
floor(x)
```

function ceil64[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-ceil64 "Copy/link to this API element")

@pure @llvm

```python
ceil64(x: float)
```

```llvm
declare double @llvm.ceil.f64(double)
%y = call double @llvm.ceil.f64(double %x)
ret double %y
```

function ceil32[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-ceil32 "Copy/link to this API element")

@pure @llvm

```python
ceil32(x: float32)
```

```llvm
declare float @llvm.ceil.f32(float)
%y = call float @llvm.ceil.f32(float %x)
ret float %y
```

function ceil16[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-ceil16 "Copy/link to this API element")

@pure @llvm

```python
ceil16(x: float16)
```

```llvm
declare half @llvm.ceil.f16(half)
%y = call half @llvm.ceil.f16(half %x)
ret half %y
```

function ceil[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-ceil "Copy/link to this API element")

```python
ceil(x)
```

function trunc64[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-trunc64 "Copy/link to this API element")

@pure @llvm

```python
trunc64(x: float)
```

```llvm
declare double @llvm.trunc.f64(double)
%y = call double @llvm.trunc.f64(double %x)
ret double %y
```

function trunc32[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-trunc32 "Copy/link to this API element")

@pure @llvm

```python
trunc32(x: float32)
```

```llvm
declare float @llvm.trunc.f32(float)
%y = call float @llvm.trunc.f32(float %x)
ret float %y
```

function trunc16[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-trunc16 "Copy/link to this API element")

@pure @llvm

```python
trunc16(x: float16)
```

```llvm
declare half @llvm.trunc.f16(half)
%y = call half @llvm.trunc.f16(half %x)
ret half %y
```

function trunc[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-trunc "Copy/link to this API element")

```python
trunc(x)
```

function exp64[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-exp64 "Copy/link to this API element")

@pure @llvm

```python
exp64(x: float)
```

```llvm
declare double @llvm.exp.f64(double)
%y = call double @llvm.exp.f64(double %x)
ret double %y
```

function exp32[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-exp32 "Copy/link to this API element")

@pure @llvm

```python
exp32(x: float32)
```

```llvm
declare float @llvm.exp.f32(float)
%y = call float @llvm.exp.f32(float %x)
ret float %y
```

function exp16[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-exp16 "Copy/link to this API element")

@pure @llvm

```python
exp16(x: float16)
```

```llvm
declare half @llvm.exp.f16(half)
%y = call half @llvm.exp.f16(half %x)
ret half %y
```

function exp[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-exp "Copy/link to this API element")

```python
exp(x)
```

function exp2\_64[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-exp2_64 "Copy/link to this API element")

@pure @llvm

```python
exp2_64(x: float)
```

```llvm
declare double @llvm.exp2.f64(double)
%y = call double @llvm.exp2.f64(double %x)
ret double %y
```

function exp2\_32[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-exp2_32 "Copy/link to this API element")

@pure @llvm

```python
exp2_32(x: float32)
```

```llvm
declare float @llvm.exp2.f32(float)
%y = call float @llvm.exp2.f32(float %x)
ret float %y
```

function exp2\_16[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-exp2_16 "Copy/link to this API element")

@pure @llvm

```python
exp2_16(x: float16)
```

```llvm
declare half @llvm.exp2.f16(half)
%y = call half @llvm.exp2.f16(half %x)
ret half %y
```

function exp2[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-exp2 "Copy/link to this API element")

```python
exp2(x)
```

function expm1\_64[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-expm1_64 "Copy/link to this API element")

```python
expm1_64(x: float)
```

function expm1\_32[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-expm1_32 "Copy/link to this API element")

```python
expm1_32(x: float32)
```

function expm1[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-expm1 "Copy/link to this API element")

```python
expm1(x)
```

function log64[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-log64 "Copy/link to this API element")

@pure @llvm

```python
log64(x: float)
```

```llvm
declare double @llvm.log.f64(double)
%y = call double @llvm.log.f64(double %x)
ret double %y
```

function log32[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-log32 "Copy/link to this API element")

@pure @llvm

```python
log32(x: float32)
```

```llvm
declare float @llvm.log.f32(float)
%y = call float @llvm.log.f32(float %x)
ret float %y
```

function log16[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-log16 "Copy/link to this API element")

@pure @llvm

```python
log16(x: float16)
```

```llvm
declare half @llvm.log.f16(half)
%y = call half @llvm.log.f16(half %x)
ret half %y
```

function log[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-log "Copy/link to this API element")

```python
log(x)
```

function log2\_64[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-log2_64 "Copy/link to this API element")

@pure @llvm

```python
log2_64(x: float)
```

```llvm
declare double @llvm.log2.f64(double)
%y = call double @llvm.log2.f64(double %x)
ret double %y
```

function log2\_32[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-log2_32 "Copy/link to this API element")

@pure @llvm

```python
log2_32(x: float32)
```

```llvm
declare float @llvm.log2.f32(float)
%y = call float @llvm.log2.f32(float %x)
ret float %y
```

function log2\_16[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-log2_16 "Copy/link to this API element")

@pure @llvm

```python
log2_16(x: float16)
```

```llvm
declare half @llvm.log2.f16(half)
%y = call half @llvm.log2.f16(half %x)
ret half %y
```

function log2[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-log2 "Copy/link to this API element")

```python
log2(x)
```

function log10\_64[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-log10_64 "Copy/link to this API element")

@pure @llvm

```python
log10_64(x: float)
```

```llvm
declare double @llvm.log10.f64(double)
%y = call double @llvm.log10.f64(double %x)
ret double %y
```

function log10\_32[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-log10_32 "Copy/link to this API element")

@pure @llvm

```python
log10_32(x: float32)
```

```llvm
declare float @llvm.log10.f32(float)
%y = call float @llvm.log10.f32(float %x)
ret float %y
```

function log10\_16[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-log10_16 "Copy/link to this API element")

@pure @llvm

```python
log10_16(x: float16)
```

```llvm
declare half @llvm.log10.f16(half)
%y = call half @llvm.log10.f16(half %x)
ret half %y
```

function log10[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-log10 "Copy/link to this API element")

```python
log10(x)
```

function log1p64[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-log1p64 "Copy/link to this API element")

```python
log1p64(x: float)
```

function log1p32[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-log1p32 "Copy/link to this API element")

```python
log1p32(x: float32)
```

function log1p[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-log1p "Copy/link to this API element")

```python
log1p(x)
```

function sqrt64[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-sqrt64 "Copy/link to this API element")

@pure @llvm

```python
sqrt64(x: float)
```

```llvm
declare double @llvm.sqrt.f64(double)
%y = call double @llvm.sqrt.f64(double %x)
ret double %y
```

function sqrt32[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-sqrt32 "Copy/link to this API element")

@pure @llvm

```python
sqrt32(x: float32)
```

```llvm
declare float @llvm.sqrt.f32(float)
%y = call float @llvm.sqrt.f32(float %x)
ret float %y
```

function sqrt16[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-sqrt16 "Copy/link to this API element")

@pure @llvm

```python
sqrt16(x: float16)
```

```llvm
declare half @llvm.sqrt.f16(half)
%y = call half @llvm.sqrt.f16(half %x)
ret half %y
```

function sqrt[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-sqrt "Copy/link to this API element")

```python
sqrt(x)
```

function cbrt64[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-cbrt64 "Copy/link to this API element")

```python
cbrt64(x: float)
```

function cbrt32[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-cbrt32 "Copy/link to this API element")

```python
cbrt32(x: float32)
```

function cbrt[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-cbrt "Copy/link to this API element")

```python
cbrt(x)
```

function sin64[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-sin64 "Copy/link to this API element")

@pure @llvm

```python
sin64(x: float)
```

```llvm
declare double @llvm.sin.f64(double)
%y = call double @llvm.sin.f64(double %x)
ret double %y
```

function sin32[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-sin32 "Copy/link to this API element")

@pure @llvm

```python
sin32(x: float32)
```

```llvm
declare float @llvm.sin.f32(float)
%y = call float @llvm.sin.f32(float %x)
ret float %y
```

function sin16[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-sin16 "Copy/link to this API element")

@pure @llvm

```python
sin16(x: float16)
```

```llvm
declare half @llvm.sin.f16(half)
%y = call half @llvm.sin.f16(half %x)
ret half %y
```

function sin[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-sin "Copy/link to this API element")

```python
sin(x)
```

function cos64[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-cos64 "Copy/link to this API element")

@pure @llvm

```python
cos64(x: float)
```

```llvm
declare double @llvm.cos.f64(double)
%y = call double @llvm.cos.f64(double %x)
ret double %y
```

function cos32[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-cos32 "Copy/link to this API element")

@pure @llvm

```python
cos32(x: float32)
```

```llvm
declare float @llvm.cos.f32(float)
%y = call float @llvm.cos.f32(float %x)
ret float %y
```

function cos16[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-cos16 "Copy/link to this API element")

@pure @llvm

```python
cos16(x: float16)
```

```llvm
declare half @llvm.cos.f16(half)
%y = call half @llvm.cos.f16(half %x)
ret half %y
```

function cos[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-cos "Copy/link to this API element")

```python
cos(x)
```

function tan64[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-tan64 "Copy/link to this API element")

```python
tan64(x: float)
```

function tan32[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-tan32 "Copy/link to this API element")

```python
tan32(x: float32)
```

function tan[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-tan "Copy/link to this API element")

```python
tan(x)
```

function asin64[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-asin64 "Copy/link to this API element")

```python
asin64(x: float)
```

function asin32[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-asin32 "Copy/link to this API element")

```python
asin32(x: float32)
```

function asin[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-asin "Copy/link to this API element")

```python
asin(x)
```

function acos64[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-acos64 "Copy/link to this API element")

```python
acos64(x: float)
```

function acos32[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-acos32 "Copy/link to this API element")

```python
acos32(x: float32)
```

function acos[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-acos "Copy/link to this API element")

```python
acos(x)
```

function atan64[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-atan64 "Copy/link to this API element")

```python
atan64(x: float)
```

function atan32[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-atan32 "Copy/link to this API element")

```python
atan32(x: float32)
```

function atan[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-atan "Copy/link to this API element")

```python
atan(x)
```

function sinh64[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-sinh64 "Copy/link to this API element")

```python
sinh64(x: float)
```

function sinh32[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-sinh32 "Copy/link to this API element")

```python
sinh32(x: float32)
```

function sinh[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-sinh "Copy/link to this API element")

```python
sinh(x)
```

function cosh64[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-cosh64 "Copy/link to this API element")

```python
cosh64(x: float)
```

function cosh32[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-cosh32 "Copy/link to this API element")

```python
cosh32(x: float32)
```

function cosh[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-cosh "Copy/link to this API element")

```python
cosh(x)
```

function tanh64[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-tanh64 "Copy/link to this API element")

```python
tanh64(x: float)
```

function tanh32[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-tanh32 "Copy/link to this API element")

```python
tanh32(x: float32)
```

function tanh[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-tanh "Copy/link to this API element")

```python
tanh(x)
```

function asinh64[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-asinh64 "Copy/link to this API element")

```python
asinh64(x: float)
```

function asinh32[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-asinh32 "Copy/link to this API element")

```python
asinh32(x: float32)
```

function asinh[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-asinh "Copy/link to this API element")

```python
asinh(x)
```

function acosh64[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-acosh64 "Copy/link to this API element")

```python
acosh64(x: float)
```

function acosh32[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-acosh32 "Copy/link to this API element")

```python
acosh32(x: float32)
```

function acosh[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-acosh "Copy/link to this API element")

```python
acosh(x)
```

function atanh64[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-atanh64 "Copy/link to this API element")

```python
atanh64(x: float)
```

function atanh32[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-atanh32 "Copy/link to this API element")

```python
atanh32(x: float32)
```

function atanh[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-atanh "Copy/link to this API element")

```python
atanh(x)
```

function atan2\_64[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-atan2_64 "Copy/link to this API element")

```python
atan2_64(x: float, y: float)
```

function atan2\_32[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-atan2_32 "Copy/link to this API element")

```python
atan2_32(x: float32, y: float32)
```

function atan2[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-atan2 "Copy/link to this API element")

```python
atan2(x, y)
```

function copysign64[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-copysign64 "Copy/link to this API element")

@pure @llvm

```python
copysign64(x: float, y: float)
```

```llvm
declare double @llvm.copysign.f64(double, double)
%z = call double @llvm.copysign.f64(double %x, double %y)
ret double %z
```

function copysign32[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-copysign32 "Copy/link to this API element")

@pure @llvm

```python
copysign32(x: float32, y: float32)
```

```llvm
declare float @llvm.copysign.f32(float, float)
%z = call float @llvm.copysign.f32(float %x, float %y)
ret float %z
```

function copysign16[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-copysign16 "Copy/link to this API element")

@pure @llvm

```python
copysign16(x: float16, y: float16)
```

```llvm
declare half @llvm.copysign.f16(half, half)
%z = call half @llvm.copysign.f16(half %x, half %y)
ret half %z
```

function copysign[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-copysign "Copy/link to this API element")

```python
copysign(x, y)
```

function pow64[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-pow64 "Copy/link to this API element")

@pure @llvm

```python
pow64(x: float, y: float)
```

```llvm
declare double @llvm.pow.f64(double, double)
%z = call double @llvm.pow.f64(double %x, double %y)
ret double %z
```

function pow32[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-pow32 "Copy/link to this API element")

@pure @llvm

```python
pow32(x: float32, y: float32)
```

```llvm
declare float @llvm.pow.f32(float, float)
%z = call float @llvm.pow.f32(float %x, float %y)
ret float %z
```

function pow16[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-pow16 "Copy/link to this API element")

@pure @llvm

```python
pow16(x: float16, y: float16)
```

```llvm
declare half @llvm.pow.f16(half, half)
%z = call half @llvm.pow.f16(half %x, half %y)
ret half %z
```

function pow[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-pow "Copy/link to this API element")

```python
pow(x, y)
```

function frexp64[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-frexp64 "Copy/link to this API element")

@pure @llvm

```python
frexp64(x: float)
```

```llvm
declare { double, i32 } @llvm.frexp.f64(double)
%y = call { double, i32 } @llvm.frexp.f64(double %x)
%y0 = extractvalue { double, i32 } %y, 0
%y1 = extractvalue { double, i32 } %y, 1
%z1 = sext i32 %y1 to i64
%r0 = insertvalue { double, i64 } undef, double %y0, 0
%r1 = insertvalue { double, i64 } %r0, i64 %z1, 1
ret { double, i64 } %r1
```

function frexp32[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-frexp32 "Copy/link to this API element")

@pure @llvm

```python
frexp32(x: float32)
```

```llvm
declare { float, i32 } @llvm.frexp.f32(float)
%y = call { float, i32 } @llvm.frexp.f32(float %x)
%y0 = extractvalue { float, i32 } %y, 0
%y1 = extractvalue { float, i32 } %y, 1
%z1 = sext i32 %y1 to i64
%r0 = insertvalue { float, i64 } undef, float %y0, 0
%r1 = insertvalue { float, i64 } %r0, i64 %z1, 1
ret { float, i64 } %r1
```

function frexp16[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-frexp16 "Copy/link to this API element")

@pure @llvm

```python
frexp16(x: float16)
```

```llvm
declare { half, i32 } @llvm.frexp.f16(half)
%y = call { half, i32 } @llvm.frexp.f16(half %x)
%y0 = extractvalue { half, i32 } %y, 0
%y1 = extractvalue { half, i32 } %y, 1
%z1 = sext i32 %y1 to i64
%r0 = insertvalue { half, i64 } undef, half %y0, 0
%r1 = insertvalue { half, i64 } %r0, i64 %z1, 1
ret { half, i64 } %r1
```

function frexp[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-frexp "Copy/link to this API element")

```python
frexp(x)
```

function ldexp64[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-ldexp64 "Copy/link to this API element")

@pure @llvm

```python
ldexp64(x: float, exp: int)
```

```llvm
declare double @llvm.ldexp.f64(double, i32)
%e = trunc i64 %exp to i32
%y = call double @llvm.ldexp.f64(double %x, i32 %e)
ret double %y
```

function ldexp32[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-ldexp32 "Copy/link to this API element")

@pure @llvm

```python
ldexp32(x: float32, exp: int)
```

```llvm
declare float @llvm.ldexp.f32(float, i32)
%e = trunc i64 %exp to i32
%y = call float @llvm.ldexp.f32(float %x, i32 %e)
ret float %y
```

function ldexp16[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-ldexp16 "Copy/link to this API element")

```python
ldexp16(x: float16, exp: int)
```

function ldexp[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-ldexp "Copy/link to this API element")

```python
ldexp(x, exp: int)
```

variable LOGE2[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#variable-loge2 "Copy/link to this API element")

```python
LOGE2 = 0.693147180559945309417232121458176568
```

variable LOGE10[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#variable-loge10 "Copy/link to this API element")

```python
LOGE10 = 2.302585092994045684017991454684364208
```

variable LOG2E[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#variable-log2e "Copy/link to this API element")

```python
LOG2E = 1.442695040888963407359924681001892137
```

variable LOG10E[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#variable-log10e "Copy/link to this API element")

```python
LOG10E = 0.434294481903251827651128918916605082
```

variable SQRT2[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#variable-sqrt2 "Copy/link to this API element")

```python
SQRT2 = 1.414213562373095048801688724209698079
```

variable PI[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#variable-pi "Copy/link to this API element")

```python
PI = 3.141592653589793238462643383279502884
```

variable PI\_2[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#variable-pi_2 "Copy/link to this API element")

```python
PI_2 = 1.570796326794896619231321691639751442
```

variable E[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#variable-e "Copy/link to this API element")

```python
E = 2.718281828459045235360287471352662498
```

function logaddexp64[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-logaddexp64 "Copy/link to this API element")

```python
logaddexp64(x: float, y: float)
```

function logaddexp32[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-logaddexp32 "Copy/link to this API element")

```python
logaddexp32(x: float32, y: float32)
```

function logaddexp[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-logaddexp "Copy/link to this API element")

```python
logaddexp(x, y)
```

function logaddexp2\_64[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-logaddexp2_64 "Copy/link to this API element")

```python
logaddexp2_64(x: float, y: float)
```

function logaddexp2\_32[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-logaddexp2_32 "Copy/link to this API element")

```python
logaddexp2_32(x: float32, y: float32)
```

function logaddexp2[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-logaddexp2 "Copy/link to this API element")

```python
logaddexp2(x, y)
```

function hypot64[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-hypot64 "Copy/link to this API element")

```python
hypot64(x: float, y: float)
```

function hypot32[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-hypot32 "Copy/link to this API element")

```python
hypot32(x: float32, y: float32)
```

function hypot[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-hypot "Copy/link to this API element")

```python
hypot(x, y)
```

function modf64[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-modf64 "Copy/link to this API element")

```python
modf64(x: float)
```

function modf32[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-modf32 "Copy/link to this API element")

```python
modf32(x: float32)
```

function modf16[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-modf16 "Copy/link to this API element")

```python
modf16(x: float16)
```

function modf[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-modf "Copy/link to this API element")

```python
modf(x)
```

function nextafter64[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-nextafter64 "Copy/link to this API element")

```python
nextafter64(x: float, y: float)
```

function nextafter32[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-nextafter32 "Copy/link to this API element")

```python
nextafter32(x: float32, y: float32)
```

function nextafter16[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-nextafter16 "Copy/link to this API element")

```python
nextafter16(x: float16, y: float16)
```

function nextafter[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-nextafter "Copy/link to this API element")

```python
nextafter(x, y)
```

function cdiv\_int[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-cdiv_int "Copy/link to this API element")

@pure @llvm

```python
cdiv_int(x: Int[N], y: Int[N], N: Literal[int])
```

```llvm
%z = sdiv i{=N} %x, %y
ret i{=N} %z
```

@overload

```python
cdiv_int(x: int, y: int)
```

```llvm
%z = sdiv i64 %x, %y
ret i64 %z
```

function cmod\_int[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-cmod_int "Copy/link to this API element")

@pure @llvm

```python
cmod_int(x: Int[N], y: Int[N], N: Literal[int])
```

```llvm
%z = srem i{=N} %x, %y
ret i{=N} %z
```

@overload

```python
cmod_int(x: int, y: int)
```

```llvm
%z = srem i64 %x, %y
ret i64 %z
```

function pydiv[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-pydiv "Copy/link to this API element")

```python
pydiv(x: Int[N], y: Int[N], N: Literal[int])
```

@overload

```python
pydiv(x: int, y: int)
```

function pymod[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-pymod "Copy/link to this API element")

```python
pymod(x: Int[N], y: Int[N], N: Literal[int])
```

@overload

```python
pymod(x: int, y: int)
```

function cdiv64[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-cdiv64 "Copy/link to this API element")

@pure @llvm

```python
cdiv64(x: float, y: float)
```

```llvm
%z = fdiv double %x, %y
ret double %z
```

function cdiv32[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-cdiv32 "Copy/link to this API element")

@pure @llvm

```python
cdiv32(x: float32, y: float32)
```

```llvm
%z = fdiv float %x, %y
ret float %z
```

function cdiv16[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-cdiv16 "Copy/link to this API element")

@pure @llvm

```python
cdiv16(x: float16, y: float16)
```

```llvm
%z = fdiv half %x, %y
ret half %z
```

function cdiv[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-cdiv "Copy/link to this API element")

```python
cdiv(x, y)
```

function cmod64[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-cmod64 "Copy/link to this API element")

@pure @llvm

```python
cmod64(x: float, y: float)
```

```llvm
%z = frem double %x, %y
ret double %z
```

function cmod32[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-cmod32 "Copy/link to this API element")

@pure @llvm

```python
cmod32(x: float32, y: float32)
```

```llvm
%z = frem float %x, %y
ret float %z
```

function cmod16[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-cmod16 "Copy/link to this API element")

@pure @llvm

```python
cmod16(x: float16, y: float16)
```

```llvm
%z = frem half %x, %y
ret half %z
```

function cmod[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-cmod "Copy/link to this API element")

```python
cmod(x, y)
```

function pyfmod[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-pyfmod "Copy/link to this API element")

```python
pyfmod(x, y)
```

function i0\_64[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-i0_64 "Copy/link to this API element")

```python
i0_64(x: float)
```

function i0\_32[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-i0_32 "Copy/link to this API element")

```python
i0_32(x: float32)
```

function i0[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-i0 "Copy/link to this API element")

```python
i0(x)
```

function call\_vectorized\_loop[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-call_vectorized_loop "Copy/link to this API element")

```python
call_vectorized_loop(in1: Ptr[T], is1: int, in2: Ptr[T], is2: int, out: Ptr, os: int, n: int, func: Literal[str], T: type)
```

function isint[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-isint "Copy/link to this API element")

```python
isint(X: type)
```

function isfloat[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-isfloat "Copy/link to this API element")

```python
isfloat(X: type)
```

function iscomplex[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-iscomplex "Copy/link to this API element")

```python
iscomplex(X: type)
```

function isinexact[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-isinexact "Copy/link to this API element")

```python
isinexact(X: type)
```

function sizeof[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-sizeof "Copy/link to this API element")

```python
sizeof(X: type)
```

function atomic[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-atomic "Copy/link to this API element")

```python
atomic(X: type)
```

function free[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-free "Copy/link to this API element")

```python
free(p: Ptr[T], T: type)
```

function realloc[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-realloc "Copy/link to this API element")

```python
realloc(p: Ptr[T], newsize: int, oldsize: int, T: type)
```

function strides[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-strides "Copy/link to this API element")

```python
strides(shape, forder: bool, X: type)
```

function cast[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-cast "Copy/link to this API element")

```python
cast(x, T: type)
```

function coerce[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-coerce "Copy/link to this API element")

```python
coerce(T1: type, T2: type)
```

function op\_types[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-op_types "Copy/link to this API element")

```python
op_types(T1: type, T2: type)
```

function to\_float[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-to_float "Copy/link to this API element")

```python
to_float(x)
```

function str\_to\_dtype[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-str_to_dtype "Copy/link to this API element")

```python
str_to_dtype(s: Literal[str])
```

function dtype\_to\_str[](https://docs.exaloop.io/libraries/api/modules/numpy/util/#function-dtype_to_str "Copy/link to this API element")

```python
dtype_to_str(dtype: type, include_byteorder: bool = True)
```

Source: https://docs.exaloop.io/libraries/api/modules/numpy/util/
