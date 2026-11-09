# float128

Standard library class

class float128[](https://docs.exaloop.io/libraries/api/classes/float128/#class-float128 "Copy/link to this API element")

@tuple

```python
class float128
```

Constructors 10

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/classes/float128/#class-float128-__new__ "Copy/link to this API element")

```python
__new__(what)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/classes/float128/#class-float128-__new__-overload-2 "Copy/link to this API element")

```python
__new__(what: bool)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/classes/float128/#class-float128-__new__-overload-3 "Copy/link to this API element")

@pure @llvm

```python
__new__(what: Int[N], N: Literal[int])
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```llvm
%0 = sitofp i{=N} %what to fp128
ret fp128 %0
```

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/classes/float128/#class-float128-__new__-overload-4 "Copy/link to this API element")

@pure @llvm

```python
__new__(what: UInt[N], N: Literal[int])
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```llvm
%0 = uitofp i{=N} %what to fp128
ret fp128 %0
```

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/classes/float128/#class-float128-__new__-overload-5 "Copy/link to this API element")

@pure @llvm

```python
__new__(what: float32)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```llvm
%0 = fpext float %what to fp128
ret fp128 %0
```

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/classes/float128/#class-float128-__new__-overload-6 "Copy/link to this API element")

@pure @llvm

```python
__new__(what: float16)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```llvm
%0 = fpext half %what to fp128
ret fp128 %0
```

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/classes/float128/#class-float128-__new__-overload-7 "Copy/link to this API element")

@pure @llvm

```python
__new__(what: bfloat16)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```llvm
%0 = fpext bfloat %what to fp128
ret fp128 %0
```

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/classes/float128/#class-float128-__new__-overload-8 "Copy/link to this API element")

@pure @llvm

```python
__new__(self: float)
```

Added by [`internal.types.float`](https://docs.exaloop.io/libraries/api/modules/internal/types/float/)

```llvm
%0 = fpext double %self to fp128
ret fp128 %0
```

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/classes/float128/#class-float128-__new__-overload-9 "Copy/link to this API element")

@overload

```python
__new__(what: float128)
```

Added by [`internal.types.float`](https://docs.exaloop.io/libraries/api/modules/internal/types/float/)

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/classes/float128/#class-float128-__new__-overload-10 "Copy/link to this API element")

@overload

```python
__new__()
```

Added by [`internal.types.float`](https://docs.exaloop.io/libraries/api/modules/internal/types/float/)

Methods 2

cdiv[](https://docs.exaloop.io/libraries/api/classes/float128/#class-float128-cdiv "Copy/link to this API element")

```python
cdiv(self: Self, other: float128, Self: type)
```

Added by [`internal.types.float`](https://docs.exaloop.io/libraries/api/modules/internal/types/float/)

cmod[](https://docs.exaloop.io/libraries/api/classes/float128/#class-float128-cmod "Copy/link to this API element")

```python
cmod(self: Self, other: float128, Self: type)
```

Added by [`internal.types.float`](https://docs.exaloop.io/libraries/api/modules/internal/types/float/)

Special methods 32

\_\_add\_\_[](https://docs.exaloop.io/libraries/api/classes/float128/#class-float128-__add__ "Copy/link to this API element")

@commutative

```python
__add__(self: Self, b: float, Self: type)
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
__add__(a: float128, b: float128)
```

Added by [`internal.types.float`](https://docs.exaloop.io/libraries/api/modules/internal/types/float/)

```llvm
%tmp = fadd fp128 %a, %b
ret fp128 %tmp
```

\_\_sub\_\_[](https://docs.exaloop.io/libraries/api/classes/float128/#class-float128-__sub__ "Copy/link to this API element")

```python
__sub__(self: Self, b: float, Self: type)
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
__sub__(a: float128, b: float128)
```

Added by [`internal.types.float`](https://docs.exaloop.io/libraries/api/modules/internal/types/float/)

```llvm
%tmp = fsub fp128 %a, %b
ret fp128 %tmp
```

\_\_mul\_\_[](https://docs.exaloop.io/libraries/api/classes/float128/#class-float128-__mul__ "Copy/link to this API element")

@commutative

```python
__mul__(self: Self, b: float, Self: type)
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
__mul__(a: float128, b: float128)
```

Added by [`internal.types.float`](https://docs.exaloop.io/libraries/api/modules/internal/types/float/)

```llvm
%tmp = fmul fp128 %a, %b
ret fp128 %tmp
```

\_\_floordiv\_\_[](https://docs.exaloop.io/libraries/api/classes/float128/#class-float128-__floordiv__ "Copy/link to this API element")

```python
__floordiv__(self: Self, b: float, Self: type)
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
__floordiv__(self: Self, other: float128, Self: type)
```

Added by [`internal.types.float`](https://docs.exaloop.io/libraries/api/modules/internal/types/float/)

\_\_truediv\_\_[](https://docs.exaloop.io/libraries/api/classes/float128/#class-float128-__truediv__ "Copy/link to this API element")

```python
__truediv__(self: Self, b: float, Self: type)
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
__truediv__(self: Self, other: float128, Self: type)
```

Added by [`internal.types.float`](https://docs.exaloop.io/libraries/api/modules/internal/types/float/)

\_\_mod\_\_[](https://docs.exaloop.io/libraries/api/classes/float128/#class-float128-__mod__ "Copy/link to this API element")

```python
__mod__(self: Self, b: float, Self: type)
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
__mod__(self: Self, other: float128, Self: type)
```

Added by [`internal.types.float`](https://docs.exaloop.io/libraries/api/modules/internal/types/float/)

\_\_divmod\_\_[](https://docs.exaloop.io/libraries/api/classes/float128/#class-float128-__divmod__ "Copy/link to this API element")

```python
__divmod__(self: Self, b: float, Self: type)
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
__divmod__(self: Self, other: float128, Self: type)
```

Added by [`internal.types.float`](https://docs.exaloop.io/libraries/api/modules/internal/types/float/)

\_\_pow\_\_[](https://docs.exaloop.io/libraries/api/classes/float128/#class-float128-__pow__ "Copy/link to this API element")

```python
__pow__(self: Self, b: float, Self: type)
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
__pow__(a: float128, b: float128)
```

Added by [`internal.types.float`](https://docs.exaloop.io/libraries/api/modules/internal/types/float/)

```llvm
declare fp128 @llvm.pow.f128(fp128 %a, fp128 %b)
%tmp = call fp128 @llvm.pow.f128(fp128 %a, fp128 %b)
ret fp128 %tmp
```

\_\_eq\_\_[](https://docs.exaloop.io/libraries/api/classes/float128/#class-float128-__eq__ "Copy/link to this API element")

```python
__eq__(self: Self, b: float, Self: type)
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
__eq__(a: float128, b: float128)
```

Added by [`internal.types.float`](https://docs.exaloop.io/libraries/api/modules/internal/types/float/)

```llvm
%tmp = fcmp oeq fp128 %a, %b
%res = zext i1 %tmp to i8
ret i8 %res
```

\_\_ne\_\_[](https://docs.exaloop.io/libraries/api/classes/float128/#class-float128-__ne__ "Copy/link to this API element")

```python
__ne__(self: Self, b: float, Self: type)
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
__ne__(a: float128, b: float128)
```

Added by [`internal.types.float`](https://docs.exaloop.io/libraries/api/modules/internal/types/float/)

```llvm
%tmp = fcmp une fp128 %a, %b
%res = zext i1 %tmp to i8
ret i8 %res
```

\_\_lt\_\_[](https://docs.exaloop.io/libraries/api/classes/float128/#class-float128-__lt__ "Copy/link to this API element")

```python
__lt__(self: Self, b: float, Self: type)
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
__lt__(a: float128, b: float128)
```

Added by [`internal.types.float`](https://docs.exaloop.io/libraries/api/modules/internal/types/float/)

```llvm
%tmp = fcmp olt fp128 %a, %b
%res = zext i1 %tmp to i8
ret i8 %res
```

\_\_gt\_\_[](https://docs.exaloop.io/libraries/api/classes/float128/#class-float128-__gt__ "Copy/link to this API element")

```python
__gt__(self: Self, b: float, Self: type)
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
__gt__(a: float128, b: float128)
```

Added by [`internal.types.float`](https://docs.exaloop.io/libraries/api/modules/internal/types/float/)

```llvm
%tmp = fcmp ogt fp128 %a, %b
%res = zext i1 %tmp to i8
ret i8 %res
```

\_\_le\_\_[](https://docs.exaloop.io/libraries/api/classes/float128/#class-float128-__le__ "Copy/link to this API element")

```python
__le__(self: Self, b: float, Self: type)
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
__le__(a: float128, b: float128)
```

Added by [`internal.types.float`](https://docs.exaloop.io/libraries/api/modules/internal/types/float/)

```llvm
%tmp = fcmp ole fp128 %a, %b
%res = zext i1 %tmp to i8
ret i8 %res
```

\_\_ge\_\_[](https://docs.exaloop.io/libraries/api/classes/float128/#class-float128-__ge__ "Copy/link to this API element")

```python
__ge__(self: Self, b: float, Self: type)
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
__ge__(a: float128, b: float128)
```

Added by [`internal.types.float`](https://docs.exaloop.io/libraries/api/modules/internal/types/float/)

```llvm
%tmp = fcmp oge fp128 %a, %b
%res = zext i1 %tmp to i8
ret i8 %res
```

\_\_repr\_\_[](https://docs.exaloop.io/libraries/api/classes/float128/#class-float128-__repr__ "Copy/link to this API element")

```python
__repr__(self: Self, Self: type)
```

Added by [`internal.types.float`](https://docs.exaloop.io/libraries/api/modules/internal/types/float/)

\_\_format\_\_[](https://docs.exaloop.io/libraries/api/classes/float128/#class-float128-__format__ "Copy/link to this API element")

```python
__format__(self: Self, format_spec: str, Self: type)
```

Added by [`internal.types.float`](https://docs.exaloop.io/libraries/api/modules/internal/types/float/)

\_\_copy\_\_[](https://docs.exaloop.io/libraries/api/classes/float128/#class-float128-__copy__ "Copy/link to this API element")

```python
__copy__(self: Self, Self: type)
```

Added by [`internal.types.float`](https://docs.exaloop.io/libraries/api/modules/internal/types/float/)

\_\_deepcopy\_\_[](https://docs.exaloop.io/libraries/api/classes/float128/#class-float128-__deepcopy__ "Copy/link to this API element")

```python
__deepcopy__(self: Self, Self: type)
```

Added by [`internal.types.float`](https://docs.exaloop.io/libraries/api/modules/internal/types/float/)

\_\_int\_\_[](https://docs.exaloop.io/libraries/api/classes/float128/#class-float128-__int__ "Copy/link to this API element")

@pure @llvm

```python
__int__(self: Self, Self: type)
```

Added by [`internal.types.float`](https://docs.exaloop.io/libraries/api/modules/internal/types/float/)

```llvm
%0 = fptosi fp128 %self to i64
ret i64 %0
```

\_\_float\_\_[](https://docs.exaloop.io/libraries/api/classes/float128/#class-float128-__float__ "Copy/link to this API element")

@pure @llvm

```python
__float__(self: Self, Self: type)
```

Added by [`internal.types.float`](https://docs.exaloop.io/libraries/api/modules/internal/types/float/)

```llvm
%0 = fptrunc fp128 %self to double
ret double %0
```

\_\_bool\_\_[](https://docs.exaloop.io/libraries/api/classes/float128/#class-float128-__bool__ "Copy/link to this API element")

@pure @llvm

```python
__bool__(self: Self, Self: type)
```

Added by [`internal.types.float`](https://docs.exaloop.io/libraries/api/modules/internal/types/float/)

```llvm
%0 = fcmp une fp128 %self, 0xL00000000000000000000000000000000
%1 = zext i1 %0 to i8
ret i8 %1
```

\_\_pos\_\_[](https://docs.exaloop.io/libraries/api/classes/float128/#class-float128-__pos__ "Copy/link to this API element")

```python
__pos__(self: Self, Self: type)
```

Added by [`internal.types.float`](https://docs.exaloop.io/libraries/api/modules/internal/types/float/)

\_\_neg\_\_[](https://docs.exaloop.io/libraries/api/classes/float128/#class-float128-__neg__ "Copy/link to this API element")

@pure @llvm

```python
__neg__(self: Self, Self: type)
```

Added by [`internal.types.float`](https://docs.exaloop.io/libraries/api/modules/internal/types/float/)

```llvm
%0 = fneg fp128 %self
ret fp128 %0
```

\_\_abs\_\_[](https://docs.exaloop.io/libraries/api/classes/float128/#class-float128-__abs__ "Copy/link to this API element")

@pure @llvm

```python
__abs__(a: float128)
```

Added by [`internal.types.float`](https://docs.exaloop.io/libraries/api/modules/internal/types/float/)

```llvm
declare fp128 @llvm.fabs.f128(fp128 %a)
%tmp = call fp128 @llvm.fabs.f128(fp128 %a)
ret fp128 %tmp
```

\_\_floor\_\_[](https://docs.exaloop.io/libraries/api/classes/float128/#class-float128-__floor__ "Copy/link to this API element")

@pure @llvm

```python
__floor__(a: float128)
```

Added by [`internal.types.float`](https://docs.exaloop.io/libraries/api/modules/internal/types/float/)

```llvm
declare fp128 @llvm.floor.f128(fp128 %a)
%tmp = call fp128 @llvm.floor.f128(fp128 %a)
ret fp128 %tmp
```

\_\_ceil\_\_[](https://docs.exaloop.io/libraries/api/classes/float128/#class-float128-__ceil__ "Copy/link to this API element")

@pure @llvm

```python
__ceil__(a: float128)
```

Added by [`internal.types.float`](https://docs.exaloop.io/libraries/api/modules/internal/types/float/)

```llvm
declare fp128 @llvm.ceil.f128(fp128 %a)
%tmp = call fp128 @llvm.ceil.f128(fp128 %a)
ret fp128 %tmp
```

\_\_trunc\_\_[](https://docs.exaloop.io/libraries/api/classes/float128/#class-float128-__trunc__ "Copy/link to this API element")

@pure @llvm

```python
__trunc__(a: float128)
```

Added by [`internal.types.float`](https://docs.exaloop.io/libraries/api/modules/internal/types/float/)

```llvm
declare fp128 @llvm.trunc.f128(fp128 %a)
%tmp = call fp128 @llvm.trunc.f128(fp128 %a)
ret fp128 %tmp
```

\_\_hash\_\_[](https://docs.exaloop.io/libraries/api/classes/float128/#class-float128-__hash__ "Copy/link to this API element")

```python
__hash__(self: Self, Self: type)
```

Added by [`internal.types.float`](https://docs.exaloop.io/libraries/api/modules/internal/types/float/)

\_\_match\_\_[](https://docs.exaloop.io/libraries/api/classes/float128/#class-float128-__match__ "Copy/link to this API element")

```python
__match__(self: Self, obj: float128, Self: type)
```

Added by [`internal.types.float`](https://docs.exaloop.io/libraries/api/modules/internal/types/float/)

\_\_complex\_\_[](https://docs.exaloop.io/libraries/api/classes/float128/#class-float128-__complex__ "Copy/link to this API element")

```python
__complex__(self: Self, Self: type)
```

Added by [`internal.types.complex`](https://docs.exaloop.io/libraries/api/modules/internal/types/complex/)

\_\_pickle\_\_[](https://docs.exaloop.io/libraries/api/classes/float128/#class-float128-__pickle__ "Copy/link to this API element")

```python
__pickle__(self: Self, jar: Jar, Self: type)
```

Added by [`pickle`](https://docs.exaloop.io/libraries/api/modules/pickle/)

\_\_unpickle\_\_[](https://docs.exaloop.io/libraries/api/classes/float128/#class-float128-__unpickle__ "Copy/link to this API element")

```python
__unpickle__(jar: Jar)
```

Added by [`pickle`](https://docs.exaloop.io/libraries/api/modules/pickle/)

Source: https://docs.exaloop.io/libraries/api/classes/float128/
