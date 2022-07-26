Functions are defined as follows:

``` python
def foo(a, b, c):
    return a + b + c

print(foo(1, 2, 3))  # prints 6
```

Functions don't have to return a value:

``` python
def proc(a, b):
    print(a, 'followed by', b)

proc(1, 's')


def proc2(a, b):
    if a == 5:
        return
    print(a, 'followed by', b)

proc2(1, 's')
proc2(5, 's')  # this prints nothing
```

Codon is a strongly-typed language, so you can restrict argument and
return types:

``` python
def fn(a: int, b: float):
    return a + b  # this works because int implements __add__(float)

fn(1, 2.2)  # 3.2
fn(1.1, 2)  # error: 1.1. is not an int


def fn2(a: int, b):
    return a - b

fn2(1, 2)    # -1
fn2(1, 1.1)  # -0.1; works because int implements __sub__(float)
fn2(1, 's')  # error: there is no int.__sub__(str)!


def fn3(a, b) -> int:
    return a + b

fn3(1, 2)      # works, since 1 + 2 is an int
fn3('s', 'u')  # error: 's'+'u' returns 'su' which is str
               # but the signature indicates that it must return int
```

Default and named arguments are also supported:

``` python
def foo(a, b: int, c: float = 1.0, d: str = 'hi'):
    print(a, b, c, d)

foo(1, 2)             # prints "1 2 1 hi"
foo(1, d='foo', b=1)  # prints "1 1 1 foo"
```

As are optional arguments:

``` python
# type of b promoted to Optional[int]
def foo(a, b: int = None):
    print(a, b + 1)

foo(1, 2)  # prints "1 3"
foo(1)     # raises ValueError, since b is None
```

# Generics

Codon emulates Python's lax runtime type checking using a technique known as
*monomorphization*. If a function has an argument without a type definition,
Codon will treat it as a *generic* function, and will generate different instantiations
for each different invocation:

``` python
def foo(x):
    print(x)  # print relies on typeof(x).__repr__(x) method to print the representation of x

foo(1)        # Codon automatically generates foo(x: int) and calls int.__repr__ when needed
foo('s')      # Codon automatically generates foo(x: str) and calls str.__repr__ when needed
foo([1, 2])   # Codon automatically generates foo(x: List[int]) and calls List[int].__repr__ when needed
```

But what if you need to mix type definitions and generic types? Say,
your function can take a list of *anything*? You can use generic
type parameters:

``` python
def foo(x: List[T], T: type):
    print(x)

foo([1, 2])           # prints [1, 2]
foo(['s', 'u'])       # prints [s, u]
foo(5)                # error: 5 is not a list!
foo(['s', 'u'], int)  # fails: T is int, so foo expects List[int] but it got List[str]


def foo(x, R: type) -> R:
    print(x)
    return 1

foo(4, int)  # prints 4, returns 1
foo(4, str)  # error: return type is str, but foo returns int!
```

{% hint style="info" %}
Coming from C++? `foo(x: List[T], T: type): ...` is roughly the same as
`template<typename T, typename U> U foo(T x) { ... }`.
{% endhint %}

Generic type parameters are an optional way to enforce various typing constraints.
