# numpy.npio

Standard library module [View source](https://github.com/exaloop/codon/blob/master/stdlib/numpy/npio.codon)

function memmap[](https://docs.exaloop.io/libraries/api/modules/numpy/npio/#function-memmap "Copy/link to this API element")

```python
memmap(filename, dtype: type = u8, mode: str = "r+", offset: int = 0, shape = None, order: str = "C")
```

function parse\_header[](https://docs.exaloop.io/libraries/api/modules/numpy/npio/#function-parse_header "Copy/link to this API element")

```python
parse_header(header: str)
```

function load[](https://docs.exaloop.io/libraries/api/modules/numpy/npio/#function-load "Copy/link to this API element")

```python
load(file, mmap_mode: Optional[str] = None, ndim: Literal[int] = 1, dtype: type = float)
```

function save[](https://docs.exaloop.io/libraries/api/modules/numpy/npio/#function-save "Copy/link to this API element")

```python
save(file, arr)
```

function savetxt[](https://docs.exaloop.io/libraries/api/modules/numpy/npio/#function-savetxt "Copy/link to this API element")

```python
savetxt(fname, X, delimiter: str = " ", newline: str = "\n", header: str = "", footer: str = "", comments: str = "# ")
```

function fromfile[](https://docs.exaloop.io/libraries/api/modules/numpy/npio/#function-fromfile "Copy/link to this API element")

```python
fromfile(file, dtype: type = float, count: int = -1, sep: str = "", offset: int = 0)
```

function fromstring[](https://docs.exaloop.io/libraries/api/modules/numpy/npio/#function-fromstring "Copy/link to this API element")

```python
fromstring(string, dtype: type = float, count: int = -1, sep: str = "")
```

class ndarray[](https://docs.exaloop.io/libraries/api/modules/numpy/npio/#class-ndarray "Copy/link to this API element")

@extend

```python
class ndarray[dtype, ndim]
```

Extends [`numpy.ndarray.ndarray`](https://docs.exaloop.io/libraries/api/classes/numpy/ndarray/ndarray/) · [View complete class API →](https://docs.exaloop.io/libraries/api/classes/numpy/ndarray/ndarray/)

Methods 1

tofile[](https://docs.exaloop.io/libraries/api/modules/numpy/npio/#class-ndarray-tofile "Copy/link to this API element")

```python
tofile(self: Self, file, sep: str = "", Self: type)
```

class Converters[](https://docs.exaloop.io/libraries/api/modules/numpy/npio/#class-converters "Copy/link to this API element")

@tuple

```python
class Converters[dtype, F, M, U]
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/numpy/npio/Converters/)

Fields 7

`funcs` `F`

`mask` `M`

`usecols` `U`

`dtype` `type`

`F` `type`

`M` `type`

`U` `type`

Constructors 1

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/npio/#class-converters-__new__ "Copy/link to this API element")

```python
__new__(funcs, mask, usecols, dtype: type)
```

Special methods 1

\_\_call\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/npio/#class-converters-__call__ "Copy/link to this API element")

```python
__call__(self: Self, field: str, idx: int, Self: type)
```

function normalize\_col[](https://docs.exaloop.io/libraries/api/modules/numpy/npio/#function-normalize_col "Copy/link to this API element")

```python
normalize_col(col: int, num_fields: int)
```

function default\_fill[](https://docs.exaloop.io/libraries/api/modules/numpy/npio/#function-default_fill "Copy/link to this API element")

```python
default_fill(dtype: type)
```

function malformed[](https://docs.exaloop.io/libraries/api/modules/numpy/npio/#function-malformed "Copy/link to this API element")

```python
malformed(row: int, num_fields: int)
```

function min\_dim[](https://docs.exaloop.io/libraries/api/modules/numpy/npio/#function-min_dim "Copy/link to this API element")

```python
min_dim(arr: ndarray, ndmin: Literal[int])
```

function make\_conv[](https://docs.exaloop.io/libraries/api/modules/numpy/npio/#function-make_conv "Copy/link to this API element")

```python
make_conv(converters, num_fields: int, usecols, dtype: type)
```

class CSVReader[](https://docs.exaloop.io/libraries/api/modules/numpy/npio/#class-csvreader "Copy/link to this API element")

```python
class CSVReader
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/numpy/npio/CSVReader/)

Constructors 1

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/npio/#class-csvreader-__init__ "Copy/link to this API element")

```python
__init__(self: Self, path: str, delimiter: str = ",", quotechar: str = """, comments: str = "", Self: type)
```

Methods 9

is\_delimiter[](https://docs.exaloop.io/libraries/api/modules/numpy/npio/#class-csvreader-is_delimiter "Copy/link to this API element")

```python
is_delimiter(self: Self, c, Self: type)
```

is\_comment[](https://docs.exaloop.io/libraries/api/modules/numpy/npio/#class-csvreader-is_comment "Copy/link to this API element")

```python
is_comment(self: Self, c, Self: type)
```

is\_whitespace\_delimiter[](https://docs.exaloop.io/libraries/api/modules/numpy/npio/#class-csvreader-is_whitespace_delimiter "Copy/link to this API element")

```python
is_whitespace_delimiter(self: Self, Self: type)
```

skip\_past\_line[](https://docs.exaloop.io/libraries/api/modules/numpy/npio/#class-csvreader-skip_past_line "Copy/link to this API element")

```python
skip_past_line(self: Self, i: int, Self: type)
```

skip\_delimiter[](https://docs.exaloop.io/libraries/api/modules/numpy/npio/#class-csvreader-skip_delimiter "Copy/link to this API element")

```python
skip_delimiter(self: Self, i: int, Self: type)
```

skip\_comments[](https://docs.exaloop.io/libraries/api/modules/numpy/npio/#class-csvreader-skip_comments "Copy/link to this API element")

```python
skip_comments(self: Self, i: int, Self: type)
```

skip\_lines[](https://docs.exaloop.io/libraries/api/modules/numpy/npio/#class-csvreader-skip_lines "Copy/link to this API element")

```python
skip_lines(self: Self, i: int, skip: int, Self: type)
```

get\_num\_fields[](https://docs.exaloop.io/libraries/api/modules/numpy/npio/#class-csvreader-get_num_fields "Copy/link to this API element")

```python
get_num_fields(self: Self, i0: int, Self: type)
```

parse[](https://docs.exaloop.io/libraries/api/modules/numpy/npio/#class-csvreader-parse "Copy/link to this API element")

```python
parse(self: Self, row_callback, num_fields: int, maxrows: int, i0: int, Self: type)
```

Special methods 2

\_\_enter\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/npio/#class-csvreader-__enter__ "Copy/link to this API element")

```python
__enter__(self: Self, Self: type)
```

\_\_exit\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/npio/#class-csvreader-__exit__ "Copy/link to this API element")

```python
__exit__(self: Self, Self: type)
```

class ArrayUpdate[](https://docs.exaloop.io/libraries/api/modules/numpy/npio/#class-arrayupdate "Copy/link to this API element")

```python
class ArrayUpdate[A, C, F]
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/numpy/npio/ArrayUpdate/)

Fields 7

`arr` `A`

`cols` `C`

`conv` `F`

`last_row` `int`

`A` `type`

`C` `type`

`F` `type`

Constructors 1

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/npio/#class-arrayupdate-__init__ "Copy/link to this API element")

```python
__init__(self: Self, arr: A, cols: C, conv: F, Self: type)
```

Properties 1

cap[](https://docs.exaloop.io/libraries/api/modules/numpy/npio/#class-arrayupdate-cap "Copy/link to this API element")

@property

```python
cap(self: Self, Self: type)
```

Methods 4

resize\_arrays[](https://docs.exaloop.io/libraries/api/modules/numpy/npio/#class-arrayupdate-resize_arrays "Copy/link to this API element")

```python
resize_arrays(self: Self, new_cap: int, Self: type)
```

trim\_arrays[](https://docs.exaloop.io/libraries/api/modules/numpy/npio/#class-arrayupdate-trim_arrays "Copy/link to this API element")

```python
trim_arrays(self: Self, Self: type)
```

convert[](https://docs.exaloop.io/libraries/api/modules/numpy/npio/#class-arrayupdate-convert "Copy/link to this API element")

```python
convert(self: Self, field: str, idx: int, dtype: type, Self: type)
```

convert\_static[](https://docs.exaloop.io/libraries/api/modules/numpy/npio/#class-arrayupdate-convert_static "Copy/link to this API element")

```python
convert_static(self: Self, field: str, idx: Literal[int], dtype: type, Self: type)
```

Special methods 1

\_\_call\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/npio/#class-arrayupdate-__call__ "Copy/link to this API element")

```python
__call__(self: Self, fields: Ptr[str], num_fields: int, row: int, Self: type)
```

function loadtxt[](https://docs.exaloop.io/libraries/api/modules/numpy/npio/#function-loadtxt "Copy/link to this API element")

```python
loadtxt(fname: str, dtype: type = float, comments = "#", delimiter = None, converters = None, skiprows: int = 0, usecols = None, unpack: Literal[bool] = False, ndmin: Literal[int] = 0, max_rows: Optional[int] = None, quotechar: Optional[str] = None)
```

class CSVReaderGen[](https://docs.exaloop.io/libraries/api/modules/numpy/npio/#class-csvreadergen "Copy/link to this API element")

```python
class CSVReaderGen
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/numpy/npio/CSVReaderGen/)

Constructors 1

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/npio/#class-csvreadergen-__init__ "Copy/link to this API element")

```python
__init__(self: Self, path: str, delimiter: str = ",", comments: str = "", Self: type)
```

Methods 21

fix\_names[](https://docs.exaloop.io/libraries/api/modules/numpy/npio/#class-csvreadergen-fix_names "Copy/link to this API element")

```python
fix_names(self: Self, excludelist: List[str], deletechars: str, replace_space: str, upper: bool, lower: bool, Self: type)
```

is\_delimiter[](https://docs.exaloop.io/libraries/api/modules/numpy/npio/#class-csvreadergen-is_delimiter "Copy/link to this API element")

```python
is_delimiter(self: Self, c, Self: type)
```

is\_whitespace\_delimiter[](https://docs.exaloop.io/libraries/api/modules/numpy/npio/#class-csvreadergen-is_whitespace_delimiter "Copy/link to this API element")

```python
is_whitespace_delimiter(self: Self, Self: type)
```

is\_comment[](https://docs.exaloop.io/libraries/api/modules/numpy/npio/#class-csvreadergen-is_comment "Copy/link to this API element")

```python
is_comment(self: Self, c, Self: type)
```

skip\_lines[](https://docs.exaloop.io/libraries/api/modules/numpy/npio/#class-csvreadergen-skip_lines "Copy/link to this API element")

```python
skip_lines(self: Self, i: int, skip: int, Self: type)
```

find\_length[](https://docs.exaloop.io/libraries/api/modules/numpy/npio/#class-csvreadergen-find_length "Copy/link to this API element")

```python
find_length(self: Self, skip_footer: int, Self: type)
```

skip\_delimiter[](https://docs.exaloop.io/libraries/api/modules/numpy/npio/#class-csvreadergen-skip_delimiter "Copy/link to this API element")

```python
skip_delimiter(self: Self, i: int, Self: type)
```

```python
skip_delimiter(self: Self, i: int, line: str, Self: type)
```

skip\_past\_line[](https://docs.exaloop.io/libraries/api/modules/numpy/npio/#class-csvreadergen-skip_past_line "Copy/link to this API element")

```python
skip_past_line(self: Self, i: int, Self: type)
```

skip\_comments\_no\_whitespace[](https://docs.exaloop.io/libraries/api/modules/numpy/npio/#class-csvreadergen-skip_comments_no_whitespace "Copy/link to this API element")

```python
skip_comments_no_whitespace(self: Self, i: int, Self: type)
```

skip\_comments[](https://docs.exaloop.io/libraries/api/modules/numpy/npio/#class-csvreadergen-skip_comments "Copy/link to this API element")

```python
skip_comments(self: Self, i: int, Self: type)
```

is\_comment\_or\_blank\_line[](https://docs.exaloop.io/libraries/api/modules/numpy/npio/#class-csvreadergen-is_comment_or_blank_line "Copy/link to this API element")

```python
is_comment_or_blank_line(self: Self, line: str, Self: type)
```

get\_num\_fields[](https://docs.exaloop.io/libraries/api/modules/numpy/npio/#class-csvreadergen-get_num_fields "Copy/link to this API element")

```python
get_num_fields(self: Self, i: int, get_names: bool, Self: type)
```

partition\_line[](https://docs.exaloop.io/libraries/api/modules/numpy/npio/#class-csvreadergen-partition_line "Copy/link to this API element")

```python
partition_line(self: Self, line: str, delimiter, num_fields: int, row: int, invalid_raise: bool, Self: type)
```

get\_num\_fields\_spaced[](https://docs.exaloop.io/libraries/api/modules/numpy/npio/#class-csvreadergen-get_num_fields_spaced "Copy/link to this API element")

```python
get_num_fields_spaced(self: Self, i: int, get_names: bool, delimiter, Self: type)
```

get\_num\_fields\_single[](https://docs.exaloop.io/libraries/api/modules/numpy/npio/#class-csvreadergen-get_num_fields_single "Copy/link to this API element")

```python
get_num_fields_single(self: Self, line: str, get_names: bool, Self: type)
```

get\_num\_fields\_single\_spaced[](https://docs.exaloop.io/libraries/api/modules/numpy/npio/#class-csvreadergen-get_num_fields_single_spaced "Copy/link to this API element")

```python
get_num_fields_single_spaced(self: Self, line: str, get_names: bool, delimiter, Self: type)
```

translate\_cols[](https://docs.exaloop.io/libraries/api/modules/numpy/npio/#class-csvreadergen-translate_cols "Copy/link to this API element")

```python
translate_cols(self: Self, usecols, num_fields: int, Self: type)
```

parse[](https://docs.exaloop.io/libraries/api/modules/numpy/npio/#class-csvreadergen-parse "Copy/link to this API element")

```python
parse(self: Self, i: int, row_callback, num_fields: int, maxrows: int, invalid_raise: bool, Self: type)
```

parse\_spaced[](https://docs.exaloop.io/libraries/api/modules/numpy/npio/#class-csvreadergen-parse_spaced "Copy/link to this API element")

```python
parse_spaced(self: Self, i: int, row_callback, num_fields: int, maxrows: int, invalid_raise: bool, delimiter, Self: type)
```

parse\_single[](https://docs.exaloop.io/libraries/api/modules/numpy/npio/#class-csvreadergen-parse_single "Copy/link to this API element")

```python
parse_single(self: Self, line: str, row_callback, row: int, fields: Ptr[str], num_fields: int, invalid_raise: bool, Self: type)
```

parse\_single\_spaced[](https://docs.exaloop.io/libraries/api/modules/numpy/npio/#class-csvreadergen-parse_single_spaced "Copy/link to this API element")

```python
parse_single_spaced(self: Self, line: str, row_callback, row: int, fields: Ptr[str], num_fields: int, invalid_raise: bool, delimiter, Self: type)
```

Special methods 2

\_\_enter\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/npio/#class-csvreadergen-__enter__ "Copy/link to this API element")

```python
__enter__(self: Self, Self: type)
```

\_\_exit\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/npio/#class-csvreadergen-__exit__ "Copy/link to this API element")

```python
__exit__(self: Self, Self: type)
```

class ArrayUpdateGen[](https://docs.exaloop.io/libraries/api/modules/numpy/npio/#class-arrayupdategen "Copy/link to this API element")

```python
class ArrayUpdateGen[A, C, F, M]
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/numpy/npio/ArrayUpdateGen/)

Fields 11

`arr` `A`

`cols` `C`

`conv` `F`

`filling_values` `M`

`autostrip` `bool`

`loose` `bool`

`last_row` `int`

`A` `type`

`C` `type`

`F` `type`

`M` `type`

Constructors 1

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/npio/#class-arrayupdategen-__init__ "Copy/link to this API element")

```python
__init__(self: Self, arr: A, cols: C, conv: F, filling_values: M, autostrip: bool, loose: bool, Self: type)
```

Properties 1

cap[](https://docs.exaloop.io/libraries/api/modules/numpy/npio/#class-arrayupdategen-cap "Copy/link to this API element")

@property

```python
cap(self: Self, Self: type)
```

Methods 6

resize\_arrays[](https://docs.exaloop.io/libraries/api/modules/numpy/npio/#class-arrayupdategen-resize_arrays "Copy/link to this API element")

```python
resize_arrays(self: Self, new_cap: int, Self: type)
```

trim\_arrays[](https://docs.exaloop.io/libraries/api/modules/numpy/npio/#class-arrayupdategen-trim_arrays "Copy/link to this API element")

```python
trim_arrays(self: Self, Self: type)
```

fill\_value[](https://docs.exaloop.io/libraries/api/modules/numpy/npio/#class-arrayupdategen-fill_value "Copy/link to this API element")

```python
fill_value(self: Self, idx: int, dtype: type, Self: type)
```

fill\_value\_static[](https://docs.exaloop.io/libraries/api/modules/numpy/npio/#class-arrayupdategen-fill_value_static "Copy/link to this API element")

```python
fill_value_static(self: Self, idx: Literal[int], dtype: type, Self: type)
```

convert[](https://docs.exaloop.io/libraries/api/modules/numpy/npio/#class-arrayupdategen-convert "Copy/link to this API element")

```python
convert(self: Self, field: str, idx: int, dtype: type, Self: type)
```

convert\_static[](https://docs.exaloop.io/libraries/api/modules/numpy/npio/#class-arrayupdategen-convert_static "Copy/link to this API element")

```python
convert_static(self: Self, field: str, idx: Literal[int], dtype: type, Self: type)
```

Special methods 1

\_\_call\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/npio/#class-arrayupdategen-__call__ "Copy/link to this API element")

```python
__call__(self: Self, fields: Ptr[str], num_fields: int, row: int, Self: type)
```

function genfromtxt[](https://docs.exaloop.io/libraries/api/modules/numpy/npio/#function-genfromtxt "Copy/link to this API element")

```python
genfromtxt(fname, dtype: type = float, comments: Optional[str] = "#", delimiter = None, skip_header: int = 0, skip_footer: int = 0, converters = None, filling_values = None, usecols = None, names = None, excludelist = None, deletechars: str = " !#$%&\'()*+, -./:;<=>?@[\\]^{|}~", replace_space: str = "_", autostrip: bool = False, case_sensitive = True, unpack: Literal[bool] = False, loose: bool = True, invalid_raise: bool = True, max_rows: Optional[int] = None, ndmin: Literal[int] = 0)
```

class ndarray[](https://docs.exaloop.io/libraries/api/modules/numpy/npio/#class-ndarray "Copy/link to this API element")

@extend

```python
class ndarray[dtype, ndim]
```

Extends [`numpy.ndarray.ndarray`](https://docs.exaloop.io/libraries/api/classes/numpy/ndarray/ndarray/) · [View complete class API →](https://docs.exaloop.io/libraries/api/classes/numpy/ndarray/ndarray/)

Special methods 2

\_\_pickle\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/npio/#class-ndarray-__pickle__ "Copy/link to this API element")

```python
__pickle__(self: Self, jar: Jar, Self: type)
```

\_\_unpickle\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/npio/#class-ndarray-__unpickle__ "Copy/link to this API element")

```python
__unpickle__(jar: Jar)
```

Source: https://docs.exaloop.io/libraries/api/modules/numpy/npio/
