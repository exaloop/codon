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

# Annotations

Sometimes it can be helpful to annotate `@llvm` functions to give
the compiler more information as to how they behave. Codon has
a number of default annotations for LLVM functions (all of
which also apply to external/C functions):

- `@pure`: Function does not capture arguments (aside from
  return value capturing as in `def foo(x): return x`), does not
  modify arguments, and has no side effects. This is a
  mathematically "pure" function.

- `@no_side_effect`: Very similar to `@pure` but function may
  return different results on different calls, such as the C
  function `time()`.

- `@nocapture`: Function does not capture any of its arguments
  (again excluding return value capturing).

- `@self_captures`: Function's first (`self`) argument captures
  the other arguments, an example being `List.__setitem__()`.

These are mutually-exclusive annotations. Another complementary
annotation `@derives` can be used to indicate that the return
value of the function captures its arguments.

These annotations are completely optional and do not affect
program semantics.
