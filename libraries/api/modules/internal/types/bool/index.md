# internal.types.bool

Standard library Module [View source](https://github.com/exaloop/codon/blob/master/stdlib/internal/types/bool.codon)

class bool[](https://docs.exaloop.io/libraries/api/modules/internal/types/bool/#class-bool "Copy/link to this API element")

@extend

```python
class bool
```

Extends [`internal.core.bool`](https://docs.exaloop.io/libraries/api/classes/internal/core/bool/) · [View complete class API →](https://docs.exaloop.io/libraries/api/classes/internal/core/bool/)

Constructors 2

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/bool/#class-bool-__new__ "Copy/link to this API element")

```python
__new__() -> bool
```

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/bool/#class-bool-__new__-overload-2 "Copy/link to this API element")

@overload

```python
__new__(what, /) -> bool
```

Special methods 27

\_\_repr\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/bool/#class-bool-__repr__ "Copy/link to this API element")

```python
__repr__(self: Self, /) -> str
```

\_\_copy\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/bool/#class-bool-__copy__ "Copy/link to this API element")

```python
__copy__(self: Self) -> bool
```

\_\_deepcopy\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/bool/#class-bool-__deepcopy__ "Copy/link to this API element")

```python
__deepcopy__(self: Self) -> bool
```

\_\_bool\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/bool/#class-bool-__bool__ "Copy/link to this API element")

```python
__bool__(self: Self, /) -> bool
```

\_\_hash\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/bool/#class-bool-__hash__ "Copy/link to this API element")

```python
__hash__(self: Self, /) -> int
```

\_\_add\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/bool/#class-bool-__add__ "Copy/link to this API element")

@commutative

```python
__add__(self: Self, other: bool, /)
```

\_\_sub\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/bool/#class-bool-__sub__ "Copy/link to this API element")

```python
__sub__(self: Self, other: bool, /)
```

\_\_mul\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/bool/#class-bool-__mul__ "Copy/link to this API element")

@commutative

```python
__mul__(self: Self, other: bool, /)
```

\_\_lshift\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/bool/#class-bool-__lshift__ "Copy/link to this API element")

```python
__lshift__(self: Self, other: bool, /)
```

\_\_rshift\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/bool/#class-bool-__rshift__ "Copy/link to this API element")

```python
__rshift__(self: Self, other: bool, /)
```

\_\_floordiv\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/bool/#class-bool-__floordiv__ "Copy/link to this API element")

```python
__floordiv__(self: Self, other: bool, /)
```

\_\_truediv\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/bool/#class-bool-__truediv__ "Copy/link to this API element")

```python
__truediv__(self: Self, other: bool, /)
```

\_\_mod\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/bool/#class-bool-__mod__ "Copy/link to this API element")

```python
__mod__(self: Self, other: bool, /)
```

\_\_divmod\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/bool/#class-bool-__divmod__ "Copy/link to this API element")

```python
__divmod__(self: Self, other: bool, /)
```

\_\_pow\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/bool/#class-bool-__pow__ "Copy/link to this API element")

```python
__pow__(self: Self, other: bool, /)
```

\_\_invert\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/bool/#class-bool-__invert__ "Copy/link to this API element")

@pure @llvm

```python
__invert__(self: Self, /) -> bool
```

```llvm
%0 = trunc i8 %self to i1
%1 = xor i1 %0, true
%2 = zext i1 %1 to i8
ret i8 %2
```

\_\_eq\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/bool/#class-bool-__eq__ "Copy/link to this API element")

@pure @llvm

```python
__eq__(self: Self, other: bool, /) -> bool
```

```llvm
%0 = icmp eq i8 %self, %other
%1 = zext i1 %0 to i8
ret i8 %1
```

\_\_ne\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/bool/#class-bool-__ne__ "Copy/link to this API element")

@pure @llvm

```python
__ne__(self: Self, other: bool, /) -> bool
```

```llvm
%0 = icmp ne i8 %self, %other
%1 = zext i1 %0 to i8
ret i8 %1
```

\_\_lt\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/bool/#class-bool-__lt__ "Copy/link to this API element")

@pure @llvm

```python
__lt__(self: Self, other: bool, /) -> bool
```

```llvm
%0 = icmp ult i8 %self, %other
%1 = zext i1 %0 to i8
ret i8 %1
```

\_\_gt\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/bool/#class-bool-__gt__ "Copy/link to this API element")

@pure @llvm

```python
__gt__(self: Self, other: bool, /) -> bool
```

```llvm
%0 = icmp ugt i8 %self, %other
%1 = zext i1 %0 to i8
ret i8 %1
```

\_\_le\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/bool/#class-bool-__le__ "Copy/link to this API element")

@pure @llvm

```python
__le__(self: Self, other: bool, /) -> bool
```

```llvm
%0 = icmp ule i8 %self, %other
%1 = zext i1 %0 to i8
ret i8 %1
```

\_\_ge\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/bool/#class-bool-__ge__ "Copy/link to this API element")

@pure @llvm

```python
__ge__(self: Self, other: bool, /) -> bool
```

```llvm
%0 = icmp uge i8 %self, %other
%1 = zext i1 %0 to i8
ret i8 %1
```

\_\_and\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/bool/#class-bool-__and__ "Copy/link to this API element")

@pure @commutative @associative @llvm

```python
__and__(self: Self, other: bool, /) -> bool
```

```llvm
%0 = and i8 %self, %other
ret i8 %0
```

\_\_or\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/bool/#class-bool-__or__ "Copy/link to this API element")

@pure @commutative @associative @llvm

```python
__or__(self: Self, other: bool, /) -> bool
```

```llvm
%0 = or i8 %self, %other
ret i8 %0
```

\_\_xor\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/bool/#class-bool-__xor__ "Copy/link to this API element")

@pure @commutative @associative @llvm

```python
__xor__(self: Self, other: bool, /) -> bool
```

```llvm
%0 = xor i8 %self, %other
ret i8 %0
```

\_\_int\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/bool/#class-bool-__int__ "Copy/link to this API element")

@pure @llvm

```python
__int__(self: Self, /) -> int
```

```llvm
%0 = zext i8 %self to i64
ret i64 %0
```

\_\_float\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/bool/#class-bool-__float__ "Copy/link to this API element")

@pure @llvm

```python
__float__(self: Self, /) -> float
```

```llvm
%0 = uitofp i8 %self to double
ret double %0
```

Source: https://docs.exaloop.io/libraries/api/modules/internal/types/bool/
