# numpy.ndarray

Standard library Module [View source](https://github.com/exaloop/codon/blob/master/stdlib/numpy/ndarray.codon)

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
num(self: Self)
```

c\_contiguous[](https://docs.exaloop.io/libraries/api/modules/numpy/ndarray/#class-flagsobj-c_contiguous "Copy/link to this API element")

@property

```python
c_contiguous(self: Self)
```

f\_contiguous[](https://docs.exaloop.io/libraries/api/modules/numpy/ndarray/#class-flagsobj-f_contiguous "Copy/link to this API element")

@property

```python
f_contiguous(self: Self)
```

contiguous[](https://docs.exaloop.io/libraries/api/modules/numpy/ndarray/#class-flagsobj-contiguous "Copy/link to this API element")

@property

```python
contiguous(self: Self)
```

owndata[](https://docs.exaloop.io/libraries/api/modules/numpy/ndarray/#class-flagsobj-owndata "Copy/link to this API element")

@property

```python
owndata(self: Self)
```

writeable[](https://docs.exaloop.io/libraries/api/modules/numpy/ndarray/#class-flagsobj-writeable "Copy/link to this API element")

@property

```python
writeable(self: Self)
```

aligned[](https://docs.exaloop.io/libraries/api/modules/numpy/ndarray/#class-flagsobj-aligned "Copy/link to this API element")

@property

```python
aligned(self: Self)
```

writebackifcopy[](https://docs.exaloop.io/libraries/api/modules/numpy/ndarray/#class-flagsobj-writebackifcopy "Copy/link to this API element")

@property

```python
writebackifcopy(self: Self)
```

Special methods 1

\_\_str\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/ndarray/#class-flagsobj-__str__ "Copy/link to this API element")

```python
__str__(self: Self)
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
__init__(self: Self, base: A)
```

Properties 1

coords[](https://docs.exaloop.io/libraries/api/modules/numpy/ndarray/#class-flatiter-coords "Copy/link to this API element")

@property

```python
coords(self: Self)
```

Methods 1

copy[](https://docs.exaloop.io/libraries/api/modules/numpy/ndarray/#class-flatiter-copy "Copy/link to this API element")

```python
copy(self: Self)
```

Special methods 3

\_\_iter\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/ndarray/#class-flatiter-__iter__ "Copy/link to this API element")

```python
__iter__(self: Self)
```

\_\_getitem\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/ndarray/#class-flatiter-__getitem__ "Copy/link to this API element")

```python
__getitem__(self: Self, index: int)
```

```python
__getitem__(self: Self, s: slice)
```

\_\_setitem\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/ndarray/#class-flatiter-__setitem__ "Copy/link to this API element")

```python
__setitem__(self: Self, index: int, value)
```

```python
__setitem__(self: Self, s: slice, value)
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
__new__(shape: Tuple[(ndim, int)], strides: Tuple[(ndim, int)], data: Ptr[dtype]) -> ndarray[(dtype, ndim)]
```

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/ndarray/#class-ndarray-__new__-overload-2 "Copy/link to this API element")

```python
__new__(shape: Tuple[(ndim, int)], data: Ptr[dtype], fcontig: bool = False)
```

Properties 9

shape[](https://docs.exaloop.io/libraries/api/modules/numpy/ndarray/#class-ndarray-shape "Copy/link to this API element")

@property

```python
shape(self: Self)
```

strides[](https://docs.exaloop.io/libraries/api/modules/numpy/ndarray/#class-ndarray-strides "Copy/link to this API element")

@property

```python
strides(self: Self)
```

flags[](https://docs.exaloop.io/libraries/api/modules/numpy/ndarray/#class-ndarray-flags "Copy/link to this API element")

@property

```python
flags(self: Self)
```

data[](https://docs.exaloop.io/libraries/api/modules/numpy/ndarray/#class-ndarray-data "Copy/link to this API element")

@property

```python
data(self: Self)
```

size[](https://docs.exaloop.io/libraries/api/modules/numpy/ndarray/#class-ndarray-size "Copy/link to this API element")

@property

```python
size(self: Self)
```

itemsize[](https://docs.exaloop.io/libraries/api/modules/numpy/ndarray/#class-ndarray-itemsize "Copy/link to this API element")

@property

```python
itemsize(self: Self)
```

nbytes[](https://docs.exaloop.io/libraries/api/modules/numpy/ndarray/#class-ndarray-nbytes "Copy/link to this API element")

@property

```python
nbytes(self: Self)
```

T[](https://docs.exaloop.io/libraries/api/modules/numpy/ndarray/#class-ndarray-t "Copy/link to this API element")

@property

```python
T(self: Self)
```

flat[](https://docs.exaloop.io/libraries/api/modules/numpy/ndarray/#class-ndarray-flat "Copy/link to this API element")

@property

```python
flat(self: Self)
```

Methods 15

item[](https://docs.exaloop.io/libraries/api/modules/numpy/ndarray/#class-ndarray-item "Copy/link to this API element")

```python
item(self: Self, *args)
```

transpose[](https://docs.exaloop.io/libraries/api/modules/numpy/ndarray/#class-ndarray-transpose "Copy/link to this API element")

```python
transpose(self: Self, *axes)
```

```python
transpose(self: Self, axes: List[int])
```

swapaxes[](https://docs.exaloop.io/libraries/api/modules/numpy/ndarray/#class-ndarray-swapaxes "Copy/link to this API element")

```python
swapaxes(self: Self, axis1: int, axis2: int)
```

reshape[](https://docs.exaloop.io/libraries/api/modules/numpy/ndarray/#class-ndarray-reshape "Copy/link to this API element")

```python
reshape(self: Self, *shape, order: str = "C")
```

tobytes[](https://docs.exaloop.io/libraries/api/modules/numpy/ndarray/#class-ndarray-tobytes "Copy/link to this API element")

```python
tobytes(self: Self, order: str = "C")
```

ravel[](https://docs.exaloop.io/libraries/api/modules/numpy/ndarray/#class-ndarray-ravel "Copy/link to this API element")

```python
ravel(self: Self, order: str = "C")
```

flatten[](https://docs.exaloop.io/libraries/api/modules/numpy/ndarray/#class-ndarray-flatten "Copy/link to this API element")

```python
flatten(self: Self, order: str = "C")
```

flat[](https://docs.exaloop.io/libraries/api/modules/numpy/ndarray/#class-ndarray-flat "Copy/link to this API element")

```python
flat(self: Self, value)
```

tolist[](https://docs.exaloop.io/libraries/api/modules/numpy/ndarray/#class-ndarray-tolist "Copy/link to this API element")

```python
tolist(self: Self)
```

astype[](https://docs.exaloop.io/libraries/api/modules/numpy/ndarray/#class-ndarray-astype "Copy/link to this API element")

```python
astype[dtype: type](self: Self, order: str = "K", copy: bool = True)
```

copy[](https://docs.exaloop.io/libraries/api/modules/numpy/ndarray/#class-ndarray-copy "Copy/link to this API element")

```python
copy(self: Self, order: str = "C")
```

map[](https://docs.exaloop.io/libraries/api/modules/numpy/ndarray/#class-ndarray-map "Copy/link to this API element")

```python
map[inplace: Literal[bool] = False](self: Self, fn)
```

fill[](https://docs.exaloop.io/libraries/api/modules/numpy/ndarray/#class-ndarray-fill "Copy/link to this API element")

```python
fill(self: Self, value)
```

view[](https://docs.exaloop.io/libraries/api/modules/numpy/ndarray/#class-ndarray-view "Copy/link to this API element")

```python
view[dtype: type](self: Self)
```

byteswap[](https://docs.exaloop.io/libraries/api/modules/numpy/ndarray/#class-ndarray-byteswap "Copy/link to this API element")

```python
byteswap(self: Self, inplace: bool = False)
```

Special methods 7

\_\_len\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/ndarray/#class-ndarray-__len__ "Copy/link to this API element")

```python
__len__(self: Self)
```

\_\_iter\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/ndarray/#class-ndarray-__iter__ "Copy/link to this API element")

```python
__iter__(self: Self)
```

\_\_copy\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/ndarray/#class-ndarray-__copy__ "Copy/link to this API element")

```python
__copy__(self: Self)
```

\_\_int\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/ndarray/#class-ndarray-__int__ "Copy/link to this API element")

```python
__int__(self: Self)
```

\_\_float\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/ndarray/#class-ndarray-__float__ "Copy/link to this API element")

```python
__float__(self: Self)
```

\_\_complex\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/ndarray/#class-ndarray-__complex__ "Copy/link to this API element")

```python
__complex__(self: Self)
```

\_\_bool\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/ndarray/#class-ndarray-__bool__ "Copy/link to this API element")

```python
__bool__(self: Self)
```

Source: https://docs.exaloop.io/libraries/api/modules/numpy/ndarray/
