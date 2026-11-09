---
title: Inline LLVM IR
---

Codon supports inline [LLVM IR](https://llvm.org/docs/LangRef.html) via
the `@llvm` annotation:

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

The substitution chooses a type, not an instruction. This example uses the
integer `add` instruction, so a floating-point implementation would need
`fadd` instead.

LLVM intrinsics can be accessed with `declare`:

``` python
@llvm
def popcnt(n: int) -> int:
    declare i64 @llvm.ctpop.i64(i64)
    %0 = call i64 @llvm.ctpop.i64(i64 %n)
    ret i64 %0

print(popcnt(42))  # 3
```

## Returning `None`

An inline LLVM function returning Codon's `None` uses an empty structure as
its LLVM return value, rather than `ret void`. For example, a helper that
writes through a pointer can be defined as follows:

``` python
@llvm
def store_value(pointer: Ptr[int], value: int) -> None:
  store i64 %value, ptr %pointer, align 1
  ret {} zeroinitializer

buffer = Ptr[int](1)
store_value(buffer, 42)
print(buffer[0])  # 42
```

The pointer still needs to refer to writable storage for an integer; inline
LLVM does not add bounds or lifetime checks. This function intentionally has
no purity annotation because its store changes memory visible to the caller.

## Annotations

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
annotation can be used to indicate that the return
value of the function captures its arguments:

- `@derives`: Return value of the function captures its arguments.

These annotations describe contracts that the optimizer may rely on; they do
not add runtime checks. Correct annotations preserve the function's intended
behavior while enabling optimizations, but an incorrect contract can cause
operations to be removed or reordered. When a function's effects are not
clear, leaving an annotation out is preferable to making an unsupported
promise.
