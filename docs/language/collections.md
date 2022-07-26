Collections are largely the same as in Python:

``` python
l = [1, 2, 3]                         # type: List[int]; a list of integers
s = {1.1, 3.3, 2.2, 3.3}              # type: Set[float]; a set of floats
d = {1: 'hi', 2: 'ola', 3: 'zdravo'}  # type: Dict[int, str]; a dictionary of int to str

ln = []                               # an empty list whose type is inferred based on usage
ln = List[int]()                      # an empty list with explicit element type
dn = {}                               # an empty dict whose type is inferred based on usage
dn = Dict[int, float]()               # an empty dictionary with explicit element types
sn = set()                            # an empty set whose type is inferred based on usage
sn = Set[str]()                       # an empty set with explicit element type
```

Lists also take an optional `capacity` constructor argument, which can be useful
when creating large lists:

``` python
squares = list(capacity=1_000_000)  # list with room for 1M elements

# Fill the list
for i in range(1_000_000):
    squares.append(i ** 2)
```

{% hint style="info" %}
Dictionaries and sets are unordered and are based on [klib](https://github.com/attractivechaos/klib).
{% endhint %}

# Comprehensions

Comprehensions are a nifty, Pythonic way to create collections, and are fully
supported by Codon:

``` python
l = [i for i in range(5)]                                  # type: List[int]; l is [0, 1, 2, 3, 4]
l = [i for i in range(15) if i % 2 == 1 if i > 10]         # type: List[int]; l is [11, 13]
l = [i * j for i in range(5) for j in range(5) if i == j]  # l is [0, 1, 4, 9, 16]

s = {abs(i - j) for i in range(5) for j in range(5)}  # s is {0, 1, 2, 3, 4}
d = {i: f'item {i+1}' for i in range(3)}              # d is {0: "item 1", 1: "item 2", 2: "item 3"}
```

You can also use generators to create collections:

``` python
g = (i for i in range(10))
print(list(g))  # prints list of integers from 0 to 9, inclusive
```

# Tuples

``` python
t = (1, 2.3, 'hi')  # type: Tuple[int, float, str]
t[1]                # type: float
u = (1, )           # type: Tuple[int]
```

As all types must be known at compile time, tuple indexing works only if
a tuple is homogenous (all types are the same) or if the value of the
index is known at compile time.

You can, however, iterate over heterogenous tuples in Codon. This is
achieved behind the scenes by unrolling the loop to accommodate the
different types.

``` python
t = (1, 2.3, 'hi')
t[1]  # works because 1 is a constant int

x = int(argv[1])
t[x]  # compile error: x is not known at compile time

# This is a homogenous tuple (all member types are the same)
u = (1, 2, 3)  # type: Tuple[int, int, int]
u[x]           # works because tuple members share the same type regardless of x
for i in u:    # works
    print(i)

# Also works
v = (42, 'x', 3.14)
for i in v:
    print(i)
```

{% hint style="warning" %}
Just like in Python, tuples are immutable, so `a = (1, 2); a[1] = 1` will not compile.
{% endhint %}

Codon supports most of Python's tuple unpacking syntax:

``` python
x, y = 1, 2                # x is 1, y is 2
(x, (y, z)) = 1, (2, 3)    # x is 1, y is 2, z is 3
[x, (y, z)] = (1, [2, 3])  # x is 1, y is 2, z is 3

l = range(1, 8)    # l is [1, 2, 3, 4, 5, 6, 7]
a, b, *mid, c = l  # a is 1, b is 2, mid is [3, 4, 5, 6], c is 7
a, *end = l        # a is 1, end is [2, 3, 4, 5, 6, 7]
*beg, c = l        # c is 7, beg is [1, 2, 3, 4, 5, 6]
(*x, ) = range(3)  # x is [0, 1, 2]
*x = range(3)      # error: this does not work

*sth, a, b = (1, 2, 3, 4)      # sth is (1, 2), a is 3, b is 4
*sth, a, b = (1.1, 2, 3.3, 4)  # error: this only works on homogenous tuples for now

(x, y), *pff, z = [1, 2], 'this'
print(x, y, pff, z)               # x is 1, y is 2, pff is an empty tuple --- () ---, and z is "this"

s, *q = 'XYZ'  # works on strings as well; s is "X" and q is "YZ"
```

# Strong typing

Because Codon is strongly typed, these won't compile:

``` python
l = [1, 's']   # is it a List[int] or List[str]? you cannot mix-and-match types
d = {1: 'hi'}
d[2] = 3       # d is a Dict[int, str]; the assigned value must be a str

t = (1, 2.2)  # Tuple[int, float]
lt = list(t)  # compile error: t is not homogenous

lp = [1, 2.1, 3, 5]  # compile error: Codon will not automatically cast a float to an int
```

This will work, though:

``` python
u = (1, 2, 3)
lu = list(u)  # works: u is homogenous
```
