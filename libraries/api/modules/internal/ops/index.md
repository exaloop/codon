# internal.ops

Standard library Module [View source](https://github.com/exaloop/codon/blob/master/stdlib/internal/ops.codon)

class Int[](https://docs.exaloop.io/libraries/api/modules/internal/ops/#class-int "Copy/link to this API element")

@extend

```python
class Int[N]
```

Extends [`internal.core.Int`](https://docs.exaloop.io/libraries/api/classes/internal/core/Int/) · [View complete class API →](https://docs.exaloop.io/libraries/api/classes/internal/core/Int/)

Constructors 7

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/ops/#class-int-__new__ "Copy/link to this API element")

```python
__new__(what, /) -> Int[N]
```

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/ops/#class-int-__new__-overload-2 "Copy/link to this API element")

```python
__new__(what: bool, /) -> Int[N]
```

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/ops/#class-int-__new__-overload-3 "Copy/link to this API element")

```python
__new__(what: float, /) -> Int[N]
```

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/ops/#class-int-__new__-overload-4 "Copy/link to this API element")

```python
__new__(what: float128, /) -> Int[N]
```

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/ops/#class-int-__new__-overload-5 "Copy/link to this API element")

```python
__new__(what: float32, /) -> Int[N]
```

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/ops/#class-int-__new__-overload-6 "Copy/link to this API element")

```python
__new__(what: float16, /) -> Int[N]
```

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/ops/#class-int-__new__-overload-7 "Copy/link to this API element")

```python
__new__(what: bfloat16, /) -> Int[N]
```

class UInt[](https://docs.exaloop.io/libraries/api/modules/internal/ops/#class-uint "Copy/link to this API element")

@extend

```python
class UInt[N]
```

Extends [`internal.core.UInt`](https://docs.exaloop.io/libraries/api/classes/internal/core/UInt/) · [View complete class API →](https://docs.exaloop.io/libraries/api/classes/internal/core/UInt/)

Constructors 7

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/ops/#class-uint-__new__ "Copy/link to this API element")

```python
__new__(what) -> UInt[N]
```

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/ops/#class-uint-__new__-overload-2 "Copy/link to this API element")

```python
__new__(what: bool) -> UInt[N]
```

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/ops/#class-uint-__new__-overload-3 "Copy/link to this API element")

```python
__new__(what: float) -> UInt[N]
```

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/ops/#class-uint-__new__-overload-4 "Copy/link to this API element")

```python
__new__(what: float128) -> UInt[N]
```

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/ops/#class-uint-__new__-overload-5 "Copy/link to this API element")

```python
__new__(what: float32) -> UInt[N]
```

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/ops/#class-uint-__new__-overload-6 "Copy/link to this API element")

```python
__new__(what: float16) -> UInt[N]
```

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/ops/#class-uint-__new__-overload-7 "Copy/link to this API element")

```python
__new__(what: bfloat16) -> UInt[N]
```

class float[](https://docs.exaloop.io/libraries/api/modules/internal/ops/#class-float "Copy/link to this API element")

@extend

```python
class float
```

Extends [`internal.core.float`](https://docs.exaloop.io/libraries/api/classes/internal/core/float/) · [View complete class API →](https://docs.exaloop.io/libraries/api/classes/internal/core/float/)

Constructors 7

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/ops/#class-float-__new__ "Copy/link to this API element")

```python
__new__(what, /) -> float
```

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/ops/#class-float-__new__-overload-2 "Copy/link to this API element")

```python
__new__(what: bool, /) -> float
```

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/ops/#class-float-__new__-overload-3 "Copy/link to this API element")

@pure @llvm

```python
__new__[N: Literal[int]](what: Int[N], /) -> float
```

```llvm
%0 = sitofp i{=N} %what to double
ret double %0
```

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/ops/#class-float-__new__-overload-4 "Copy/link to this API element")

@pure @llvm

```python
__new__[N: Literal[int]](what: UInt[N], /) -> float
```

```llvm
%0 = uitofp i{=N} %what to double
ret double %0
```

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/ops/#class-float-__new__-overload-5 "Copy/link to this API element")

@pure @llvm

```python
__new__(what: float32, /) -> float
```

```llvm
%0 = fpext float %what to double
ret double %0
```

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/ops/#class-float-__new__-overload-6 "Copy/link to this API element")

@pure @llvm

```python
__new__(what: float16, /) -> float
```

```llvm
%0 = fpext half %what to double
ret double %0
```

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/ops/#class-float-__new__-overload-7 "Copy/link to this API element")

@pure @llvm

```python
__new__(what: bfloat16, /) -> float
```

```llvm
%0 = fpext bfloat %what to double
ret double %0
```

class float128[](https://docs.exaloop.io/libraries/api/modules/internal/ops/#class-float128 "Copy/link to this API element")

@extend

```python
class float128
```

Extends [`internal.core.float128`](https://docs.exaloop.io/libraries/api/classes/internal/core/float128/) · [View complete class API →](https://docs.exaloop.io/libraries/api/classes/internal/core/float128/)

Constructors 7

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/ops/#class-float128-__new__ "Copy/link to this API element")

```python
__new__(what) -> float128
```

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/ops/#class-float128-__new__-overload-2 "Copy/link to this API element")

```python
__new__(what: bool) -> float128
```

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/ops/#class-float128-__new__-overload-3 "Copy/link to this API element")

@pure @llvm

```python
__new__[N: Literal[int]](what: Int[N]) -> float128
```

```llvm
%0 = sitofp i{=N} %what to fp128
ret fp128 %0
```

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/ops/#class-float128-__new__-overload-4 "Copy/link to this API element")

@pure @llvm

```python
__new__[N: Literal[int]](what: UInt[N]) -> float128
```

```llvm
%0 = uitofp i{=N} %what to fp128
ret fp128 %0
```

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/ops/#class-float128-__new__-overload-5 "Copy/link to this API element")

@pure @llvm

```python
__new__(what: float32) -> float128
```

```llvm
%0 = fpext float %what to fp128
ret fp128 %0
```

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/ops/#class-float128-__new__-overload-6 "Copy/link to this API element")

@pure @llvm

```python
__new__(what: float16) -> float128
```

```llvm
%0 = fpext half %what to fp128
ret fp128 %0
```

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/ops/#class-float128-__new__-overload-7 "Copy/link to this API element")

@pure @llvm

```python
__new__(what: bfloat16) -> float128
```

```llvm
%0 = fpext bfloat %what to fp128
ret fp128 %0
```

class float32[](https://docs.exaloop.io/libraries/api/modules/internal/ops/#class-float32 "Copy/link to this API element")

@extend

```python
class float32
```

Extends [`internal.core.float32`](https://docs.exaloop.io/libraries/api/classes/internal/core/float32/) · [View complete class API →](https://docs.exaloop.io/libraries/api/classes/internal/core/float32/)

Constructors 7

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/ops/#class-float32-__new__ "Copy/link to this API element")

```python
__new__(what) -> float32
```

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/ops/#class-float32-__new__-overload-2 "Copy/link to this API element")

```python
__new__(what: bool) -> float32
```

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/ops/#class-float32-__new__-overload-3 "Copy/link to this API element")

@pure @llvm

```python
__new__[N: Literal[int]](what: Int[N]) -> float32
```

```llvm
%0 = sitofp i{=N} %what to float
ret float %0
```

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/ops/#class-float32-__new__-overload-4 "Copy/link to this API element")

@pure @llvm

```python
__new__[N: Literal[int]](what: UInt[N]) -> float32
```

```llvm
%0 = uitofp i{=N} %what to float
ret float %0
```

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/ops/#class-float32-__new__-overload-5 "Copy/link to this API element")

@pure @llvm

```python
__new__(what: float128) -> float32
```

```llvm
%0 = fptrunc fp128 %what to float
ret float %0
```

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/ops/#class-float32-__new__-overload-6 "Copy/link to this API element")

@pure @llvm

```python
__new__(what: float16) -> float32
```

```llvm
%0 = fpext half %what to float
ret float %0
```

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/ops/#class-float32-__new__-overload-7 "Copy/link to this API element")

@pure @llvm

```python
__new__(what: bfloat16) -> float32
```

```llvm
%0 = fpext bfloat %what to float
ret float %0
```

class float16[](https://docs.exaloop.io/libraries/api/modules/internal/ops/#class-float16 "Copy/link to this API element")

@extend

```python
class float16
```

Extends [`internal.core.float16`](https://docs.exaloop.io/libraries/api/classes/internal/core/float16/) · [View complete class API →](https://docs.exaloop.io/libraries/api/classes/internal/core/float16/)

Constructors 7

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/ops/#class-float16-__new__ "Copy/link to this API element")

```python
__new__(what) -> float16
```

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/ops/#class-float16-__new__-overload-2 "Copy/link to this API element")

```python
__new__(what: bool) -> float16
```

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/ops/#class-float16-__new__-overload-3 "Copy/link to this API element")

@pure @llvm

```python
__new__[N: Literal[int]](what: Int[N]) -> float16
```

```llvm
%0 = sitofp i{=N} %what to half
ret half %0
```

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/ops/#class-float16-__new__-overload-4 "Copy/link to this API element")

@pure @llvm

```python
__new__[N: Literal[int]](what: UInt[N]) -> float16
```

```llvm
%0 = uitofp i{=N} %what to half
ret half %0
```

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/ops/#class-float16-__new__-overload-5 "Copy/link to this API element")

@pure @llvm

```python
__new__(what: float128) -> float16
```

```llvm
%0 = fptrunc fp128 %what to half
ret half %0
```

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/ops/#class-float16-__new__-overload-6 "Copy/link to this API element")

@pure @llvm

```python
__new__(what: float32) -> float16
```

```llvm
%0 = fptrunc float %what to half
ret half %0
```

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/ops/#class-float16-__new__-overload-7 "Copy/link to this API element")

@pure @llvm

```python
__new__(what: bfloat16) -> float16
```

```llvm
%0 = fpext bfloat %what to float
%1 = fptrunc float %0 to half
ret half %1
```

class bfloat16[](https://docs.exaloop.io/libraries/api/modules/internal/ops/#class-bfloat16 "Copy/link to this API element")

@extend

```python
class bfloat16
```

Extends [`internal.core.bfloat16`](https://docs.exaloop.io/libraries/api/classes/internal/core/bfloat16/) · [View complete class API →](https://docs.exaloop.io/libraries/api/classes/internal/core/bfloat16/)

Constructors 7

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/ops/#class-bfloat16-__new__ "Copy/link to this API element")

```python
__new__(what) -> bfloat16
```

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/ops/#class-bfloat16-__new__-overload-2 "Copy/link to this API element")

```python
__new__(what: bool) -> bfloat16
```

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/ops/#class-bfloat16-__new__-overload-3 "Copy/link to this API element")

@pure @llvm

```python
__new__[N: Literal[int]](what: Int[N]) -> bfloat16
```

```llvm
%0 = sitofp i{=N} %what to bfloat
ret bfloat %0
```

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/ops/#class-bfloat16-__new__-overload-4 "Copy/link to this API element")

@pure @llvm

```python
__new__[N: Literal[int]](what: UInt[N]) -> bfloat16
```

```llvm
%0 = uitofp i{=N} %what to bfloat
ret bfloat %0
```

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/ops/#class-bfloat16-__new__-overload-5 "Copy/link to this API element")

@pure @llvm

```python
__new__(what: float128) -> bfloat16
```

```llvm
%0 = fptrunc fp128 %what to bfloat
ret bfloat %0
```

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/ops/#class-bfloat16-__new__-overload-6 "Copy/link to this API element")

@pure @llvm

```python
__new__(what: float32) -> bfloat16
```

```llvm
%0 = fptrunc float %what to bfloat
ret bfloat %0
```

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/ops/#class-bfloat16-__new__-overload-7 "Copy/link to this API element")

@pure @llvm

```python
__new__(what: float16) -> bfloat16
```

```llvm
%0 = fpext float16 %what to float
%1 = fptrunc float %0 to bfloat
ret bfloat %1
```

class Int[](https://docs.exaloop.io/libraries/api/modules/internal/ops/#class-int "Copy/link to this API element")

@extend

```python
class Int[N]
```

Extends [`internal.core.Int`](https://docs.exaloop.io/libraries/api/classes/internal/core/Int/) · [View complete class API →](https://docs.exaloop.io/libraries/api/classes/internal/core/Int/)

Special methods 14

\_\_add\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/ops/#class-int-__add__ "Copy/link to this API element")

@commutative

```python
__add__(self: Self, b: float, /)
```

```python
__add__(self: Self, b: float128, /)
```

```python
__add__(self: Self, b: float32, /)
```

```python
__add__(self: Self, b: float16, /)
```

```python
__add__(self: Self, b: bfloat16, /)
```

```python
__add__(self: Self, b: Int, /)
```

```python
__add__(self: Self, b: UInt, /)
```

```python
__add__(self: Self, b: bool, /)
```

\_\_sub\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/ops/#class-int-__sub__ "Copy/link to this API element")

```python
__sub__(self: Self, b: float, /)
```

```python
__sub__(self: Self, b: float128, /)
```

```python
__sub__(self: Self, b: float32, /)
```

```python
__sub__(self: Self, b: float16, /)
```

```python
__sub__(self: Self, b: bfloat16, /)
```

```python
__sub__(self: Self, b: Int, /)
```

```python
__sub__(self: Self, b: UInt, /)
```

```python
__sub__(self: Self, b: bool, /)
```

\_\_mul\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/ops/#class-int-__mul__ "Copy/link to this API element")

@commutative

```python
__mul__(self: Self, b: float, /)
```

```python
__mul__(self: Self, b: float128, /)
```

```python
__mul__(self: Self, b: float32, /)
```

```python
__mul__(self: Self, b: float16, /)
```

```python
__mul__(self: Self, b: bfloat16, /)
```

```python
__mul__(self: Self, b: Int, /)
```

```python
__mul__(self: Self, b: UInt, /)
```

```python
__mul__(self: Self, b: bool, /)
```

\_\_floordiv\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/ops/#class-int-__floordiv__ "Copy/link to this API element")

```python
__floordiv__(self: Self, b: float, /)
```

```python
__floordiv__(self: Self, b: float128, /)
```

```python
__floordiv__(self: Self, b: float32, /)
```

```python
__floordiv__(self: Self, b: float16, /)
```

```python
__floordiv__(self: Self, b: bfloat16, /)
```

```python
__floordiv__(self: Self, b: Int, /)
```

```python
__floordiv__(self: Self, b: UInt, /)
```

```python
__floordiv__(self: Self, b: bool, /)
```

\_\_truediv\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/ops/#class-int-__truediv__ "Copy/link to this API element")

```python
__truediv__(self: Self, b: float, /)
```

```python
__truediv__(self: Self, b: float128, /)
```

```python
__truediv__(self: Self, b: float32, /)
```

```python
__truediv__(self: Self, b: float16, /)
```

```python
__truediv__(self: Self, b: bfloat16, /)
```

```python
__truediv__(self: Self, b: Int, /)
```

```python
__truediv__(self: Self, b: UInt, /)
```

```python
__truediv__(self: Self, b: bool, /)
```

\_\_mod\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/ops/#class-int-__mod__ "Copy/link to this API element")

```python
__mod__(self: Self, b: float, /)
```

```python
__mod__(self: Self, b: float128, /)
```

```python
__mod__(self: Self, b: float32, /)
```

```python
__mod__(self: Self, b: float16, /)
```

```python
__mod__(self: Self, b: bfloat16, /)
```

```python
__mod__(self: Self, b: Int, /)
```

```python
__mod__(self: Self, b: UInt, /)
```

```python
__mod__(self: Self, b: bool, /)
```

\_\_divmod\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/ops/#class-int-__divmod__ "Copy/link to this API element")

```python
__divmod__(self: Self, b: float, /)
```

```python
__divmod__(self: Self, b: float128, /)
```

```python
__divmod__(self: Self, b: float32, /)
```

```python
__divmod__(self: Self, b: float16, /)
```

```python
__divmod__(self: Self, b: bfloat16, /)
```

```python
__divmod__(self: Self, b: Int, /)
```

```python
__divmod__(self: Self, b: UInt, /)
```

```python
__divmod__(self: Self, b: bool, /)
```

\_\_pow\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/ops/#class-int-__pow__ "Copy/link to this API element")

```python
__pow__(self: Self, b: float, /)
```

```python
__pow__(self: Self, b: float128, /)
```

```python
__pow__(self: Self, b: float32, /)
```

```python
__pow__(self: Self, b: float16, /)
```

```python
__pow__(self: Self, b: bfloat16, /)
```

```python
__pow__(self: Self, b: Int, /)
```

```python
__pow__(self: Self, b: UInt, /)
```

```python
__pow__(self: Self, b: bool, /)
```

\_\_eq\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/ops/#class-int-__eq__ "Copy/link to this API element")

```python
__eq__(self: Self, b: float, /)
```

```python
__eq__(self: Self, b: float128, /)
```

```python
__eq__(self: Self, b: float32, /)
```

```python
__eq__(self: Self, b: float16, /)
```

```python
__eq__(self: Self, b: bfloat16, /)
```

```python
__eq__(self: Self, b: Int, /)
```

```python
__eq__(self: Self, b: UInt, /)
```

```python
__eq__(self: Self, b: bool, /)
```

\_\_ne\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/ops/#class-int-__ne__ "Copy/link to this API element")

```python
__ne__(self: Self, b: float, /)
```

```python
__ne__(self: Self, b: float128, /)
```

```python
__ne__(self: Self, b: float32, /)
```

```python
__ne__(self: Self, b: float16, /)
```

```python
__ne__(self: Self, b: bfloat16, /)
```

```python
__ne__(self: Self, b: Int, /)
```

```python
__ne__(self: Self, b: UInt, /)
```

```python
__ne__(self: Self, b: bool, /)
```

\_\_lt\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/ops/#class-int-__lt__ "Copy/link to this API element")

```python
__lt__(self: Self, b: float, /)
```

```python
__lt__(self: Self, b: float128, /)
```

```python
__lt__(self: Self, b: float32, /)
```

```python
__lt__(self: Self, b: float16, /)
```

```python
__lt__(self: Self, b: bfloat16, /)
```

```python
__lt__(self: Self, b: Int, /)
```

```python
__lt__(self: Self, b: UInt, /)
```

```python
__lt__(self: Self, b: bool, /)
```

\_\_gt\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/ops/#class-int-__gt__ "Copy/link to this API element")

```python
__gt__(self: Self, b: float, /)
```

```python
__gt__(self: Self, b: float128, /)
```

```python
__gt__(self: Self, b: float32, /)
```

```python
__gt__(self: Self, b: float16, /)
```

```python
__gt__(self: Self, b: bfloat16, /)
```

```python
__gt__(self: Self, b: Int, /)
```

```python
__gt__(self: Self, b: UInt, /)
```

```python
__gt__(self: Self, b: bool, /)
```

\_\_le\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/ops/#class-int-__le__ "Copy/link to this API element")

```python
__le__(self: Self, b: float, /)
```

```python
__le__(self: Self, b: float128, /)
```

```python
__le__(self: Self, b: float32, /)
```

```python
__le__(self: Self, b: float16, /)
```

```python
__le__(self: Self, b: bfloat16, /)
```

```python
__le__(self: Self, b: Int, /)
```

```python
__le__(self: Self, b: UInt, /)
```

```python
__le__(self: Self, b: bool, /)
```

\_\_ge\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/ops/#class-int-__ge__ "Copy/link to this API element")

```python
__ge__(self: Self, b: float, /)
```

```python
__ge__(self: Self, b: float128, /)
```

```python
__ge__(self: Self, b: float32, /)
```

```python
__ge__(self: Self, b: float16, /)
```

```python
__ge__(self: Self, b: bfloat16, /)
```

```python
__ge__(self: Self, b: Int, /)
```

```python
__ge__(self: Self, b: UInt, /)
```

```python
__ge__(self: Self, b: bool, /)
```

class UInt[](https://docs.exaloop.io/libraries/api/modules/internal/ops/#class-uint "Copy/link to this API element")

@extend

```python
class UInt[N]
```

Extends [`internal.core.UInt`](https://docs.exaloop.io/libraries/api/classes/internal/core/UInt/) · [View complete class API →](https://docs.exaloop.io/libraries/api/classes/internal/core/UInt/)

Special methods 14

\_\_add\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/ops/#class-uint-__add__ "Copy/link to this API element")

@commutative

```python
__add__(self: Self, b: float)
```

```python
__add__(self: Self, b: float128)
```

```python
__add__(self: Self, b: float32)
```

```python
__add__(self: Self, b: float16)
```

```python
__add__(self: Self, b: bfloat16)
```

```python
__add__(self: Self, b: Int)
```

```python
__add__(self: Self, b: UInt)
```

```python
__add__(self: Self, b: bool)
```

\_\_sub\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/ops/#class-uint-__sub__ "Copy/link to this API element")

```python
__sub__(self: Self, b: float)
```

```python
__sub__(self: Self, b: float128)
```

```python
__sub__(self: Self, b: float32)
```

```python
__sub__(self: Self, b: float16)
```

```python
__sub__(self: Self, b: bfloat16)
```

```python
__sub__(self: Self, b: Int)
```

```python
__sub__(self: Self, b: UInt)
```

```python
__sub__(self: Self, b: bool)
```

\_\_mul\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/ops/#class-uint-__mul__ "Copy/link to this API element")

@commutative

```python
__mul__(self: Self, b: float)
```

```python
__mul__(self: Self, b: float128)
```

```python
__mul__(self: Self, b: float32)
```

```python
__mul__(self: Self, b: float16)
```

```python
__mul__(self: Self, b: bfloat16)
```

```python
__mul__(self: Self, b: Int)
```

```python
__mul__(self: Self, b: UInt)
```

```python
__mul__(self: Self, b: bool)
```

\_\_floordiv\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/ops/#class-uint-__floordiv__ "Copy/link to this API element")

```python
__floordiv__(self: Self, b: float)
```

```python
__floordiv__(self: Self, b: float128)
```

```python
__floordiv__(self: Self, b: float32)
```

```python
__floordiv__(self: Self, b: float16)
```

```python
__floordiv__(self: Self, b: bfloat16)
```

```python
__floordiv__(self: Self, b: Int)
```

```python
__floordiv__(self: Self, b: UInt)
```

```python
__floordiv__(self: Self, b: bool)
```

\_\_truediv\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/ops/#class-uint-__truediv__ "Copy/link to this API element")

```python
__truediv__(self: Self, b: float)
```

```python
__truediv__(self: Self, b: float128)
```

```python
__truediv__(self: Self, b: float32)
```

```python
__truediv__(self: Self, b: float16)
```

```python
__truediv__(self: Self, b: bfloat16)
```

```python
__truediv__(self: Self, b: Int)
```

```python
__truediv__(self: Self, b: UInt)
```

```python
__truediv__(self: Self, b: bool)
```

\_\_mod\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/ops/#class-uint-__mod__ "Copy/link to this API element")

```python
__mod__(self: Self, b: float)
```

```python
__mod__(self: Self, b: float128)
```

```python
__mod__(self: Self, b: float32)
```

```python
__mod__(self: Self, b: float16)
```

```python
__mod__(self: Self, b: bfloat16)
```

```python
__mod__(self: Self, b: Int)
```

```python
__mod__(self: Self, b: UInt)
```

```python
__mod__(self: Self, b: bool)
```

\_\_divmod\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/ops/#class-uint-__divmod__ "Copy/link to this API element")

```python
__divmod__(self: Self, b: float)
```

```python
__divmod__(self: Self, b: float128)
```

```python
__divmod__(self: Self, b: float32)
```

```python
__divmod__(self: Self, b: float16)
```

```python
__divmod__(self: Self, b: bfloat16)
```

```python
__divmod__(self: Self, b: Int)
```

```python
__divmod__(self: Self, b: UInt)
```

```python
__divmod__(self: Self, b: bool)
```

\_\_pow\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/ops/#class-uint-__pow__ "Copy/link to this API element")

```python
__pow__(self: Self, b: float)
```

```python
__pow__(self: Self, b: float128)
```

```python
__pow__(self: Self, b: float32)
```

```python
__pow__(self: Self, b: float16)
```

```python
__pow__(self: Self, b: bfloat16)
```

```python
__pow__(self: Self, b: Int)
```

```python
__pow__(self: Self, b: UInt)
```

```python
__pow__(self: Self, b: bool)
```

\_\_eq\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/ops/#class-uint-__eq__ "Copy/link to this API element")

```python
__eq__(self: Self, b: float)
```

```python
__eq__(self: Self, b: float128)
```

```python
__eq__(self: Self, b: float32)
```

```python
__eq__(self: Self, b: float16)
```

```python
__eq__(self: Self, b: bfloat16)
```

```python
__eq__(self: Self, b: Int)
```

```python
__eq__(self: Self, b: UInt)
```

```python
__eq__(self: Self, b: bool)
```

\_\_ne\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/ops/#class-uint-__ne__ "Copy/link to this API element")

```python
__ne__(self: Self, b: float)
```

```python
__ne__(self: Self, b: float128)
```

```python
__ne__(self: Self, b: float32)
```

```python
__ne__(self: Self, b: float16)
```

```python
__ne__(self: Self, b: bfloat16)
```

```python
__ne__(self: Self, b: Int)
```

```python
__ne__(self: Self, b: UInt)
```

```python
__ne__(self: Self, b: bool)
```

\_\_lt\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/ops/#class-uint-__lt__ "Copy/link to this API element")

```python
__lt__(self: Self, b: float)
```

```python
__lt__(self: Self, b: float128)
```

```python
__lt__(self: Self, b: float32)
```

```python
__lt__(self: Self, b: float16)
```

```python
__lt__(self: Self, b: bfloat16)
```

```python
__lt__(self: Self, b: Int)
```

```python
__lt__(self: Self, b: UInt)
```

```python
__lt__(self: Self, b: bool)
```

\_\_gt\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/ops/#class-uint-__gt__ "Copy/link to this API element")

```python
__gt__(self: Self, b: float)
```

```python
__gt__(self: Self, b: float128)
```

```python
__gt__(self: Self, b: float32)
```

```python
__gt__(self: Self, b: float16)
```

```python
__gt__(self: Self, b: bfloat16)
```

```python
__gt__(self: Self, b: Int)
```

```python
__gt__(self: Self, b: UInt)
```

```python
__gt__(self: Self, b: bool)
```

\_\_le\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/ops/#class-uint-__le__ "Copy/link to this API element")

```python
__le__(self: Self, b: float)
```

```python
__le__(self: Self, b: float128)
```

```python
__le__(self: Self, b: float32)
```

```python
__le__(self: Self, b: float16)
```

```python
__le__(self: Self, b: bfloat16)
```

```python
__le__(self: Self, b: Int)
```

```python
__le__(self: Self, b: UInt)
```

```python
__le__(self: Self, b: bool)
```

\_\_ge\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/ops/#class-uint-__ge__ "Copy/link to this API element")

```python
__ge__(self: Self, b: float)
```

```python
__ge__(self: Self, b: float128)
```

```python
__ge__(self: Self, b: float32)
```

```python
__ge__(self: Self, b: float16)
```

```python
__ge__(self: Self, b: bfloat16)
```

```python
__ge__(self: Self, b: Int)
```

```python
__ge__(self: Self, b: UInt)
```

```python
__ge__(self: Self, b: bool)
```

class float[](https://docs.exaloop.io/libraries/api/modules/internal/ops/#class-float "Copy/link to this API element")

@extend

```python
class float
```

Extends [`internal.core.float`](https://docs.exaloop.io/libraries/api/classes/internal/core/float/) · [View complete class API →](https://docs.exaloop.io/libraries/api/classes/internal/core/float/)

Special methods 14

\_\_add\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/ops/#class-float-__add__ "Copy/link to this API element")

@commutative

```python
__add__(self: Self, b: float128, /)
```

```python
__add__(self: Self, b: float32, /)
```

```python
__add__(self: Self, b: float16, /)
```

```python
__add__(self: Self, b: bfloat16, /)
```

```python
__add__(self: Self, b: Int, /)
```

```python
__add__(self: Self, b: UInt, /)
```

```python
__add__(self: Self, b: bool, /)
```

\_\_sub\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/ops/#class-float-__sub__ "Copy/link to this API element")

```python
__sub__(self: Self, b: float128, /)
```

```python
__sub__(self: Self, b: float32, /)
```

```python
__sub__(self: Self, b: float16, /)
```

```python
__sub__(self: Self, b: bfloat16, /)
```

```python
__sub__(self: Self, b: Int, /)
```

```python
__sub__(self: Self, b: UInt, /)
```

```python
__sub__(self: Self, b: bool, /)
```

\_\_mul\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/ops/#class-float-__mul__ "Copy/link to this API element")

@commutative

```python
__mul__(self: Self, b: float128, /)
```

```python
__mul__(self: Self, b: float32, /)
```

```python
__mul__(self: Self, b: float16, /)
```

```python
__mul__(self: Self, b: bfloat16, /)
```

```python
__mul__(self: Self, b: Int, /)
```

```python
__mul__(self: Self, b: UInt, /)
```

```python
__mul__(self: Self, b: bool, /)
```

\_\_floordiv\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/ops/#class-float-__floordiv__ "Copy/link to this API element")

```python
__floordiv__(self: Self, b: float128, /)
```

```python
__floordiv__(self: Self, b: float32, /)
```

```python
__floordiv__(self: Self, b: float16, /)
```

```python
__floordiv__(self: Self, b: bfloat16, /)
```

```python
__floordiv__(self: Self, b: Int, /)
```

```python
__floordiv__(self: Self, b: UInt, /)
```

```python
__floordiv__(self: Self, b: bool, /)
```

\_\_truediv\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/ops/#class-float-__truediv__ "Copy/link to this API element")

```python
__truediv__(self: Self, b: float128, /)
```

```python
__truediv__(self: Self, b: float32, /)
```

```python
__truediv__(self: Self, b: float16, /)
```

```python
__truediv__(self: Self, b: bfloat16, /)
```

```python
__truediv__(self: Self, b: Int, /)
```

```python
__truediv__(self: Self, b: UInt, /)
```

```python
__truediv__(self: Self, b: bool, /)
```

\_\_mod\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/ops/#class-float-__mod__ "Copy/link to this API element")

```python
__mod__(self: Self, b: float128, /)
```

```python
__mod__(self: Self, b: float32, /)
```

```python
__mod__(self: Self, b: float16, /)
```

```python
__mod__(self: Self, b: bfloat16, /)
```

```python
__mod__(self: Self, b: Int, /)
```

```python
__mod__(self: Self, b: UInt, /)
```

```python
__mod__(self: Self, b: bool, /)
```

\_\_divmod\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/ops/#class-float-__divmod__ "Copy/link to this API element")

```python
__divmod__(self: Self, b: float128, /)
```

```python
__divmod__(self: Self, b: float32, /)
```

```python
__divmod__(self: Self, b: float16, /)
```

```python
__divmod__(self: Self, b: bfloat16, /)
```

```python
__divmod__(self: Self, b: Int, /)
```

```python
__divmod__(self: Self, b: UInt, /)
```

```python
__divmod__(self: Self, b: bool, /)
```

\_\_pow\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/ops/#class-float-__pow__ "Copy/link to this API element")

```python
__pow__(self: Self, b: float128, /)
```

```python
__pow__(self: Self, b: float32, /)
```

```python
__pow__(self: Self, b: float16, /)
```

```python
__pow__(self: Self, b: bfloat16, /)
```

```python
__pow__(self: Self, b: Int, /)
```

```python
__pow__(self: Self, b: UInt, /)
```

```python
__pow__(self: Self, b: bool, /)
```

\_\_eq\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/ops/#class-float-__eq__ "Copy/link to this API element")

```python
__eq__(self: Self, b: float128, /)
```

```python
__eq__(self: Self, b: float32, /)
```

```python
__eq__(self: Self, b: float16, /)
```

```python
__eq__(self: Self, b: bfloat16, /)
```

```python
__eq__(self: Self, b: Int, /)
```

```python
__eq__(self: Self, b: UInt, /)
```

```python
__eq__(self: Self, b: bool, /)
```

\_\_ne\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/ops/#class-float-__ne__ "Copy/link to this API element")

```python
__ne__(self: Self, b: float128, /)
```

```python
__ne__(self: Self, b: float32, /)
```

```python
__ne__(self: Self, b: float16, /)
```

```python
__ne__(self: Self, b: bfloat16, /)
```

```python
__ne__(self: Self, b: Int, /)
```

```python
__ne__(self: Self, b: UInt, /)
```

```python
__ne__(self: Self, b: bool, /)
```

\_\_lt\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/ops/#class-float-__lt__ "Copy/link to this API element")

```python
__lt__(self: Self, b: float128, /)
```

```python
__lt__(self: Self, b: float32, /)
```

```python
__lt__(self: Self, b: float16, /)
```

```python
__lt__(self: Self, b: bfloat16, /)
```

```python
__lt__(self: Self, b: Int, /)
```

```python
__lt__(self: Self, b: UInt, /)
```

```python
__lt__(self: Self, b: bool, /)
```

\_\_gt\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/ops/#class-float-__gt__ "Copy/link to this API element")

```python
__gt__(self: Self, b: float128, /)
```

```python
__gt__(self: Self, b: float32, /)
```

```python
__gt__(self: Self, b: float16, /)
```

```python
__gt__(self: Self, b: bfloat16, /)
```

```python
__gt__(self: Self, b: Int, /)
```

```python
__gt__(self: Self, b: UInt, /)
```

```python
__gt__(self: Self, b: bool, /)
```

\_\_le\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/ops/#class-float-__le__ "Copy/link to this API element")

```python
__le__(self: Self, b: float128, /)
```

```python
__le__(self: Self, b: float32, /)
```

```python
__le__(self: Self, b: float16, /)
```

```python
__le__(self: Self, b: bfloat16, /)
```

```python
__le__(self: Self, b: Int, /)
```

```python
__le__(self: Self, b: UInt, /)
```

```python
__le__(self: Self, b: bool, /)
```

\_\_ge\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/ops/#class-float-__ge__ "Copy/link to this API element")

```python
__ge__(self: Self, b: float128, /)
```

```python
__ge__(self: Self, b: float32, /)
```

```python
__ge__(self: Self, b: float16, /)
```

```python
__ge__(self: Self, b: bfloat16, /)
```

```python
__ge__(self: Self, b: Int, /)
```

```python
__ge__(self: Self, b: UInt, /)
```

```python
__ge__(self: Self, b: bool, /)
```

class float128[](https://docs.exaloop.io/libraries/api/modules/internal/ops/#class-float128 "Copy/link to this API element")

@extend

```python
class float128
```

Extends [`internal.core.float128`](https://docs.exaloop.io/libraries/api/classes/internal/core/float128/) · [View complete class API →](https://docs.exaloop.io/libraries/api/classes/internal/core/float128/)

Special methods 14

\_\_add\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/ops/#class-float128-__add__ "Copy/link to this API element")

@commutative

```python
__add__(self: Self, b: float)
```

```python
__add__(self: Self, b: float32)
```

```python
__add__(self: Self, b: float16)
```

```python
__add__(self: Self, b: bfloat16)
```

```python
__add__(self: Self, b: Int)
```

```python
__add__(self: Self, b: UInt)
```

```python
__add__(self: Self, b: bool)
```

\_\_sub\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/ops/#class-float128-__sub__ "Copy/link to this API element")

```python
__sub__(self: Self, b: float)
```

```python
__sub__(self: Self, b: float32)
```

```python
__sub__(self: Self, b: float16)
```

```python
__sub__(self: Self, b: bfloat16)
```

```python
__sub__(self: Self, b: Int)
```

```python
__sub__(self: Self, b: UInt)
```

```python
__sub__(self: Self, b: bool)
```

\_\_mul\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/ops/#class-float128-__mul__ "Copy/link to this API element")

@commutative

```python
__mul__(self: Self, b: float)
```

```python
__mul__(self: Self, b: float32)
```

```python
__mul__(self: Self, b: float16)
```

```python
__mul__(self: Self, b: bfloat16)
```

```python
__mul__(self: Self, b: Int)
```

```python
__mul__(self: Self, b: UInt)
```

```python
__mul__(self: Self, b: bool)
```

\_\_floordiv\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/ops/#class-float128-__floordiv__ "Copy/link to this API element")

```python
__floordiv__(self: Self, b: float)
```

```python
__floordiv__(self: Self, b: float32)
```

```python
__floordiv__(self: Self, b: float16)
```

```python
__floordiv__(self: Self, b: bfloat16)
```

```python
__floordiv__(self: Self, b: Int)
```

```python
__floordiv__(self: Self, b: UInt)
```

```python
__floordiv__(self: Self, b: bool)
```

\_\_truediv\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/ops/#class-float128-__truediv__ "Copy/link to this API element")

```python
__truediv__(self: Self, b: float)
```

```python
__truediv__(self: Self, b: float32)
```

```python
__truediv__(self: Self, b: float16)
```

```python
__truediv__(self: Self, b: bfloat16)
```

```python
__truediv__(self: Self, b: Int)
```

```python
__truediv__(self: Self, b: UInt)
```

```python
__truediv__(self: Self, b: bool)
```

\_\_mod\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/ops/#class-float128-__mod__ "Copy/link to this API element")

```python
__mod__(self: Self, b: float)
```

```python
__mod__(self: Self, b: float32)
```

```python
__mod__(self: Self, b: float16)
```

```python
__mod__(self: Self, b: bfloat16)
```

```python
__mod__(self: Self, b: Int)
```

```python
__mod__(self: Self, b: UInt)
```

```python
__mod__(self: Self, b: bool)
```

\_\_divmod\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/ops/#class-float128-__divmod__ "Copy/link to this API element")

```python
__divmod__(self: Self, b: float)
```

```python
__divmod__(self: Self, b: float32)
```

```python
__divmod__(self: Self, b: float16)
```

```python
__divmod__(self: Self, b: bfloat16)
```

```python
__divmod__(self: Self, b: Int)
```

```python
__divmod__(self: Self, b: UInt)
```

```python
__divmod__(self: Self, b: bool)
```

\_\_pow\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/ops/#class-float128-__pow__ "Copy/link to this API element")

```python
__pow__(self: Self, b: float)
```

```python
__pow__(self: Self, b: float32)
```

```python
__pow__(self: Self, b: float16)
```

```python
__pow__(self: Self, b: bfloat16)
```

```python
__pow__(self: Self, b: Int)
```

```python
__pow__(self: Self, b: UInt)
```

```python
__pow__(self: Self, b: bool)
```

\_\_eq\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/ops/#class-float128-__eq__ "Copy/link to this API element")

```python
__eq__(self: Self, b: float)
```

```python
__eq__(self: Self, b: float32)
```

```python
__eq__(self: Self, b: float16)
```

```python
__eq__(self: Self, b: bfloat16)
```

```python
__eq__(self: Self, b: Int)
```

```python
__eq__(self: Self, b: UInt)
```

```python
__eq__(self: Self, b: bool)
```

\_\_ne\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/ops/#class-float128-__ne__ "Copy/link to this API element")

```python
__ne__(self: Self, b: float)
```

```python
__ne__(self: Self, b: float32)
```

```python
__ne__(self: Self, b: float16)
```

```python
__ne__(self: Self, b: bfloat16)
```

```python
__ne__(self: Self, b: Int)
```

```python
__ne__(self: Self, b: UInt)
```

```python
__ne__(self: Self, b: bool)
```

\_\_lt\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/ops/#class-float128-__lt__ "Copy/link to this API element")

```python
__lt__(self: Self, b: float)
```

```python
__lt__(self: Self, b: float32)
```

```python
__lt__(self: Self, b: float16)
```

```python
__lt__(self: Self, b: bfloat16)
```

```python
__lt__(self: Self, b: Int)
```

```python
__lt__(self: Self, b: UInt)
```

```python
__lt__(self: Self, b: bool)
```

\_\_gt\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/ops/#class-float128-__gt__ "Copy/link to this API element")

```python
__gt__(self: Self, b: float)
```

```python
__gt__(self: Self, b: float32)
```

```python
__gt__(self: Self, b: float16)
```

```python
__gt__(self: Self, b: bfloat16)
```

```python
__gt__(self: Self, b: Int)
```

```python
__gt__(self: Self, b: UInt)
```

```python
__gt__(self: Self, b: bool)
```

\_\_le\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/ops/#class-float128-__le__ "Copy/link to this API element")

```python
__le__(self: Self, b: float)
```

```python
__le__(self: Self, b: float32)
```

```python
__le__(self: Self, b: float16)
```

```python
__le__(self: Self, b: bfloat16)
```

```python
__le__(self: Self, b: Int)
```

```python
__le__(self: Self, b: UInt)
```

```python
__le__(self: Self, b: bool)
```

\_\_ge\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/ops/#class-float128-__ge__ "Copy/link to this API element")

```python
__ge__(self: Self, b: float)
```

```python
__ge__(self: Self, b: float32)
```

```python
__ge__(self: Self, b: float16)
```

```python
__ge__(self: Self, b: bfloat16)
```

```python
__ge__(self: Self, b: Int)
```

```python
__ge__(self: Self, b: UInt)
```

```python
__ge__(self: Self, b: bool)
```

class float32[](https://docs.exaloop.io/libraries/api/modules/internal/ops/#class-float32 "Copy/link to this API element")

@extend

```python
class float32
```

Extends [`internal.core.float32`](https://docs.exaloop.io/libraries/api/classes/internal/core/float32/) · [View complete class API →](https://docs.exaloop.io/libraries/api/classes/internal/core/float32/)

Special methods 14

\_\_add\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/ops/#class-float32-__add__ "Copy/link to this API element")

@commutative

```python
__add__(self: Self, b: float)
```

```python
__add__(self: Self, b: float128)
```

```python
__add__(self: Self, b: float16)
```

```python
__add__(self: Self, b: bfloat16)
```

```python
__add__(self: Self, b: Int)
```

```python
__add__(self: Self, b: UInt)
```

```python
__add__(self: Self, b: bool)
```

\_\_sub\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/ops/#class-float32-__sub__ "Copy/link to this API element")

```python
__sub__(self: Self, b: float)
```

```python
__sub__(self: Self, b: float128)
```

```python
__sub__(self: Self, b: float16)
```

```python
__sub__(self: Self, b: bfloat16)
```

```python
__sub__(self: Self, b: Int)
```

```python
__sub__(self: Self, b: UInt)
```

```python
__sub__(self: Self, b: bool)
```

\_\_mul\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/ops/#class-float32-__mul__ "Copy/link to this API element")

@commutative

```python
__mul__(self: Self, b: float)
```

```python
__mul__(self: Self, b: float128)
```

```python
__mul__(self: Self, b: float16)
```

```python
__mul__(self: Self, b: bfloat16)
```

```python
__mul__(self: Self, b: Int)
```

```python
__mul__(self: Self, b: UInt)
```

```python
__mul__(self: Self, b: bool)
```

\_\_floordiv\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/ops/#class-float32-__floordiv__ "Copy/link to this API element")

```python
__floordiv__(self: Self, b: float)
```

```python
__floordiv__(self: Self, b: float128)
```

```python
__floordiv__(self: Self, b: float16)
```

```python
__floordiv__(self: Self, b: bfloat16)
```

```python
__floordiv__(self: Self, b: Int)
```

```python
__floordiv__(self: Self, b: UInt)
```

```python
__floordiv__(self: Self, b: bool)
```

\_\_truediv\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/ops/#class-float32-__truediv__ "Copy/link to this API element")

```python
__truediv__(self: Self, b: float)
```

```python
__truediv__(self: Self, b: float128)
```

```python
__truediv__(self: Self, b: float16)
```

```python
__truediv__(self: Self, b: bfloat16)
```

```python
__truediv__(self: Self, b: Int)
```

```python
__truediv__(self: Self, b: UInt)
```

```python
__truediv__(self: Self, b: bool)
```

\_\_mod\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/ops/#class-float32-__mod__ "Copy/link to this API element")

```python
__mod__(self: Self, b: float)
```

```python
__mod__(self: Self, b: float128)
```

```python
__mod__(self: Self, b: float16)
```

```python
__mod__(self: Self, b: bfloat16)
```

```python
__mod__(self: Self, b: Int)
```

```python
__mod__(self: Self, b: UInt)
```

```python
__mod__(self: Self, b: bool)
```

\_\_divmod\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/ops/#class-float32-__divmod__ "Copy/link to this API element")

```python
__divmod__(self: Self, b: float)
```

```python
__divmod__(self: Self, b: float128)
```

```python
__divmod__(self: Self, b: float16)
```

```python
__divmod__(self: Self, b: bfloat16)
```

```python
__divmod__(self: Self, b: Int)
```

```python
__divmod__(self: Self, b: UInt)
```

```python
__divmod__(self: Self, b: bool)
```

\_\_pow\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/ops/#class-float32-__pow__ "Copy/link to this API element")

```python
__pow__(self: Self, b: float)
```

```python
__pow__(self: Self, b: float128)
```

```python
__pow__(self: Self, b: float16)
```

```python
__pow__(self: Self, b: bfloat16)
```

```python
__pow__(self: Self, b: Int)
```

```python
__pow__(self: Self, b: UInt)
```

```python
__pow__(self: Self, b: bool)
```

\_\_eq\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/ops/#class-float32-__eq__ "Copy/link to this API element")

```python
__eq__(self: Self, b: float)
```

```python
__eq__(self: Self, b: float128)
```

```python
__eq__(self: Self, b: float16)
```

```python
__eq__(self: Self, b: bfloat16)
```

```python
__eq__(self: Self, b: Int)
```

```python
__eq__(self: Self, b: UInt)
```

```python
__eq__(self: Self, b: bool)
```

\_\_ne\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/ops/#class-float32-__ne__ "Copy/link to this API element")

```python
__ne__(self: Self, b: float)
```

```python
__ne__(self: Self, b: float128)
```

```python
__ne__(self: Self, b: float16)
```

```python
__ne__(self: Self, b: bfloat16)
```

```python
__ne__(self: Self, b: Int)
```

```python
__ne__(self: Self, b: UInt)
```

```python
__ne__(self: Self, b: bool)
```

\_\_lt\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/ops/#class-float32-__lt__ "Copy/link to this API element")

```python
__lt__(self: Self, b: float)
```

```python
__lt__(self: Self, b: float128)
```

```python
__lt__(self: Self, b: float16)
```

```python
__lt__(self: Self, b: bfloat16)
```

```python
__lt__(self: Self, b: Int)
```

```python
__lt__(self: Self, b: UInt)
```

```python
__lt__(self: Self, b: bool)
```

\_\_gt\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/ops/#class-float32-__gt__ "Copy/link to this API element")

```python
__gt__(self: Self, b: float)
```

```python
__gt__(self: Self, b: float128)
```

```python
__gt__(self: Self, b: float16)
```

```python
__gt__(self: Self, b: bfloat16)
```

```python
__gt__(self: Self, b: Int)
```

```python
__gt__(self: Self, b: UInt)
```

```python
__gt__(self: Self, b: bool)
```

\_\_le\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/ops/#class-float32-__le__ "Copy/link to this API element")

```python
__le__(self: Self, b: float)
```

```python
__le__(self: Self, b: float128)
```

```python
__le__(self: Self, b: float16)
```

```python
__le__(self: Self, b: bfloat16)
```

```python
__le__(self: Self, b: Int)
```

```python
__le__(self: Self, b: UInt)
```

```python
__le__(self: Self, b: bool)
```

\_\_ge\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/ops/#class-float32-__ge__ "Copy/link to this API element")

```python
__ge__(self: Self, b: float)
```

```python
__ge__(self: Self, b: float128)
```

```python
__ge__(self: Self, b: float16)
```

```python
__ge__(self: Self, b: bfloat16)
```

```python
__ge__(self: Self, b: Int)
```

```python
__ge__(self: Self, b: UInt)
```

```python
__ge__(self: Self, b: bool)
```

class float16[](https://docs.exaloop.io/libraries/api/modules/internal/ops/#class-float16 "Copy/link to this API element")

@extend

```python
class float16
```

Extends [`internal.core.float16`](https://docs.exaloop.io/libraries/api/classes/internal/core/float16/) · [View complete class API →](https://docs.exaloop.io/libraries/api/classes/internal/core/float16/)

Special methods 14

\_\_add\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/ops/#class-float16-__add__ "Copy/link to this API element")

@commutative

```python
__add__(self: Self, b: float)
```

```python
__add__(self: Self, b: float128)
```

```python
__add__(self: Self, b: float32)
```

```python
__add__(self: Self, b: bfloat16)
```

```python
__add__(self: Self, b: Int)
```

```python
__add__(self: Self, b: UInt)
```

```python
__add__(self: Self, b: bool)
```

\_\_sub\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/ops/#class-float16-__sub__ "Copy/link to this API element")

```python
__sub__(self: Self, b: float)
```

```python
__sub__(self: Self, b: float128)
```

```python
__sub__(self: Self, b: float32)
```

```python
__sub__(self: Self, b: bfloat16)
```

```python
__sub__(self: Self, b: Int)
```

```python
__sub__(self: Self, b: UInt)
```

```python
__sub__(self: Self, b: bool)
```

\_\_mul\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/ops/#class-float16-__mul__ "Copy/link to this API element")

@commutative

```python
__mul__(self: Self, b: float)
```

```python
__mul__(self: Self, b: float128)
```

```python
__mul__(self: Self, b: float32)
```

```python
__mul__(self: Self, b: bfloat16)
```

```python
__mul__(self: Self, b: Int)
```

```python
__mul__(self: Self, b: UInt)
```

```python
__mul__(self: Self, b: bool)
```

\_\_floordiv\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/ops/#class-float16-__floordiv__ "Copy/link to this API element")

```python
__floordiv__(self: Self, b: float)
```

```python
__floordiv__(self: Self, b: float128)
```

```python
__floordiv__(self: Self, b: float32)
```

```python
__floordiv__(self: Self, b: bfloat16)
```

```python
__floordiv__(self: Self, b: Int)
```

```python
__floordiv__(self: Self, b: UInt)
```

```python
__floordiv__(self: Self, b: bool)
```

\_\_truediv\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/ops/#class-float16-__truediv__ "Copy/link to this API element")

```python
__truediv__(self: Self, b: float)
```

```python
__truediv__(self: Self, b: float128)
```

```python
__truediv__(self: Self, b: float32)
```

```python
__truediv__(self: Self, b: bfloat16)
```

```python
__truediv__(self: Self, b: Int)
```

```python
__truediv__(self: Self, b: UInt)
```

```python
__truediv__(self: Self, b: bool)
```

\_\_mod\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/ops/#class-float16-__mod__ "Copy/link to this API element")

```python
__mod__(self: Self, b: float)
```

```python
__mod__(self: Self, b: float128)
```

```python
__mod__(self: Self, b: float32)
```

```python
__mod__(self: Self, b: bfloat16)
```

```python
__mod__(self: Self, b: Int)
```

```python
__mod__(self: Self, b: UInt)
```

```python
__mod__(self: Self, b: bool)
```

\_\_divmod\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/ops/#class-float16-__divmod__ "Copy/link to this API element")

```python
__divmod__(self: Self, b: float)
```

```python
__divmod__(self: Self, b: float128)
```

```python
__divmod__(self: Self, b: float32)
```

```python
__divmod__(self: Self, b: bfloat16)
```

```python
__divmod__(self: Self, b: Int)
```

```python
__divmod__(self: Self, b: UInt)
```

```python
__divmod__(self: Self, b: bool)
```

\_\_pow\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/ops/#class-float16-__pow__ "Copy/link to this API element")

```python
__pow__(self: Self, b: float)
```

```python
__pow__(self: Self, b: float128)
```

```python
__pow__(self: Self, b: float32)
```

```python
__pow__(self: Self, b: bfloat16)
```

```python
__pow__(self: Self, b: Int)
```

```python
__pow__(self: Self, b: UInt)
```

```python
__pow__(self: Self, b: bool)
```

\_\_eq\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/ops/#class-float16-__eq__ "Copy/link to this API element")

```python
__eq__(self: Self, b: float)
```

```python
__eq__(self: Self, b: float128)
```

```python
__eq__(self: Self, b: float32)
```

```python
__eq__(self: Self, b: bfloat16)
```

```python
__eq__(self: Self, b: Int)
```

```python
__eq__(self: Self, b: UInt)
```

```python
__eq__(self: Self, b: bool)
```

\_\_ne\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/ops/#class-float16-__ne__ "Copy/link to this API element")

```python
__ne__(self: Self, b: float)
```

```python
__ne__(self: Self, b: float128)
```

```python
__ne__(self: Self, b: float32)
```

```python
__ne__(self: Self, b: bfloat16)
```

```python
__ne__(self: Self, b: Int)
```

```python
__ne__(self: Self, b: UInt)
```

```python
__ne__(self: Self, b: bool)
```

\_\_lt\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/ops/#class-float16-__lt__ "Copy/link to this API element")

```python
__lt__(self: Self, b: float)
```

```python
__lt__(self: Self, b: float128)
```

```python
__lt__(self: Self, b: float32)
```

```python
__lt__(self: Self, b: bfloat16)
```

```python
__lt__(self: Self, b: Int)
```

```python
__lt__(self: Self, b: UInt)
```

```python
__lt__(self: Self, b: bool)
```

\_\_gt\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/ops/#class-float16-__gt__ "Copy/link to this API element")

```python
__gt__(self: Self, b: float)
```

```python
__gt__(self: Self, b: float128)
```

```python
__gt__(self: Self, b: float32)
```

```python
__gt__(self: Self, b: bfloat16)
```

```python
__gt__(self: Self, b: Int)
```

```python
__gt__(self: Self, b: UInt)
```

```python
__gt__(self: Self, b: bool)
```

\_\_le\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/ops/#class-float16-__le__ "Copy/link to this API element")

```python
__le__(self: Self, b: float)
```

```python
__le__(self: Self, b: float128)
```

```python
__le__(self: Self, b: float32)
```

```python
__le__(self: Self, b: bfloat16)
```

```python
__le__(self: Self, b: Int)
```

```python
__le__(self: Self, b: UInt)
```

```python
__le__(self: Self, b: bool)
```

\_\_ge\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/ops/#class-float16-__ge__ "Copy/link to this API element")

```python
__ge__(self: Self, b: float)
```

```python
__ge__(self: Self, b: float128)
```

```python
__ge__(self: Self, b: float32)
```

```python
__ge__(self: Self, b: bfloat16)
```

```python
__ge__(self: Self, b: Int)
```

```python
__ge__(self: Self, b: UInt)
```

```python
__ge__(self: Self, b: bool)
```

class bfloat16[](https://docs.exaloop.io/libraries/api/modules/internal/ops/#class-bfloat16 "Copy/link to this API element")

@extend

```python
class bfloat16
```

Extends [`internal.core.bfloat16`](https://docs.exaloop.io/libraries/api/classes/internal/core/bfloat16/) · [View complete class API →](https://docs.exaloop.io/libraries/api/classes/internal/core/bfloat16/)

Special methods 14

\_\_add\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/ops/#class-bfloat16-__add__ "Copy/link to this API element")

@commutative

```python
__add__(self: Self, b: float)
```

```python
__add__(self: Self, b: float128)
```

```python
__add__(self: Self, b: float32)
```

```python
__add__(self: Self, b: float16)
```

```python
__add__(self: Self, b: Int)
```

```python
__add__(self: Self, b: UInt)
```

```python
__add__(self: Self, b: bool)
```

\_\_sub\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/ops/#class-bfloat16-__sub__ "Copy/link to this API element")

```python
__sub__(self: Self, b: float)
```

```python
__sub__(self: Self, b: float128)
```

```python
__sub__(self: Self, b: float32)
```

```python
__sub__(self: Self, b: float16)
```

```python
__sub__(self: Self, b: Int)
```

```python
__sub__(self: Self, b: UInt)
```

```python
__sub__(self: Self, b: bool)
```

\_\_mul\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/ops/#class-bfloat16-__mul__ "Copy/link to this API element")

@commutative

```python
__mul__(self: Self, b: float)
```

```python
__mul__(self: Self, b: float128)
```

```python
__mul__(self: Self, b: float32)
```

```python
__mul__(self: Self, b: float16)
```

```python
__mul__(self: Self, b: Int)
```

```python
__mul__(self: Self, b: UInt)
```

```python
__mul__(self: Self, b: bool)
```

\_\_floordiv\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/ops/#class-bfloat16-__floordiv__ "Copy/link to this API element")

```python
__floordiv__(self: Self, b: float)
```

```python
__floordiv__(self: Self, b: float128)
```

```python
__floordiv__(self: Self, b: float32)
```

```python
__floordiv__(self: Self, b: float16)
```

```python
__floordiv__(self: Self, b: Int)
```

```python
__floordiv__(self: Self, b: UInt)
```

```python
__floordiv__(self: Self, b: bool)
```

\_\_truediv\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/ops/#class-bfloat16-__truediv__ "Copy/link to this API element")

```python
__truediv__(self: Self, b: float)
```

```python
__truediv__(self: Self, b: float128)
```

```python
__truediv__(self: Self, b: float32)
```

```python
__truediv__(self: Self, b: float16)
```

```python
__truediv__(self: Self, b: Int)
```

```python
__truediv__(self: Self, b: UInt)
```

```python
__truediv__(self: Self, b: bool)
```

\_\_mod\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/ops/#class-bfloat16-__mod__ "Copy/link to this API element")

```python
__mod__(self: Self, b: float)
```

```python
__mod__(self: Self, b: float128)
```

```python
__mod__(self: Self, b: float32)
```

```python
__mod__(self: Self, b: float16)
```

```python
__mod__(self: Self, b: Int)
```

```python
__mod__(self: Self, b: UInt)
```

```python
__mod__(self: Self, b: bool)
```

\_\_divmod\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/ops/#class-bfloat16-__divmod__ "Copy/link to this API element")

```python
__divmod__(self: Self, b: float)
```

```python
__divmod__(self: Self, b: float128)
```

```python
__divmod__(self: Self, b: float32)
```

```python
__divmod__(self: Self, b: float16)
```

```python
__divmod__(self: Self, b: Int)
```

```python
__divmod__(self: Self, b: UInt)
```

```python
__divmod__(self: Self, b: bool)
```

\_\_pow\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/ops/#class-bfloat16-__pow__ "Copy/link to this API element")

```python
__pow__(self: Self, b: float)
```

```python
__pow__(self: Self, b: float128)
```

```python
__pow__(self: Self, b: float32)
```

```python
__pow__(self: Self, b: float16)
```

```python
__pow__(self: Self, b: Int)
```

```python
__pow__(self: Self, b: UInt)
```

```python
__pow__(self: Self, b: bool)
```

\_\_eq\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/ops/#class-bfloat16-__eq__ "Copy/link to this API element")

```python
__eq__(self: Self, b: float)
```

```python
__eq__(self: Self, b: float128)
```

```python
__eq__(self: Self, b: float32)
```

```python
__eq__(self: Self, b: float16)
```

```python
__eq__(self: Self, b: Int)
```

```python
__eq__(self: Self, b: UInt)
```

```python
__eq__(self: Self, b: bool)
```

\_\_ne\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/ops/#class-bfloat16-__ne__ "Copy/link to this API element")

```python
__ne__(self: Self, b: float)
```

```python
__ne__(self: Self, b: float128)
```

```python
__ne__(self: Self, b: float32)
```

```python
__ne__(self: Self, b: float16)
```

```python
__ne__(self: Self, b: Int)
```

```python
__ne__(self: Self, b: UInt)
```

```python
__ne__(self: Self, b: bool)
```

\_\_lt\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/ops/#class-bfloat16-__lt__ "Copy/link to this API element")

```python
__lt__(self: Self, b: float)
```

```python
__lt__(self: Self, b: float128)
```

```python
__lt__(self: Self, b: float32)
```

```python
__lt__(self: Self, b: float16)
```

```python
__lt__(self: Self, b: Int)
```

```python
__lt__(self: Self, b: UInt)
```

```python
__lt__(self: Self, b: bool)
```

\_\_gt\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/ops/#class-bfloat16-__gt__ "Copy/link to this API element")

```python
__gt__(self: Self, b: float)
```

```python
__gt__(self: Self, b: float128)
```

```python
__gt__(self: Self, b: float32)
```

```python
__gt__(self: Self, b: float16)
```

```python
__gt__(self: Self, b: Int)
```

```python
__gt__(self: Self, b: UInt)
```

```python
__gt__(self: Self, b: bool)
```

\_\_le\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/ops/#class-bfloat16-__le__ "Copy/link to this API element")

```python
__le__(self: Self, b: float)
```

```python
__le__(self: Self, b: float128)
```

```python
__le__(self: Self, b: float32)
```

```python
__le__(self: Self, b: float16)
```

```python
__le__(self: Self, b: Int)
```

```python
__le__(self: Self, b: UInt)
```

```python
__le__(self: Self, b: bool)
```

\_\_ge\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/ops/#class-bfloat16-__ge__ "Copy/link to this API element")

```python
__ge__(self: Self, b: float)
```

```python
__ge__(self: Self, b: float128)
```

```python
__ge__(self: Self, b: float32)
```

```python
__ge__(self: Self, b: float16)
```

```python
__ge__(self: Self, b: Int)
```

```python
__ge__(self: Self, b: UInt)
```

```python
__ge__(self: Self, b: bool)
```

class bool[](https://docs.exaloop.io/libraries/api/modules/internal/ops/#class-bool "Copy/link to this API element")

@extend

```python
class bool
```

Extends [`internal.core.bool`](https://docs.exaloop.io/libraries/api/classes/internal/core/bool/) · [View complete class API →](https://docs.exaloop.io/libraries/api/classes/internal/core/bool/)

Special methods 14

\_\_add\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/ops/#class-bool-__add__ "Copy/link to this API element")

@commutative

```python
__add__(self: Self, b: float, /)
```

```python
__add__(self: Self, b: float128, /)
```

```python
__add__(self: Self, b: float32, /)
```

```python
__add__(self: Self, b: float16, /)
```

```python
__add__(self: Self, b: bfloat16, /)
```

```python
__add__(self: Self, b: Int, /)
```

```python
__add__(self: Self, b: UInt, /)
```

\_\_sub\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/ops/#class-bool-__sub__ "Copy/link to this API element")

```python
__sub__(self: Self, b: float, /)
```

```python
__sub__(self: Self, b: float128, /)
```

```python
__sub__(self: Self, b: float32, /)
```

```python
__sub__(self: Self, b: float16, /)
```

```python
__sub__(self: Self, b: bfloat16, /)
```

```python
__sub__(self: Self, b: Int, /)
```

```python
__sub__(self: Self, b: UInt, /)
```

\_\_mul\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/ops/#class-bool-__mul__ "Copy/link to this API element")

@commutative

```python
__mul__(self: Self, b: float, /)
```

```python
__mul__(self: Self, b: float128, /)
```

```python
__mul__(self: Self, b: float32, /)
```

```python
__mul__(self: Self, b: float16, /)
```

```python
__mul__(self: Self, b: bfloat16, /)
```

```python
__mul__(self: Self, b: Int, /)
```

```python
__mul__(self: Self, b: UInt, /)
```

\_\_floordiv\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/ops/#class-bool-__floordiv__ "Copy/link to this API element")

```python
__floordiv__(self: Self, b: float, /)
```

```python
__floordiv__(self: Self, b: float128, /)
```

```python
__floordiv__(self: Self, b: float32, /)
```

```python
__floordiv__(self: Self, b: float16, /)
```

```python
__floordiv__(self: Self, b: bfloat16, /)
```

```python
__floordiv__(self: Self, b: Int, /)
```

```python
__floordiv__(self: Self, b: UInt, /)
```

\_\_truediv\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/ops/#class-bool-__truediv__ "Copy/link to this API element")

```python
__truediv__(self: Self, b: float, /)
```

```python
__truediv__(self: Self, b: float128, /)
```

```python
__truediv__(self: Self, b: float32, /)
```

```python
__truediv__(self: Self, b: float16, /)
```

```python
__truediv__(self: Self, b: bfloat16, /)
```

```python
__truediv__(self: Self, b: Int, /)
```

```python
__truediv__(self: Self, b: UInt, /)
```

\_\_mod\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/ops/#class-bool-__mod__ "Copy/link to this API element")

```python
__mod__(self: Self, b: float, /)
```

```python
__mod__(self: Self, b: float128, /)
```

```python
__mod__(self: Self, b: float32, /)
```

```python
__mod__(self: Self, b: float16, /)
```

```python
__mod__(self: Self, b: bfloat16, /)
```

```python
__mod__(self: Self, b: Int, /)
```

```python
__mod__(self: Self, b: UInt, /)
```

\_\_divmod\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/ops/#class-bool-__divmod__ "Copy/link to this API element")

```python
__divmod__(self: Self, b: float, /)
```

```python
__divmod__(self: Self, b: float128, /)
```

```python
__divmod__(self: Self, b: float32, /)
```

```python
__divmod__(self: Self, b: float16, /)
```

```python
__divmod__(self: Self, b: bfloat16, /)
```

```python
__divmod__(self: Self, b: Int, /)
```

```python
__divmod__(self: Self, b: UInt, /)
```

\_\_pow\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/ops/#class-bool-__pow__ "Copy/link to this API element")

```python
__pow__(self: Self, b: float, /)
```

```python
__pow__(self: Self, b: float128, /)
```

```python
__pow__(self: Self, b: float32, /)
```

```python
__pow__(self: Self, b: float16, /)
```

```python
__pow__(self: Self, b: bfloat16, /)
```

```python
__pow__(self: Self, b: Int, /)
```

```python
__pow__(self: Self, b: UInt, /)
```

\_\_eq\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/ops/#class-bool-__eq__ "Copy/link to this API element")

```python
__eq__(self: Self, b: float, /)
```

```python
__eq__(self: Self, b: float128, /)
```

```python
__eq__(self: Self, b: float32, /)
```

```python
__eq__(self: Self, b: float16, /)
```

```python
__eq__(self: Self, b: bfloat16, /)
```

```python
__eq__(self: Self, b: Int, /)
```

```python
__eq__(self: Self, b: UInt, /)
```

\_\_ne\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/ops/#class-bool-__ne__ "Copy/link to this API element")

```python
__ne__(self: Self, b: float, /)
```

```python
__ne__(self: Self, b: float128, /)
```

```python
__ne__(self: Self, b: float32, /)
```

```python
__ne__(self: Self, b: float16, /)
```

```python
__ne__(self: Self, b: bfloat16, /)
```

```python
__ne__(self: Self, b: Int, /)
```

```python
__ne__(self: Self, b: UInt, /)
```

\_\_lt\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/ops/#class-bool-__lt__ "Copy/link to this API element")

```python
__lt__(self: Self, b: float, /)
```

```python
__lt__(self: Self, b: float128, /)
```

```python
__lt__(self: Self, b: float32, /)
```

```python
__lt__(self: Self, b: float16, /)
```

```python
__lt__(self: Self, b: bfloat16, /)
```

```python
__lt__(self: Self, b: Int, /)
```

```python
__lt__(self: Self, b: UInt, /)
```

\_\_gt\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/ops/#class-bool-__gt__ "Copy/link to this API element")

```python
__gt__(self: Self, b: float, /)
```

```python
__gt__(self: Self, b: float128, /)
```

```python
__gt__(self: Self, b: float32, /)
```

```python
__gt__(self: Self, b: float16, /)
```

```python
__gt__(self: Self, b: bfloat16, /)
```

```python
__gt__(self: Self, b: Int, /)
```

```python
__gt__(self: Self, b: UInt, /)
```

\_\_le\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/ops/#class-bool-__le__ "Copy/link to this API element")

```python
__le__(self: Self, b: float, /)
```

```python
__le__(self: Self, b: float128, /)
```

```python
__le__(self: Self, b: float32, /)
```

```python
__le__(self: Self, b: float16, /)
```

```python
__le__(self: Self, b: bfloat16, /)
```

```python
__le__(self: Self, b: Int, /)
```

```python
__le__(self: Self, b: UInt, /)
```

\_\_ge\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/ops/#class-bool-__ge__ "Copy/link to this API element")

```python
__ge__(self: Self, b: float, /)
```

```python
__ge__(self: Self, b: float128, /)
```

```python
__ge__(self: Self, b: float32, /)
```

```python
__ge__(self: Self, b: float16, /)
```

```python
__ge__(self: Self, b: bfloat16, /)
```

```python
__ge__(self: Self, b: Int, /)
```

```python
__ge__(self: Self, b: UInt, /)
```

Source: https://docs.exaloop.io/libraries/api/modules/internal/ops/
