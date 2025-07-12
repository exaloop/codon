Codon's type system is designed to be non-intrusive, meaning
it will infer types as much as possible without manual type
annotations. This way, Python programs typically "just work"
without requiring code changes.

Occasionally, it can be helpful to use Codon's type system to
express more intricate type relationships. This can be achieved
through *generics*.

## Generic functions

Imagine we want to enforce that a particular function should
only accept a list argument; we can achieve that using
generic types:

``` python
def foo[T](x: list[T]):  # 'T' is a generic type parameter
    print(max(x))

foo([20, 42, 12])        # 42
foo(['hello', 'world'])  # world
foo(100)                 # error: 'int' does not match expected type 'List[T]'
```

!!! info

    This syntax is supported by Python 3.12 and up. See [PEP 695](https://peps.python.org/pep-0695/).

In this code, `T` is a *generic type parameter* that gets realized
based on the argument type:

- `foo([20, 42, 12])` &#8594; the argument is a list of integers,
  so `T` is realized as `int`.
- `foo(['hello', 'world'])` &#8594; the argument is a list of strings,
  so `T` is realized as `str`.
- `foo(100)` &#8594; the argument is not a list at all, so a type checking
  error occurs.

Alternatively, Codon allows the generic type parameter to be specified
as an argument:

``` python
def foo(x: list[T], T: type):
    print(max(x))

foo([20, 42, 12], int)    # can specify 'T' explicitly...
foo([20, 42, 12], T=int)  # ... or by name
foo([20, 42, 12])         # still works; 'T' inferred
```

This syntax is useful for allowing generic type parameters to be specified
explicitly as arguments when the function is called, or for providing default
values for them:

``` python
def bar(x: list[T], T: type = int):
    print(type(x))

bar([])  # <class 'List[int]'>
```

## Generic classes

Classes can also be generic:

``` python
class A[T]:  # 'T' is a generic type parameter
    x: T

    def __init__(self, x: T):
        self.x = x

    def __repr__(self):
        return f'A({self.x})'

x = A(42)            # 'T' inferred
y = A[str]('hello')  # 'T' explicitly given as 'str'

print(x)  # A(42)
print(y)  # A(hello)
```

Generic type parameters can also be listed after the class fields:

``` python
class A:  # identical to above definition
    x: T
    T: type

    def __init__(self, x: T):
        self.x = x
```
