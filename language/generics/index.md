# Generics

Codon infers types from the way values are created and used, which often leaves ordinary Python code free of annotations. Explicit types become useful when describing an interface, choosing the element type of an empty collection, or expressing relationships between a function's inputs and outputs.

A _generic_ definition leaves some of those types open until it is used. For example, `list[int]` and `list[str]` share a container implementation while storing different element types. A generic function can make the same kind of distinction without requiring a separate source definition for each type.

## Describing data

Container annotations describe the types stored in a collection, not its current contents or length. A type alias can give a frequently used combination a convenient name:

```python
Counts = dict[str, int]

def add_count(counts: Counts, name: str):
    counts[name] = counts.get(name, 0) + 1

counts: Counts = {}
add_count(counts, "oak")
add_count(counts, "oak")
print(counts["oak"])  # 2
```

The dictionary can grow at runtime, while its key and value types remain `str` and `int`. `Counts` is an alias for that dictionary type, rather than a new container class.

### Values that may be absent

`Optional[T]` represents either a value of type `T` or `None`. It is useful for operations that may have no result without treating that outcome as an error:

```python
def first_positive(values: list[int]) -> Optional[int]:
    for value in values:
        if value > 0:
            return value
    return None

result = first_positive([-3, 0, 7])
if result is not None:
    print(result + 1)  # 8
else:
    print("No positive value")
```

Checking for `None` before using the result makes both cases visible to the reader and allows the non-empty branch to work with the underlying value. For a default argument, the same annotation can distinguish an omitted value from a meaningful value such as zero.

### Values with several possible types

`Union` describes a finite set of alternatives. This gives a collection a single element type while still allowing its elements to hold different kinds of values:

```python
values: list[Union[int, str]] = [2, "ready"]
for value in values:
    if isinstance(value, int):
        print(value + 1)  # 3
    else:
        print(value.upper())  # READY
```

The `isinstance()` check selects the operation appropriate to the stored value. Unlike the compile-time checks in a function specialized for one concrete type, a union may need this distinction at runtime. When alternatives share a common interface, a [base class](https://docs.exaloop.io/language/classes/#inheritance) can be a more useful representation than enumerating each concrete type in a union.

## Generic functions

Imagine we want to enforce that a particular function should only accept a list argument; we can achieve that using generic types:

```python
def foo[T](x: list[T]):  # 'T' is a generic type parameter
    print(max(x))

foo([20, 42, 12])        # 42
foo(['hello', 'world'])  # world
```

info

This syntax is supported by Python 3.12 and up. See [PEP 695](https://peps.python.org/pep-0695/).

In this code, `T` is a _generic type parameter_ that gets realized based on the argument type:

-   `foo([20, 42, 12])` → the argument is a list of integers, so `T` is realized as `int`.
-   `foo(['hello', 'world'])` → the argument is a list of strings, so `T` is realized as `str`.
-   `foo(100)` → the argument is not a list at all, so a type checking error occurs.

For comparison, the following call is intentionally invalid and fails during compilation, before the program runs:

```python
foo(100)
```

Alternatively, Codon allows the generic type parameter to be specified as an argument:

```python
def foo(x: list[T], T: type):
    print(max(x))

foo([20, 42, 12], int)    # can specify 'T' explicitly...
foo([20, 42, 12], T=int)  # ... or by name
foo([20, 42, 12])         # still works; 'T' inferred
```

This syntax is useful for allowing generic type parameters to be specified explicitly as arguments when the function is called, or for providing default values for them:

```python
def bar(x: list[T], T: type = int):
    print(type(x))

bar([])  # <class 'List[int]'>
```

## Generic classes

Classes can also be generic:

```python
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

```python
class A:  # identical to above definition
    x: T
    T: type

    def __init__(self, x: T):
        self.x = x
```

Source: https://docs.exaloop.io/language/generics/
