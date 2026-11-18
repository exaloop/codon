# internal.types.ptr

Standard library Module [View source](https://github.com/exaloop/codon/blob/master/stdlib/internal/types/ptr.codon)

class Ptr[](https://docs.exaloop.io/libraries/api/modules/internal/types/ptr/#class-ptr "Copy/link to this API element")

@extend

```python
class Ptr[T]
```

Extends [`internal.core.Ptr`](https://docs.exaloop.io/libraries/api/classes/internal/core/Ptr/) · [View complete class API →](https://docs.exaloop.io/libraries/api/classes/internal/core/Ptr/)

Constructors 4

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/ptr/#class-ptr-__new__ "Copy/link to this API element")

@pure @llvm

```python
__new__() -> Ptr[T]
```

```llvm
ret ptr null
```

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/ptr/#class-ptr-__new__-overload-2 "Copy/link to this API element")

@overload

```python
__new__(sz: int) -> Ptr[T]
```

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/ptr/#class-ptr-__new__-overload-3 "Copy/link to this API element")

@pure @derives @overload @llvm

```python
__new__(other: Ptr[T]) -> Ptr[T]
```

```llvm
ret ptr %other
```

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/ptr/#class-ptr-__new__-overload-4 "Copy/link to this API element")

@pure @derives @overload @llvm

```python
__new__(other: Ptr) -> Ptr[T]
```

```llvm
ret ptr %other
```

Methods 1

as\_byte[](https://docs.exaloop.io/libraries/api/modules/internal/types/ptr/#class-ptr-as_byte "Copy/link to this API element")

@pure @derives @llvm

```python
as_byte(self: Self) -> Ptr[u8]
```

```llvm
ret ptr %self
```

Special methods 22

\_\_int\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/ptr/#class-ptr-__int__ "Copy/link to this API element")

@pure @llvm

```python
__int__(self: Self) -> int
```

```llvm
%0 = ptrtoint ptr %self to i64
ret i64 %0
```

\_\_copy\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/ptr/#class-ptr-__copy__ "Copy/link to this API element")

@pure @llvm

```python
__copy__(self: Self) -> Ptr[T]
```

```llvm
ret ptr %self
```

\_\_bool\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/ptr/#class-ptr-__bool__ "Copy/link to this API element")

@pure @llvm

```python
__bool__(self: Self) -> bool
```

```llvm
%0 = icmp ne ptr %self, null
%1 = zext i1 %0 to i8
ret i8 %1
```

\_\_getitem\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/ptr/#class-ptr-__getitem__ "Copy/link to this API element")

@pure @derives @llvm

```python
__getitem__(self: Self, index: int) -> T
```

```llvm
%0 = getelementptr {=T}, ptr %self, i64 %index
%1 = load {=T}, ptr %0
ret {=T} %1
```

\_\_setitem\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/ptr/#class-ptr-__setitem__ "Copy/link to this API element")

@self\_captures @llvm

```python
__setitem__(self: Self, index: int, what: T) -> None
```

```llvm
%0 = getelementptr {=T}, ptr %self, i64 %index
store {=T} %what, ptr %0
ret {} {}
```

\_\_add\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/ptr/#class-ptr-__add__ "Copy/link to this API element")

@pure @derives @llvm

```python
__add__(self: Self, other: int) -> Ptr[T]
```

```llvm
%0 = getelementptr {=T}, ptr %self, i64 %other
ret ptr %0
```

\_\_sub\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/ptr/#class-ptr-__sub__ "Copy/link to this API element")

@pure @llvm

```python
__sub__(self: Self, other: Ptr[T]) -> int
```

```llvm
%0 = ptrtoint ptr %self to i64
%1 = ptrtoint ptr %other to i64
%2 = sub i64 %0, %1
%3 = sdiv exact i64 %2, ptrtoint (ptr getelementptr ({=T}, {=T}* null, i32 1) to i64)
ret i64 %3
```

@derives

```python
__sub__(self: Self, other: int) -> Ptr[T]
```

```llvm
%0 = sub i64 0, %other
%1 = getelementptr {=T}, {=T}* %self, i64 %0
ret {=T}* %1
```

\_\_eq\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/ptr/#class-ptr-__eq__ "Copy/link to this API element")

@pure @llvm

```python
__eq__(self: Self, other: Ptr[T]) -> bool
```

```llvm
%0 = icmp eq ptr %self, %other
%1 = zext i1 %0 to i8
ret i8 %1
```

\_\_ne\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/ptr/#class-ptr-__ne__ "Copy/link to this API element")

@pure @llvm

```python
__ne__(self: Self, other: Ptr[T]) -> bool
```

```llvm
%0 = icmp ne ptr %self, %other
%1 = zext i1 %0 to i8
ret i8 %1
```

\_\_lt\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/ptr/#class-ptr-__lt__ "Copy/link to this API element")

@pure @llvm

```python
__lt__(self: Self, other: Ptr[T]) -> bool
```

```llvm
%0 = icmp slt ptr %self, %other
%1 = zext i1 %0 to i8
ret i8 %1
```

\_\_gt\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/ptr/#class-ptr-__gt__ "Copy/link to this API element")

@pure @llvm

```python
__gt__(self: Self, other: Ptr[T]) -> bool
```

```llvm
%0 = icmp sgt ptr %self, %other
%1 = zext i1 %0 to i8
ret i8 %1
```

\_\_le\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/ptr/#class-ptr-__le__ "Copy/link to this API element")

@pure @llvm

```python
__le__(self: Self, other: Ptr[T]) -> bool
```

```llvm
%0 = icmp sle ptr %self, %other
%1 = zext i1 %0 to i8
ret i8 %1
```

\_\_ge\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/ptr/#class-ptr-__ge__ "Copy/link to this API element")

@pure @llvm

```python
__ge__(self: Self, other: Ptr[T]) -> bool
```

```llvm
%0 = icmp sge ptr %self, %other
%1 = zext i1 %0 to i8
ret i8 %1
```

\_\_prefetch\_r0\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/ptr/#class-ptr-__prefetch_r0__ "Copy/link to this API element")

@nocapture @llvm

```python
__prefetch_r0__(self: Self) -> None
```

```llvm
declare void @llvm.prefetch(ptr nocapture readonly, i32, i32, i32)
call void @llvm.prefetch(ptr %self, i32 0, i32 0, i32 1)
ret {} {}
```

\_\_prefetch\_r1\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/ptr/#class-ptr-__prefetch_r1__ "Copy/link to this API element")

@nocapture @llvm

```python
__prefetch_r1__(self: Self) -> None
```

```llvm
declare void @llvm.prefetch(ptr nocapture readonly, i32, i32, i32)
call void @llvm.prefetch(ptr %self, i32 0, i32 1, i32 1)
ret {} {}
```

\_\_prefetch\_r2\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/ptr/#class-ptr-__prefetch_r2__ "Copy/link to this API element")

@nocapture @llvm

```python
__prefetch_r2__(self: Self) -> None
```

```llvm
declare void @llvm.prefetch(ptr nocapture readonly, i32, i32, i32)
call void @llvm.prefetch(ptr %self, i32 0, i32 2, i32 1)
ret {} {}
```

\_\_prefetch\_r3\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/ptr/#class-ptr-__prefetch_r3__ "Copy/link to this API element")

@nocapture @llvm

```python
__prefetch_r3__(self: Self) -> None
```

```llvm
declare void @llvm.prefetch(ptr nocapture readonly, i32, i32, i32)
call void @llvm.prefetch(ptr %self, i32 0, i32 3, i32 1)
ret {} {}
```

\_\_prefetch\_w0\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/ptr/#class-ptr-__prefetch_w0__ "Copy/link to this API element")

@nocapture @llvm

```python
__prefetch_w0__(self: Self) -> None
```

```llvm
declare void @llvm.prefetch(ptr nocapture readonly, i32, i32, i32)
call void @llvm.prefetch(ptr %self, i32 1, i32 0, i32 1)
ret {} {}
```

\_\_prefetch\_w1\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/ptr/#class-ptr-__prefetch_w1__ "Copy/link to this API element")

@nocapture @llvm

```python
__prefetch_w1__(self: Self) -> None
```

```llvm
declare void @llvm.prefetch(ptr nocapture readonly, i32, i32, i32)
call void @llvm.prefetch(ptr %self, i32 1, i32 1, i32 1)
ret {} {}
```

\_\_prefetch\_w2\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/ptr/#class-ptr-__prefetch_w2__ "Copy/link to this API element")

@nocapture @llvm

```python
__prefetch_w2__(self: Self) -> None
```

```llvm
declare void @llvm.prefetch(ptr nocapture readonly, i32, i32, i32)
call void @llvm.prefetch(ptr %self, i32 1, i32 2, i32 1)
ret {} {}
```

\_\_prefetch\_w3\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/ptr/#class-ptr-__prefetch_w3__ "Copy/link to this API element")

@nocapture @llvm

```python
__prefetch_w3__(self: Self) -> None
```

```llvm
declare void @llvm.prefetch(ptr nocapture readonly, i32, i32, i32)
call void @llvm.prefetch(ptr %self, i32 1, i32 3, i32 1)
ret {} {}
```

\_\_repr\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/ptr/#class-ptr-__repr__ "Copy/link to this API element")

```python
__repr__(self: Self) -> str
```

alias ptr[](https://docs.exaloop.io/libraries/api/modules/internal/types/ptr/#alias-ptr "Copy/link to this API element")

```python
ptr = Ptr
```

Alias of [`Ptr`](https://docs.exaloop.io/libraries/api/classes/internal/core/Ptr/)

alias Jar[](https://docs.exaloop.io/libraries/api/modules/internal/types/ptr/#alias-jar "Copy/link to this API element")

```python
Jar = Ptr[u8]
```

Alias of [`Ptr[u8]`](https://docs.exaloop.io/libraries/api/classes/internal/core/Ptr/)

class NoneType[](https://docs.exaloop.io/libraries/api/modules/internal/types/ptr/#class-nonetype "Copy/link to this API element")

@extend

```python
class NoneType
```

Extends [`internal.core.NoneType`](https://docs.exaloop.io/libraries/api/classes/internal/core/NoneType/) · [View complete class API →](https://docs.exaloop.io/libraries/api/classes/internal/core/NoneType/)

Special methods 4

\_\_eq\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/ptr/#class-nonetype-__eq__ "Copy/link to this API element")

```python
__eq__(self: Self, other: NoneType)
```

\_\_ne\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/ptr/#class-nonetype-__ne__ "Copy/link to this API element")

```python
__ne__(self: Self, other: NoneType)
```

\_\_bool\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/ptr/#class-nonetype-__bool__ "Copy/link to this API element")

```python
__bool__(self: Self) -> bool
```

\_\_repr\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/ptr/#class-nonetype-__repr__ "Copy/link to this API element")

```python
__repr__(self: Self) -> str
```

Source: https://docs.exaloop.io/libraries/api/modules/internal/types/ptr/
