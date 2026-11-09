# internal.types.int

Standard library module [View source](https://github.com/exaloop/codon/blob/master/stdlib/internal/types/int.codon)

class Int[](https://docs.exaloop.io/libraries/api/modules/internal/types/int/#class-int "Copy/link to this API element")

@extend

```python
class Int[N]
```

Extends [`Int`](https://docs.exaloop.io/libraries/api/classes/Int/) · [View complete class API →](https://docs.exaloop.io/libraries/api/classes/Int/)

Constructors 5

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/int/#class-int-__new__ "Copy/link to this API element")

```python
__new__()
```

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/int/#class-int-__new__-overload-2 "Copy/link to this API element")

@overload

```python
__new__(what: Int[M], M: Literal[int])
```

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/int/#class-int-__new__-overload-3 "Copy/link to this API element")

@overload

```python
__new__(what: UInt[M], M: Literal[int])
```

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/int/#class-int-__new__-overload-4 "Copy/link to this API element")

@overload

```python
__new__(what: str, base: int)
```

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/int/#class-int-__new__-overload-5 "Copy/link to this API element")

@overload

```python
__new__(what)
```

Properties 2

real[](https://docs.exaloop.io/libraries/api/modules/internal/types/int/#class-int-real "Copy/link to this API element")

@property

```python
real(self: Self, Self: type)
```

imag[](https://docs.exaloop.io/libraries/api/modules/internal/types/int/#class-int-imag "Copy/link to this API element")

@property

```python
imag(self: Self, Self: type)
```

Methods 4

cdiv[](https://docs.exaloop.io/libraries/api/modules/internal/types/int/#class-int-cdiv "Copy/link to this API element")

```python
cdiv(self: Self, other: Int[N], Self: type)
```

cmod[](https://docs.exaloop.io/libraries/api/modules/internal/types/int/#class-int-cmod "Copy/link to this API element")

```python
cmod(self: Self, other: Int[N], Self: type)
```

popcnt[](https://docs.exaloop.io/libraries/api/modules/internal/types/int/#class-int-popcnt "Copy/link to this API element")

```python
popcnt(self: Self, Self: type)
```

bit\_length[](https://docs.exaloop.io/libraries/api/modules/internal/types/int/#class-int-bit_length "Copy/link to this API element")

```python
bit_length(self: Self, Self: type)
```

Special methods 42

\_\_int\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/int/#class-int-__int__ "Copy/link to this API element")

```python
__int__(self: Self, Self: type)
```

\_\_index\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/int/#class-int-__index__ "Copy/link to this API element")

```python
__index__(self: Self, Self: type)
```

\_\_copy\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/int/#class-int-__copy__ "Copy/link to this API element")

```python
__copy__(self: Self, Self: type)
```

\_\_deepcopy\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/int/#class-int-__deepcopy__ "Copy/link to this API element")

```python
__deepcopy__(self: Self, Self: type)
```

\_\_hash\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/int/#class-int-__hash__ "Copy/link to this API element")

```python
__hash__(self: Self, Self: type)
```

\_\_float\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/int/#class-int-__float__ "Copy/link to this API element")

@pure @llvm

```python
__float__(self: Self, Self: type)
```

```llvm
%0 = sitofp i{=N} %self to double
ret double %0
```

\_\_bool\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/int/#class-int-__bool__ "Copy/link to this API element")

@pure @llvm

```python
__bool__(self: Self, Self: type)
```

```llvm
%0 = icmp ne i{=N} %self, 0
%1 = zext i1 %0 to i8
ret i8 %1
```

\_\_pos\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/int/#class-int-__pos__ "Copy/link to this API element")

```python
__pos__(self: Self, Self: type)
```

\_\_neg\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/int/#class-int-__neg__ "Copy/link to this API element")

@pure @llvm

```python
__neg__(self: Self, Self: type)
```

```llvm
%0 = sub i{=N} 0, %self
ret i{=N} %0
```

\_\_invert\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/int/#class-int-__invert__ "Copy/link to this API element")

@pure @llvm

```python
__invert__(self: Self, Self: type)
```

```llvm
%0 = xor i{=N} %self, -1
ret i{=N} %0
```

\_\_abs\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/int/#class-int-__abs__ "Copy/link to this API element")

@pure @llvm

```python
__abs__(self: Self, Self: type)
```

```llvm
declare i{=N} @llvm.abs.i{=N}(i{=N}, i1)
%0 = call i{=N} @llvm.abs.i{=N}(i{=N} %self, i1 false)
ret i{=N} %0
```

\_\_add\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/int/#class-int-__add__ "Copy/link to this API element")

@pure @commutative @associative @llvm

```python
__add__(self: Self, other: Int[N], Self: type)
```

```llvm
%0 = add i{=N} %self, %other
ret i{=N} %0
```

\_\_sub\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/int/#class-int-__sub__ "Copy/link to this API element")

@pure @llvm

```python
__sub__(self: Self, other: Int[N], Self: type)
```

```llvm
%0 = sub i{=N} %self, %other
ret i{=N} %0
```

\_\_mul\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/int/#class-int-__mul__ "Copy/link to this API element")

@pure @commutative @associative @distributive @llvm

```python
__mul__(self: Self, other: Int[N], Self: type)
```

```llvm
%0 = mul i{=N} %self, %other
ret i{=N} %0
```

\_\_floordiv\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/int/#class-int-__floordiv__ "Copy/link to this API element")

```python
__floordiv__(self: Self, other: Int[N], Self: type)
```

\_\_truediv\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/int/#class-int-__truediv__ "Copy/link to this API element")

```python
__truediv__(self: Self, other: Int[N], Self: type)
```

\_\_mod\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/int/#class-int-__mod__ "Copy/link to this API element")

```python
__mod__(self: Self, other: Int[N], Self: type)
```

\_\_divmod\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/int/#class-int-__divmod__ "Copy/link to this API element")

```python
__divmod__(self: Self, other: Int[N], Self: type)
```

\_\_lshift\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/int/#class-int-__lshift__ "Copy/link to this API element")

```python
__lshift__(self: Self, other: Int[N], Self: type)
```

```python
__lshift__(self: Self, other: int, Self: type)
```

\_\_rshift\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/int/#class-int-__rshift__ "Copy/link to this API element")

```python
__rshift__(self: Self, other: Int[N], Self: type)
```

```python
__rshift__(self: Self, other: int, Self: type)
```

\_\_eq\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/int/#class-int-__eq__ "Copy/link to this API element")

@pure @llvm

```python
__eq__(self: Self, other: Int[N], Self: type)
```

```llvm
%0 = icmp eq i{=N} %self, %other
%1 = zext i1 %0 to i8
ret i8 %1
```

\_\_ne\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/int/#class-int-__ne__ "Copy/link to this API element")

@pure @llvm

```python
__ne__(self: Self, other: Int[N], Self: type)
```

```llvm
%0 = icmp ne i{=N} %self, %other
%1 = zext i1 %0 to i8
ret i8 %1
```

\_\_lt\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/int/#class-int-__lt__ "Copy/link to this API element")

@pure @llvm

```python
__lt__(self: Self, other: Int[N], Self: type)
```

```llvm
%0 = icmp slt i{=N} %self, %other
%1 = zext i1 %0 to i8
ret i8 %1
```

\_\_gt\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/int/#class-int-__gt__ "Copy/link to this API element")

@pure @llvm

```python
__gt__(self: Self, other: Int[N], Self: type)
```

```llvm
%0 = icmp sgt i{=N} %self, %other
%1 = zext i1 %0 to i8
ret i8 %1
```

\_\_le\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/int/#class-int-__le__ "Copy/link to this API element")

@pure @llvm

```python
__le__(self: Self, other: Int[N], Self: type)
```

```llvm
%0 = icmp sle i{=N} %self, %other
%1 = zext i1 %0 to i8
ret i8 %1
```

\_\_ge\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/int/#class-int-__ge__ "Copy/link to this API element")

@pure @llvm

```python
__ge__(self: Self, other: Int[N], Self: type)
```

```llvm
%0 = icmp sge i{=N} %self, %other
%1 = zext i1 %0 to i8
ret i8 %1
```

\_\_and\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/int/#class-int-__and__ "Copy/link to this API element")

@pure @commutative @associative @llvm

```python
__and__(self: Self, other: Int[N], Self: type)
```

```llvm
%0 = and i{=N} %self, %other
ret i{=N} %0
```

\_\_or\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/int/#class-int-__or__ "Copy/link to this API element")

@pure @commutative @associative @llvm

```python
__or__(self: Self, other: Int[N], Self: type)
```

```llvm
%0 = or i{=N} %self, %other
ret i{=N} %0
```

\_\_xor\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/int/#class-int-__xor__ "Copy/link to this API element")

@pure @commutative @associative @llvm

```python
__xor__(self: Self, other: Int[N], Self: type)
```

```llvm
%0 = xor i{=N} %self, %other
ret i{=N} %0
```

\_\_pow\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/int/#class-int-__pow__ "Copy/link to this API element")

```python
__pow__(self: Self, exp: Int[N], Self: type)
```

```python
__pow__(self: Self, exp: Literal[int], Self: type)
```

\_\_repr\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/int/#class-int-__repr__ "Copy/link to this API element")

```python
__repr__(self: Self, Self: type)
```

\_\_match\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/int/#class-int-__match__ "Copy/link to this API element")

```python
__match__(self: Self, obj: Int[N], Self: type)
```

\_\_round\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/int/#class-int-__round__ "Copy/link to this API element")

```python
__round__(self: Self, ndigits = None, Self: type)
```

\_\_atomic\_xchg\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/int/#class-int-__atomic_xchg__ "Copy/link to this API element")

@nocapture @llvm

```python
__atomic_xchg__(d: Ptr[Int[N]], b: Int[N])
```

```llvm
%tmp = atomicrmw xchg ptr %d, i{=N} %b seq_cst
ret i{=N} %tmp
```

\_\_atomic\_add\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/int/#class-int-__atomic_add__ "Copy/link to this API element")

@nocapture @llvm

```python
__atomic_add__(d: Ptr[Int[N]], b: Int[N])
```

```llvm
%tmp = atomicrmw add ptr %d, i{=N} %b seq_cst
ret i{=N} %tmp
```

\_\_atomic\_sub\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/int/#class-int-__atomic_sub__ "Copy/link to this API element")

@nocapture @llvm

```python
__atomic_sub__(d: Ptr[Int[N]], b: Int[N])
```

```llvm
%tmp = atomicrmw sub ptr %d, i{=N} %b seq_cst
ret i{=N} %tmp
```

\_\_atomic\_and\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/int/#class-int-__atomic_and__ "Copy/link to this API element")

@nocapture @llvm

```python
__atomic_and__(d: Ptr[Int[N]], b: Int[N])
```

```llvm
%tmp = atomicrmw and ptr %d, i{=N} %b seq_cst
ret i{=N} %tmp
```

\_\_atomic\_nand\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/int/#class-int-__atomic_nand__ "Copy/link to this API element")

@nocapture @llvm

```python
__atomic_nand__(d: Ptr[Int[N]], b: Int[N])
```

```llvm
%tmp = atomicrmw nand ptr %d, i{=N} %b seq_cst
ret i{=N} %tmp
```

\_\_atomic\_or\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/int/#class-int-__atomic_or__ "Copy/link to this API element")

@nocapture @llvm

```python
__atomic_or__(d: Ptr[Int[N]], b: Int[N])
```

```llvm
%tmp = atomicrmw or ptr %d, i{=N} %b seq_cst
ret i{=N} %tmp
```

\_\_atomic\_xor\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/int/#class-int-__atomic_xor__ "Copy/link to this API element")

@nocapture @llvm

```python
__atomic_xor__(d: Ptr[Int[N]], b: Int[N])
```

```llvm
%tmp = atomicrmw xor ptr %d, i{=N} %b seq_cst
ret i{=N} %tmp
```

\_\_atomic\_min\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/int/#class-int-__atomic_min__ "Copy/link to this API element")

@nocapture @llvm

```python
__atomic_min__(d: Ptr[Int[N]], b: Int[N])
```

```llvm
%tmp = atomicrmw min ptr %d, i{=N} %b seq_cst
ret i{=N} %tmp
```

\_\_atomic\_max\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/int/#class-int-__atomic_max__ "Copy/link to this API element")

@nocapture @llvm

```python
__atomic_max__(d: Ptr[Int[N]], b: Int[N])
```

```llvm
%tmp = atomicrmw max ptr %d, i{=N} %b seq_cst
ret i{=N} %tmp
```

class UInt[](https://docs.exaloop.io/libraries/api/modules/internal/types/int/#class-uint "Copy/link to this API element")

@extend

```python
class UInt[N]
```

Extends [`UInt`](https://docs.exaloop.io/libraries/api/classes/UInt/) · [View complete class API →](https://docs.exaloop.io/libraries/api/classes/UInt/)

Constructors 4

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/int/#class-uint-__new__ "Copy/link to this API element")

```python
__new__()
```

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/int/#class-uint-__new__-overload-2 "Copy/link to this API element")

@overload

```python
__new__(what: UInt[M], M: Literal[int])
```

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/int/#class-uint-__new__-overload-3 "Copy/link to this API element")

@overload

```python
__new__(what: Int[M], M: Literal[int])
```

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/int/#class-uint-__new__-overload-4 "Copy/link to this API element")

```python
__new__(what: str, base: int = 10)
```

Properties 2

real[](https://docs.exaloop.io/libraries/api/modules/internal/types/int/#class-uint-real "Copy/link to this API element")

@property

```python
real(self: Self, Self: type)
```

imag[](https://docs.exaloop.io/libraries/api/modules/internal/types/int/#class-uint-imag "Copy/link to this API element")

@property

```python
imag(self: Self, Self: type)
```

Methods 4

cdiv[](https://docs.exaloop.io/libraries/api/modules/internal/types/int/#class-uint-cdiv "Copy/link to this API element")

```python
cdiv(self: Self, other: UInt[N], Self: type)
```

cmod[](https://docs.exaloop.io/libraries/api/modules/internal/types/int/#class-uint-cmod "Copy/link to this API element")

```python
cmod(self: Self, other: UInt[N], Self: type)
```

popcnt[](https://docs.exaloop.io/libraries/api/modules/internal/types/int/#class-uint-popcnt "Copy/link to this API element")

```python
popcnt(self: Self, Self: type)
```

bit\_length[](https://docs.exaloop.io/libraries/api/modules/internal/types/int/#class-uint-bit_length "Copy/link to this API element")

```python
bit_length(self: Self, Self: type)
```

Special methods 42

\_\_int\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/int/#class-uint-__int__ "Copy/link to this API element")

```python
__int__(self: Self, Self: type)
```

\_\_index\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/int/#class-uint-__index__ "Copy/link to this API element")

```python
__index__(self: Self, Self: type)
```

\_\_copy\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/int/#class-uint-__copy__ "Copy/link to this API element")

```python
__copy__(self: Self, Self: type)
```

\_\_deepcopy\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/int/#class-uint-__deepcopy__ "Copy/link to this API element")

```python
__deepcopy__(self: Self, Self: type)
```

\_\_hash\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/int/#class-uint-__hash__ "Copy/link to this API element")

```python
__hash__(self: Self, Self: type)
```

\_\_float\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/int/#class-uint-__float__ "Copy/link to this API element")

@pure @llvm

```python
__float__(self: Self, Self: type)
```

```llvm
%0 = uitofp i{=N} %self to double
ret double %0
```

\_\_bool\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/int/#class-uint-__bool__ "Copy/link to this API element")

@pure @llvm

```python
__bool__(self: Self, Self: type)
```

```llvm
%0 = icmp ne i{=N} %self, 0
%1 = zext i1 %0 to i8
ret i8 %1
```

\_\_pos\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/int/#class-uint-__pos__ "Copy/link to this API element")

```python
__pos__(self: Self, Self: type)
```

\_\_neg\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/int/#class-uint-__neg__ "Copy/link to this API element")

@pure @llvm

```python
__neg__(self: Self, Self: type)
```

```llvm
%0 = sub i{=N} 0, %self
ret i{=N} %0
```

\_\_invert\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/int/#class-uint-__invert__ "Copy/link to this API element")

@pure @llvm

```python
__invert__(self: Self, Self: type)
```

```llvm
%0 = xor i{=N} %self, -1
ret i{=N} %0
```

\_\_abs\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/int/#class-uint-__abs__ "Copy/link to this API element")

```python
__abs__(self: Self, Self: type)
```

\_\_add\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/int/#class-uint-__add__ "Copy/link to this API element")

@pure @commutative @associative @llvm

```python
__add__(self: Self, other: UInt[N], Self: type)
```

```llvm
%0 = add i{=N} %self, %other
ret i{=N} %0
```

\_\_sub\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/int/#class-uint-__sub__ "Copy/link to this API element")

@pure @llvm

```python
__sub__(self: Self, other: UInt[N], Self: type)
```

```llvm
%0 = sub i{=N} %self, %other
ret i{=N} %0
```

\_\_mul\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/int/#class-uint-__mul__ "Copy/link to this API element")

@pure @commutative @associative @distributive @llvm

```python
__mul__(self: Self, other: UInt[N], Self: type)
```

```llvm
%0 = mul i{=N} %self, %other
ret i{=N} %0
```

\_\_floordiv\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/int/#class-uint-__floordiv__ "Copy/link to this API element")

```python
__floordiv__(self: Self, other: UInt[N], Self: type)
```

\_\_truediv\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/int/#class-uint-__truediv__ "Copy/link to this API element")

```python
__truediv__(self: Self, other: UInt[N], Self: type)
```

\_\_mod\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/int/#class-uint-__mod__ "Copy/link to this API element")

```python
__mod__(self: Self, other: UInt[N], Self: type)
```

\_\_divmod\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/int/#class-uint-__divmod__ "Copy/link to this API element")

```python
__divmod__(self: Self, other: UInt[N], Self: type)
```

\_\_lshift\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/int/#class-uint-__lshift__ "Copy/link to this API element")

```python
__lshift__(self: Self, other: UInt[N], Self: type)
```

```python
__lshift__(self: Self, other: int, Self: type)
```

\_\_rshift\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/int/#class-uint-__rshift__ "Copy/link to this API element")

```python
__rshift__(self: Self, other: UInt[N], Self: type)
```

```python
__rshift__(self: Self, other: int, Self: type)
```

\_\_eq\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/int/#class-uint-__eq__ "Copy/link to this API element")

@pure @llvm

```python
__eq__(self: Self, other: UInt[N], Self: type)
```

```llvm
%0 = icmp eq i{=N} %self, %other
%1 = zext i1 %0 to i8
ret i8 %1
```

\_\_ne\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/int/#class-uint-__ne__ "Copy/link to this API element")

@pure @llvm

```python
__ne__(self: Self, other: UInt[N], Self: type)
```

```llvm
%0 = icmp ne i{=N} %self, %other
%1 = zext i1 %0 to i8
ret i8 %1
```

\_\_lt\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/int/#class-uint-__lt__ "Copy/link to this API element")

@pure @llvm

```python
__lt__(self: Self, other: UInt[N], Self: type)
```

```llvm
%0 = icmp ult i{=N} %self, %other
%1 = zext i1 %0 to i8
ret i8 %1
```

\_\_gt\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/int/#class-uint-__gt__ "Copy/link to this API element")

@pure @llvm

```python
__gt__(self: Self, other: UInt[N], Self: type)
```

```llvm
%0 = icmp ugt i{=N} %self, %other
%1 = zext i1 %0 to i8
ret i8 %1
```

\_\_le\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/int/#class-uint-__le__ "Copy/link to this API element")

@pure @llvm

```python
__le__(self: Self, other: UInt[N], Self: type)
```

```llvm
%0 = icmp ule i{=N} %self, %other
%1 = zext i1 %0 to i8
ret i8 %1
```

\_\_ge\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/int/#class-uint-__ge__ "Copy/link to this API element")

@pure @llvm

```python
__ge__(self: Self, other: UInt[N], Self: type)
```

```llvm
%0 = icmp uge i{=N} %self, %other
%1 = zext i1 %0 to i8
ret i8 %1
```

\_\_and\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/int/#class-uint-__and__ "Copy/link to this API element")

@pure @commutative @associative @llvm

```python
__and__(self: Self, other: UInt[N], Self: type)
```

```llvm
%0 = and i{=N} %self, %other
ret i{=N} %0
```

\_\_or\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/int/#class-uint-__or__ "Copy/link to this API element")

@pure @commutative @associative @llvm

```python
__or__(self: Self, other: UInt[N], Self: type)
```

```llvm
%0 = or i{=N} %self, %other
ret i{=N} %0
```

\_\_xor\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/int/#class-uint-__xor__ "Copy/link to this API element")

@pure @commutative @associative @llvm

```python
__xor__(self: Self, other: UInt[N], Self: type)
```

```llvm
%0 = xor i{=N} %self, %other
ret i{=N} %0
```

\_\_pow\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/int/#class-uint-__pow__ "Copy/link to this API element")

```python
__pow__(self: Self, exp: UInt[N], Self: type)
```

```python
__pow__(self: Self, exp: Literal[int], Self: type)
```

\_\_repr\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/int/#class-uint-__repr__ "Copy/link to this API element")

```python
__repr__(self: Self, Self: type)
```

\_\_match\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/int/#class-uint-__match__ "Copy/link to this API element")

```python
__match__(self: Self, obj: UInt[N], Self: type)
```

\_\_round\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/int/#class-uint-__round__ "Copy/link to this API element")

```python
__round__(self: Self, ndigits = None, Self: type)
```

\_\_atomic\_xchg\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/int/#class-uint-__atomic_xchg__ "Copy/link to this API element")

@nocapture @llvm

```python
__atomic_xchg__(d: Ptr[UInt[N]], b: UInt[N])
```

```llvm
%tmp = atomicrmw xchg ptr %d, i{=N} %b seq_cst
ret i{=N} %tmp
```

\_\_atomic\_add\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/int/#class-uint-__atomic_add__ "Copy/link to this API element")

@nocapture @llvm

```python
__atomic_add__(d: Ptr[UInt[N]], b: UInt[N])
```

```llvm
%tmp = atomicrmw add ptr %d, i{=N} %b seq_cst
ret i{=N} %tmp
```

\_\_atomic\_sub\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/int/#class-uint-__atomic_sub__ "Copy/link to this API element")

@nocapture @llvm

```python
__atomic_sub__(d: Ptr[UInt[N]], b: UInt[N])
```

```llvm
%tmp = atomicrmw sub ptr %d, i{=N} %b seq_cst
ret i{=N} %tmp
```

\_\_atomic\_and\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/int/#class-uint-__atomic_and__ "Copy/link to this API element")

@nocapture @llvm

```python
__atomic_and__(d: Ptr[UInt[N]], b: UInt[N])
```

```llvm
%tmp = atomicrmw and ptr %d, i{=N} %b seq_cst
ret i{=N} %tmp
```

\_\_atomic\_nand\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/int/#class-uint-__atomic_nand__ "Copy/link to this API element")

@nocapture @llvm

```python
__atomic_nand__(d: Ptr[UInt[N]], b: UInt[N])
```

```llvm
%tmp = atomicrmw nand ptr %d, i{=N} %b seq_cst
ret i{=N} %tmp
```

\_\_atomic\_or\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/int/#class-uint-__atomic_or__ "Copy/link to this API element")

@nocapture @llvm

```python
__atomic_or__(d: Ptr[UInt[N]], b: UInt[N])
```

```llvm
%tmp = atomicrmw or ptr %d, i{=N} %b seq_cst
ret i{=N} %tmp
```

\_\_atomic\_xor\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/int/#class-uint-__atomic_xor__ "Copy/link to this API element")

@nocapture @llvm

```python
__atomic_xor__(d: Ptr[UInt[N]], b: UInt[N])
```

```llvm
%tmp = atomicrmw xor ptr %d, i{=N} %b seq_cst
ret i{=N} %tmp
```

\_\_atomic\_min\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/int/#class-uint-__atomic_min__ "Copy/link to this API element")

@nocapture @llvm

```python
__atomic_min__(d: Ptr[UInt[N]], b: UInt[N])
```

```llvm
%tmp = atomicrmw umin ptr %d, i{=N} %b seq_cst
ret i{=N} %tmp
```

\_\_atomic\_max\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/types/int/#class-uint-__atomic_max__ "Copy/link to this API element")

@nocapture @llvm

```python
__atomic_max__(d: Ptr[UInt[N]], b: UInt[N])
```

```llvm
%tmp = atomicrmw umax ptr %d, i{=N} %b seq_cst
ret i{=N} %tmp
```

Source: https://docs.exaloop.io/libraries/api/modules/internal/types/int/
