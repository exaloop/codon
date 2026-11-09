# float

Standard library class

class float[](https://docs.exaloop.io/libraries/api/classes/float/#class-float "Copy/link to this API element")

@tuple

```python
class float
```

Constructors 9

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/classes/float/#class-float-__new__ "Copy/link to this API element")

```python
__new__(what)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/classes/float/#class-float-__new__-overload-2 "Copy/link to this API element")

```python
__new__(what: bool)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/classes/float/#class-float-__new__-overload-3 "Copy/link to this API element")

@pure @llvm

```python
__new__(what: Int[N], N: Literal[int])
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```llvm
%0 = sitofp i{=N} %what to double
ret double %0
```

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/classes/float/#class-float-__new__-overload-4 "Copy/link to this API element")

@pure @llvm

```python
__new__(what: UInt[N], N: Literal[int])
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```llvm
%0 = uitofp i{=N} %what to double
ret double %0
```

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/classes/float/#class-float-__new__-overload-5 "Copy/link to this API element")

@pure @llvm

```python
__new__(what: float32)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```llvm
%0 = fpext float %what to double
ret double %0
```

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/classes/float/#class-float-__new__-overload-6 "Copy/link to this API element")

@pure @llvm

```python
__new__(what: float16)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```llvm
%0 = fpext half %what to double
ret double %0
```

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/classes/float/#class-float-__new__-overload-7 "Copy/link to this API element")

@pure @llvm

```python
__new__(what: bfloat16)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```llvm
%0 = fpext bfloat %what to double
ret double %0
```

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/classes/float/#class-float-__new__-overload-8 "Copy/link to this API element")

```python
__new__()
```

Added by [`internal.types.float`](https://docs.exaloop.io/libraries/api/modules/internal/types/float/)

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/classes/float/#class-float-__new__-overload-9 "Copy/link to this API element")

@overload

```python
__new__(what)
```

Added by [`internal.types.float`](https://docs.exaloop.io/libraries/api/modules/internal/types/float/)

Properties 2

real[](https://docs.exaloop.io/libraries/api/classes/float/#class-float-real "Copy/link to this API element")

@property

```python
real(self: Self, Self: type)
```

Added by [`internal.types.float`](https://docs.exaloop.io/libraries/api/modules/internal/types/float/)

imag[](https://docs.exaloop.io/libraries/api/classes/float/#class-float-imag "Copy/link to this API element")

@property

```python
imag(self: Self, Self: type)
```

Added by [`internal.types.float`](https://docs.exaloop.io/libraries/api/modules/internal/types/float/)

Methods 2

cdiv[](https://docs.exaloop.io/libraries/api/classes/float/#class-float-cdiv "Copy/link to this API element")

```python
cdiv(self: Self, other: float, Self: type)
```

Added by [`internal.types.float`](https://docs.exaloop.io/libraries/api/modules/internal/types/float/)

cmod[](https://docs.exaloop.io/libraries/api/classes/float/#class-float-cmod "Copy/link to this API element")

```python
cmod(self: Self, other: float, Self: type)
```

Added by [`internal.types.float`](https://docs.exaloop.io/libraries/api/modules/internal/types/float/)

Special methods 47

\_\_add\_\_[](https://docs.exaloop.io/libraries/api/classes/float/#class-float-__add__ "Copy/link to this API element")

@commutative

```python
__add__(self: Self, b: float128, Self: type)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```python
__add__(self: Self, b: float32, Self: type)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```python
__add__(self: Self, b: float16, Self: type)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```python
__add__(self: Self, b: bfloat16, Self: type)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```python
__add__(self: Self, b: Int, Self: type)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```python
__add__(self: Self, b: UInt, Self: type)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```python
__add__(self: Self, b: bool, Self: type)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

@pure @llvm

```python
__add__(a: float, b: float)
```

Added by [`internal.types.float`](https://docs.exaloop.io/libraries/api/modules/internal/types/float/)

```llvm
%tmp = fadd double %a, %b
ret double %tmp
```

\_\_sub\_\_[](https://docs.exaloop.io/libraries/api/classes/float/#class-float-__sub__ "Copy/link to this API element")

```python
__sub__(self: Self, b: float128, Self: type)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```python
__sub__(self: Self, b: float32, Self: type)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```python
__sub__(self: Self, b: float16, Self: type)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```python
__sub__(self: Self, b: bfloat16, Self: type)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```python
__sub__(self: Self, b: Int, Self: type)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```python
__sub__(self: Self, b: UInt, Self: type)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```python
__sub__(self: Self, b: bool, Self: type)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

@pure @llvm

```python
__sub__(a: float, b: float)
```

Added by [`internal.types.float`](https://docs.exaloop.io/libraries/api/modules/internal/types/float/)

```llvm
%tmp = fsub double %a, %b
ret double %tmp
```

\_\_mul\_\_[](https://docs.exaloop.io/libraries/api/classes/float/#class-float-__mul__ "Copy/link to this API element")

@commutative

```python
__mul__(self: Self, b: float128, Self: type)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```python
__mul__(self: Self, b: float32, Self: type)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```python
__mul__(self: Self, b: float16, Self: type)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```python
__mul__(self: Self, b: bfloat16, Self: type)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```python
__mul__(self: Self, b: Int, Self: type)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```python
__mul__(self: Self, b: UInt, Self: type)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```python
__mul__(self: Self, b: bool, Self: type)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

@pure @llvm

```python
__mul__(a: float, b: float)
```

Added by [`internal.types.float`](https://docs.exaloop.io/libraries/api/modules/internal/types/float/)

```llvm
%tmp = fmul double %a, %b
ret double %tmp
```

\_\_floordiv\_\_[](https://docs.exaloop.io/libraries/api/classes/float/#class-float-__floordiv__ "Copy/link to this API element")

```python
__floordiv__(self: Self, b: float128, Self: type)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```python
__floordiv__(self: Self, b: float32, Self: type)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```python
__floordiv__(self: Self, b: float16, Self: type)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```python
__floordiv__(self: Self, b: bfloat16, Self: type)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```python
__floordiv__(self: Self, b: Int, Self: type)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```python
__floordiv__(self: Self, b: UInt, Self: type)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```python
__floordiv__(self: Self, b: bool, Self: type)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```python
__floordiv__(self: Self, other: float, Self: type)
```

Added by [`internal.types.float`](https://docs.exaloop.io/libraries/api/modules/internal/types/float/)

\_\_truediv\_\_[](https://docs.exaloop.io/libraries/api/classes/float/#class-float-__truediv__ "Copy/link to this API element")

```python
__truediv__(self: Self, b: float128, Self: type)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```python
__truediv__(self: Self, b: float32, Self: type)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```python
__truediv__(self: Self, b: float16, Self: type)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```python
__truediv__(self: Self, b: bfloat16, Self: type)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```python
__truediv__(self: Self, b: Int, Self: type)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```python
__truediv__(self: Self, b: UInt, Self: type)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```python
__truediv__(self: Self, b: bool, Self: type)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```python
__truediv__(self: Self, other: float, Self: type)
```

Added by [`internal.types.float`](https://docs.exaloop.io/libraries/api/modules/internal/types/float/)

\_\_mod\_\_[](https://docs.exaloop.io/libraries/api/classes/float/#class-float-__mod__ "Copy/link to this API element")

```python
__mod__(self: Self, b: float128, Self: type)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```python
__mod__(self: Self, b: float32, Self: type)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```python
__mod__(self: Self, b: float16, Self: type)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```python
__mod__(self: Self, b: bfloat16, Self: type)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```python
__mod__(self: Self, b: Int, Self: type)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```python
__mod__(self: Self, b: UInt, Self: type)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```python
__mod__(self: Self, b: bool, Self: type)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```python
__mod__(self: Self, other: float, Self: type)
```

Added by [`internal.types.float`](https://docs.exaloop.io/libraries/api/modules/internal/types/float/)

\_\_divmod\_\_[](https://docs.exaloop.io/libraries/api/classes/float/#class-float-__divmod__ "Copy/link to this API element")

```python
__divmod__(self: Self, b: float128, Self: type)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```python
__divmod__(self: Self, b: float32, Self: type)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```python
__divmod__(self: Self, b: float16, Self: type)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```python
__divmod__(self: Self, b: bfloat16, Self: type)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```python
__divmod__(self: Self, b: Int, Self: type)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```python
__divmod__(self: Self, b: UInt, Self: type)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```python
__divmod__(self: Self, b: bool, Self: type)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```python
__divmod__(self: Self, other: float, Self: type)
```

Added by [`internal.types.float`](https://docs.exaloop.io/libraries/api/modules/internal/types/float/)

\_\_pow\_\_[](https://docs.exaloop.io/libraries/api/classes/float/#class-float-__pow__ "Copy/link to this API element")

```python
__pow__(self: Self, b: float128, Self: type)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```python
__pow__(self: Self, b: float32, Self: type)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```python
__pow__(self: Self, b: float16, Self: type)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```python
__pow__(self: Self, b: bfloat16, Self: type)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```python
__pow__(self: Self, b: Int, Self: type)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```python
__pow__(self: Self, b: UInt, Self: type)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```python
__pow__(self: Self, b: bool, Self: type)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

@pure @llvm

```python
__pow__(a: float, b: float)
```

Added by [`internal.types.float`](https://docs.exaloop.io/libraries/api/modules/internal/types/float/)

```llvm
declare double @llvm.pow.f64(double %a, double %b)
%tmp = call double @llvm.pow.f64(double %a, double %b)
ret double %tmp
```

\_\_eq\_\_[](https://docs.exaloop.io/libraries/api/classes/float/#class-float-__eq__ "Copy/link to this API element")

```python
__eq__(self: Self, b: float128, Self: type)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```python
__eq__(self: Self, b: float32, Self: type)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```python
__eq__(self: Self, b: float16, Self: type)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```python
__eq__(self: Self, b: bfloat16, Self: type)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```python
__eq__(self: Self, b: Int, Self: type)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```python
__eq__(self: Self, b: UInt, Self: type)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```python
__eq__(self: Self, b: bool, Self: type)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

@pure @llvm

```python
__eq__(a: float, b: float)
```

Added by [`internal.types.float`](https://docs.exaloop.io/libraries/api/modules/internal/types/float/)

```llvm
%tmp = fcmp oeq double %a, %b
%res = zext i1 %tmp to i8
ret i8 %res
```

\_\_ne\_\_[](https://docs.exaloop.io/libraries/api/classes/float/#class-float-__ne__ "Copy/link to this API element")

```python
__ne__(self: Self, b: float128, Self: type)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```python
__ne__(self: Self, b: float32, Self: type)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```python
__ne__(self: Self, b: float16, Self: type)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```python
__ne__(self: Self, b: bfloat16, Self: type)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```python
__ne__(self: Self, b: Int, Self: type)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```python
__ne__(self: Self, b: UInt, Self: type)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```python
__ne__(self: Self, b: bool, Self: type)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

@pure @llvm

```python
__ne__(a: float, b: float)
```

Added by [`internal.types.float`](https://docs.exaloop.io/libraries/api/modules/internal/types/float/)

```llvm
%tmp = fcmp une double %a, %b
%res = zext i1 %tmp to i8
ret i8 %res
```

\_\_lt\_\_[](https://docs.exaloop.io/libraries/api/classes/float/#class-float-__lt__ "Copy/link to this API element")

```python
__lt__(self: Self, b: float128, Self: type)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```python
__lt__(self: Self, b: float32, Self: type)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```python
__lt__(self: Self, b: float16, Self: type)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```python
__lt__(self: Self, b: bfloat16, Self: type)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```python
__lt__(self: Self, b: Int, Self: type)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```python
__lt__(self: Self, b: UInt, Self: type)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```python
__lt__(self: Self, b: bool, Self: type)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

@pure @llvm

```python
__lt__(a: float, b: float)
```

Added by [`internal.types.float`](https://docs.exaloop.io/libraries/api/modules/internal/types/float/)

```llvm
%tmp = fcmp olt double %a, %b
%res = zext i1 %tmp to i8
ret i8 %res
```

\_\_gt\_\_[](https://docs.exaloop.io/libraries/api/classes/float/#class-float-__gt__ "Copy/link to this API element")

```python
__gt__(self: Self, b: float128, Self: type)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```python
__gt__(self: Self, b: float32, Self: type)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```python
__gt__(self: Self, b: float16, Self: type)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```python
__gt__(self: Self, b: bfloat16, Self: type)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```python
__gt__(self: Self, b: Int, Self: type)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```python
__gt__(self: Self, b: UInt, Self: type)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```python
__gt__(self: Self, b: bool, Self: type)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

@pure @llvm

```python
__gt__(a: float, b: float)
```

Added by [`internal.types.float`](https://docs.exaloop.io/libraries/api/modules/internal/types/float/)

```llvm
%tmp = fcmp ogt double %a, %b
%res = zext i1 %tmp to i8
ret i8 %res
```

\_\_le\_\_[](https://docs.exaloop.io/libraries/api/classes/float/#class-float-__le__ "Copy/link to this API element")

```python
__le__(self: Self, b: float128, Self: type)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```python
__le__(self: Self, b: float32, Self: type)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```python
__le__(self: Self, b: float16, Self: type)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```python
__le__(self: Self, b: bfloat16, Self: type)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```python
__le__(self: Self, b: Int, Self: type)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```python
__le__(self: Self, b: UInt, Self: type)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```python
__le__(self: Self, b: bool, Self: type)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

@pure @llvm

```python
__le__(a: float, b: float)
```

Added by [`internal.types.float`](https://docs.exaloop.io/libraries/api/modules/internal/types/float/)

```llvm
%tmp = fcmp ole double %a, %b
%res = zext i1 %tmp to i8
ret i8 %res
```

\_\_ge\_\_[](https://docs.exaloop.io/libraries/api/classes/float/#class-float-__ge__ "Copy/link to this API element")

```python
__ge__(self: Self, b: float128, Self: type)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```python
__ge__(self: Self, b: float32, Self: type)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```python
__ge__(self: Self, b: float16, Self: type)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```python
__ge__(self: Self, b: bfloat16, Self: type)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```python
__ge__(self: Self, b: Int, Self: type)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```python
__ge__(self: Self, b: UInt, Self: type)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```python
__ge__(self: Self, b: bool, Self: type)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

@pure @llvm

```python
__ge__(a: float, b: float)
```

Added by [`internal.types.float`](https://docs.exaloop.io/libraries/api/modules/internal/types/float/)

```llvm
%tmp = fcmp oge double %a, %b
%res = zext i1 %tmp to i8
ret i8 %res
```

\_\_repr\_\_[](https://docs.exaloop.io/libraries/api/classes/float/#class-float-__repr__ "Copy/link to this API element")

```python
__repr__(self: Self, Self: type)
```

Added by [`internal.types.float`](https://docs.exaloop.io/libraries/api/modules/internal/types/float/)

\_\_copy\_\_[](https://docs.exaloop.io/libraries/api/classes/float/#class-float-__copy__ "Copy/link to this API element")

```python
__copy__(self: Self, Self: type)
```

Added by [`internal.types.float`](https://docs.exaloop.io/libraries/api/modules/internal/types/float/)

\_\_deepcopy\_\_[](https://docs.exaloop.io/libraries/api/classes/float/#class-float-__deepcopy__ "Copy/link to this API element")

```python
__deepcopy__(self: Self, Self: type)
```

Added by [`internal.types.float`](https://docs.exaloop.io/libraries/api/modules/internal/types/float/)

\_\_int\_\_[](https://docs.exaloop.io/libraries/api/classes/float/#class-float-__int__ "Copy/link to this API element")

@pure @llvm

```python
__int__(self: Self, Self: type)
```

Added by [`internal.types.float`](https://docs.exaloop.io/libraries/api/modules/internal/types/float/)

```llvm
%0 = fptosi double %self to i64
ret i64 %0
```

\_\_float\_\_[](https://docs.exaloop.io/libraries/api/classes/float/#class-float-__float__ "Copy/link to this API element")

```python
__float__(self: Self, Self: type)
```

Added by [`internal.types.float`](https://docs.exaloop.io/libraries/api/modules/internal/types/float/)

\_\_bool\_\_[](https://docs.exaloop.io/libraries/api/classes/float/#class-float-__bool__ "Copy/link to this API element")

@pure @llvm

```python
__bool__(self: Self, Self: type)
```

Added by [`internal.types.float`](https://docs.exaloop.io/libraries/api/modules/internal/types/float/)

```llvm
%0 = fcmp une double %self, 0.000000e+00
%1 = zext i1 %0 to i8
ret i8 %1
```

\_\_pos\_\_[](https://docs.exaloop.io/libraries/api/classes/float/#class-float-__pos__ "Copy/link to this API element")

```python
__pos__(self: Self, Self: type)
```

Added by [`internal.types.float`](https://docs.exaloop.io/libraries/api/modules/internal/types/float/)

\_\_neg\_\_[](https://docs.exaloop.io/libraries/api/classes/float/#class-float-__neg__ "Copy/link to this API element")

@pure @llvm

```python
__neg__(self: Self, Self: type)
```

Added by [`internal.types.float`](https://docs.exaloop.io/libraries/api/modules/internal/types/float/)

```llvm
%0 = fneg double %self
ret double %0
```

\_\_abs\_\_[](https://docs.exaloop.io/libraries/api/classes/float/#class-float-__abs__ "Copy/link to this API element")

@pure @llvm

```python
__abs__(a: float)
```

Added by [`internal.types.float`](https://docs.exaloop.io/libraries/api/modules/internal/types/float/)

```llvm
declare double @llvm.fabs.f64(double %a)
%tmp = call double @llvm.fabs.f64(double %a)
ret double %tmp
```

\_\_floor\_\_[](https://docs.exaloop.io/libraries/api/classes/float/#class-float-__floor__ "Copy/link to this API element")

@pure @llvm

```python
__floor__(a: float)
```

Added by [`internal.types.float`](https://docs.exaloop.io/libraries/api/modules/internal/types/float/)

```llvm
declare double @llvm.floor.f64(double %a)
%tmp = call double @llvm.floor.f64(double %a)
ret double %tmp
```

\_\_ceil\_\_[](https://docs.exaloop.io/libraries/api/classes/float/#class-float-__ceil__ "Copy/link to this API element")

@pure @llvm

```python
__ceil__(a: float)
```

Added by [`internal.types.float`](https://docs.exaloop.io/libraries/api/modules/internal/types/float/)

```llvm
declare double @llvm.ceil.f64(double %a)
%tmp = call double @llvm.ceil.f64(double %a)
ret double %tmp
```

\_\_trunc\_\_[](https://docs.exaloop.io/libraries/api/classes/float/#class-float-__trunc__ "Copy/link to this API element")

@pure @llvm

```python
__trunc__(a: float)
```

Added by [`internal.types.float`](https://docs.exaloop.io/libraries/api/modules/internal/types/float/)

```llvm
declare double @llvm.trunc.f64(double %a)
%tmp = call double @llvm.trunc.f64(double %a)
ret double %tmp
```

\_\_round\_\_[](https://docs.exaloop.io/libraries/api/classes/float/#class-float-__round__ "Copy/link to this API element")

```python
__round__(self: Self, ndigits = None, Self: type)
```

Added by [`internal.types.float`](https://docs.exaloop.io/libraries/api/modules/internal/types/float/)

\_\_atomic\_xchg\_\_[](https://docs.exaloop.io/libraries/api/classes/float/#class-float-__atomic_xchg__ "Copy/link to this API element")

@nocapture @llvm

```python
__atomic_xchg__(d: Ptr[float], b: float)
```

Added by [`internal.types.float`](https://docs.exaloop.io/libraries/api/modules/internal/types/float/)

```llvm
%tmp = atomicrmw xchg ptr %d, double %b seq_cst
ret {} {}
```

\_\_atomic\_add\_\_[](https://docs.exaloop.io/libraries/api/classes/float/#class-float-__atomic_add__ "Copy/link to this API element")

@nocapture @llvm

```python
__atomic_add__(d: Ptr[float], b: float)
```

Added by [`internal.types.float`](https://docs.exaloop.io/libraries/api/modules/internal/types/float/)

```llvm
0:
%1 = load atomic i64, ptr %d monotonic, align 8
%2 = bitcast i64 %1 to double
%3 = fadd double %2, %b
%4 = bitcast double %3 to i64
%5 = cmpxchg weak ptr %d, i64 %1, i64 %4 seq_cst monotonic, align 8
%6 = extractvalue { i64, i1 } %5, 1
br i1 %6, label %15, label %7
7:                                                ; preds = %0, %7
%8 = phi { i64, i1 } [ %13, %7 ], [ %5, %0 ]
%9 = extractvalue { i64, i1 } %8, 0
%10 = bitcast i64 %9 to double
%11 = fadd double %10, %b
%12 = bitcast double %11 to i64
%13 = cmpxchg weak ptr %d, i64 %9, i64 %12 seq_cst monotonic, align 8
%14 = extractvalue { i64, i1 } %13, 1
br i1 %14, label %15, label %7
15:                                               ; preds = %7, %0
%16 = phi double [ %2, %0 ], [ %10, %7 ]
ret double %16
```

\_\_atomic\_sub\_\_[](https://docs.exaloop.io/libraries/api/classes/float/#class-float-__atomic_sub__ "Copy/link to this API element")

@nocapture @llvm

```python
__atomic_sub__(d: Ptr[float], b: float)
```

Added by [`internal.types.float`](https://docs.exaloop.io/libraries/api/modules/internal/types/float/)

```llvm
0:
%1 = load atomic i64, ptr %d monotonic, align 8
%2 = bitcast i64 %1 to double
%3 = fsub double %2, %b
%4 = bitcast double %3 to i64
%5 = cmpxchg weak ptr %d, i64 %1, i64 %4 seq_cst monotonic, align 8
%6 = extractvalue { i64, i1 } %5, 1
br i1 %6, label %15, label %7
7:                                                ; preds = %0, %7
%8 = phi { i64, i1 } [ %13, %7 ], [ %5, %0 ]
%9 = extractvalue { i64, i1 } %8, 0
%10 = bitcast i64 %9 to double
%11 = fsub double %10, %b
%12 = bitcast double %11 to i64
%13 = cmpxchg weak ptr %d, i64 %9, i64 %12 seq_cst monotonic, align 8
%14 = extractvalue { i64, i1 } %13, 1
br i1 %14, label %15, label %7
15:                                               ; preds = %7, %0
%16 = phi double [ %2, %0 ], [ %10, %7 ]
ret double %16
```

\_\_hash\_\_[](https://docs.exaloop.io/libraries/api/classes/float/#class-float-__hash__ "Copy/link to this API element")

```python
__hash__(self: Self, Self: type)
```

Added by [`internal.types.float`](https://docs.exaloop.io/libraries/api/modules/internal/types/float/)

\_\_match\_\_[](https://docs.exaloop.io/libraries/api/classes/float/#class-float-__match__ "Copy/link to this API element")

```python
__match__(self: Self, obj: float, Self: type)
```

Added by [`internal.types.float`](https://docs.exaloop.io/libraries/api/modules/internal/types/float/)

\_\_suffix\_f32\_\_[](https://docs.exaloop.io/libraries/api/classes/float/#class-float-__suffix_f32__ "Copy/link to this API element")

```python
__suffix_f32__(double)
```

Added by [`internal.types.float`](https://docs.exaloop.io/libraries/api/modules/internal/types/float/)

\_\_suffix\_f16\_\_[](https://docs.exaloop.io/libraries/api/classes/float/#class-float-__suffix_f16__ "Copy/link to this API element")

```python
__suffix_f16__(double)
```

Added by [`internal.types.float`](https://docs.exaloop.io/libraries/api/modules/internal/types/float/)

\_\_suffix\_bf16\_\_[](https://docs.exaloop.io/libraries/api/classes/float/#class-float-__suffix_bf16__ "Copy/link to this API element")

```python
__suffix_bf16__(double)
```

Added by [`internal.types.float`](https://docs.exaloop.io/libraries/api/modules/internal/types/float/)

\_\_suffix\_f128\_\_[](https://docs.exaloop.io/libraries/api/classes/float/#class-float-__suffix_f128__ "Copy/link to this API element")

```python
__suffix_f128__(double)
```

Added by [`internal.types.float`](https://docs.exaloop.io/libraries/api/modules/internal/types/float/)

\_\_to\_py\_\_[](https://docs.exaloop.io/libraries/api/classes/float/#class-float-__to_py__ "Copy/link to this API element")

```python
__to_py__(self: Self, Self: type)
```

Added by [`internal.python`](https://docs.exaloop.io/libraries/api/modules/internal/python/)

\_\_from\_py\_\_[](https://docs.exaloop.io/libraries/api/classes/float/#class-float-__from_py__ "Copy/link to this API element")

```python
__from_py__(d: cobj)
```

Added by [`internal.python`](https://docs.exaloop.io/libraries/api/modules/internal/python/)

\_\_suffix\_j\_\_[](https://docs.exaloop.io/libraries/api/classes/float/#class-float-__suffix_j__ "Copy/link to this API element")

```python
__suffix_j__(x: float)
```

Added by [`internal.types.complex`](https://docs.exaloop.io/libraries/api/modules/internal/types/complex/)

\_\_complex\_\_[](https://docs.exaloop.io/libraries/api/classes/float/#class-float-__complex__ "Copy/link to this API element")

```python
__complex__(self: Self, Self: type)
```

Added by [`internal.types.complex`](https://docs.exaloop.io/libraries/api/modules/internal/types/complex/)

\_\_str\_\_[](https://docs.exaloop.io/libraries/api/classes/float/#class-float-__str__ "Copy/link to this API element")

```python
__str__(self: Self, Self: type)
```

Added by [`internal.format`](https://docs.exaloop.io/libraries/api/modules/internal/format/)

\_\_format\_\_[](https://docs.exaloop.io/libraries/api/classes/float/#class-float-__format__ "Copy/link to this API element")

```python
__format__(self: Self, format_spec: str, Self: type)
```

Added by [`internal.format`](https://docs.exaloop.io/libraries/api/modules/internal/format/)

\_\_pickle\_\_[](https://docs.exaloop.io/libraries/api/classes/float/#class-float-__pickle__ "Copy/link to this API element")

```python
__pickle__(self: Self, jar: Jar, Self: type)
```

Added by [`pickle`](https://docs.exaloop.io/libraries/api/modules/pickle/)

\_\_unpickle\_\_[](https://docs.exaloop.io/libraries/api/classes/float/#class-float-__unpickle__ "Copy/link to this API element")

```python
__unpickle__(jar: Jar)
```

Added by [`pickle`](https://docs.exaloop.io/libraries/api/modules/pickle/)

\_\_to\_gpu\_\_[](https://docs.exaloop.io/libraries/api/classes/float/#class-float-__to_gpu__ "Copy/link to this API element")

```python
__to_gpu__(self: Self, cache: AllocCache, Self: type)
```

Added by [`internal.gpu`](https://docs.exaloop.io/libraries/api/modules/internal/gpu/)

\_\_from\_gpu\_\_[](https://docs.exaloop.io/libraries/api/classes/float/#class-float-__from_gpu__ "Copy/link to this API element")

```python
__from_gpu__(self: Self, other: float, Self: type)
```

Added by [`internal.gpu`](https://docs.exaloop.io/libraries/api/modules/internal/gpu/)

\_\_from\_gpu\_new\_\_[](https://docs.exaloop.io/libraries/api/classes/float/#class-float-__from_gpu_new__ "Copy/link to this API element")

```python
__from_gpu_new__(other: float)
```

Added by [`internal.gpu`](https://docs.exaloop.io/libraries/api/modules/internal/gpu/)

Source: https://docs.exaloop.io/libraries/api/classes/float/
