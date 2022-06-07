Codon allows inline LLVM IR via the `@llvm` annotation:

``` python
@llvm
def llvm_add(a: int, b: int) -> int:
    %res = add i64 %a, %b
    ret i64 %res

print(llvm_add(3, 4))  # 7
```

Note that LLVM functions must explicitly specify argument
and return types.

LLVM functions can also be generic, and a format specifier
in the body will be replaced by the appropriate LLVM type:

``` python
@llvm
def llvm_add[T](a: T, b: T) -> T:
    %res = add {=T} %a, %b
    ret {=T} %res

print(llvm_add(3, 4))          # 7
print(llvm_add(i8(5), i8(6)))  # 11
```

You can also access LLVM intrinsics with `declare`:

``` python
@llvm
def popcnt(n: int) -> int:
    declare i64 @llvm.ctpop.i64(i64)
    %0 = call i64 @llvm.ctpop.i64(i64 %n)
    ret i64 %0

print(popcnt(42))  # 3
```
