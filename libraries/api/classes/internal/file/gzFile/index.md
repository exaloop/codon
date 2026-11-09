# gzFile

Standard library class `internal.file.gzFile` [View source](https://github.com/exaloop/codon/blob/master/stdlib/internal/file.codon)

class gzFile[](https://docs.exaloop.io/libraries/api/classes/internal/file/gzFile/#class-gzfile "Copy/link to this API element")

@tuple

```python
class gzFile[T]
```

Fields 1

`T` `type`

Constructors 2

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/file/gzFile/#class-gzfile-__new__ "Copy/link to this API element")

```python
__new__(fp: cobj, mode: str)
```

Defined in [`internal.file`](https://docs.exaloop.io/libraries/api/modules/internal/file/)

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/file/gzFile/#class-gzfile-__new__-overload-2 "Copy/link to this API element")

```python
__new__(path: str, mode: str, compresslevel: int = 9, encoding: str = "utf-8", errors: str = "strict", newline: Optional[str] = None)
```

Defined in [`internal.file`](https://docs.exaloop.io/libraries/api/modules/internal/file/)

Methods 8

close[](https://docs.exaloop.io/libraries/api/classes/internal/file/gzFile/#class-gzfile-close "Copy/link to this API element")

```python
close(self: Self, Self: type)
```

Defined in [`internal.file`](https://docs.exaloop.io/libraries/api/modules/internal/file/)

readlines[](https://docs.exaloop.io/libraries/api/classes/internal/file/gzFile/#class-gzfile-readlines "Copy/link to this API element")

```python
readlines(self: Self, Self: type)
```

Defined in [`internal.file`](https://docs.exaloop.io/libraries/api/modules/internal/file/)

readline[](https://docs.exaloop.io/libraries/api/classes/internal/file/gzFile/#class-gzfile-readline "Copy/link to this API element")

```python
readline(self: Self, Self: type)
```

Defined in [`internal.file`](https://docs.exaloop.io/libraries/api/modules/internal/file/)

write[](https://docs.exaloop.io/libraries/api/classes/internal/file/gzFile/#class-gzfile-write "Copy/link to this API element")

```python
write(self: Self, s: T, Self: type)
```

Defined in [`internal.file`](https://docs.exaloop.io/libraries/api/modules/internal/file/)

read[](https://docs.exaloop.io/libraries/api/classes/internal/file/gzFile/#class-gzfile-read "Copy/link to this API element")

```python
read(self: Self, sz: int = -1, Self: type)
```

Defined in [`internal.file`](https://docs.exaloop.io/libraries/api/modules/internal/file/)

tell[](https://docs.exaloop.io/libraries/api/classes/internal/file/gzFile/#class-gzfile-tell "Copy/link to this API element")

```python
tell(self: Self, Self: type)
```

Defined in [`internal.file`](https://docs.exaloop.io/libraries/api/modules/internal/file/)

seek[](https://docs.exaloop.io/libraries/api/classes/internal/file/gzFile/#class-gzfile-seek "Copy/link to this API element")

```python
seek(self: Self, offset: int, whence: int, Self: type)
```

Defined in [`internal.file`](https://docs.exaloop.io/libraries/api/modules/internal/file/)

flush[](https://docs.exaloop.io/libraries/api/classes/internal/file/gzFile/#class-gzfile-flush "Copy/link to this API element")

```python
flush(self: Self, Self: type)
```

Defined in [`internal.file`](https://docs.exaloop.io/libraries/api/modules/internal/file/)

Special methods 4

\_\_iter\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/file/gzFile/#class-gzfile-__iter__ "Copy/link to this API element")

```python
__iter__(self: Self, Self: type)
```

Defined in [`internal.file`](https://docs.exaloop.io/libraries/api/modules/internal/file/)

\_\_enter\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/file/gzFile/#class-gzfile-__enter__ "Copy/link to this API element")

```python
__enter__(self: Self, Self: type)
```

Defined in [`internal.file`](https://docs.exaloop.io/libraries/api/modules/internal/file/)

\_\_exit\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/file/gzFile/#class-gzfile-__exit__ "Copy/link to this API element")

```python
__exit__(self: Self, Self: type)
```

Defined in [`internal.file`](https://docs.exaloop.io/libraries/api/modules/internal/file/)

\_\_file\_write\_gen\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/file/gzFile/#class-gzfile-__file_write_gen__ "Copy/link to this API element")

```python
__file_write_gen__(self: Self, g: Generator[T], T: type, Self: type)
```

Defined in [`internal.file`](https://docs.exaloop.io/libraries/api/modules/internal/file/)

Source: https://docs.exaloop.io/libraries/api/classes/internal/file/gzFile/
