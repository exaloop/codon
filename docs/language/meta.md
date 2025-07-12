## Literals

Codon supports compile-time metaprogramming through the
special `Literal` type. `Literal`s represent constants which
can be operated on and manipulated *at compile time*.
There are three types of `Literal`s:

- `Literal[int]`: integer constant
- `Literal[str]`: string constant
- `Literal[bool]`: boolean constant

### Integer literals

Integer literals can be defined explicitly as follows:

``` python
n: Literal[int] = 42
```

Literals must be known at compile time. For example, the following
code will cause a compilation error:

``` python
from sys import argv
n: Literal[int] = len(argv)  # error: value is not literal!
```

Arithmetic operations on integer literals result in other
integer literals:

``` python
n: Literal[int] = 42
m: Literal[int] = n + 1  # valid int literal
```

Conditional expressions on literals also result in other
literals:

``` python
n: Literal[int] = 42
m: Literal[int] = n//2 if n%2 == 0 else 3*n + 1
```

Integer literals can be passed as function arguments, and
also returned from functions:

``` python
def fib(n: Literal[int]) -> Literal[int]:
    return 1 if n < 2 else fib(n - 1) + fib(n - 2)

n: Literal[int] = fib(10)  # computed entirely at compile time!
```

### String literals

Much like integer literals, string literals represent constant
strings:

``` python
s: Literal[str] = 'hello'
```

Whereas integer literals can be manipulated via arithmetic operations
to produce other integer literals, string literals can be manipulated
via string operations to produce new string literals:

``` python
t: Literal[str] = s[1]   # 'e'
u: Literal[str] = s[3:]  # 'lo'
```

Much like integer literals, string literals can similarly be used in
literal conditional expressions and as function argument or return
types.

### Boolean literals

Finally, boolean literals represent constant booleans:

``` python
b: Literal[bool] = True
```

Boolean operators can be used on boolean literals to produce new
boolean literals:

``` python
b: Literal[bool] = True
d: Literal[bool] = not b  # False
```

Some operations on integer and string literals produce boolean literals:

``` python
n: Literal[int] = 42
s: Literal[str] = 'hello'

b: Literal[bool] = (n < 10)          # False
d: Literal[bool] = (s[2:4] == 'll')  # True
```

## Static loops

It is also possible to express loops where the loop index is a literal
integer, via the `codon.static` module:

``` python
import codon.static

for i in static.range(10):
    m: Literal[int] = 3*i + 1
    print(m)
```

Static loops are unrolled at compile time, which allows the loop index
to take on literal values.

Static loops can also be used to create tuples, the lengths of which
must be compile-time constants in Codon:

``` python
import codon.static
t = tuple(i*i for i in static.range(5))
print(t)  # (0, 1, 4, 9, 16)
```

You can loop over another tuple by obtaining its length as an integer
literal via `static.len()`:

``` python
import codon.static

t = tuple(i*i for i in static.range(5))
u = tuple(t[i] + 1 for i in static.range(static.len(t)))

print(u)  # (1, 2, 5, 10, 17)
```

## Static evaluation

Literal expressions can be used as conditions in `if` statements, which
enables the compiler to eliminate branches that it knows wil not be
entered at runtime. This can be used to avoid type checking errors, for
example:

``` python
def foo(x):
    if isinstance(x, int):
        return x + 1
    elif isinstance(x, str):
        return x + '!'
    else:
        return x

print(foo(42))       # 43
print(foo('hello'))  # hello!
print(foo(3.14))     # 3.14
```

Normally, Codon's type checker would flag an expression like `x + 1` as
an error if the type of `x` is `str`. However, in the code above, the
branches that are not applicable to the type of `x` are eliminated so as
to allow the code to type check and compile.

Here is another, more involved example:

``` python
def flatten(x):
    if isinstance(x, list):
        for a in x:
            flatten(a)
    else:
        print(x)

flatten([[1,2,3], [], [4, 5], [6]])  # 1, 2, ..., 6
```

Standard static typing on this program would be problematic since, if `x`
is an `int`, it would not be iterable and hence would produce an error on
`for a in x`. Static evaluation solves this problem by evaluating
`isinstance(x, list)` at compile time and avoiding type checking the block
containing the loop when `x` is not a list.

Static evaluation works with literal expressions, `isinstance()`, `hasattr()`
and type comparisons like `type1 is type2`.
