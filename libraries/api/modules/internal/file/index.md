# internal.file

Standard library module [View source](https://github.com/exaloop/codon/blob/master/stdlib/internal/file.codon)

class File[](https://docs.exaloop.io/libraries/api/modules/internal/file/#class-file "Copy/link to this API element")

@tuple

```python
class File[T]
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/internal/file/File/)

Fields 1

`T` `type`

Constructors 3

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/file/#class-file-__new__ "Copy/link to this API element")

```python
__new__(fp: cobj, mode: Literal[str])
```

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/file/#class-file-__new__-overload-2 "Copy/link to this API element")

```python
__new__(path: str, mode: str, encoding: str = "utf-8", errors: str = "strict", newline: Optional[str] = None)
```

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/file/#class-file-__new__-overload-3 "Copy/link to this API element")

```python
__new__(fd: int, mode: str)
```

Methods 9

readlines[](https://docs.exaloop.io/libraries/api/modules/internal/file/#class-file-readlines "Copy/link to this API element")

```python
readlines(self: Self, Self: type)
```

readline[](https://docs.exaloop.io/libraries/api/modules/internal/file/#class-file-readline "Copy/link to this API element")

```python
readline(self: Self, Self: type)
```

write[](https://docs.exaloop.io/libraries/api/modules/internal/file/#class-file-write "Copy/link to this API element")

```python
write(self: Self, s: T, Self: type)
```

read[](https://docs.exaloop.io/libraries/api/modules/internal/file/#class-file-read "Copy/link to this API element")

@inline

```python
read(self: Self, sz: int = -1, Self: type)
```

tell[](https://docs.exaloop.io/libraries/api/modules/internal/file/#class-file-tell "Copy/link to this API element")

```python
tell(self: Self, Self: type)
```

seekable[](https://docs.exaloop.io/libraries/api/modules/internal/file/#class-file-seekable "Copy/link to this API element")

```python
seekable(self: Self, Self: type)
```

seek[](https://docs.exaloop.io/libraries/api/modules/internal/file/#class-file-seek "Copy/link to this API element")

```python
seek(self: Self, offset: int, whence: int, Self: type)
```

flush[](https://docs.exaloop.io/libraries/api/modules/internal/file/#class-file-flush "Copy/link to this API element")

```python
flush(self: Self, Self: type)
```

close[](https://docs.exaloop.io/libraries/api/modules/internal/file/#class-file-close "Copy/link to this API element")

```python
close(self: Self, Self: type)
```

Special methods 4

\_\_enter\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/file/#class-file-__enter__ "Copy/link to this API element")

```python
__enter__(self: Self, Self: type)
```

\_\_exit\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/file/#class-file-__exit__ "Copy/link to this API element")

```python
__exit__(self: Self, Self: type)
```

\_\_iter\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/file/#class-file-__iter__ "Copy/link to this API element")

```python
__iter__(self: Self, Self: type)
```

\_\_file\_write\_gen\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/file/#class-file-__file_write_gen__ "Copy/link to this API element")

```python
__file_write_gen__(self: Self, g: Generator[T], T: type, Self: type)
```

class gzFile[](https://docs.exaloop.io/libraries/api/modules/internal/file/#class-gzfile "Copy/link to this API element")

@tuple

```python
class gzFile[T]
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/internal/file/gzFile/)

Fields 1

`T` `type`

Constructors 2

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/file/#class-gzfile-__new__ "Copy/link to this API element")

```python
__new__(fp: cobj, mode: str)
```

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/file/#class-gzfile-__new__-overload-2 "Copy/link to this API element")

```python
__new__(path: str, mode: str, compresslevel: int = 9, encoding: str = "utf-8", errors: str = "strict", newline: Optional[str] = None)
```

Methods 8

close[](https://docs.exaloop.io/libraries/api/modules/internal/file/#class-gzfile-close "Copy/link to this API element")

```python
close(self: Self, Self: type)
```

readlines[](https://docs.exaloop.io/libraries/api/modules/internal/file/#class-gzfile-readlines "Copy/link to this API element")

```python
readlines(self: Self, Self: type)
```

readline[](https://docs.exaloop.io/libraries/api/modules/internal/file/#class-gzfile-readline "Copy/link to this API element")

```python
readline(self: Self, Self: type)
```

write[](https://docs.exaloop.io/libraries/api/modules/internal/file/#class-gzfile-write "Copy/link to this API element")

```python
write(self: Self, s: T, Self: type)
```

read[](https://docs.exaloop.io/libraries/api/modules/internal/file/#class-gzfile-read "Copy/link to this API element")

```python
read(self: Self, sz: int = -1, Self: type)
```

tell[](https://docs.exaloop.io/libraries/api/modules/internal/file/#class-gzfile-tell "Copy/link to this API element")

```python
tell(self: Self, Self: type)
```

seek[](https://docs.exaloop.io/libraries/api/modules/internal/file/#class-gzfile-seek "Copy/link to this API element")

```python
seek(self: Self, offset: int, whence: int, Self: type)
```

flush[](https://docs.exaloop.io/libraries/api/modules/internal/file/#class-gzfile-flush "Copy/link to this API element")

```python
flush(self: Self, Self: type)
```

Special methods 4

\_\_iter\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/file/#class-gzfile-__iter__ "Copy/link to this API element")

```python
__iter__(self: Self, Self: type)
```

\_\_enter\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/file/#class-gzfile-__enter__ "Copy/link to this API element")

```python
__enter__(self: Self, Self: type)
```

\_\_exit\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/file/#class-gzfile-__exit__ "Copy/link to this API element")

```python
__exit__(self: Self, Self: type)
```

\_\_file\_write\_gen\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/file/#class-gzfile-__file_write_gen__ "Copy/link to this API element")

```python
__file_write_gen__(self: Self, g: Generator[T], T: type, Self: type)
```

class bzFile[](https://docs.exaloop.io/libraries/api/modules/internal/file/#class-bzfile "Copy/link to this API element")

@tuple

```python
class bzFile[T]
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/internal/file/bzFile/)

Fields 1

`T` `type`

Constructors 1

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/file/#class-bzfile-__new__ "Copy/link to this API element")

```python
__new__(path: str, mode: str, compresslevel: int = 9, encoding: str = "utf-8", errors: str = "strict", newline: Optional[str] = None)
```

Methods 9

close[](https://docs.exaloop.io/libraries/api/modules/internal/file/#class-bzfile-close "Copy/link to this API element")

```python
close(self: Self, Self: type)
```

readlines[](https://docs.exaloop.io/libraries/api/modules/internal/file/#class-bzfile-readlines "Copy/link to this API element")

```python
readlines(self: Self, Self: type)
```

readline[](https://docs.exaloop.io/libraries/api/modules/internal/file/#class-bzfile-readline "Copy/link to this API element")

```python
readline(self: Self, Self: type)
```

write[](https://docs.exaloop.io/libraries/api/modules/internal/file/#class-bzfile-write "Copy/link to this API element")

```python
write(self: Self, s: T, Self: type)
```

read[](https://docs.exaloop.io/libraries/api/modules/internal/file/#class-bzfile-read "Copy/link to this API element")

```python
read(self: Self, sz: int = -1, Self: type)
```

tell[](https://docs.exaloop.io/libraries/api/modules/internal/file/#class-bzfile-tell "Copy/link to this API element")

```python
tell(self: Self, Self: type)
```

seekable[](https://docs.exaloop.io/libraries/api/modules/internal/file/#class-bzfile-seekable "Copy/link to this API element")

```python
seekable(self: Self, Self: type)
```

seek[](https://docs.exaloop.io/libraries/api/modules/internal/file/#class-bzfile-seek "Copy/link to this API element")

```python
seek(self: Self, offset: int, whence: int, Self: type)
```

flush[](https://docs.exaloop.io/libraries/api/modules/internal/file/#class-bzfile-flush "Copy/link to this API element")

```python
flush(self: Self, Self: type)
```

Special methods 4

\_\_iter\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/file/#class-bzfile-__iter__ "Copy/link to this API element")

```python
__iter__(self: Self, Self: type)
```

\_\_enter\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/file/#class-bzfile-__enter__ "Copy/link to this API element")

```python
__enter__(self: Self, Self: type)
```

\_\_exit\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/file/#class-bzfile-__exit__ "Copy/link to this API element")

```python
__exit__(self: Self, Self: type)
```

\_\_file\_write\_gen\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/file/#class-bzfile-__file_write_gen__ "Copy/link to this API element")

```python
__file_write_gen__(self: Self, g: Generator[T], T: type, Self: type)
```

function open[](https://docs.exaloop.io/libraries/api/modules/internal/file/#function-open "Copy/link to this API element")

```python
open(path, mode: str = "r", encoding: Optional[str] = None, errors: Optional[str] = None, newline: Optional[str] = None)
```

@overload

```python
open(path, mode: Literal[str], encoding: Optional[str] = None, errors: Optional[str] = None, newline: Optional[str] = None)
```

function gzopen[](https://docs.exaloop.io/libraries/api/modules/internal/file/#function-gzopen "Copy/link to this API element")

```python
gzopen(path: str, mode: str = "rb", compresslevel: int = 9, encoding: Optional[str] = None, errors: Optional[str] = None, newline: Optional[str] = None)
```

@overload

```python
gzopen(path: str, mode: Literal[str], compresslevel: int = 9, encoding: Optional[str] = None, errors: Optional[str] = None, newline: Optional[str] = None)
```

function bzopen[](https://docs.exaloop.io/libraries/api/modules/internal/file/#function-bzopen "Copy/link to this API element")

```python
bzopen(path: str, mode: str = "rb", compresslevel: int = 9, encoding: Optional[str] = None, errors: Optional[str] = None, newline: Optional[str] = None)
```

@overload

```python
bzopen(path: str, mode: Literal[str], compresslevel: int = 9, encoding: Optional[str] = None, errors: Optional[str] = None, newline: Optional[str] = None)
```

function is\_binary[](https://docs.exaloop.io/libraries/api/modules/internal/file/#function-is_binary "Copy/link to this API element")

```python
is_binary(path: str)
```

Source: https://docs.exaloop.io/libraries/api/modules/internal/file/
