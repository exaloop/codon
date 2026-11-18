# re

Standard library Module [View source](https://github.com/exaloop/codon/blob/master/stdlib/re.codon)

variable A[](https://docs.exaloop.io/libraries/api/modules/re/#variable-a "Copy/link to this API element")

```python
A: Literal[int] = (1 << 0)
```

variable ASCII[](https://docs.exaloop.io/libraries/api/modules/re/#variable-ascii "Copy/link to this API element")

```python
ASCII: Literal[int] = (1 << 0)
```

variable DEBUG[](https://docs.exaloop.io/libraries/api/modules/re/#variable-debug "Copy/link to this API element")

```python
DEBUG: Literal[int] = (1 << 1)
```

variable I[](https://docs.exaloop.io/libraries/api/modules/re/#variable-i "Copy/link to this API element")

```python
I: Literal[int] = (1 << 2)
```

variable IGNORECASE[](https://docs.exaloop.io/libraries/api/modules/re/#variable-ignorecase "Copy/link to this API element")

```python
IGNORECASE: Literal[int] = (1 << 2)
```

variable L[](https://docs.exaloop.io/libraries/api/modules/re/#variable-l "Copy/link to this API element")

```python
L: Literal[int] = (1 << 3)
```

variable LOCALE[](https://docs.exaloop.io/libraries/api/modules/re/#variable-locale "Copy/link to this API element")

```python
LOCALE: Literal[int] = (1 << 3)
```

variable M[](https://docs.exaloop.io/libraries/api/modules/re/#variable-m "Copy/link to this API element")

```python
M: Literal[int] = (1 << 4)
```

variable MULTILINE[](https://docs.exaloop.io/libraries/api/modules/re/#variable-multiline "Copy/link to this API element")

```python
MULTILINE: Literal[int] = (1 << 4)
```

variable S[](https://docs.exaloop.io/libraries/api/modules/re/#variable-s "Copy/link to this API element")

```python
S: Literal[int] = (1 << 5)
```

variable DOTALL[](https://docs.exaloop.io/libraries/api/modules/re/#variable-dotall "Copy/link to this API element")

```python
DOTALL: Literal[int] = (1 << 5)
```

variable X[](https://docs.exaloop.io/libraries/api/modules/re/#variable-x "Copy/link to this API element")

```python
X: Literal[int] = (1 << 6)
```

variable VERBOSE[](https://docs.exaloop.io/libraries/api/modules/re/#variable-verbose "Copy/link to this API element")

```python
VERBOSE: Literal[int] = (1 << 6)
```

class Span[](https://docs.exaloop.io/libraries/api/modules/re/#class-span "Copy/link to this API element")

@tuple

```python
class Span
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/re/Span/)

Fields 2

`start` `int`

`end` `int`

Special methods 1

\_\_bool\_\_[](https://docs.exaloop.io/libraries/api/modules/re/#class-span-__bool__ "Copy/link to this API element")

```python
__bool__(self: Self)
```

function seq\_re\_match[](https://docs.exaloop.io/libraries/api/modules/re/#function-seq_re_match "Copy/link to this API element")

@pure @C

```python
seq_re_match(re: cobj, anchor: int, string: bytes, pos: int, endpos: int) -> Ptr[Span]
```

function seq\_re\_match\_one[](https://docs.exaloop.io/libraries/api/modules/re/#function-seq_re_match_one "Copy/link to this API element")

@pure @C

```python
seq_re_match_one(re: cobj, anchor: int, string: bytes, pos: int, endpos: int) -> Span
```

function seq\_re\_pattern\_groups[](https://docs.exaloop.io/libraries/api/modules/re/#function-seq_re_pattern_groups "Copy/link to this API element")

@pure @C

```python
seq_re_pattern_groups(re: cobj) -> int
```

function seq\_re\_group\_name\_to\_index[](https://docs.exaloop.io/libraries/api/modules/re/#function-seq_re_group_name_to_index "Copy/link to this API element")

@pure @C

```python
seq_re_group_name_to_index(re: cobj, name: bytes) -> int
```

function seq\_re\_group\_index\_to\_name[](https://docs.exaloop.io/libraries/api/modules/re/#function-seq_re_group_index_to_name "Copy/link to this API element")

@pure @C

```python
seq_re_group_index_to_name(re: cobj, index: int) -> bytes
```

function seq\_re\_pattern\_error[](https://docs.exaloop.io/libraries/api/modules/re/#function-seq_re_pattern_error "Copy/link to this API element")

@pure @C

```python
seq_re_pattern_error(re: cobj) -> bytes
```

function seq\_re\_escape[](https://docs.exaloop.io/libraries/api/modules/re/#function-seq_re_escape "Copy/link to this API element")

@pure @C

```python
seq_re_escape(pattern: bytes) -> bytes
```

function seq\_re\_purge[](https://docs.exaloop.io/libraries/api/modules/re/#function-seq_re_purge "Copy/link to this API element")

@C

```python
seq_re_purge() -> None
```

function seq\_re\_compile[](https://docs.exaloop.io/libraries/api/modules/re/#function-seq_re_compile "Copy/link to this API element")

@pure @C

```python
seq_re_compile(pattern: bytes, flags: int, is_bytes: bool) -> cobj
```

class error[](https://docs.exaloop.io/libraries/api/modules/re/#class-error "Copy/link to this API element")

```python
class error(Exception)
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/re/error/)

Fields 1

`pattern` `str`

Constructors 1

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/re/#class-error-__init__ "Copy/link to this API element")

```python
__init__(self: Self, message: str = "", pattern: str = "")
```

Properties 1

msg[](https://docs.exaloop.io/libraries/api/modules/re/#class-error-msg "Copy/link to this API element")

@property

```python
msg(self: Self)
```

class Pattern[](https://docs.exaloop.io/libraries/api/modules/re/#class-pattern "Copy/link to this API element")

@tuple

```python
class Pattern[T]
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/re/Pattern/)

Fields 3

`pattern` `T`

`flags` `int`

`T` `type`

function compile[](https://docs.exaloop.io/libraries/api/modules/re/#function-compile "Copy/link to this API element")

```python
compile(pattern: str, flags: int = 0) -> Pattern[str]
```

@overload

```python
compile(pattern: bytes, flags: int = 0) -> Pattern[bytes]
```

function search[](https://docs.exaloop.io/libraries/api/modules/re/#function-search "Copy/link to this API element")

```python
search(pattern: str, string: str, flags: int = 0)
```

@overload

```python
search(pattern: bytes, string: bytes, flags: int = 0)
```

function match[](https://docs.exaloop.io/libraries/api/modules/re/#function-match "Copy/link to this API element")

```python
match(pattern: str, string: str, flags: int = 0)
```

@overload

```python
match(pattern: bytes, string: bytes, flags: int = 0)
```

function fullmatch[](https://docs.exaloop.io/libraries/api/modules/re/#function-fullmatch "Copy/link to this API element")

```python
fullmatch(pattern: str, string: str, flags: int = 0)
```

@overload

```python
fullmatch(pattern: bytes, string: bytes, flags: int = 0)
```

function finditer[](https://docs.exaloop.io/libraries/api/modules/re/#function-finditer "Copy/link to this API element")

```python
finditer(pattern: str, string: str, flags: int = 0)
```

@overload

```python
finditer(pattern: bytes, string: bytes, flags: int = 0) -> Generator[Match[bytes]]
```

function findall[](https://docs.exaloop.io/libraries/api/modules/re/#function-findall "Copy/link to this API element")

```python
findall(pattern: str, string: str, flags: int = 0)
```

@overload

```python
findall(pattern: bytes, string: bytes, flags: int = 0)
```

function split[](https://docs.exaloop.io/libraries/api/modules/re/#function-split "Copy/link to this API element")

```python
split(pattern: str, string: str, maxsplit: int = 0, flags: int = 0)
```

@overload

```python
split(pattern: bytes, string: bytes, maxsplit: int = 0, flags: int = 0)
```

function sub[](https://docs.exaloop.io/libraries/api/modules/re/#function-sub "Copy/link to this API element")

```python
sub(pattern: str, repl, string: str, count: int = 0, flags: int = 0)
```

@overload

```python
sub(pattern: bytes, repl, string: bytes, count: int = 0, flags: int = 0)
```

function subn[](https://docs.exaloop.io/libraries/api/modules/re/#function-subn "Copy/link to this API element")

```python
subn(pattern: str, repl, string: str, count: int = 0, flags: int = 0)
```

@overload

```python
subn(pattern: bytes, repl, string: bytes, count: int = 0, flags: int = 0)
```

function escape[](https://docs.exaloop.io/libraries/api/modules/re/#function-escape "Copy/link to this API element")

```python
escape(pattern: str)
```

@overload

```python
escape(pattern: bytes)
```

function purge[](https://docs.exaloop.io/libraries/api/modules/re/#function-purge "Copy/link to this API element")

```python
purge()
```

class Match[](https://docs.exaloop.io/libraries/api/modules/re/#class-match "Copy/link to this API element")

@tuple

```python
class Match[T]
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/re/Match/)

Fields 5

`pos` `int`

`endpos` `int`

`re` `Pattern[T]`

`string` `T`

`T` `type`

Properties 2

lastindex[](https://docs.exaloop.io/libraries/api/modules/re/#class-match-lastindex "Copy/link to this API element")

@property

```python
lastindex(self: Self)
```

lastgroup[](https://docs.exaloop.io/libraries/api/modules/re/#class-match-lastgroup "Copy/link to this API element")

@property

```python
lastgroup(self: Self)
```

Methods 7

group[](https://docs.exaloop.io/libraries/api/modules/re/#class-match-group "Copy/link to this API element")

```python
group(self: Self, /, *args)
```

start[](https://docs.exaloop.io/libraries/api/modules/re/#class-match-start "Copy/link to this API element")

```python
start(self: Self, group = 0, /)
```

end[](https://docs.exaloop.io/libraries/api/modules/re/#class-match-end "Copy/link to this API element")

```python
end(self: Self, group = 0, /)
```

span[](https://docs.exaloop.io/libraries/api/modules/re/#class-match-span "Copy/link to this API element")

```python
span(self: Self, group = 0, /)
```

expand[](https://docs.exaloop.io/libraries/api/modules/re/#class-match-expand "Copy/link to this API element")

```python
expand(self: Self, /, template: str)
```

groups[](https://docs.exaloop.io/libraries/api/modules/re/#class-match-groups "Copy/link to this API element")

```python
groups(self: Self, /, default: Optional[str] = None)
```

groupdict[](https://docs.exaloop.io/libraries/api/modules/re/#class-match-groupdict "Copy/link to this API element")

```python
groupdict(self: Self, /, default: Optional[str] = None)
```

Special methods 4

\_\_getitem\_\_[](https://docs.exaloop.io/libraries/api/modules/re/#class-match-__getitem__ "Copy/link to this API element")

```python
__getitem__(self: Self, g, /)
```

\_\_copy\_\_[](https://docs.exaloop.io/libraries/api/modules/re/#class-match-__copy__ "Copy/link to this API element")

```python
__copy__(self: Self, /)
```

\_\_deepcopy\_\_[](https://docs.exaloop.io/libraries/api/modules/re/#class-match-__deepcopy__ "Copy/link to this API element")

```python
__deepcopy__(self: Self, /)
```

\_\_bool\_\_[](https://docs.exaloop.io/libraries/api/modules/re/#class-match-__bool__ "Copy/link to this API element")

```python
__bool__(self: Self)
```

class Pattern[](https://docs.exaloop.io/libraries/api/modules/re/#class-pattern "Copy/link to this API element")

@extend

```python
class Pattern[T]
```

Extends [`re.Pattern`](https://docs.exaloop.io/libraries/api/classes/re/Pattern/) · [View complete class API →](https://docs.exaloop.io/libraries/api/classes/re/Pattern/)

Properties 2

groups[](https://docs.exaloop.io/libraries/api/modules/re/#class-pattern-groups "Copy/link to this API element")

@property

```python
groups(self: Self)
```

groupindex[](https://docs.exaloop.io/libraries/api/modules/re/#class-pattern-groupindex "Copy/link to this API element")

@property

```python
groupindex(self: Self)
```

Methods 8

search[](https://docs.exaloop.io/libraries/api/modules/re/#class-pattern-search "Copy/link to this API element")

```python
search(self: Self, /, string: T, pos: Optional[int] = None, endpos: Optional[int] = None) -> Optional[Match[T]]
```

match[](https://docs.exaloop.io/libraries/api/modules/re/#class-pattern-match "Copy/link to this API element")

```python
match(self: Self, /, string: T, pos: Optional[int] = None, endpos: Optional[int] = None) -> Optional[Match[T]]
```

fullmatch[](https://docs.exaloop.io/libraries/api/modules/re/#class-pattern-fullmatch "Copy/link to this API element")

```python
fullmatch(self: Self, /, string: T, pos: Optional[int] = None, endpos: Optional[int] = None) -> Optional[Match[T]]
```

finditer[](https://docs.exaloop.io/libraries/api/modules/re/#class-pattern-finditer "Copy/link to this API element")

```python
finditer(self: Self, /, string: T, pos: Optional[int] = None, endpos: Optional[int] = None) -> Generator[Match[T]]
```

findall[](https://docs.exaloop.io/libraries/api/modules/re/#class-pattern-findall "Copy/link to this API element")

```python
findall(self: Self, /, string: T, pos: Optional[int] = None, endpos: Optional[int] = None)
```

split[](https://docs.exaloop.io/libraries/api/modules/re/#class-pattern-split "Copy/link to this API element")

```python
split(self: Self, /, string: T, maxsplit: int = 0)
```

subn[](https://docs.exaloop.io/libraries/api/modules/re/#class-pattern-subn "Copy/link to this API element")

```python
subn(self: Self, /, repl, string: T, count: int = 0)
```

sub[](https://docs.exaloop.io/libraries/api/modules/re/#class-pattern-sub "Copy/link to this API element")

```python
sub(self: Self, /, repl, string: T, count: int = 0)
```

Special methods 1

\_\_bool\_\_[](https://docs.exaloop.io/libraries/api/modules/re/#class-pattern-__bool__ "Copy/link to this API element")

```python
__bool__(self: Self)
```

Source: https://docs.exaloop.io/libraries/api/modules/re/
