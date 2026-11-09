# operator

Standard library module [View source](https://github.com/exaloop/codon/blob/master/stdlib/operator.codon)

variable lt[](https://docs.exaloop.io/libraries/api/modules/operator/#variable-lt "Copy/link to this API element")

```python
lt = lambda a, b: (a < b)
```

variable le[](https://docs.exaloop.io/libraries/api/modules/operator/#variable-le "Copy/link to this API element")

```python
le = lambda a, b: (a <= b)
```

variable eq[](https://docs.exaloop.io/libraries/api/modules/operator/#variable-eq "Copy/link to this API element")

```python
eq = lambda a, b: (a == b)
```

variable ne[](https://docs.exaloop.io/libraries/api/modules/operator/#variable-ne "Copy/link to this API element")

```python
ne = lambda a, b: (a != b)
```

variable gt[](https://docs.exaloop.io/libraries/api/modules/operator/#variable-gt "Copy/link to this API element")

```python
gt = lambda a, b: (a > b)
```

variable ge[](https://docs.exaloop.io/libraries/api/modules/operator/#variable-ge "Copy/link to this API element")

```python
ge = lambda a, b: (a >= b)
```

function not\_[](https://docs.exaloop.io/libraries/api/modules/operator/#function-not_ "Copy/link to this API element")

```python
not_(a)
```

function truth[](https://docs.exaloop.io/libraries/api/modules/operator/#function-truth "Copy/link to this API element")

```python
truth(a)
```

function is\_[](https://docs.exaloop.io/libraries/api/modules/operator/#function-is_ "Copy/link to this API element")

```python
is_(a, b)
```

function is\_not[](https://docs.exaloop.io/libraries/api/modules/operator/#function-is_not "Copy/link to this API element")

```python
is_not(a, b)
```

function abs[](https://docs.exaloop.io/libraries/api/modules/operator/#function-abs "Copy/link to this API element")

```python
abs(a)
```

function add[](https://docs.exaloop.io/libraries/api/modules/operator/#function-add "Copy/link to this API element")

```python
add(a, b)
```

function and\_[](https://docs.exaloop.io/libraries/api/modules/operator/#function-and_ "Copy/link to this API element")

```python
and_(a, b)
```

function floordiv[](https://docs.exaloop.io/libraries/api/modules/operator/#function-floordiv "Copy/link to this API element")

```python
floordiv(a, b)
```

function index[](https://docs.exaloop.io/libraries/api/modules/operator/#function-index "Copy/link to this API element")

```python
index(a)
```

function inv[](https://docs.exaloop.io/libraries/api/modules/operator/#function-inv "Copy/link to this API element")

```python
inv(a)
```

variable invert[](https://docs.exaloop.io/libraries/api/modules/operator/#variable-invert "Copy/link to this API element")

```python
invert = inv
```

function lshift[](https://docs.exaloop.io/libraries/api/modules/operator/#function-lshift "Copy/link to this API element")

```python
lshift(a, b)
```

function mod[](https://docs.exaloop.io/libraries/api/modules/operator/#function-mod "Copy/link to this API element")

```python
mod(a, b)
```

function mul[](https://docs.exaloop.io/libraries/api/modules/operator/#function-mul "Copy/link to this API element")

```python
mul(a, b)
```

function matmul[](https://docs.exaloop.io/libraries/api/modules/operator/#function-matmul "Copy/link to this API element")

```python
matmul(a, b)
```

function neg[](https://docs.exaloop.io/libraries/api/modules/operator/#function-neg "Copy/link to this API element")

```python
neg(a)
```

function or\_[](https://docs.exaloop.io/libraries/api/modules/operator/#function-or_ "Copy/link to this API element")

```python
or_(a, b)
```

function pos[](https://docs.exaloop.io/libraries/api/modules/operator/#function-pos "Copy/link to this API element")

```python
pos(a)
```

function pow[](https://docs.exaloop.io/libraries/api/modules/operator/#function-pow "Copy/link to this API element")

```python
pow(a, b)
```

function rshift[](https://docs.exaloop.io/libraries/api/modules/operator/#function-rshift "Copy/link to this API element")

```python
rshift(a, b)
```

function sub[](https://docs.exaloop.io/libraries/api/modules/operator/#function-sub "Copy/link to this API element")

```python
sub(a, b)
```

function truediv[](https://docs.exaloop.io/libraries/api/modules/operator/#function-truediv "Copy/link to this API element")

```python
truediv(a, b)
```

function xor[](https://docs.exaloop.io/libraries/api/modules/operator/#function-xor "Copy/link to this API element")

```python
xor(a, b)
```

function concat[](https://docs.exaloop.io/libraries/api/modules/operator/#function-concat "Copy/link to this API element")

```python
concat(a, b)
```

function contains[](https://docs.exaloop.io/libraries/api/modules/operator/#function-contains "Copy/link to this API element")

```python
contains(a, b)
```

function countOf[](https://docs.exaloop.io/libraries/api/modules/operator/#function-countof "Copy/link to this API element")

```python
countOf(a, b)
```

function delitem[](https://docs.exaloop.io/libraries/api/modules/operator/#function-delitem "Copy/link to this API element")

```python
delitem(a, b)
```

function getitem[](https://docs.exaloop.io/libraries/api/modules/operator/#function-getitem "Copy/link to this API element")

```python
getitem(a, b)
```

function indexOf[](https://docs.exaloop.io/libraries/api/modules/operator/#function-indexof "Copy/link to this API element")

```python
indexOf(a, b)
```

function setitem[](https://docs.exaloop.io/libraries/api/modules/operator/#function-setitem "Copy/link to this API element")

```python
setitem(a, b, c)
```

function length\_hint[](https://docs.exaloop.io/libraries/api/modules/operator/#function-length_hint "Copy/link to this API element")

```python
length_hint(a, default = 0)
```

function attrgetter[](https://docs.exaloop.io/libraries/api/modules/operator/#function-attrgetter "Copy/link to this API element")

```python
attrgetter(attr: Literal[str])
```

function itemgetter[](https://docs.exaloop.io/libraries/api/modules/operator/#function-itemgetter "Copy/link to this API element")

```python
itemgetter(*items)
```

@overload

```python
itemgetter(item: Literal[int])
```

function methodcaller[](https://docs.exaloop.io/libraries/api/modules/operator/#function-methodcaller "Copy/link to this API element")

```python
methodcaller(name: Literal[str], *args, **kwargs)
```

function iadd[](https://docs.exaloop.io/libraries/api/modules/operator/#function-iadd "Copy/link to this API element")

```python
iadd(a, b)
```

function iand[](https://docs.exaloop.io/libraries/api/modules/operator/#function-iand "Copy/link to this API element")

```python
iand(a, b)
```

function iconcat[](https://docs.exaloop.io/libraries/api/modules/operator/#function-iconcat "Copy/link to this API element")

```python
iconcat(a, b)
```

function ifloordiv[](https://docs.exaloop.io/libraries/api/modules/operator/#function-ifloordiv "Copy/link to this API element")

```python
ifloordiv(a, b)
```

function ilshift[](https://docs.exaloop.io/libraries/api/modules/operator/#function-ilshift "Copy/link to this API element")

```python
ilshift(a, b)
```

function imod[](https://docs.exaloop.io/libraries/api/modules/operator/#function-imod "Copy/link to this API element")

```python
imod(a, b)
```

function imul[](https://docs.exaloop.io/libraries/api/modules/operator/#function-imul "Copy/link to this API element")

```python
imul(a, b)
```

function imatmul[](https://docs.exaloop.io/libraries/api/modules/operator/#function-imatmul "Copy/link to this API element")

```python
imatmul(a, b)
```

function ior[](https://docs.exaloop.io/libraries/api/modules/operator/#function-ior "Copy/link to this API element")

```python
ior(a, b)
```

function ipow[](https://docs.exaloop.io/libraries/api/modules/operator/#function-ipow "Copy/link to this API element")

```python
ipow(a, b)
```

function irshift[](https://docs.exaloop.io/libraries/api/modules/operator/#function-irshift "Copy/link to this API element")

```python
irshift(a, b)
```

function isub[](https://docs.exaloop.io/libraries/api/modules/operator/#function-isub "Copy/link to this API element")

```python
isub(a, b)
```

function itruediv[](https://docs.exaloop.io/libraries/api/modules/operator/#function-itruediv "Copy/link to this API element")

```python
itruediv(a, b)
```

function ixor[](https://docs.exaloop.io/libraries/api/modules/operator/#function-ixor "Copy/link to this API element")

```python
ixor(a, b)
```

Source: https://docs.exaloop.io/libraries/api/modules/operator/
