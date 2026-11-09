# sortedlist

Standard library module [View source](https://github.com/exaloop/codon/blob/master/stdlib/sortedlist.codon)

variable DEFAULT\_LOAD\_FACTOR[](https://docs.exaloop.io/libraries/api/modules/sortedlist/#variable-default_load_factor "Copy/link to this API element")

```python
DEFAULT_LOAD_FACTOR = 1000
```

class SortedList[](https://docs.exaloop.io/libraries/api/modules/sortedlist/#class-sortedlist "Copy/link to this API element")

```python
class SortedList[T]
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/sortedlist/SortedList/)

Fields 1

`T` `type`

Constructors 1

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/sortedlist/#class-sortedlist-__init__ "Copy/link to this API element")

```python
__init__(self: Self, Self: type)
```

Properties 1

left[](https://docs.exaloop.io/libraries/api/modules/sortedlist/#class-sortedlist-left "Copy/link to this API element")

@property

```python
left(self: Self, Self: type)
```

Methods 2

clear[](https://docs.exaloop.io/libraries/api/modules/sortedlist/#class-sortedlist-clear "Copy/link to this API element")

```python
clear(self: Self, Self: type)
```

Remove all values from sorted list. Runtime complexity: $O(n)$

add[](https://docs.exaloop.io/libraries/api/modules/sortedlist/#class-sortedlist-add "Copy/link to this API element")

```python
add(self: Self, value: T, Self: type)
```

Add `value` to sorted list. Runtime complexity: $O(\\log(n))$ (approximate).

```python
>>> sl = SortedList()
>>> sl.add(3)
>>> sl.add(1)
>>> sl.add(2)
>>> sl
SortedList([1, 2, 3])
```

Special methods 3

\_\_iter\_\_[](https://docs.exaloop.io/libraries/api/modules/sortedlist/#class-sortedlist-__iter__ "Copy/link to this API element")

```python
__iter__(self: Self, Self: type)
```

\_\_len\_\_[](https://docs.exaloop.io/libraries/api/modules/sortedlist/#class-sortedlist-__len__ "Copy/link to this API element")

```python
__len__(self: Self, Self: type)
```

\_\_bool\_\_[](https://docs.exaloop.io/libraries/api/modules/sortedlist/#class-sortedlist-__bool__ "Copy/link to this API element")

```python
__bool__(self: Self, Self: type)
```

Source: https://docs.exaloop.io/libraries/api/modules/sortedlist/
