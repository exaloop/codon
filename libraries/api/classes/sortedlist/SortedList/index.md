# SortedList

Standard library class `sortedlist.SortedList` [View source](https://github.com/exaloop/codon/blob/master/stdlib/sortedlist.codon)

class SortedList[](https://docs.exaloop.io/libraries/api/classes/sortedlist/SortedList/#class-sortedlist "Copy/link to this API element")

```python
class SortedList[T]
```

Fields 1

`T` `type`

Constructors 1

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/classes/sortedlist/SortedList/#class-sortedlist-__init__ "Copy/link to this API element")

```python
__init__(self: Self, Self: type)
```

Defined in [`sortedlist`](https://docs.exaloop.io/libraries/api/modules/sortedlist/)

Properties 1

left[](https://docs.exaloop.io/libraries/api/classes/sortedlist/SortedList/#class-sortedlist-left "Copy/link to this API element")

@property

```python
left(self: Self, Self: type)
```

Defined in [`sortedlist`](https://docs.exaloop.io/libraries/api/modules/sortedlist/)

Methods 2

clear[](https://docs.exaloop.io/libraries/api/classes/sortedlist/SortedList/#class-sortedlist-clear "Copy/link to this API element")

```python
clear(self: Self, Self: type)
```

Defined in [`sortedlist`](https://docs.exaloop.io/libraries/api/modules/sortedlist/)

Remove all values from sorted list. Runtime complexity: $O(n)$

add[](https://docs.exaloop.io/libraries/api/classes/sortedlist/SortedList/#class-sortedlist-add "Copy/link to this API element")

```python
add(self: Self, value: T, Self: type)
```

Defined in [`sortedlist`](https://docs.exaloop.io/libraries/api/modules/sortedlist/)

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

\_\_iter\_\_[](https://docs.exaloop.io/libraries/api/classes/sortedlist/SortedList/#class-sortedlist-__iter__ "Copy/link to this API element")

```python
__iter__(self: Self, Self: type)
```

Defined in [`sortedlist`](https://docs.exaloop.io/libraries/api/modules/sortedlist/)

\_\_len\_\_[](https://docs.exaloop.io/libraries/api/classes/sortedlist/SortedList/#class-sortedlist-__len__ "Copy/link to this API element")

```python
__len__(self: Self, Self: type)
```

Defined in [`sortedlist`](https://docs.exaloop.io/libraries/api/modules/sortedlist/)

\_\_bool\_\_[](https://docs.exaloop.io/libraries/api/classes/sortedlist/SortedList/#class-sortedlist-__bool__ "Copy/link to this API element")

```python
__bool__(self: Self, Self: type)
```

Defined in [`sortedlist`](https://docs.exaloop.io/libraries/api/modules/sortedlist/)

Source: https://docs.exaloop.io/libraries/api/classes/sortedlist/SortedList/
