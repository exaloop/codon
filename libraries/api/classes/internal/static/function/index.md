# function

Standard library class `internal.static.function` [View source](https://github.com/exaloop/codon/blob/master/stdlib/internal/static.codon#L46)

class function[](https://docs.exaloop.io/libraries/api/classes/internal/static/function/#class-function "Copy/link to this API element")

```python
class function
```

Methods 10

realized[](https://docs.exaloop.io/libraries/api/classes/internal/static/function/#class-function-realized "Copy/link to this API element")

@\_\_generated\_\_

```python
realized(fn, *args)
```

Defined in [`internal.static`](https://docs.exaloop.io/libraries/api/modules/internal/static/)

overloads[](https://docs.exaloop.io/libraries/api/classes/internal/static/function/#class-function-overloads "Copy/link to this API element")

@\_\_generated\_\_

```python
overloads[T: type, F: Literal[str]]()
```

Defined in [`internal.static`](https://docs.exaloop.io/libraries/api/modules/internal/static/)

args[](https://docs.exaloop.io/libraries/api/classes/internal/static/function/#class-function-args "Copy/link to this API element")

@\_\_generated\_\_

```python
args(F)
```

Defined in [`internal.static`](https://docs.exaloop.io/libraries/api/modules/internal/static/)

arg\_kind[](https://docs.exaloop.io/libraries/api/classes/internal/static/function/#class-function-arg_kind "Copy/link to this API element")

@\_\_generated\_\_

```python
arg_kind[i: Literal[int]](F) -> Literal[str]
```

Defined in [`internal.static`](https://docs.exaloop.io/libraries/api/modules/internal/static/)

Return the parameter kind at the same index as args(F).

```text
    Kinds are positional_only, positional_or_keyword, var_positional,
    keyword_only, and var_keyword. Generic parameters are skipped;
    bound methods and partials retain the underlying function's parameters.
    An out-of-range index is a compile-time error.
```

has\_type[](https://docs.exaloop.io/libraries/api/classes/internal/static/function/#class-function-has_type "Copy/link to this API element")

@\_\_generated\_\_

```python
has_type[i: Literal[int]](F)
```

Defined in [`internal.static`](https://docs.exaloop.io/libraries/api/modules/internal/static/)

get\_type[](https://docs.exaloop.io/libraries/api/classes/internal/static/function/#class-function-get_type "Copy/link to this API element")

@\_\_generated\_\_

```python
get_type[i: Literal[int]](F)
```

Defined in [`internal.static`](https://docs.exaloop.io/libraries/api/modules/internal/static/)

can\_call[](https://docs.exaloop.io/libraries/api/classes/internal/static/function/#class-function-can_call "Copy/link to this API element")

@no\_argument\_wrap @\_\_generated\_\_

```python
can_call(F, *args, **kwargs)
```

Defined in [`internal.static`](https://docs.exaloop.io/libraries/api/modules/internal/static/)

wrap\_args[](https://docs.exaloop.io/libraries/api/classes/internal/static/function/#class-function-wrap_args "Copy/link to this API element")

@\_\_generated\_\_

```python
wrap_args(F, *args, **kwargs)
```

Defined in [`internal.static`](https://docs.exaloop.io/libraries/api/modules/internal/static/)

has\_default[](https://docs.exaloop.io/libraries/api/classes/internal/static/function/#class-function-has_default "Copy/link to this API element")

@\_\_generated\_\_

```python
has_default[i: Literal[int]](F)
```

Defined in [`internal.static`](https://docs.exaloop.io/libraries/api/modules/internal/static/)

get\_default[](https://docs.exaloop.io/libraries/api/classes/internal/static/function/#class-function-get_default "Copy/link to this API element")

@\_\_generated\_\_

```python
get_default[i: Literal[int]](F)
```

Defined in [`internal.static`](https://docs.exaloop.io/libraries/api/modules/internal/static/)

Source: https://docs.exaloop.io/libraries/api/classes/internal/static/function/
