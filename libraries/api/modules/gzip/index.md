# gzip

Standard library Module [View source](https://github.com/exaloop/codon/blob/master/stdlib/gzip.codon)

alias GzipFile[](https://docs.exaloop.io/libraries/api/modules/gzip/#alias-gzipfile "Copy/link to this API element")

```python
GzipFile = gzFile
```

Alias of [`gzFile`](https://docs.exaloop.io/libraries/api/classes/internal/file/gzFile/)

function open[](https://docs.exaloop.io/libraries/api/modules/gzip/#function-open "Copy/link to this API element")

```python
open(path: str, mode: str = "rb", compresslevel: int = 9, encoding: Optional[str] = None, errors: Optional[str] = None, newline: Optional[str] = None) -> gzFile[bytes]
```

@overload

```python
open[mode: Literal[str]](path: str, compresslevel: int = 9, encoding: Optional[str] = None, errors: Optional[str] = None, newline: Optional[str] = None)
```

Source: https://docs.exaloop.io/libraries/api/modules/gzip/
