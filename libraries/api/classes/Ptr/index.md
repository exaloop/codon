# Ptr

Standard library class

class Ptr[](https://docs.exaloop.io/libraries/api/classes/Ptr/#class-ptr "Copy/link to this API element")

@tuple

```python
class Ptr[T]
```

Fields 1

`T` `type`

Constructors 4

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/classes/Ptr/#class-ptr-__new__ "Copy/link to this API element")

@pure @llvm

```python
__new__()
```

Added by [`internal.types.ptr`](https://docs.exaloop.io/libraries/api/modules/internal/types/ptr/)

```llvm
ret ptr null
```

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/classes/Ptr/#class-ptr-__new__-overload-2 "Copy/link to this API element")

@overload

```python
__new__(sz: int)
```

Added by [`internal.types.ptr`](https://docs.exaloop.io/libraries/api/modules/internal/types/ptr/)

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/classes/Ptr/#class-ptr-__new__-overload-3 "Copy/link to this API element")

@pure @derives @overload @llvm

```python
__new__(other: Ptr[T])
```

Added by [`internal.types.ptr`](https://docs.exaloop.io/libraries/api/modules/internal/types/ptr/)

```llvm
ret ptr %other
```

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/classes/Ptr/#class-ptr-__new__-overload-4 "Copy/link to this API element")

@pure @derives @overload @llvm

```python
__new__(other: Ptr)
```

Added by [`internal.types.ptr`](https://docs.exaloop.io/libraries/api/modules/internal/types/ptr/)

```llvm
ret ptr %other
```

Methods 1

as\_byte[](https://docs.exaloop.io/libraries/api/classes/Ptr/#class-ptr-as_byte "Copy/link to this API element")

@pure @derives @llvm

```python
as_byte(self: Self, Self: type)
```

Added by [`internal.types.ptr`](https://docs.exaloop.io/libraries/api/modules/internal/types/ptr/)

```llvm
ret ptr %self
```

Special methods 26

\_\_int\_\_[](https://docs.exaloop.io/libraries/api/classes/Ptr/#class-ptr-__int__ "Copy/link to this API element")

@pure @llvm

```python
__int__(self: Self, Self: type)
```

Added by [`internal.types.ptr`](https://docs.exaloop.io/libraries/api/modules/internal/types/ptr/)

```llvm
%0 = ptrtoint ptr %self to i64
ret i64 %0
```

\_\_copy\_\_[](https://docs.exaloop.io/libraries/api/classes/Ptr/#class-ptr-__copy__ "Copy/link to this API element")

@pure @llvm

```python
__copy__(self: Self, Self: type)
```

Added by [`internal.types.ptr`](https://docs.exaloop.io/libraries/api/modules/internal/types/ptr/)

```llvm
ret ptr %self
```

\_\_bool\_\_[](https://docs.exaloop.io/libraries/api/classes/Ptr/#class-ptr-__bool__ "Copy/link to this API element")

@pure @llvm

```python
__bool__(self: Self, Self: type)
```

Added by [`internal.types.ptr`](https://docs.exaloop.io/libraries/api/modules/internal/types/ptr/)

```llvm
%0 = icmp ne ptr %self, null
%1 = zext i1 %0 to i8
ret i8 %1
```

\_\_getitem\_\_[](https://docs.exaloop.io/libraries/api/classes/Ptr/#class-ptr-__getitem__ "Copy/link to this API element")

@pure @derives @llvm

```python
__getitem__(self: Self, index: int, Self: type)
```

Added by [`internal.types.ptr`](https://docs.exaloop.io/libraries/api/modules/internal/types/ptr/)

```llvm
%0 = getelementptr {=T}, ptr %self, i64 %index
%1 = load {=T}, ptr %0
ret {=T} %1
```

```python
__getitem__(self: Ptr[Vec[S, M]], index: int, S: type, M: Literal[int])
```

Added by [`simd`](https://docs.exaloop.io/libraries/api/modules/simd/)

```llvm
%address = getelementptr <{=M} x {=S}>, ptr %self, i64 %index
%result = load <{=M} x {=S}>, ptr %address, align 1
ret <{=M} x {=S}> %result
```

\_\_setitem\_\_[](https://docs.exaloop.io/libraries/api/classes/Ptr/#class-ptr-__setitem__ "Copy/link to this API element")

@self\_captures @llvm

```python
__setitem__(self: Self, index: int, what: T, Self: type)
```

Added by [`internal.types.ptr`](https://docs.exaloop.io/libraries/api/modules/internal/types/ptr/)

```llvm
%0 = getelementptr {=T}, ptr %self, i64 %index
store {=T} %what, ptr %0
ret {} {}
```

```python
__setitem__(self: Ptr[Vec[S, M]], index: int, what: Vec[S, M], S: type, M: Literal[int])
```

Added by [`simd`](https://docs.exaloop.io/libraries/api/modules/simd/)

```llvm
%address = getelementptr <{=M} x {=S}>, ptr %self, i64 %index
store <{=M} x {=S}> %what, ptr %address, align 1
ret {} zeroinitializer
```

\_\_add\_\_[](https://docs.exaloop.io/libraries/api/classes/Ptr/#class-ptr-__add__ "Copy/link to this API element")

@pure @derives @llvm

```python
__add__(self: Self, other: int, Self: type)
```

Added by [`internal.types.ptr`](https://docs.exaloop.io/libraries/api/modules/internal/types/ptr/)

```llvm
%0 = getelementptr {=T}, ptr %self, i64 %other
ret ptr %0
```

\_\_sub\_\_[](https://docs.exaloop.io/libraries/api/classes/Ptr/#class-ptr-__sub__ "Copy/link to this API element")

@pure @llvm

```python
__sub__(self: Self, other: Ptr[T], Self: type)
```

Added by [`internal.types.ptr`](https://docs.exaloop.io/libraries/api/modules/internal/types/ptr/)

```llvm
%0 = ptrtoint ptr %self to i64
%1 = ptrtoint ptr %other to i64
%2 = sub i64 %0, %1
%3 = sdiv exact i64 %2, ptrtoint (ptr getelementptr ({=T}, {=T}* null, i32 1) to i64)
ret i64 %3
```

@derives

```python
__sub__(self: Self, other: int, Self: type)
```

Added by [`internal.types.ptr`](https://docs.exaloop.io/libraries/api/modules/internal/types/ptr/)

```llvm
%0 = sub i64 0, %other
%1 = getelementptr {=T}, {=T}* %self, i64 %0
ret {=T}* %1
```

\_\_eq\_\_[](https://docs.exaloop.io/libraries/api/classes/Ptr/#class-ptr-__eq__ "Copy/link to this API element")

@pure @llvm

```python
__eq__(self: Self, other: Ptr[T], Self: type)
```

Added by [`internal.types.ptr`](https://docs.exaloop.io/libraries/api/modules/internal/types/ptr/)

```llvm
%0 = icmp eq ptr %self, %other
%1 = zext i1 %0 to i8
ret i8 %1
```

\_\_ne\_\_[](https://docs.exaloop.io/libraries/api/classes/Ptr/#class-ptr-__ne__ "Copy/link to this API element")

@pure @llvm

```python
__ne__(self: Self, other: Ptr[T], Self: type)
```

Added by [`internal.types.ptr`](https://docs.exaloop.io/libraries/api/modules/internal/types/ptr/)

```llvm
%0 = icmp ne ptr %self, %other
%1 = zext i1 %0 to i8
ret i8 %1
```

\_\_lt\_\_[](https://docs.exaloop.io/libraries/api/classes/Ptr/#class-ptr-__lt__ "Copy/link to this API element")

@pure @llvm

```python
__lt__(self: Self, other: Ptr[T], Self: type)
```

Added by [`internal.types.ptr`](https://docs.exaloop.io/libraries/api/modules/internal/types/ptr/)

```llvm
%0 = icmp slt ptr %self, %other
%1 = zext i1 %0 to i8
ret i8 %1
```

\_\_gt\_\_[](https://docs.exaloop.io/libraries/api/classes/Ptr/#class-ptr-__gt__ "Copy/link to this API element")

@pure @llvm

```python
__gt__(self: Self, other: Ptr[T], Self: type)
```

Added by [`internal.types.ptr`](https://docs.exaloop.io/libraries/api/modules/internal/types/ptr/)

```llvm
%0 = icmp sgt ptr %self, %other
%1 = zext i1 %0 to i8
ret i8 %1
```

\_\_le\_\_[](https://docs.exaloop.io/libraries/api/classes/Ptr/#class-ptr-__le__ "Copy/link to this API element")

@pure @llvm

```python
__le__(self: Self, other: Ptr[T], Self: type)
```

Added by [`internal.types.ptr`](https://docs.exaloop.io/libraries/api/modules/internal/types/ptr/)

```llvm
%0 = icmp sle ptr %self, %other
%1 = zext i1 %0 to i8
ret i8 %1
```

\_\_ge\_\_[](https://docs.exaloop.io/libraries/api/classes/Ptr/#class-ptr-__ge__ "Copy/link to this API element")

@pure @llvm

```python
__ge__(self: Self, other: Ptr[T], Self: type)
```

Added by [`internal.types.ptr`](https://docs.exaloop.io/libraries/api/modules/internal/types/ptr/)

```llvm
%0 = icmp sge ptr %self, %other
%1 = zext i1 %0 to i8
ret i8 %1
```

\_\_prefetch\_r0\_\_[](https://docs.exaloop.io/libraries/api/classes/Ptr/#class-ptr-__prefetch_r0__ "Copy/link to this API element")

@nocapture @llvm

```python
__prefetch_r0__(self: Self, Self: type)
```

Added by [`internal.types.ptr`](https://docs.exaloop.io/libraries/api/modules/internal/types/ptr/)

```llvm
declare void @llvm.prefetch(ptr nocapture readonly, i32, i32, i32)
call void @llvm.prefetch(ptr %self, i32 0, i32 0, i32 1)
ret {} {}
```

\_\_prefetch\_r1\_\_[](https://docs.exaloop.io/libraries/api/classes/Ptr/#class-ptr-__prefetch_r1__ "Copy/link to this API element")

@nocapture @llvm

```python
__prefetch_r1__(self: Self, Self: type)
```

Added by [`internal.types.ptr`](https://docs.exaloop.io/libraries/api/modules/internal/types/ptr/)

```llvm
declare void @llvm.prefetch(ptr nocapture readonly, i32, i32, i32)
call void @llvm.prefetch(ptr %self, i32 0, i32 1, i32 1)
ret {} {}
```

\_\_prefetch\_r2\_\_[](https://docs.exaloop.io/libraries/api/classes/Ptr/#class-ptr-__prefetch_r2__ "Copy/link to this API element")

@nocapture @llvm

```python
__prefetch_r2__(self: Self, Self: type)
```

Added by [`internal.types.ptr`](https://docs.exaloop.io/libraries/api/modules/internal/types/ptr/)

```llvm
declare void @llvm.prefetch(ptr nocapture readonly, i32, i32, i32)
call void @llvm.prefetch(ptr %self, i32 0, i32 2, i32 1)
ret {} {}
```

\_\_prefetch\_r3\_\_[](https://docs.exaloop.io/libraries/api/classes/Ptr/#class-ptr-__prefetch_r3__ "Copy/link to this API element")

@nocapture @llvm

```python
__prefetch_r3__(self: Self, Self: type)
```

Added by [`internal.types.ptr`](https://docs.exaloop.io/libraries/api/modules/internal/types/ptr/)

```llvm
declare void @llvm.prefetch(ptr nocapture readonly, i32, i32, i32)
call void @llvm.prefetch(ptr %self, i32 0, i32 3, i32 1)
ret {} {}
```

\_\_prefetch\_w0\_\_[](https://docs.exaloop.io/libraries/api/classes/Ptr/#class-ptr-__prefetch_w0__ "Copy/link to this API element")

@nocapture @llvm

```python
__prefetch_w0__(self: Self, Self: type)
```

Added by [`internal.types.ptr`](https://docs.exaloop.io/libraries/api/modules/internal/types/ptr/)

```llvm
declare void @llvm.prefetch(ptr nocapture readonly, i32, i32, i32)
call void @llvm.prefetch(ptr %self, i32 1, i32 0, i32 1)
ret {} {}
```

\_\_prefetch\_w1\_\_[](https://docs.exaloop.io/libraries/api/classes/Ptr/#class-ptr-__prefetch_w1__ "Copy/link to this API element")

@nocapture @llvm

```python
__prefetch_w1__(self: Self, Self: type)
```

Added by [`internal.types.ptr`](https://docs.exaloop.io/libraries/api/modules/internal/types/ptr/)

```llvm
declare void @llvm.prefetch(ptr nocapture readonly, i32, i32, i32)
call void @llvm.prefetch(ptr %self, i32 1, i32 1, i32 1)
ret {} {}
```

\_\_prefetch\_w2\_\_[](https://docs.exaloop.io/libraries/api/classes/Ptr/#class-ptr-__prefetch_w2__ "Copy/link to this API element")

@nocapture @llvm

```python
__prefetch_w2__(self: Self, Self: type)
```

Added by [`internal.types.ptr`](https://docs.exaloop.io/libraries/api/modules/internal/types/ptr/)

```llvm
declare void @llvm.prefetch(ptr nocapture readonly, i32, i32, i32)
call void @llvm.prefetch(ptr %self, i32 1, i32 2, i32 1)
ret {} {}
```

\_\_prefetch\_w3\_\_[](https://docs.exaloop.io/libraries/api/classes/Ptr/#class-ptr-__prefetch_w3__ "Copy/link to this API element")

@nocapture @llvm

```python
__prefetch_w3__(self: Self, Self: type)
```

Added by [`internal.types.ptr`](https://docs.exaloop.io/libraries/api/modules/internal/types/ptr/)

```llvm
declare void @llvm.prefetch(ptr nocapture readonly, i32, i32, i32)
call void @llvm.prefetch(ptr %self, i32 1, i32 3, i32 1)
ret {} {}
```

\_\_repr\_\_[](https://docs.exaloop.io/libraries/api/classes/Ptr/#class-ptr-__repr__ "Copy/link to this API element")

```python
__repr__(self: Self, Self: type)
```

Added by [`internal.types.ptr`](https://docs.exaloop.io/libraries/api/modules/internal/types/ptr/)

\_\_format\_\_[](https://docs.exaloop.io/libraries/api/classes/Ptr/#class-ptr-__format__ "Copy/link to this API element")

```python
__format__(self: Self, format_spec: str, Self: type)
```

Added by [`internal.format`](https://docs.exaloop.io/libraries/api/modules/internal/format/)

\_\_to\_gpu\_\_[](https://docs.exaloop.io/libraries/api/classes/Ptr/#class-ptr-__to_gpu__ "Copy/link to this API element")

```python
__to_gpu__(self: Self, cache: AllocCache, Self: type)
```

Added by [`internal.gpu`](https://docs.exaloop.io/libraries/api/modules/internal/gpu/)

\_\_from\_gpu\_\_[](https://docs.exaloop.io/libraries/api/classes/Ptr/#class-ptr-__from_gpu__ "Copy/link to this API element")

```python
__from_gpu__(self: Self, other: Ptr[T], Self: type)
```

Added by [`internal.gpu`](https://docs.exaloop.io/libraries/api/modules/internal/gpu/)

\_\_from\_gpu\_new\_\_[](https://docs.exaloop.io/libraries/api/classes/Ptr/#class-ptr-__from_gpu_new__ "Copy/link to this API element")

```python
__from_gpu_new__(other: Ptr[T])
```

Added by [`internal.gpu`](https://docs.exaloop.io/libraries/api/modules/internal/gpu/)

Source: https://docs.exaloop.io/libraries/api/classes/Ptr/
