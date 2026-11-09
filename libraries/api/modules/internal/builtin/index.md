# internal.builtin

Standard library module [View source](https://github.com/exaloop/codon/blob/master/stdlib/internal/builtin.codon)

function getattr[](https://docs.exaloop.io/libraries/api/modules/internal/builtin/#function-getattr "Copy/link to this API element")

```python
getattr(obj, attr: Literal[str], default, typ: type = NoneType)
```

@overload

```python
getattr(obj, attr: Literal[str], type: type = NoneType)
```

function id[](https://docs.exaloop.io/libraries/api/modules/internal/builtin/#function-id "Copy/link to this API element")

```python
id(x)
```

function print[](https://docs.exaloop.io/libraries/api/modules/internal/builtin/#function-print "Copy/link to this API element")

```python
print(*args, sep: str = " ", end: str = "\n", file = None, flush: bool = False)
```

function input[](https://docs.exaloop.io/libraries/api/modules/internal/builtin/#function-input "Copy/link to this API element")

```python
input(prompt: str = "")
```

function min[](https://docs.exaloop.io/libraries/api/modules/internal/builtin/#function-min "Copy/link to this API element")

```python
min(*args, key = None, default = None)
```

function max[](https://docs.exaloop.io/libraries/api/modules/internal/builtin/#function-max "Copy/link to this API element")

```python
max(*args, key = None, default = None)
```

function len[](https://docs.exaloop.io/libraries/api/modules/internal/builtin/#function-len "Copy/link to this API element")

```python
len(x)
```

function iter[](https://docs.exaloop.io/libraries/api/modules/internal/builtin/#function-iter "Copy/link to this API element")

```python
iter(x)
```

function abs[](https://docs.exaloop.io/libraries/api/modules/internal/builtin/#function-abs "Copy/link to this API element")

```python
abs(x)
```

function hash[](https://docs.exaloop.io/libraries/api/modules/internal/builtin/#function-hash "Copy/link to this API element")

```python
hash(x)
```

function ord[](https://docs.exaloop.io/libraries/api/modules/internal/builtin/#function-ord "Copy/link to this API element")

```python
ord(character: str)
```

function divmod[](https://docs.exaloop.io/libraries/api/modules/internal/builtin/#function-divmod "Copy/link to this API element")

```python
divmod(a, b)
```

function chr[](https://docs.exaloop.io/libraries/api/modules/internal/builtin/#function-chr "Copy/link to this API element")

```python
chr(codepoint: int)
```

function next[](https://docs.exaloop.io/libraries/api/modules/internal/builtin/#function-next "Copy/link to this API element")

```python
next(g: Generator[T], default: Optional[T] = None, T: type)
```

function any[](https://docs.exaloop.io/libraries/api/modules/internal/builtin/#function-any "Copy/link to this API element")

```python
any(x: Generator[T], T: type)
```

function all[](https://docs.exaloop.io/libraries/api/modules/internal/builtin/#function-all "Copy/link to this API element")

```python
all(x: Generator[T], T: type)
```

function zip[](https://docs.exaloop.io/libraries/api/modules/internal/builtin/#function-zip "Copy/link to this API element")

```python
zip(*args)
```

function filter[](https://docs.exaloop.io/libraries/api/modules/internal/builtin/#function-filter "Copy/link to this API element")

```python
filter(f: CallableTrait[T, bool], x: Generator[T], T: type)
```

function map[](https://docs.exaloop.io/libraries/api/modules/internal/builtin/#function-map "Copy/link to this API element")

```python
map(f, *args)
```

function enumerate[](https://docs.exaloop.io/libraries/api/modules/internal/builtin/#function-enumerate "Copy/link to this API element")

```python
enumerate(x, start: int = 0)
```

function echo[](https://docs.exaloop.io/libraries/api/modules/internal/builtin/#function-echo "Copy/link to this API element")

```python
echo(x)
```

function reversed[](https://docs.exaloop.io/libraries/api/modules/internal/builtin/#function-reversed "Copy/link to this API element")

```python
reversed(x)
```

function round[](https://docs.exaloop.io/libraries/api/modules/internal/builtin/#function-round "Copy/link to this API element")

```python
round(x, ndigits = None)
```

function sum[](https://docs.exaloop.io/libraries/api/modules/internal/builtin/#function-sum "Copy/link to this API element")

```python
sum(x, start = 0)
```

function repr[](https://docs.exaloop.io/libraries/api/modules/internal/builtin/#function-repr "Copy/link to this API element")

```python
repr(x)
```

function bin[](https://docs.exaloop.io/libraries/api/modules/internal/builtin/#function-bin "Copy/link to this API element")

```python
bin(n)
```

function oct[](https://docs.exaloop.io/libraries/api/modules/internal/builtin/#function-oct "Copy/link to this API element")

```python
oct(n)
```

function hex[](https://docs.exaloop.io/libraries/api/modules/internal/builtin/#function-hex "Copy/link to this API element")

```python
hex(n)
```

function format[](https://docs.exaloop.io/libraries/api/modules/internal/builtin/#function-format "Copy/link to this API element")

```python
format(value, format_spec: str = "")
```

function pow[](https://docs.exaloop.io/libraries/api/modules/internal/builtin/#function-pow "Copy/link to this API element")

```python
pow(base: float, exp: float)
```

@overload

```python
pow(base: int, exp: int, mod: Optional[int] = None)
```

class Int[](https://docs.exaloop.io/libraries/api/modules/internal/builtin/#class-int "Copy/link to this API element")

@extend

```python
class Int[N]
```

Extends [`Int`](https://docs.exaloop.io/libraries/api/classes/Int/) · [View complete class API →](https://docs.exaloop.io/libraries/api/classes/Int/)

class UInt[](https://docs.exaloop.io/libraries/api/modules/internal/builtin/#class-uint "Copy/link to this API element")

@extend

```python
class UInt[N]
```

Extends [`UInt`](https://docs.exaloop.io/libraries/api/classes/UInt/) · [View complete class API →](https://docs.exaloop.io/libraries/api/classes/UInt/)

class float[](https://docs.exaloop.io/libraries/api/modules/internal/builtin/#class-float "Copy/link to this API element")

@extend

```python
class float
```

Extends [`float`](https://docs.exaloop.io/libraries/api/classes/float/) · [View complete class API →](https://docs.exaloop.io/libraries/api/classes/float/)

class complex[](https://docs.exaloop.io/libraries/api/modules/internal/builtin/#class-complex "Copy/link to this API element")

@extend

```python
class complex
```

Extends [`internal.types.complex.complex`](https://docs.exaloop.io/libraries/api/classes/internal/types/complex/complex/) · [View complete class API →](https://docs.exaloop.io/libraries/api/classes/internal/types/complex/complex/)

class float32[](https://docs.exaloop.io/libraries/api/modules/internal/builtin/#class-float32 "Copy/link to this API element")

@extend

```python
class float32
```

Extends [`float32`](https://docs.exaloop.io/libraries/api/classes/float32/) · [View complete class API →](https://docs.exaloop.io/libraries/api/classes/float32/)

class float16[](https://docs.exaloop.io/libraries/api/modules/internal/builtin/#class-float16 "Copy/link to this API element")

@extend

```python
class float16
```

Extends [`float16`](https://docs.exaloop.io/libraries/api/classes/float16/) · [View complete class API →](https://docs.exaloop.io/libraries/api/classes/float16/)

class bfloat16[](https://docs.exaloop.io/libraries/api/modules/internal/builtin/#class-bfloat16 "Copy/link to this API element")

@extend

```python
class bfloat16
```

Extends [`bfloat16`](https://docs.exaloop.io/libraries/api/classes/bfloat16/) · [View complete class API →](https://docs.exaloop.io/libraries/api/classes/bfloat16/)

class complex64[](https://docs.exaloop.io/libraries/api/modules/internal/builtin/#class-complex64 "Copy/link to this API element")

@extend

```python
class complex64
```

Extends [`internal.types.complex.complex64`](https://docs.exaloop.io/libraries/api/classes/internal/types/complex/complex64/) · [View complete class API →](https://docs.exaloop.io/libraries/api/classes/internal/types/complex/complex64/)

function cast[](https://docs.exaloop.io/libraries/api/modules/internal/builtin/#function-cast "Copy/link to this API element")

@inline

```python
cast(U: type, obj)
```

Source: https://docs.exaloop.io/libraries/api/modules/internal/builtin/
