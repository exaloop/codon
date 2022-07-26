If you know Python, you already know 99% of Codon. This section
covers the Codon language as well as some of the key differences
and additional features on top of Python.

# Printing

``` python
print('hello world')

from sys import stderr
print('hello world', end='', file=stderr)
```

# Comments

``` python
# Codon comments start with "# 'and go until the end of the line

"""
Multi-line comments are
possible like this.
"""
```

# Literals

``` python
# Booleans
True   # type: bool
False

# Numbers
a = 1             # type: int; a signed 64-bit integer
b = 1.12          # type: float; a 64-bit float (just like "double" in C)
c = 5u            # unsigned int; an unsigned 64-bit int
d = Int[8](12)    # 8-bit signed integer; you can go all the way to Int[2048]
e = UInt[8](200)  # 8-bit unsigned integer
f = byte(3)       # Codon's byte is equivalent to C's char; equivalent to Int[8]

h = 0x12AF   # hexadecimal integers are also welcome
g = 3.11e+9  # scientific notation is also supported
g = .223     # and this is also float
g = .11E-1   # and this as well

# Strings
s = 'hello! "^_^" '              # type: str
t = "hello there! \t \\ '^_^' "  # \t is a tab character; \\ stands for \
raw = r"hello\n"                 # raw strings do not escape slashes; this would print "hello\n"
fstr = f"a is {a + 1}"           # an f-string; prints "a is 2"
fstr = f"hi! {a+1=}"             # an f-string; prints "hi! a+1=2"
t = """
hello!
multiline string
"""

# The following escape sequences are supported:
#   \\, \', \", \a, \b, \f, \n, \r, \t, \v,
#   \xHHH (HHH is hex code), \OOO (OOO is octal code)
```

# Assignments and operators

``` python
a = 1 + 2              # this is 3
a = (1).__add__(2)     # you can use a function call instead of an operator; this is also 3
a = int.__add__(1, 2)  # this is equivalent to the previous line
b = 5 / 2.0            # this is 2.5
c = 5 // 2             # this is 2; // is an integer division
a *= 2                 # a is now 6
```

Here is the list of binary operators and each one's associated magic method:

  | Operator           | Magic method   | Description                  |
  |--------------------|----------------|------------------------------|
  | `+`                | `__add__`      | addition                     |
  | `-`                | `__sub__`      | subtraction                  |
  | `*`                | `__mul__`      | multiplication               |
  | `/`                | `__truediv__`  | float (true) division        |
  | `//`               | `__floordiv__` | integer (floor) division     |
  | `**`               | `__pow__`      | exponentiation               |
  | `%`                | `__mod__`      | modulo                       |
  | `@`                | `__matmul__`   | matrix multiplication        |
  | `&`                | `__and__`      | bitwise and                  |
  | <code>&vert;<code> | `__or__`       | bitwise or                   |
  | `^`                | `__xor__`      | bitwise xor                  |
  | `<<`               | `__lshift__`   | left bit shift               |
  | `>>`               | `__rshift__`   | right bit shift              |
  | `<`                | `__lt__`       | less than                    |
  | `<=`               | `__le__`       | less than or equal to        |
  | `>`                | `__gt__`       | greater than                 |
  | `>=`               | `__ge__`       | greater than or equal to     |
  | `==`               | `__eq__`       | equal to                     |
  | `!=`               | `__ne__`       | not equal to                 |
  | `in`               | `__contains__` | belongs to                   |
  | `and`              | none           | boolean and (short-circuits) |
  | `or`               | none           | boolean or (short-circuits)  |

Codon also has the following unary operators:

  | Operator | Magic method | Description      |
  |----------|--------------|------------------|
  | `~`      | `__invert__` | bitwise not      |
  | `+`      | `__pos__`    | unary positive   |
  | `-`      | `__neg__`    | unary negation   |
  | `not`    | none         | boolean negation |

# Control flow

## Conditionals

Codon supports the standard Python conditional syntax:

``` python
if a or b or some_cond():
    print(1)
elif whatever() or 1 < a <= b < c < 4:  # chained comparisons are supported
    print('meh...')
else:
    print('lo and behold!')

a = b if sth() else c  # ternary conditional operator
```

Codon extends the Python conditional syntax with a `match` statement,
which is inspired by Rust's:

``` python
match a + some_heavy_expr():  # assuming that the type of this expression is int
    case 1:         # is it 1?
        print('hi')
    case 2 ... 10:  # is it 2, 3, 4, 5, 6, 7, 8, 9 or 10?
        print('wow!')
    case _:         # "default" case
        print('meh...')

match bool_expr():  # now it's a bool expression
    case True:
        print('yay')
    case False:
        print('nay')

match str_expr():  # now it's a str expression
    case 'abc': print("it's ABC time!")
    case 'def' | 'ghi':  # you can chain multiple rules with the "|" operator
        print("it's not ABC time!")
    case s if len(s) > 10: print("so looong!")  # conditional match expression
    case _: assert False

match some_tuple:  # assuming type of some_tuple is Tuple[int, int]
    case (1, 2): ...
    case (a, _) if a == 42:  # you can do away with useless terms with an underscore
        print('hitchhiker!')
    case (a, 50 ... 100) | (10 ... 20, b):  # you can nest match expressions
        print('complex!')

match list_foo():
    case []:                   # [] matches an empty list
        print('A')
    case [1, 2, 3]:            # make sure that list_foo() returns List[int] though!
        print('B')
    case [1, 2, ..., 5]:       # matches any list that starts with 1 and 2 and ends with 5
        print('C')
    case [..., 6] | [6, ...]:  # matches a list that starts or ends with 6
        print('D')
    case [..., w] if w < 0:    # matches a list that ends with a negative integer
        print('E')
    case [...]:                # any other list
        print('F')
```

You can mix, match and chain match rules as long as the match type
matches the expression type.

## Loops

Standard fare:

``` python
a = 10
while a > 0:  # prints even numbers from 9 to 1
    a -= 1
    if a % 2 == 1:
        continue
    print(a)

for i in range(10):  # prints numbers from 0 to 7, inclusive
    print(i)
    if i > 6:
        break
```

`for` construct can iterate over any generator, which means any object
that implements the `__iter__` magic method. In practice, generators,
lists, sets, dictionaries, homogenous tuples, ranges, and many more
types implement this method. If you need to implement one yourself,
just keep in mind that `__iter__` is a generator and not a function.

# Imports

You can import functions and classes from another Codon module by doing:

``` python
# Create foo.codon with a bunch of useful methods
import foo

foo.useful1()
p = foo.FooType()

# Create bar.codon with a bunch of useful methods
from bar import x, y
x(y)

from bar import z as bar_z
bar_z()
```

`import foo` looks for `foo.codon` or `foo/__init__.codon` in the
current directory.

# Exceptions

Again, if you know how to do this in Python, you know how to do it in
Codon:

``` python
def throwable():
     raise ValueError("doom and gloom")

try:
    throwable()
except ValueError as e:
    print("we caught the exception")
except:
    print("ouch, we're in deep trouble")
finally:
    print("whatever, it's done")
```

{% hint style="warning" %}
Right now, Codon cannot catch multiple exceptions in one statement. Thus
`catch (Exc1, Exc2, Exc3) as var` will not compile, since the type of `var`
needs to be known ahead of time.
{% endhint %}

If you have an object that implements `__enter__` and `__exit__` methods
to manage its lifetime (say, a `File`), you can use a `with` statement
to make your life easier:

``` python
with open('foo.txt') as f, open('foo_copy.txt', 'w') as fo:
    for l in f:
        fo.write(l)
```
