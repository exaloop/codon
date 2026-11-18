# internal.types.complex

Standard library Module [View source](https://github.com/exaloop/codon/blob/master/stdlib/internal/types/complex.codon)

class complex64[](https://docs.exaloop.io/libraries/api/modules/internal/types/complex/#class-complex64 "Copy/link to this API element")

@tuple

```python
class complex64
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/internal/types/complex/complex64/)

Fields 2

`real` `float32`

`imag` `float32`

class complex[](https://docs.exaloop.io/libraries/api/modules/internal/types/complex/#class-complex "Copy/link to this API element")

@tuple

```python
class complex
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/internal/types/complex/complex/)

Fields 2

`real` `float`

`imag` `float`

Constructors 3

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/complex/#class-complex-__new__ "Copy/link to this API element")

```python
__new__() -> complex
```

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/complex/#class-complex-__new__-overload-2 "Copy/link to this API element")

@overload

```python
__new__(what)
```

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/complex/#class-complex-__new__-overload-3 "Copy/link to this API element")

@overload

```python
__new__(real, imag) -> complex
```

Methods 2

cdiv[](https://docs.exaloop.io/libraries/api/modules/internal/types/complex/#class-complex-cdiv "Copy/link to this API element")

```python
cdiv(self: Self, other: complex) -> complex
```

conjugate[](https://docs.exaloop.io/libraries/api/modules/internal/types/complex/#class-complex-conjugate "Copy/link to this API element")

```python
conjugate(self: Self, /) -> complex
```

Special methods 20

\_\_complex\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/complex/#class-complex-__complex__ "Copy/link to this API element")

```python
__complex__(self: Self, /) -> complex
```

\_\_bool\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/complex/#class-complex-__bool__ "Copy/link to this API element")

```python
__bool__(self: Self, /) -> bool
```

\_\_pos\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/complex/#class-complex-__pos__ "Copy/link to this API element")

```python
__pos__(self: Self, /) -> complex
```

\_\_neg\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/complex/#class-complex-__neg__ "Copy/link to this API element")

```python
__neg__(self: Self, /) -> complex
```

\_\_abs\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/complex/#class-complex-__abs__ "Copy/link to this API element")

```python
__abs__(self: Self, /) -> float
```

\_\_copy\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/complex/#class-complex-__copy__ "Copy/link to this API element")

```python
__copy__(self: Self) -> complex
```

\_\_hash\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/complex/#class-complex-__hash__ "Copy/link to this API element")

```python
__hash__(self: Self, /) -> int
```

\_\_add\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/complex/#class-complex-__add__ "Copy/link to this API element")

```python
__add__(self: Self, other: float, /) -> complex
```

```python
__add__(self: Self, other: int, /) -> complex
```

```python
__add__(self: Self, other: complex, /) -> complex
```

\_\_sub\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/complex/#class-complex-__sub__ "Copy/link to this API element")

```python
__sub__(self: Self, other: float, /) -> complex
```

```python
__sub__(self: Self, other: int, /) -> complex
```

```python
__sub__(self: Self, other: complex, /) -> complex
```

\_\_mul\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/complex/#class-complex-__mul__ "Copy/link to this API element")

```python
__mul__(self: Self, other: float, /) -> complex
```

```python
__mul__(self: Self, other: int, /) -> complex
```

```python
__mul__(self: Self, other: complex, /) -> complex
```

\_\_truediv\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/complex/#class-complex-__truediv__ "Copy/link to this API element")

```python
__truediv__(self: Self, other: float, /) -> complex
```

```python
__truediv__(self: Self, other: int, /) -> complex
```

```python
__truediv__(self: Self, other: complex, /) -> complex
```

\_\_eq\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/complex/#class-complex-__eq__ "Copy/link to this API element")

```python
__eq__(self: Self, other: float, /) -> bool
```

```python
__eq__(self: Self, other: int, /) -> bool
```

```python
__eq__(self: Self, other: complex, /) -> bool
```

\_\_ne\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/complex/#class-complex-__ne__ "Copy/link to this API element")

```python
__ne__(self: Self, other: float, /) -> bool
```

```python
__ne__(self: Self, other: int, /) -> bool
```

```python
__ne__(self: Self, other: complex, /) -> bool
```

\_\_pow\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/complex/#class-complex-__pow__ "Copy/link to this API element")

```python
__pow__(self: Self, other: float, /) -> complex
```

```python
__pow__(self: Self, other: int, /) -> complex
```

```python
__pow__(self: Self, other: complex, /) -> complex
```

\_\_radd\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/complex/#class-complex-__radd__ "Copy/link to this API element")

```python
__radd__(self: Self, other: float, /) -> complex
```

```python
__radd__(self: Self, other: int, /) -> complex
```

\_\_rsub\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/complex/#class-complex-__rsub__ "Copy/link to this API element")

```python
__rsub__(self: Self, other: float, /) -> complex
```

```python
__rsub__(self: Self, other: int, /) -> complex
```

\_\_rmul\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/complex/#class-complex-__rmul__ "Copy/link to this API element")

```python
__rmul__(self: Self, other: float, /) -> complex
```

```python
__rmul__(self: Self, other: int, /) -> complex
```

\_\_rtruediv\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/complex/#class-complex-__rtruediv__ "Copy/link to this API element")

```python
__rtruediv__(self: Self, other: float, /) -> complex
```

```python
__rtruediv__(self: Self, other: int, /) -> complex
```

\_\_rpow\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/complex/#class-complex-__rpow__ "Copy/link to this API element")

```python
__rpow__(self: Self, other: float, /) -> complex
```

```python
__rpow__(self: Self, other: int, /) -> complex
```

\_\_repr\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/complex/#class-complex-__repr__ "Copy/link to this API element")

```python
__repr__(self: Self, /) -> str
```

class Int[](https://docs.exaloop.io/libraries/api/modules/internal/types/complex/#class-int "Copy/link to this API element")

@extend

```python
class Int[N]
```

Extends [`internal.core.Int`](https://docs.exaloop.io/libraries/api/classes/internal/core/Int/) · [View complete class API →](https://docs.exaloop.io/libraries/api/classes/internal/core/Int/)

Special methods 1

\_\_suffix\_j\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/complex/#class-int-__suffix_j__ "Copy/link to this API element")

```python
__suffix_j__(x: int) -> complex
```

class float[](https://docs.exaloop.io/libraries/api/modules/internal/types/complex/#class-float "Copy/link to this API element")

@extend

```python
class float
```

Extends [`internal.core.float`](https://docs.exaloop.io/libraries/api/classes/internal/core/float/) · [View complete class API →](https://docs.exaloop.io/libraries/api/classes/internal/core/float/)

Special methods 1

\_\_suffix\_j\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/complex/#class-float-__suffix_j__ "Copy/link to this API element")

```python
__suffix_j__(x: float) -> complex
```

alias f32[](https://docs.exaloop.io/libraries/api/modules/internal/types/complex/#alias-f32 "Copy/link to this API element")

```python
f32 = float32
```

Alias of [`float32`](https://docs.exaloop.io/libraries/api/classes/internal/core/float32/)

class complex64[](https://docs.exaloop.io/libraries/api/modules/internal/types/complex/#class-complex64 "Copy/link to this API element")

@extend

```python
class complex64
```

Extends [`internal.types.complex.complex64`](https://docs.exaloop.io/libraries/api/classes/internal/types/complex/complex64/) · [View complete class API →](https://docs.exaloop.io/libraries/api/classes/internal/types/complex/complex64/)

Constructors 5

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/complex/#class-complex64-__new__ "Copy/link to this API element")

```python
__new__() -> complex64
```

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/complex/#class-complex64-__new__-overload-2 "Copy/link to this API element")

```python
__new__(other)
```

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/complex/#class-complex64-__new__-overload-3 "Copy/link to this API element")

```python
__new__(real: f32)
```

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/complex/#class-complex64-__new__-overload-4 "Copy/link to this API element")

```python
__new__(other: complex) -> complex64
```

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/complex/#class-complex64-__new__-overload-5 "Copy/link to this API element")

```python
__new__(real, imag) -> complex64
```

Methods 2

cdiv[](https://docs.exaloop.io/libraries/api/modules/internal/types/complex/#class-complex64-cdiv "Copy/link to this API element")

```python
cdiv(self: Self, other: complex64) -> complex64
```

conjugate[](https://docs.exaloop.io/libraries/api/modules/internal/types/complex/#class-complex64-conjugate "Copy/link to this API element")

```python
conjugate(self: Self) -> complex64
```

Special methods 21

\_\_complex\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/complex/#class-complex64-__complex__ "Copy/link to this API element")

```python
__complex__(self: Self) -> complex
```

\_\_bool\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/complex/#class-complex64-__bool__ "Copy/link to this API element")

```python
__bool__(self: Self) -> bool
```

\_\_pos\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/complex/#class-complex64-__pos__ "Copy/link to this API element")

```python
__pos__(self: Self) -> complex64
```

\_\_neg\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/complex/#class-complex64-__neg__ "Copy/link to this API element")

```python
__neg__(self: Self) -> complex64
```

\_\_abs\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/complex/#class-complex64-__abs__ "Copy/link to this API element")

```python
__abs__(self: Self) -> f32
```

\_\_copy\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/complex/#class-complex64-__copy__ "Copy/link to this API element")

```python
__copy__(self: Self) -> complex64
```

\_\_hash\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/complex/#class-complex64-__hash__ "Copy/link to this API element")

```python
__hash__(self: Self) -> int
```

\_\_add\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/complex/#class-complex64-__add__ "Copy/link to this API element")

```python
__add__(self: Self, other: complex) -> complex
```

```python
__add__(self: Self, other: float32) -> complex64
```

```python
__add__(self: Self, other: float) -> complex
```

```python
__add__(self: Self, other: int) -> complex
```

```python
__add__(self: Self, other: complex64) -> complex64
```

\_\_sub\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/complex/#class-complex64-__sub__ "Copy/link to this API element")

```python
__sub__(self: Self, other: complex) -> complex
```

```python
__sub__(self: Self, other: float32) -> complex64
```

```python
__sub__(self: Self, other: float) -> complex
```

```python
__sub__(self: Self, other: int) -> complex
```

```python
__sub__(self: Self, other: complex64) -> complex64
```

\_\_mul\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/complex/#class-complex64-__mul__ "Copy/link to this API element")

```python
__mul__(self: Self, other: complex) -> complex
```

```python
__mul__(self: Self, other: float32) -> complex64
```

```python
__mul__(self: Self, other: float) -> complex
```

```python
__mul__(self: Self, other: int) -> complex
```

```python
__mul__(self: Self, other: complex64) -> complex64
```

\_\_truediv\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/complex/#class-complex64-__truediv__ "Copy/link to this API element")

```python
__truediv__(self: Self, other: complex) -> complex
```

```python
__truediv__(self: Self, other: float32) -> complex64
```

```python
__truediv__(self: Self, other: float) -> complex
```

```python
__truediv__(self: Self, other: int) -> complex
```

```python
__truediv__(self: Self, other: complex64) -> complex64
```

\_\_pow\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/complex/#class-complex64-__pow__ "Copy/link to this API element")

```python
__pow__(self: Self, other: complex) -> complex
```

```python
__pow__(self: Self, other: float32) -> complex64
```

```python
__pow__(self: Self, other: float) -> complex
```

```python
__pow__(self: Self, other: int) -> complex64
```

```python
__pow__(self: Self, other: complex64) -> complex64
```

\_\_eq\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/complex/#class-complex64-__eq__ "Copy/link to this API element")

```python
__eq__(self: Self, other: complex) -> bool
```

```python
__eq__(self: Self, other: float32) -> bool
```

```python
__eq__(self: Self, other: float) -> bool
```

```python
__eq__(self: Self, other: int) -> bool
```

```python
__eq__(self: Self, other: complex64) -> bool
```

\_\_ne\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/complex/#class-complex64-__ne__ "Copy/link to this API element")

```python
__ne__(self: Self, other: complex) -> bool
```

```python
__ne__(self: Self, other: float32) -> bool
```

```python
__ne__(self: Self, other: float) -> bool
```

```python
__ne__(self: Self, other: int) -> bool
```

```python
__ne__(self: Self, other: complex64) -> bool
```

\_\_radd\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/complex/#class-complex64-__radd__ "Copy/link to this API element")

```python
__radd__(self: Self, other: complex) -> complex
```

```python
__radd__(self: Self, other: float32) -> complex64
```

```python
__radd__(self: Self, other: float) -> complex
```

```python
__radd__(self: Self, other: int) -> complex
```

\_\_rsub\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/complex/#class-complex64-__rsub__ "Copy/link to this API element")

```python
__rsub__(self: Self, other: complex) -> complex
```

```python
__rsub__(self: Self, other: float32) -> complex64
```

```python
__rsub__(self: Self, other: float) -> complex
```

```python
__rsub__(self: Self, other: int) -> complex
```

\_\_rmul\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/complex/#class-complex64-__rmul__ "Copy/link to this API element")

```python
__rmul__(self: Self, other: complex) -> complex
```

```python
__rmul__(self: Self, other: float32) -> complex64
```

```python
__rmul__(self: Self, other: float) -> complex
```

```python
__rmul__(self: Self, other: int) -> complex
```

\_\_rtruediv\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/complex/#class-complex64-__rtruediv__ "Copy/link to this API element")

```python
__rtruediv__(self: Self, other: complex) -> complex
```

```python
__rtruediv__(self: Self, other: float32) -> complex64
```

```python
__rtruediv__(self: Self, other: float) -> complex
```

```python
__rtruediv__(self: Self, other: int) -> complex
```

\_\_rpow\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/complex/#class-complex64-__rpow__ "Copy/link to this API element")

```python
__rpow__(self: Self, other: complex) -> complex
```

```python
__rpow__(self: Self, other: float32) -> complex64
```

```python
__rpow__(self: Self, other: float) -> complex
```

```python
__rpow__(self: Self, other: int) -> complex
```

\_\_repr\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/complex/#class-complex64-__repr__ "Copy/link to this API element")

```python
__repr__(self: Self) -> str
```

\_\_str\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/complex/#class-complex64-__str__ "Copy/link to this API element")

```python
__str__(self: Self) -> str
```

class Int[](https://docs.exaloop.io/libraries/api/modules/internal/types/complex/#class-int "Copy/link to this API element")

@extend

```python
class Int[N]
```

Extends [`internal.core.Int`](https://docs.exaloop.io/libraries/api/classes/internal/core/Int/) · [View complete class API →](https://docs.exaloop.io/libraries/api/classes/internal/core/Int/)

Special methods 1

\_\_complex\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/complex/#class-int-__complex__ "Copy/link to this API element")

```python
__complex__(self: Self) -> complex
```

class UInt[](https://docs.exaloop.io/libraries/api/modules/internal/types/complex/#class-uint "Copy/link to this API element")

@extend

```python
class UInt[N]
```

Extends [`internal.core.UInt`](https://docs.exaloop.io/libraries/api/classes/internal/core/UInt/) · [View complete class API →](https://docs.exaloop.io/libraries/api/classes/internal/core/UInt/)

Special methods 1

\_\_complex\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/complex/#class-uint-__complex__ "Copy/link to this API element")

```python
__complex__(self: Self) -> complex
```

class float[](https://docs.exaloop.io/libraries/api/modules/internal/types/complex/#class-float "Copy/link to this API element")

@extend

```python
class float
```

Extends [`internal.core.float`](https://docs.exaloop.io/libraries/api/classes/internal/core/float/) · [View complete class API →](https://docs.exaloop.io/libraries/api/classes/internal/core/float/)

Special methods 1

\_\_complex\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/complex/#class-float-__complex__ "Copy/link to this API element")

```python
__complex__(self: Self) -> complex
```

class float128[](https://docs.exaloop.io/libraries/api/modules/internal/types/complex/#class-float128 "Copy/link to this API element")

@extend

```python
class float128
```

Extends [`internal.core.float128`](https://docs.exaloop.io/libraries/api/classes/internal/core/float128/) · [View complete class API →](https://docs.exaloop.io/libraries/api/classes/internal/core/float128/)

Special methods 1

\_\_complex\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/complex/#class-float128-__complex__ "Copy/link to this API element")

```python
__complex__(self: Self) -> complex
```

class float32[](https://docs.exaloop.io/libraries/api/modules/internal/types/complex/#class-float32 "Copy/link to this API element")

@extend

```python
class float32
```

Extends [`internal.core.float32`](https://docs.exaloop.io/libraries/api/classes/internal/core/float32/) · [View complete class API →](https://docs.exaloop.io/libraries/api/classes/internal/core/float32/)

Special methods 1

\_\_complex\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/complex/#class-float32-__complex__ "Copy/link to this API element")

```python
__complex__(self: Self) -> complex
```

class float16[](https://docs.exaloop.io/libraries/api/modules/internal/types/complex/#class-float16 "Copy/link to this API element")

@extend

```python
class float16
```

Extends [`internal.core.float16`](https://docs.exaloop.io/libraries/api/classes/internal/core/float16/) · [View complete class API →](https://docs.exaloop.io/libraries/api/classes/internal/core/float16/)

Special methods 1

\_\_complex\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/complex/#class-float16-__complex__ "Copy/link to this API element")

```python
__complex__(self: Self) -> complex
```

class bfloat16[](https://docs.exaloop.io/libraries/api/modules/internal/types/complex/#class-bfloat16 "Copy/link to this API element")

@extend

```python
class bfloat16
```

Extends [`internal.core.bfloat16`](https://docs.exaloop.io/libraries/api/classes/internal/core/bfloat16/) · [View complete class API →](https://docs.exaloop.io/libraries/api/classes/internal/core/bfloat16/)

Special methods 1

\_\_complex\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/complex/#class-bfloat16-__complex__ "Copy/link to this API element")

```python
__complex__(self: Self) -> complex
```

Source: https://docs.exaloop.io/libraries/api/modules/internal/types/complex/
