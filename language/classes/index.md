# Classes

Classes group state and behavior using Python's familiar syntax. A regular class creates reference objects, so several names can refer to the same instance. For example, this class stores a point's coordinates and provides a readable string representation:

```python
class Point:
    def __init__(self, x, y):
        self.x = x
        self.y = y

    def __str__(self):
        return f'({self.x}, {self.y})'

p = Point(3, 4)
print(p)  # (3, 4)
```

Codon will automatically infer class fields if none are specified explicitly. Alternatively, the class fields can be specified in the class body:

```python
class Point:
    x: int
    y: int

    def __init__(self, x, y):
        self.x = x
        self.y = y

    def __str__(self):
        return f'({self.x}, {self.y})'
```

Class fields can reference the enclosing class through the `Optional` type:

```python
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

## Field initialization

Codon reports a compile-time error when a field may be uninitialized when `__init__` returns normally. The check follows calls to known helper functions and methods, including base constructors called through `super()`. Initialization can therefore be split across helpers:

```python
class Buffer:
    values: list[int]

    def __init__(self):
        self.initialize()

    def initialize(self):
        self.values = []
```

The analysis is conservative: assignments on only one branch do not guarantee initialization, and initialization that cannot be verified statically may still produce an error. The check applies to regular classes; tuple classes and compiler-internal classes are exempt. Tuple classes return complete tuple values from `__new__` rather than initializing fields through `__init__`.

The check verifies initialization rather than supplying default values or adding runtime guards. A field therefore needs a value before code reads it, even when its initialization is delegated to a helper.

### Custom allocation

Reference classes can define a static `__new__` method to control allocation or return an existing instance. Constructor arguments are evaluated once and passed to both the custom `__new__` and `__init__`. If `__new__` returns an unrelated type, `__init__` is skipped. Most classes only need `__init__`; custom allocation is useful when the ordinary creation of a new instance is not the desired behavior.

## Properties and class attributes

A property exposes a computed value through attribute syntax. A setter can translate an assignment back into the object's stored representation, keeping the conversion in one place:

```python
class Temperature:
    celsius: float

    def __init__(self, celsius: float):
        self.celsius = celsius

    @property
    def fahrenheit(self) -> float:
        return self.celsius * 9.0 / 5.0 + 32.0

    @fahrenheit.setter
    def fahrenheit(self, value: float):
        self.celsius = (value - 32.0) * 5.0 / 9.0

temperature = Temperature(100.0)
print(temperature.fahrenheit)  # 212.0
temperature.fahrenheit = 32.0
print(temperature.celsius)  # 0.0
```

An ordinary field belongs to each instance, whereas a `ClassVar` belongs to the class. For example, a shared counter can assign identifiers as objects are created:

```python
class Record:
    next_identifier: ClassVar[int] = 0
    identifier: int

    def __init__(self):
        self.identifier = Record.next_identifier
        Record.next_identifier += 1

first = Record()
second = Record()
print(first.identifier, second.identifier)  # 0 1
```

Here, each record keeps its own identifier while creation updates a common counter. As with other shared mutable state, concurrent updates require appropriate synchronization.

## Self

An unannotated instance-method argument named `self` has type `Self`. Codon binds `Self` to the receiver's static type, so an inherited method can use the derived type without upcasting to the class that defined the method:

```python
class A:
    def foo(self):
        return f"{type(self)} ({Self})"
class B(A):
    pass

print(A().foo())  # <class 'A'> (<class 'A'>)
print(B().foo())  # <class 'B'> (<class 'B'>)
```

Return types are inferred normally. A method can also explicitly annotate its return type as `Self` if needed. An explicit `self` annotation such as `self: Context` keeps that annotation and does not default to `Self`.

`Self` also works for tuple classes and can be declared explicitly as a generic parameter (`Self: type`), including in static methods. It does not itself make a method virtual.

## Overloading methods

In Python, class methods can be defined to take arguments of arbitrary types, and to reason about them through functions like `isinstance()`. While the same works in Codon, Codon also offers another way to separate out method logic for different input types: _method overloading_.

Multiple methods with the same name but different arguments or argument types can be defined in the same class. Codon will use the method corresponding to the argument types provided in a given call of that method. For example:

```python
class Point:
    def __init__(self, x: int, y: int):
        self.x = x
        self.y = y

    def distance(self, other: Point):
        return abs(self.x - other.x) + abs(self.y - other.y)

    def distance(self, x: int, y: int):
        return self.distance(Point(x, y))

