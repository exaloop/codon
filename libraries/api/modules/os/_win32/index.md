# os.\_win32

Standard library module [View source](https://github.com/exaloop/codon/blob/master/stdlib/os/_win32.codon)

Windows backend for `os`; native little-endian Windows x64/ARM64 only.

All foreign declarations are local to functions and nothing calls Windows at module initialization. The public `os` module supplies compile-time dispatch and the `environ` mapping. This module does not import `os` or port the compiler/runtime. Called functions require Kernel32 and a compatible Microsoft UCRT; no new C++ runtime helpers are needed.

Paths, commands and environment use UTF-16LE with `surrogatepass`, never ANSI. `getcwdb()` uses UTF-8 with `surrogatepass`, as modern Python on Windows does. Extended-length paths are accepted as supplied; no automatic long-path prefix/manifest is added. `mkdir()` ignores `mode` and inherits the parent's Windows security descriptor (even for `0o700`); `chmod()` changes only the read-only attribute. `access()` is an attribute probe, not an ACL check, and `X_OK` means existence. Descriptor I/O retains the descriptor's CRT text/binary mode and can return partial transfers.

Environment updates are not atomic across the CRT and OS and must be externally synchronized with other environment writers. The CRT treats an empty value as deletion; `putenv()` restores an actual empty OS value afterwards, but cannot make CRT `getenv()` distinguish empty from missing. Other CRT instances/ANSI mirrors cannot be kept fully Unicode-consistent. No snapshot is updated here.

`OSError` carries `errno`, `strerror`, applicable filenames and the original `winerror` for Windows API failures. CRT failures supply `errno` without inventing `winerror`. Toolhelp reports the recorded parent PID, which can have exited or been reused. This backend has not been executed on Windows as part of its implementation.

function stat[](https://docs.exaloop.io/libraries/api/modules/os/_win32/#function-stat "Copy/link to this API element")

```python
stat(path: str, follow_symlinks: bool = True)
```

function fstat[](https://docs.exaloop.io/libraries/api/modules/os/_win32/#function-fstat "Copy/link to this API element")

```python
fstat(fd: int)
```

function open[](https://docs.exaloop.io/libraries/api/modules/os/_win32/#function-open "Copy/link to this API element")

```python
open(path: str, flags: int, mode: int = 0o777)
```

function listdir[](https://docs.exaloop.io/libraries/api/modules/os/_win32/#function-listdir "Copy/link to this API element")

```python
listdir(path: str = ".")
```

function realpath[](https://docs.exaloop.io/libraries/api/modules/os/_win32/#function-realpath "Copy/link to this API element")

```python
realpath(path: str, strict: bool = False)
```

function make\_env\_map[](https://docs.exaloop.io/libraries/api/modules/os/_win32/#function-make_env_map "Copy/link to this API element")

```python
make_env_map()
```

Return an uppercase-key snapshot of the actual Windows environment.

function putenv[](https://docs.exaloop.io/libraries/api/modules/os/_win32/#function-putenv "Copy/link to this API element")

```python
putenv(key: str, value: str)
```

Update CRT and OS environment, but not the public `environ` snapshot.

function unsetenv[](https://docs.exaloop.io/libraries/api/modules/os/_win32/#function-unsetenv "Copy/link to this API element")

```python
unsetenv(key: str)
```

function process\_getenv[](https://docs.exaloop.io/libraries/api/modules/os/_win32/#function-process_getenv "Copy/link to this API element")

```python
process_getenv(key: str)
```

Query the true OS environment, distinguishing empty from missing.

function system[](https://docs.exaloop.io/libraries/api/modules/os/_win32/#function-system "Copy/link to this API element")

```python
system(cmd: str)
```

Run the Windows command processor; return its native status, not wait bits.

function mkdir[](https://docs.exaloop.io/libraries/api/modules/os/_win32/#function-mkdir "Copy/link to this API element")

```python
mkdir(path: str, mode: int = 0o777)
```

function rmdir[](https://docs.exaloop.io/libraries/api/modules/os/_win32/#function-rmdir "Copy/link to this API element")

```python
rmdir(path: str)
```

function chdir[](https://docs.exaloop.io/libraries/api/modules/os/_win32/#function-chdir "Copy/link to this API element")

```python
chdir(path: str)
```

function getcwd[](https://docs.exaloop.io/libraries/api/modules/os/_win32/#function-getcwd "Copy/link to this API element")

```python
getcwd()
```

function getcwdb[](https://docs.exaloop.io/libraries/api/modules/os/_win32/#function-getcwdb "Copy/link to this API element")

```python
getcwdb()
```

function unlink[](https://docs.exaloop.io/libraries/api/modules/os/_win32/#function-unlink "Copy/link to this API element")

```python
unlink(path: str)
```

function remove[](https://docs.exaloop.io/libraries/api/modules/os/_win32/#function-remove "Copy/link to this API element")

```python
remove(path: str)
```

function rename[](https://docs.exaloop.io/libraries/api/modules/os/_win32/#function-rename "Copy/link to this API element")

```python
rename(src: str, dst: str)
```

function replace[](https://docs.exaloop.io/libraries/api/modules/os/_win32/#function-replace "Copy/link to this API element")

```python
replace(src: str, dst: str)
```

function link[](https://docs.exaloop.io/libraries/api/modules/os/_win32/#function-link "Copy/link to this API element")

```python
link(src: str, dst: str)
```

function symlink[](https://docs.exaloop.io/libraries/api/modules/os/_win32/#function-symlink "Copy/link to this API element")

```python
symlink(src: str, dst: str, target_is_directory: bool = False)
```

function readlink[](https://docs.exaloop.io/libraries/api/modules/os/_win32/#function-readlink "Copy/link to this API element")

```python
readlink(path: str)
```

function access[](https://docs.exaloop.io/libraries/api/modules/os/_win32/#function-access "Copy/link to this API element")

```python
access(path: str, mode: int)
```

function chmod[](https://docs.exaloop.io/libraries/api/modules/os/_win32/#function-chmod "Copy/link to this API element")

```python
chmod(path: str, mode: int)
```

function getpid[](https://docs.exaloop.io/libraries/api/modules/os/_win32/#function-getpid "Copy/link to this API element")

```python
getpid()
```

function getppid[](https://docs.exaloop.io/libraries/api/modules/os/_win32/#function-getppid "Copy/link to this API element")

```python
getppid()
```

function close[](https://docs.exaloop.io/libraries/api/modules/os/_win32/#function-close "Copy/link to this API element")

```python
close(fd: int)
```

function read[](https://docs.exaloop.io/libraries/api/modules/os/_win32/#function-read "Copy/link to this API element")

```python
read(fd: int, n: int)
```

function write[](https://docs.exaloop.io/libraries/api/modules/os/_win32/#function-write "Copy/link to this API element")

```python
write(fd: int, data: bytes)
```

function lseek[](https://docs.exaloop.io/libraries/api/modules/os/_win32/#function-lseek "Copy/link to this API element")

```python
lseek(fd: int, pos: int, how: int)
```

function fsync[](https://docs.exaloop.io/libraries/api/modules/os/_win32/#function-fsync "Copy/link to this API element")

```python
fsync(fd: int)
```

function ftruncate[](https://docs.exaloop.io/libraries/api/modules/os/_win32/#function-ftruncate "Copy/link to this API element")

```python
ftruncate(fd: int, length: int)
```

function truncate[](https://docs.exaloop.io/libraries/api/modules/os/_win32/#function-truncate "Copy/link to this API element")

```python
truncate(path: str, length: int)
```

function getfullpathname[](https://docs.exaloop.io/libraries/api/modules/os/_win32/#function-getfullpathname "Copy/link to this API element")

```python
getfullpathname(path: str)
```

Resolve Windows drive-relative/UNC syntax without requiring existence.

Source: https://docs.exaloop.io/libraries/api/modules/os/_win32/
