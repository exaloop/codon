# os.path

Standard library Module [View source](https://github.com/exaloop/codon/blob/master/stdlib/os/path.codon)

Native path operations (POSIX, or Windows when targeting `win32`).

variable curdir[](https://docs.exaloop.io/libraries/api/modules/os/path/#variable-curdir "Copy/link to this API element")

```python
curdir: Literal[str] = "."
```

variable pardir[](https://docs.exaloop.io/libraries/api/modules/os/path/#variable-pardir "Copy/link to this API element")

```python
pardir: Literal[str] = ".."
```

variable extsep[](https://docs.exaloop.io/libraries/api/modules/os/path/#variable-extsep "Copy/link to this API element")

```python
extsep: Literal[str] = "."
```

function exists[](https://docs.exaloop.io/libraries/api/modules/os/path/#function-exists "Copy/link to this API element")

```python
exists(path: str) -> bool
```

function lexists[](https://docs.exaloop.io/libraries/api/modules/os/path/#function-lexists "Copy/link to this API element")

```python
lexists(path: str) -> bool
```

function isfile[](https://docs.exaloop.io/libraries/api/modules/os/path/#function-isfile "Copy/link to this API element")

```python
isfile(path: str) -> bool
```

function isdir[](https://docs.exaloop.io/libraries/api/modules/os/path/#function-isdir "Copy/link to this API element")

```python
isdir(path: str) -> bool
```

function islink[](https://docs.exaloop.io/libraries/api/modules/os/path/#function-islink "Copy/link to this API element")

```python
islink(path: str) -> bool
```

function samefile[](https://docs.exaloop.io/libraries/api/modules/os/path/#function-samefile "Copy/link to this API element")

```python
samefile(path1: str, path2: str) -> bool
```

function sameopenfile[](https://docs.exaloop.io/libraries/api/modules/os/path/#function-sameopenfile "Copy/link to this API element")

```python
sameopenfile(fd1: int, fd2: int) -> bool
```

function getsize[](https://docs.exaloop.io/libraries/api/modules/os/path/#function-getsize "Copy/link to this API element")

```python
getsize(path: str) -> int
```

function getmtime[](https://docs.exaloop.io/libraries/api/modules/os/path/#function-getmtime "Copy/link to this API element")

```python
getmtime(path: str) -> float
```

function getatime[](https://docs.exaloop.io/libraries/api/modules/os/path/#function-getatime "Copy/link to this API element")

```python
getatime(path: str) -> float
```

function getctime[](https://docs.exaloop.io/libraries/api/modules/os/path/#function-getctime "Copy/link to this API element")

```python
getctime(path: str) -> float
```

function expanduser[](https://docs.exaloop.io/libraries/api/modules/os/path/#function-expanduser "Copy/link to this API element")

```python
expanduser(path: str) -> str
```

function realpath[](https://docs.exaloop.io/libraries/api/modules/os/path/#function-realpath "Copy/link to this API element")

```python
realpath(path: str, *, strict: bool = False) -> str
```

function isabs[](https://docs.exaloop.io/libraries/api/modules/os/path/#function-isabs "Copy/link to this API element")

```python
isabs(path: str) -> bool
```

function join[](https://docs.exaloop.io/libraries/api/modules/os/path/#function-join "Copy/link to this API element")

```python
join(a: str, *paths) -> str
```

function split[](https://docs.exaloop.io/libraries/api/modules/os/path/#function-split "Copy/link to this API element")

```python
split(path: str) -> Tuple[(str, str)]
```

function dirname[](https://docs.exaloop.io/libraries/api/modules/os/path/#function-dirname "Copy/link to this API element")

```python
dirname(path: str) -> str
```

function basename[](https://docs.exaloop.io/libraries/api/modules/os/path/#function-basename "Copy/link to this API element")

```python
basename(path: str) -> str
```

function splitdrive[](https://docs.exaloop.io/libraries/api/modules/os/path/#function-splitdrive "Copy/link to this API element")

```python
splitdrive(path: str) -> Tuple[(str, str)]
```

function splitroot[](https://docs.exaloop.io/libraries/api/modules/os/path/#function-splitroot "Copy/link to this API element")

```python
splitroot(path: str) -> Tuple[(str, str, str)]
```

function normcase[](https://docs.exaloop.io/libraries/api/modules/os/path/#function-normcase "Copy/link to this API element")

```python
normcase(path: str) -> str
```

function normpath[](https://docs.exaloop.io/libraries/api/modules/os/path/#function-normpath "Copy/link to this API element")

```python
normpath(path: str) -> str
```

function abspath[](https://docs.exaloop.io/libraries/api/modules/os/path/#function-abspath "Copy/link to this API element")

```python
abspath(path: str) -> str
```

function relpath[](https://docs.exaloop.io/libraries/api/modules/os/path/#function-relpath "Copy/link to this API element")

```python
relpath(path: str, start: str = ".") -> str
```

function commonprefix[](https://docs.exaloop.io/libraries/api/modules/os/path/#function-commonprefix "Copy/link to this API element")

```python
commonprefix(paths) -> str
```

function commonpath[](https://docs.exaloop.io/libraries/api/modules/os/path/#function-commonpath "Copy/link to this API element")

```python
commonpath(paths) -> str
```

function expandvars[](https://docs.exaloop.io/libraries/api/modules/os/path/#function-expandvars "Copy/link to this API element")

```python
expandvars(path: str) -> str
```

function splitext[](https://docs.exaloop.io/libraries/api/modules/os/path/#function-splitext "Copy/link to this API element")

```python
splitext(p: str) -> Tuple[(str, str)]
```

Split a pathname into `(root, ext)`.

```text
The extension runs from the last dot to the end, ignoring leading dots.
`ext` may be empty.
```

Source: https://docs.exaloop.io/libraries/api/modules/os/path/
