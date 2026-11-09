# ndarray

Standard library class `numpy.ndarray.ndarray` [View source](https://github.com/exaloop/codon/blob/master/stdlib/numpy/ndarray.codon)

class ndarray[](https://docs.exaloop.io/libraries/api/classes/numpy/ndarray/ndarray/#class-ndarray "Copy/link to this API element")

@tuple

```python
class ndarray[dtype, ndim]
```

Fields 2

`dtype` `type`

`ndim` `Literal[int]`

Constructors 2

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/ndarray/ndarray/#class-ndarray-__new__ "Copy/link to this API element")

```python
__new__(shape: Tuple[ndim, int], strides: Tuple[ndim, int], data: Ptr[dtype])
```

Defined in [`numpy.ndarray`](https://docs.exaloop.io/libraries/api/modules/numpy/ndarray/)

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/ndarray/ndarray/#class-ndarray-__new__-overload-2 "Copy/link to this API element")

```python
__new__(shape: Tuple[ndim, int], data: Ptr[dtype], fcontig: bool = False)
```

Defined in [`numpy.ndarray`](https://docs.exaloop.io/libraries/api/modules/numpy/ndarray/)

Properties 11

shape[](https://docs.exaloop.io/libraries/api/classes/numpy/ndarray/ndarray/#class-ndarray-shape "Copy/link to this API element")

@property

```python
shape(self: Self, Self: type)
```

Defined in [`numpy.ndarray`](https://docs.exaloop.io/libraries/api/modules/numpy/ndarray/)

strides[](https://docs.exaloop.io/libraries/api/classes/numpy/ndarray/ndarray/#class-ndarray-strides "Copy/link to this API element")

@property

```python
strides(self: Self, Self: type)
```

Defined in [`numpy.ndarray`](https://docs.exaloop.io/libraries/api/modules/numpy/ndarray/)

flags[](https://docs.exaloop.io/libraries/api/classes/numpy/ndarray/ndarray/#class-ndarray-flags "Copy/link to this API element")

@property

```python
flags(self: Self, Self: type)
```

Defined in [`numpy.ndarray`](https://docs.exaloop.io/libraries/api/modules/numpy/ndarray/)

data[](https://docs.exaloop.io/libraries/api/classes/numpy/ndarray/ndarray/#class-ndarray-data "Copy/link to this API element")

@property

```python
data(self: Self, Self: type)
```

Defined in [`numpy.ndarray`](https://docs.exaloop.io/libraries/api/modules/numpy/ndarray/)

size[](https://docs.exaloop.io/libraries/api/classes/numpy/ndarray/ndarray/#class-ndarray-size "Copy/link to this API element")

@property

```python
size(self: Self, Self: type)
```

Defined in [`numpy.ndarray`](https://docs.exaloop.io/libraries/api/modules/numpy/ndarray/)

itemsize[](https://docs.exaloop.io/libraries/api/classes/numpy/ndarray/ndarray/#class-ndarray-itemsize "Copy/link to this API element")

@property

```python
itemsize(self: Self, Self: type)
```

Defined in [`numpy.ndarray`](https://docs.exaloop.io/libraries/api/modules/numpy/ndarray/)

nbytes[](https://docs.exaloop.io/libraries/api/classes/numpy/ndarray/ndarray/#class-ndarray-nbytes "Copy/link to this API element")

@property

```python
nbytes(self: Self, Self: type)
```

Defined in [`numpy.ndarray`](https://docs.exaloop.io/libraries/api/modules/numpy/ndarray/)

T[](https://docs.exaloop.io/libraries/api/classes/numpy/ndarray/ndarray/#class-ndarray-t "Copy/link to this API element")

@property

```python
T(self: Self, Self: type)
```

Defined in [`numpy.ndarray`](https://docs.exaloop.io/libraries/api/modules/numpy/ndarray/)

flat[](https://docs.exaloop.io/libraries/api/classes/numpy/ndarray/ndarray/#class-ndarray-flat "Copy/link to this API element")

@property

```python
flat(self: Self, Self: type)
```

Defined in [`numpy.ndarray`](https://docs.exaloop.io/libraries/api/modules/numpy/ndarray/)

real[](https://docs.exaloop.io/libraries/api/classes/numpy/ndarray/ndarray/#class-ndarray-real "Copy/link to this API element")

@property

```python
real(self: Self, Self: type)
```

Added by [`numpy.routines`](https://docs.exaloop.io/libraries/api/modules/numpy/routines/)

imag[](https://docs.exaloop.io/libraries/api/classes/numpy/ndarray/ndarray/#class-ndarray-imag "Copy/link to this API element")

@property

```python
imag(self: Self, Self: type)
```

Added by [`numpy.routines`](https://docs.exaloop.io/libraries/api/modules/numpy/routines/)

Methods 54

item[](https://docs.exaloop.io/libraries/api/classes/numpy/ndarray/ndarray/#class-ndarray-item "Copy/link to this API element")

```python
item(self: Self, *args, Self: type)
```

Defined in [`numpy.ndarray`](https://docs.exaloop.io/libraries/api/modules/numpy/ndarray/)

transpose[](https://docs.exaloop.io/libraries/api/classes/numpy/ndarray/ndarray/#class-ndarray-transpose "Copy/link to this API element")

```python
transpose(self: Self, *axes, Self: type)
```

Defined in [`numpy.ndarray`](https://docs.exaloop.io/libraries/api/modules/numpy/ndarray/)

```python
transpose(self: Self, axes: List[int], Self: type)
```

Defined in [`numpy.ndarray`](https://docs.exaloop.io/libraries/api/modules/numpy/ndarray/)

swapaxes[](https://docs.exaloop.io/libraries/api/classes/numpy/ndarray/ndarray/#class-ndarray-swapaxes "Copy/link to this API element")

```python
swapaxes(self: Self, axis1: int, axis2: int, Self: type)
```

Defined in [`numpy.ndarray`](https://docs.exaloop.io/libraries/api/modules/numpy/ndarray/)

reshape[](https://docs.exaloop.io/libraries/api/classes/numpy/ndarray/ndarray/#class-ndarray-reshape "Copy/link to this API element")

```python
reshape(self: Self, *shape, order: str = "C", Self: type)
```

Defined in [`numpy.ndarray`](https://docs.exaloop.io/libraries/api/modules/numpy/ndarray/)

tobytes[](https://docs.exaloop.io/libraries/api/classes/numpy/ndarray/ndarray/#class-ndarray-tobytes "Copy/link to this API element")

```python
tobytes(self: Self, order: str = "C", Self: type)
```

Defined in [`numpy.ndarray`](https://docs.exaloop.io/libraries/api/modules/numpy/ndarray/)

ravel[](https://docs.exaloop.io/libraries/api/classes/numpy/ndarray/ndarray/#class-ndarray-ravel "Copy/link to this API element")

```python
ravel(self: Self, order: str = "C", Self: type)
```

Defined in [`numpy.ndarray`](https://docs.exaloop.io/libraries/api/modules/numpy/ndarray/)

flatten[](https://docs.exaloop.io/libraries/api/classes/numpy/ndarray/ndarray/#class-ndarray-flatten "Copy/link to this API element")

```python
flatten(self: Self, order: str = "C", Self: type)
```

Defined in [`numpy.ndarray`](https://docs.exaloop.io/libraries/api/modules/numpy/ndarray/)

flat[](https://docs.exaloop.io/libraries/api/classes/numpy/ndarray/ndarray/#class-ndarray-flat "Copy/link to this API element")

```python
flat(self: Self, value, Self: type)
```

Defined in [`numpy.ndarray`](https://docs.exaloop.io/libraries/api/modules/numpy/ndarray/)

```python
flat(self: Self, val, Self: type)
```

Added by [`numpy.routines`](https://docs.exaloop.io/libraries/api/modules/numpy/routines/)

tolist[](https://docs.exaloop.io/libraries/api/classes/numpy/ndarray/ndarray/#class-ndarray-tolist "Copy/link to this API element")

```python
tolist(self: Self, Self: type)
```

Defined in [`numpy.ndarray`](https://docs.exaloop.io/libraries/api/modules/numpy/ndarray/)

astype[](https://docs.exaloop.io/libraries/api/classes/numpy/ndarray/ndarray/#class-ndarray-astype "Copy/link to this API element")

```python
astype(self: Self, dtype: type, order: str = "K", copy: bool = True, Self: type)
```

Defined in [`numpy.ndarray`](https://docs.exaloop.io/libraries/api/modules/numpy/ndarray/)

copy[](https://docs.exaloop.io/libraries/api/classes/numpy/ndarray/ndarray/#class-ndarray-copy "Copy/link to this API element")

```python
copy(self: Self, order: str = "C", Self: type)
```

Defined in [`numpy.ndarray`](https://docs.exaloop.io/libraries/api/modules/numpy/ndarray/)

map[](https://docs.exaloop.io/libraries/api/classes/numpy/ndarray/ndarray/#class-ndarray-map "Copy/link to this API element")

```python
map(self: Self, fn, inplace: Literal[bool] = False, Self: type)
```

Defined in [`numpy.ndarray`](https://docs.exaloop.io/libraries/api/modules/numpy/ndarray/)

fill[](https://docs.exaloop.io/libraries/api/classes/numpy/ndarray/ndarray/#class-ndarray-fill "Copy/link to this API element")

```python
fill(self: Self, value, Self: type)
```

Defined in [`numpy.ndarray`](https://docs.exaloop.io/libraries/api/modules/numpy/ndarray/)

view[](https://docs.exaloop.io/libraries/api/classes/numpy/ndarray/ndarray/#class-ndarray-view "Copy/link to this API element")

```python
view(self: Self, dtype: type, Self: type)
```

Defined in [`numpy.ndarray`](https://docs.exaloop.io/libraries/api/modules/numpy/ndarray/)

byteswap[](https://docs.exaloop.io/libraries/api/classes/numpy/ndarray/ndarray/#class-ndarray-byteswap "Copy/link to this API element")

```python
byteswap(self: Self, inplace: bool = False, Self: type)
```

Defined in [`numpy.ndarray`](https://docs.exaloop.io/libraries/api/modules/numpy/ndarray/)

sort[](https://docs.exaloop.io/libraries/api/classes/numpy/ndarray/ndarray/#class-ndarray-sort "Copy/link to this API element")

```python
sort(self: Self, axis: int = -1, kind: Optional[str] = None, Self: type)
```

Added by [`numpy.sorting`](https://docs.exaloop.io/libraries/api/modules/numpy/sorting/)

argsort[](https://docs.exaloop.io/libraries/api/classes/numpy/ndarray/ndarray/#class-ndarray-argsort "Copy/link to this API element")

```python
argsort(self: Self, axis: int = -1, kind: Optional[str] = None, Self: type)
```

Added by [`numpy.sorting`](https://docs.exaloop.io/libraries/api/modules/numpy/sorting/)

partition[](https://docs.exaloop.io/libraries/api/classes/numpy/ndarray/ndarray/#class-ndarray-partition "Copy/link to this API element")

```python
partition(self: Self, kth, axis: int = -1, kind: str = "introselect", Self: type)
```

Added by [`numpy.sorting`](https://docs.exaloop.io/libraries/api/modules/numpy/sorting/)

argpartition[](https://docs.exaloop.io/libraries/api/classes/numpy/ndarray/ndarray/#class-ndarray-argpartition "Copy/link to this API element")

```python
argpartition(self: Self, kth, axis: int = -1, kind: str = "introselect", Self: type)
```

Added by [`numpy.sorting`](https://docs.exaloop.io/libraries/api/modules/numpy/sorting/)

take[](https://docs.exaloop.io/libraries/api/classes/numpy/ndarray/ndarray/#class-ndarray-take "Copy/link to this API element")

```python
take(self: Self, indices, axis = None, out = None, mode: str = "raise", Self: type)
```

Added by [`numpy.routines`](https://docs.exaloop.io/libraries/api/modules/numpy/routines/)

squeeze[](https://docs.exaloop.io/libraries/api/classes/numpy/ndarray/ndarray/#class-ndarray-squeeze "Copy/link to this API element")

```python
squeeze(self: Self, axis = None, Self: type)
```

Added by [`numpy.routines`](https://docs.exaloop.io/libraries/api/modules/numpy/routines/)

nonzero[](https://docs.exaloop.io/libraries/api/classes/numpy/ndarray/ndarray/#class-ndarray-nonzero "Copy/link to this API element")

```python
nonzero(self: Self, Self: type)
```

Added by [`numpy.routines`](https://docs.exaloop.io/libraries/api/modules/numpy/routines/)

searchsorted[](https://docs.exaloop.io/libraries/api/classes/numpy/ndarray/ndarray/#class-ndarray-searchsorted "Copy/link to this API element")

```python
searchsorted(self: Self, v, side: str = "left", sorter = None, Self: type)
```

Added by [`numpy.routines`](https://docs.exaloop.io/libraries/api/modules/numpy/routines/)

repeat[](https://docs.exaloop.io/libraries/api/classes/numpy/ndarray/ndarray/#class-ndarray-repeat "Copy/link to this API element")

```python
repeat(self: Self, repeats, axis = None, Self: type)
```

Added by [`numpy.routines`](https://docs.exaloop.io/libraries/api/modules/numpy/routines/)

compress[](https://docs.exaloop.io/libraries/api/classes/numpy/ndarray/ndarray/#class-ndarray-compress "Copy/link to this API element")

```python
compress(self: Self, condition, axis = None, out = None, Self: type)
```

Added by [`numpy.routines`](https://docs.exaloop.io/libraries/api/modules/numpy/routines/)

choose[](https://docs.exaloop.io/libraries/api/classes/numpy/ndarray/ndarray/#class-ndarray-choose "Copy/link to this API element")

```python
choose(self: Self, choices, out = None, mode: str = "raise", Self: type)
```

Added by [`numpy.routines`](https://docs.exaloop.io/libraries/api/modules/numpy/routines/)

diagonal[](https://docs.exaloop.io/libraries/api/classes/numpy/ndarray/ndarray/#class-ndarray-diagonal "Copy/link to this API element")

```python
diagonal(self: Self, offset: int = 0, axis1: int = 0, axis2: int = 1, Self: type)
```

Added by [`numpy.routines`](https://docs.exaloop.io/libraries/api/modules/numpy/routines/)

put[](https://docs.exaloop.io/libraries/api/classes/numpy/ndarray/ndarray/#class-ndarray-put "Copy/link to this API element")

```python
put(self: Self, ind, v, mode: str = "raise", Self: type)
```

Added by [`numpy.routines`](https://docs.exaloop.io/libraries/api/modules/numpy/routines/)

round[](https://docs.exaloop.io/libraries/api/classes/numpy/ndarray/ndarray/#class-ndarray-round "Copy/link to this API element")

```python
round(self: Self, decimals: int = 0, out = None, Self: type)
```

Added by [`numpy.routines`](https://docs.exaloop.io/libraries/api/modules/numpy/routines/)

clip[](https://docs.exaloop.io/libraries/api/classes/numpy/ndarray/ndarray/#class-ndarray-clip "Copy/link to this API element")

```python
clip(self: Self, min = None, max = None, out = None, Self: type)
```

Added by [`numpy.routines`](https://docs.exaloop.io/libraries/api/modules/numpy/routines/)

real[](https://docs.exaloop.io/libraries/api/classes/numpy/ndarray/ndarray/#class-ndarray-real "Copy/link to this API element")

```python
real(self: Self, val, Self: type)
```

Added by [`numpy.routines`](https://docs.exaloop.io/libraries/api/modules/numpy/routines/)

imag[](https://docs.exaloop.io/libraries/api/classes/numpy/ndarray/ndarray/#class-ndarray-imag "Copy/link to this API element")

```python
imag(self: Self, val, Self: type)
```

Added by [`numpy.routines`](https://docs.exaloop.io/libraries/api/modules/numpy/routines/)

conj[](https://docs.exaloop.io/libraries/api/classes/numpy/ndarray/ndarray/#class-ndarray-conj "Copy/link to this API element")

```python
conj(self: Self, Self: type)
```

Added by [`numpy.routines`](https://docs.exaloop.io/libraries/api/modules/numpy/routines/)

conjugate[](https://docs.exaloop.io/libraries/api/classes/numpy/ndarray/ndarray/#class-ndarray-conjugate "Copy/link to this API element")

```python
conjugate(self: Self, Self: type)
```

Added by [`numpy.routines`](https://docs.exaloop.io/libraries/api/modules/numpy/routines/)

tofile[](https://docs.exaloop.io/libraries/api/classes/numpy/ndarray/ndarray/#class-ndarray-tofile "Copy/link to this API element")

```python
tofile(self: Self, file, sep: str = "", Self: type)
```

Added by [`numpy.npio`](https://docs.exaloop.io/libraries/api/modules/numpy/npio/)

dot[](https://docs.exaloop.io/libraries/api/classes/numpy/ndarray/ndarray/#class-ndarray-dot "Copy/link to this API element")

```python
dot(self: Self, other, Self: type)
```

Added by [`numpy.linalg.linalg`](https://docs.exaloop.io/libraries/api/modules/numpy/linalg/linalg/)

trace[](https://docs.exaloop.io/libraries/api/classes/numpy/ndarray/ndarray/#class-ndarray-trace "Copy/link to this API element")

```python
trace(self: Self, offset: int = 0, axis1: int = 0, axis2: int = 1, dtype: type = NoneType, out = None, Self: type)
```

Added by [`numpy.linalg.linalg`](https://docs.exaloop.io/libraries/api/modules/numpy/linalg/linalg/)

sum[](https://docs.exaloop.io/libraries/api/classes/numpy/ndarray/ndarray/#class-ndarray-sum "Copy/link to this API element")

```python
sum(self: Self, axis = None, dtype: type = NoneType, out = None, keepdims: Literal[bool] = False, initial = 0, where = util._NoValue(), Self: type)
```

Added by [`numpy.reductions`](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/)

prod[](https://docs.exaloop.io/libraries/api/classes/numpy/ndarray/ndarray/#class-ndarray-prod "Copy/link to this API element")

```python
prod(self: Self, axis = None, dtype: type = NoneType, out = None, keepdims: Literal[bool] = False, initial = 1, where = util._NoValue(), Self: type)
```

Added by [`numpy.reductions`](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/)

mean[](https://docs.exaloop.io/libraries/api/classes/numpy/ndarray/ndarray/#class-ndarray-mean "Copy/link to this API element")

```python
mean(self: Self, axis = None, dtype: type = NoneType, out = None, keepdims: Literal[bool] = False, where = util._NoValue(), Self: type)
```

Added by [`numpy.reductions`](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/)

nanmean[](https://docs.exaloop.io/libraries/api/classes/numpy/ndarray/ndarray/#class-ndarray-nanmean "Copy/link to this API element")

```python
nanmean(self: Self, axis = None, dtype: type = NoneType, out = None, keepdims: Literal[bool] = False, where = util._NoValue(), Self: type)
```

Added by [`numpy.reductions`](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/)

var[](https://docs.exaloop.io/libraries/api/classes/numpy/ndarray/ndarray/#class-ndarray-var "Copy/link to this API element")

```python
var(self: Self, axis = None, dtype: type = NoneType, out = None, ddof: int = 0, keepdims: Literal[bool] = False, where = util._NoValue(), Self: type)
```

Added by [`numpy.reductions`](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/)

nanvar[](https://docs.exaloop.io/libraries/api/classes/numpy/ndarray/ndarray/#class-ndarray-nanvar "Copy/link to this API element")

```python
nanvar(self: Self, axis = None, dtype: type = NoneType, out = None, ddof: int = 0, keepdims: Literal[bool] = False, where = util._NoValue(), Self: type)
```

Added by [`numpy.reductions`](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/)

std[](https://docs.exaloop.io/libraries/api/classes/numpy/ndarray/ndarray/#class-ndarray-std "Copy/link to this API element")

```python
std(self: Self, axis = None, dtype: type = NoneType, out = None, ddof: int = 0, keepdims: Literal[bool] = False, where = util._NoValue(), Self: type)
```

Added by [`numpy.reductions`](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/)

nanstd[](https://docs.exaloop.io/libraries/api/classes/numpy/ndarray/ndarray/#class-ndarray-nanstd "Copy/link to this API element")

```python
nanstd(self: Self, axis = None, dtype: type = NoneType, out = None, ddof: int = 0, keepdims: Literal[bool] = False, where = util._NoValue(), Self: type)
```

Added by [`numpy.reductions`](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/)

min[](https://docs.exaloop.io/libraries/api/classes/numpy/ndarray/ndarray/#class-ndarray-min "Copy/link to this API element")

```python
min(self: Self, axis = None, dtype: type = NoneType, out = None, ddof: int = 0, keepdims: Literal[bool] = False, initial = util._NoValue(), where = util._NoValue(), Self: type)
```

Added by [`numpy.reductions`](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/)

ptp[](https://docs.exaloop.io/libraries/api/classes/numpy/ndarray/ndarray/#class-ndarray-ptp "Copy/link to this API element")

```python
ptp(self: Self, axis = None, out = None, keepdims: Literal[bool] = False, Self: type)
```

Added by [`numpy.reductions`](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/)

max[](https://docs.exaloop.io/libraries/api/classes/numpy/ndarray/ndarray/#class-ndarray-max "Copy/link to this API element")

```python
max(self: Self, axis = None, dtype: type = NoneType, out = None, ddof: int = 0, keepdims: Literal[bool] = False, initial = util._NoValue(), where = util._NoValue(), Self: type)
```

Added by [`numpy.reductions`](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/)

argmin[](https://docs.exaloop.io/libraries/api/classes/numpy/ndarray/ndarray/#class-ndarray-argmin "Copy/link to this API element")

```python
argmin(self: Self, axis = None, out = None, keepdims: Literal[bool] = False, Self: type)
```

Added by [`numpy.reductions`](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/)

argmax[](https://docs.exaloop.io/libraries/api/classes/numpy/ndarray/ndarray/#class-ndarray-argmax "Copy/link to this API element")

```python
argmax(self: Self, axis = None, out = None, keepdims: Literal[bool] = False, Self: type)
```

Added by [`numpy.reductions`](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/)

any[](https://docs.exaloop.io/libraries/api/classes/numpy/ndarray/ndarray/#class-ndarray-any "Copy/link to this API element")

```python
any(self: Self, axis = None, out = None, keepdims: Literal[bool] = False, where = util._NoValue(), Self: type)
```

Added by [`numpy.reductions`](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/)

all[](https://docs.exaloop.io/libraries/api/classes/numpy/ndarray/ndarray/#class-ndarray-all "Copy/link to this API element")

```python
all(self: Self, axis = None, out = None, keepdims: Literal[bool] = False, where = util._NoValue(), Self: type)
```

Added by [`numpy.reductions`](https://docs.exaloop.io/libraries/api/modules/numpy/reductions/)

cumprod[](https://docs.exaloop.io/libraries/api/classes/numpy/ndarray/ndarray/#class-ndarray-cumprod "Copy/link to this API element")

```python
cumprod(self: Self, axis = None, dtype: type = NoneType, out = None, Self: type)
```

Added by [`numpy.ndmath`](https://docs.exaloop.io/libraries/api/modules/numpy/ndmath/)

cumsum[](https://docs.exaloop.io/libraries/api/classes/numpy/ndarray/ndarray/#class-ndarray-cumsum "Copy/link to this API element")

```python
cumsum(self: Self, axis = None, dtype: type = NoneType, out = None, Self: type)
```

Added by [`numpy.ndmath`](https://docs.exaloop.io/libraries/api/modules/numpy/ndmath/)

Special methods 65

\_\_len\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/ndarray/ndarray/#class-ndarray-__len__ "Copy/link to this API element")

```python
__len__(self: Self, Self: type)
```

Defined in [`numpy.ndarray`](https://docs.exaloop.io/libraries/api/modules/numpy/ndarray/)

\_\_iter\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/ndarray/ndarray/#class-ndarray-__iter__ "Copy/link to this API element")

```python
__iter__(self: Self, Self: type)
```

Defined in [`numpy.ndarray`](https://docs.exaloop.io/libraries/api/modules/numpy/ndarray/)

\_\_copy\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/ndarray/ndarray/#class-ndarray-__copy__ "Copy/link to this API element")

```python
__copy__(self: Self, Self: type)
```

Defined in [`numpy.ndarray`](https://docs.exaloop.io/libraries/api/modules/numpy/ndarray/)

\_\_int\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/ndarray/ndarray/#class-ndarray-__int__ "Copy/link to this API element")

```python
__int__(self: Self, Self: type)
```

Defined in [`numpy.ndarray`](https://docs.exaloop.io/libraries/api/modules/numpy/ndarray/)

\_\_float\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/ndarray/ndarray/#class-ndarray-__float__ "Copy/link to this API element")

```python
__float__(self: Self, Self: type)
```

Defined in [`numpy.ndarray`](https://docs.exaloop.io/libraries/api/modules/numpy/ndarray/)

\_\_complex\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/ndarray/ndarray/#class-ndarray-__complex__ "Copy/link to this API element")

```python
__complex__(self: Self, Self: type)
```

Defined in [`numpy.ndarray`](https://docs.exaloop.io/libraries/api/modules/numpy/ndarray/)

\_\_bool\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/ndarray/ndarray/#class-ndarray-__bool__ "Copy/link to this API element")

```python
__bool__(self: Self, Self: type)
```

Defined in [`numpy.ndarray`](https://docs.exaloop.io/libraries/api/modules/numpy/ndarray/)

\_\_add\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/ndarray/ndarray/#class-ndarray-__add__ "Copy/link to this API element")

@inline

```python
__add__(self: Self, other, Self: type)
```

Added by [`numpy.operators`](https://docs.exaloop.io/libraries/api/modules/numpy/operators/)

\_\_radd\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/ndarray/ndarray/#class-ndarray-__radd__ "Copy/link to this API element")

@inline

```python
__radd__(self: Self, other, Self: type)
```

Added by [`numpy.operators`](https://docs.exaloop.io/libraries/api/modules/numpy/operators/)

\_\_iadd\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/ndarray/ndarray/#class-ndarray-__iadd__ "Copy/link to this API element")

@inline

```python
__iadd__(self: Self, other, Self: type)
```

Added by [`numpy.operators`](https://docs.exaloop.io/libraries/api/modules/numpy/operators/)

\_\_sub\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/ndarray/ndarray/#class-ndarray-__sub__ "Copy/link to this API element")

@inline

```python
__sub__(self: Self, other, Self: type)
```

Added by [`numpy.operators`](https://docs.exaloop.io/libraries/api/modules/numpy/operators/)

\_\_rsub\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/ndarray/ndarray/#class-ndarray-__rsub__ "Copy/link to this API element")

@inline

```python
__rsub__(self: Self, other, Self: type)
```

Added by [`numpy.operators`](https://docs.exaloop.io/libraries/api/modules/numpy/operators/)

\_\_isub\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/ndarray/ndarray/#class-ndarray-__isub__ "Copy/link to this API element")

@inline

```python
__isub__(self: Self, other, Self: type)
```

Added by [`numpy.operators`](https://docs.exaloop.io/libraries/api/modules/numpy/operators/)

\_\_mul\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/ndarray/ndarray/#class-ndarray-__mul__ "Copy/link to this API element")

@inline

```python
__mul__(self: Self, other, Self: type)
```

Added by [`numpy.operators`](https://docs.exaloop.io/libraries/api/modules/numpy/operators/)

\_\_rmul\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/ndarray/ndarray/#class-ndarray-__rmul__ "Copy/link to this API element")

@inline

```python
__rmul__(self: Self, other, Self: type)
```

Added by [`numpy.operators`](https://docs.exaloop.io/libraries/api/modules/numpy/operators/)

\_\_imul\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/ndarray/ndarray/#class-ndarray-__imul__ "Copy/link to this API element")

@inline

```python
__imul__(self: Self, other, Self: type)
```

Added by [`numpy.operators`](https://docs.exaloop.io/libraries/api/modules/numpy/operators/)

\_\_mod\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/ndarray/ndarray/#class-ndarray-__mod__ "Copy/link to this API element")

@inline

```python
__mod__(self: Self, other, Self: type)
```

Added by [`numpy.operators`](https://docs.exaloop.io/libraries/api/modules/numpy/operators/)

\_\_rmod\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/ndarray/ndarray/#class-ndarray-__rmod__ "Copy/link to this API element")

@inline

```python
__rmod__(self: Self, other, Self: type)
```

Added by [`numpy.operators`](https://docs.exaloop.io/libraries/api/modules/numpy/operators/)

\_\_imod\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/ndarray/ndarray/#class-ndarray-__imod__ "Copy/link to this API element")

@inline

```python
__imod__(self: Self, other, Self: type)
```

Added by [`numpy.operators`](https://docs.exaloop.io/libraries/api/modules/numpy/operators/)

\_\_pow\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/ndarray/ndarray/#class-ndarray-__pow__ "Copy/link to this API element")

@inline

```python
__pow__(self: Self, other, Self: type)
```

Added by [`numpy.operators`](https://docs.exaloop.io/libraries/api/modules/numpy/operators/)

\_\_rpow\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/ndarray/ndarray/#class-ndarray-__rpow__ "Copy/link to this API element")

@inline

```python
__rpow__(self: Self, other, Self: type)
```

Added by [`numpy.operators`](https://docs.exaloop.io/libraries/api/modules/numpy/operators/)

\_\_ipow\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/ndarray/ndarray/#class-ndarray-__ipow__ "Copy/link to this API element")

@inline

```python
__ipow__(self: Self, other, Self: type)
```

Added by [`numpy.operators`](https://docs.exaloop.io/libraries/api/modules/numpy/operators/)

\_\_truediv\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/ndarray/ndarray/#class-ndarray-__truediv__ "Copy/link to this API element")

@inline

```python
__truediv__(self: Self, other, Self: type)
```

Added by [`numpy.operators`](https://docs.exaloop.io/libraries/api/modules/numpy/operators/)

\_\_rtruediv\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/ndarray/ndarray/#class-ndarray-__rtruediv__ "Copy/link to this API element")

@inline

```python
__rtruediv__(self: Self, other, Self: type)
```

Added by [`numpy.operators`](https://docs.exaloop.io/libraries/api/modules/numpy/operators/)

\_\_itruediv\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/ndarray/ndarray/#class-ndarray-__itruediv__ "Copy/link to this API element")

@inline

```python
__itruediv__(self: Self, other, Self: type)
```

Added by [`numpy.operators`](https://docs.exaloop.io/libraries/api/modules/numpy/operators/)

\_\_floordiv\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/ndarray/ndarray/#class-ndarray-__floordiv__ "Copy/link to this API element")

@inline

```python
__floordiv__(self: Self, other, Self: type)
```

Added by [`numpy.operators`](https://docs.exaloop.io/libraries/api/modules/numpy/operators/)

\_\_rfloordiv\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/ndarray/ndarray/#class-ndarray-__rfloordiv__ "Copy/link to this API element")

@inline

```python
__rfloordiv__(self: Self, other, Self: type)
```

Added by [`numpy.operators`](https://docs.exaloop.io/libraries/api/modules/numpy/operators/)

\_\_ifloordiv\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/ndarray/ndarray/#class-ndarray-__ifloordiv__ "Copy/link to this API element")

@inline

```python
__ifloordiv__(self: Self, other, Self: type)
```

Added by [`numpy.operators`](https://docs.exaloop.io/libraries/api/modules/numpy/operators/)

\_\_lshift\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/ndarray/ndarray/#class-ndarray-__lshift__ "Copy/link to this API element")

@inline

```python
__lshift__(self: Self, other, Self: type)
```

Added by [`numpy.operators`](https://docs.exaloop.io/libraries/api/modules/numpy/operators/)

\_\_rlshift\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/ndarray/ndarray/#class-ndarray-__rlshift__ "Copy/link to this API element")

@inline

```python
__rlshift__(self: Self, other, Self: type)
```

Added by [`numpy.operators`](https://docs.exaloop.io/libraries/api/modules/numpy/operators/)

\_\_ilshift\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/ndarray/ndarray/#class-ndarray-__ilshift__ "Copy/link to this API element")

@inline

```python
__ilshift__(self: Self, other, Self: type)
```

Added by [`numpy.operators`](https://docs.exaloop.io/libraries/api/modules/numpy/operators/)

\_\_rshift\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/ndarray/ndarray/#class-ndarray-__rshift__ "Copy/link to this API element")

@inline

```python
__rshift__(self: Self, other, Self: type)
```

Added by [`numpy.operators`](https://docs.exaloop.io/libraries/api/modules/numpy/operators/)

\_\_rrshift\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/ndarray/ndarray/#class-ndarray-__rrshift__ "Copy/link to this API element")

@inline

```python
__rrshift__(self: Self, other, Self: type)
```

Added by [`numpy.operators`](https://docs.exaloop.io/libraries/api/modules/numpy/operators/)

\_\_irshift\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/ndarray/ndarray/#class-ndarray-__irshift__ "Copy/link to this API element")

@inline

```python
__irshift__(self: Self, other, Self: type)
```

Added by [`numpy.operators`](https://docs.exaloop.io/libraries/api/modules/numpy/operators/)

\_\_and\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/ndarray/ndarray/#class-ndarray-__and__ "Copy/link to this API element")

@inline

```python
__and__(self: Self, other, Self: type)
```

Added by [`numpy.operators`](https://docs.exaloop.io/libraries/api/modules/numpy/operators/)

\_\_rand\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/ndarray/ndarray/#class-ndarray-__rand__ "Copy/link to this API element")

@inline

```python
__rand__(self: Self, other, Self: type)
```

Added by [`numpy.operators`](https://docs.exaloop.io/libraries/api/modules/numpy/operators/)

\_\_iand\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/ndarray/ndarray/#class-ndarray-__iand__ "Copy/link to this API element")

@inline

```python
__iand__(self: Self, other, Self: type)
```

Added by [`numpy.operators`](https://docs.exaloop.io/libraries/api/modules/numpy/operators/)

\_\_or\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/ndarray/ndarray/#class-ndarray-__or__ "Copy/link to this API element")

@inline

```python
__or__(self: Self, other, Self: type)
```

Added by [`numpy.operators`](https://docs.exaloop.io/libraries/api/modules/numpy/operators/)

\_\_ror\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/ndarray/ndarray/#class-ndarray-__ror__ "Copy/link to this API element")

@inline

```python
__ror__(self: Self, other, Self: type)
```

Added by [`numpy.operators`](https://docs.exaloop.io/libraries/api/modules/numpy/operators/)

\_\_ior\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/ndarray/ndarray/#class-ndarray-__ior__ "Copy/link to this API element")

@inline

```python
__ior__(self: Self, other, Self: type)
```

Added by [`numpy.operators`](https://docs.exaloop.io/libraries/api/modules/numpy/operators/)

\_\_xor\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/ndarray/ndarray/#class-ndarray-__xor__ "Copy/link to this API element")

@inline

```python
__xor__(self: Self, other, Self: type)
```

Added by [`numpy.operators`](https://docs.exaloop.io/libraries/api/modules/numpy/operators/)

\_\_rxor\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/ndarray/ndarray/#class-ndarray-__rxor__ "Copy/link to this API element")

@inline

```python
__rxor__(self: Self, other, Self: type)
```

Added by [`numpy.operators`](https://docs.exaloop.io/libraries/api/modules/numpy/operators/)

\_\_ixor\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/ndarray/ndarray/#class-ndarray-__ixor__ "Copy/link to this API element")

@inline

```python
__ixor__(self: Self, other, Self: type)
```

Added by [`numpy.operators`](https://docs.exaloop.io/libraries/api/modules/numpy/operators/)

\_\_pos\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/ndarray/ndarray/#class-ndarray-__pos__ "Copy/link to this API element")

@inline

```python
__pos__(self: Self, Self: type)
```

Added by [`numpy.operators`](https://docs.exaloop.io/libraries/api/modules/numpy/operators/)

\_\_neg\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/ndarray/ndarray/#class-ndarray-__neg__ "Copy/link to this API element")

@inline

```python
__neg__(self: Self, Self: type)
```

Added by [`numpy.operators`](https://docs.exaloop.io/libraries/api/modules/numpy/operators/)

\_\_invert\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/ndarray/ndarray/#class-ndarray-__invert__ "Copy/link to this API element")

@inline

```python
__invert__(self: Self, Self: type)
```

Added by [`numpy.operators`](https://docs.exaloop.io/libraries/api/modules/numpy/operators/)

\_\_eq\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/ndarray/ndarray/#class-ndarray-__eq__ "Copy/link to this API element")

@inline

```python
__eq__(self: Self, other, Self: type)
```

Added by [`numpy.operators`](https://docs.exaloop.io/libraries/api/modules/numpy/operators/)

\_\_ne\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/ndarray/ndarray/#class-ndarray-__ne__ "Copy/link to this API element")

@inline

```python
__ne__(self: Self, other, Self: type)
```

Added by [`numpy.operators`](https://docs.exaloop.io/libraries/api/modules/numpy/operators/)

\_\_lt\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/ndarray/ndarray/#class-ndarray-__lt__ "Copy/link to this API element")

@inline

```python
__lt__(self: Self, other, Self: type)
```

Added by [`numpy.operators`](https://docs.exaloop.io/libraries/api/modules/numpy/operators/)

\_\_le\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/ndarray/ndarray/#class-ndarray-__le__ "Copy/link to this API element")

@inline

```python
__le__(self: Self, other, Self: type)
```

Added by [`numpy.operators`](https://docs.exaloop.io/libraries/api/modules/numpy/operators/)

\_\_gt\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/ndarray/ndarray/#class-ndarray-__gt__ "Copy/link to this API element")

@inline

```python
__gt__(self: Self, other, Self: type)
```

Added by [`numpy.operators`](https://docs.exaloop.io/libraries/api/modules/numpy/operators/)

\_\_ge\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/ndarray/ndarray/#class-ndarray-__ge__ "Copy/link to this API element")

@inline

```python
__ge__(self: Self, other, Self: type)
```

Added by [`numpy.operators`](https://docs.exaloop.io/libraries/api/modules/numpy/operators/)

\_\_pickle\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/ndarray/ndarray/#class-ndarray-__pickle__ "Copy/link to this API element")

```python
__pickle__(self: Self, jar: Jar, Self: type)
```

Added by [`numpy.npio`](https://docs.exaloop.io/libraries/api/modules/numpy/npio/)

\_\_unpickle\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/ndarray/ndarray/#class-ndarray-__unpickle__ "Copy/link to this API element")

```python
__unpickle__(jar: Jar)
```

Added by [`numpy.npio`](https://docs.exaloop.io/libraries/api/modules/numpy/npio/)

\_\_repr\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/ndarray/ndarray/#class-ndarray-__repr__ "Copy/link to this API element")

```python
__repr__(self: Self, Self: type)
```

Added by [`numpy.format`](https://docs.exaloop.io/libraries/api/modules/numpy/format/)

\_\_str\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/ndarray/ndarray/#class-ndarray-__str__ "Copy/link to this API element")

```python
__str__(self: Self, Self: type)
```

Added by [`numpy.format`](https://docs.exaloop.io/libraries/api/modules/numpy/format/)

\_\_to\_gpu\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/ndarray/ndarray/#class-ndarray-__to_gpu__ "Copy/link to this API element")

```python
__to_gpu__(self: Self, cache: gpu.AllocCache, Self: type)
```

Added by [`numpy.ndgpu`](https://docs.exaloop.io/libraries/api/modules/numpy/ndgpu/)

\_\_from\_gpu\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/ndarray/ndarray/#class-ndarray-__from_gpu__ "Copy/link to this API element")

```python
__from_gpu__(self: Self, other: ndarray[dtype, ndim], Self: type)
```

Added by [`numpy.ndgpu`](https://docs.exaloop.io/libraries/api/modules/numpy/ndgpu/)

\_\_from\_gpu\_new\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/ndarray/ndarray/#class-ndarray-__from_gpu_new__ "Copy/link to this API element")

```python
__from_gpu_new__(other: ndarray[dtype, ndim])
```

Added by [`numpy.ndgpu`](https://docs.exaloop.io/libraries/api/modules/numpy/ndgpu/)

\_\_matmul\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/ndarray/ndarray/#class-ndarray-__matmul__ "Copy/link to this API element")

```python
__matmul__(self: Self, other, Self: type)
```

Added by [`numpy.linalg.linalg`](https://docs.exaloop.io/libraries/api/modules/numpy/linalg/linalg/)

\_\_getitem\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/ndarray/ndarray/#class-ndarray-__getitem__ "Copy/link to this API element")

```python
__getitem__(self: Self, indexes, Self: type)
```

Added by [`numpy.indexing`](https://docs.exaloop.io/libraries/api/modules/numpy/indexing/)

\_\_setitem\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/ndarray/ndarray/#class-ndarray-__setitem__ "Copy/link to this API element")

```python
__setitem__(self: Self, indexes, item, Self: type)
```

Added by [`numpy.indexing`](https://docs.exaloop.io/libraries/api/modules/numpy/indexing/)

\_\_to\_py\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/ndarray/ndarray/#class-ndarray-__to_py__ "Copy/link to this API element")

```python
__to_py__(self: Self, Self: type)
```

Added by [`numpy.pybridge`](https://docs.exaloop.io/libraries/api/modules/numpy/pybridge/)

\_\_from\_py\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/ndarray/ndarray/#class-ndarray-__from_py__ "Copy/link to this API element")

```python
__from_py__(a: cobj)
```

Added by [`numpy.pybridge`](https://docs.exaloop.io/libraries/api/modules/numpy/pybridge/)

\_\_abs\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/ndarray/ndarray/#class-ndarray-__abs__ "Copy/link to this API element")

@inline

```python
__abs__(self: Self, Self: type)
```

Added by [`numpy.ndmath`](https://docs.exaloop.io/libraries/api/modules/numpy/ndmath/)

Source: https://docs.exaloop.io/libraries/api/classes/numpy/ndarray/ndarray/
