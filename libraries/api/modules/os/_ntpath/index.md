# os.\_ntpath

Standard library module [View source](https://github.com/exaloop/codon/blob/master/stdlib/os/_ntpath.codon)

Windows string path operations, importable on every target.

Lexical operations follow Python 3.13 Windows path rules. They do not inspect the filesystem or resolve symlinks. Relative `abspath()` and `relpath()` need Windows' current-directory and per-drive state; on other targets only fully qualified paths can be resolved. Environment expansion has a mapping-based helper for host-independent tests.

variable curdir[](https://docs.exaloop.io/libraries/api/modules/os/_ntpath/#variable-curdir "Copy/link to this API element")

```python
curdir: Literal[str] = "."
```

variable pardir[](https://docs.exaloop.io/libraries/api/modules/os/_ntpath/#variable-pardir "Copy/link to this API element")

```python
pardir: Literal[str] = ".."
```

variable sep[](https://docs.exaloop.io/libraries/api/modules/os/_ntpath/#variable-sep "Copy/link to this API element")

```python
sep: Literal[str] = "\\"
```

variable pathsep[](https://docs.exaloop.io/libraries/api/modules/os/_ntpath/#variable-pathsep "Copy/link to this API element")

```python
pathsep: Literal[str] = ";"
```

variable defpath[](https://docs.exaloop.io/libraries/api/modules/os/_ntpath/#variable-defpath "Copy/link to this API element")

```python
defpath: Literal[str] = ".;C:\\bin"
```

variable extsep[](https://docs.exaloop.io/libraries/api/modules/os/_ntpath/#variable-extsep "Copy/link to this API element")

```python
extsep: Literal[str] = "."
```

variable altsep[](https://docs.exaloop.io/libraries/api/modules/os/_ntpath/#variable-altsep "Copy/link to this API element")

```python
altsep: Optional[str] = "/"
```

variable devnull[](https://docs.exaloop.io/libraries/api/modules/os/_ntpath/#variable-devnull "Copy/link to this API element")

```python
devnull: Literal[str] = "nul"
```

function isabs[](https://docs.exaloop.io/libraries/api/modules/os/_ntpath/#function-isabs "Copy/link to this API element")

```python
isabs(path: str)
```

function splitroot[](https://docs.exaloop.io/libraries/api/modules/os/_ntpath/#function-splitroot "Copy/link to this API element")

```python
splitroot(path: str)
```

function splitdrive[](https://docs.exaloop.io/libraries/api/modules/os/_ntpath/#function-splitdrive "Copy/link to this API element")

```python
splitdrive(path: str)
```

function join[](https://docs.exaloop.io/libraries/api/modules/os/_ntpath/#function-join "Copy/link to this API element")

```python
join(a: str, *paths)
```

function split[](https://docs.exaloop.io/libraries/api/modules/os/_ntpath/#function-split "Copy/link to this API element")

```python
split(path: str)
```

function dirname[](https://docs.exaloop.io/libraries/api/modules/os/_ntpath/#function-dirname "Copy/link to this API element")

```python
dirname(path: str)
```

function basename[](https://docs.exaloop.io/libraries/api/modules/os/_ntpath/#function-basename "Copy/link to this API element")

```python
basename(path: str)
```

function splitext[](https://docs.exaloop.io/libraries/api/modules/os/_ntpath/#function-splitext "Copy/link to this API element")

```python
splitext(path: str)
```

function normcase[](https://docs.exaloop.io/libraries/api/modules/os/_ntpath/#function-normcase "Copy/link to this API element")

```python
normcase(path: str)
```

function normpath[](https://docs.exaloop.io/libraries/api/modules/os/_ntpath/#function-normpath "Copy/link to this API element")

```python
normpath(path: str)
```

function abspath[](https://docs.exaloop.io/libraries/api/modules/os/_ntpath/#function-abspath "Copy/link to this API element")

```python
abspath(path: str)
```

function relpath[](https://docs.exaloop.io/libraries/api/modules/os/_ntpath/#function-relpath "Copy/link to this API element")

```python
relpath(path: str, start: str = ".")
```

function commonprefix[](https://docs.exaloop.io/libraries/api/modules/os/_ntpath/#function-commonprefix "Copy/link to this API element")

```python
commonprefix(paths)
```

function commonpath[](https://docs.exaloop.io/libraries/api/modules/os/_ntpath/#function-commonpath "Copy/link to this API element")

```python
commonpath(paths)
```

function expanduser[](https://docs.exaloop.io/libraries/api/modules/os/_ntpath/#function-expanduser "Copy/link to this API element")

```python
expanduser(path: str)
```

function expandvars[](https://docs.exaloop.io/libraries/api/modules/os/_ntpath/#function-expandvars "Copy/link to this API element")

```python
expandvars(path: str)
```

Source: https://docs.exaloop.io/libraries/api/modules/os/_ntpath/
