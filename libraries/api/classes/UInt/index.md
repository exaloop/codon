# UInt

Standard library class

class UInt[](https://docs.exaloop.io/libraries/api/classes/UInt/#class-uint "Copy/link to this API element")

@tuple

```python
class UInt[N]
```

Fields 1

`N` `Literal[int]`

Constructors 11

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/classes/UInt/#class-uint-__new__ "Copy/link to this API element")

```python
__new__(what)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/classes/UInt/#class-uint-__new__-overload-2 "Copy/link to this API element")

```python
__new__(what: bool)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/classes/UInt/#class-uint-__new__-overload-3 "Copy/link to this API element")

```python
__new__(what: float)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/classes/UInt/#class-uint-__new__-overload-4 "Copy/link to this API element")

```python
__new__(what: float128)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/classes/UInt/#class-uint-__new__-overload-5 "Copy/link to this API element")

```python
__new__(what: float32)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/classes/UInt/#class-uint-__new__-overload-6 "Copy/link to this API element")

```python
__new__(what: float16)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/classes/UInt/#class-uint-__new__-overload-7 "Copy/link to this API element")

```python
__new__(what: bfloat16)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/classes/UInt/#class-uint-__new__-overload-8 "Copy/link to this API element")

```python
__new__()
```

Added by [`internal.types.int`](https://docs.exaloop.io/libraries/api/modules/internal/types/int/)

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/classes/UInt/#class-uint-__new__-overload-9 "Copy/link to this API element")

@overload

```python
__new__(what: UInt[M], M: Literal[int])
```

Added by [`internal.types.int`](https://docs.exaloop.io/libraries/api/modules/internal/types/int/)

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/classes/UInt/#class-uint-__new__-overload-10 "Copy/link to this API element")

@overload

```python
__new__(what: Int[M], M: Literal[int])
```

Added by [`internal.types.int`](https://docs.exaloop.io/libraries/api/modules/internal/types/int/)

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/classes/UInt/#class-uint-__new__-overload-11 "Copy/link to this API element")

```python
__new__(what: str, base: int = 10)
```

Added by [`internal.types.int`](https://docs.exaloop.io/libraries/api/modules/internal/types/int/)

Properties 2

real[](https://docs.exaloop.io/libraries/api/classes/UInt/#class-uint-real "Copy/link to this API element")

@property

```python
real(self: Self, Self: type)
```

Added by [`internal.types.int`](https://docs.exaloop.io/libraries/api/modules/internal/types/int/)

imag[](https://docs.exaloop.io/libraries/api/classes/UInt/#class-uint-imag "Copy/link to this API element")

@property

```python
imag(self: Self, Self: type)
```

Added by [`internal.types.int`](https://docs.exaloop.io/libraries/api/modules/internal/types/int/)

Methods 4

cdiv[](https://docs.exaloop.io/libraries/api/classes/UInt/#class-uint-cdiv "Copy/link to this API element")

```python
cdiv(self: Self, other: UInt[N], Self: type)
```

Added by [`internal.types.int`](https://docs.exaloop.io/libraries/api/modules/internal/types/int/)

cmod[](https://docs.exaloop.io/libraries/api/classes/UInt/#class-uint-cmod "Copy/link to this API element")

```python
cmod(self: Self, other: UInt[N], Self: type)
```

Added by [`internal.types.int`](https://docs.exaloop.io/libraries/api/modules/internal/types/int/)

popcnt[](https://docs.exaloop.io/libraries/api/classes/UInt/#class-uint-popcnt "Copy/link to this API element")

```python
popcnt(self: Self, Self: type)
```

Added by [`internal.types.int`](https://docs.exaloop.io/libraries/api/modules/internal/types/int/)

bit\_length[](https://docs.exaloop.io/libraries/api/classes/UInt/#class-uint-bit_length "Copy/link to this API element")

```python
bit_length(self: Self, Self: type)
```

Added by [`internal.types.int`](https://docs.exaloop.io/libraries/api/modules/internal/types/int/)

Special methods 50

\_\_add\_\_[](https://docs.exaloop.io/libraries/api/classes/UInt/#class-uint-__add__ "Copy/link to this API element")

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

@pure @associative @llvm

```python
__add__(self: Self, other: UInt[N], Self: type)
```

Added by [`internal.types.int`](https://docs.exaloop.io/libraries/api/modules/internal/types/int/)

```llvm
%0 = add i{=N} %self, %other
ret i{=N} %0
```

\_\_sub\_\_[](https://docs.exaloop.io/libraries/api/classes/UInt/#class-uint-__sub__ "Copy/link to this API element")

```python
__sub__(self: Self, b: float, Self: type)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

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
__sub__(self: Self, other: UInt[N], Self: type)
```

Added by [`internal.types.int`](https://docs.exaloop.io/libraries/api/modules/internal/types/int/)

```llvm
%0 = sub i{=N} %self, %other
ret i{=N} %0
```

\_\_mul\_\_[](https://docs.exaloop.io/libraries/api/classes/UInt/#class-uint-__mul__ "Copy/link to this API element")

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

@pure @associative @distributive @llvm

```python
__mul__(self: Self, other: UInt[N], Self: type)
```

Added by [`internal.types.int`](https://docs.exaloop.io/libraries/api/modules/internal/types/int/)

```llvm
%0 = mul i{=N} %self, %other
ret i{=N} %0
```

\_\_floordiv\_\_[](https://docs.exaloop.io/libraries/api/classes/UInt/#class-uint-__floordiv__ "Copy/link to this API element")

```python
__floordiv__(self: Self, b: float, Self: type)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

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
__floordiv__(self: Self, other: UInt[N], Self: type)
```

Added by [`internal.types.int`](https://docs.exaloop.io/libraries/api/modules/internal/types/int/)

\_\_truediv\_\_[](https://docs.exaloop.io/libraries/api/classes/UInt/#class-uint-__truediv__ "Copy/link to this API element")

```python
__truediv__(self: Self, b: float, Self: type)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

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
__truediv__(self: Self, other: UInt[N], Self: type)
```

Added by [`internal.types.int`](https://docs.exaloop.io/libraries/api/modules/internal/types/int/)

\_\_mod\_\_[](https://docs.exaloop.io/libraries/api/classes/UInt/#class-uint-__mod__ "Copy/link to this API element")

```python
__mod__(self: Self, b: float, Self: type)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

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
__mod__(self: Self, other: UInt[N], Self: type)
```

Added by [`internal.types.int`](https://docs.exaloop.io/libraries/api/modules/internal/types/int/)

\_\_divmod\_\_[](https://docs.exaloop.io/libraries/api/classes/UInt/#class-uint-__divmod__ "Copy/link to this API element")

```python
__divmod__(self: Self, b: float, Self: type)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

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
__divmod__(self: Self, other: UInt[N], Self: type)
```

Added by [`internal.types.int`](https://docs.exaloop.io/libraries/api/modules/internal/types/int/)

\_\_pow\_\_[](https://docs.exaloop.io/libraries/api/classes/UInt/#class-uint-__pow__ "Copy/link to this API element")

```python
__pow__(self: Self, b: float, Self: type)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

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

```python
__pow__(self: Self, exp: UInt[N], Self: type)
```

Added by [`internal.types.int`](https://docs.exaloop.io/libraries/api/modules/internal/types/int/)

```python
__pow__(self: Self, exp: Literal[int], Self: type)
```

Added by [`internal.types.int`](https://docs.exaloop.io/libraries/api/modules/internal/types/int/)

\_\_eq\_\_[](https://docs.exaloop.io/libraries/api/classes/UInt/#class-uint-__eq__ "Copy/link to this API element")

```python
__eq__(self: Self, b: float, Self: type)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

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
__eq__(self: Self, other: UInt[N], Self: type)
```

Added by [`internal.types.int`](https://docs.exaloop.io/libraries/api/modules/internal/types/int/)

```llvm
%0 = icmp eq i{=N} %self, %other
%1 = zext i1 %0 to i8
ret i8 %1
```

\_\_ne\_\_[](https://docs.exaloop.io/libraries/api/classes/UInt/#class-uint-__ne__ "Copy/link to this API element")

```python
__ne__(self: Self, b: float, Self: type)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

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
__ne__(self: Self, other: UInt[N], Self: type)
```

Added by [`internal.types.int`](https://docs.exaloop.io/libraries/api/modules/internal/types/int/)

```llvm
%0 = icmp ne i{=N} %self, %other
%1 = zext i1 %0 to i8
ret i8 %1
```

\_\_lt\_\_[](https://docs.exaloop.io/libraries/api/classes/UInt/#class-uint-__lt__ "Copy/link to this API element")

```python
__lt__(self: Self, b: float, Self: type)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

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
__lt__(self: Self, other: UInt[N], Self: type)
```

Added by [`internal.types.int`](https://docs.exaloop.io/libraries/api/modules/internal/types/int/)

```llvm
%0 = icmp ult i{=N} %self, %other
%1 = zext i1 %0 to i8
ret i8 %1
```

\_\_gt\_\_[](https://docs.exaloop.io/libraries/api/classes/UInt/#class-uint-__gt__ "Copy/link to this API element")

```python
__gt__(self: Self, b: float, Self: type)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

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
__gt__(self: Self, other: UInt[N], Self: type)
```

Added by [`internal.types.int`](https://docs.exaloop.io/libraries/api/modules/internal/types/int/)

```llvm
%0 = icmp ugt i{=N} %self, %other
%1 = zext i1 %0 to i8
ret i8 %1
```

\_\_le\_\_[](https://docs.exaloop.io/libraries/api/classes/UInt/#class-uint-__le__ "Copy/link to this API element")

```python
__le__(self: Self, b: float, Self: type)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

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
__le__(self: Self, other: UInt[N], Self: type)
```

Added by [`internal.types.int`](https://docs.exaloop.io/libraries/api/modules/internal/types/int/)

```llvm
%0 = icmp ule i{=N} %self, %other
%1 = zext i1 %0 to i8
ret i8 %1
```

\_\_ge\_\_[](https://docs.exaloop.io/libraries/api/classes/UInt/#class-uint-__ge__ "Copy/link to this API element")

```python
__ge__(self: Self, b: float, Self: type)
```

Added by [`internal.ops`](https://docs.exaloop.io/libraries/api/modules/internal/ops/)

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
__ge__(self: Self, other: UInt[N], Self: type)
```

Added by [`internal.types.int`](https://docs.exaloop.io/libraries/api/modules/internal/types/int/)

```llvm
%0 = icmp uge i{=N} %self, %other
%1 = zext i1 %0 to i8
ret i8 %1
```

\_\_complex\_\_[](https://docs.exaloop.io/libraries/api/classes/UInt/#class-uint-__complex__ "Copy/link to this API element")

```python
__complex__(self: Self, Self: type)
```

Added by [`internal.types.complex`](https://docs.exaloop.io/libraries/api/modules/internal/types/complex/)

\_\_format\_\_[](https://docs.exaloop.io/libraries/api/classes/UInt/#class-uint-__format__ "Copy/link to this API element")

```python
__format__(self: Self, format_spec: str, Self: type)
```

Added by [`internal.format`](https://docs.exaloop.io/libraries/api/modules/internal/format/)

\_\_str\_\_[](https://docs.exaloop.io/libraries/api/classes/UInt/#class-uint-__str__ "Copy/link to this API element")

```python
__str__(self: Self, Self: type)
```

Added by [`internal.format`](https://docs.exaloop.io/libraries/api/modules/internal/format/)

\_\_pickle\_\_[](https://docs.exaloop.io/libraries/api/classes/UInt/#class-uint-__pickle__ "Copy/link to this API element")

```python
__pickle__(self: Self, jar: Jar, Self: type)
```

Added by [`pickle`](https://docs.exaloop.io/libraries/api/modules/pickle/)

\_\_unpickle\_\_[](https://docs.exaloop.io/libraries/api/classes/UInt/#class-uint-__unpickle__ "Copy/link to this API element")

```python
__unpickle__(jar: Jar)
```

Added by [`pickle`](https://docs.exaloop.io/libraries/api/modules/pickle/)

\_\_to\_gpu\_\_[](https://docs.exaloop.io/libraries/api/classes/UInt/#class-uint-__to_gpu__ "Copy/link to this API element")

```python
__to_gpu__(self: Self, cache: AllocCache, Self: type)
```

Added by [`internal.gpu`](https://docs.exaloop.io/libraries/api/modules/internal/gpu/)

\_\_from\_gpu\_\_[](https://docs.exaloop.io/libraries/api/classes/UInt/#class-uint-__from_gpu__ "Copy/link to this API element")

```python
__from_gpu__(self: Self, other: UInt[N], Self: type)
```

Added by [`internal.gpu`](https://docs.exaloop.io/libraries/api/modules/internal/gpu/)

\_\_from\_gpu\_new\_\_[](https://docs.exaloop.io/libraries/api/classes/UInt/#class-uint-__from_gpu_new__ "Copy/link to this API element")

```python
__from_gpu_new__(other: UInt[N])
```

Added by [`internal.gpu`](https://docs.exaloop.io/libraries/api/modules/internal/gpu/)

\_\_int\_\_[](https://docs.exaloop.io/libraries/api/classes/UInt/#class-uint-__int__ "Copy/link to this API element")

```python
__int__(self: Self, Self: type)
```

Added by [`internal.types.int`](https://docs.exaloop.io/libraries/api/modules/internal/types/int/)

\_\_index\_\_[](https://docs.exaloop.io/libraries/api/classes/UInt/#class-uint-__index__ "Copy/link to this API element")

```python
__index__(self: Self, Self: type)
```

Added by [`internal.types.int`](https://docs.exaloop.io/libraries/api/modules/internal/types/int/)

\_\_copy\_\_[](https://docs.exaloop.io/libraries/api/classes/UInt/#class-uint-__copy__ "Copy/link to this API element")

```python
__copy__(self: Self, Self: type)
```

Added by [`internal.types.int`](https://docs.exaloop.io/libraries/api/modules/internal/types/int/)

\_\_deepcopy\_\_[](https://docs.exaloop.io/libraries/api/classes/UInt/#class-uint-__deepcopy__ "Copy/link to this API element")

```python
__deepcopy__(self: Self, Self: type)
```

Added by [`internal.types.int`](https://docs.exaloop.io/libraries/api/modules/internal/types/int/)

\_\_hash\_\_[](https://docs.exaloop.io/libraries/api/classes/UInt/#class-uint-__hash__ "Copy/link to this API element")

```python
__hash__(self: Self, Self: type)
```

Added by [`internal.types.int`](https://docs.exaloop.io/libraries/api/modules/internal/types/int/)

\_\_float\_\_[](https://docs.exaloop.io/libraries/api/classes/UInt/#class-uint-__float__ "Copy/link to this API element")

@pure @llvm

```python
__float__(self: Self, Self: type)
```

Added by [`internal.types.int`](https://docs.exaloop.io/libraries/api/modules/internal/types/int/)

```llvm
%0 = uitofp i{=N} %self to double
ret double %0
```

\_\_bool\_\_[](https://docs.exaloop.io/libraries/api/classes/UInt/#class-uint-__bool__ "Copy/link to this API element")

@pure @llvm

```python
__bool__(self: Self, Self: type)
```

Added by [`internal.types.int`](https://docs.exaloop.io/libraries/api/modules/internal/types/int/)

```llvm
%0 = icmp ne i{=N} %self, 0
%1 = zext i1 %0 to i8
ret i8 %1
```

\_\_pos\_\_[](https://docs.exaloop.io/libraries/api/classes/UInt/#class-uint-__pos__ "Copy/link to this API element")

```python
__pos__(self: Self, Self: type)
```

Added by [`internal.types.int`](https://docs.exaloop.io/libraries/api/modules/internal/types/int/)

\_\_neg\_\_[](https://docs.exaloop.io/libraries/api/classes/UInt/#class-uint-__neg__ "Copy/link to this API element")

@pure @llvm

```python
__neg__(self: Self, Self: type)
```

Added by [`internal.types.int`](https://docs.exaloop.io/libraries/api/modules/internal/types/int/)

```llvm
%0 = sub i{=N} 0, %self
ret i{=N} %0
```

\_\_invert\_\_[](https://docs.exaloop.io/libraries/api/classes/UInt/#class-uint-__invert__ "Copy/link to this API element")

@pure @llvm

```python
__invert__(self: Self, Self: type)
```

Added by [`internal.types.int`](https://docs.exaloop.io/libraries/api/modules/internal/types/int/)

```llvm
%0 = xor i{=N} %self, -1
ret i{=N} %0
```

\_\_abs\_\_[](https://docs.exaloop.io/libraries/api/classes/UInt/#class-uint-__abs__ "Copy/link to this API element")

```python
__abs__(self: Self, Self: type)
```

Added by [`internal.types.int`](https://docs.exaloop.io/libraries/api/modules/internal/types/int/)

\_\_lshift\_\_[](https://docs.exaloop.io/libraries/api/classes/UInt/#class-uint-__lshift__ "Copy/link to this API element")

```python
__lshift__(self: Self, other: UInt[N], Self: type)
```

Added by [`internal.types.int`](https://docs.exaloop.io/libraries/api/modules/internal/types/int/)

```python
__lshift__(self: Self, other: int, Self: type)
```

Added by [`internal.types.int`](https://docs.exaloop.io/libraries/api/modules/internal/types/int/)

\_\_rshift\_\_[](https://docs.exaloop.io/libraries/api/classes/UInt/#class-uint-__rshift__ "Copy/link to this API element")

```python
__rshift__(self: Self, other: UInt[N], Self: type)
```

Added by [`internal.types.int`](https://docs.exaloop.io/libraries/api/modules/internal/types/int/)

```python
__rshift__(self: Self, other: int, Self: type)
```

Added by [`internal.types.int`](https://docs.exaloop.io/libraries/api/modules/internal/types/int/)

\_\_and\_\_[](https://docs.exaloop.io/libraries/api/classes/UInt/#class-uint-__and__ "Copy/link to this API element")

@pure @commutative @associative @llvm

```python
__and__(self: Self, other: UInt[N], Self: type)
```

Added by [`internal.types.int`](https://docs.exaloop.io/libraries/api/modules/internal/types/int/)

```llvm
%0 = and i{=N} %self, %other
ret i{=N} %0
```

\_\_or\_\_[](https://docs.exaloop.io/libraries/api/classes/UInt/#class-uint-__or__ "Copy/link to this API element")

@pure @commutative @associative @llvm

```python
__or__(self: Self, other: UInt[N], Self: type)
```

Added by [`internal.types.int`](https://docs.exaloop.io/libraries/api/modules/internal/types/int/)

```llvm
%0 = or i{=N} %self, %other
ret i{=N} %0
```

\_\_xor\_\_[](https://docs.exaloop.io/libraries/api/classes/UInt/#class-uint-__xor__ "Copy/link to this API element")

@pure @commutative @associative @llvm

```python
__xor__(self: Self, other: UInt[N], Self: type)
```

Added by [`internal.types.int`](https://docs.exaloop.io/libraries/api/modules/internal/types/int/)

```llvm
%0 = xor i{=N} %self, %other
ret i{=N} %0
```

\_\_repr\_\_[](https://docs.exaloop.io/libraries/api/classes/UInt/#class-uint-__repr__ "Copy/link to this API element")

```python
__repr__(self: Self, Self: type)
```

Added by [`internal.types.int`](https://docs.exaloop.io/libraries/api/modules/internal/types/int/)

\_\_match\_\_[](https://docs.exaloop.io/libraries/api/classes/UInt/#class-uint-__match__ "Copy/link to this API element")

```python
__match__(self: Self, obj: UInt[N], Self: type)
```

Added by [`internal.types.int`](https://docs.exaloop.io/libraries/api/modules/internal/types/int/)

\_\_round\_\_[](https://docs.exaloop.io/libraries/api/classes/UInt/#class-uint-__round__ "Copy/link to this API element")

```python
__round__(self: Self, ndigits = None, Self: type)
```

Added by [`internal.types.int`](https://docs.exaloop.io/libraries/api/modules/internal/types/int/)

\_\_atomic\_xchg\_\_[](https://docs.exaloop.io/libraries/api/classes/UInt/#class-uint-__atomic_xchg__ "Copy/link to this API element")

@nocapture @llvm

```python
__atomic_xchg__(d: Ptr[UInt[N]], b: UInt[N])
```

Added by [`internal.types.int`](https://docs.exaloop.io/libraries/api/modules/internal/types/int/)

```llvm
%tmp = atomicrmw xchg ptr %d, i{=N} %b seq_cst
ret i{=N} %tmp
```

\_\_atomic\_add\_\_[](https://docs.exaloop.io/libraries/api/classes/UInt/#class-uint-__atomic_add__ "Copy/link to this API element")

@nocapture @llvm

```python
__atomic_add__(d: Ptr[UInt[N]], b: UInt[N])
```

Added by [`internal.types.int`](https://docs.exaloop.io/libraries/api/modules/internal/types/int/)

```llvm
%tmp = atomicrmw add ptr %d, i{=N} %b seq_cst
ret i{=N} %tmp
```

\_\_atomic\_sub\_\_[](https://docs.exaloop.io/libraries/api/classes/UInt/#class-uint-__atomic_sub__ "Copy/link to this API element")

@nocapture @llvm

```python
__atomic_sub__(d: Ptr[UInt[N]], b: UInt[N])
```

Added by [`internal.types.int`](https://docs.exaloop.io/libraries/api/modules/internal/types/int/)

```llvm
%tmp = atomicrmw sub ptr %d, i{=N} %b seq_cst
ret i{=N} %tmp
```

\_\_atomic\_and\_\_[](https://docs.exaloop.io/libraries/api/classes/UInt/#class-uint-__atomic_and__ "Copy/link to this API element")

@nocapture @llvm

```python
__atomic_and__(d: Ptr[UInt[N]], b: UInt[N])
```

Added by [`internal.types.int`](https://docs.exaloop.io/libraries/api/modules/internal/types/int/)

```llvm
%tmp = atomicrmw and ptr %d, i{=N} %b seq_cst
ret i{=N} %tmp
```

\_\_atomic\_nand\_\_[](https://docs.exaloop.io/libraries/api/classes/UInt/#class-uint-__atomic_nand__ "Copy/link to this API element")

@nocapture @llvm

```python
__atomic_nand__(d: Ptr[UInt[N]], b: UInt[N])
```

Added by [`internal.types.int`](https://docs.exaloop.io/libraries/api/modules/internal/types/int/)

```llvm
%tmp = atomicrmw nand ptr %d, i{=N} %b seq_cst
ret i{=N} %tmp
```

\_\_atomic\_or\_\_[](https://docs.exaloop.io/libraries/api/classes/UInt/#class-uint-__atomic_or__ "Copy/link to this API element")

@nocapture @llvm

```python
__atomic_or__(d: Ptr[UInt[N]], b: UInt[N])
```

Added by [`internal.types.int`](https://docs.exaloop.io/libraries/api/modules/internal/types/int/)

```llvm
%tmp = atomicrmw or ptr %d, i{=N} %b seq_cst
ret i{=N} %tmp
```

\_\_atomic\_xor\_\_[](https://docs.exaloop.io/libraries/api/classes/UInt/#class-uint-__atomic_xor__ "Copy/link to this API element")

@nocapture @llvm

```python
__atomic_xor__(d: Ptr[UInt[N]], b: UInt[N])
```

Added by [`internal.types.int`](https://docs.exaloop.io/libraries/api/modules/internal/types/int/)

```llvm
%tmp = atomicrmw xor ptr %d, i{=N} %b seq_cst
ret i{=N} %tmp
```

\_\_atomic\_min\_\_[](https://docs.exaloop.io/libraries/api/classes/UInt/#class-uint-__atomic_min__ "Copy/link to this API element")

@nocapture @llvm

```python
__atomic_min__(d: Ptr[UInt[N]], b: UInt[N])
```

Added by [`internal.types.int`](https://docs.exaloop.io/libraries/api/modules/internal/types/int/)

```llvm
%tmp = atomicrmw umin ptr %d, i{=N} %b seq_cst
ret i{=N} %tmp
```

\_\_atomic\_max\_\_[](https://docs.exaloop.io/libraries/api/classes/UInt/#class-uint-__atomic_max__ "Copy/link to this API element")

@nocapture @llvm

```python
__atomic_max__(d: Ptr[UInt[N]], b: UInt[N])
```

Added by [`internal.types.int`](https://docs.exaloop.io/libraries/api/modules/internal/types/int/)

```llvm
%tmp = atomicrmw umax ptr %d, i{=N} %b seq_cst
ret i{=N} %tmp
```

Source: https://docs.exaloop.io/libraries/api/classes/UInt/
