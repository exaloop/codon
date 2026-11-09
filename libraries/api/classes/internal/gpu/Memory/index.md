# Memory

Standard library class `internal.gpu.Memory` [View source](https://github.com/exaloop/codon/blob/master/stdlib/internal/gpu.codon)

class Memory[](https://docs.exaloop.io/libraries/api/classes/internal/gpu/Memory/#class-memory "Copy/link to this API element")

@tuple

```python
class Memory[T]
```

Fields 1

`T` `type`

Constructors 1

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/gpu/Memory/#class-memory-__new__ "Copy/link to this API element")

```python
__new__(ptr: cobj)
```

Defined in [`internal.gpu`](https://docs.exaloop.io/libraries/api/modules/internal/gpu/)

Properties 1

ptr[](https://docs.exaloop.io/libraries/api/classes/internal/gpu/Memory/#class-memory-ptr "Copy/link to this API element")

@property

```python
ptr(self: Self, Self: type)
```

Defined in [`internal.gpu`](https://docs.exaloop.io/libraries/api/modules/internal/gpu/)

Methods 1

to\_ptr[](https://docs.exaloop.io/libraries/api/classes/internal/gpu/Memory/#class-memory-to_ptr "Copy/link to this API element")

@pure @llvm

```python
to_ptr(p: CUdeviceptr)
```

Defined in [`internal.gpu`](https://docs.exaloop.io/libraries/api/modules/internal/gpu/)

```llvm
%0 = inttoptr i64 %p to ptr
ret ptr %0
```

Source: https://docs.exaloop.io/libraries/api/classes/internal/gpu/Memory/
