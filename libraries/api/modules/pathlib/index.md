# pathlib

Standard library Module [View source](https://github.com/exaloop/codon/blob/master/stdlib/pathlib.codon)

Native filesystem paths represented by the immutable `Path` tuple class.

Lexical operations preserve `..` components. Methods that change the filesystem do not change the stored path.

class Path[](https://docs.exaloop.io/libraries/api/modules/pathlib/#class-path "Copy/link to this API element")

@tuple

```python
class Path
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/pathlib/Path/)

Constructors 1

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/pathlib/#class-path-__new__ "Copy/link to this API element")

```python
__new__(*segments) -> Path
```

Properties 10

drive[](https://docs.exaloop.io/libraries/api/modules/pathlib/#class-path-drive "Copy/link to this API element")

@property

```python
drive(self: Self) -> str
```

root[](https://docs.exaloop.io/libraries/api/modules/pathlib/#class-path-root "Copy/link to this API element")

@property

```python
root(self: Self) -> str
```

anchor[](https://docs.exaloop.io/libraries/api/modules/pathlib/#class-path-anchor "Copy/link to this API element")

@property

```python
anchor(self: Self) -> str
```

parts[](https://docs.exaloop.io/libraries/api/modules/pathlib/#class-path-parts "Copy/link to this API element")

@property

```python
parts(self: Self) -> DynamicTuple[str]
```

name[](https://docs.exaloop.io/libraries/api/modules/pathlib/#class-path-name "Copy/link to this API element")

@property

```python
name(self: Self) -> str
```

suffix[](https://docs.exaloop.io/libraries/api/modules/pathlib/#class-path-suffix "Copy/link to this API element")

@property

```python
suffix(self: Self) -> str
```

suffixes[](https://docs.exaloop.io/libraries/api/modules/pathlib/#class-path-suffixes "Copy/link to this API element")

@property

```python
suffixes(self: Self) -> List[str]
```

stem[](https://docs.exaloop.io/libraries/api/modules/pathlib/#class-path-stem "Copy/link to this API element")

@property

```python
stem(self: Self) -> str
```

parent[](https://docs.exaloop.io/libraries/api/modules/pathlib/#class-path-parent "Copy/link to this API element")

@property

```python
parent(self: Self)
```

parents[](https://docs.exaloop.io/libraries/api/modules/pathlib/#class-path-parents "Copy/link to this API element")

@property

```python
parents(self: Self) -> DynamicTuple[Path]
```

Methods 48

joinpath[](https://docs.exaloop.io/libraries/api/modules/pathlib/#class-path-joinpath "Copy/link to this API element")

```python
joinpath(self: Self, *segments)
```

as\_posix[](https://docs.exaloop.io/libraries/api/modules/pathlib/#class-path-as_posix "Copy/link to this API element")

```python
as_posix(self: Self) -> str
```

with\_name[](https://docs.exaloop.io/libraries/api/modules/pathlib/#class-path-with_name "Copy/link to this API element")

```python
with_name(self: Self, name: str)
```

with\_stem[](https://docs.exaloop.io/libraries/api/modules/pathlib/#class-path-with_stem "Copy/link to this API element")

```python
with_stem(self: Self, stem: str)
```

with\_suffix[](https://docs.exaloop.io/libraries/api/modules/pathlib/#class-path-with_suffix "Copy/link to this API element")

```python
with_suffix(self: Self, suffix: str)
```

is\_absolute[](https://docs.exaloop.io/libraries/api/modules/pathlib/#class-path-is_absolute "Copy/link to this API element")

```python
is_absolute(self: Self) -> bool
```

is\_relative\_to[](https://docs.exaloop.io/libraries/api/modules/pathlib/#class-path-is_relative_to "Copy/link to this API element")

```python
is_relative_to(self: Self, other) -> bool
```

relative\_to[](https://docs.exaloop.io/libraries/api/modules/pathlib/#class-path-relative_to "Copy/link to this API element")

```python
relative_to(self: Self, other, *, walk_up: bool = False)
```

cwd[](https://docs.exaloop.io/libraries/api/modules/pathlib/#class-path-cwd "Copy/link to this API element")

```python
cwd()
```

home[](https://docs.exaloop.io/libraries/api/modules/pathlib/#class-path-home "Copy/link to this API element")

```python
home()
```

absolute[](https://docs.exaloop.io/libraries/api/modules/pathlib/#class-path-absolute "Copy/link to this API element")

```python
absolute(self: Self)
```

resolve[](https://docs.exaloop.io/libraries/api/modules/pathlib/#class-path-resolve "Copy/link to this API element")

```python
resolve(self: Self, strict: bool = False)
```

expanduser[](https://docs.exaloop.io/libraries/api/modules/pathlib/#class-path-expanduser "Copy/link to this API element")

```python
expanduser(self: Self)
```

as\_uri[](https://docs.exaloop.io/libraries/api/modules/pathlib/#class-path-as_uri "Copy/link to this API element")

```python
as_uri(self: Self) -> str
```

stat[](https://docs.exaloop.io/libraries/api/modules/pathlib/#class-path-stat "Copy/link to this API element")

```python
stat(self: Self, *, follow_symlinks: bool = True)
```

lstat[](https://docs.exaloop.io/libraries/api/modules/pathlib/#class-path-lstat "Copy/link to this API element")

```python
lstat(self: Self)
```

exists[](https://docs.exaloop.io/libraries/api/modules/pathlib/#class-path-exists "Copy/link to this API element")

```python
exists(self: Self, *, follow_symlinks: bool = True) -> bool
```

is\_file[](https://docs.exaloop.io/libraries/api/modules/pathlib/#class-path-is_file "Copy/link to this API element")

```python
is_file(self: Self, *, follow_symlinks: bool = True) -> bool
```

is\_dir[](https://docs.exaloop.io/libraries/api/modules/pathlib/#class-path-is_dir "Copy/link to this API element")

```python
is_dir(self: Self, *, follow_symlinks: bool = True) -> bool
```

is\_symlink[](https://docs.exaloop.io/libraries/api/modules/pathlib/#class-path-is_symlink "Copy/link to this API element")

```python
is_symlink(self: Self) -> bool
```

is\_socket[](https://docs.exaloop.io/libraries/api/modules/pathlib/#class-path-is_socket "Copy/link to this API element")

```python
is_socket(self: Self) -> bool
```

is\_fifo[](https://docs.exaloop.io/libraries/api/modules/pathlib/#class-path-is_fifo "Copy/link to this API element")

```python
is_fifo(self: Self) -> bool
```

is\_block\_device[](https://docs.exaloop.io/libraries/api/modules/pathlib/#class-path-is_block_device "Copy/link to this API element")

```python
is_block_device(self: Self) -> bool
```

is\_char\_device[](https://docs.exaloop.io/libraries/api/modules/pathlib/#class-path-is_char_device "Copy/link to this API element")

```python
is_char_device(self: Self) -> bool
```

is\_mount[](https://docs.exaloop.io/libraries/api/modules/pathlib/#class-path-is_mount "Copy/link to this API element")

```python
is_mount(self: Self) -> bool
```

samefile[](https://docs.exaloop.io/libraries/api/modules/pathlib/#class-path-samefile "Copy/link to this API element")

```python
samefile(self: Self, other) -> bool
```

open[](https://docs.exaloop.io/libraries/api/modules/pathlib/#class-path-open "Copy/link to this API element")

```python
open(self: Self, mode: str = "r", encoding: Optional[str] = None, errors: Optional[str] = None, newline: Optional[str] = None)
```

```python
open[mode: Literal[str]](self: Self, encoding: Optional[str] = None, errors: Optional[str] = None, newline: Optional[str] = None)
```

read\_text[](https://docs.exaloop.io/libraries/api/modules/pathlib/#class-path-read_text "Copy/link to this API element")

```python
read_text(self: Self, encoding: Optional[str] = None, errors: Optional[str] = None, newline: Optional[str] = None) -> str
```

read\_bytes[](https://docs.exaloop.io/libraries/api/modules/pathlib/#class-path-read_bytes "Copy/link to this API element")

```python
read_bytes(self: Self) -> bytes
```

write\_text[](https://docs.exaloop.io/libraries/api/modules/pathlib/#class-path-write_text "Copy/link to this API element")

```python
write_text(self: Self, data: str, encoding: Optional[str] = None, errors: Optional[str] = None, newline: Optional[str] = None) -> int
```

write\_bytes[](https://docs.exaloop.io/libraries/api/modules/pathlib/#class-path-write_bytes "Copy/link to this API element")

```python
write_bytes(self: Self, data: bytes) -> int
```

iterdir[](https://docs.exaloop.io/libraries/api/modules/pathlib/#class-path-iterdir "Copy/link to this API element")

```python
iterdir(self: Self)
```

mkdir[](https://docs.exaloop.io/libraries/api/modules/pathlib/#class-path-mkdir "Copy/link to this API element")

```python
mkdir(self: Self, mode: int = 0o777, parents: bool = False, exist_ok: bool = False)
```

rmdir[](https://docs.exaloop.io/libraries/api/modules/pathlib/#class-path-rmdir "Copy/link to this API element")

```python
rmdir(self: Self)
```

unlink[](https://docs.exaloop.io/libraries/api/modules/pathlib/#class-path-unlink "Copy/link to this API element")

```python
unlink(self: Self, missing_ok: bool = False)
```

rename[](https://docs.exaloop.io/libraries/api/modules/pathlib/#class-path-rename "Copy/link to this API element")

```python
rename(self: Self, target)
```

replace[](https://docs.exaloop.io/libraries/api/modules/pathlib/#class-path-replace "Copy/link to this API element")

```python
replace(self: Self, target)
```

chmod[](https://docs.exaloop.io/libraries/api/modules/pathlib/#class-path-chmod "Copy/link to this API element")

```python
chmod(self: Self, mode: int, *, follow_symlinks: bool = True)
```

lchmod[](https://docs.exaloop.io/libraries/api/modules/pathlib/#class-path-lchmod "Copy/link to this API element")

```python
lchmod(self: Self, mode: int)
```

symlink\_to[](https://docs.exaloop.io/libraries/api/modules/pathlib/#class-path-symlink_to "Copy/link to this API element")

```python
symlink_to(self: Self, target, target_is_directory: bool = False)
```

hardlink\_to[](https://docs.exaloop.io/libraries/api/modules/pathlib/#class-path-hardlink_to "Copy/link to this API element")

```python
hardlink_to(self: Self, target)
```

readlink[](https://docs.exaloop.io/libraries/api/modules/pathlib/#class-path-readlink "Copy/link to this API element")

```python
readlink(self: Self)
```

touch[](https://docs.exaloop.io/libraries/api/modules/pathlib/#class-path-touch "Copy/link to this API element")

```python
touch(self: Self, mode: int = 0o666, exist_ok: bool = True)
```

match[](https://docs.exaloop.io/libraries/api/modules/pathlib/#class-path-match "Copy/link to this API element")

```python
match(self: Self, pattern, *, case_sensitive: Optional[bool] = None) -> bool
```

full\_match[](https://docs.exaloop.io/libraries/api/modules/pathlib/#class-path-full_match "Copy/link to this API element")

```python
full_match(self: Self, pattern, *, case_sensitive: Optional[bool] = None) -> bool
```

glob[](https://docs.exaloop.io/libraries/api/modules/pathlib/#class-path-glob "Copy/link to this API element")

```python
glob(self: Self, pattern, *, case_sensitive: Optional[bool] = None, recurse_symlinks: bool = False)
```

rglob[](https://docs.exaloop.io/libraries/api/modules/pathlib/#class-path-rglob "Copy/link to this API element")

```python
rglob(self: Self, pattern, *, case_sensitive: Optional[bool] = None, recurse_symlinks: bool = False)
```

walk[](https://docs.exaloop.io/libraries/api/modules/pathlib/#class-path-walk "Copy/link to this API element")

```python
walk(self: Self, top_down: bool = True, on_error = None, follow_symlinks: bool = False)
```

Special methods 12

\_\_str\_\_[](https://docs.exaloop.io/libraries/api/modules/pathlib/#class-path-__str__ "Copy/link to this API element")

```python
__str__(self: Self) -> str
```

\_\_repr\_\_[](https://docs.exaloop.io/libraries/api/modules/pathlib/#class-path-__repr__ "Copy/link to this API element")

```python
__repr__(self: Self) -> str
```

\_\_fspath\_\_[](https://docs.exaloop.io/libraries/api/modules/pathlib/#class-path-__fspath__ "Copy/link to this API element")

```python
__fspath__(self: Self) -> str
```

\_\_hash\_\_[](https://docs.exaloop.io/libraries/api/modules/pathlib/#class-path-__hash__ "Copy/link to this API element")

```python
__hash__(self: Self) -> int
```

\_\_eq\_\_[](https://docs.exaloop.io/libraries/api/modules/pathlib/#class-path-__eq__ "Copy/link to this API element")

```python
__eq__(self: Self, other) -> bool
```

\_\_ne\_\_[](https://docs.exaloop.io/libraries/api/modules/pathlib/#class-path-__ne__ "Copy/link to this API element")

```python
__ne__(self: Self, other, /) -> bool
```

\_\_lt\_\_[](https://docs.exaloop.io/libraries/api/modules/pathlib/#class-path-__lt__ "Copy/link to this API element")

```python
__lt__(self: Self, other: Path) -> bool
```

\_\_le\_\_[](https://docs.exaloop.io/libraries/api/modules/pathlib/#class-path-__le__ "Copy/link to this API element")

```python
__le__(self: Self, other: Path) -> bool
```

\_\_gt\_\_[](https://docs.exaloop.io/libraries/api/modules/pathlib/#class-path-__gt__ "Copy/link to this API element")

```python
__gt__(self: Self, other: Path) -> bool
```

\_\_ge\_\_[](https://docs.exaloop.io/libraries/api/modules/pathlib/#class-path-__ge__ "Copy/link to this API element")

```python
__ge__(self: Self, other: Path) -> bool
```

\_\_truediv\_\_[](https://docs.exaloop.io/libraries/api/modules/pathlib/#class-path-__truediv__ "Copy/link to this API element")

```python
__truediv__(self: Self, other)
```

\_\_rtruediv\_\_[](https://docs.exaloop.io/libraries/api/modules/pathlib/#class-path-__rtruediv__ "Copy/link to this API element")

```python
__rtruediv__(self: Self, other)
```

Source: https://docs.exaloop.io/libraries/api/modules/pathlib/
