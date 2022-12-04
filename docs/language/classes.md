Codon supports classes just like Python. However, you must declare
class members and their types in the preamble of each class (like
you would do with Python's dataclasses):

``` python
class Foo:
    x: int
    y: int

    def __init__(self, x: int, y: int):  # constructor
        self.x, self.y = x, y

    def method(self):
        print(self.x, self.y)

f = Foo(1, 2)
f.method()  # prints "1 2"
```

Unlike Python, Codon supports method overloading:

``` python
class Foo:
    x: int
    y: int

    def __init__(self):                    # constructor
        self.x, self.y = 0, 0

    def __init__(self, x: int, y: int):    # another constructor
        self.x, self.y = x, y

    def __init__(self, x: int, y: float):  # yet another constructor
        self.x, self.y = x, int(y)

    def method(self: Foo):
        print(self.x, self.y)

Foo().method()          # prints "0 0"
Foo(1, 2).method()      # prints "1 2"
Foo(1, 2.3).method()    # prints "1 2"
Foo(1.1, 2.3).method()  # error: there is no Foo.__init__(float, float)
```

Classes can also be generic:

``` python
class Container[T]:
    elements: List[T]

    def __init__(self, elements: List[T]):
        self.elements = elements
```

Classes create objects that are passed by reference:

``` python
class Point:
    x: int
    y: int

p = Point(1, 2)
q = p  # this is a reference!
p.x = 2
print((p.x, p.y), (q.x, q.y))  # (2, 2), (2, 2)
```

If you need to copy an object's contents, implement the `__copy__`
magic method and use `q = copy(p)` instead.

Classes can inherit from other classes:

```python
class NamedPoint(Point):
    name: str

    def __init__(self, x: int, y: int, name: str):
        super().__init__(x, y)
        self.name = name
```

{% hint style="warning" %}
Currently, inheritance in Codon is still under active development.
Treat it as a beta feature.
{% endhint %}

# Named tuples

Codon also supports pass-by-value types via the `@tuple` annotation, which are
effectively named tuples (equivalent to Python's `collections.namedtuple`):

``` python
@tuple
class Point:
    x: int
    y: int

p = Point(1, 2)
q = p  # this is a copy!
print((p.x, p.y), (q.x, q.y))  # (1, 2), (1, 2)
```

However, named tuples are immutable. The following code will not compile:

``` python
p = Point(1, 2)
p.x = 2  # error: immutable type
```

You can also add methods to named tuples:

``` python
@tuple
class Point:
    x: int
    y: int

    def __new__():          # named tuples are constructed via __new__, not __init__
        return Point(0, 1)

    def some_method(self):
        return self.x + self.y

p = Point()             # p is (0, 1)
print(p.some_method())  # 1
```

# Type extensions

Suppose you have a class that lacks a method or an operator that might
be really useful. Codon provides an `@extend` annotation that allows
programmers to add and modify methods of various types at compile time,
including built-in types like `int` or `str`. This actually allows much
of the functionality of built-in types to be implemented in Codon as type
extensions in the standard library.

``` python
class Foo:
    ...

f = Foo(...)

# We need foo.cool() but it does not exist... not a problem for Codon
@extend
class Foo:
    def cool(self: Foo):
        ...

f.cool()  # works!

# Let's add support for adding integers and strings:
@extend
class int:
    def __add__(self: int, other: str):
        return self + int(other)

print(5 + '4')  # 9
```

Note that all type extensions are performed strictly at compile time and
incur no runtime overhead.

{% hint style="warning" %}
Type extensions in Codon are also a beta feature.
{% endhint %}


# Magic methods

Here is a list of useful magic methods that you might want to add and
overload:

  | Magic method  | Description                                                                         |
  |---------------|-------------------------------------------------------------------------------------|
  | `__copy__`    | copy-constructor for `copy` method                                                  |
  | `__len__`     | for `len` method                                                                    |
  | `__bool__`    | for `bool` method and condition checking                                            |
  | `__getitem__` | overload `obj[key]`                                                                 |
  | `__setitem__` | overload `obj[key] = value`                                                         |
  | `__delitem__` | overload `del obj[key]`                                                             |
  | `__iter__`    | support iterating over the object                                                   |
  | `__repr__`    | support printing and `str` conversion                                               |
