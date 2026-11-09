# os

Standard library package [View source](https://github.com/exaloop/codon/blob/master/stdlib/os/__init__.codon)

Native operating-system interfaces for POSIX and Windows.

POSIX paths and environment use UTF-8 with `surrogateescape` to preserve arbitrary bytes; Windows uses wide-character APIs. Filesystem APIs currently accept `str` paths only; `fsencode()` and `fsdecode()` additionally accept `bytes` and `str` identity inputs, respectively.

Failures raise `OSError` with `errno`, a system diagnostic and applicable filenames (plus `winerror` for Windows API failures). File-descriptor reads and writes may transfer fewer bytes than requested. Windows permissions and available process APIs differ from POSIX.

variable curdir[](https://docs.exaloop.io/libraries/api/modules/os/#variable-curdir "Copy/link to this API element")

```python
curdir: Literal[str] = "."
```

variable pardir[](https://docs.exaloop.io/libraries/api/modules/os/#variable-pardir "Copy/link to this API element")

```python
pardir: Literal[str] = ".."
```

variable extsep[](https://docs.exaloop.io/libraries/api/modules/os/#variable-extsep "Copy/link to this API element")

```python
extsep: Literal[str] = "."
```

variable SEEK\_SET[](https://docs.exaloop.io/libraries/api/modules/os/#variable-seek_set "Copy/link to this API element")

```python
SEEK_SET: Literal[int] = 0
```

variable SEEK\_CUR[](https://docs.exaloop.io/libraries/api/modules/os/#variable-seek_cur "Copy/link to this API element")

```python
SEEK_CUR: Literal[int] = 1
```

variable SEEK\_END[](https://docs.exaloop.io/libraries/api/modules/os/#variable-seek_end "Copy/link to this API element")

```python
SEEK_END: Literal[int] = 2
```

variable F\_OK[](https://docs.exaloop.io/libraries/api/modules/os/#variable-f_ok "Copy/link to this API element")

```python
F_OK: Literal[int] = 0
```

variable R\_OK[](https://docs.exaloop.io/libraries/api/modules/os/#variable-r_ok "Copy/link to this API element")

```python
R_OK: Literal[int] = 4
```

variable W\_OK[](https://docs.exaloop.io/libraries/api/modules/os/#variable-w_ok "Copy/link to this API element")

```python
W_OK: Literal[int] = 2
```

variable X\_OK[](https://docs.exaloop.io/libraries/api/modules/os/#variable-x_ok "Copy/link to this API element")

```python
X_OK: Literal[int] = 1
```

variable O\_RDONLY[](https://docs.exaloop.io/libraries/api/modules/os/#variable-o_rdonly "Copy/link to this API element")

```python
O_RDONLY: Literal[int] = 0
```

variable O\_WRONLY[](https://docs.exaloop.io/libraries/api/modules/os/#variable-o_wronly "Copy/link to this API element")

```python
O_WRONLY: Literal[int] = 1
```

variable O\_RDWR[](https://docs.exaloop.io/libraries/api/modules/os/#variable-o_rdwr "Copy/link to this API element")

```python
O_RDWR: Literal[int] = 2
```

function fspath[](https://docs.exaloop.io/libraries/api/modules/os/#function-fspath "Copy/link to this API element")

```python
fspath(path)
```

function fsencode[](https://docs.exaloop.io/libraries/api/modules/os/#function-fsencode "Copy/link to this API element")

```python
fsencode(filename: str)
```

@overload

```python
fsencode(filename: bytes)
```

function fsdecode[](https://docs.exaloop.io/libraries/api/modules/os/#function-fsdecode "Copy/link to this API element")

```python
fsdecode(filename: bytes)
```

@overload

```python
fsdecode(filename: str)
```

function system[](https://docs.exaloop.io/libraries/api/modules/os/#function-system "Copy/link to this API element")

```python
system(cmd: str)
```

function putenv[](https://docs.exaloop.io/libraries/api/modules/os/#function-putenv "Copy/link to this API element")

```python
putenv(key: str, value: str)
```

Set the process environment without updating the `environ` mapping.

function unsetenv[](https://docs.exaloop.io/libraries/api/modules/os/#function-unsetenv "Copy/link to this API element")

```python
unsetenv(key: str)
```

Remove a process variable without updating the `environ` mapping.

variable environ[](https://docs.exaloop.io/libraries/api/modules/os/#variable-environ "Copy/link to this API element")

```python
environ = _Environ(_make_env_map(), copy=False)
```

function getenv[](https://docs.exaloop.io/libraries/api/modules/os/#function-getenv "Copy/link to this API element")

```python
getenv(key: str, default = None)
```

class stat\_result[](https://docs.exaloop.io/libraries/api/modules/os/#class-stat_result "Copy/link to this API element")

```python
class stat_result
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/os/stat_result/)

Fields 14

`st_mode` `int`

`st_ino` `int`

`st_dev` `int`

`st_nlink` `int`

`st_uid` `int`

`st_gid` `int`

`st_size` `int`

`st_atime` `float`

`st_mtime` `float`

`st_ctime` `float`

`st_atime_ns` `int`

`st_mtime_ns` `int`

`st_ctime_ns` `int`

`st_rdev` `int`

Constructors 2

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/os/#class-stat_result-__init__ "Copy/link to this API element")

```python
__init__(self: Self, fields: List[int], Self: type)
```

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/os/#class-stat_result-__init__-overload-2 "Copy/link to this API element")

```python
__init__(self: Self, fields: Ptr[int], Self: type)
```

Special methods 4

\_\_len\_\_[](https://docs.exaloop.io/libraries/api/modules/os/#class-stat_result-__len__ "Copy/link to this API element")

```python
__len__(self: Self, Self: type)
```

\_\_getitem\_\_[](https://docs.exaloop.io/libraries/api/modules/os/#class-stat_result-__getitem__ "Copy/link to this API element")

```python
__getitem__(self: Self, index: int, Self: type)
```

\_\_iter\_\_[](https://docs.exaloop.io/libraries/api/modules/os/#class-stat_result-__iter__ "Copy/link to this API element")

```python
__iter__(self: Self, Self: type)
```

\_\_repr\_\_[](https://docs.exaloop.io/libraries/api/modules/os/#class-stat_result-__repr__ "Copy/link to this API element")

```python
__repr__(self: Self, Self: type)
```

function stat[](https://docs.exaloop.io/libraries/api/modules/os/#function-stat "Copy/link to this API element")

```python
stat(path: str, follow_symlinks: bool = True)
```

@overload

```python
stat(fd: int)
```

function fstat[](https://docs.exaloop.io/libraries/api/modules/os/#function-fstat "Copy/link to this API element")

```python
fstat(fd: int)
```

function lstat[](https://docs.exaloop.io/libraries/api/modules/os/#function-lstat "Copy/link to this API element")

```python
lstat(path: str)
```

function open[](https://docs.exaloop.io/libraries/api/modules/os/#function-open "Copy/link to this API element")

```python
open(path: str, flags: int, mode: int = 0o777)
```

function listdir[](https://docs.exaloop.io/libraries/api/modules/os/#function-listdir "Copy/link to this API element")

```python
listdir(path: str = ".")
```

function makedirs[](https://docs.exaloop.io/libraries/api/modules/os/#function-makedirs "Copy/link to this API element")

```python
makedirs(name: str, mode: int = 0o777, exist_ok: bool = False)
```

function removedirs[](https://docs.exaloop.io/libraries/api/modules/os/#function-removedirs "Copy/link to this API element")

```python
removedirs(name: str)
```

function mkdir[](https://docs.exaloop.io/libraries/api/modules/os/#function-mkdir "Copy/link to this API element")

```python
mkdir(path: str, mode: int = 0o777)
```

function rmdir[](https://docs.exaloop.io/libraries/api/modules/os/#function-rmdir "Copy/link to this API element")

```python
rmdir(path: str)
```

function chdir[](https://docs.exaloop.io/libraries/api/modules/os/#function-chdir "Copy/link to this API element")

```python
chdir(path: str)
```

function getcwdb[](https://docs.exaloop.io/libraries/api/modules/os/#function-getcwdb "Copy/link to this API element")

```python
getcwdb()
```

function getcwd[](https://docs.exaloop.io/libraries/api/modules/os/#function-getcwd "Copy/link to this API element")

```python
getcwd()
```

function unlink[](https://docs.exaloop.io/libraries/api/modules/os/#function-unlink "Copy/link to this API element")

```python
unlink(path: str)
```

function remove[](https://docs.exaloop.io/libraries/api/modules/os/#function-remove "Copy/link to this API element")

```python
remove(path: str)
```

function rename[](https://docs.exaloop.io/libraries/api/modules/os/#function-rename "Copy/link to this API element")

```python
rename(src: str, dst: str)
```

function replace[](https://docs.exaloop.io/libraries/api/modules/os/#function-replace "Copy/link to this API element")

```python
replace(src: str, dst: str)
```

function link[](https://docs.exaloop.io/libraries/api/modules/os/#function-link "Copy/link to this API element")

```python
link(src: str, dst: str)
```

function symlink[](https://docs.exaloop.io/libraries/api/modules/os/#function-symlink "Copy/link to this API element")

```python
symlink(src: str, dst: str, target_is_directory: bool = False)
```

function readlink[](https://docs.exaloop.io/libraries/api/modules/os/#function-readlink "Copy/link to this API element")

```python
readlink(path: str)
```

function access[](https://docs.exaloop.io/libraries/api/modules/os/#function-access "Copy/link to this API element")

```python
access(path: str, mode: int)
```

function chmod[](https://docs.exaloop.io/libraries/api/modules/os/#function-chmod "Copy/link to this API element")

```python
chmod(path: str, mode: int, follow_symlinks: bool = True)
```

function getpid[](https://docs.exaloop.io/libraries/api/modules/os/#function-getpid "Copy/link to this API element")

```python
getpid()
```

function getppid[](https://docs.exaloop.io/libraries/api/modules/os/#function-getppid "Copy/link to this API element")

```python
getppid()
```

function close[](https://docs.exaloop.io/libraries/api/modules/os/#function-close "Copy/link to this API element")

```python
close(fd: int)
```

function read[](https://docs.exaloop.io/libraries/api/modules/os/#function-read "Copy/link to this API element")

```python
read(fd: int, n: int)
```

function write[](https://docs.exaloop.io/libraries/api/modules/os/#function-write "Copy/link to this API element")

```python
write(fd: int, data: bytes)
```

function lseek[](https://docs.exaloop.io/libraries/api/modules/os/#function-lseek "Copy/link to this API element")

```python
lseek(fd: int, pos: int, how: int)
```

function fsync[](https://docs.exaloop.io/libraries/api/modules/os/#function-fsync "Copy/link to this API element")

```python
fsync(fd: int)
```

function ftruncate[](https://docs.exaloop.io/libraries/api/modules/os/#function-ftruncate "Copy/link to this API element")

```python
ftruncate(fd: int, length: int)
```

function truncate[](https://docs.exaloop.io/libraries/api/modules/os/#function-truncate "Copy/link to this API element")

```python
truncate(path: str, length: int)
```

Modules 4

[`_ntpath` →](https://docs.exaloop.io/libraries/api/modules/os/_ntpath/) [`_status` →](https://docs.exaloop.io/libraries/api/modules/os/_status/) [`_win32` →](https://docs.exaloop.io/libraries/api/modules/os/_win32/) [`path` →](https://docs.exaloop.io/libraries/api/modules/os/path/)

Source: https://docs.exaloop.io/libraries/api/modules/os/
