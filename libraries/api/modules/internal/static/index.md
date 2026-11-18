# internal.static

Standard library Module [View source](https://github.com/exaloop/codon/blob/master/stdlib/internal/static.codon)

function len[](https://docs.exaloop.io/libraries/api/modules/internal/static/#function-len "Copy/link to this API element")

@\_\_generated\_\_

```python
len(obj)
```

function platform[](https://docs.exaloop.io/libraries/api/modules/internal/static/#function-platform "Copy/link to this API element")

```python
platform() -> Literal[str]
```

function print[](https://docs.exaloop.io/libraries/api/modules/internal/static/#function-print "Copy/link to this API element")

@\_\_generated\_\_ @no\_argument\_wrap

```python
print(*args)
```

function range[](https://docs.exaloop.io/libraries/api/modules/internal/static/#function-range "Copy/link to this API element")

```python
range[start: Literal[int], stop: Literal[int], step: Literal[int] = 1]()
```

@overload

```python
range[stop: Literal[int]]()
```

function enumerate[](https://docs.exaloop.io/libraries/api/modules/internal/static/#function-enumerate "Copy/link to this API element")

```python
enumerate(tup)
```

function tuple[](https://docs.exaloop.io/libraries/api/modules/internal/static/#function-tuple "Copy/link to this API element")

@\_\_generated\_\_

```python
tuple(*args)
```

function has\_rtti[](https://docs.exaloop.io/libraries/api/modules/internal/static/#function-has_rtti "Copy/link to this API element")

@\_\_generated\_\_

```python
has_rtti[T: type]()
```

class function[](https://docs.exaloop.io/libraries/api/modules/internal/static/#class-function "Copy/link to this API element")

```python
class function
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/internal/static/function/)

Methods 9

realized[](https://docs.exaloop.io/libraries/api/modules/internal/static/#class-function-realized "Copy/link to this API element")

@\_\_generated\_\_

```python
realized(fn, *args)
```

overloads[](https://docs.exaloop.io/libraries/api/modules/internal/static/#class-function-overloads "Copy/link to this API element")

@\_\_generated\_\_

```python
overloads[T: type, F: Literal[str]]()
```

args[](https://docs.exaloop.io/libraries/api/modules/internal/static/#class-function-args "Copy/link to this API element")

@\_\_generated\_\_

```python
args(F)
```

has\_type[](https://docs.exaloop.io/libraries/api/modules/internal/static/#class-function-has_type "Copy/link to this API element")

@\_\_generated\_\_

```python
has_type[i: Literal[int]](F)
```

get\_type[](https://docs.exaloop.io/libraries/api/modules/internal/static/#class-function-get_type "Copy/link to this API element")

@\_\_generated\_\_

```python
get_type[i: Literal[int]](F)
```

can\_call[](https://docs.exaloop.io/libraries/api/modules/internal/static/#class-function-can_call "Copy/link to this API element")

@no\_argument\_wrap @\_\_generated\_\_

```python
can_call(F, *args, **kwargs)
```

wrap\_args[](https://docs.exaloop.io/libraries/api/modules/internal/static/#class-function-wrap_args "Copy/link to this API element")

@\_\_generated\_\_

```python
wrap_args(F, *args, **kwargs)
```

has\_default[](https://docs.exaloop.io/libraries/api/modules/internal/static/#class-function-has_default "Copy/link to this API element")

@\_\_generated\_\_

```python
has_default[i: Literal[int]](F)
```

get\_default[](https://docs.exaloop.io/libraries/api/modules/internal/static/#class-function-get_default "Copy/link to this API element")

@\_\_generated\_\_

```python
get_default[i: Literal[int]](F)
```

function vars[](https://docs.exaloop.io/libraries/api/modules/internal/static/#function-vars "Copy/link to this API element")

@\_\_generated\_\_

```python
vars[with_index: Literal[bool] = False](obj)
```

function vars\_types[](https://docs.exaloop.io/libraries/api/modules/internal/static/#function-vars_types "Copy/link to this API element")

@\_\_generated\_\_

```python
vars_types[T: type, with_index: Literal[bool] = False]()
```

function methods[](https://docs.exaloop.io/libraries/api/modules/internal/static/#function-methods "Copy/link to this API element")

@\_\_generated\_\_

```python
methods[T: type]()
```

function children[](https://docs.exaloop.io/libraries/api/modules/internal/static/#function-children "Copy/link to this API element")

@\_\_generated\_\_

```python
children[T: type]()
```

function related[](https://docs.exaloop.io/libraries/api/modules/internal/static/#function-related "Copy/link to this API element")

@\_\_generated\_\_

```python
related[T: type]()
```

function tuple\_type[](https://docs.exaloop.io/libraries/api/modules/internal/static/#function-tuple_type "Copy/link to this API element")

@\_\_generated\_\_

```python
tuple_type[T: type, N: Literal[int]]()
```

function format[](https://docs.exaloop.io/libraries/api/modules/internal/static/#function-format "Copy/link to this API element")

@\_\_generated\_\_

```python
format[fmt: Literal[str], arg: Literal[str]]() -> Literal[str]
```

%% is used instead of {} for arguments.

function int\_to\_string[](https://docs.exaloop.io/libraries/api/modules/internal/static/#function-int_to_string "Copy/link to this API element")

@\_\_generated\_\_

```python
int_to_string[val: Literal[int]]() -> Literal[str]
```

function loop\_string[](https://docs.exaloop.io/libraries/api/modules/internal/static/#function-loop_string "Copy/link to this API element")

```python
loop_string[start: Literal[int], stop: Literal[int], val: Literal[str], delim: Literal[str]]() -> Literal[str]
```

function contains[](https://docs.exaloop.io/libraries/api/modules/internal/static/#function-contains "Copy/link to this API element")

```python
contains[text: Literal[str], pattern: Literal[str]]() -> Literal[bool]
```

Source: https://docs.exaloop.io/libraries/api/modules/internal/static/
