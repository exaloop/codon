# numpy.ndarray

Standard library module [View source](https://github.com/exaloop/codon/blob/master/stdlib/numpy/ndarray.codon)

variable newaxis[](https://docs.exaloop.io/libraries/api/modules/numpy/ndarray/#variable-newaxis "Copy/link to this API element")

```python
newaxis = None
```

class flagsobj[](https://docs.exaloop.io/libraries/api/modules/numpy/ndarray/#class-flagsobj "Copy/link to this API element")

@tuple

```python
class flagsobj
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/numpy/ndarray/flagsobj/)

Constructors 2

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/ndarray/#class-flagsobj-__new__ "Copy/link to this API element")

```python
__new__(f: int)
```

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/ndarray/#class-flagsobj-__new__-overload-2 "Copy/link to this API element")

```python
__new__(ccontig: bool, fcontig: bool)
```

Properties 8

num[](https://docs.exaloop.io/libraries/api/modules/numpy/ndarray/#class-flagsobj-num "Copy/link to this API element")

@property

```python
num(self: Self, Self: type)
```

c\_contiguous[](https://docs.exaloop.io/libraries/api/modules/numpy/ndarray/#class-flagsobj-c_contiguous "Copy/link to this API element")

@property

```python
c_contiguous(self: Self, Self: type)
```

f\_contiguous[](https://docs.exaloop.io/libraries/api/modules/numpy/ndarray/#class-flagsobj-f_contiguous "Copy/link to this API element")

@property

```python
f_contiguous(self: Self, Self: type)
```

contiguous[](https://docs.exaloop.io/libraries/api/modules/numpy/ndarray/#class-flagsobj-contiguous "Copy/link to this API element")

@property

```python
contiguous(self: Self, Self: type)
```

owndata[](https://docs.exaloop.io/libraries/api/modules/numpy/ndarray/#class-flagsobj-owndata "Copy/link to this API element")

@property

```python
owndata(self: Self, Self: type)
```

writeable[](https://docs.exaloop.io/libraries/api/modules/numpy/ndarray/#class-flagsobj-writeable "Copy/link to this API element")

@property

```python
writeable(self: Self, Self: type)
```

aligned[](https://docs.exaloop.io/libraries/api/modules/numpy/ndarray/#class-flagsobj-aligned "Copy/link to this API element")

@property

```python
aligned(self: Self, Self: type)
```

writebackifcopy[](https://docs.exaloop.io/libraries/api/modules/numpy/ndarray/#class-flagsobj-writebackifcopy "Copy/link to this API element")

@property

```python
writebackifcopy(self: Self, Self: type)
```

Special methods 1

\_\_str\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/ndarray/#class-flagsobj-__str__ "Copy/link to this API element")

```python
__str__(self: Self, Self: type)
```

class flatiter[](https://docs.exaloop.io/libraries/api/modules/numpy/ndarray/#class-flatiter "Copy/link to this API element")

```python
class flatiter[A]
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/numpy/ndarray/flatiter/)

Fields 3

`base` `A`

`index` `int`

`A` `type`

Constructors 1

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/ndarray/#class-flatiter-__init__ "Copy/link to this API element")

```python
__init__(self: Self, base: A, Self: type)
```

Properties 1

coords[](https://docs.exaloop.io/libraries/api/modules/numpy/ndarray/#class-flatiter-coords "Copy/link to this API element")

@property

```python
coords(self: Self, Self: type)
```

Methods 1

copy[](https://docs.exaloop.io/libraries/api/modules/numpy/ndarray/#class-flatiter-copy "Copy/link to this API element")

```python
copy(self: Self, Self: type)
```

Special methods 3

\_\_iter\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/ndarray/#class-flatiter-__iter__ "Copy/link to this API element")

```python
__iter__(self: Self, Self: type)
```

\_\_getitem\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/ndarray/#class-flatiter-__getitem__ "Copy/link to this API element")

```python
__getitem__(self: Self, index: int, Self: type)
```

```python
__getitem__(self: Self, s: Slice, Self: type)
```

\_\_setitem\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/ndarray/#class-flatiter-__setitem__ "Copy/link to this API element")

```python
__setitem__(self: Self, index: int, value, Self: type)
```

```python
__setitem__(self: Self, s: Slice, value, Self: type)
```

class ndarray[](https://docs.exaloop.io/libraries/api/modules/numpy/ndarray/#class-ndarray "Copy/link to this API element")

@tuple

```python
class ndarray[dtype, ndim]
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/numpy/ndarray/ndarray/)

Fields 2

`dtype` `type`

`ndim` `Literal[int]`

Constructors 2

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/ndarray/#class-ndarray-__new__ "Copy/link to this API element")

```python
__new__(shape: Tuple[ndim, int], strides: Tuple[ndim, int], data: Ptr[dtype])
```

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/ndarray/#class-ndarray-__new__-overload-2 "Copy/link to this API element")

```python
__new__(shape: Tuple[ndim, int], data: Ptr[dtype], fcontig: bool = False)
```

Properties 9

shape[](https://docs.exaloop.io/libraries/api/modules/numpy/ndarray/#class-ndarray-shape "Copy/link to this API element")

@property

```python
shape(self: Self, Self: type)
```

strides[](https://docs.exaloop.io/libraries/api/modules/numpy/ndarray/#class-ndarray-strides "Copy/link to this API element")

@property

```python
strides(self: Self, Self: type)
```

flags[](https://docs.exaloop.io/libraries/api/modules/numpy/ndarray/#class-ndarray-flags "Copy/link to this API element")

@property

```python
flags(self: Self, Self: type)
```

data[](https://docs.exaloop.io/libraries/api/modules/numpy/ndarray/#class-ndarray-data "Copy/link to this API element")

@property

```python
data(self: Self, Self: type)
```

size[](https://docs.exaloop.io/libraries/api/modules/numpy/ndarray/#class-ndarray-size "Copy/link to this API element")

@property

```python
size(self: Self, Self: type)
```

itemsize[](https://docs.exaloop.io/libraries/api/modules/numpy/ndarray/#class-ndarray-itemsize "Copy/link to this API element")

@property

```python
itemsize(self: Self, Self: type)
```

nbytes[](https://docs.exaloop.io/libraries/api/modules/numpy/ndarray/#class-ndarray-nbytes "Copy/link to this API element")

@property

```python
nbytes(self: Self, Self: type)
```

T[](https://docs.exaloop.io/libraries/api/modules/numpy/ndarray/#class-ndarray-t "Copy/link to this API element")

@property

```python
T(self: Self, Self: type)
```

flat[](https://docs.exaloop.io/libraries/api/modules/numpy/ndarray/#class-ndarray-flat "Copy/link to this API element")

@property

```python
flat(self: Self, Self: type)
```

Methods 15

item[](https://docs.exaloop.io/libraries/api/modules/numpy/ndarray/#class-ndarray-item "Copy/link to this API element")

```python
item(self: Self, *args, Self: type)
```

transpose[](https://docs.exaloop.io/libraries/api/modules/numpy/ndarray/#class-ndarray-transpose "Copy/link to this API element")

```python
transpose(self: Self, *axes, Self: type)
```

```python
transpose(self: Self, axes: List[int], Self: type)
```

swapaxes[](https://docs.exaloop.io/libraries/api/modules/numpy/ndarray/#class-ndarray-swapaxes "Copy/link to this API element")

```python
swapaxes(self: Self, axis1: int, axis2: int, Self: type)
```

reshape[](https://docs.exaloop.io/libraries/api/modules/numpy/ndarray/#class-ndarray-reshape "Copy/link to this API element")

```python
reshape(self: Self, *shape, order: str = "C", Self: type)
```

tobytes[](https://docs.exaloop.io/libraries/api/modules/numpy/ndarray/#class-ndarray-tobytes "Copy/link to this API element")

```python
tobytes(self: Self, order: str = "C", Self: type)
```

ravel[](https://docs.exaloop.io/libraries/api/modules/numpy/ndarray/#class-ndarray-ravel "Copy/link to this API element")

```python
ravel(self: Self, order: str = "C", Self: type)
```

flatten[](https://docs.exaloop.io/libraries/api/modules/numpy/ndarray/#class-ndarray-flatten "Copy/link to this API element")

```python
flatten(self: Self, order: str = "C", Self: type)
```

flat[](https://docs.exaloop.io/libraries/api/modules/numpy/ndarray/#class-ndarray-flat "Copy/link to this API element")

```python
flat(self: Self, value, Self: type)
```

tolist[](https://docs.exaloop.io/libraries/api/modules/numpy/ndarray/#class-ndarray-tolist "Copy/link to this API element")

```python
tolist(self: Self, Self: type)
```

astype[](https://docs.exaloop.io/libraries/api/modules/numpy/ndarray/#class-ndarray-astype "Copy/link to this API element")

```python
astype(self: Self, dtype: type, order: str = "K", copy: bool = True, Self: type)
```

copy[](https://docs.exaloop.io/libraries/api/modules/numpy/ndarray/#class-ndarray-copy "Copy/link to this API element")

```python
copy(self: Self, order: str = "C", Self: type)
```

map[](https://docs.exaloop.io/libraries/api/modules/numpy/ndarray/#class-ndarray-map "Copy/link to this API element")

```python
map(self: Self, fn, inplace: Literal[bool] = False, Self: type)
```

fill[](https://docs.exaloop.io/libraries/api/modules/numpy/ndarray/#class-ndarray-fill "Copy/link to this API element")

```python
fill(self: Self, value, Self: type)
```

view[](https://docs.exaloop.io/libraries/api/modules/numpy/ndarray/#class-ndarray-view "Copy/link to this API element")

```python
view(self: Self, dtype: type, Self: type)
```

byteswap[](https://docs.exaloop.io/libraries/api/modules/numpy/ndarray/#class-ndarray-byteswap "Copy/link to this API element")

```python
byteswap(self: Self, inplace: bool = False, Self: type)
```

Special methods 7

\_\_len\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/ndarray/#class-ndarray-__len__ "Copy/link to this API element")

```python
__len__(self: Self, Self: type)
```

\_\_iter\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/ndarray/#class-ndarray-__iter__ "Copy/link to this API element")

```python
__iter__(self: Self, Self: type)
```

\_\_copy\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/ndarray/#class-ndarray-__copy__ "Copy/link to this API element")

```python
__copy__(self: Self, Self: type)
```

\_\_int\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/ndarray/#class-ndarray-__int__ "Copy/link to this API element")

```python
__int__(self: Self, Self: type)
```

\_\_float\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/ndarray/#class-ndarray-__float__ "Copy/link to this API element")

```python
__float__(self: Self, Self: type)
```

\_\_complex\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/ndarray/#class-ndarray-__complex__ "Copy/link to this API element")

```python
__complex__(self: Self, Self: type)
```

\_\_bool\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/ndarray/#class-ndarray-__bool__ "Copy/link to this API element")

```python
__bool__(self: Self, Self: type)
```

Source: https://docs.exaloop.io/libraries/api/modules/numpy/ndarray/
