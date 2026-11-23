# numpy.ufunc

Standard library Module [View source](https://github.com/exaloop/codon/blob/master/stdlib/numpy/ufunc.codon)

function decide\_types[](https://docs.exaloop.io/libraries/api/modules/numpy/ufunc/#function-decide_types "Copy/link to this API element")

```python
decide_types[dtype: type](x, y)
```

function decide\_types\_copysign[](https://docs.exaloop.io/libraries/api/modules/numpy/ufunc/#function-decide_types_copysign "Copy/link to this API element")

```python
decide_types_copysign[dtype: type](x, y)
```

function decide\_types\_ldexp[](https://docs.exaloop.io/libraries/api/modules/numpy/ufunc/#function-decide_types_ldexp "Copy/link to this API element")

```python
decide_types_ldexp[dtype: type](x, y)
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
__new__[name: Literal[str], F: type](op: F) -> UnaryUFunc[(name, F)]
```

Properties 2

nin[](https://docs.exaloop.io/libraries/api/modules/numpy/ufunc/#class-unaryufunc-nin "Copy/link to this API element")

@property

```python
nin(self: Self)
```

nout[](https://docs.exaloop.io/libraries/api/modules/numpy/ufunc/#class-unaryufunc-nout "Copy/link to this API element")

@property

```python
nout(self: Self)
```

Methods 1

at[](https://docs.exaloop.io/libraries/api/modules/numpy/ufunc/#class-unaryufunc-at "Copy/link to this API element")

```python
at(self: Self, a, indices)
```

Special methods 1

\_\_call\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/ufunc/#class-unaryufunc-__call__ "Copy/link to this API element")

```python
__call__[dtype: type = NoneType](self: Self, x, out = None, where = True)
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
__new__[name: Literal[str], F: type](op: F) -> UnaryUFunc2[(name, F)]
```

Properties 2

nin[](https://docs.exaloop.io/libraries/api/modules/numpy/ufunc/#class-unaryufunc2-nin "Copy/link to this API element")

@property

```python
nin(self: Self)
```

nout[](https://docs.exaloop.io/libraries/api/modules/numpy/ufunc/#class-unaryufunc2-nout "Copy/link to this API element")

@property

```python
nout(self: Self)
```

Special methods 1

\_\_call\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/ufunc/#class-unaryufunc2-__call__ "Copy/link to this API element")

```python
__call__(self: Self, x, out1 = None, out2 = None, out = None, where = True)
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
__new__[name: Literal[str], F: type, I: type](op: F, identity: I = None) -> BinaryUFunc[(name, F, I)]
```

Properties 2

nin[](https://docs.exaloop.io/libraries/api/modules/numpy/ufunc/#class-binaryufunc-nin "Copy/link to this API element")

@property

```python
nin(self: Self)
```

nout[](https://docs.exaloop.io/libraries/api/modules/numpy/ufunc/#class-binaryufunc-nout "Copy/link to this API element")

@property

```python
nout(self: Self)
```

Methods 4

reduce[](https://docs.exaloop.io/libraries/api/modules/numpy/ufunc/#class-binaryufunc-reduce "Copy/link to this API element")

```python
reduce[dtype: type = NoneType, keepdims: Literal[bool] = False](self: Self, array, axis = 0, out = None, initial = util._NoValue(), where = True)
```

accumulate[](https://docs.exaloop.io/libraries/api/modules/numpy/ufunc/#class-binaryufunc-accumulate "Copy/link to this API element")

```python
accumulate[dtype: type = NoneType](self: Self, array, axis: int = 0, out = None)
```

at[](https://docs.exaloop.io/libraries/api/modules/numpy/ufunc/#class-binaryufunc-at "Copy/link to this API element")

```python
at(self: Self, a, indices, b)
```

outer[](https://docs.exaloop.io/libraries/api/modules/numpy/ufunc/#class-binaryufunc-outer "Copy/link to this API element")

```python
outer[dtype: type = NoneType](self: Self, A, B)
```

Special methods 1

\_\_call\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/ufunc/#class-binaryufunc-__call__ "Copy/link to this API element")

```python
__call__[dtype: type = NoneType](self: Self, x, y, out = None, where = True)
```

Source: https://docs.exaloop.io/libraries/api/modules/numpy/ufunc/
