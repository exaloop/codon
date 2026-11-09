# pathlib

Standard library module [View source](https://github.com/exaloop/codon/blob/master/stdlib/pathlib.codon)

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
__new__(*segments)
```

Properties 10

drive[](https://docs.exaloop.io/libraries/api/modules/pathlib/#class-path-drive "Copy/link to this API element")

@property

```python
drive(self: Self, Self: type)
```

root[](https://docs.exaloop.io/libraries/api/modules/pathlib/#class-path-root "Copy/link to this API element")

@property

```python
root(self: Self, Self: type)
```

anchor[](https://docs.exaloop.io/libraries/api/modules/pathlib/#class-path-anchor "Copy/link to this API element")

@property

```python
anchor(self: Self, Self: type)
```

parts[](https://docs.exaloop.io/libraries/api/modules/pathlib/#class-path-parts "Copy/link to this API element")

@property

```python
parts(self: Self, Self: type)
```

name[](https://docs.exaloop.io/libraries/api/modules/pathlib/#class-path-name "Copy/link to this API element")

@property

```python
name(self: Self, Self: type)
```

suffix[](https://docs.exaloop.io/libraries/api/modules/pathlib/#class-path-suffix "Copy/link to this API element")

@property

```python
suffix(self: Self, Self: type)
```

suffixes[](https://docs.exaloop.io/libraries/api/modules/pathlib/#class-path-suffixes "Copy/link to this API element")

@property

```python
suffixes(self: Self, Self: type)
```

stem[](https://docs.exaloop.io/libraries/api/modules/pathlib/#class-path-stem "Copy/link to this API element")

@property

```python
stem(self: Self, Self: type)
```

parent[](https://docs.exaloop.io/libraries/api/modules/pathlib/#class-path-parent "Copy/link to this API element")

@property

```python
parent(self: Self, Self: type)
```

parents[](https://docs.exaloop.io/libraries/api/modules/pathlib/#class-path-parents "Copy/link to this API element")

@property

```python
parents(self: Self, Self: type)
```

Methods 48

joinpath[](https://docs.exaloop.io/libraries/api/modules/pathlib/#class-path-joinpath "Copy/link to this API element")

```python
joinpath(self: Self, *segments, Self: type)
```

as\_posix[](https://docs.exaloop.io/libraries/api/modules/pathlib/#class-path-as_posix "Copy/link to this API element")

```python
as_posix(self: Self, Self: type)
```

with\_name[](https://docs.exaloop.io/libraries/api/modules/pathlib/#class-path-with_name "Copy/link to this API element")

```python
with_name(self: Self, name: str, Self: type)
```

with\_stem[](https://docs.exaloop.io/libraries/api/modules/pathlib/#class-path-with_stem "Copy/link to this API element")

```python
with_stem(self: Self, stem: str, Self: type)
```

with\_suffix[](https://docs.exaloop.io/libraries/api/modules/pathlib/#class-path-with_suffix "Copy/link to this API element")

```python
with_suffix(self: Self, suffix: str, Self: type)
```

is\_absolute[](https://docs.exaloop.io/libraries/api/modules/pathlib/#class-path-is_absolute "Copy/link to this API element")

```python
is_absolute(self: Self, Self: type)
```

is\_relative\_to[](https://docs.exaloop.io/libraries/api/modules/pathlib/#class-path-is_relative_to "Copy/link to this API element")

```python
is_relative_to(self: Self, other, Self: type)
```

relative\_to[](https://docs.exaloop.io/libraries/api/modules/pathlib/#class-path-relative_to "Copy/link to this API element")

```python
relative_to(self: Self, other, walk_up: bool = False, Self: type)
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
absolute(self: Self, Self: type)
```

resolve[](https://docs.exaloop.io/libraries/api/modules/pathlib/#class-path-resolve "Copy/link to this API element")

```python
resolve(self: Self, strict: bool = False, Self: type)
```

expanduser[](https://docs.exaloop.io/libraries/api/modules/pathlib/#class-path-expanduser "Copy/link to this API element")

```python
expanduser(self: Self, Self: type)
```

as\_uri[](https://docs.exaloop.io/libraries/api/modules/pathlib/#class-path-as_uri "Copy/link to this API element")

```python
as_uri(self: Self, Self: type)
```

stat[](https://docs.exaloop.io/libraries/api/modules/pathlib/#class-path-stat "Copy/link to this API element")

```python
stat(self: Self, follow_symlinks: bool = True, Self: type)
```

lstat[](https://docs.exaloop.io/libraries/api/modules/pathlib/#class-path-lstat "Copy/link to this API element")

```python
lstat(self: Self, Self: type)
```

exists[](https://docs.exaloop.io/libraries/api/modules/pathlib/#class-path-exists "Copy/link to this API element")

```python
exists(self: Self, follow_symlinks: bool = True, Self: type)
```

is\_file[](https://docs.exaloop.io/libraries/api/modules/pathlib/#class-path-is_file "Copy/link to this API element")

```python
is_file(self: Self, follow_symlinks: bool = True, Self: type)
```

is\_dir[](https://docs.exaloop.io/libraries/api/modules/pathlib/#class-path-is_dir "Copy/link to this API element")

```python
is_dir(self: Self, follow_symlinks: bool = True, Self: type)
```

is\_symlink[](https://docs.exaloop.io/libraries/api/modules/pathlib/#class-path-is_symlink "Copy/link to this API element")

```python
is_symlink(self: Self, Self: type)
```

is\_socket[](https://docs.exaloop.io/libraries/api/modules/pathlib/#class-path-is_socket "Copy/link to this API element")

```python
is_socket(self: Self, Self: type)
```

is\_fifo[](https://docs.exaloop.io/libraries/api/modules/pathlib/#class-path-is_fifo "Copy/link to this API element")

```python
is_fifo(self: Self, Self: type)
```

is\_block\_device[](https://docs.exaloop.io/libraries/api/modules/pathlib/#class-path-is_block_device "Copy/link to this API element")

```python
is_block_device(self: Self, Self: type)
```

is\_char\_device[](https://docs.exaloop.io/libraries/api/modules/pathlib/#class-path-is_char_device "Copy/link to this API element")

```python
is_char_device(self: Self, Self: type)
```

is\_mount[](https://docs.exaloop.io/libraries/api/modules/pathlib/#class-path-is_mount "Copy/link to this API element")

```python
is_mount(self: Self, Self: type)
```

samefile[](https://docs.exaloop.io/libraries/api/modules/pathlib/#class-path-samefile "Copy/link to this API element")

```python
samefile(self: Self, other, Self: type)
```

open[](https://docs.exaloop.io/libraries/api/modules/pathlib/#class-path-open "Copy/link to this API element")

```python
open(self: Self, mode: str = "r", encoding: Optional[str] = None, errors: Optional[str] = None, newline: Optional[str] = None, Self: type)
```

```python
open(self: Self, mode: Literal[str], encoding: Optional[str] = None, errors: Optional[str] = None, newline: Optional[str] = None, Self: type)
```

read\_text[](https://docs.exaloop.io/libraries/api/modules/pathlib/#class-path-read_text "Copy/link to this API element")

```python
read_text(self: Self, encoding: Optional[str] = None, errors: Optional[str] = None, newline: Optional[str] = None, Self: type)
```

read\_bytes[](https://docs.exaloop.io/libraries/api/modules/pathlib/#class-path-read_bytes "Copy/link to this API element")

```python
read_bytes(self: Self, Self: type)
```

write\_text[](https://docs.exaloop.io/libraries/api/modules/pathlib/#class-path-write_text "Copy/link to this API element")

```python
write_text(self: Self, data: str, encoding: Optional[str] = None, errors: Optional[str] = None, newline: Optional[str] = None, Self: type)
```

write\_bytes[](https://docs.exaloop.io/libraries/api/modules/pathlib/#class-path-write_bytes "Copy/link to this API element")

```python
write_bytes(self: Self, data: bytes, Self: type)
```

iterdir[](https://docs.exaloop.io/libraries/api/modules/pathlib/#class-path-iterdir "Copy/link to this API element")

```python
iterdir(self: Self, Self: type)
```

mkdir[](https://docs.exaloop.io/libraries/api/modules/pathlib/#class-path-mkdir "Copy/link to this API element")

```python
mkdir(self: Self, mode: int = 0o777, parents: bool = False, exist_ok: bool = False, Self: type)
```

rmdir[](https://docs.exaloop.io/libraries/api/modules/pathlib/#class-path-rmdir "Copy/link to this API element")

```python
rmdir(self: Self, Self: type)
```

unlink[](https://docs.exaloop.io/libraries/api/modules/pathlib/#class-path-unlink "Copy/link to this API element")

```python
unlink(self: Self, missing_ok: bool = False, Self: type)
```

rename[](https://docs.exaloop.io/libraries/api/modules/pathlib/#class-path-rename "Copy/link to this API element")

```python
rename(self: Self, target, Self: type)
```

replace[](https://docs.exaloop.io/libraries/api/modules/pathlib/#class-path-replace "Copy/link to this API element")

```python
replace(self: Self, target, Self: type)
```

chmod[](https://docs.exaloop.io/libraries/api/modules/pathlib/#class-path-chmod "Copy/link to this API element")

```python
chmod(self: Self, mode: int, follow_symlinks: bool = True, Self: type)
```

lchmod[](https://docs.exaloop.io/libraries/api/modules/pathlib/#class-path-lchmod "Copy/link to this API element")

```python
lchmod(self: Self, mode: int, Self: type)
```

symlink\_to[](https://docs.exaloop.io/libraries/api/modules/pathlib/#class-path-symlink_to "Copy/link to this API element")

```python
symlink_to(self: Self, target, target_is_directory: bool = False, Self: type)
```

hardlink\_to[](https://docs.exaloop.io/libraries/api/modules/pathlib/#class-path-hardlink_to "Copy/link to this API element")

```python
hardlink_to(self: Self, target, Self: type)
```

readlink[](https://docs.exaloop.io/libraries/api/modules/pathlib/#class-path-readlink "Copy/link to this API element")

```python
readlink(self: Self, Self: type)
```

touch[](https://docs.exaloop.io/libraries/api/modules/pathlib/#class-path-touch "Copy/link to this API element")

```python
touch(self: Self, mode: int = 0o666, exist_ok: bool = True, Self: type)
```

match[](https://docs.exaloop.io/libraries/api/modules/pathlib/#class-path-match "Copy/link to this API element")

```python
match(self: Self, pattern, case_sensitive: Optional[bool] = None, Self: type)
```

full\_match[](https://docs.exaloop.io/libraries/api/modules/pathlib/#class-path-full_match "Copy/link to this API element")

```python
full_match(self: Self, pattern, case_sensitive: Optional[bool] = None, Self: type)
```

glob[](https://docs.exaloop.io/libraries/api/modules/pathlib/#class-path-glob "Copy/link to this API element")

```python
glob(self: Self, pattern, case_sensitive: Optional[bool] = None, recurse_symlinks: bool = False, Self: type)
```

rglob[](https://docs.exaloop.io/libraries/api/modules/pathlib/#class-path-rglob "Copy/link to this API element")

```python
rglob(self: Self, pattern, case_sensitive: Optional[bool] = None, recurse_symlinks: bool = False, Self: type)
```

walk[](https://docs.exaloop.io/libraries/api/modules/pathlib/#class-path-walk "Copy/link to this API element")

```python
walk(self: Self, top_down: bool = True, on_error = None, follow_symlinks: bool = False, Self: type)
```

Special methods 12

\_\_str\_\_[](https://docs.exaloop.io/libraries/api/modules/pathlib/#class-path-__str__ "Copy/link to this API element")

```python
__str__(self: Self, Self: type)
```

\_\_repr\_\_[](https://docs.exaloop.io/libraries/api/modules/pathlib/#class-path-__repr__ "Copy/link to this API element")

```python
__repr__(self: Self, Self: type)
```

\_\_fspath\_\_[](https://docs.exaloop.io/libraries/api/modules/pathlib/#class-path-__fspath__ "Copy/link to this API element")

```python
__fspath__(self: Self, Self: type)
```

\_\_hash\_\_[](https://docs.exaloop.io/libraries/api/modules/pathlib/#class-path-__hash__ "Copy/link to this API element")

```python
__hash__(self: Self, Self: type)
```

\_\_eq\_\_[](https://docs.exaloop.io/libraries/api/modules/pathlib/#class-path-__eq__ "Copy/link to this API element")

```python
__eq__(self: Self, other, Self: type)
```

\_\_ne\_\_[](https://docs.exaloop.io/libraries/api/modules/pathlib/#class-path-__ne__ "Copy/link to this API element")

```python
__ne__(self: Self, other, Self: type)
```

\_\_lt\_\_[](https://docs.exaloop.io/libraries/api/modules/pathlib/#class-path-__lt__ "Copy/link to this API element")

```python
__lt__(self: Self, other: Path, Self: type)
```

\_\_le\_\_[](https://docs.exaloop.io/libraries/api/modules/pathlib/#class-path-__le__ "Copy/link to this API element")

```python
__le__(self: Self, other: Path, Self: type)
```

\_\_gt\_\_[](https://docs.exaloop.io/libraries/api/modules/pathlib/#class-path-__gt__ "Copy/link to this API element")

```python
__gt__(self: Self, other: Path, Self: type)
```

\_\_ge\_\_[](https://docs.exaloop.io/libraries/api/modules/pathlib/#class-path-__ge__ "Copy/link to this API element")

```python
__ge__(self: Self, other: Path, Self: type)
```

\_\_truediv\_\_[](https://docs.exaloop.io/libraries/api/modules/pathlib/#class-path-__truediv__ "Copy/link to this API element")

```python
__truediv__(self: Self, other, Self: type)
```

\_\_rtruediv\_\_[](https://docs.exaloop.io/libraries/api/modules/pathlib/#class-path-__rtruediv__ "Copy/link to this API element")

```python
__rtruediv__(self: Self, other, Self: type)
```

Source: https://docs.exaloop.io/libraries/api/modules/pathlib/
