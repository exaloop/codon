# internal.gpu

Standard library module [View source](https://github.com/exaloop/codon/blob/master/stdlib/internal/gpu.codon)

variable CUDA\_SUCCESS[](https://docs.exaloop.io/libraries/api/modules/internal/gpu/#variable-cuda_success "Copy/link to this API element")

```python
CUDA_SUCCESS: Literal[int] = 0
```

variable CUDA\_ERROR\_NOT\_FOUND[](https://docs.exaloop.io/libraries/api/modules/internal/gpu/#variable-cuda_error_not_found "Copy/link to this API element")

```python
CUDA_ERROR_NOT_FOUND: Literal[int] = 500
```

variable CUresult[](https://docs.exaloop.io/libraries/api/modules/internal/gpu/#variable-curesult "Copy/link to this API element")

```python
CUresult = i32
```

variable CUdevice[](https://docs.exaloop.io/libraries/api/modules/internal/gpu/#variable-cudevice "Copy/link to this API element")

```python
CUdevice = i32
```

variable CUdeviceptr[](https://docs.exaloop.io/libraries/api/modules/internal/gpu/#variable-cudeviceptr "Copy/link to this API element")

```python
CUdeviceptr = u64
```

variable CUmodule[](https://docs.exaloop.io/libraries/api/modules/internal/gpu/#variable-cumodule "Copy/link to this API element")

```python
CUmodule = cobj
```

variable CUcontext[](https://docs.exaloop.io/libraries/api/modules/internal/gpu/#variable-cucontext "Copy/link to this API element")

```python
CUcontext = cobj
```

variable CUfunction[](https://docs.exaloop.io/libraries/api/modules/internal/gpu/#variable-cufunction "Copy/link to this API element")

```python
CUfunction = cobj
```

variable modules[](https://docs.exaloop.io/libraries/api/modules/internal/gpu/#variable-modules "Copy/link to this API element")

```python
modules: List[CUmodule] = []
```

variable cuCtxCreate[](https://docs.exaloop.io/libraries/api/modules/internal/gpu/#variable-cuctxcreate "Copy/link to this API element")

```python
cuCtxCreate = Function[([Ptr[cobj], u32, CUdevice], CUresult)](cobj())
```

variable cuMemAlloc[](https://docs.exaloop.io/libraries/api/modules/internal/gpu/#variable-cumemalloc "Copy/link to this API element")

```python
cuMemAlloc = Function[([Ptr[CUdeviceptr], i64], CUresult)](cobj())
```

variable cuMemcpyDtoH[](https://docs.exaloop.io/libraries/api/modules/internal/gpu/#variable-cumemcpydtoh "Copy/link to this API element")

```python
cuMemcpyDtoH = Function[([cobj, CUdeviceptr, u64], CUresult)](cobj())
```

variable cuMemcpyHtoD[](https://docs.exaloop.io/libraries/api/modules/internal/gpu/#variable-cumemcpyhtod "Copy/link to this API element")

```python
cuMemcpyHtoD = Function[([CUdeviceptr, cobj, u64], CUresult)](cobj())
```

variable cuMemFree[](https://docs.exaloop.io/libraries/api/modules/internal/gpu/#variable-cumemfree "Copy/link to this API element")

```python
cuMemFree = Function[([CUdeviceptr], CUresult)](cobj())
```

variable cuDeviceComputeCapability[](https://docs.exaloop.io/libraries/api/modules/internal/gpu/#variable-cudevicecomputecapability "Copy/link to this API element")

```python
cuDeviceComputeCapability = Function[([Ptr[i32], Ptr[i32], CUdevice], CUresult)](cobj())
```

variable cuDeviceGet[](https://docs.exaloop.io/libraries/api/modules/internal/gpu/#variable-cudeviceget "Copy/link to this API element")

```python
cuDeviceGet = Function[([Ptr[CUdevice], i32], CUresult)](cobj())
```

variable cuDeviceGetCount[](https://docs.exaloop.io/libraries/api/modules/internal/gpu/#variable-cudevicegetcount "Copy/link to this API element")

```python
cuDeviceGetCount = Function[([Ptr[i32]], CUresult)](cobj())
```

variable cuDeviceGetName[](https://docs.exaloop.io/libraries/api/modules/internal/gpu/#variable-cudevicegetname "Copy/link to this API element")

```python
cuDeviceGetName = Function[([Ptr[u8], i32, CUdevice], CUresult)](cobj())
```

variable cuGetErrorName[](https://docs.exaloop.io/libraries/api/modules/internal/gpu/#variable-cugeterrorname "Copy/link to this API element")

```python
cuGetErrorName = Function[([CUresult, Ptr[cobj]], CUresult)](cobj())
```

variable cuGetErrorString[](https://docs.exaloop.io/libraries/api/modules/internal/gpu/#variable-cugeterrorstring "Copy/link to this API element")

```python
cuGetErrorString = Function[([CUresult, Ptr[cobj]], CUresult)](cobj())
```

variable cuInit[](https://docs.exaloop.io/libraries/api/modules/internal/gpu/#variable-cuinit "Copy/link to this API element")

```python
cuInit = Function[([u32], CUresult)](cobj())
```

variable cuLaunchKernel[](https://docs.exaloop.io/libraries/api/modules/internal/gpu/#variable-culaunchkernel "Copy/link to this API element")

```python
cuLaunchKernel = Function[([CUfunction, u32, u32, u32, u32, u32, u32, u32, cobj, cobj, cobj], CUresult)](cobj())
```

variable cuModuleGetFunction[](https://docs.exaloop.io/libraries/api/modules/internal/gpu/#variable-cumodulegetfunction "Copy/link to this API element")

```python
cuModuleGetFunction = Function[([Ptr[CUfunction], CUmodule, cobj], CUresult)](cobj())
```

variable cuModuleLoadData[](https://docs.exaloop.io/libraries/api/modules/internal/gpu/#variable-cumoduleloaddata "Copy/link to this API element")

```python
cuModuleLoadData = Function[([Ptr[CUmodule], cobj], CUresult)](cobj())
```

class CUDAError[](https://docs.exaloop.io/libraries/api/modules/internal/gpu/#class-cudaerror "Copy/link to this API element")

```python
class CUDAError
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/internal/gpu/CUDAError/)

Fields 1

`result` `int`

Constructors 1

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/gpu/#class-cudaerror-__init__ "Copy/link to this API element")

```python
__init__(self: Self, result: int, message: str = "", Self: type)
```

class Device[](https://docs.exaloop.io/libraries/api/modules/internal/gpu/#class-device "Copy/link to this API element")

@tuple

```python
class Device
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/internal/gpu/Device/)

Constructors 1

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/gpu/#class-device-__new__ "Copy/link to this API element")

```python
__new__(idx: int)
```

Properties 1

compute\_capability[](https://docs.exaloop.io/libraries/api/modules/internal/gpu/#class-device-compute_capability "Copy/link to this API element")

@property

```python
compute_capability(self: Self, Self: type)
```

Methods 1

count[](https://docs.exaloop.io/libraries/api/modules/internal/gpu/#class-device-count "Copy/link to this API element")

@staticmethod

```python
count()
```

Special methods 3

\_\_str\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/gpu/#class-device-__str__ "Copy/link to this API element")

```python
__str__(self: Self, Self: type)
```

\_\_index\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/gpu/#class-device-__index__ "Copy/link to this API element")

```python
__index__(self: Self, Self: type)
```

\_\_bool\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/gpu/#class-device-__bool__ "Copy/link to this API element")

```python
__bool__(self: Self, Self: type)
```

function cuda\_init\_handles[](https://docs.exaloop.io/libraries/api/modules/internal/gpu/#function-cuda_init_handles "Copy/link to this API element")

```python
cuda_init_handles(cuda_handle: cobj)
```

function cuda\_check[](https://docs.exaloop.io/libraries/api/modules/internal/gpu/#function-cuda_check "Copy/link to this API element")

```python
cuda_check(err)
```

function nvptx\_init[](https://docs.exaloop.io/libraries/api/modules/internal/gpu/#function-nvptx_init "Copy/link to this API element")

```python
nvptx_init()
```

function nvptx\_load\_module[](https://docs.exaloop.io/libraries/api/modules/internal/gpu/#function-nvptx_load_module "Copy/link to this API element")

```python
nvptx_load_module()
```

function cuda\_init[](https://docs.exaloop.io/libraries/api/modules/internal/gpu/#function-cuda_init "Copy/link to this API element")

```python
cuda_init(debug: bool = False)
```

class Memory[](https://docs.exaloop.io/libraries/api/modules/internal/gpu/#class-memory "Copy/link to this API element")

@tuple

```python
class Memory[T]
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/internal/gpu/Memory/)

Fields 1

`T` `type`

Constructors 1

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/gpu/#class-memory-__new__ "Copy/link to this API element")

```python
__new__(ptr: cobj)
```

Properties 1

ptr[](https://docs.exaloop.io/libraries/api/modules/internal/gpu/#class-memory-ptr "Copy/link to this API element")

@property

```python
ptr(self: Self, Self: type)
```

Methods 1

to\_ptr[](https://docs.exaloop.io/libraries/api/modules/internal/gpu/#class-memory-to_ptr "Copy/link to this API element")

@pure @llvm

```python
to_ptr(p: CUdeviceptr)
```

```llvm
%0 = inttoptr i64 %p to ptr
ret ptr %0
```

function syncthreads[](https://docs.exaloop.io/libraries/api/modules/internal/gpu/#function-syncthreads "Copy/link to this API element")

@llvm

```python
syncthreads()
```

```llvm
declare void @llvm.nvvm.barrier0()
call void @llvm.nvvm.barrier0()
ret {} {}
```

class Dim3[](https://docs.exaloop.io/libraries/api/modules/internal/gpu/#class-dim3 "Copy/link to this API element")

@tuple

```python
class Dim3
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/internal/gpu/Dim3/)

Constructors 1

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/gpu/#class-dim3-__new__ "Copy/link to this API element")

```python
__new__(x: int, y: int, z: int)
```

Properties 3

x[](https://docs.exaloop.io/libraries/api/modules/internal/gpu/#class-dim3-x "Copy/link to this API element")

@property

```python
x(self: Self, Self: type)
```

y[](https://docs.exaloop.io/libraries/api/modules/internal/gpu/#class-dim3-y "Copy/link to this API element")

@property

```python
y(self: Self, Self: type)
```

z[](https://docs.exaloop.io/libraries/api/modules/internal/gpu/#class-dim3-z "Copy/link to this API element")

@property

```python
z(self: Self, Self: type)
```

class Thread[](https://docs.exaloop.io/libraries/api/modules/internal/gpu/#class-thread "Copy/link to this API element")

@tuple

```python
class Thread
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/internal/gpu/Thread/)

Properties 3

x[](https://docs.exaloop.io/libraries/api/modules/internal/gpu/#class-thread-x "Copy/link to this API element")

@property

```python
x(self: Self, Self: type)
```

y[](https://docs.exaloop.io/libraries/api/modules/internal/gpu/#class-thread-y "Copy/link to this API element")

@property

```python
y(self: Self, Self: type)
```

z[](https://docs.exaloop.io/libraries/api/modules/internal/gpu/#class-thread-z "Copy/link to this API element")

@property

```python
z(self: Self, Self: type)
```

class Block[](https://docs.exaloop.io/libraries/api/modules/internal/gpu/#class-block "Copy/link to this API element")

@tuple

```python
class Block
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/internal/gpu/Block/)

Properties 4

x[](https://docs.exaloop.io/libraries/api/modules/internal/gpu/#class-block-x "Copy/link to this API element")

@property

```python
x(self: Self, Self: type)
```

y[](https://docs.exaloop.io/libraries/api/modules/internal/gpu/#class-block-y "Copy/link to this API element")

@property

```python
y(self: Self, Self: type)
```

z[](https://docs.exaloop.io/libraries/api/modules/internal/gpu/#class-block-z "Copy/link to this API element")

@property

```python
z(self: Self, Self: type)
```

dim[](https://docs.exaloop.io/libraries/api/modules/internal/gpu/#class-block-dim "Copy/link to this API element")

@property

```python
dim(self: Self, Self: type)
```

class Grid[](https://docs.exaloop.io/libraries/api/modules/internal/gpu/#class-grid "Copy/link to this API element")

@tuple

```python
class Grid
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/internal/gpu/Grid/)

Properties 1

dim[](https://docs.exaloop.io/libraries/api/modules/internal/gpu/#class-grid-dim "Copy/link to this API element")

@property

```python
dim(self: Self, Self: type)
```

class Warp[](https://docs.exaloop.io/libraries/api/modules/internal/gpu/#class-warp "Copy/link to this API element")

@tuple

```python
class Warp
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/internal/gpu/Warp/)

Special methods 1

\_\_len\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/gpu/#class-warp-__len__ "Copy/link to this API element")

```python
__len__(self: Self, Self: type)
```

variable thread[](https://docs.exaloop.io/libraries/api/modules/internal/gpu/#variable-thread "Copy/link to this API element")

```python
thread = Thread()
```

variable block[](https://docs.exaloop.io/libraries/api/modules/internal/gpu/#variable-block "Copy/link to this API element")

```python
block = Block()
```

variable grid[](https://docs.exaloop.io/libraries/api/modules/internal/gpu/#variable-grid "Copy/link to this API element")

```python
grid = Grid()
```

variable warp[](https://docs.exaloop.io/libraries/api/modules/internal/gpu/#variable-warp "Copy/link to this API element")

```python
warp = Warp()
```

class AllocCache[](https://docs.exaloop.io/libraries/api/modules/internal/gpu/#class-alloccache "Copy/link to this API element")

@tuple

```python
class AllocCache
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/internal/gpu/AllocCache/)

Fields 1

`v` `List[Ptr[u8]]`

Methods 2

add[](https://docs.exaloop.io/libraries/api/modules/internal/gpu/#class-alloccache-add "Copy/link to this API element")

```python
add(self: Self, p: Ptr[u8], Self: type)
```

free[](https://docs.exaloop.io/libraries/api/modules/internal/gpu/#class-alloccache-free "Copy/link to this API element")

```python
free(self: Self, Self: type)
```

function nvptx\_function[](https://docs.exaloop.io/libraries/api/modules/internal/gpu/#function-nvptx_function "Copy/link to this API element")

```python
nvptx_function(name: Literal[str])
```

function canonical\_dim[](https://docs.exaloop.io/libraries/api/modules/internal/gpu/#function-canonical_dim "Copy/link to this API element")

```python
canonical_dim(dim)
```

function offsets[](https://docs.exaloop.io/libraries/api/modules/internal/gpu/#function-offsets "Copy/link to this API element")

```python
offsets(t)
```

function kernel\_wrapper[](https://docs.exaloop.io/libraries/api/modules/internal/gpu/#function-kernel_wrapper "Copy/link to this API element")

```python
kernel_wrapper(*args, grid, block, fn)
```

function kernel[](https://docs.exaloop.io/libraries/api/modules/internal/gpu/#function-kernel "Copy/link to this API element")

```python
kernel(fn)
```

class Pointer[](https://docs.exaloop.io/libraries/api/modules/internal/gpu/#class-pointer "Copy/link to this API element")

@tuple

```python
class Pointer[T]
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/internal/gpu/Pointer/)

Fields 1

`T` `type`

Special methods 3

\_\_to\_gpu\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/gpu/#class-pointer-__to_gpu__ "Copy/link to this API element")

```python
__to_gpu__(self: Self, cache: AllocCache, Self: type)
```

\_\_from\_gpu\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/gpu/#class-pointer-__from_gpu__ "Copy/link to this API element")

```python
__from_gpu__(self: Self, other: Ptr[T], Self: type)
```

\_\_from\_gpu\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/gpu/#class-pointer-__from_gpu_new__ "Copy/link to this API element")

```python
__from_gpu_new__(other: Ptr[T])
```

function raw[](https://docs.exaloop.io/libraries/api/modules/internal/gpu/#function-raw "Copy/link to this API element")

```python
raw(v)
```

class Ptr[](https://docs.exaloop.io/libraries/api/modules/internal/gpu/#class-ptr "Copy/link to this API element")

@extend

```python
class Ptr[T]
```

Extends [`Ptr`](https://docs.exaloop.io/libraries/api/classes/Ptr/) · [View complete class API →](https://docs.exaloop.io/libraries/api/classes/Ptr/)

Special methods 3

\_\_to\_gpu\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/gpu/#class-ptr-__to_gpu__ "Copy/link to this API element")

```python
__to_gpu__(self: Self, cache: AllocCache, Self: type)
```

\_\_from\_gpu\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/gpu/#class-ptr-__from_gpu__ "Copy/link to this API element")

```python
__from_gpu__(self: Self, other: Ptr[T], Self: type)
```

\_\_from\_gpu\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/gpu/#class-ptr-__from_gpu_new__ "Copy/link to this API element")

```python
__from_gpu_new__(other: Ptr[T])
```

class NoneType[](https://docs.exaloop.io/libraries/api/modules/internal/gpu/#class-nonetype "Copy/link to this API element")

@extend

```python
class NoneType
```

Extends [`NoneType`](https://docs.exaloop.io/libraries/api/classes/NoneType/) · [View complete class API →](https://docs.exaloop.io/libraries/api/classes/NoneType/)

Special methods 3

\_\_to\_gpu\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/gpu/#class-nonetype-__to_gpu__ "Copy/link to this API element")

```python
__to_gpu__(self: Self, cache: AllocCache, Self: type)
```

\_\_from\_gpu\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/gpu/#class-nonetype-__from_gpu__ "Copy/link to this API element")

```python
__from_gpu__(self: Self, other: NoneType, Self: type)
```

\_\_from\_gpu\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/gpu/#class-nonetype-__from_gpu_new__ "Copy/link to this API element")

```python
__from_gpu_new__(other: NoneType)
```

class float[](https://docs.exaloop.io/libraries/api/modules/internal/gpu/#class-float "Copy/link to this API element")

@extend

```python
class float
```

Extends [`float`](https://docs.exaloop.io/libraries/api/classes/float/) · [View complete class API →](https://docs.exaloop.io/libraries/api/classes/float/)

Special methods 3

\_\_to\_gpu\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/gpu/#class-float-__to_gpu__ "Copy/link to this API element")

```python
__to_gpu__(self: Self, cache: AllocCache, Self: type)
```

\_\_from\_gpu\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/gpu/#class-float-__from_gpu__ "Copy/link to this API element")

```python
__from_gpu__(self: Self, other: float, Self: type)
```

\_\_from\_gpu\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/gpu/#class-float-__from_gpu_new__ "Copy/link to this API element")

```python
__from_gpu_new__(other: float)
```

class float32[](https://docs.exaloop.io/libraries/api/modules/internal/gpu/#class-float32 "Copy/link to this API element")

@extend

```python
class float32
```

Extends [`float32`](https://docs.exaloop.io/libraries/api/classes/float32/) · [View complete class API →](https://docs.exaloop.io/libraries/api/classes/float32/)

Special methods 3

\_\_to\_gpu\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/gpu/#class-float32-__to_gpu__ "Copy/link to this API element")

```python
__to_gpu__(self: Self, cache: AllocCache, Self: type)
```

\_\_from\_gpu\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/gpu/#class-float32-__from_gpu__ "Copy/link to this API element")

```python
__from_gpu__(self: Self, other: float32, Self: type)
```

\_\_from\_gpu\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/gpu/#class-float32-__from_gpu_new__ "Copy/link to this API element")

```python
__from_gpu_new__(other: float32)
```

class float16[](https://docs.exaloop.io/libraries/api/modules/internal/gpu/#class-float16 "Copy/link to this API element")

@extend

```python
class float16
```

Extends [`float16`](https://docs.exaloop.io/libraries/api/classes/float16/) · [View complete class API →](https://docs.exaloop.io/libraries/api/classes/float16/)

Special methods 3

\_\_to\_gpu\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/gpu/#class-float16-__to_gpu__ "Copy/link to this API element")

```python
__to_gpu__(self: Self, cache: AllocCache, Self: type)
```

\_\_from\_gpu\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/gpu/#class-float16-__from_gpu__ "Copy/link to this API element")

```python
__from_gpu__(self: Self, other: float16, Self: type)
```

\_\_from\_gpu\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/gpu/#class-float16-__from_gpu_new__ "Copy/link to this API element")

```python
__from_gpu_new__(other: float16)
```

class bfloat16[](https://docs.exaloop.io/libraries/api/modules/internal/gpu/#class-bfloat16 "Copy/link to this API element")

@extend

```python
class bfloat16
```

Extends [`bfloat16`](https://docs.exaloop.io/libraries/api/classes/bfloat16/) · [View complete class API →](https://docs.exaloop.io/libraries/api/classes/bfloat16/)

Special methods 3

\_\_to\_gpu\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/gpu/#class-bfloat16-__to_gpu__ "Copy/link to this API element")

```python
__to_gpu__(self: Self, cache: AllocCache, Self: type)
```

\_\_from\_gpu\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/gpu/#class-bfloat16-__from_gpu__ "Copy/link to this API element")

```python
__from_gpu__(self: Self, other: bfloat16, Self: type)
```

\_\_from\_gpu\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/gpu/#class-bfloat16-__from_gpu_new__ "Copy/link to this API element")

```python
__from_gpu_new__(other: bfloat16)
```

class bool[](https://docs.exaloop.io/libraries/api/modules/internal/gpu/#class-bool "Copy/link to this API element")

@extend

```python
class bool
```

Extends [`bool`](https://docs.exaloop.io/libraries/api/classes/bool/) · [View complete class API →](https://docs.exaloop.io/libraries/api/classes/bool/)

Special methods 3

\_\_to\_gpu\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/gpu/#class-bool-__to_gpu__ "Copy/link to this API element")

```python
__to_gpu__(self: Self, cache: AllocCache, Self: type)
```

\_\_from\_gpu\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/gpu/#class-bool-__from_gpu__ "Copy/link to this API element")

```python
__from_gpu__(self: Self, other: bool, Self: type)
```

\_\_from\_gpu\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/gpu/#class-bool-__from_gpu_new__ "Copy/link to this API element")

```python
__from_gpu_new__(other: bool)
```

class Int[](https://docs.exaloop.io/libraries/api/modules/internal/gpu/#class-int "Copy/link to this API element")

@extend

```python
class Int[N]
```

Extends [`Int`](https://docs.exaloop.io/libraries/api/classes/Int/) · [View complete class API →](https://docs.exaloop.io/libraries/api/classes/Int/)

Special methods 3

\_\_to\_gpu\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/gpu/#class-int-__to_gpu__ "Copy/link to this API element")

```python
__to_gpu__(self: Self, cache: AllocCache, Self: type)
```

\_\_from\_gpu\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/gpu/#class-int-__from_gpu__ "Copy/link to this API element")

```python
__from_gpu__(self: Self, other: Int[N], Self: type)
```

\_\_from\_gpu\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/gpu/#class-int-__from_gpu_new__ "Copy/link to this API element")

```python
__from_gpu_new__(other: Int[N])
```

class UInt[](https://docs.exaloop.io/libraries/api/modules/internal/gpu/#class-uint "Copy/link to this API element")

@extend

```python
class UInt[N]
```

Extends [`UInt`](https://docs.exaloop.io/libraries/api/classes/UInt/) · [View complete class API →](https://docs.exaloop.io/libraries/api/classes/UInt/)

Special methods 3

\_\_to\_gpu\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/gpu/#class-uint-__to_gpu__ "Copy/link to this API element")

```python
__to_gpu__(self: Self, cache: AllocCache, Self: type)
```

\_\_from\_gpu\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/gpu/#class-uint-__from_gpu__ "Copy/link to this API element")

```python
__from_gpu__(self: Self, other: UInt[N], Self: type)
```

\_\_from\_gpu\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/gpu/#class-uint-__from_gpu_new__ "Copy/link to this API element")

```python
__from_gpu_new__(other: UInt[N])
```

class str[](https://docs.exaloop.io/libraries/api/modules/internal/gpu/#class-str "Copy/link to this API element")

@extend

```python
class str
```

Extends [`str`](https://docs.exaloop.io/libraries/api/classes/str/) · [View complete class API →](https://docs.exaloop.io/libraries/api/classes/str/)

Special methods 3

\_\_to\_gpu\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/gpu/#class-str-__to_gpu__ "Copy/link to this API element")

```python
__to_gpu__(self: Self, cache: AllocCache, Self: type)
```

\_\_from\_gpu\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/gpu/#class-str-__from_gpu__ "Copy/link to this API element")

```python
__from_gpu__(self: Self, other: str, Self: type)
```

\_\_from\_gpu\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/gpu/#class-str-__from_gpu_new__ "Copy/link to this API element")

```python
__from_gpu_new__(other: str)
```

class bytes[](https://docs.exaloop.io/libraries/api/modules/internal/gpu/#class-bytes "Copy/link to this API element")

@extend

```python
class bytes
```

Extends [`bytes`](https://docs.exaloop.io/libraries/api/classes/bytes/) · [View complete class API →](https://docs.exaloop.io/libraries/api/classes/bytes/)

Special methods 3

\_\_to\_gpu\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/gpu/#class-bytes-__to_gpu__ "Copy/link to this API element")

```python
__to_gpu__(self: Self, cache: AllocCache, Self: type)
```

\_\_from\_gpu\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/gpu/#class-bytes-__from_gpu__ "Copy/link to this API element")

```python
__from_gpu__(self: Self, other: bytes, Self: type)
```

\_\_from\_gpu\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/gpu/#class-bytes-__from_gpu_new__ "Copy/link to this API element")

```python
__from_gpu_new__(other: bytes)
```

class List[](https://docs.exaloop.io/libraries/api/modules/internal/gpu/#class-list "Copy/link to this API element")

@extend

```python
class List[T]
```

Extends [`List`](https://docs.exaloop.io/libraries/api/classes/List/) · [View complete class API →](https://docs.exaloop.io/libraries/api/classes/List/)

Special methods 3

\_\_to\_gpu\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/gpu/#class-list-__to_gpu__ "Copy/link to this API element")

@inline

```python
__to_gpu__(self: Self, cache: AllocCache, Self: type)
```

\_\_from\_gpu\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/gpu/#class-list-__from_gpu__ "Copy/link to this API element")

@inline

```python
__from_gpu__(self: Self, other: List[T], Self: type)
```

\_\_from\_gpu\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/gpu/#class-list-__from_gpu_new__ "Copy/link to this API element")

@inline

```python
__from_gpu_new__(other: List[T])
```

class DynamicTuple[](https://docs.exaloop.io/libraries/api/modules/internal/gpu/#class-dynamictuple "Copy/link to this API element")

@extend

```python
class DynamicTuple[T]
```

Extends [`DynamicTuple`](https://docs.exaloop.io/libraries/api/classes/DynamicTuple/) · [View complete class API →](https://docs.exaloop.io/libraries/api/classes/DynamicTuple/)

Special methods 3

\_\_to\_gpu\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/gpu/#class-dynamictuple-__to_gpu__ "Copy/link to this API element")

@inline

```python
__to_gpu__(self: Self, cache: AllocCache, Self: type)
```

\_\_from\_gpu\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/gpu/#class-dynamictuple-__from_gpu__ "Copy/link to this API element")

@inline

```python
__from_gpu__(self: Self, other: DynamicTuple[T], Self: type)
```

\_\_from\_gpu\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/gpu/#class-dynamictuple-__from_gpu_new__ "Copy/link to this API element")

@inline

```python
__from_gpu_new__(other: DynamicTuple[T])
```

class OrderedDict[](https://docs.exaloop.io/libraries/api/modules/internal/gpu/#class-ordereddict "Copy/link to this API element")

@extend

```python
class OrderedDict[K, V]
```

Extends [`OrderedDict`](https://docs.exaloop.io/libraries/api/classes/OrderedDict/) · [View complete class API →](https://docs.exaloop.io/libraries/api/classes/OrderedDict/)

Special methods 3

\_\_to\_gpu\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/gpu/#class-ordereddict-__to_gpu__ "Copy/link to this API element")

```python
__to_gpu__(self: Self, cache: AllocCache, Self: type)
```

\_\_from\_gpu\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/gpu/#class-ordereddict-__from_gpu__ "Copy/link to this API element")

```python
__from_gpu__(self: Self, other: OrderedDict[K, V], Self: type)
```

\_\_from\_gpu\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/gpu/#class-ordereddict-__from_gpu_new__ "Copy/link to this API element")

```python
__from_gpu_new__(other: OrderedDict[K, V])
```

class Set[](https://docs.exaloop.io/libraries/api/modules/internal/gpu/#class-set "Copy/link to this API element")

@extend

```python
class Set[K]
```

Extends [`Set`](https://docs.exaloop.io/libraries/api/classes/Set/) · [View complete class API →](https://docs.exaloop.io/libraries/api/classes/Set/)

Special methods 3

\_\_to\_gpu\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/gpu/#class-set-__to_gpu__ "Copy/link to this API element")

```python
__to_gpu__(self: Self, cache: AllocCache, Self: type)
```

\_\_from\_gpu\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/gpu/#class-set-__from_gpu__ "Copy/link to this API element")

```python
__from_gpu__(self: Self, other: Set[K], Self: type)
```

\_\_from\_gpu\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/gpu/#class-set-__from_gpu_new__ "Copy/link to this API element")

```python
__from_gpu_new__(other: Set[K])
```

class Optional[](https://docs.exaloop.io/libraries/api/modules/internal/gpu/#class-optional "Copy/link to this API element")

@extend

```python
class Optional[T]
```

Extends [`Optional`](https://docs.exaloop.io/libraries/api/classes/Optional/) · [View complete class API →](https://docs.exaloop.io/libraries/api/classes/Optional/)

Special methods 3

\_\_to\_gpu\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/gpu/#class-optional-__to_gpu__ "Copy/link to this API element")

```python
__to_gpu__(self: Self, cache: AllocCache, Self: type)
```

\_\_from\_gpu\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/gpu/#class-optional-__from_gpu__ "Copy/link to this API element")

```python
__from_gpu__(self: Self, other: Optional[T], Self: type)
```

\_\_from\_gpu\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/gpu/#class-optional-__from_gpu_new__ "Copy/link to this API element")

```python
__from_gpu_new__(other: Optional[T])
```

class type[](https://docs.exaloop.io/libraries/api/modules/internal/gpu/#class-type "Copy/link to this API element")

@extend

```python
class type[T]
```

Extends [`internal.core.type`](https://docs.exaloop.io/libraries/api/classes/internal/core/type/) · [View complete class API →](https://docs.exaloop.io/libraries/api/classes/internal/core/type/)

Source: https://docs.exaloop.io/libraries/api/modules/internal/gpu/
