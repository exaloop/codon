Codon supports Python classes as you would expect. For example:

``` python
class Point:
    def __init__(self, x, y):
        self.x = x
        self.y = y

    def __str__(self):
        return f'({self.x}, {self.y})'

p = Point(3, 4)
print(p)  # (3, 4)
```

Codon will automatically infer class fields if none are
specified explicitly. Alternatively, the class fields can
be specified in the class body:

``` python
class Point:
    x: int
    y: int

    def __init__(self, x, y):
        self.x = x
        self.y = y

    def __str__(self):
        return f'({self.x}, {self.y})'
```

Class fields can reference the enclosing class through
the `Optional` type:

``` python
class Point:
    x: int
    y: int
    other: Optional[Point]

    def __init__(self, x, y, other: Optional[Point] = None):
        self.x = x
        self.y = y
        self.other = other

    def __str__(self):
        if self.other is None:
            return f'({self.x}, {self.y})'
        else:
            return f'({self.x}, {self.y}) -> {str(self.other)}'

p = Point(3, 4)
print(p)  # (3, 4)

q = Point(5, 6, p)
print(q)  # (5, 6) -> (3, 4)
```

## Overloading methods

In Python, class methods can be defined to take arguments of arbitrary types,
and to reason about them through functions like `isinstance()`. While the same
works in Codon, Codon also offers another way to separate out method logic
for different input types: *method overloading*.

Multiple methods with the same name but different arguments or argument types
can be defined in the same class. Codon will use the method corresponding to
the argument types provided in a given call of that method. For example:

``` python
class Point:
    ...

    def foo(self, n: int):
        print('int-foo called!', n)

    def foo(self, s: str):
        print('str-foo called!', s)

p = Point(3, 4)
p.foo(42)     # int-foo called! 42
p.foo('abc')  # str-foo called! abc
```

Method resolution occurs *bottom-up*, meaning if multiple methods are applicable
for a given set of arguments, the latest one will be used.

!!! note

    Non-method functions can also be overloaded in Codon by adding the `@overload`
    decorator, which prevents latter definitions from shadowing previous ones.

## Tuple classes

Regular classes are mutable and passed around by reference. Internally,
class data is dynamically allocated and a pointer to the allocated data
is used to represent the class instance.

Codon supports an alternative type of class that is immutable and avoids
heap allocation: *tuple classes*. A tuple class is defined via the `@tuple`
class annotation. For example, we can rewrite the `Point` class above as
a tuple class:

``` python
@tuple
class Point:
    x: int
    y: int

    def __str__(self):
        return f'({self.x}, {self.y})'
```

Because tuple class instances are immutable, tuple classes do not use
the usual `__init__` method, and can instead define new constructors via
the `__new__` method. A default `__new__` which takes all of the tuple
class's fields as arguments is automatically generated. We can define
additional `__new__` methods as follows, for instance:

``` python
@tuple
class Point:
    x: int
    y: int

    # constructor (A)
    def __new__():
        return Point(0, 0)

    # constructor (B)
    def __new__(x: int):
        return Point(x, 0)

    def __str__(self):
        return f'({self.x}, {self.y})'

zero = Point()  # calls constructor (A)
one = Point(1)  # calls constructor (B)

print(zero)  # (0, 0)
print(one)   # (1, 0)

zero.x = 1  # error: cannot modify tuple attributes
```

Tuple classes can be more efficient than standard classes, particularly
when storing many instances in an array or list.

Internally, tuple classes correspond to C `struct`s. For example, the `Point`
tuple class above would correspond exactly to the following `struct` definition
in C:

``` c
struct Point {
  int64_t x;
  int64_t y;
};
```

As a result, tuple classes can also be used when interoperating with a C API,
as they can mirror API-specific data structures or layouts.

## Inheritance

Codon supports Python's inheritance and dynamic polymorphism. For example:

``` python
class Shape:

    def area(self):
        return 0.0

    def describe(self):
        return "This is a shape."

class Circle(Shape):
    radius: float

    def __init__(self, radius):
        self.radius = radius

    def area(self):
        return 3.1416 * self.radius**2

    def describe(self):
        return f"A circle with radius {self.radius}"

class Rectangle(Shape):
    width: float
    height: float

    def __init__(self, width, height):
        self.width = width
        self.height = height

    def area(self):
        return self.width * self.height

    def describe(self):
        return f"A rectangle with width {self.width} and height {self.height}"

class Square(Rectangle):

    def __init__(self, width):
        super().__init__(width, width)

    def describe(self):
        return super().describe().replace('rectangle', 'square')


shapes: list[Shape] = []
shapes.append(Circle(5))
shapes.append(Rectangle(4, 6))
shapes.append(Square(3))

for shape in shapes:
    print(shape.describe(), f'(area={shape.area()})')
```

!!! warning

    Tuple classes cannot be subclassed using standard inheritance. However, they can
    be subclassed via static inheritance, as described below.

In the code above, the methods `area()` and `describe()` are overriden by the subclasses
of `Square`. Codon follows Python's semantics and method resolution order.

### Static inheritance

In addition to Python's dynamic inheritance, Codon supports static inheritance
(or *early binding*), which can be expressed via the special `Static` type:

``` python
class Foo:
    x: int

    def __init__(self, x: int):
        self.x = x

    def hello(self):
        print('Foo')

class Bar(Static[Foo]):

    def hello(self):
        print('Bar')

foo = Foo(1)
bar = Bar(2)

print(foo.x, bar.x)  # 1 2
foo.hello()          # Foo
bar.hello()          # Bar
```

The `hello()` method calls are resolved at compile time instead of at runtime, as would be the
case with standard, dynamic inheritance. Static inheritance is useful when you want to reuse a
particular class's functionality without paying the cost of
[dynamic dispatch](https://en.wikipedia.org/wiki/Dynamic_dispatch) that is incurred with dynamic
inheritance.

Static inheritance also works on tuple classes:

``` python
@tuple
class Foo:
    x: int

    def hello(self):
        print('Foo')

@tuple
class Bar(Static[Foo]):

    def hello(self):
        print('Bar')

foo = Foo(1)
bar = Bar(2)

print(foo.x, bar.x)  # 1 2
foo.hello()          # Foo
bar.hello()          # Bar
```

### Exceptions

Subclasses of exception classes like `Exception`, `ValueError`, etc. must inherit from `Exception`
in order to be thrown and caught. Furthermore, when calling their parent class's constructor, exception
subclasses must supply their type name as the first argument. Here is an example:

``` python
class MyException(Exception):
    x: int

    def __init__(self, x: int):
        super().__init__('MyException', 'my exception message')
        self.x = x

try:
    raise MyException(42)
except MyException as e:
    print('caught:', str(e), e.x)  # caught: my exception message 42
```
