# os.path

Standard library module [View source](https://github.com/exaloop/codon/blob/master/stdlib/os/path.codon)

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
exists(path: str)
```

function lexists[](https://docs.exaloop.io/libraries/api/modules/os/path/#function-lexists "Copy/link to this API element")

```python
lexists(path: str)
```

function isfile[](https://docs.exaloop.io/libraries/api/modules/os/path/#function-isfile "Copy/link to this API element")

```python
isfile(path: str)
```

function isdir[](https://docs.exaloop.io/libraries/api/modules/os/path/#function-isdir "Copy/link to this API element")

```python
isdir(path: str)
```

function islink[](https://docs.exaloop.io/libraries/api/modules/os/path/#function-islink "Copy/link to this API element")

```python
islink(path: str)
```

function samefile[](https://docs.exaloop.io/libraries/api/modules/os/path/#function-samefile "Copy/link to this API element")

```python
samefile(path1: str, path2: str)
```

function sameopenfile[](https://docs.exaloop.io/libraries/api/modules/os/path/#function-sameopenfile "Copy/link to this API element")

```python
sameopenfile(fd1: int, fd2: int)
```

function getsize[](https://docs.exaloop.io/libraries/api/modules/os/path/#function-getsize "Copy/link to this API element")

```python
getsize(path: str)
```

function getmtime[](https://docs.exaloop.io/libraries/api/modules/os/path/#function-getmtime "Copy/link to this API element")

```python
getmtime(path: str)
```

function getatime[](https://docs.exaloop.io/libraries/api/modules/os/path/#function-getatime "Copy/link to this API element")

```python
getatime(path: str)
```

function getctime[](https://docs.exaloop.io/libraries/api/modules/os/path/#function-getctime "Copy/link to this API element")

```python
getctime(path: str)
```

function expanduser[](https://docs.exaloop.io/libraries/api/modules/os/path/#function-expanduser "Copy/link to this API element")

```python
expanduser(path: str)
```

function realpath[](https://docs.exaloop.io/libraries/api/modules/os/path/#function-realpath "Copy/link to this API element")

```python
realpath(path: str, strict: bool = False)
```

function isabs[](https://docs.exaloop.io/libraries/api/modules/os/path/#function-isabs "Copy/link to this API element")

```python
isabs(path: str)
```

function join[](https://docs.exaloop.io/libraries/api/modules/os/path/#function-join "Copy/link to this API element")

```python
join(a: str, *paths)
```

function split[](https://docs.exaloop.io/libraries/api/modules/os/path/#function-split "Copy/link to this API element")

```python
split(path: str)
```

function dirname[](https://docs.exaloop.io/libraries/api/modules/os/path/#function-dirname "Copy/link to this API element")

```python
dirname(path: str)
```

function basename[](https://docs.exaloop.io/libraries/api/modules/os/path/#function-basename "Copy/link to this API element")

```python
basename(path: str)
```

function splitdrive[](https://docs.exaloop.io/libraries/api/modules/os/path/#function-splitdrive "Copy/link to this API element")

```python
splitdrive(path: str)
```

function splitroot[](https://docs.exaloop.io/libraries/api/modules/os/path/#function-splitroot "Copy/link to this API element")

```python
splitroot(path: str)
```

function normcase[](https://docs.exaloop.io/libraries/api/modules/os/path/#function-normcase "Copy/link to this API element")

```python
normcase(path: str)
```

function normpath[](https://docs.exaloop.io/libraries/api/modules/os/path/#function-normpath "Copy/link to this API element")

```python
normpath(path: str)
```

function abspath[](https://docs.exaloop.io/libraries/api/modules/os/path/#function-abspath "Copy/link to this API element")

```python
abspath(path: str)
```

function relpath[](https://docs.exaloop.io/libraries/api/modules/os/path/#function-relpath "Copy/link to this API element")

```python
relpath(path: str, start: str = ".")
```

function commonprefix[](https://docs.exaloop.io/libraries/api/modules/os/path/#function-commonprefix "Copy/link to this API element")

```python
commonprefix(paths)
```

function commonpath[](https://docs.exaloop.io/libraries/api/modules/os/path/#function-commonpath "Copy/link to this API element")

```python
commonpath(paths)
```

function expandvars[](https://docs.exaloop.io/libraries/api/modules/os/path/#function-expandvars "Copy/link to this API element")

```python
expandvars(path: str)
```

function splitext[](https://docs.exaloop.io/libraries/api/modules/os/path/#function-splitext "Copy/link to this API element")

```python
splitext(p: str)
```

Split a pathname into `(root, ext)`.

```text
The extension runs from the last dot to the end, ignoring leading dots.
`ext` may be empty.
```

Source: https://docs.exaloop.io/libraries/api/modules/os/path/
