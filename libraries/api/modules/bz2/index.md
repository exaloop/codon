# bz2

Standard library Module [View source](https://github.com/exaloop/codon/blob/master/stdlib/bz2.codon)

alias BZ2File[](https://docs.exaloop.io/libraries/api/modules/bz2/#alias-bz2file "Copy/link to this API element")

```python
BZ2File = bzFile
```

Alias of [`bzFile`](https://docs.exaloop.io/libraries/api/classes/internal/file/bzFile/)

function open[](https://docs.exaloop.io/libraries/api/modules/bz2/#function-open "Copy/link to this API element")

```python
open(path: str, mode: str = "rb", compresslevel: int = 9, encoding: Optional[str] = None, errors: Optional[str] = None, newline: Optional[str] = None) -> bzFile[bytes]
```

@overload

```python
open[mode: Literal[str]](path: str, compresslevel: int = 9, encoding: Optional[str] = None, errors: Optional[str] = None, newline: Optional[str] = None)
```

Source: https://docs.exaloop.io/libraries/api/modules/bz2/
