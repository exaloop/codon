# internal.types.float

Standard library Module [View source](https://github.com/exaloop/codon/blob/master/stdlib/internal/types/float.codon)

class float[](https://docs.exaloop.io/libraries/api/modules/internal/types/float/#class-float "Copy/link to this API element")

@extend

```python
class float
```

Extends [`internal.core.float`](https://docs.exaloop.io/libraries/api/classes/internal/core/float/) · [View complete class API →](https://docs.exaloop.io/libraries/api/classes/internal/core/float/)

Constructors 2

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/float/#class-float-__new__ "Copy/link to this API element")

```python
__new__() -> float
```

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/float/#class-float-__new__-overload-2 "Copy/link to this API element")

@overload

```python
__new__(what, /) -> float
```

Properties 2

real[](https://docs.exaloop.io/libraries/api/modules/internal/types/float/#class-float-real "Copy/link to this API element")

@property

```python
real(self: Self) -> float
```

imag[](https://docs.exaloop.io/libraries/api/modules/internal/types/float/#class-float-imag "Copy/link to this API element")

@property

```python
imag(self: Self) -> float
```

Methods 2

cdiv[](https://docs.exaloop.io/libraries/api/modules/internal/types/float/#class-float-cdiv "Copy/link to this API element")

```python
cdiv(self: Self, other: float) -> float
```

cmod[](https://docs.exaloop.io/libraries/api/modules/internal/types/float/#class-float-cmod "Copy/link to this API element")

```python
cmod(self: Self, other: float) -> float
```

Special methods 32

\_\_repr\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/float/#class-float-__repr__ "Copy/link to this API element")

```python
__repr__(self: Self, /) -> str
```

\_\_copy\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/float/#class-float-__copy__ "Copy/link to this API element")

```python
__copy__(self: Self) -> float
```

\_\_deepcopy\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/float/#class-float-__deepcopy__ "Copy/link to this API element")

```python
__deepcopy__(self: Self) -> float
```

\_\_int\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/float/#class-float-__int__ "Copy/link to this API element")

@pure @llvm

```python
__int__(self: Self, /) -> int
```

```llvm
%0 = fptosi double %self to i64
ret i64 %0
```

\_\_float\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/float/#class-float-__float__ "Copy/link to this API element")

```python
__float__(self: Self, /) -> float
```

\_\_bool\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/float/#class-float-__bool__ "Copy/link to this API element")

@pure @llvm

```python
__bool__(self: Self, /) -> bool
```

```llvm
%0 = fcmp une double %self, 0.000000e+00
%1 = zext i1 %0 to i8
ret i8 %1
```

\_\_pos\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/float/#class-float-__pos__ "Copy/link to this API element")

```python
__pos__(self: Self, /) -> float
```

\_\_neg\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/float/#class-float-__neg__ "Copy/link to this API element")

@pure @llvm

```python
__neg__(self: Self, /) -> float
```

```llvm
%0 = fneg double %self
ret double %0
```

\_\_add\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/float/#class-float-__add__ "Copy/link to this API element")

@pure @commutative @llvm

```python
__add__(a: float, b: float, /) -> float
```

```llvm
%tmp = fadd double %a, %b
ret double %tmp
```

\_\_sub\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/float/#class-float-__sub__ "Copy/link to this API element")

@pure @llvm

```python
__sub__(a: float, b: float, /) -> float
```

```llvm
%tmp = fsub double %a, %b
ret double %tmp
```

\_\_mul\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/float/#class-float-__mul__ "Copy/link to this API element")

@pure @commutative @llvm

```python
__mul__(a: float, b: float, /) -> float
```

```llvm
%tmp = fmul double %a, %b
ret double %tmp
```

\_\_floordiv\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/float/#class-float-__floordiv__ "Copy/link to this API element")

```python
__floordiv__(self: Self, other: float, /) -> float
```

\_\_truediv\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/float/#class-float-__truediv__ "Copy/link to this API element")

```python
__truediv__(self: Self, other: float, /) -> float
```

\_\_mod\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/float/#class-float-__mod__ "Copy/link to this API element")

```python
__mod__(self: Self, other: float, /) -> float
```

\_\_divmod\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/float/#class-float-__divmod__ "Copy/link to this API element")

```python
__divmod__(self: Self, other: float, /) -> Tuple[(float, float)]
```

\_\_eq\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/float/#class-float-__eq__ "Copy/link to this API element")

@pure @llvm

```python
__eq__(a: float, b: float, /) -> bool
```

```llvm
%tmp = fcmp oeq double %a, %b
%res = zext i1 %tmp to i8
ret i8 %res
```

\_\_ne\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/float/#class-float-__ne__ "Copy/link to this API element")

@pure @llvm

```python
__ne__(a: float, b: float, /) -> bool
```

```llvm
%tmp = fcmp une double %a, %b
%res = zext i1 %tmp to i8
ret i8 %res
```

\_\_lt\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/float/#class-float-__lt__ "Copy/link to this API element")

@pure @llvm

```python
__lt__(a: float, b: float, /) -> bool
```

```llvm
%tmp = fcmp olt double %a, %b
%res = zext i1 %tmp to i8
ret i8 %res
```

\_\_gt\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/float/#class-float-__gt__ "Copy/link to this API element")

@pure @llvm

```python
__gt__(a: float, b: float, /) -> bool
```

```llvm
%tmp = fcmp ogt double %a, %b
%res = zext i1 %tmp to i8
ret i8 %res
```

\_\_le\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/float/#class-float-__le__ "Copy/link to this API element")

@pure @llvm

```python
__le__(a: float, b: float, /) -> bool
```

```llvm
%tmp = fcmp ole double %a, %b
%res = zext i1 %tmp to i8
ret i8 %res
```

\_\_ge\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/float/#class-float-__ge__ "Copy/link to this API element")

@pure @llvm

```python
__ge__(a: float, b: float, /) -> bool
```

```llvm
%tmp = fcmp oge double %a, %b
%res = zext i1 %tmp to i8
ret i8 %res
```

\_\_abs\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/float/#class-float-__abs__ "Copy/link to this API element")

@pure @llvm

```python
__abs__(a: float, /) -> float
```

```llvm
declare double @llvm.fabs.f64(double %a)
%tmp = call double @llvm.fabs.f64(double %a)
ret double %tmp
```

\_\_floor\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/float/#class-float-__floor__ "Copy/link to this API element")

@pure @llvm

```python
__floor__(a: float, /) -> float
```

```llvm
declare double @llvm.floor.f64(double %a)
%tmp = call double @llvm.floor.f64(double %a)
ret double %tmp
```

\_\_ceil\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/float/#class-float-__ceil__ "Copy/link to this API element")

@pure @llvm

```python
__ceil__(a: float, /) -> float
```

```llvm
declare double @llvm.ceil.f64(double %a)
%tmp = call double @llvm.ceil.f64(double %a)
ret double %tmp
```

\_\_trunc\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/float/#class-float-__trunc__ "Copy/link to this API element")

@pure @llvm

```python
__trunc__(a: float, /) -> float
```

```llvm
declare double @llvm.trunc.f64(double %a)
%tmp = call double @llvm.trunc.f64(double %a)
ret double %tmp
```

\_\_pow\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/float/#class-float-__pow__ "Copy/link to this API element")

@pure @llvm

```python
__pow__(a: float, b: float, /) -> float
```

```llvm
declare double @llvm.pow.f64(double %a, double %b)
%tmp = call double @llvm.pow.f64(double %a, double %b)
ret double %tmp
```

\_\_round\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/float/#class-float-__round__ "Copy/link to this API element")

```python
__round__(self: Self, ndigits = None, /)
```

\_\_atomic\_xchg\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/float/#class-float-__atomic_xchg__ "Copy/link to this API element")

@nocapture @llvm

```python
__atomic_xchg__(d: Ptr[float], b: float) -> None
```

```llvm
%tmp = atomicrmw xchg ptr %d, double %b seq_cst
ret {} {}
```

\_\_atomic\_add\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/float/#class-float-__atomic_add__ "Copy/link to this API element")

@nocapture @llvm

```python
__atomic_add__(d: Ptr[float], b: float) -> float
```

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

\_\_atomic\_sub\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/float/#class-float-__atomic_sub__ "Copy/link to this API element")

@nocapture @llvm

```python
__atomic_sub__(d: Ptr[float], b: float) -> float
```

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

\_\_hash\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/float/#class-float-__hash__ "Copy/link to this API element")

```python
__hash__(self: Self, /) -> int
```

\_\_match\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/float/#class-float-__match__ "Copy/link to this API element")

```python
__match__(self: Self, obj: float) -> bool
```

class float32[](https://docs.exaloop.io/libraries/api/modules/internal/types/float/#class-float32 "Copy/link to this API element")

@extend

```python
class float32
```

Extends [`internal.core.float32`](https://docs.exaloop.io/libraries/api/classes/internal/core/float32/) · [View complete class API →](https://docs.exaloop.io/libraries/api/classes/internal/core/float32/)

Constructors 4

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/float/#class-float32-__new__ "Copy/link to this API element")

@pure @llvm

```python
__new__(self: float) -> float32
```

```llvm
%0 = fptrunc double %self to float
ret float %0
```

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/float/#class-float32-__new__-overload-2 "Copy/link to this API element")

@overload

```python
__new__(what: float32) -> float32
```

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/float/#class-float32-__new__-overload-3 "Copy/link to this API element")

@overload

```python
__new__(what: str) -> float32
```

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/float/#class-float32-__new__-overload-4 "Copy/link to this API element")

@overload

```python
__new__() -> float32
```

Methods 2

cdiv[](https://docs.exaloop.io/libraries/api/modules/internal/types/float/#class-float32-cdiv "Copy/link to this API element")

```python
cdiv(self: Self, other: float32) -> float32
```

cmod[](https://docs.exaloop.io/libraries/api/modules/internal/types/float/#class-float32-cmod "Copy/link to this API element")

```python
cmod(self: Self, other: float32) -> float32
```

Special methods 32

\_\_repr\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/float/#class-float32-__repr__ "Copy/link to this API element")

```python
__repr__(self: Self) -> str
```

\_\_format\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/float/#class-float32-__format__ "Copy/link to this API element")

```python
__format__(self: Self, format_spec: str) -> str
```

\_\_copy\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/float/#class-float32-__copy__ "Copy/link to this API element")

```python
__copy__(self: Self) -> float32
```

\_\_deepcopy\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/float/#class-float32-__deepcopy__ "Copy/link to this API element")

```python
__deepcopy__(self: Self) -> float32
```

\_\_int\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/float/#class-float32-__int__ "Copy/link to this API element")

@pure @llvm

```python
__int__(self: Self) -> int
```

```llvm
%0 = fptosi float %self to i64
ret i64 %0
```

\_\_float\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/float/#class-float32-__float__ "Copy/link to this API element")

@pure @llvm

```python
__float__(self: Self) -> float
```

```llvm
%0 = fpext float %self to double
ret double %0
```

\_\_bool\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/float/#class-float32-__bool__ "Copy/link to this API element")

@pure @llvm

```python
__bool__(self: Self) -> bool
```

```llvm
%0 = fcmp une float %self, 0.000000e+00
%1 = zext i1 %0 to i8
ret i8 %1
```

\_\_pos\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/float/#class-float32-__pos__ "Copy/link to this API element")

```python
__pos__(self: Self) -> float32
```

\_\_neg\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/float/#class-float32-__neg__ "Copy/link to this API element")

@pure @llvm

```python
__neg__(self: Self) -> float32
```

```llvm
%0 = fneg float %self
ret float %0
```

\_\_add\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/float/#class-float32-__add__ "Copy/link to this API element")

@pure @commutative @llvm

```python
__add__(a: float32, b: float32) -> float32
```

```llvm
%tmp = fadd float %a, %b
ret float %tmp
```

\_\_sub\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/float/#class-float32-__sub__ "Copy/link to this API element")

@pure @llvm

```python
__sub__(a: float32, b: float32) -> float32
```

```llvm
%tmp = fsub float %a, %b
ret float %tmp
```

\_\_mul\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/float/#class-float32-__mul__ "Copy/link to this API element")

@pure @commutative @llvm

```python
__mul__(a: float32, b: float32) -> float32
```

```llvm
%tmp = fmul float %a, %b
ret float %tmp
```

\_\_floordiv\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/float/#class-float32-__floordiv__ "Copy/link to this API element")

```python
__floordiv__(self: Self, other: float32) -> float32
```

\_\_truediv\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/float/#class-float32-__truediv__ "Copy/link to this API element")

```python
__truediv__(self: Self, other: float32) -> float32
```

\_\_mod\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/float/#class-float32-__mod__ "Copy/link to this API element")

```python
__mod__(self: Self, other: float32) -> float32
```

\_\_divmod\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/float/#class-float32-__divmod__ "Copy/link to this API element")

```python
__divmod__(self: Self, other: float32) -> Tuple[(float32, float32)]
```

\_\_eq\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/float/#class-float32-__eq__ "Copy/link to this API element")

@pure @llvm

```python
__eq__(a: float32, b: float32) -> bool
```

```llvm
%tmp = fcmp oeq float %a, %b
%res = zext i1 %tmp to i8
ret i8 %res
```

\_\_ne\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/float/#class-float32-__ne__ "Copy/link to this API element")

@pure @llvm

```python
__ne__(a: float32, b: float32) -> bool
```

```llvm
%tmp = fcmp une float %a, %b
%res = zext i1 %tmp to i8
ret i8 %res
```

\_\_lt\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/float/#class-float32-__lt__ "Copy/link to this API element")

@pure @llvm

```python
__lt__(a: float32, b: float32) -> bool
```

```llvm
%tmp = fcmp olt float %a, %b
%res = zext i1 %tmp to i8
ret i8 %res
```

\_\_gt\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/float/#class-float32-__gt__ "Copy/link to this API element")

@pure @llvm

```python
__gt__(a: float32, b: float32) -> bool
```

```llvm
%tmp = fcmp ogt float %a, %b
%res = zext i1 %tmp to i8
ret i8 %res
```

\_\_le\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/float/#class-float32-__le__ "Copy/link to this API element")

@pure @llvm

```python
__le__(a: float32, b: float32) -> bool
```

```llvm
%tmp = fcmp ole float %a, %b
%res = zext i1 %tmp to i8
ret i8 %res
```

\_\_ge\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/float/#class-float32-__ge__ "Copy/link to this API element")

@pure @llvm

```python
__ge__(a: float32, b: float32) -> bool
```

```llvm
%tmp = fcmp oge float %a, %b
%res = zext i1 %tmp to i8
ret i8 %res
```

\_\_abs\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/float/#class-float32-__abs__ "Copy/link to this API element")

@pure @llvm

```python
__abs__(a: float32) -> float32
```

```llvm
declare float @llvm.fabs.f32(float %a)
%tmp = call float @llvm.fabs.f32(float %a)
ret float %tmp
```

\_\_floor\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/float/#class-float32-__floor__ "Copy/link to this API element")

@pure @llvm

```python
__floor__(a: float32) -> float32
```

```llvm
declare float @llvm.floor.f32(float %a)
%tmp = call float @llvm.floor.f32(float %a)
ret float %tmp
```

\_\_ceil\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/float/#class-float32-__ceil__ "Copy/link to this API element")

@pure @llvm

```python
__ceil__(a: float32) -> float32
```

```llvm
declare float @llvm.ceil.f32(float %a)
%tmp = call float @llvm.ceil.f32(float %a)
ret float %tmp
```

\_\_trunc\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/float/#class-float32-__trunc__ "Copy/link to this API element")

@pure @llvm

```python
__trunc__(a: float32) -> float32
```

```llvm
declare float @llvm.trunc.f32(float %a)
%tmp = call float @llvm.trunc.f32(float %a)
ret float %tmp
```

\_\_pow\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/float/#class-float32-__pow__ "Copy/link to this API element")

@pure @llvm

```python
__pow__(a: float32, b: float32) -> float32
```

```llvm
declare float @llvm.pow.f32(float %a, float %b)
%tmp = call float @llvm.pow.f32(float %a, float %b)
ret float %tmp
```

\_\_atomic\_xchg\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/float/#class-float32-__atomic_xchg__ "Copy/link to this API element")

@nocapture @llvm

```python
__atomic_xchg__(d: Ptr[float32], b: float32) -> None
```

```llvm
%tmp = atomicrmw xchg ptr %d, float %b seq_cst
ret {} {}
```

\_\_atomic\_add\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/float/#class-float32-__atomic_add__ "Copy/link to this API element")

@nocapture @llvm

```python
__atomic_add__(d: Ptr[float32], b: float32) -> float32
```

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

\_\_atomic\_sub\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/float/#class-float32-__atomic_sub__ "Copy/link to this API element")

@nocapture @llvm

```python
__atomic_sub__(d: Ptr[float32], b: float32) -> float32
```

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

\_\_hash\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/float/#class-float32-__hash__ "Copy/link to this API element")

```python
__hash__(self: Self) -> int
```

\_\_match\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/float/#class-float32-__match__ "Copy/link to this API element")

```python
__match__(self: Self, obj: float32) -> bool
```

class float16[](https://docs.exaloop.io/libraries/api/modules/internal/types/float/#class-float16 "Copy/link to this API element")

@extend

```python
class float16
```

Extends [`internal.core.float16`](https://docs.exaloop.io/libraries/api/classes/internal/core/float16/) · [View complete class API →](https://docs.exaloop.io/libraries/api/classes/internal/core/float16/)

Constructors 4

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/float/#class-float16-__new__ "Copy/link to this API element")

@pure @llvm

```python
__new__(self: float) -> float16
```

```llvm
%0 = fptrunc double %self to half
ret half %0
```

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/float/#class-float16-__new__-overload-2 "Copy/link to this API element")

@overload

```python
__new__(what: float16) -> float16
```

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/float/#class-float16-__new__-overload-3 "Copy/link to this API element")

@overload

```python
__new__(what: str) -> float16
```

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/float/#class-float16-__new__-overload-4 "Copy/link to this API element")

@overload

```python
__new__() -> float16
```

Methods 2

cdiv[](https://docs.exaloop.io/libraries/api/modules/internal/types/float/#class-float16-cdiv "Copy/link to this API element")

```python
cdiv(self: Self, other: float16) -> float16
```

cmod[](https://docs.exaloop.io/libraries/api/modules/internal/types/float/#class-float16-cmod "Copy/link to this API element")

```python
cmod(self: Self, other: float16) -> float16
```

Special methods 29

\_\_repr\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/float/#class-float16-__repr__ "Copy/link to this API element")

```python
__repr__(self: Self) -> str
```

\_\_format\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/float/#class-float16-__format__ "Copy/link to this API element")

```python
__format__(self: Self, format_spec: str) -> str
```

\_\_copy\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/float/#class-float16-__copy__ "Copy/link to this API element")

```python
__copy__(self: Self) -> float16
```

\_\_deepcopy\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/float/#class-float16-__deepcopy__ "Copy/link to this API element")

```python
__deepcopy__(self: Self) -> float16
```

\_\_int\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/float/#class-float16-__int__ "Copy/link to this API element")

@pure @llvm

```python
__int__(self: Self) -> int
```

```llvm
%0 = fptosi half %self to i64
ret i64 %0
```

\_\_float\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/float/#class-float16-__float__ "Copy/link to this API element")

@pure @llvm

```python
__float__(self: Self) -> float
```

```llvm
%0 = fpext half %self to double
ret double %0
```

\_\_bool\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/float/#class-float16-__bool__ "Copy/link to this API element")

@pure @llvm

```python
__bool__(self: Self) -> bool
```

```llvm
%0 = fcmp une half %self, 0.000000e+00
%1 = zext i1 %0 to i8
ret i8 %1
```

\_\_pos\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/float/#class-float16-__pos__ "Copy/link to this API element")

```python
__pos__(self: Self) -> float16
```

\_\_neg\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/float/#class-float16-__neg__ "Copy/link to this API element")

@pure @llvm

```python
__neg__(self: Self) -> float16
```

```llvm
%0 = fneg half %self
ret half %0
```

\_\_add\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/float/#class-float16-__add__ "Copy/link to this API element")

@pure @commutative @llvm

```python
__add__(a: float16, b: float16) -> float16
```

```llvm
%tmp = fadd half %a, %b
ret half %tmp
```

\_\_sub\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/float/#class-float16-__sub__ "Copy/link to this API element")

@pure @llvm

```python
__sub__(a: float16, b: float16) -> float16
```

```llvm
%tmp = fsub half %a, %b
ret half %tmp
```

\_\_mul\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/float/#class-float16-__mul__ "Copy/link to this API element")

@pure @commutative @llvm

```python
__mul__(a: float16, b: float16) -> float16
```

```llvm
%tmp = fmul half %a, %b
ret half %tmp
```

\_\_floordiv\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/float/#class-float16-__floordiv__ "Copy/link to this API element")

```python
__floordiv__(self: Self, other: float16) -> float16
```

\_\_truediv\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/float/#class-float16-__truediv__ "Copy/link to this API element")

```python
__truediv__(self: Self, other: float16) -> float16
```

\_\_mod\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/float/#class-float16-__mod__ "Copy/link to this API element")

```python
__mod__(self: Self, other: float16) -> float16
```

\_\_divmod\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/float/#class-float16-__divmod__ "Copy/link to this API element")

```python
__divmod__(self: Self, other: float16) -> Tuple[(float16, float16)]
```

\_\_eq\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/float/#class-float16-__eq__ "Copy/link to this API element")

@pure @llvm

```python
__eq__(a: float16, b: float16) -> bool
```

```llvm
%tmp = fcmp oeq half %a, %b
%res = zext i1 %tmp to i8
ret i8 %res
```

\_\_ne\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/float/#class-float16-__ne__ "Copy/link to this API element")

@pure @llvm

```python
__ne__(a: float16, b: float16) -> bool
```

```llvm
%tmp = fcmp une half %a, %b
%res = zext i1 %tmp to i8
ret i8 %res
```

\_\_lt\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/float/#class-float16-__lt__ "Copy/link to this API element")

@pure @llvm

```python
__lt__(a: float16, b: float16) -> bool
```

```llvm
%tmp = fcmp olt half %a, %b
%res = zext i1 %tmp to i8
ret i8 %res
```

\_\_gt\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/float/#class-float16-__gt__ "Copy/link to this API element")

@pure @llvm

```python
__gt__(a: float16, b: float16) -> bool
```

```llvm
%tmp = fcmp ogt half %a, %b
%res = zext i1 %tmp to i8
ret i8 %res
```

\_\_le\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/float/#class-float16-__le__ "Copy/link to this API element")

@pure @llvm

```python
__le__(a: float16, b: float16) -> bool
```

```llvm
%tmp = fcmp ole half %a, %b
%res = zext i1 %tmp to i8
ret i8 %res
```

\_\_ge\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/float/#class-float16-__ge__ "Copy/link to this API element")

@pure @llvm

```python
__ge__(a: float16, b: float16) -> bool
```

```llvm
%tmp = fcmp oge half %a, %b
%res = zext i1 %tmp to i8
ret i8 %res
```

\_\_abs\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/float/#class-float16-__abs__ "Copy/link to this API element")

@pure @llvm

```python
__abs__(a: float16) -> float16
```

```llvm
declare half @llvm.fabs.f16(half %a)
%tmp = call half @llvm.fabs.f16(half %a)
ret half %tmp
```

\_\_floor\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/float/#class-float16-__floor__ "Copy/link to this API element")

@pure @llvm

```python
__floor__(a: float16) -> float16
```

```llvm
declare half @llvm.floor.f16(half %a)
%tmp = call half @llvm.floor.f16(half %a)
ret half %tmp
```

\_\_ceil\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/float/#class-float16-__ceil__ "Copy/link to this API element")

@pure @llvm

```python
__ceil__(a: float16) -> float16
```

```llvm
declare half @llvm.ceil.f16(half %a)
%tmp = call half @llvm.ceil.f16(half %a)
ret half %tmp
```

\_\_trunc\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/float/#class-float16-__trunc__ "Copy/link to this API element")

@pure @llvm

```python
__trunc__(a: float16) -> float16
```

```llvm
declare half @llvm.trunc.f16(half %a)
%tmp = call half @llvm.trunc.f16(half %a)
ret half %tmp
```

\_\_pow\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/float/#class-float16-__pow__ "Copy/link to this API element")

@pure @llvm

```python
__pow__(a: float16, b: float16) -> float16
```

```llvm
declare half @llvm.pow.f16(half %a, half %b)
%tmp = call half @llvm.pow.f16(half %a, half %b)
ret half %tmp
```

\_\_hash\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/float/#class-float16-__hash__ "Copy/link to this API element")

```python
__hash__(self: Self) -> int
```

\_\_match\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/float/#class-float16-__match__ "Copy/link to this API element")

```python
__match__(self: Self, obj: float16) -> bool
```

class bfloat16[](https://docs.exaloop.io/libraries/api/modules/internal/types/float/#class-bfloat16 "Copy/link to this API element")

@extend

```python
class bfloat16
```

Extends [`internal.core.bfloat16`](https://docs.exaloop.io/libraries/api/classes/internal/core/bfloat16/) · [View complete class API →](https://docs.exaloop.io/libraries/api/classes/internal/core/bfloat16/)

Constructors 4

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/float/#class-bfloat16-__new__ "Copy/link to this API element")

@pure @llvm

```python
__new__(self: float) -> bfloat16
```

```llvm
%0 = fptrunc double %self to bfloat
ret bfloat %0
```

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/float/#class-bfloat16-__new__-overload-2 "Copy/link to this API element")

@overload

```python
__new__(what: bfloat16) -> bfloat16
```

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/float/#class-bfloat16-__new__-overload-3 "Copy/link to this API element")

@overload

```python
__new__(what: str) -> bfloat16
```

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/float/#class-bfloat16-__new__-overload-4 "Copy/link to this API element")

@overload

```python
__new__() -> bfloat16
```

Methods 2

cdiv[](https://docs.exaloop.io/libraries/api/modules/internal/types/float/#class-bfloat16-cdiv "Copy/link to this API element")

```python
cdiv(self: Self, other: bfloat16) -> bfloat16
```

cmod[](https://docs.exaloop.io/libraries/api/modules/internal/types/float/#class-bfloat16-cmod "Copy/link to this API element")

```python
cmod(self: Self, other: bfloat16) -> bfloat16
```

Special methods 29

\_\_repr\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/float/#class-bfloat16-__repr__ "Copy/link to this API element")

```python
__repr__(self: Self) -> str
```

\_\_format\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/float/#class-bfloat16-__format__ "Copy/link to this API element")

```python
__format__(self: Self, format_spec: str) -> str
```

\_\_copy\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/float/#class-bfloat16-__copy__ "Copy/link to this API element")

```python
__copy__(self: Self) -> bfloat16
```

\_\_deepcopy\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/float/#class-bfloat16-__deepcopy__ "Copy/link to this API element")

```python
__deepcopy__(self: Self) -> bfloat16
```

\_\_int\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/float/#class-bfloat16-__int__ "Copy/link to this API element")

@pure @llvm

```python
__int__(self: Self) -> int
```

```llvm
%0 = fptosi bfloat %self to i64
ret i64 %0
```

\_\_float\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/float/#class-bfloat16-__float__ "Copy/link to this API element")

@pure @llvm

```python
__float__(self: Self) -> float
```

```llvm
%0 = fpext bfloat %self to double
ret double %0
```

\_\_bool\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/float/#class-bfloat16-__bool__ "Copy/link to this API element")

@pure @llvm

```python
__bool__(self: Self) -> bool
```

```llvm
%0 = fcmp une bfloat %self, 0.000000e+00
%1 = zext i1 %0 to i8
ret i8 %1
```

\_\_pos\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/float/#class-bfloat16-__pos__ "Copy/link to this API element")

```python
__pos__(self: Self) -> bfloat16
```

\_\_neg\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/float/#class-bfloat16-__neg__ "Copy/link to this API element")

@pure @llvm

```python
__neg__(self: Self) -> bfloat16
```

```llvm
%0 = fneg bfloat %self
ret bfloat %0
```

\_\_add\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/float/#class-bfloat16-__add__ "Copy/link to this API element")

@pure @commutative @llvm

```python
__add__(a: bfloat16, b: bfloat16) -> bfloat16
```

```llvm
%tmp = fadd bfloat %a, %b
ret bfloat %tmp
```

\_\_sub\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/float/#class-bfloat16-__sub__ "Copy/link to this API element")

@pure @llvm

```python
__sub__(a: bfloat16, b: bfloat16) -> bfloat16
```

```llvm
%tmp = fsub bfloat %a, %b
ret bfloat %tmp
```

\_\_mul\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/float/#class-bfloat16-__mul__ "Copy/link to this API element")

@pure @commutative @llvm

```python
__mul__(a: bfloat16, b: bfloat16) -> bfloat16
```

```llvm
%tmp = fmul bfloat %a, %b
ret bfloat %tmp
```

\_\_floordiv\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/float/#class-bfloat16-__floordiv__ "Copy/link to this API element")

```python
__floordiv__(self: Self, other: bfloat16) -> bfloat16
```

\_\_truediv\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/float/#class-bfloat16-__truediv__ "Copy/link to this API element")

```python
__truediv__(self: Self, other: bfloat16) -> bfloat16
```

\_\_mod\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/float/#class-bfloat16-__mod__ "Copy/link to this API element")

```python
__mod__(self: Self, other: bfloat16) -> bfloat16
```

\_\_divmod\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/float/#class-bfloat16-__divmod__ "Copy/link to this API element")

```python
__divmod__(self: Self, other: bfloat16) -> Tuple[(bfloat16, bfloat16)]
```

\_\_eq\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/float/#class-bfloat16-__eq__ "Copy/link to this API element")

@pure @llvm

```python
__eq__(a: bfloat16, b: bfloat16) -> bool
```

```llvm
%tmp = fcmp oeq bfloat %a, %b
%res = zext i1 %tmp to i8
ret i8 %res
```

\_\_ne\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/float/#class-bfloat16-__ne__ "Copy/link to this API element")

@pure @llvm

```python
__ne__(a: bfloat16, b: bfloat16) -> bool
```

```llvm
%tmp = fcmp une bfloat %a, %b
%res = zext i1 %tmp to i8
ret i8 %res
```

\_\_lt\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/float/#class-bfloat16-__lt__ "Copy/link to this API element")

@pure @llvm

```python
__lt__(a: bfloat16, b: bfloat16) -> bool
```

```llvm
%tmp = fcmp olt bfloat %a, %b
%res = zext i1 %tmp to i8
ret i8 %res
```

\_\_gt\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/float/#class-bfloat16-__gt__ "Copy/link to this API element")

@pure @llvm

```python
__gt__(a: bfloat16, b: bfloat16) -> bool
```

```llvm
%tmp = fcmp ogt bfloat %a, %b
%res = zext i1 %tmp to i8
ret i8 %res
```

\_\_le\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/float/#class-bfloat16-__le__ "Copy/link to this API element")

@pure @llvm

```python
__le__(a: bfloat16, b: bfloat16) -> bool
```

```llvm
%tmp = fcmp ole bfloat %a, %b
%res = zext i1 %tmp to i8
ret i8 %res
```

\_\_ge\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/float/#class-bfloat16-__ge__ "Copy/link to this API element")

@pure @llvm

```python
__ge__(a: bfloat16, b: bfloat16) -> bool
```

```llvm
%tmp = fcmp oge bfloat %a, %b
%res = zext i1 %tmp to i8
ret i8 %res
```

\_\_abs\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/float/#class-bfloat16-__abs__ "Copy/link to this API element")

@pure @llvm

```python
__abs__(a: bfloat16) -> bfloat16
```

```llvm
declare bfloat @llvm.fabs.bf16(bfloat %a)
%tmp = call bfloat @llvm.fabs.bf16(bfloat %a)
ret bfloat %tmp
```

\_\_floor\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/float/#class-bfloat16-__floor__ "Copy/link to this API element")

@pure @llvm

```python
__floor__(a: bfloat16) -> bfloat16
```

```llvm
declare bfloat @llvm.floor.bf16(bfloat %a)
%tmp = call bfloat @llvm.floor.bf16(bfloat %a)
ret bfloat %tmp
```

\_\_ceil\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/float/#class-bfloat16-__ceil__ "Copy/link to this API element")

@pure @llvm

```python
__ceil__(a: bfloat16) -> bfloat16
```

```llvm
declare bfloat @llvm.ceil.bf16(bfloat %a)
%tmp = call bfloat @llvm.ceil.bf16(bfloat %a)
ret bfloat %tmp
```

\_\_trunc\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/float/#class-bfloat16-__trunc__ "Copy/link to this API element")

@pure @llvm

```python
__trunc__(a: bfloat16) -> bfloat16
```

```llvm
declare bfloat @llvm.trunc.bf16(bfloat %a)
%tmp = call bfloat @llvm.trunc.bf16(bfloat %a)
ret bfloat %tmp
```

\_\_pow\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/float/#class-bfloat16-__pow__ "Copy/link to this API element")

@pure @llvm

```python
__pow__(a: bfloat16, b: bfloat16) -> bfloat16
```

```llvm
declare bfloat @llvm.pow.bf16(bfloat %a, bfloat %b)
%tmp = call bfloat @llvm.pow.bf16(bfloat %a, bfloat %b)
ret bfloat %tmp
```

\_\_hash\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/float/#class-bfloat16-__hash__ "Copy/link to this API element")

```python
__hash__(self: Self) -> int
```

\_\_match\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/float/#class-bfloat16-__match__ "Copy/link to this API element")

```python
__match__(self: Self, obj: bfloat16) -> bool
```

class float128[](https://docs.exaloop.io/libraries/api/modules/internal/types/float/#class-float128 "Copy/link to this API element")

@extend

```python
class float128
```

Extends [`internal.core.float128`](https://docs.exaloop.io/libraries/api/classes/internal/core/float128/) · [View complete class API →](https://docs.exaloop.io/libraries/api/classes/internal/core/float128/)

Constructors 3

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/float/#class-float128-__new__ "Copy/link to this API element")

@pure @llvm

```python
__new__(self: float) -> float128
```

```llvm
%0 = fpext double %self to fp128
ret fp128 %0
```

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/float/#class-float128-__new__-overload-2 "Copy/link to this API element")

@overload

```python
__new__(what: float128) -> float128
```

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/float/#class-float128-__new__-overload-3 "Copy/link to this API element")

@overload

```python
__new__() -> float128
```

Methods 2

cdiv[](https://docs.exaloop.io/libraries/api/modules/internal/types/float/#class-float128-cdiv "Copy/link to this API element")

```python
cdiv(self: Self, other: float128) -> float128
```

cmod[](https://docs.exaloop.io/libraries/api/modules/internal/types/float/#class-float128-cmod "Copy/link to this API element")

```python
cmod(self: Self, other: float128) -> float128
```

Special methods 29

\_\_repr\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/float/#class-float128-__repr__ "Copy/link to this API element")

```python
__repr__(self: Self) -> str
```

\_\_format\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/float/#class-float128-__format__ "Copy/link to this API element")

```python
__format__(self: Self, format_spec: str) -> str
```

\_\_copy\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/float/#class-float128-__copy__ "Copy/link to this API element")

```python
__copy__(self: Self) -> float128
```

\_\_deepcopy\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/float/#class-float128-__deepcopy__ "Copy/link to this API element")

```python
__deepcopy__(self: Self) -> float128
```

\_\_int\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/float/#class-float128-__int__ "Copy/link to this API element")

@pure @llvm

```python
__int__(self: Self) -> int
```

```llvm
%0 = fptosi fp128 %self to i64
ret i64 %0
```

\_\_float\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/float/#class-float128-__float__ "Copy/link to this API element")

@pure @llvm

```python
__float__(self: Self) -> float
```

```llvm
%0 = fptrunc fp128 %self to double
ret double %0
```

\_\_bool\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/float/#class-float128-__bool__ "Copy/link to this API element")

@pure @llvm

```python
__bool__(self: Self) -> bool
```

```llvm
%0 = fcmp une fp128 %self, 0xL00000000000000000000000000000000
%1 = zext i1 %0 to i8
ret i8 %1
```

\_\_pos\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/float/#class-float128-__pos__ "Copy/link to this API element")

```python
__pos__(self: Self) -> float128
```

\_\_neg\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/float/#class-float128-__neg__ "Copy/link to this API element")

@pure @llvm

```python
__neg__(self: Self) -> float128
```

```llvm
%0 = fneg fp128 %self
ret fp128 %0
```

\_\_add\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/float/#class-float128-__add__ "Copy/link to this API element")

@pure @commutative @llvm

```python
__add__(a: float128, b: float128) -> float128
```

```llvm
%tmp = fadd fp128 %a, %b
ret fp128 %tmp
```

\_\_sub\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/float/#class-float128-__sub__ "Copy/link to this API element")

@pure @llvm

```python
__sub__(a: float128, b: float128) -> float128
```

```llvm
%tmp = fsub fp128 %a, %b
ret fp128 %tmp
```

\_\_mul\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/float/#class-float128-__mul__ "Copy/link to this API element")

@pure @commutative @llvm

```python
__mul__(a: float128, b: float128) -> float128
```

```llvm
%tmp = fmul fp128 %a, %b
ret fp128 %tmp
```

\_\_floordiv\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/float/#class-float128-__floordiv__ "Copy/link to this API element")

```python
__floordiv__(self: Self, other: float128) -> float128
```

\_\_truediv\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/float/#class-float128-__truediv__ "Copy/link to this API element")

```python
__truediv__(self: Self, other: float128) -> float128
```

\_\_mod\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/float/#class-float128-__mod__ "Copy/link to this API element")

```python
__mod__(self: Self, other: float128) -> float128
```

\_\_divmod\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/float/#class-float128-__divmod__ "Copy/link to this API element")

```python
__divmod__(self: Self, other: float128) -> Tuple[(float128, float128)]
```

\_\_eq\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/float/#class-float128-__eq__ "Copy/link to this API element")

@pure @llvm

```python
__eq__(a: float128, b: float128) -> bool
```

```llvm
%tmp = fcmp oeq fp128 %a, %b
%res = zext i1 %tmp to i8
ret i8 %res
```

\_\_ne\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/float/#class-float128-__ne__ "Copy/link to this API element")

@pure @llvm

```python
__ne__(a: float128, b: float128) -> bool
```

```llvm
%tmp = fcmp une fp128 %a, %b
%res = zext i1 %tmp to i8
ret i8 %res
```

\_\_lt\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/float/#class-float128-__lt__ "Copy/link to this API element")

@pure @llvm

```python
__lt__(a: float128, b: float128) -> bool
```

```llvm
%tmp = fcmp olt fp128 %a, %b
%res = zext i1 %tmp to i8
ret i8 %res
```

\_\_gt\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/float/#class-float128-__gt__ "Copy/link to this API element")

@pure @llvm

```python
__gt__(a: float128, b: float128) -> bool
```

```llvm
%tmp = fcmp ogt fp128 %a, %b
%res = zext i1 %tmp to i8
ret i8 %res
```

\_\_le\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/float/#class-float128-__le__ "Copy/link to this API element")

@pure @llvm

```python
__le__(a: float128, b: float128) -> bool
```

```llvm
%tmp = fcmp ole fp128 %a, %b
%res = zext i1 %tmp to i8
ret i8 %res
```

\_\_ge\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/float/#class-float128-__ge__ "Copy/link to this API element")

@pure @llvm

```python
__ge__(a: float128, b: float128) -> bool
```

```llvm
%tmp = fcmp oge fp128 %a, %b
%res = zext i1 %tmp to i8
ret i8 %res
```

\_\_abs\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/float/#class-float128-__abs__ "Copy/link to this API element")

@pure @llvm

```python
__abs__(a: float128) -> float128
```

```llvm
declare fp128 @llvm.fabs.f128(fp128 %a)
%tmp = call fp128 @llvm.fabs.f128(fp128 %a)
ret fp128 %tmp
```

\_\_floor\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/float/#class-float128-__floor__ "Copy/link to this API element")

@pure @llvm

```python
__floor__(a: float128) -> float128
```

```llvm
declare fp128 @llvm.floor.f128(fp128 %a)
%tmp = call fp128 @llvm.floor.f128(fp128 %a)
ret fp128 %tmp
```

\_\_ceil\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/float/#class-float128-__ceil__ "Copy/link to this API element")

@pure @llvm

```python
__ceil__(a: float128) -> float128
```

```llvm
declare fp128 @llvm.ceil.f128(fp128 %a)
%tmp = call fp128 @llvm.ceil.f128(fp128 %a)
ret fp128 %tmp
```

\_\_trunc\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/float/#class-float128-__trunc__ "Copy/link to this API element")

@pure @llvm

```python
__trunc__(a: float128) -> float128
```

```llvm
declare fp128 @llvm.trunc.f128(fp128 %a)
%tmp = call fp128 @llvm.trunc.f128(fp128 %a)
ret fp128 %tmp
```

\_\_pow\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/float/#class-float128-__pow__ "Copy/link to this API element")

@pure @llvm

```python
__pow__(a: float128, b: float128) -> float128
```

```llvm
declare fp128 @llvm.pow.f128(fp128 %a, fp128 %b)
%tmp = call fp128 @llvm.pow.f128(fp128 %a, fp128 %b)
ret fp128 %tmp
```

\_\_hash\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/float/#class-float128-__hash__ "Copy/link to this API element")

```python
__hash__(self: Self) -> int
```

\_\_match\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/float/#class-float128-__match__ "Copy/link to this API element")

```python
__match__(self: Self, obj: float128) -> bool
```

class float[](https://docs.exaloop.io/libraries/api/modules/internal/types/float/#class-float "Copy/link to this API element")

@extend

```python
class float
```

Extends [`internal.core.float`](https://docs.exaloop.io/libraries/api/classes/internal/core/float/) · [View complete class API →](https://docs.exaloop.io/libraries/api/classes/internal/core/float/)

Special methods 4

\_\_suffix\_f32\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/float/#class-float-__suffix_f32__ "Copy/link to this API element")

```python
__suffix_f32__(double) -> float32
```

\_\_suffix\_f16\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/float/#class-float-__suffix_f16__ "Copy/link to this API element")

```python
__suffix_f16__(double) -> float16
```

\_\_suffix\_bf16\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/float/#class-float-__suffix_bf16__ "Copy/link to this API element")

```python
__suffix_bf16__(double) -> bfloat16
```

\_\_suffix\_f128\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/float/#class-float-__suffix_f128__ "Copy/link to this API element")

```python
__suffix_f128__(double) -> float128
```

alias f16[](https://docs.exaloop.io/libraries/api/modules/internal/types/float/#alias-f16 "Copy/link to this API element")

```python
f16 = float16
```

Alias of [`float16`](https://docs.exaloop.io/libraries/api/classes/internal/core/float16/)

alias bf16[](https://docs.exaloop.io/libraries/api/modules/internal/types/float/#alias-bf16 "Copy/link to this API element")

```python
bf16 = bfloat16
```

Alias of [`bfloat16`](https://docs.exaloop.io/libraries/api/classes/internal/core/bfloat16/)

alias f32[](https://docs.exaloop.io/libraries/api/modules/internal/types/float/#alias-f32 "Copy/link to this API element")

```python
f32 = float32
```

Alias of [`float32`](https://docs.exaloop.io/libraries/api/classes/internal/core/float32/)

alias f64[](https://docs.exaloop.io/libraries/api/modules/internal/types/float/#alias-f64 "Copy/link to this API element")

```python
f64 = float
```

Alias of [`float`](https://docs.exaloop.io/libraries/api/classes/internal/core/float/)

alias f128[](https://docs.exaloop.io/libraries/api/modules/internal/types/float/#alias-f128 "Copy/link to this API element")

```python
f128 = float128
```

Alias of [`float128`](https://docs.exaloop.io/libraries/api/classes/internal/core/float128/)

Source: https://docs.exaloop.io/libraries/api/modules/internal/types/float/
