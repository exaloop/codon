# numpy.format

Standard library module [View source](https://github.com/exaloop/codon/blob/master/stdlib/numpy/format.codon)

variable MAX\_FLOAT\_PREC[](https://docs.exaloop.io/libraries/api/modules/numpy/format/#variable-max_float_prec "Copy/link to this API element")

```python
MAX_FLOAT_PREC = 4
```

class DefaultFormat[](https://docs.exaloop.io/libraries/api/modules/numpy/format/#class-defaultformat "Copy/link to this API element")

```python
class DefaultFormat
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/numpy/format/DefaultFormat/)

Special methods 1

\_\_call\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/format/#class-defaultformat-__call__ "Copy/link to this API element")

```python
__call__(self: Self, x, Self: type)
```

class TimeDeltaFormat[](https://docs.exaloop.io/libraries/api/modules/numpy/format/#class-timedeltaformat "Copy/link to this API element")

```python
class TimeDeltaFormat
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/numpy/format/TimeDeltaFormat/)

Special methods 1

\_\_call\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/format/#class-timedeltaformat-__call__ "Copy/link to this API element")

```python
__call__(self: Self, x, Self: type)
```

class FloatingFormat[](https://docs.exaloop.io/libraries/api/modules/numpy/format/#class-floatingformat "Copy/link to this API element")

```python
class FloatingFormat
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/numpy/format/FloatingFormat/)

Constructors 2

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/format/#class-floatingformat-__init__ "Copy/link to this API element")

```python
__init__(self: Self, scientific: bool = False, sign: bool = False, unique: bool = True, trim: str = ".", precision: int = -1, min_digits: int = -1, pad_left: int = -1, pad_right: int = -1, exp_size: int = -1, Self: type)
```

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/format/#class-floatingformat-__init__-overload-2 "Copy/link to this API element")

```python
__init__(self: Self, a: ndarray, sign: bool = False, Self: type)
```

Properties 9

scientific[](https://docs.exaloop.io/libraries/api/modules/numpy/format/#class-floatingformat-scientific "Copy/link to this API element")

@property

```python
scientific(self: Self, Self: type)
```

sign[](https://docs.exaloop.io/libraries/api/modules/numpy/format/#class-floatingformat-sign "Copy/link to this API element")

@property

```python
sign(self: Self, Self: type)
```

unique[](https://docs.exaloop.io/libraries/api/modules/numpy/format/#class-floatingformat-unique "Copy/link to this API element")

@property

```python
unique(self: Self, Self: type)
```

trim[](https://docs.exaloop.io/libraries/api/modules/numpy/format/#class-floatingformat-trim "Copy/link to this API element")

@property

```python
trim(self: Self, Self: type)
```

precision[](https://docs.exaloop.io/libraries/api/modules/numpy/format/#class-floatingformat-precision "Copy/link to this API element")

@property

```python
precision(self: Self, Self: type)
```

min\_digits[](https://docs.exaloop.io/libraries/api/modules/numpy/format/#class-floatingformat-min_digits "Copy/link to this API element")

@property

```python
min_digits(self: Self, Self: type)
```

pad\_left[](https://docs.exaloop.io/libraries/api/modules/numpy/format/#class-floatingformat-pad_left "Copy/link to this API element")

@property

```python
pad_left(self: Self, Self: type)
```

pad\_right[](https://docs.exaloop.io/libraries/api/modules/numpy/format/#class-floatingformat-pad_right "Copy/link to this API element")

@property

```python
pad_right(self: Self, Self: type)
```

exp\_size[](https://docs.exaloop.io/libraries/api/modules/numpy/format/#class-floatingformat-exp_size "Copy/link to this API element")

@property

```python
exp_size(self: Self, Self: type)
```

Special methods 1

\_\_call\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/format/#class-floatingformat-__call__ "Copy/link to this API element")

```python
__call__(self: Self, x, Self: type)
```

class ComplexFormat[](https://docs.exaloop.io/libraries/api/modules/numpy/format/#class-complexformat "Copy/link to this API element")

```python
class ComplexFormat
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/numpy/format/ComplexFormat/)

Constructors 1

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/format/#class-complexformat-__init__ "Copy/link to this API element")

```python
__init__(self: Self, a: ndarray, Self: type)
```

Special methods 1

\_\_call\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/format/#class-complexformat-__call__ "Copy/link to this API element")

```python
__call__(self: Self, x, Self: type)
```

function get\_formatter[](https://docs.exaloop.io/libraries/api/modules/numpy/format/#function-get_formatter "Copy/link to this API element")

```python
get_formatter(a: ndarray)
```

class ndarray[](https://docs.exaloop.io/libraries/api/modules/numpy/format/#class-ndarray "Copy/link to this API element")

@extend

```python
class ndarray[dtype, ndim]
```

Extends [`numpy.ndarray.ndarray`](https://docs.exaloop.io/libraries/api/classes/numpy/ndarray/ndarray/) · [View complete class API →](https://docs.exaloop.io/libraries/api/classes/numpy/ndarray/ndarray/)

Special methods 2

\_\_repr\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/format/#class-ndarray-__repr__ "Copy/link to this API element")

```python
__repr__(self: Self, Self: type)
```

\_\_str\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/format/#class-ndarray-__str__ "Copy/link to this API element")

```python
__str__(self: Self, Self: type)
```

Source: https://docs.exaloop.io/libraries/api/modules/numpy/format/
