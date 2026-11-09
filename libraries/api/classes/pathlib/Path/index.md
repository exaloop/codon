# Path

Standard library class `pathlib.Path` [View source](https://github.com/exaloop/codon/blob/master/stdlib/pathlib.codon)

class Path[](https://docs.exaloop.io/libraries/api/classes/pathlib/Path/#class-path "Copy/link to this API element")

@tuple

```python
class Path
```

Constructors 1

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/classes/pathlib/Path/#class-path-__new__ "Copy/link to this API element")

```python
__new__(*segments)
```

Defined in [`pathlib`](https://docs.exaloop.io/libraries/api/modules/pathlib/)

Properties 10

drive[](https://docs.exaloop.io/libraries/api/classes/pathlib/Path/#class-path-drive "Copy/link to this API element")

@property

```python
drive(self: Self, Self: type)
```

Defined in [`pathlib`](https://docs.exaloop.io/libraries/api/modules/pathlib/)

root[](https://docs.exaloop.io/libraries/api/classes/pathlib/Path/#class-path-root "Copy/link to this API element")

@property

```python
root(self: Self, Self: type)
```

Defined in [`pathlib`](https://docs.exaloop.io/libraries/api/modules/pathlib/)

anchor[](https://docs.exaloop.io/libraries/api/classes/pathlib/Path/#class-path-anchor "Copy/link to this API element")

@property

```python
anchor(self: Self, Self: type)
```

Defined in [`pathlib`](https://docs.exaloop.io/libraries/api/modules/pathlib/)

parts[](https://docs.exaloop.io/libraries/api/classes/pathlib/Path/#class-path-parts "Copy/link to this API element")

@property

```python
parts(self: Self, Self: type)
```

Defined in [`pathlib`](https://docs.exaloop.io/libraries/api/modules/pathlib/)

name[](https://docs.exaloop.io/libraries/api/classes/pathlib/Path/#class-path-name "Copy/link to this API element")

@property

```python
name(self: Self, Self: type)
```

Defined in [`pathlib`](https://docs.exaloop.io/libraries/api/modules/pathlib/)

suffix[](https://docs.exaloop.io/libraries/api/classes/pathlib/Path/#class-path-suffix "Copy/link to this API element")

@property

```python
suffix(self: Self, Self: type)
```

Defined in [`pathlib`](https://docs.exaloop.io/libraries/api/modules/pathlib/)

suffixes[](https://docs.exaloop.io/libraries/api/classes/pathlib/Path/#class-path-suffixes "Copy/link to this API element")

@property

```python
suffixes(self: Self, Self: type)
```

Defined in [`pathlib`](https://docs.exaloop.io/libraries/api/modules/pathlib/)

stem[](https://docs.exaloop.io/libraries/api/classes/pathlib/Path/#class-path-stem "Copy/link to this API element")

@property

```python
stem(self: Self, Self: type)
```

Defined in [`pathlib`](https://docs.exaloop.io/libraries/api/modules/pathlib/)

parent[](https://docs.exaloop.io/libraries/api/classes/pathlib/Path/#class-path-parent "Copy/link to this API element")

@property

```python
parent(self: Self, Self: type)
```

Defined in [`pathlib`](https://docs.exaloop.io/libraries/api/modules/pathlib/)

parents[](https://docs.exaloop.io/libraries/api/classes/pathlib/Path/#class-path-parents "Copy/link to this API element")

@property

```python
parents(self: Self, Self: type)
```

Defined in [`pathlib`](https://docs.exaloop.io/libraries/api/modules/pathlib/)

Methods 48

joinpath[](https://docs.exaloop.io/libraries/api/classes/pathlib/Path/#class-path-joinpath "Copy/link to this API element")

```python
joinpath(self: Self, *segments, Self: type)
```

Defined in [`pathlib`](https://docs.exaloop.io/libraries/api/modules/pathlib/)

as\_posix[](https://docs.exaloop.io/libraries/api/classes/pathlib/Path/#class-path-as_posix "Copy/link to this API element")

```python
as_posix(self: Self, Self: type)
```

Defined in [`pathlib`](https://docs.exaloop.io/libraries/api/modules/pathlib/)

with\_name[](https://docs.exaloop.io/libraries/api/classes/pathlib/Path/#class-path-with_name "Copy/link to this API element")

```python
with_name(self: Self, name: str, Self: type)
```

Defined in [`pathlib`](https://docs.exaloop.io/libraries/api/modules/pathlib/)

with\_stem[](https://docs.exaloop.io/libraries/api/classes/pathlib/Path/#class-path-with_stem "Copy/link to this API element")

```python
with_stem(self: Self, stem: str, Self: type)
```

Defined in [`pathlib`](https://docs.exaloop.io/libraries/api/modules/pathlib/)

with\_suffix[](https://docs.exaloop.io/libraries/api/classes/pathlib/Path/#class-path-with_suffix "Copy/link to this API element")

```python
with_suffix(self: Self, suffix: str, Self: type)
```

Defined in [`pathlib`](https://docs.exaloop.io/libraries/api/modules/pathlib/)

is\_absolute[](https://docs.exaloop.io/libraries/api/classes/pathlib/Path/#class-path-is_absolute "Copy/link to this API element")

```python
is_absolute(self: Self, Self: type)
```

Defined in [`pathlib`](https://docs.exaloop.io/libraries/api/modules/pathlib/)

is\_relative\_to[](https://docs.exaloop.io/libraries/api/classes/pathlib/Path/#class-path-is_relative_to "Copy/link to this API element")

```python
is_relative_to(self: Self, other, Self: type)
```

Defined in [`pathlib`](https://docs.exaloop.io/libraries/api/modules/pathlib/)

relative\_to[](https://docs.exaloop.io/libraries/api/classes/pathlib/Path/#class-path-relative_to "Copy/link to this API element")

```python
relative_to(self: Self, other, walk_up: bool = False, Self: type)
```

Defined in [`pathlib`](https://docs.exaloop.io/libraries/api/modules/pathlib/)

cwd[](https://docs.exaloop.io/libraries/api/classes/pathlib/Path/#class-path-cwd "Copy/link to this API element")

```python
cwd()
```

Defined in [`pathlib`](https://docs.exaloop.io/libraries/api/modules/pathlib/)

home[](https://docs.exaloop.io/libraries/api/classes/pathlib/Path/#class-path-home "Copy/link to this API element")

```python
home()
```

Defined in [`pathlib`](https://docs.exaloop.io/libraries/api/modules/pathlib/)

absolute[](https://docs.exaloop.io/libraries/api/classes/pathlib/Path/#class-path-absolute "Copy/link to this API element")

```python
absolute(self: Self, Self: type)
```

Defined in [`pathlib`](https://docs.exaloop.io/libraries/api/modules/pathlib/)

resolve[](https://docs.exaloop.io/libraries/api/classes/pathlib/Path/#class-path-resolve "Copy/link to this API element")

```python
resolve(self: Self, strict: bool = False, Self: type)
```

Defined in [`pathlib`](https://docs.exaloop.io/libraries/api/modules/pathlib/)

expanduser[](https://docs.exaloop.io/libraries/api/classes/pathlib/Path/#class-path-expanduser "Copy/link to this API element")

```python
expanduser(self: Self, Self: type)
```

Defined in [`pathlib`](https://docs.exaloop.io/libraries/api/modules/pathlib/)

as\_uri[](https://docs.exaloop.io/libraries/api/classes/pathlib/Path/#class-path-as_uri "Copy/link to this API element")

```python
as_uri(self: Self, Self: type)
```

Defined in [`pathlib`](https://docs.exaloop.io/libraries/api/modules/pathlib/)

stat[](https://docs.exaloop.io/libraries/api/classes/pathlib/Path/#class-path-stat "Copy/link to this API element")

```python
stat(self: Self, follow_symlinks: bool = True, Self: type)
```

Defined in [`pathlib`](https://docs.exaloop.io/libraries/api/modules/pathlib/)

lstat[](https://docs.exaloop.io/libraries/api/classes/pathlib/Path/#class-path-lstat "Copy/link to this API element")

```python
lstat(self: Self, Self: type)
```

Defined in [`pathlib`](https://docs.exaloop.io/libraries/api/modules/pathlib/)

exists[](https://docs.exaloop.io/libraries/api/classes/pathlib/Path/#class-path-exists "Copy/link to this API element")

```python
exists(self: Self, follow_symlinks: bool = True, Self: type)
```

Defined in [`pathlib`](https://docs.exaloop.io/libraries/api/modules/pathlib/)

is\_file[](https://docs.exaloop.io/libraries/api/classes/pathlib/Path/#class-path-is_file "Copy/link to this API element")

```python
is_file(self: Self, follow_symlinks: bool = True, Self: type)
```

Defined in [`pathlib`](https://docs.exaloop.io/libraries/api/modules/pathlib/)

is\_dir[](https://docs.exaloop.io/libraries/api/classes/pathlib/Path/#class-path-is_dir "Copy/link to this API element")

```python
is_dir(self: Self, follow_symlinks: bool = True, Self: type)
```

Defined in [`pathlib`](https://docs.exaloop.io/libraries/api/modules/pathlib/)

is\_symlink[](https://docs.exaloop.io/libraries/api/classes/pathlib/Path/#class-path-is_symlink "Copy/link to this API element")

```python
is_symlink(self: Self, Self: type)
```

Defined in [`pathlib`](https://docs.exaloop.io/libraries/api/modules/pathlib/)

is\_socket[](https://docs.exaloop.io/libraries/api/classes/pathlib/Path/#class-path-is_socket "Copy/link to this API element")

```python
is_socket(self: Self, Self: type)
```

Defined in [`pathlib`](https://docs.exaloop.io/libraries/api/modules/pathlib/)

is\_fifo[](https://docs.exaloop.io/libraries/api/classes/pathlib/Path/#class-path-is_fifo "Copy/link to this API element")

```python
is_fifo(self: Self, Self: type)
```

Defined in [`pathlib`](https://docs.exaloop.io/libraries/api/modules/pathlib/)

is\_block\_device[](https://docs.exaloop.io/libraries/api/classes/pathlib/Path/#class-path-is_block_device "Copy/link to this API element")

```python
is_block_device(self: Self, Self: type)
```

Defined in [`pathlib`](https://docs.exaloop.io/libraries/api/modules/pathlib/)

is\_char\_device[](https://docs.exaloop.io/libraries/api/classes/pathlib/Path/#class-path-is_char_device "Copy/link to this API element")

```python
is_char_device(self: Self, Self: type)
```

Defined in [`pathlib`](https://docs.exaloop.io/libraries/api/modules/pathlib/)

is\_mount[](https://docs.exaloop.io/libraries/api/classes/pathlib/Path/#class-path-is_mount "Copy/link to this API element")

```python
is_mount(self: Self, Self: type)
```

Defined in [`pathlib`](https://docs.exaloop.io/libraries/api/modules/pathlib/)

samefile[](https://docs.exaloop.io/libraries/api/classes/pathlib/Path/#class-path-samefile "Copy/link to this API element")

```python
samefile(self: Self, other, Self: type)
```

Defined in [`pathlib`](https://docs.exaloop.io/libraries/api/modules/pathlib/)

open[](https://docs.exaloop.io/libraries/api/classes/pathlib/Path/#class-path-open "Copy/link to this API element")

```python
open(self: Self, mode: str = "r", encoding: Optional[str] = None, errors: Optional[str] = None, newline: Optional[str] = None, Self: type)
```

Defined in [`pathlib`](https://docs.exaloop.io/libraries/api/modules/pathlib/)

```python
open(self: Self, mode: Literal[str], encoding: Optional[str] = None, errors: Optional[str] = None, newline: Optional[str] = None, Self: type)
```

Defined in [`pathlib`](https://docs.exaloop.io/libraries/api/modules/pathlib/)

read\_text[](https://docs.exaloop.io/libraries/api/classes/pathlib/Path/#class-path-read_text "Copy/link to this API element")

```python
read_text(self: Self, encoding: Optional[str] = None, errors: Optional[str] = None, newline: Optional[str] = None, Self: type)
```

Defined in [`pathlib`](https://docs.exaloop.io/libraries/api/modules/pathlib/)

read\_bytes[](https://docs.exaloop.io/libraries/api/classes/pathlib/Path/#class-path-read_bytes "Copy/link to this API element")

```python
read_bytes(self: Self, Self: type)
```

Defined in [`pathlib`](https://docs.exaloop.io/libraries/api/modules/pathlib/)

write\_text[](https://docs.exaloop.io/libraries/api/classes/pathlib/Path/#class-path-write_text "Copy/link to this API element")

```python
write_text(self: Self, data: str, encoding: Optional[str] = None, errors: Optional[str] = None, newline: Optional[str] = None, Self: type)
```

Defined in [`pathlib`](https://docs.exaloop.io/libraries/api/modules/pathlib/)

write\_bytes[](https://docs.exaloop.io/libraries/api/classes/pathlib/Path/#class-path-write_bytes "Copy/link to this API element")

```python
write_bytes(self: Self, data: bytes, Self: type)
```

Defined in [`pathlib`](https://docs.exaloop.io/libraries/api/modules/pathlib/)

iterdir[](https://docs.exaloop.io/libraries/api/classes/pathlib/Path/#class-path-iterdir "Copy/link to this API element")

```python
iterdir(self: Self, Self: type)
```

Defined in [`pathlib`](https://docs.exaloop.io/libraries/api/modules/pathlib/)

mkdir[](https://docs.exaloop.io/libraries/api/classes/pathlib/Path/#class-path-mkdir "Copy/link to this API element")

```python
mkdir(self: Self, mode: int = 0o777, parents: bool = False, exist_ok: bool = False, Self: type)
```

Defined in [`pathlib`](https://docs.exaloop.io/libraries/api/modules/pathlib/)

rmdir[](https://docs.exaloop.io/libraries/api/classes/pathlib/Path/#class-path-rmdir "Copy/link to this API element")

```python
rmdir(self: Self, Self: type)
```

Defined in [`pathlib`](https://docs.exaloop.io/libraries/api/modules/pathlib/)

unlink[](https://docs.exaloop.io/libraries/api/classes/pathlib/Path/#class-path-unlink "Copy/link to this API element")

```python
unlink(self: Self, missing_ok: bool = False, Self: type)
```

Defined in [`pathlib`](https://docs.exaloop.io/libraries/api/modules/pathlib/)

rename[](https://docs.exaloop.io/libraries/api/classes/pathlib/Path/#class-path-rename "Copy/link to this API element")

```python
rename(self: Self, target, Self: type)
```

Defined in [`pathlib`](https://docs.exaloop.io/libraries/api/modules/pathlib/)

replace[](https://docs.exaloop.io/libraries/api/classes/pathlib/Path/#class-path-replace "Copy/link to this API element")

```python
replace(self: Self, target, Self: type)
```

Defined in [`pathlib`](https://docs.exaloop.io/libraries/api/modules/pathlib/)

chmod[](https://docs.exaloop.io/libraries/api/classes/pathlib/Path/#class-path-chmod "Copy/link to this API element")

```python
chmod(self: Self, mode: int, follow_symlinks: bool = True, Self: type)
```

Defined in [`pathlib`](https://docs.exaloop.io/libraries/api/modules/pathlib/)

lchmod[](https://docs.exaloop.io/libraries/api/classes/pathlib/Path/#class-path-lchmod "Copy/link to this API element")

```python
lchmod(self: Self, mode: int, Self: type)
```

Defined in [`pathlib`](https://docs.exaloop.io/libraries/api/modules/pathlib/)

symlink\_to[](https://docs.exaloop.io/libraries/api/classes/pathlib/Path/#class-path-symlink_to "Copy/link to this API element")

```python
symlink_to(self: Self, target, target_is_directory: bool = False, Self: type)
```

Defined in [`pathlib`](https://docs.exaloop.io/libraries/api/modules/pathlib/)

hardlink\_to[](https://docs.exaloop.io/libraries/api/classes/pathlib/Path/#class-path-hardlink_to "Copy/link to this API element")

```python
hardlink_to(self: Self, target, Self: type)
```

Defined in [`pathlib`](https://docs.exaloop.io/libraries/api/modules/pathlib/)

readlink[](https://docs.exaloop.io/libraries/api/classes/pathlib/Path/#class-path-readlink "Copy/link to this API element")

```python
readlink(self: Self, Self: type)
```

Defined in [`pathlib`](https://docs.exaloop.io/libraries/api/modules/pathlib/)

touch[](https://docs.exaloop.io/libraries/api/classes/pathlib/Path/#class-path-touch "Copy/link to this API element")

```python
touch(self: Self, mode: int = 0o666, exist_ok: bool = True, Self: type)
```

Defined in [`pathlib`](https://docs.exaloop.io/libraries/api/modules/pathlib/)

match[](https://docs.exaloop.io/libraries/api/classes/pathlib/Path/#class-path-match "Copy/link to this API element")

```python
match(self: Self, pattern, case_sensitive: Optional[bool] = None, Self: type)
```

Defined in [`pathlib`](https://docs.exaloop.io/libraries/api/modules/pathlib/)

full\_match[](https://docs.exaloop.io/libraries/api/classes/pathlib/Path/#class-path-full_match "Copy/link to this API element")

```python
full_match(self: Self, pattern, case_sensitive: Optional[bool] = None, Self: type)
```

Defined in [`pathlib`](https://docs.exaloop.io/libraries/api/modules/pathlib/)

glob[](https://docs.exaloop.io/libraries/api/classes/pathlib/Path/#class-path-glob "Copy/link to this API element")

```python
glob(self: Self, pattern, case_sensitive: Optional[bool] = None, recurse_symlinks: bool = False, Self: type)
```

Defined in [`pathlib`](https://docs.exaloop.io/libraries/api/modules/pathlib/)

rglob[](https://docs.exaloop.io/libraries/api/classes/pathlib/Path/#class-path-rglob "Copy/link to this API element")

```python
rglob(self: Self, pattern, case_sensitive: Optional[bool] = None, recurse_symlinks: bool = False, Self: type)
```

Defined in [`pathlib`](https://docs.exaloop.io/libraries/api/modules/pathlib/)

walk[](https://docs.exaloop.io/libraries/api/classes/pathlib/Path/#class-path-walk "Copy/link to this API element")

```python
walk(self: Self, top_down: bool = True, on_error = None, follow_symlinks: bool = False, Self: type)
```

Defined in [`pathlib`](https://docs.exaloop.io/libraries/api/modules/pathlib/)

Special methods 12

\_\_str\_\_[](https://docs.exaloop.io/libraries/api/classes/pathlib/Path/#class-path-__str__ "Copy/link to this API element")

```python
__str__(self: Self, Self: type)
```

Defined in [`pathlib`](https://docs.exaloop.io/libraries/api/modules/pathlib/)

\_\_repr\_\_[](https://docs.exaloop.io/libraries/api/classes/pathlib/Path/#class-path-__repr__ "Copy/link to this API element")

```python
__repr__(self: Self, Self: type)
```

Defined in [`pathlib`](https://docs.exaloop.io/libraries/api/modules/pathlib/)

\_\_fspath\_\_[](https://docs.exaloop.io/libraries/api/classes/pathlib/Path/#class-path-__fspath__ "Copy/link to this API element")

```python
__fspath__(self: Self, Self: type)
```

Defined in [`pathlib`](https://docs.exaloop.io/libraries/api/modules/pathlib/)

\_\_hash\_\_[](https://docs.exaloop.io/libraries/api/classes/pathlib/Path/#class-path-__hash__ "Copy/link to this API element")

```python
__hash__(self: Self, Self: type)
```

Defined in [`pathlib`](https://docs.exaloop.io/libraries/api/modules/pathlib/)

\_\_eq\_\_[](https://docs.exaloop.io/libraries/api/classes/pathlib/Path/#class-path-__eq__ "Copy/link to this API element")

```python
__eq__(self: Self, other, Self: type)
```

Defined in [`pathlib`](https://docs.exaloop.io/libraries/api/modules/pathlib/)

\_\_ne\_\_[](https://docs.exaloop.io/libraries/api/classes/pathlib/Path/#class-path-__ne__ "Copy/link to this API element")

```python
__ne__(self: Self, other, Self: type)
```

Defined in [`pathlib`](https://docs.exaloop.io/libraries/api/modules/pathlib/)

\_\_lt\_\_[](https://docs.exaloop.io/libraries/api/classes/pathlib/Path/#class-path-__lt__ "Copy/link to this API element")

```python
__lt__(self: Self, other: Path, Self: type)
```

Defined in [`pathlib`](https://docs.exaloop.io/libraries/api/modules/pathlib/)

\_\_le\_\_[](https://docs.exaloop.io/libraries/api/classes/pathlib/Path/#class-path-__le__ "Copy/link to this API element")

```python
__le__(self: Self, other: Path, Self: type)
```

Defined in [`pathlib`](https://docs.exaloop.io/libraries/api/modules/pathlib/)

\_\_gt\_\_[](https://docs.exaloop.io/libraries/api/classes/pathlib/Path/#class-path-__gt__ "Copy/link to this API element")

```python
__gt__(self: Self, other: Path, Self: type)
```

Defined in [`pathlib`](https://docs.exaloop.io/libraries/api/modules/pathlib/)

\_\_ge\_\_[](https://docs.exaloop.io/libraries/api/classes/pathlib/Path/#class-path-__ge__ "Copy/link to this API element")

```python
__ge__(self: Self, other: Path, Self: type)
```

Defined in [`pathlib`](https://docs.exaloop.io/libraries/api/modules/pathlib/)

\_\_truediv\_\_[](https://docs.exaloop.io/libraries/api/classes/pathlib/Path/#class-path-__truediv__ "Copy/link to this API element")

```python
__truediv__(self: Self, other, Self: type)
```

Defined in [`pathlib`](https://docs.exaloop.io/libraries/api/modules/pathlib/)

\_\_rtruediv\_\_[](https://docs.exaloop.io/libraries/api/classes/pathlib/Path/#class-path-__rtruediv__ "Copy/link to this API element")

```python
__rtruediv__(self: Self, other, Self: type)
```

Defined in [`pathlib`](https://docs.exaloop.io/libraries/api/modules/pathlib/)

Source: https://docs.exaloop.io/libraries/api/classes/pathlib/Path/
