# Functions and generators

Functions in Codon use Python's `def` syntax, with positional arguments, keyword arguments and default values. Their bodies are compiled for the types used by the program; annotations can describe an interface, while unannotated parameters allow the compiler to infer the appropriate specializations.

## Parameters and results

This function scales a list of measurements. Its annotations describe the input and output element types, while the default factor keeps the common case convenient:

```python
def scale(values: list[float], factor: float = 2.0) -> list[float]:
    return [value * factor for value in values]

print(scale([1.0, 2.5]))              # [2.0, 5.0]
print(scale([1.0, 2.5], factor=3.0))  # [3.0, 7.5]
```

Removing the annotations would allow the same definition to specialize for other suitable input types. The [generics guide](https://docs.exaloop.io/language/generics/) covers that model and shows how to relate an argument's type to the elements of a collection.

A function can return several values as a tuple, which callers can unpack into separate names:

```python
def bounds(values):
    return min(values), max(values)

lower, upper = bounds([8, 3, 12])
print(lower, upper)  # 3 12
```

The number of returned tuple fields is fixed, although their values are computed at runtime. A function with no explicit return value produces `None`; when a meaningful result may be absent, an [optional return type](https://docs.exaloop.io/language/generics/#values-that-may-be-absent) can make that possibility explicit.

## Passing functions

Functions and lambdas can be passed to other functions, allowing a shared iteration pattern to work with different operations:

```python
def transform(values, operation):
    return [operation(value) for value in values]

def magnitude(value: int) -> int:
    return abs(value)

print(transform([-2, 3], magnitude))
# [2, 3]
print(transform([-2, 3], lambda value: value * value))
# [4, 9]
```

Here, the compiler specializes `transform()` for the supplied callable and input collection. Passing an operation explicitly is often useful when the same traversal needs different filtering, formatting or numerical behavior.

## Overloading

An ordinary second definition of a function replaces the first. Adding `@overload` instead keeps the earlier definition available and lets Codon select an implementation from the arguments' types:

```python
def render(value):
    return str(value)

@overload
def render(value: float):
    return f"{value:.2f}"

print(render(3.14159))  # 3.14
print(render("ready"))  # ready
```

Resolution proceeds from the most recent definition toward earlier ones, using the first applicable overload. Placing a general implementation before more specialized ones therefore provides a fallback without hiding them. Class methods support [overloading](https://docs.exaloop.io/language/classes/#overloading-methods) without requiring the decorator on each definition.

## Generators

A function containing `yield` produces a generator. Its body supplies values as the caller requests them, which makes generators useful for processing a sequence without first building another complete list:

```python
def nonblank(lines):
    for line in lines:
        text = line.strip()
        if text:
            yield text

records = nonblank([" oak ", "", "pine"])
print(list(records))  # ['oak', 'pine']
print(list(records))  # []
```

A generator is consumed as it is iterated; constructing a new one is necessary to repeat the traversal. Converting it to a list, as above, materializes the remaining values. A `for` loop can instead consume them incrementally, and a generator expression offers a compact form for simple transformations:

```python
values = [1, 2, 3, 4]
total = sum(value * value for value in values if value % 2 == 0)
print(total)  # 20
```

When iteration depends on an open file or another resource, that resource needs to remain available until iteration finishes. The [exceptions and resources guide](https://docs.exaloop.io/language/errors/) shows how a `with` block can keep file processing and cleanup together.

Source: https://docs.exaloop.io/language/functions/
