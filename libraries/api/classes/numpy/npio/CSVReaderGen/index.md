# CSVReaderGen

Standard library class `numpy.npio.CSVReaderGen` [View source](https://github.com/exaloop/codon/blob/master/stdlib/numpy/npio.codon)

class CSVReaderGen[](https://docs.exaloop.io/libraries/api/classes/numpy/npio/CSVReaderGen/#class-csvreadergen "Copy/link to this API element")

```python
class CSVReaderGen
```

Constructors 1

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/npio/CSVReaderGen/#class-csvreadergen-__init__ "Copy/link to this API element")

```python
__init__(self: Self, path: str, delimiter: str = ",", comments: str = "", Self: type)
```

Defined in [`numpy.npio`](https://docs.exaloop.io/libraries/api/modules/numpy/npio/)

Methods 21

fix\_names[](https://docs.exaloop.io/libraries/api/classes/numpy/npio/CSVReaderGen/#class-csvreadergen-fix_names "Copy/link to this API element")

```python
fix_names(self: Self, excludelist: List[str], deletechars: str, replace_space: str, upper: bool, lower: bool, Self: type)
```

Defined in [`numpy.npio`](https://docs.exaloop.io/libraries/api/modules/numpy/npio/)

is\_delimiter[](https://docs.exaloop.io/libraries/api/classes/numpy/npio/CSVReaderGen/#class-csvreadergen-is_delimiter "Copy/link to this API element")

```python
is_delimiter(self: Self, c, Self: type)
```

Defined in [`numpy.npio`](https://docs.exaloop.io/libraries/api/modules/numpy/npio/)

is\_whitespace\_delimiter[](https://docs.exaloop.io/libraries/api/classes/numpy/npio/CSVReaderGen/#class-csvreadergen-is_whitespace_delimiter "Copy/link to this API element")

```python
is_whitespace_delimiter(self: Self, Self: type)
```

Defined in [`numpy.npio`](https://docs.exaloop.io/libraries/api/modules/numpy/npio/)

is\_comment[](https://docs.exaloop.io/libraries/api/classes/numpy/npio/CSVReaderGen/#class-csvreadergen-is_comment "Copy/link to this API element")

```python
is_comment(self: Self, c, Self: type)
```

Defined in [`numpy.npio`](https://docs.exaloop.io/libraries/api/modules/numpy/npio/)

skip\_lines[](https://docs.exaloop.io/libraries/api/classes/numpy/npio/CSVReaderGen/#class-csvreadergen-skip_lines "Copy/link to this API element")

```python
skip_lines(self: Self, i: int, skip: int, Self: type)
```

Defined in [`numpy.npio`](https://docs.exaloop.io/libraries/api/modules/numpy/npio/)

find\_length[](https://docs.exaloop.io/libraries/api/classes/numpy/npio/CSVReaderGen/#class-csvreadergen-find_length "Copy/link to this API element")

```python
find_length(self: Self, skip_footer: int, Self: type)
```

Defined in [`numpy.npio`](https://docs.exaloop.io/libraries/api/modules/numpy/npio/)

skip\_delimiter[](https://docs.exaloop.io/libraries/api/classes/numpy/npio/CSVReaderGen/#class-csvreadergen-skip_delimiter "Copy/link to this API element")

```python
skip_delimiter(self: Self, i: int, Self: type)
```

Defined in [`numpy.npio`](https://docs.exaloop.io/libraries/api/modules/numpy/npio/)

```python
skip_delimiter(self: Self, i: int, line: str, Self: type)
```

Defined in [`numpy.npio`](https://docs.exaloop.io/libraries/api/modules/numpy/npio/)

skip\_past\_line[](https://docs.exaloop.io/libraries/api/classes/numpy/npio/CSVReaderGen/#class-csvreadergen-skip_past_line "Copy/link to this API element")

```python
skip_past_line(self: Self, i: int, Self: type)
```

Defined in [`numpy.npio`](https://docs.exaloop.io/libraries/api/modules/numpy/npio/)

skip\_comments\_no\_whitespace[](https://docs.exaloop.io/libraries/api/classes/numpy/npio/CSVReaderGen/#class-csvreadergen-skip_comments_no_whitespace "Copy/link to this API element")

```python
skip_comments_no_whitespace(self: Self, i: int, Self: type)
```

Defined in [`numpy.npio`](https://docs.exaloop.io/libraries/api/modules/numpy/npio/)

skip\_comments[](https://docs.exaloop.io/libraries/api/classes/numpy/npio/CSVReaderGen/#class-csvreadergen-skip_comments "Copy/link to this API element")

```python
skip_comments(self: Self, i: int, Self: type)
```

Defined in [`numpy.npio`](https://docs.exaloop.io/libraries/api/modules/numpy/npio/)

is\_comment\_or\_blank\_line[](https://docs.exaloop.io/libraries/api/classes/numpy/npio/CSVReaderGen/#class-csvreadergen-is_comment_or_blank_line "Copy/link to this API element")

```python
is_comment_or_blank_line(self: Self, line: str, Self: type)
```

Defined in [`numpy.npio`](https://docs.exaloop.io/libraries/api/modules/numpy/npio/)

get\_num\_fields[](https://docs.exaloop.io/libraries/api/classes/numpy/npio/CSVReaderGen/#class-csvreadergen-get_num_fields "Copy/link to this API element")

```python
get_num_fields(self: Self, i: int, get_names: bool, Self: type)
```

Defined in [`numpy.npio`](https://docs.exaloop.io/libraries/api/modules/numpy/npio/)

partition\_line[](https://docs.exaloop.io/libraries/api/classes/numpy/npio/CSVReaderGen/#class-csvreadergen-partition_line "Copy/link to this API element")

```python
partition_line(self: Self, line: str, delimiter, num_fields: int, row: int, invalid_raise: bool, Self: type)
```

Defined in [`numpy.npio`](https://docs.exaloop.io/libraries/api/modules/numpy/npio/)

get\_num\_fields\_spaced[](https://docs.exaloop.io/libraries/api/classes/numpy/npio/CSVReaderGen/#class-csvreadergen-get_num_fields_spaced "Copy/link to this API element")

```python
get_num_fields_spaced(self: Self, i: int, get_names: bool, delimiter, Self: type)
```

Defined in [`numpy.npio`](https://docs.exaloop.io/libraries/api/modules/numpy/npio/)

get\_num\_fields\_single[](https://docs.exaloop.io/libraries/api/classes/numpy/npio/CSVReaderGen/#class-csvreadergen-get_num_fields_single "Copy/link to this API element")

```python
get_num_fields_single(self: Self, line: str, get_names: bool, Self: type)
```

Defined in [`numpy.npio`](https://docs.exaloop.io/libraries/api/modules/numpy/npio/)

get\_num\_fields\_single\_spaced[](https://docs.exaloop.io/libraries/api/classes/numpy/npio/CSVReaderGen/#class-csvreadergen-get_num_fields_single_spaced "Copy/link to this API element")

```python
get_num_fields_single_spaced(self: Self, line: str, get_names: bool, delimiter, Self: type)
```

Defined in [`numpy.npio`](https://docs.exaloop.io/libraries/api/modules/numpy/npio/)

translate\_cols[](https://docs.exaloop.io/libraries/api/classes/numpy/npio/CSVReaderGen/#class-csvreadergen-translate_cols "Copy/link to this API element")

```python
translate_cols(self: Self, usecols, num_fields: int, Self: type)
```

Defined in [`numpy.npio`](https://docs.exaloop.io/libraries/api/modules/numpy/npio/)

parse[](https://docs.exaloop.io/libraries/api/classes/numpy/npio/CSVReaderGen/#class-csvreadergen-parse "Copy/link to this API element")

```python
parse(self: Self, i: int, row_callback, num_fields: int, maxrows: int, invalid_raise: bool, Self: type)
```

Defined in [`numpy.npio`](https://docs.exaloop.io/libraries/api/modules/numpy/npio/)

parse\_spaced[](https://docs.exaloop.io/libraries/api/classes/numpy/npio/CSVReaderGen/#class-csvreadergen-parse_spaced "Copy/link to this API element")

```python
parse_spaced(self: Self, i: int, row_callback, num_fields: int, maxrows: int, invalid_raise: bool, delimiter, Self: type)
```

Defined in [`numpy.npio`](https://docs.exaloop.io/libraries/api/modules/numpy/npio/)

parse\_single[](https://docs.exaloop.io/libraries/api/classes/numpy/npio/CSVReaderGen/#class-csvreadergen-parse_single "Copy/link to this API element")

```python
parse_single(self: Self, line: str, row_callback, row: int, fields: Ptr[str], num_fields: int, invalid_raise: bool, Self: type)
```

Defined in [`numpy.npio`](https://docs.exaloop.io/libraries/api/modules/numpy/npio/)

parse\_single\_spaced[](https://docs.exaloop.io/libraries/api/classes/numpy/npio/CSVReaderGen/#class-csvreadergen-parse_single_spaced "Copy/link to this API element")

```python
parse_single_spaced(self: Self, line: str, row_callback, row: int, fields: Ptr[str], num_fields: int, invalid_raise: bool, delimiter, Self: type)
```

Defined in [`numpy.npio`](https://docs.exaloop.io/libraries/api/modules/numpy/npio/)

Special methods 2

\_\_enter\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/npio/CSVReaderGen/#class-csvreadergen-__enter__ "Copy/link to this API element")

```python
__enter__(self: Self, Self: type)
```

Defined in [`numpy.npio`](https://docs.exaloop.io/libraries/api/modules/numpy/npio/)

\_\_exit\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/npio/CSVReaderGen/#class-csvreadergen-__exit__ "Copy/link to this API element")

```python
__exit__(self: Self, Self: type)
```

Defined in [`numpy.npio`](https://docs.exaloop.io/libraries/api/modules/numpy/npio/)

Source: https://docs.exaloop.io/libraries/api/classes/numpy/npio/CSVReaderGen/
