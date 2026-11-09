# float32

Standard library class

class float32[](https://docs.exaloop.io/libraries/api/classes/float32/#class-float32 "Copy/link to this API element")

@tuple

```python
class float32
```

Constructors 11

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/classes/float32/#class-float32-__new__ "Copy/link to this API element")

```python
__new__(what)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/classes/float32/#class-float32-__new__-overload-2 "Copy/link to this API element")

```python
__new__(what: bool)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/classes/float32/#class-float32-__new__-overload-3 "Copy/link to this API element")

@pure @llvm

```python
__new__(what: Int[N], N: Literal[int])
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```llvm
%0 = sitofp i{=N} %what to float
ret float %0
```

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/classes/float32/#class-float32-__new__-overload-4 "Copy/link to this API element")

@pure @llvm

```python
__new__(what: UInt[N], N: Literal[int])
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```llvm
%0 = uitofp i{=N} %what to float
ret float %0
```

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/classes/float32/#class-float32-__new__-overload-5 "Copy/link to this API element")

@pure @llvm

```python
__new__(what: float128)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```llvm
%0 = fptrunc fp128 %what to float
ret float %0
```

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/classes/float32/#class-float32-__new__-overload-6 "Copy/link to this API element")

@pure @llvm

```python
__new__(what: float16)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```llvm
%0 = fpext half %what to float
ret float %0
```

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/classes/float32/#class-float32-__new__-overload-7 "Copy/link to this API element")

@pure @llvm

```python
__new__(what: bfloat16)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```llvm
%0 = fpext bfloat %what to float
ret float %0
```

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/classes/float32/#class-float32-__new__-overload-8 "Copy/link to this API element")

@pure @llvm

```python
__new__(self: float)
```

Added by [`internal.types.float`](https://docs.exaloop.io/libraries/api/modules/internal/types/float/)

```llvm
%0 = fptrunc double %self to float
ret float %0
```

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/classes/float32/#class-float32-__new__-overload-9 "Copy/link to this API element")

@overload

```python
__new__(what: float32)
```

Added by [`internal.types.float`](https://docs.exaloop.io/libraries/api/modules/internal/types/float/)

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/classes/float32/#class-float32-__new__-overload-10 "Copy/link to this API element")

@overload

```python
__new__(what: str)
```

Added by [`internal.types.float`](https://docs.exaloop.io/libraries/api/modules/internal/types/float/)

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/classes/float32/#class-float32-__new__-overload-11 "Copy/link to this API element")

@overload

```python
__new__()
```

Added by [`internal.types.float`](https://docs.exaloop.io/libraries/api/modules/internal/types/float/)

Methods 2

cdiv[](https://docs.exaloop.io/libraries/api/classes/float32/#class-float32-cdiv "Copy/link to this API element")

```python
cdiv(self: Self, other: float32, Self: type)
```

Added by [`internal.types.float`](https://docs.exaloop.io/libraries/api/modules/internal/types/float/)

cmod[](https://docs.exaloop.io/libraries/api/classes/float32/#class-float32-cmod "Copy/link to this API element")

```python
cmod(self: Self, other: float32, Self: type)
```

Added by [`internal.types.float`](https://docs.exaloop.io/libraries/api/modules/internal/types/float/)

Special methods 38

\_\_add\_\_[](https://docs.exaloop.io/libraries/api/classes/float32/#class-float32-__add__ "Copy/link to this API element")

@commutative

```python
__add__(self: Self, b: float, Self: type)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```python
__add__(self: Self, b: float128, Self: type)
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
__add__(a: float32, b: float32)
```

Added by [`internal.types.float`](https://docs.exaloop.io/libraries/api/modules/internal/types/float/)

```llvm
%tmp = fadd float %a, %b
ret float %tmp
```

\_\_sub\_\_[](https://docs.exaloop.io/libraries/api/classes/float32/#class-float32-__sub__ "Copy/link to this API element")

```python
__sub__(self: Self, b: float, Self: type)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```python
__sub__(self: Self, b: float128, Self: type)
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
__sub__(a: float32, b: float32)
```

Added by [`internal.types.float`](https://docs.exaloop.io/libraries/api/modules/internal/types/float/)

```llvm
%tmp = fsub float %a, %b
ret float %tmp
```

\_\_mul\_\_[](https://docs.exaloop.io/libraries/api/classes/float32/#class-float32-__mul__ "Copy/link to this API element")

@commutative

```python
__mul__(self: Self, b: float, Self: type)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```python
__mul__(self: Self, b: float128, Self: type)
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
__mul__(a: float32, b: float32)
```

Added by [`internal.types.float`](https://docs.exaloop.io/libraries/api/modules/internal/types/float/)

```llvm
%tmp = fmul float %a, %b
ret float %tmp
```

\_\_floordiv\_\_[](https://docs.exaloop.io/libraries/api/classes/float32/#class-float32-__floordiv__ "Copy/link to this API element")

```python
__floordiv__(self: Self, b: float, Self: type)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```python
__floordiv__(self: Self, b: float128, Self: type)
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
__floordiv__(self: Self, other: float32, Self: type)
```

Added by [`internal.types.float`](https://docs.exaloop.io/libraries/api/modules/internal/types/float/)

\_\_truediv\_\_[](https://docs.exaloop.io/libraries/api/classes/float32/#class-float32-__truediv__ "Copy/link to this API element")

```python
__truediv__(self: Self, b: float, Self: type)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```python
__truediv__(self: Self, b: float128, Self: type)
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
__truediv__(self: Self, other: float32, Self: type)
```

Added by [`internal.types.float`](https://docs.exaloop.io/libraries/api/modules/internal/types/float/)

\_\_mod\_\_[](https://docs.exaloop.io/libraries/api/classes/float32/#class-float32-__mod__ "Copy/link to this API element")

```python
__mod__(self: Self, b: float, Self: type)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```python
__mod__(self: Self, b: float128, Self: type)
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
__mod__(self: Self, other: float32, Self: type)
```

Added by [`internal.types.float`](https://docs.exaloop.io/libraries/api/modules/internal/types/float/)

\_\_divmod\_\_[](https://docs.exaloop.io/libraries/api/classes/float32/#class-float32-__divmod__ "Copy/link to this API element")

```python
__divmod__(self: Self, b: float, Self: type)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```python
__divmod__(self: Self, b: float128, Self: type)
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
__divmod__(self: Self, other: float32, Self: type)
```

Added by [`internal.types.float`](https://docs.exaloop.io/libraries/api/modules/internal/types/float/)

\_\_pow\_\_[](https://docs.exaloop.io/libraries/api/classes/float32/#class-float32-__pow__ "Copy/link to this API element")

```python
__pow__(self: Self, b: float, Self: type)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```python
__pow__(self: Self, b: float128, Self: type)
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
__pow__(a: float32, b: float32)
```

Added by [`internal.types.float`](https://docs.exaloop.io/libraries/api/modules/internal/types/float/)

```llvm
declare float @llvm.pow.f32(float %a, float %b)
%tmp = call float @llvm.pow.f32(float %a, float %b)
ret float %tmp
```

\_\_eq\_\_[](https://docs.exaloop.io/libraries/api/classes/float32/#class-float32-__eq__ "Copy/link to this API element")

```python
__eq__(self: Self, b: float, Self: type)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```python
__eq__(self: Self, b: float128, Self: type)
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
__eq__(a: float32, b: float32)
```

Added by [`internal.types.float`](https://docs.exaloop.io/libraries/api/modules/internal/types/float/)

```llvm
%tmp = fcmp oeq float %a, %b
%res = zext i1 %tmp to i8
ret i8 %res
```

\_\_ne\_\_[](https://docs.exaloop.io/libraries/api/classes/float32/#class-float32-__ne__ "Copy/link to this API element")

```python
__ne__(self: Self, b: float, Self: type)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```python
__ne__(self: Self, b: float128, Self: type)
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
__ne__(a: float32, b: float32)
```

Added by [`internal.types.float`](https://docs.exaloop.io/libraries/api/modules/internal/types/float/)

```llvm
%tmp = fcmp une float %a, %b
%res = zext i1 %tmp to i8
ret i8 %res
```

\_\_lt\_\_[](https://docs.exaloop.io/libraries/api/classes/float32/#class-float32-__lt__ "Copy/link to this API element")

```python
__lt__(self: Self, b: float, Self: type)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```python
__lt__(self: Self, b: float128, Self: type)
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
__lt__(a: float32, b: float32)
```

Added by [`internal.types.float`](https://docs.exaloop.io/libraries/api/modules/internal/types/float/)

```llvm
%tmp = fcmp olt float %a, %b
%res = zext i1 %tmp to i8
ret i8 %res
```

\_\_gt\_\_[](https://docs.exaloop.io/libraries/api/classes/float32/#class-float32-__gt__ "Copy/link to this API element")

```python
__gt__(self: Self, b: float, Self: type)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```python
__gt__(self: Self, b: float128, Self: type)
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
__gt__(a: float32, b: float32)
```

Added by [`internal.types.float`](https://docs.exaloop.io/libraries/api/modules/internal/types/float/)

```llvm
%tmp = fcmp ogt float %a, %b
%res = zext i1 %tmp to i8
ret i8 %res
```

\_\_le\_\_[](https://docs.exaloop.io/libraries/api/classes/float32/#class-float32-__le__ "Copy/link to this API element")

```python
__le__(self: Self, b: float, Self: type)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```python
__le__(self: Self, b: float128, Self: type)
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
__le__(a: float32, b: float32)
```

Added by [`internal.types.float`](https://docs.exaloop.io/libraries/api/modules/internal/types/float/)

```llvm
%tmp = fcmp ole float %a, %b
%res = zext i1 %tmp to i8
ret i8 %res
```

\_\_ge\_\_[](https://docs.exaloop.io/libraries/api/classes/float32/#class-float32-__ge__ "Copy/link to this API element")

```python
__ge__(self: Self, b: float, Self: type)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```python
__ge__(self: Self, b: float128, Self: type)
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
__ge__(a: float32, b: float32)
```

Added by [`internal.types.float`](https://docs.exaloop.io/libraries/api/modules/internal/types/float/)

```llvm
%tmp = fcmp oge float %a, %b
%res = zext i1 %tmp to i8
ret i8 %res
```

\_\_repr\_\_[](https://docs.exaloop.io/libraries/api/classes/float32/#class-float32-__repr__ "Copy/link to this API element")

```python
__repr__(self: Self, Self: type)
```

Added by [`internal.types.float`](https://docs.exaloop.io/libraries/api/modules/internal/types/float/)

\_\_format\_\_[](https://docs.exaloop.io/libraries/api/classes/float32/#class-float32-__format__ "Copy/link to this API element")

```python
__format__(self: Self, format_spec: str, Self: type)
```

Added by [`internal.types.float`](https://docs.exaloop.io/libraries/api/modules/internal/types/float/)

\_\_copy\_\_[](https://docs.exaloop.io/libraries/api/classes/float32/#class-float32-__copy__ "Copy/link to this API element")

```python
__copy__(self: Self, Self: type)
```

Added by [`internal.types.float`](https://docs.exaloop.io/libraries/api/modules/internal/types/float/)

\_\_deepcopy\_\_[](https://docs.exaloop.io/libraries/api/classes/float32/#class-float32-__deepcopy__ "Copy/link to this API element")

```python
__deepcopy__(self: Self, Self: type)
```

Added by [`internal.types.float`](https://docs.exaloop.io/libraries/api/modules/internal/types/float/)

\_\_int\_\_[](https://docs.exaloop.io/libraries/api/classes/float32/#class-float32-__int__ "Copy/link to this API element")

@pure @llvm

```python
__int__(self: Self, Self: type)
```

Added by [`internal.types.float`](https://docs.exaloop.io/libraries/api/modules/internal/types/float/)

```llvm
%0 = fptosi float %self to i64
ret i64 %0
```

\_\_float\_\_[](https://docs.exaloop.io/libraries/api/classes/float32/#class-float32-__float__ "Copy/link to this API element")

@pure @llvm

```python
__float__(self: Self, Self: type)
```

Added by [`internal.types.float`](https://docs.exaloop.io/libraries/api/modules/internal/types/float/)

```llvm
%0 = fpext float %self to double
ret double %0
```

\_\_bool\_\_[](https://docs.exaloop.io/libraries/api/classes/float32/#class-float32-__bool__ "Copy/link to this API element")

@pure @llvm

```python
__bool__(self: Self, Self: type)
```

Added by [`internal.types.float`](https://docs.exaloop.io/libraries/api/modules/internal/types/float/)

```llvm
%0 = fcmp une float %self, 0.000000e+00
%1 = zext i1 %0 to i8
ret i8 %1
```

\_\_pos\_\_[](https://docs.exaloop.io/libraries/api/classes/float32/#class-float32-__pos__ "Copy/link to this API element")

```python
__pos__(self: Self, Self: type)
```

Added by [`internal.types.float`](https://docs.exaloop.io/libraries/api/modules/internal/types/float/)

\_\_neg\_\_[](https://docs.exaloop.io/libraries/api/classes/float32/#class-float32-__neg__ "Copy/link to this API element")

@pure @llvm

```python
__neg__(self: Self, Self: type)
```

Added by [`internal.types.float`](https://docs.exaloop.io/libraries/api/modules/internal/types/float/)

```llvm
%0 = fneg float %self
ret float %0
```

\_\_abs\_\_[](https://docs.exaloop.io/libraries/api/classes/float32/#class-float32-__abs__ "Copy/link to this API element")

@pure @llvm

```python
__abs__(a: float32)
```

Added by [`internal.types.float`](https://docs.exaloop.io/libraries/api/modules/internal/types/float/)

```llvm
declare float @llvm.fabs.f32(float %a)
%tmp = call float @llvm.fabs.f32(float %a)
ret float %tmp
```

\_\_floor\_\_[](https://docs.exaloop.io/libraries/api/classes/float32/#class-float32-__floor__ "Copy/link to this API element")

@pure @llvm

```python
__floor__(a: float32)
```

Added by [`internal.types.float`](https://docs.exaloop.io/libraries/api/modules/internal/types/float/)

```llvm
declare float @llvm.floor.f32(float %a)
%tmp = call float @llvm.floor.f32(float %a)
ret float %tmp
```

\_\_ceil\_\_[](https://docs.exaloop.io/libraries/api/classes/float32/#class-float32-__ceil__ "Copy/link to this API element")

@pure @llvm

```python
__ceil__(a: float32)
```

Added by [`internal.types.float`](https://docs.exaloop.io/libraries/api/modules/internal/types/float/)

```llvm
declare float @llvm.ceil.f32(float %a)
%tmp = call float @llvm.ceil.f32(float %a)
ret float %tmp
```

\_\_trunc\_\_[](https://docs.exaloop.io/libraries/api/classes/float32/#class-float32-__trunc__ "Copy/link to this API element")

@pure @llvm

```python
__trunc__(a: float32)
```

Added by [`internal.types.float`](https://docs.exaloop.io/libraries/api/modules/internal/types/float/)

```llvm
declare float @llvm.trunc.f32(float %a)
%tmp = call float @llvm.trunc.f32(float %a)
ret float %tmp
```

\_\_atomic\_xchg\_\_[](https://docs.exaloop.io/libraries/api/classes/float32/#class-float32-__atomic_xchg__ "Copy/link to this API element")

@nocapture @llvm

```python
__atomic_xchg__(d: Ptr[float32], b: float32)
```

Added by [`internal.types.float`](https://docs.exaloop.io/libraries/api/modules/internal/types/float/)

```llvm
%tmp = atomicrmw xchg ptr %d, float %b seq_cst
ret {} {}
```

\_\_atomic\_add\_\_[](https://docs.exaloop.io/libraries/api/classes/float32/#class-float32-__atomic_add__ "Copy/link to this API element")

@nocapture @llvm

```python
__atomic_add__(d: Ptr[float32], b: float32)
```

Added by [`internal.types.float`](https://docs.exaloop.io/libraries/api/modules/internal/types/float/)

```llvm
0:
%1 = load atomic i32, ptr %d monotonic, align 4
%2 = bitcast i32 %1 to float
%3 = fadd float %2, %b
%4 = bitcast float %3 to i32
%5 = cmpxchg weak ptr %d, i32 %1, i32 %4 seq_cst monotonic, align 4
%6 = extractvalue { i32, i1 } %5, 1
br i1 %6, label %15, label %7
7:                                                ; preds = %0, %7
%8 = phi { i32, i1 } [ %13, %7 ], [ %5, %0 ]
%9 = extractvalue { i32, i1 } %8, 0
%10 = bitcast i32 %9 to float
%11 = fadd float %10, %b
%12 = bitcast float %11 to i32
%13 = cmpxchg weak ptr %d, i32 %9, i32 %12 seq_cst monotonic, align 4
%14 = extractvalue { i32, i1 } %13, 1
br i1 %14, label %15, label %7
15:                                               ; preds = %7, %0
%16 = phi float [ %2, %0 ], [ %10, %7 ]
ret float %16
```

\_\_atomic\_sub\_\_[](https://docs.exaloop.io/libraries/api/classes/float32/#class-float32-__atomic_sub__ "Copy/link to this API element")

@nocapture @llvm

```python
__atomic_sub__(d: Ptr[float32], b: float32)
```

Added by [`internal.types.float`](https://docs.exaloop.io/libraries/api/modules/internal/types/float/)

```llvm
0:
%1 = load atomic i32, ptr %d monotonic, align 4
%2 = bitcast i32 %1 to float
%3 = fsub float %2, %b
%4 = bitcast float %3 to i32
%5 = cmpxchg weak ptr %d, i32 %1, i32 %4 seq_cst monotonic, align 4
%6 = extractvalue { i32, i1 } %5, 1
br i1 %6, label %15, label %7
7:                                                ; preds = %0, %7
%8 = phi { i32, i1 } [ %13, %7 ], [ %5, %0 ]
%9 = extractvalue { i32, i1 } %8, 0
%10 = bitcast i32 %9 to float
%11 = fsub float %10, %b
%12 = bitcast float %11 to i32
%13 = cmpxchg weak ptr %d, i32 %9, i32 %12 seq_cst monotonic, align 4
%14 = extractvalue { i32, i1 } %13, 1
br i1 %14, label %15, label %7
15:                                               ; preds = %7, %0
%16 = phi float [ %2, %0 ], [ %10, %7 ]
ret float %16
```

\_\_hash\_\_[](https://docs.exaloop.io/libraries/api/classes/float32/#class-float32-__hash__ "Copy/link to this API element")

```python
__hash__(self: Self, Self: type)
```

Added by [`internal.types.float`](https://docs.exaloop.io/libraries/api/modules/internal/types/float/)

\_\_match\_\_[](https://docs.exaloop.io/libraries/api/classes/float32/#class-float32-__match__ "Copy/link to this API element")

```python
__match__(self: Self, obj: float32, Self: type)
```

Added by [`internal.types.float`](https://docs.exaloop.io/libraries/api/modules/internal/types/float/)

\_\_complex\_\_[](https://docs.exaloop.io/libraries/api/classes/float32/#class-float32-__complex__ "Copy/link to this API element")

```python
__complex__(self: Self, Self: type)
```

Added by [`internal.types.complex`](https://docs.exaloop.io/libraries/api/modules/internal/types/complex/)

\_\_pickle\_\_[](https://docs.exaloop.io/libraries/api/classes/float32/#class-float32-__pickle__ "Copy/link to this API element")

```python
__pickle__(self: Self, jar: Jar, Self: type)
```

Added by [`pickle`](https://docs.exaloop.io/libraries/api/modules/pickle/)

\_\_unpickle\_\_[](https://docs.exaloop.io/libraries/api/classes/float32/#class-float32-__unpickle__ "Copy/link to this API element")

```python
__unpickle__(jar: Jar)
```

Added by [`pickle`](https://docs.exaloop.io/libraries/api/modules/pickle/)

\_\_to\_gpu\_\_[](https://docs.exaloop.io/libraries/api/classes/float32/#class-float32-__to_gpu__ "Copy/link to this API element")

```python
__to_gpu__(self: Self, cache: AllocCache, Self: type)
```

Added by [`internal.gpu`](https://docs.exaloop.io/libraries/api/modules/internal/gpu/)

\_\_from\_gpu\_\_[](https://docs.exaloop.io/libraries/api/classes/float32/#class-float32-__from_gpu__ "Copy/link to this API element")

```python
__from_gpu__(self: Self, other: float32, Self: type)
```

Added by [`internal.gpu`](https://docs.exaloop.io/libraries/api/modules/internal/gpu/)

\_\_from\_gpu\_new\_\_[](https://docs.exaloop.io/libraries/api/classes/float32/#class-float32-__from_gpu_new__ "Copy/link to this API element")

```python
__from_gpu_new__(other: float32)
```

Added by [`internal.gpu`](https://docs.exaloop.io/libraries/api/modules/internal/gpu/)

Source: https://docs.exaloop.io/libraries/api/classes/float32/