p = Point(3, 4)
print(p.distance(Point(0, 0)))  # 7
print(p.distance(2, 2))        # 3
```

Both overloads calculate Manhattan distance. One accepts another point, while the other accepts coordinates and delegates to the first implementation. Method resolution occurs _bottom-up_, meaning that if multiple methods are applicable to a call, the latest definition is selected.

note

Non-method functions can also be overloaded in Codon by adding the `@overload` decorator, which prevents later definitions from shadowing previous ones. See [function overloading](https://docs.exaloop.io/language/functions/#overloading) for an example.

## Tuple classes

Regular classes are mutable and passed around by reference. Internally, class data is dynamically allocated and a pointer to the allocated data is used to represent the class instance.

Codon also supports _tuple classes_, whose fields form a value that can be stored inline rather than requiring a separately allocated object. Their fields cannot be reassigned after construction, making them useful for small records such as coordinates or measurements. The `@tuple` annotation selects this representation:

```python
@tuple
class Point:
    x: int
    y: int

    def __str__(self):
        return f'({self.x}, {self.y})'
```

Because tuple class instances are immutable, tuple classes do not use the usual `__init__` method, and can instead define new constructors via the `__new__` method. A default `__new__` which takes all of the tuple class's fields as arguments is automatically generated. We can define additional `__new__` methods as follows, for instance:

```python
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
```

Assigning `zero.x = 1` would be a compilation error because tuple fields are immutable. This restriction applies to the fields themselves: a list stored in a tuple field is still a mutable list, and copying the tuple does not deep-copy such referenced objects.

Tuple classes can be more efficient than reference classes when many small records are stored in an array or list. They do not participate in class inheritance, either as subclasses or as base classes. Composition, such as storing a `Point` in a field of another record, lets a larger value reuse its data and methods without an inheritance relationship.

Internally, tuple classes correspond to C `struct`s. For example, the `Point` tuple class above would correspond exactly to the following `struct` definition in C:

```c
struct Point {
  int64_t x;
  int64_t y;
};
```

As a result, tuple classes can also be used when interoperating with a C API, as they can mirror API-specific data structures or layouts.

## Inheritance

Codon supports Python's inheritance and dynamic polymorphism. For example:

```python
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

The annotation `list[Shape]` gives the collection a common element type while allowing it to hold instances of different subclasses. Calls to `area()` and `describe()` through that common type select the implementation for the actual object: `Circle` and `Rectangle` override both methods, while `Square` inherits `Rectangle.area()` and supplies its own `describe()`.

`super()` lets a subclass reuse the next implementation in Python's method resolution order. Here, `Square.__init__()` delegates field initialization to `Rectangle.__init__()`, and `Square.describe()` adapts the description returned by its parent. This is useful when a subclass changes one part of a behavior without duplicating the rest of its implementation.

For a collection such as this one, the base class describes the interface that all elements share. Members introduced only by a particular subclass, such as `Circle.radius`, are not part of that shared interface; an `isinstance()` check can narrow an element to the subclass before accessing them:

```python
for shape in shapes:
    if isinstance(shape, Circle):
        print(shape.radius)  # 5.0
```

## Enumerations

Use `Enum` to define named singleton values:

```python
from enum import Enum, auto

class Color(Enum):
    RED = auto()
    GREEN = auto()
    BLUE = auto()
    CRIMSON = RED

assert Color.RED.name == 'RED'
assert Color.RED.value == 1
assert Color(1) is Color.RED
assert Color['CRIMSON'] is Color.RED
assert list(Color) == [Color.RED, Color.GREEN, Color.BLUE]
```

Equal values become aliases of the first member. Iteration skips aliases and preserves declaration order; `Color.__members__` includes all declared names. Members support identity comparisons, equality, hashing, and use as dictionary keys. `Enum` subclasses may define methods and be nested inside other classes. Class iteration, subscription, length, and containment use ordinary static methods inherited from `Enum`, and `__members__` is a normal `ClassVar`. These follow Codon's usual attribute lookup and override rules. Calling an enum with an unknown value raises `ValueError`; an unknown member name raises `KeyError`. Existing members cannot be reassigned, and an enum with members cannot be subclassed to add more members.

Codon supports class-based `Enum` declarations with one native value type per enumeration, including integer and string values. `auto()` generates integer values starting at 1, or one above the largest preceding value. Custom `__init__`/`__new__` constructors, enum mixins, the functional `Enum(...)` factory, `IntEnum`, and flag types are not currently supported.

Source: https://docs.exaloop.io/language/classes/
