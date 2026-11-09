# numpy.routines

Standard library module [View source](https://github.com/exaloop/codon/blob/master/stdlib/numpy/routines.codon)

function array[](https://docs.exaloop.io/libraries/api/modules/numpy/routines/#function-array "Copy/link to this API element")

```python
array(a, dtype: type = NoneType, copy: bool = True, order: str = "K", ndmin: Literal[int] = 0)
```

function asarray[](https://docs.exaloop.io/libraries/api/modules/numpy/routines/#function-asarray "Copy/link to this API element")

```python
asarray(a, dtype: type = NoneType, order: str = "K")
```

function asanyarray[](https://docs.exaloop.io/libraries/api/modules/numpy/routines/#function-asanyarray "Copy/link to this API element")

```python
asanyarray(a, dtype: type = NoneType, order: str = "K")
```

function asarray\_chkfinite[](https://docs.exaloop.io/libraries/api/modules/numpy/routines/#function-asarray_chkfinite "Copy/link to this API element")

```python
asarray_chkfinite(a, dtype: type = NoneType, order: str = "K")
```

function empty[](https://docs.exaloop.io/libraries/api/modules/numpy/routines/#function-empty "Copy/link to this API element")

```python
empty(shape, dtype: type = float, order: str = "C")
```

function empty\_like[](https://docs.exaloop.io/libraries/api/modules/numpy/routines/#function-empty_like "Copy/link to this API element")

```python
empty_like(prototype, dtype: type = NoneType, order: str = "K", shape = None)
```

function zeros[](https://docs.exaloop.io/libraries/api/modules/numpy/routines/#function-zeros "Copy/link to this API element")

```python
zeros(shape, dtype: type = float, order: str = "C")
```

function zeros\_like[](https://docs.exaloop.io/libraries/api/modules/numpy/routines/#function-zeros_like "Copy/link to this API element")

```python
zeros_like(prototype, dtype: type = NoneType, order: str = "K")
```

function ones[](https://docs.exaloop.io/libraries/api/modules/numpy/routines/#function-ones "Copy/link to this API element")

```python
ones(shape, dtype: type = float, order: str = "C")
```

function ones\_like[](https://docs.exaloop.io/libraries/api/modules/numpy/routines/#function-ones_like "Copy/link to this API element")

```python
ones_like(prototype, dtype: type = NoneType, order: str = "K")
```

function identity[](https://docs.exaloop.io/libraries/api/modules/numpy/routines/#function-identity "Copy/link to this API element")

```python
identity(n: int, dtype: type = float)
```

function eye[](https://docs.exaloop.io/libraries/api/modules/numpy/routines/#function-eye "Copy/link to this API element")

```python
eye(N: int, M: Optional[int] = None, k: int = 0, dtype: type = float, order: str = "C")
```

function diag[](https://docs.exaloop.io/libraries/api/modules/numpy/routines/#function-diag "Copy/link to this API element")

```python
diag(v, k: int = 0)
```

function diagflat[](https://docs.exaloop.io/libraries/api/modules/numpy/routines/#function-diagflat "Copy/link to this API element")

```python
diagflat(v, k: int = 0)
```

function tri[](https://docs.exaloop.io/libraries/api/modules/numpy/routines/#function-tri "Copy/link to this API element")

```python
tri(N: int, M: Optional[int] = None, k: int = 0, dtype: type = float)
```

function triu[](https://docs.exaloop.io/libraries/api/modules/numpy/routines/#function-triu "Copy/link to this API element")

```python
triu(x, k: int = 0)
```

function tril[](https://docs.exaloop.io/libraries/api/modules/numpy/routines/#function-tril "Copy/link to this API element")

```python
tril(x, k: int = 0)
```

function vander[](https://docs.exaloop.io/libraries/api/modules/numpy/routines/#function-vander "Copy/link to this API element")

```python
vander(x, N: Optional[int] = None, increasing: bool = False)
```

function arange[](https://docs.exaloop.io/libraries/api/modules/numpy/routines/#function-arange "Copy/link to this API element")

```python
arange(start: float, stop: float, step: float, dtype: type = float)
```

@overload

```python
arange(stop: float, step: float, dtype: type = float)
```

@overload

```python
arange(start: float, stop: float, dtype: type = float)
```

@overload

```python
arange(stop: float, dtype: type = float)
```

@overload

```python
arange(start: int, stop: int, step: int, dtype: type = int)
```

@overload

```python
arange(stop: int, step: int, dtype: type = int)
```

@overload

```python
arange(start: int, stop: int, dtype: type = int)
```

@overload

```python
arange(stop: int, dtype: type = int)
```

@overload

```python
arange(start: datetime64, stop, step = None, dtype: type = NoneType)
```

@overload

```python
arange(start: timedelta64, stop, step, dtype: type = NoneType)
```

@overload

```python
arange(start: timedelta64, stop, dtype: type = NoneType)
```

@overload

```python
arange(stop: timedelta64, dtype: type = NoneType)
```

@overload

```python
arange(start: str, stop, step = None, dtype: type = datetime64[("D", 1)])
```

function linspace[](https://docs.exaloop.io/libraries/api/modules/numpy/routines/#function-linspace "Copy/link to this API element")

```python
linspace(start: float, stop: float, num: int = 50, endpoint: bool = True, retstep: Literal[bool] = False, dtype: type = float)
```

```python
linspace(start: float, stop: float, num: int = 50, endpoint: bool = True, retstep: Literal[bool] = False, dtype: type = float)
```

function logspace[](https://docs.exaloop.io/libraries/api/modules/numpy/routines/#function-logspace "Copy/link to this API element")

```python
logspace(start: float, stop: float, num: int = 50, endpoint: bool = True, base: float = 10.0, retstep: Literal[bool] = False, dtype: type = float)
```

function geomspace[](https://docs.exaloop.io/libraries/api/modules/numpy/routines/#function-geomspace "Copy/link to this API element")

```python
geomspace(start: float, stop: float, num: int = 50, endpoint: bool = True, dtype: type = float)
```

function fromfunction[](https://docs.exaloop.io/libraries/api/modules/numpy/routines/#function-fromfunction "Copy/link to this API element")

```python
fromfunction(function, shape, dtype: type = float, **kwargs)
```

function fromiter[](https://docs.exaloop.io/libraries/api/modules/numpy/routines/#function-fromiter "Copy/link to this API element")

```python
fromiter(iterable, dtype: type, count: int = -1)
```

function frombuffer[](https://docs.exaloop.io/libraries/api/modules/numpy/routines/#function-frombuffer "Copy/link to this API element")

```python
frombuffer(buffer: str, dtype: type = float, count: int = -1, offset: int = 0)
```

function broadcast\_shapes[](https://docs.exaloop.io/libraries/api/modules/numpy/routines/#function-broadcast_shapes "Copy/link to this API element")

```python
broadcast_shapes(*args)
```

function broadcast\_to[](https://docs.exaloop.io/libraries/api/modules/numpy/routines/#function-broadcast_to "Copy/link to this API element")

```python
broadcast_to(x, shape)
```

function broadcast\_arrays[](https://docs.exaloop.io/libraries/api/modules/numpy/routines/#function-broadcast_arrays "Copy/link to this API element")

```python
broadcast_arrays(*args)
```

function meshgrid[](https://docs.exaloop.io/libraries/api/modules/numpy/routines/#function-meshgrid "Copy/link to this API element")

```python
meshgrid(*xi, copy: bool = True, sparse: Literal[bool] = False, indexing: Literal[str] = "xy")
```

function broadcast[](https://docs.exaloop.io/libraries/api/modules/numpy/routines/#function-broadcast "Copy/link to this API element")

```python
broadcast(*args)
```

function full[](https://docs.exaloop.io/libraries/api/modules/numpy/routines/#function-full "Copy/link to this API element")

```python
full(shape, fill_value, dtype: type = NoneType, order: str = "C")
```

function full\_like[](https://docs.exaloop.io/libraries/api/modules/numpy/routines/#function-full_like "Copy/link to this API element")

```python
full_like(prototype, fill_value, dtype: type = NoneType, order: str = "K")
```

function copyto[](https://docs.exaloop.io/libraries/api/modules/numpy/routines/#function-copyto "Copy/link to this API element")

```python
copyto(dst: ndarray, src, where = True)
```

function ndim[](https://docs.exaloop.io/libraries/api/modules/numpy/routines/#function-ndim "Copy/link to this API element")

```python
ndim(a)
```

function size[](https://docs.exaloop.io/libraries/api/modules/numpy/routines/#function-size "Copy/link to this API element")

```python
size(a, axis: Optional[int] = None)
```

function shape[](https://docs.exaloop.io/libraries/api/modules/numpy/routines/#function-shape "Copy/link to this API element")

```python
shape(a)
```

function reshape[](https://docs.exaloop.io/libraries/api/modules/numpy/routines/#function-reshape "Copy/link to this API element")

```python
reshape(a, newshape, order: str = "C")
```

function transpose[](https://docs.exaloop.io/libraries/api/modules/numpy/routines/#function-transpose "Copy/link to this API element")

```python
transpose(a, axes = None)
```

function ravel[](https://docs.exaloop.io/libraries/api/modules/numpy/routines/#function-ravel "Copy/link to this API element")

```python
ravel(a, order: str = "C")
```

function ascontiguousarray[](https://docs.exaloop.io/libraries/api/modules/numpy/routines/#function-ascontiguousarray "Copy/link to this API element")

```python
ascontiguousarray(a, dtype: type = NoneType)
```

function asfortranarray[](https://docs.exaloop.io/libraries/api/modules/numpy/routines/#function-asfortranarray "Copy/link to this API element")

```python
asfortranarray(a, dtype: type = NoneType)
```

function asfarray[](https://docs.exaloop.io/libraries/api/modules/numpy/routines/#function-asfarray "Copy/link to this API element")

```python
asfarray(a, dtype: type = float)
```

function moveaxis[](https://docs.exaloop.io/libraries/api/modules/numpy/routines/#function-moveaxis "Copy/link to this API element")

```python
moveaxis(a, source, destination)
```

function swapaxes[](https://docs.exaloop.io/libraries/api/modules/numpy/routines/#function-swapaxes "Copy/link to this API element")

```python
swapaxes(a, axis1: int, axis2: int)
```

function atleast\_1d[](https://docs.exaloop.io/libraries/api/modules/numpy/routines/#function-atleast_1d "Copy/link to this API element")

```python
atleast_1d(*arys)
```

function atleast\_2d[](https://docs.exaloop.io/libraries/api/modules/numpy/routines/#function-atleast_2d "Copy/link to this API element")

```python
atleast_2d(*arys)
```

function atleast\_3d[](https://docs.exaloop.io/libraries/api/modules/numpy/routines/#function-atleast_3d "Copy/link to this API element")

```python
atleast_3d(*arys)
```

function require[](https://docs.exaloop.io/libraries/api/modules/numpy/routines/#function-require "Copy/link to this API element")

```python
require(a, dtype: type = NoneType, requirements = None)
```

function concatenate[](https://docs.exaloop.io/libraries/api/modules/numpy/routines/#function-concatenate "Copy/link to this API element")

```python
concatenate(arrays, axis = 0, out = None, dtype: type = NoneType)
```

function expand\_dims[](https://docs.exaloop.io/libraries/api/modules/numpy/routines/#function-expand_dims "Copy/link to this API element")

```python
expand_dims(a, axis)
```

function stack[](https://docs.exaloop.io/libraries/api/modules/numpy/routines/#function-stack "Copy/link to this API element")

```python
stack(arrays, axis: int = 0, out = None, dtype: type = NoneType)
```

function vstack[](https://docs.exaloop.io/libraries/api/modules/numpy/routines/#function-vstack "Copy/link to this API element")

```python
vstack(tup, dtype: type = NoneType)
```

function hstack[](https://docs.exaloop.io/libraries/api/modules/numpy/routines/#function-hstack "Copy/link to this API element")

```python
hstack(tup, dtype: type = NoneType)
```

function dstack[](https://docs.exaloop.io/libraries/api/modules/numpy/routines/#function-dstack "Copy/link to this API element")

```python
dstack(tup)
```

variable row\_stack[](https://docs.exaloop.io/libraries/api/modules/numpy/routines/#variable-row_stack "Copy/link to this API element")

```python
row_stack = vstack
```

function column\_stack[](https://docs.exaloop.io/libraries/api/modules/numpy/routines/#function-column_stack "Copy/link to this API element")

```python
column_stack(tup)
```

function repeat[](https://docs.exaloop.io/libraries/api/modules/numpy/routines/#function-repeat "Copy/link to this API element")

```python
repeat(a, repeats, axis = None)
```

function delete[](https://docs.exaloop.io/libraries/api/modules/numpy/routines/#function-delete "Copy/link to this API element")

```python
delete(arr, obj, axis = None)
```

function append[](https://docs.exaloop.io/libraries/api/modules/numpy/routines/#function-append "Copy/link to this API element")

```python
append(arr, values, axis = None)
```

function insert[](https://docs.exaloop.io/libraries/api/modules/numpy/routines/#function-insert "Copy/link to this API element")

```python
insert(arr, obj, values, axis = None)
```

function array\_split[](https://docs.exaloop.io/libraries/api/modules/numpy/routines/#function-array_split "Copy/link to this API element")

```python
array_split(ary, indices_or_sections, axis: int = 0)
```

function split[](https://docs.exaloop.io/libraries/api/modules/numpy/routines/#function-split "Copy/link to this API element")

```python
split(ary, indices_or_sections, axis: int = 0)
```

function vsplit[](https://docs.exaloop.io/libraries/api/modules/numpy/routines/#function-vsplit "Copy/link to this API element")

```python
vsplit(ary, indices_or_sections)
```

function hsplit[](https://docs.exaloop.io/libraries/api/modules/numpy/routines/#function-hsplit "Copy/link to this API element")

```python
hsplit(ary, indices_or_sections)
```

function dsplit[](https://docs.exaloop.io/libraries/api/modules/numpy/routines/#function-dsplit "Copy/link to this API element")

```python
dsplit(ary, indices_or_sections)
```

function trim\_zeros[](https://docs.exaloop.io/libraries/api/modules/numpy/routines/#function-trim_zeros "Copy/link to this API element")

```python
trim_zeros(filt, trim: str = "fb")
```

function flip[](https://docs.exaloop.io/libraries/api/modules/numpy/routines/#function-flip "Copy/link to this API element")

```python
flip(m, axis = None)
```

function fliplr[](https://docs.exaloop.io/libraries/api/modules/numpy/routines/#function-fliplr "Copy/link to this API element")

```python
fliplr(m)
```

function flipud[](https://docs.exaloop.io/libraries/api/modules/numpy/routines/#function-flipud "Copy/link to this API element")

```python
flipud(m)
```

function rot90[](https://docs.exaloop.io/libraries/api/modules/numpy/routines/#function-rot90 "Copy/link to this API element")

```python
rot90(m, k: int = 1, axes: Tuple[int, int] = (0, 1))
```

function resize[](https://docs.exaloop.io/libraries/api/modules/numpy/routines/#function-resize "Copy/link to this API element")

```python
resize(a, new_shape)
```

function tile[](https://docs.exaloop.io/libraries/api/modules/numpy/routines/#function-tile "Copy/link to this API element")

```python
tile(A, reps)
```

function roll[](https://docs.exaloop.io/libraries/api/modules/numpy/routines/#function-roll "Copy/link to this API element")

```python
roll(a, shift, axis = None)
```

function block[](https://docs.exaloop.io/libraries/api/modules/numpy/routines/#function-block "Copy/link to this API element")

```python
block(arrays)
```

function isclose[](https://docs.exaloop.io/libraries/api/modules/numpy/routines/#function-isclose "Copy/link to this API element")

```python
isclose(a, b, rtol: float = 1e-05, atol: float = 1e-08, equal_nan: bool = False)
```

function allclose[](https://docs.exaloop.io/libraries/api/modules/numpy/routines/#function-allclose "Copy/link to this API element")

```python
allclose(a, b, rtol: float = 1e-05, atol: float = 1e-08, equal_nan: bool = False)
```

function array\_equal[](https://docs.exaloop.io/libraries/api/modules/numpy/routines/#function-array_equal "Copy/link to this API element")

```python
array_equal(a1, a2, equal_nan: bool = False)
```

function array\_equiv[](https://docs.exaloop.io/libraries/api/modules/numpy/routines/#function-array_equiv "Copy/link to this API element")

```python
array_equiv(a1, a2)
```

function squeeze[](https://docs.exaloop.io/libraries/api/modules/numpy/routines/#function-squeeze "Copy/link to this API element")

```python
squeeze(a, axis = None)
```

function pad[](https://docs.exaloop.io/libraries/api/modules/numpy/routines/#function-pad "Copy/link to this API element")

```python
pad(array, pad_width, mode = "constant", **kwargs)
```

function nonzero[](https://docs.exaloop.io/libraries/api/modules/numpy/routines/#function-nonzero "Copy/link to this API element")

```python
nonzero(a)
```

function flatnonzero[](https://docs.exaloop.io/libraries/api/modules/numpy/routines/#function-flatnonzero "Copy/link to this API element")

```python
flatnonzero(a)
```

function argwhere[](https://docs.exaloop.io/libraries/api/modules/numpy/routines/#function-argwhere "Copy/link to this API element")

```python
argwhere(a)
```

function where[](https://docs.exaloop.io/libraries/api/modules/numpy/routines/#function-where "Copy/link to this API element")

```python
where(condition, x, y)
```

@overload

```python
where(condition)
```

function extract[](https://docs.exaloop.io/libraries/api/modules/numpy/routines/#function-extract "Copy/link to this API element")

```python
extract(condition, arr)
```

function searchsorted[](https://docs.exaloop.io/libraries/api/modules/numpy/routines/#function-searchsorted "Copy/link to this API element")

```python
searchsorted(a, v, side: str = "left", sorter = None)
```

function take[](https://docs.exaloop.io/libraries/api/modules/numpy/routines/#function-take "Copy/link to this API element")

```python
take(a, indices, axis = None, out = None, mode: str = "raise")
```

function indices[](https://docs.exaloop.io/libraries/api/modules/numpy/routines/#function-indices "Copy/link to this API element")

```python
indices(dimensions, dtype: type = int, sparse: Literal[bool] = False)
```

function ix\_[](https://docs.exaloop.io/libraries/api/modules/numpy/routines/#function-ix_ "Copy/link to this API element")

```python
ix_(*args)
```

function ravel\_multi\_index[](https://docs.exaloop.io/libraries/api/modules/numpy/routines/#function-ravel_multi_index "Copy/link to this API element")

```python
ravel_multi_index(multi_index, dims, mode: str = "raise", order: str = "C")
```

function unravel\_index[](https://docs.exaloop.io/libraries/api/modules/numpy/routines/#function-unravel_index "Copy/link to this API element")

```python
unravel_index(indices, shape, order: str = "C")
```

function diag\_indices[](https://docs.exaloop.io/libraries/api/modules/numpy/routines/#function-diag_indices "Copy/link to this API element")

```python
diag_indices(n: int, ndim: int)
```

@overload

```python
diag_indices(n: int, ndim: Literal[int] = 2)
```

function diag\_indices\_from[](https://docs.exaloop.io/libraries/api/modules/numpy/routines/#function-diag_indices_from "Copy/link to this API element")

```python
diag_indices_from(arr)
```

function mask\_indices[](https://docs.exaloop.io/libraries/api/modules/numpy/routines/#function-mask_indices "Copy/link to this API element")

```python
mask_indices(n: int, mask_func, k = 0)
```

function tril\_indices[](https://docs.exaloop.io/libraries/api/modules/numpy/routines/#function-tril_indices "Copy/link to this API element")

```python
tril_indices(n: int, k: int = 0, m: Optional[int] = None)
```

function tril\_indices\_from[](https://docs.exaloop.io/libraries/api/modules/numpy/routines/#function-tril_indices_from "Copy/link to this API element")

```python
tril_indices_from(arr, k: int = 0)
```

function triu\_indices[](https://docs.exaloop.io/libraries/api/modules/numpy/routines/#function-triu_indices "Copy/link to this API element")

```python
triu_indices(n: int, k: int = 0, m: Optional[int] = None)
```

function triu\_indices\_from[](https://docs.exaloop.io/libraries/api/modules/numpy/routines/#function-triu_indices_from "Copy/link to this API element")

```python
triu_indices_from(arr, k: int = 0)
```

function take\_along\_axis[](https://docs.exaloop.io/libraries/api/modules/numpy/routines/#function-take_along_axis "Copy/link to this API element")

```python
take_along_axis(arr, indices, axis)
```

function choose[](https://docs.exaloop.io/libraries/api/modules/numpy/routines/#function-choose "Copy/link to this API element")

```python
choose(a, choices, out = None, mode: str = "raise")
```

function compress[](https://docs.exaloop.io/libraries/api/modules/numpy/routines/#function-compress "Copy/link to this API element")

```python
compress(condition, a, axis = None, out = None)
```

function diagonal[](https://docs.exaloop.io/libraries/api/modules/numpy/routines/#function-diagonal "Copy/link to this API element")

```python
diagonal(a, offset: int = 0, axis1: int = 0, axis2: int = 1)
```

function select[](https://docs.exaloop.io/libraries/api/modules/numpy/routines/#function-select "Copy/link to this API element")

```python
select(condlist: List, choicelist: List, default = 0)
```

function place[](https://docs.exaloop.io/libraries/api/modules/numpy/routines/#function-place "Copy/link to this API element")

```python
place(arr: ndarray, mask, vals)
```

function put[](https://docs.exaloop.io/libraries/api/modules/numpy/routines/#function-put "Copy/link to this API element")

```python
put(a: ndarray, ind, v, mode: str = "raise")
```

function put\_along\_axis[](https://docs.exaloop.io/libraries/api/modules/numpy/routines/#function-put_along_axis "Copy/link to this API element")

```python
put_along_axis(arr: ndarray, indices, values, axis)
```

function putmask[](https://docs.exaloop.io/libraries/api/modules/numpy/routines/#function-putmask "Copy/link to this API element")

```python
putmask(a: ndarray, mask, values)
```

function fill\_diagonal[](https://docs.exaloop.io/libraries/api/modules/numpy/routines/#function-fill_diagonal "Copy/link to this API element")

```python
fill_diagonal(a: ndarray, val, wrap: bool = False)
```

function round[](https://docs.exaloop.io/libraries/api/modules/numpy/routines/#function-round "Copy/link to this API element")

```python
round(a, decimals: int = 0, out = None)
```

variable around[](https://docs.exaloop.io/libraries/api/modules/numpy/routines/#variable-around "Copy/link to this API element")

```python
around = round
```

function clip[](https://docs.exaloop.io/libraries/api/modules/numpy/routines/#function-clip "Copy/link to this API element")

```python
clip(a, a_min, a_max, out = None)
```

function ndenumerate[](https://docs.exaloop.io/libraries/api/modules/numpy/routines/#function-ndenumerate "Copy/link to this API element")

```python
ndenumerate(arr)
```

function ndindex[](https://docs.exaloop.io/libraries/api/modules/numpy/routines/#function-ndindex "Copy/link to this API element")

```python
ndindex(*shape)
```

function iterable[](https://docs.exaloop.io/libraries/api/modules/numpy/routines/#function-iterable "Copy/link to this API element")

```python
iterable(y)
```

function packbits[](https://docs.exaloop.io/libraries/api/modules/numpy/routines/#function-packbits "Copy/link to this API element")

```python
packbits(a, axis = None, bitorder: str = "big")
```

function unpackbits[](https://docs.exaloop.io/libraries/api/modules/numpy/routines/#function-unpackbits "Copy/link to this API element")

```python
unpackbits(a, axis = None, count = None, bitorder: str = "big")
```

function isposinf[](https://docs.exaloop.io/libraries/api/modules/numpy/routines/#function-isposinf "Copy/link to this API element")

```python
isposinf(x, out = None)
```

function isneginf[](https://docs.exaloop.io/libraries/api/modules/numpy/routines/#function-isneginf "Copy/link to this API element")

```python
isneginf(x, out = None)
```

function iscomplex[](https://docs.exaloop.io/libraries/api/modules/numpy/routines/#function-iscomplex "Copy/link to this API element")

```python
iscomplex(x)
```

function iscomplexobj[](https://docs.exaloop.io/libraries/api/modules/numpy/routines/#function-iscomplexobj "Copy/link to this API element")

```python
iscomplexobj(x)
```

function isreal[](https://docs.exaloop.io/libraries/api/modules/numpy/routines/#function-isreal "Copy/link to this API element")

```python
isreal(x)
```

function isrealobj[](https://docs.exaloop.io/libraries/api/modules/numpy/routines/#function-isrealobj "Copy/link to this API element")

```python
isrealobj(x)
```

function isfortran[](https://docs.exaloop.io/libraries/api/modules/numpy/routines/#function-isfortran "Copy/link to this API element")

```python
isfortran(a: ndarray)
```

function isscalar[](https://docs.exaloop.io/libraries/api/modules/numpy/routines/#function-isscalar "Copy/link to this API element")

```python
isscalar(element)
```

function real[](https://docs.exaloop.io/libraries/api/modules/numpy/routines/#function-real "Copy/link to this API element")

```python
real(val)
```

function imag[](https://docs.exaloop.io/libraries/api/modules/numpy/routines/#function-imag "Copy/link to this API element")

```python
imag(val)
```

class ndarray[](https://docs.exaloop.io/libraries/api/modules/numpy/routines/#class-ndarray "Copy/link to this API element")

@extend

```python
class ndarray[dtype, ndim]
```

Extends [`numpy.ndarray.ndarray`](https://docs.exaloop.io/libraries/api/classes/numpy/ndarray/ndarray/) · [View complete class API →](https://docs.exaloop.io/libraries/api/classes/numpy/ndarray/ndarray/)

Properties 2

real[](https://docs.exaloop.io/libraries/api/modules/numpy/routines/#class-ndarray-real "Copy/link to this API element")

@property

```python
real(self: Self, Self: type)
```

imag[](https://docs.exaloop.io/libraries/api/modules/numpy/routines/#class-ndarray-imag "Copy/link to this API element")

@property

```python
imag(self: Self, Self: type)
```

Methods 16

take[](https://docs.exaloop.io/libraries/api/modules/numpy/routines/#class-ndarray-take "Copy/link to this API element")

```python
take(self: Self, indices, axis = None, out = None, mode: str = "raise", Self: type)
```

squeeze[](https://docs.exaloop.io/libraries/api/modules/numpy/routines/#class-ndarray-squeeze "Copy/link to this API element")

```python
squeeze(self: Self, axis = None, Self: type)
```

nonzero[](https://docs.exaloop.io/libraries/api/modules/numpy/routines/#class-ndarray-nonzero "Copy/link to this API element")

```python
nonzero(self: Self, Self: type)
```

searchsorted[](https://docs.exaloop.io/libraries/api/modules/numpy/routines/#class-ndarray-searchsorted "Copy/link to this API element")

```python
searchsorted(self: Self, v, side: str = "left", sorter = None, Self: type)
```

repeat[](https://docs.exaloop.io/libraries/api/modules/numpy/routines/#class-ndarray-repeat "Copy/link to this API element")

```python
repeat(self: Self, repeats, axis = None, Self: type)
```

compress[](https://docs.exaloop.io/libraries/api/modules/numpy/routines/#class-ndarray-compress "Copy/link to this API element")

```python
compress(self: Self, condition, axis = None, out = None, Self: type)
```

choose[](https://docs.exaloop.io/libraries/api/modules/numpy/routines/#class-ndarray-choose "Copy/link to this API element")

```python
choose(self: Self, choices, out = None, mode: str = "raise", Self: type)
```

diagonal[](https://docs.exaloop.io/libraries/api/modules/numpy/routines/#class-ndarray-diagonal "Copy/link to this API element")

```python
diagonal(self: Self, offset: int = 0, axis1: int = 0, axis2: int = 1, Self: type)
```

put[](https://docs.exaloop.io/libraries/api/modules/numpy/routines/#class-ndarray-put "Copy/link to this API element")

```python
put(self: Self, ind, v, mode: str = "raise", Self: type)
```

round[](https://docs.exaloop.io/libraries/api/modules/numpy/routines/#class-ndarray-round "Copy/link to this API element")

```python
round(self: Self, decimals: int = 0, out = None, Self: type)
```

clip[](https://docs.exaloop.io/libraries/api/modules/numpy/routines/#class-ndarray-clip "Copy/link to this API element")

```python
clip(self: Self, min = None, max = None, out = None, Self: type)
```

real[](https://docs.exaloop.io/libraries/api/modules/numpy/routines/#class-ndarray-real "Copy/link to this API element")

```python
real(self: Self, val, Self: type)
```

imag[](https://docs.exaloop.io/libraries/api/modules/numpy/routines/#class-ndarray-imag "Copy/link to this API element")

```python
imag(self: Self, val, Self: type)
```

conj[](https://docs.exaloop.io/libraries/api/modules/numpy/routines/#class-ndarray-conj "Copy/link to this API element")

```python
conj(self: Self, Self: type)
```

conjugate[](https://docs.exaloop.io/libraries/api/modules/numpy/routines/#class-ndarray-conjugate "Copy/link to this API element")

```python
conjugate(self: Self, Self: type)
```

flat[](https://docs.exaloop.io/libraries/api/modules/numpy/routines/#class-ndarray-flat "Copy/link to this API element")

```python
flat(self: Self, val, Self: type)
```

class busdaycalendar[](https://docs.exaloop.io/libraries/api/modules/numpy/routines/#class-busdaycalendar "Copy/link to this API element")

@extend

```python
class busdaycalendar
```

Extends [`numpy.npdatetime.busdaycalendar`](https://docs.exaloop.io/libraries/api/classes/numpy/npdatetime/busdaycalendar/) · [View complete class API →](https://docs.exaloop.io/libraries/api/classes/numpy/npdatetime/busdaycalendar/)

Properties 2

weekmask[](https://docs.exaloop.io/libraries/api/modules/numpy/routines/#class-busdaycalendar-weekmask "Copy/link to this API element")

@property

```python
weekmask(self: Self, Self: type)
```

holidays[](https://docs.exaloop.io/libraries/api/modules/numpy/routines/#class-busdaycalendar-holidays "Copy/link to this API element")

@property

```python
holidays(self: Self, Self: type)
```

function busday\_offset[](https://docs.exaloop.io/libraries/api/modules/numpy/routines/#function-busday_offset "Copy/link to this API element")

```python
busday_offset(dates, offsets, roll: str = "raise", weekmask = None, holidays = None, busdaycal = None, out = None)
```

function busday\_count[](https://docs.exaloop.io/libraries/api/modules/numpy/routines/#function-busday_count "Copy/link to this API element")

```python
busday_count(begindates, enddates, weekmask = None, holidays = None, busdaycal = None, out = None)
```

function is\_busday[](https://docs.exaloop.io/libraries/api/modules/numpy/routines/#function-is_busday "Copy/link to this API element")

```python
is_busday(dates, weekmask = None, holidays = None, busdaycal = None, out = None)
```

function datetime\_data[](https://docs.exaloop.io/libraries/api/modules/numpy/routines/#function-datetime_data "Copy/link to this API element")

```python
datetime_data(dtype: type)
```

function datetime\_as\_string[](https://docs.exaloop.io/libraries/api/modules/numpy/routines/#function-datetime_as_string "Copy/link to this API element")

```python
datetime_as_string(arr, unit = None, timezone: str = "naive")
```

Source: https://docs.exaloop.io/libraries/api/modules/numpy/routines/
