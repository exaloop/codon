# CSVReader

Standard library class `numpy.npio.CSVReader` [View source](https://github.com/exaloop/codon/blob/master/stdlib/numpy/npio.codon)

class CSVReader[](https://docs.exaloop.io/libraries/api/classes/numpy/npio/CSVReader/#class-csvreader "Copy/link to this API element")

```python
class CSVReader
```

Constructors 1

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/npio/CSVReader/#class-csvreader-__init__ "Copy/link to this API element")

```python
__init__(self: Self, path: str, delimiter: str = ",", quotechar: str = """, comments: str = "", Self: type)
```

Defined in [`numpy.npio`](https://docs.exaloop.io/libraries/api/modules/numpy/npio/)

Methods 9

is\_delimiter[](https://docs.exaloop.io/libraries/api/classes/numpy/npio/CSVReader/#class-csvreader-is_delimiter "Copy/link to this API element")

```python
is_delimiter(self: Self, c, Self: type)
```

Defined in [`numpy.npio`](https://docs.exaloop.io/libraries/api/modules/numpy/npio/)

is\_comment[](https://docs.exaloop.io/libraries/api/classes/numpy/npio/CSVReader/#class-csvreader-is_comment "Copy/link to this API element")

```python
is_comment(self: Self, c, Self: type)
```

Defined in [`numpy.npio`](https://docs.exaloop.io/libraries/api/modules/numpy/npio/)

is\_whitespace\_delimiter[](https://docs.exaloop.io/libraries/api/classes/numpy/npio/CSVReader/#class-csvreader-is_whitespace_delimiter "Copy/link to this API element")

```python
is_whitespace_delimiter(self: Self, Self: type)
```

Defined in [`numpy.npio`](https://docs.exaloop.io/libraries/api/modules/numpy/npio/)

skip\_past\_line[](https://docs.exaloop.io/libraries/api/classes/numpy/npio/CSVReader/#class-csvreader-skip_past_line "Copy/link to this API element")

```python
skip_past_line(self: Self, i: int, Self: type)
```

Defined in [`numpy.npio`](https://docs.exaloop.io/libraries/api/modules/numpy/npio/)

skip\_delimiter[](https://docs.exaloop.io/libraries/api/classes/numpy/npio/CSVReader/#class-csvreader-skip_delimiter "Copy/link to this API element")

```python
skip_delimiter(self: Self, i: int, Self: type)
```

Defined in [`numpy.npio`](https://docs.exaloop.io/libraries/api/modules/numpy/npio/)

skip\_comments[](https://docs.exaloop.io/libraries/api/classes/numpy/npio/CSVReader/#class-csvreader-skip_comments "Copy/link to this API element")

```python
skip_comments(self: Self, i: int, Self: type)
```

Defined in [`numpy.npio`](https://docs.exaloop.io/libraries/api/modules/numpy/npio/)

skip\_lines[](https://docs.exaloop.io/libraries/api/classes/numpy/npio/CSVReader/#class-csvreader-skip_lines "Copy/link to this API element")

```python
skip_lines(self: Self, i: int, skip: int, Self: type)
```

Defined in [`numpy.npio`](https://docs.exaloop.io/libraries/api/modules/numpy/npio/)

get\_num\_fields[](https://docs.exaloop.io/libraries/api/classes/numpy/npio/CSVReader/#class-csvreader-get_num_fields "Copy/link to this API element")

```python
get_num_fields(self: Self, i0: int, Self: type)
```

Defined in [`numpy.npio`](https://docs.exaloop.io/libraries/api/modules/numpy/npio/)

parse[](https://docs.exaloop.io/libraries/api/classes/numpy/npio/CSVReader/#class-csvreader-parse "Copy/link to this API element")

```python
parse(self: Self, row_callback, num_fields: int, maxrows: int, i0: int, Self: type)
```

Defined in [`numpy.npio`](https://docs.exaloop.io/libraries/api/modules/numpy/npio/)

Special methods 2

\_\_enter\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/npio/CSVReader/#class-csvreader-__enter__ "Copy/link to this API element")

```python
__enter__(self: Self, Self: type)
```

Defined in [`numpy.npio`](https://docs.exaloop.io/libraries/api/modules/numpy/npio/)

\_\_exit\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/npio/CSVReader/#class-csvreader-__exit__ "Copy/link to this API element")

```python
__exit__(self: Self, Self: type)
```

Defined in [`numpy.npio`](https://docs.exaloop.io/libraries/api/modules/numpy/npio/)

Source: https://docs.exaloop.io/libraries/api/classes/numpy/npio/CSVReader/
