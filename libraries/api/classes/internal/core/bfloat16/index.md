# bfloat16

Standard library class `internal.core.bfloat16` [View source](https://github.com/exaloop/codon/blob/master/stdlib/internal/core.codon#L83)

class bfloat16[](https://docs.exaloop.io/libraries/api/classes/internal/core/bfloat16/#class-bfloat16 "Copy/link to this API element")

@tuple

```python
class bfloat16
```

Constructors 11

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/core/bfloat16/#class-bfloat16-__new__ "Copy/link to this API element")

@pure @llvm

```python
__new__(self: float) -> bfloat16
```

Added by [`internal.types.float`](https://docs.exaloop.io/libraries/api/modules/internal/types/float/)

```llvm
%0 = fptrunc double %self to bfloat
ret bfloat %0
```

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/core/bfloat16/#class-bfloat16-__new__-overload-2 "Copy/link to this API element")

@overload

```python
__new__(what: bfloat16) -> bfloat16
```

Added by [`internal.types.float`](https://docs.exaloop.io/libraries/api/modules/internal/types/float/)

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/core/bfloat16/#class-bfloat16-__new__-overload-3 "Copy/link to this API element")

@overload

```python
__new__(what: str) -> bfloat16
```

Added by [`internal.types.float`](https://docs.exaloop.io/libraries/api/modules/internal/types/float/)

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/core/bfloat16/#class-bfloat16-__new__-overload-4 "Copy/link to this API element")

@overload

```python
__new__() -> bfloat16
```

Added by [`internal.types.float`](https://docs.exaloop.io/libraries/api/modules/internal/types/float/)

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/core/bfloat16/#class-bfloat16-__new__-overload-5 "Copy/link to this API element")

```python
__new__(what) -> bfloat16
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/core/bfloat16/#class-bfloat16-__new__-overload-6 "Copy/link to this API element")

```python
__new__(what: bool) -> bfloat16
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/core/bfloat16/#class-bfloat16-__new__-overload-7 "Copy/link to this API element")

@pure @llvm

```python
__new__[N: Literal[int]](what: Int[N]) -> bfloat16
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```llvm
%0 = sitofp i{=N} %what to bfloat
ret bfloat %0
```

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/core/bfloat16/#class-bfloat16-__new__-overload-8 "Copy/link to this API element")

@pure @llvm

```python
__new__[N: Literal[int]](what: UInt[N]) -> bfloat16
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```llvm
%0 = uitofp i{=N} %what to bfloat
ret bfloat %0
```

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/core/bfloat16/#class-bfloat16-__new__-overload-9 "Copy/link to this API element")

@pure @llvm

```python
__new__(what: float128) -> bfloat16
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```llvm
%0 = fptrunc fp128 %what to bfloat
ret bfloat %0
```

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/core/bfloat16/#class-bfloat16-__new__-overload-10 "Copy/link to this API element")

@pure @llvm

```python
__new__(what: float32) -> bfloat16
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```llvm
%0 = fptrunc float %what to bfloat
ret bfloat %0
```

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/core/bfloat16/#class-bfloat16-__new__-overload-11 "Copy/link to this API element")

@pure @llvm

```python
__new__(what: float16) -> bfloat16
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```llvm
%0 = fpext float16 %what to float
%1 = fptrunc float %0 to bfloat
ret bfloat %1
```

Methods 2

cdiv[](https://docs.exaloop.io/libraries/api/classes/internal/core/bfloat16/#class-bfloat16-cdiv "Copy/link to this API element")

```python
cdiv(self: Self, other: bfloat16) -> bfloat16
```

Added by [`internal.types.float`](https://docs.exaloop.io/libraries/api/modules/internal/types/float/)

cmod[](https://docs.exaloop.io/libraries/api/classes/internal/core/bfloat16/#class-bfloat16-cmod "Copy/link to this API element")

```python
cmod(self: Self, other: bfloat16) -> bfloat16
```

Added by [`internal.types.float`](https://docs.exaloop.io/libraries/api/modules/internal/types/float/)

Special methods 35

\_\_repr\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/core/bfloat16/#class-bfloat16-__repr__ "Copy/link to this API element")

```python
__repr__(self: Self) -> str
```

Added by [`internal.types.float`](https://docs.exaloop.io/libraries/api/modules/internal/types/float/)

\_\_format\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/core/bfloat16/#class-bfloat16-__format__ "Copy/link to this API element")

```python
__format__(self: Self, format_spec: str) -> str
```

Added by [`internal.types.float`](https://docs.exaloop.io/libraries/api/modules/internal/types/float/)

\_\_copy\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/core/bfloat16/#class-bfloat16-__copy__ "Copy/link to this API element")

```python
__copy__(self: Self) -> bfloat16
```

Added by [`internal.types.float`](https://docs.exaloop.io/libraries/api/modules/internal/types/float/)

\_\_deepcopy\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/core/bfloat16/#class-bfloat16-__deepcopy__ "Copy/link to this API element")

```python
__deepcopy__(self: Self) -> bfloat16
```

Added by [`internal.types.float`](https://docs.exaloop.io/libraries/api/modules/internal/types/float/)

\_\_int\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/core/bfloat16/#class-bfloat16-__int__ "Copy/link to this API element")

@pure @llvm

```python
__int__(self: Self) -> int
```

Added by [`internal.types.float`](https://docs.exaloop.io/libraries/api/modules/internal/types/float/)

```llvm
%0 = fptosi bfloat %self to i64
ret i64 %0
```

\_\_float\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/core/bfloat16/#class-bfloat16-__float__ "Copy/link to this API element")

@pure @llvm

```python
__float__(self: Self) -> float
```

Added by [`internal.types.float`](https://docs.exaloop.io/libraries/api/modules/internal/types/float/)

```llvm
%0 = fpext bfloat %self to double
ret double %0
```

\_\_bool\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/core/bfloat16/#class-bfloat16-__bool__ "Copy/link to this API element")

@pure @llvm

```python
__bool__(self: Self) -> bool
```

Added by [`internal.types.float`](https://docs.exaloop.io/libraries/api/modules/internal/types/float/)

```llvm
%0 = fcmp une bfloat %self, 0.000000e+00
%1 = zext i1 %0 to i8
ret i8 %1
```

\_\_pos\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/core/bfloat16/#class-bfloat16-__pos__ "Copy/link to this API element")

```python
__pos__(self: Self) -> bfloat16
```

Added by [`internal.types.float`](https://docs.exaloop.io/libraries/api/modules/internal/types/float/)

\_\_neg\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/core/bfloat16/#class-bfloat16-__neg__ "Copy/link to this API element")

@pure @llvm

```python
__neg__(self: Self) -> bfloat16
```

Added by [`internal.types.float`](https://docs.exaloop.io/libraries/api/modules/internal/types/float/)

```llvm
%0 = fneg bfloat %self
ret bfloat %0
```

\_\_add\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/core/bfloat16/#class-bfloat16-__add__ "Copy/link to this API element")

@commutative

@pure @llvm

```python
__add__(a: bfloat16, b: bfloat16) -> bfloat16
```

Added by [`internal.types.float`](https://docs.exaloop.io/libraries/api/modules/internal/types/float/)

```llvm
%tmp = fadd bfloat %a, %b
ret bfloat %tmp
```

```python
__add__(self: Self, b: float)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```python
__add__(self: Self, b: float128)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```python
__add__(self: Self, b: float32)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```python
__add__(self: Self, b: float16)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```python
__add__(self: Self, b: Int)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```python
__add__(self: Self, b: UInt)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```python
__add__(self: Self, b: bool)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

\_\_sub\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/core/bfloat16/#class-bfloat16-__sub__ "Copy/link to this API element")

@pure @llvm

```python
__sub__(a: bfloat16, b: bfloat16) -> bfloat16
```

Added by [`internal.types.float`](https://docs.exaloop.io/libraries/api/modules/internal/types/float/)

```llvm
%tmp = fsub bfloat %a, %b
ret bfloat %tmp
```

```python
__sub__(self: Self, b: float)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```python
__sub__(self: Self, b: float128)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```python
__sub__(self: Self, b: float32)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```python
__sub__(self: Self, b: float16)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```python
__sub__(self: Self, b: Int)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```python
__sub__(self: Self, b: UInt)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```python
__sub__(self: Self, b: bool)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

\_\_mul\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/core/bfloat16/#class-bfloat16-__mul__ "Copy/link to this API element")

@commutative

@pure @llvm

```python
__mul__(a: bfloat16, b: bfloat16) -> bfloat16
```

Added by [`internal.types.float`](https://docs.exaloop.io/libraries/api/modules/internal/types/float/)

```llvm
%tmp = fmul bfloat %a, %b
ret bfloat %tmp
```

```python
__mul__(self: Self, b: float)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```python
__mul__(self: Self, b: float128)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```python
__mul__(self: Self, b: float32)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```python
__mul__(self: Self, b: float16)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```python
__mul__(self: Self, b: Int)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```python
__mul__(self: Self, b: UInt)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```python
__mul__(self: Self, b: bool)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

\_\_floordiv\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/core/bfloat16/#class-bfloat16-__floordiv__ "Copy/link to this API element")

```python
__floordiv__(self: Self, other: bfloat16) -> bfloat16
```

Added by [`internal.types.float`](https://docs.exaloop.io/libraries/api/modules/internal/types/float/)

```python
__floordiv__(self: Self, b: float)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```python
__floordiv__(self: Self, b: float128)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```python
__floordiv__(self: Self, b: float32)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```python
__floordiv__(self: Self, b: float16)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```python
__floordiv__(self: Self, b: Int)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```python
__floordiv__(self: Self, b: UInt)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```python
__floordiv__(self: Self, b: bool)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

\_\_truediv\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/core/bfloat16/#class-bfloat16-__truediv__ "Copy/link to this API element")

```python
__truediv__(self: Self, other: bfloat16) -> bfloat16
```

Added by [`internal.types.float`](https://docs.exaloop.io/libraries/api/modules/internal/types/float/)

```python
__truediv__(self: Self, b: float)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```python
__truediv__(self: Self, b: float128)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```python
__truediv__(self: Self, b: float32)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```python
__truediv__(self: Self, b: float16)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```python
__truediv__(self: Self, b: Int)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```python
__truediv__(self: Self, b: UInt)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```python
__truediv__(self: Self, b: bool)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

\_\_mod\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/core/bfloat16/#class-bfloat16-__mod__ "Copy/link to this API element")

```python
__mod__(self: Self, other: bfloat16) -> bfloat16
```

Added by [`internal.types.float`](https://docs.exaloop.io/libraries/api/modules/internal/types/float/)

```python
__mod__(self: Self, b: float)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```python
__mod__(self: Self, b: float128)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```python
__mod__(self: Self, b: float32)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```python
__mod__(self: Self, b: float16)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```python
__mod__(self: Self, b: Int)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```python
__mod__(self: Self, b: UInt)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```python
__mod__(self: Self, b: bool)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

\_\_divmod\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/core/bfloat16/#class-bfloat16-__divmod__ "Copy/link to this API element")

```python
__divmod__(self: Self, other: bfloat16) -> Tuple[(bfloat16, bfloat16)]
```

Added by [`internal.types.float`](https://docs.exaloop.io/libraries/api/modules/internal/types/float/)

```python
__divmod__(self: Self, b: float)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```python
__divmod__(self: Self, b: float128)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```python
__divmod__(self: Self, b: float32)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```python
__divmod__(self: Self, b: float16)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```python
__divmod__(self: Self, b: Int)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```python
__divmod__(self: Self, b: UInt)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```python
__divmod__(self: Self, b: bool)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

\_\_eq\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/core/bfloat16/#class-bfloat16-__eq__ "Copy/link to this API element")

@pure @llvm

```python
__eq__(a: bfloat16, b: bfloat16) -> bool
```

Added by [`internal.types.float`](https://docs.exaloop.io/libraries/api/modules/internal/types/float/)

```llvm
%tmp = fcmp oeq bfloat %a, %b
%res = zext i1 %tmp to i8
ret i8 %res
```

```python
__eq__(self: Self, b: float)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```python
__eq__(self: Self, b: float128)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```python
__eq__(self: Self, b: float32)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```python
__eq__(self: Self, b: float16)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```python
__eq__(self: Self, b: Int)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```python
__eq__(self: Self, b: UInt)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```python
__eq__(self: Self, b: bool)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

\_\_ne\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/core/bfloat16/#class-bfloat16-__ne__ "Copy/link to this API element")

@pure @llvm

```python
__ne__(a: bfloat16, b: bfloat16) -> bool
```

Added by [`internal.types.float`](https://docs.exaloop.io/libraries/api/modules/internal/types/float/)

```llvm
%tmp = fcmp une bfloat %a, %b
%res = zext i1 %tmp to i8
ret i8 %res
```

```python
__ne__(self: Self, b: float)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```python
__ne__(self: Self, b: float128)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```python
__ne__(self: Self, b: float32)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```python
__ne__(self: Self, b: float16)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```python
__ne__(self: Self, b: Int)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```python
__ne__(self: Self, b: UInt)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```python
__ne__(self: Self, b: bool)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

\_\_lt\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/core/bfloat16/#class-bfloat16-__lt__ "Copy/link to this API element")

@pure @llvm

```python
__lt__(a: bfloat16, b: bfloat16) -> bool
```

Added by [`internal.types.float`](https://docs.exaloop.io/libraries/api/modules/internal/types/float/)

```llvm
%tmp = fcmp olt bfloat %a, %b
%res = zext i1 %tmp to i8
ret i8 %res
```

```python
__lt__(self: Self, b: float)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```python
__lt__(self: Self, b: float128)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```python
__lt__(self: Self, b: float32)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```python
__lt__(self: Self, b: float16)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```python
__lt__(self: Self, b: Int)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```python
__lt__(self: Self, b: UInt)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```python
__lt__(self: Self, b: bool)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

\_\_gt\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/core/bfloat16/#class-bfloat16-__gt__ "Copy/link to this API element")

@pure @llvm

```python
__gt__(a: bfloat16, b: bfloat16) -> bool
```

Added by [`internal.types.float`](https://docs.exaloop.io/libraries/api/modules/internal/types/float/)

```llvm
%tmp = fcmp ogt bfloat %a, %b
%res = zext i1 %tmp to i8
ret i8 %res
```

```python
__gt__(self: Self, b: float)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```python
__gt__(self: Self, b: float128)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```python
__gt__(self: Self, b: float32)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```python
__gt__(self: Self, b: float16)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```python
__gt__(self: Self, b: Int)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```python
__gt__(self: Self, b: UInt)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```python
__gt__(self: Self, b: bool)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

\_\_le\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/core/bfloat16/#class-bfloat16-__le__ "Copy/link to this API element")

@pure @llvm

```python
__le__(a: bfloat16, b: bfloat16) -> bool
```

Added by [`internal.types.float`](https://docs.exaloop.io/libraries/api/modules/internal/types/float/)

```llvm
%tmp = fcmp ole bfloat %a, %b
%res = zext i1 %tmp to i8
ret i8 %res
```

```python
__le__(self: Self, b: float)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```python
__le__(self: Self, b: float128)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```python
__le__(self: Self, b: float32)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```python
__le__(self: Self, b: float16)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```python
__le__(self: Self, b: Int)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```python
__le__(self: Self, b: UInt)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```python
__le__(self: Self, b: bool)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

\_\_ge\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/core/bfloat16/#class-bfloat16-__ge__ "Copy/link to this API element")

@pure @llvm

```python
__ge__(a: bfloat16, b: bfloat16) -> bool
```

Added by [`internal.types.float`](https://docs.exaloop.io/libraries/api/modules/internal/types/float/)

```llvm
%tmp = fcmp oge bfloat %a, %b
%res = zext i1 %tmp to i8
ret i8 %res
```

```python
__ge__(self: Self, b: float)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```python
__ge__(self: Self, b: float128)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```python
__ge__(self: Self, b: float32)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```python
__ge__(self: Self, b: float16)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```python
__ge__(self: Self, b: Int)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```python
__ge__(self: Self, b: UInt)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```python
__ge__(self: Self, b: bool)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

\_\_abs\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/core/bfloat16/#class-bfloat16-__abs__ "Copy/link to this API element")

@pure @llvm

```python
__abs__(a: bfloat16) -> bfloat16
```

Added by [`internal.types.float`](https://docs.exaloop.io/libraries/api/modules/internal/types/float/)

```llvm
declare bfloat @llvm.fabs.bf16(bfloat %a)
%tmp = call bfloat @llvm.fabs.bf16(bfloat %a)
ret bfloat %tmp
```

\_\_floor\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/core/bfloat16/#class-bfloat16-__floor__ "Copy/link to this API element")

@pure @llvm

```python
__floor__(a: bfloat16) -> bfloat16
```

Added by [`internal.types.float`](https://docs.exaloop.io/libraries/api/modules/internal/types/float/)

```llvm
declare bfloat @llvm.floor.bf16(bfloat %a)
%tmp = call bfloat @llvm.floor.bf16(bfloat %a)
ret bfloat %tmp
```

\_\_ceil\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/core/bfloat16/#class-bfloat16-__ceil__ "Copy/link to this API element")

@pure @llvm

```python
__ceil__(a: bfloat16) -> bfloat16
```

Added by [`internal.types.float`](https://docs.exaloop.io/libraries/api/modules/internal/types/float/)

```llvm
declare bfloat @llvm.ceil.bf16(bfloat %a)
%tmp = call bfloat @llvm.ceil.bf16(bfloat %a)
ret bfloat %tmp
```

\_\_trunc\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/core/bfloat16/#class-bfloat16-__trunc__ "Copy/link to this API element")

@pure @llvm

```python
__trunc__(a: bfloat16) -> bfloat16
```

Added by [`internal.types.float`](https://docs.exaloop.io/libraries/api/modules/internal/types/float/)

```llvm
declare bfloat @llvm.trunc.bf16(bfloat %a)
%tmp = call bfloat @llvm.trunc.bf16(bfloat %a)
ret bfloat %tmp
```

\_\_pow\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/core/bfloat16/#class-bfloat16-__pow__ "Copy/link to this API element")

@pure @llvm

```python
__pow__(a: bfloat16, b: bfloat16) -> bfloat16
```

Added by [`internal.types.float`](https://docs.exaloop.io/libraries/api/modules/internal/types/float/)

```llvm
declare bfloat @llvm.pow.bf16(bfloat %a, bfloat %b)
%tmp = call bfloat @llvm.pow.bf16(bfloat %a, bfloat %b)
ret bfloat %tmp
```

```python
__pow__(self: Self, b: float)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```python
__pow__(self: Self, b: float128)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```python
__pow__(self: Self, b: float32)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```python
__pow__(self: Self, b: float16)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```python
__pow__(self: Self, b: Int)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```python
__pow__(self: Self, b: UInt)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

```python
__pow__(self: Self, b: bool)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

\_\_hash\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/core/bfloat16/#class-bfloat16-__hash__ "Copy/link to this API element")

```python
__hash__(self: Self) -> int
```

Added by [`internal.types.float`](https://docs.exaloop.io/libraries/api/modules/internal/types/float/)

\_\_match\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/core/bfloat16/#class-bfloat16-__match__ "Copy/link to this API element")

```python
__match__(self: Self, obj: bfloat16) -> bool
```

Added by [`internal.types.float`](https://docs.exaloop.io/libraries/api/modules/internal/types/float/)

\_\_complex\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/core/bfloat16/#class-bfloat16-__complex__ "Copy/link to this API element")

```python
__complex__(self: Self) -> complex
```

Added by [`internal.types.complex`](https://docs.exaloop.io/libraries/api/modules/internal/types/complex/)

\_\_to\_gpu\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/core/bfloat16/#class-bfloat16-__to_gpu__ "Copy/link to this API element")

```python
__to_gpu__(self: Self, cache: AllocCache)
```

Added by [`internal.gpu`](https://docs.exaloop.io/libraries/api/modules/internal/gpu/)

\_\_from\_gpu\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/core/bfloat16/#class-bfloat16-__from_gpu__ "Copy/link to this API element")

```python
__from_gpu__(self: Self, other: bfloat16)
```

Added by [`internal.gpu`](https://docs.exaloop.io/libraries/api/modules/internal/gpu/)

\_\_from\_gpu\_new\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/core/bfloat16/#class-bfloat16-__from_gpu_new__ "Copy/link to this API element")

```python
__from_gpu_new__(other: bfloat16)
```

Added by [`internal.gpu`](https://docs.exaloop.io/libraries/api/modules/internal/gpu/)

\_\_pickle\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/core/bfloat16/#class-bfloat16-__pickle__ "Copy/link to this API element")

```python
__pickle__(self: Self, jar: Jar)
```

Added by [`pickle`](https://docs.exaloop.io/libraries/api/modules/pickle/)

\_\_unpickle\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/core/bfloat16/#class-bfloat16-__unpickle__ "Copy/link to this API element")

```python
__unpickle__(jar: Jar) -> bfloat16
```

Added by [`pickle`](https://docs.exaloop.io/libraries/api/modules/pickle/)

Source: https://docs.exaloop.io/libraries/api/classes/internal/core/bfloat16/
