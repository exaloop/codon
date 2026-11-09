# numpy.ufunc

Standard library module [View source](https://github.com/exaloop/codon/blob/master/stdlib/numpy/ufunc.codon)

function decide\_types[](https://docs.exaloop.io/libraries/api/modules/numpy/ufunc/#function-decide_types "Copy/link to this API element")

```python
decide_types(x, y, dtype: type)
```

function decide\_types\_copysign[](https://docs.exaloop.io/libraries/api/modules/numpy/ufunc/#function-decide_types_copysign "Copy/link to this API element")

```python
decide_types_copysign(x, y, dtype: type)
```

function decide\_types\_ldexp[](https://docs.exaloop.io/libraries/api/modules/numpy/ufunc/#function-decide_types_ldexp "Copy/link to this API element")

```python
decide_types_ldexp(x, y, dtype: type)
```

class UnaryUFunc[](https://docs.exaloop.io/libraries/api/modules/numpy/ufunc/#class-unaryufunc "Copy/link to this API element")

@tuple

```python
class UnaryUFunc[__name__, F]
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/numpy/ufunc/UnaryUFunc/)

Fields 1

`F` `type`

Constructors 1

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/ufunc/#class-unaryufunc-__new__ "Copy/link to this API element")

```python
__new__(op: F, name: Literal[str], F: type)
```

Properties 2

nin[](https://docs.exaloop.io/libraries/api/modules/numpy/ufunc/#class-unaryufunc-nin "Copy/link to this API element")

@property

```python
nin(self: Self, Self: type)
```

nout[](https://docs.exaloop.io/libraries/api/modules/numpy/ufunc/#class-unaryufunc-nout "Copy/link to this API element")

@property

```python
nout(self: Self, Self: type)
```

Methods 1

at[](https://docs.exaloop.io/libraries/api/modules/numpy/ufunc/#class-unaryufunc-at "Copy/link to this API element")

```python
at(self: Self, a, indices, Self: type)
```

Special methods 1

\_\_call\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/ufunc/#class-unaryufunc-__call__ "Copy/link to this API element")

```python
__call__(self: Self, x, out = None, where = True, dtype: type = NoneType, Self: type)
```

class UnaryUFunc2[](https://docs.exaloop.io/libraries/api/modules/numpy/ufunc/#class-unaryufunc2 "Copy/link to this API element")

@tuple

```python
class UnaryUFunc2[__name__, F]
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/numpy/ufunc/UnaryUFunc2/)

Fields 1

`F` `type`

Constructors 1

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/ufunc/#class-unaryufunc2-__new__ "Copy/link to this API element")

```python
__new__(op: F, name: Literal[str], F: type)
```

Properties 2

nin[](https://docs.exaloop.io/libraries/api/modules/numpy/ufunc/#class-unaryufunc2-nin "Copy/link to this API element")

@property

```python
nin(self: Self, Self: type)
```

nout[](https://docs.exaloop.io/libraries/api/modules/numpy/ufunc/#class-unaryufunc2-nout "Copy/link to this API element")

@property

```python
nout(self: Self, Self: type)
```

Special methods 1

\_\_call\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/ufunc/#class-unaryufunc2-__call__ "Copy/link to this API element")

```python
__call__(self: Self, x, out1 = None, out2 = None, out = None, where = True, Self: type)
```

class BinaryUFunc[](https://docs.exaloop.io/libraries/api/modules/numpy/ufunc/#class-binaryufunc "Copy/link to this API element")

@tuple

```python
class BinaryUFunc[__name__, F, I]
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/numpy/ufunc/BinaryUFunc/)

Fields 3

`identity` `I`

`F` `type`

`I` `type`

Constructors 1

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/ufunc/#class-binaryufunc-__new__ "Copy/link to this API element")

```python
__new__(op: F, name: Literal[str], identity: I = None, F: type, I: type)
```

Properties 2

nin[](https://docs.exaloop.io/libraries/api/modules/numpy/ufunc/#class-binaryufunc-nin "Copy/link to this API element")

@property

```python
nin(self: Self, Self: type)
```

nout[](https://docs.exaloop.io/libraries/api/modules/numpy/ufunc/#class-binaryufunc-nout "Copy/link to this API element")

@property

```python
nout(self: Self, Self: type)
```

Methods 4

reduce[](https://docs.exaloop.io/libraries/api/modules/numpy/ufunc/#class-binaryufunc-reduce "Copy/link to this API element")

```python
reduce(self: Self, array, axis = 0, dtype: type = NoneType, out = None, keepdims: Literal[bool] = False, initial = util._NoValue(), where = True, Self: type)
```

accumulate[](https://docs.exaloop.io/libraries/api/modules/numpy/ufunc/#class-binaryufunc-accumulate "Copy/link to this API element")

```python
accumulate(self: Self, array, axis: int = 0, dtype: type = NoneType, out = None, Self: type)
```

at[](https://docs.exaloop.io/libraries/api/modules/numpy/ufunc/#class-binaryufunc-at "Copy/link to this API element")

```python
at(self: Self, a, indices, b, Self: type)
```

outer[](https://docs.exaloop.io/libraries/api/modules/numpy/ufunc/#class-binaryufunc-outer "Copy/link to this API element")

```python
outer(self: Self, A, B, dtype: type = NoneType, Self: type)
```

Special methods 1

\_\_call\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/ufunc/#class-binaryufunc-__call__ "Copy/link to this API element")

```python
__call__(self: Self, x, y, out = None, where = True, dtype: type = NoneType, Self: type)
```

Source: https://docs.exaloop.io/libraries/api/modules/numpy/ufunc/
