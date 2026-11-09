# File

Standard library class `internal.file.File` [View source](https://github.com/exaloop/codon/blob/master/stdlib/internal/file.codon)

class File[](https://docs.exaloop.io/libraries/api/classes/internal/file/File/#class-file "Copy/link to this API element")

@tuple

```python
class File[T]
```

Fields 1

`T` `type`

Constructors 3

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/file/File/#class-file-__new__ "Copy/link to this API element")

```python
__new__(fp: cobj, mode: Literal[str])
```

Defined in [`internal.file`](https://docs.exaloop.io/libraries/api/modules/internal/file/)

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/file/File/#class-file-__new__-overload-2 "Copy/link to this API element")

```python
__new__(path: str, mode: str, encoding: str = "utf-8", errors: str = "strict", newline: Optional[str] = None)
```

Defined in [`internal.file`](https://docs.exaloop.io/libraries/api/modules/internal/file/)

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/file/File/#class-file-__new__-overload-3 "Copy/link to this API element")

```python
__new__(fd: int, mode: str)
```

Defined in [`internal.file`](https://docs.exaloop.io/libraries/api/modules/internal/file/)

Methods 9

readlines[](https://docs.exaloop.io/libraries/api/classes/internal/file/File/#class-file-readlines "Copy/link to this API element")

```python
readlines(self: Self, Self: type)
```

Defined in [`internal.file`](https://docs.exaloop.io/libraries/api/modules/internal/file/)

readline[](https://docs.exaloop.io/libraries/api/classes/internal/file/File/#class-file-readline "Copy/link to this API element")

```python
readline(self: Self, Self: type)
```

Defined in [`internal.file`](https://docs.exaloop.io/libraries/api/modules/internal/file/)

write[](https://docs.exaloop.io/libraries/api/classes/internal/file/File/#class-file-write "Copy/link to this API element")

```python
write(self: Self, s: T, Self: type)
```

Defined in [`internal.file`](https://docs.exaloop.io/libraries/api/modules/internal/file/)

read[](https://docs.exaloop.io/libraries/api/classes/internal/file/File/#class-file-read "Copy/link to this API element")

@inline

```python
read(self: Self, sz: int = -1, Self: type)
```

Defined in [`internal.file`](https://docs.exaloop.io/libraries/api/modules/internal/file/)

tell[](https://docs.exaloop.io/libraries/api/classes/internal/file/File/#class-file-tell "Copy/link to this API element")

```python
tell(self: Self, Self: type)
```

Defined in [`internal.file`](https://docs.exaloop.io/libraries/api/modules/internal/file/)

seekable[](https://docs.exaloop.io/libraries/api/classes/internal/file/File/#class-file-seekable "Copy/link to this API element")

```python
seekable(self: Self, Self: type)
```

Defined in [`internal.file`](https://docs.exaloop.io/libraries/api/modules/internal/file/)

seek[](https://docs.exaloop.io/libraries/api/classes/internal/file/File/#class-file-seek "Copy/link to this API element")

```python
seek(self: Self, offset: int, whence: int, Self: type)
```

Defined in [`internal.file`](https://docs.exaloop.io/libraries/api/modules/internal/file/)

flush[](https://docs.exaloop.io/libraries/api/classes/internal/file/File/#class-file-flush "Copy/link to this API element")

```python
flush(self: Self, Self: type)
```

Defined in [`internal.file`](https://docs.exaloop.io/libraries/api/modules/internal/file/)

close[](https://docs.exaloop.io/libraries/api/classes/internal/file/File/#class-file-close "Copy/link to this API element")

```python
close(self: Self, Self: type)
```

Defined in [`internal.file`](https://docs.exaloop.io/libraries/api/modules/internal/file/)

Special methods 4

\_\_enter\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/file/File/#class-file-__enter__ "Copy/link to this API element")

```python
__enter__(self: Self, Self: type)
```

Defined in [`internal.file`](https://docs.exaloop.io/libraries/api/modules/internal/file/)

\_\_exit\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/file/File/#class-file-__exit__ "Copy/link to this API element")

```python
__exit__(self: Self, Self: type)
```

Defined in [`internal.file`](https://docs.exaloop.io/libraries/api/modules/internal/file/)

\_\_iter\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/file/File/#class-file-__iter__ "Copy/link to this API element")

```python
__iter__(self: Self, Self: type)
```

Defined in [`internal.file`](https://docs.exaloop.io/libraries/api/modules/internal/file/)

\_\_file\_write\_gen\_\_[](https://docs.exaloop.io/libraries/api/classes/internal/file/File/#class-file-__file_write_gen__ "Copy/link to this API element")

```python
__file_write_gen__(self: Self, g: Generator[T], T: type, Self: type)
```

Defined in [`internal.file`](https://docs.exaloop.io/libraries/api/modules/internal/file/)

Source: https://docs.exaloop.io/libraries/api/classes/internal/file/File/
