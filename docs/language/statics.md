Sometimes, certain values or conditions need to be known
at compile time. For example, the bit width `N` of an
integer type `Int[N]`, or the size `M` of a static array
`__array__[int](M)` need to be compile time constants.

To accomodate this, Codon uses *static values*, i.e.
values that are known and can be operated on at compile
time. `Static[T]` represents a static value of type `T`.
Currently, `T` can only be `int` or `str`.

For example, we can parameterize the bit width of an
integer type as follows:

``` python
N: Static[int] = 32

a = Int[N](10)      # 32-bit integer 10
b = Int[2 * N](20)  # 64-bit integer 20
```

All of the standard arithmetic operations can be applied
to static integers to produce new static integers.

Statics can also be passed to the `codon` compiler via the
`-D` flag, as in `-DN=32`.

Classes can also be parameterized by statics:

``` python
class MyInt[N: Static[int]]:
    n: Int[N]

x = MyInt[16](i16(42))
```

# Static evaluation

In certain cases a program might need to check a particular
type and perform different actions based on it. For example:

``` python
def flatten(x):
    if isinstance(x, list):
        for a in x:
            flatten(a)
    else:
        print(x)

flatten([[1,2,3], [], [4, 5], [6]])
```

Standard static typing on this program would be problematic
since, if `x` is an `int`, it would not be iterable and hence
would produce an error on `for a in x`. Codon solves this problem
by evaluating certain conditions at compile time, such as
`isinstance(x, list)`, and avoiding type checking blocks that it
knows will never be reached. In fact, this program works and flattens
the argument list.

Static evaluation works with plain static types as well as general
types used in conjunction with `type`, `isinstance` or `hasattr`.
