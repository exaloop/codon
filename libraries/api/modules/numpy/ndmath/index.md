# numpy.ndmath

Standard library module [View source](https://github.com/exaloop/codon/blob/master/stdlib/numpy/ndmath.codon)

function cumprod[](https://docs.exaloop.io/libraries/api/modules/numpy/ndmath/#function-cumprod "Copy/link to this API element")

```python
cumprod(a, axis = None, dtype: type = NoneType, out = None)
```

function cumsum[](https://docs.exaloop.io/libraries/api/modules/numpy/ndmath/#function-cumsum "Copy/link to this API element")

```python
cumsum(a, axis = None, dtype: type = NoneType, out = None)
```

function diff[](https://docs.exaloop.io/libraries/api/modules/numpy/ndmath/#function-diff "Copy/link to this API element")

```python
diff(a, axis: int = -1, n: int = 1, append = None, prepend = None)
```

function gradient[](https://docs.exaloop.io/libraries/api/modules/numpy/ndmath/#function-gradient "Copy/link to this API element")

```python
gradient(f, *varargs, axis = None, edge_order: int = 1)
```

function ediff1d[](https://docs.exaloop.io/libraries/api/modules/numpy/ndmath/#function-ediff1d "Copy/link to this API element")

```python
ediff1d(ary, to_begin = None, to_end = None)
```

function cross[](https://docs.exaloop.io/libraries/api/modules/numpy/ndmath/#function-cross "Copy/link to this API element")

```python
cross(a, b, axisa: int = -1, axisb: int = -1, axisc: int = -1, axis = None)
```

function trapz[](https://docs.exaloop.io/libraries/api/modules/numpy/ndmath/#function-trapz "Copy/link to this API element")

```python
trapz(y, x = None, dx = 1.0, axis: int = -1)
```

function convolve[](https://docs.exaloop.io/libraries/api/modules/numpy/ndmath/#function-convolve "Copy/link to this API element")

```python
convolve(v, a, mode: str = "full")
```

variable add[](https://docs.exaloop.io/libraries/api/modules/numpy/ndmath/#variable-add "Copy/link to this API element")

```python
add = BinaryUFunc(operator.add, "add", 0)
```

variable subtract[](https://docs.exaloop.io/libraries/api/modules/numpy/ndmath/#variable-subtract "Copy/link to this API element")

```python
subtract = BinaryUFunc(operator.sub, "subtract")
```

variable multiply[](https://docs.exaloop.io/libraries/api/modules/numpy/ndmath/#variable-multiply "Copy/link to this API element")

```python
multiply = BinaryUFunc(operator.mul, "multiply", 1)
```

variable divide[](https://docs.exaloop.io/libraries/api/modules/numpy/ndmath/#variable-divide "Copy/link to this API element")

```python
divide = BinaryUFunc(_true_divide, "divide")
```

variable true\_divide[](https://docs.exaloop.io/libraries/api/modules/numpy/ndmath/#variable-true_divide "Copy/link to this API element")

```python
true_divide = divide
```

variable logaddexp[](https://docs.exaloop.io/libraries/api/modules/numpy/ndmath/#variable-logaddexp "Copy/link to this API element")

```python
logaddexp = BinaryUFunc(_logaddexp, "logaddexp")
```

variable logaddexp2[](https://docs.exaloop.io/libraries/api/modules/numpy/ndmath/#variable-logaddexp2 "Copy/link to this API element")

```python
logaddexp2 = BinaryUFunc(_logaddexp2, "logaddexp2")
```

variable floor\_divide[](https://docs.exaloop.io/libraries/api/modules/numpy/ndmath/#variable-floor_divide "Copy/link to this API element")

```python
floor_divide = BinaryUFunc(_floor_divide, "floor_divide")
```

variable negative[](https://docs.exaloop.io/libraries/api/modules/numpy/ndmath/#variable-negative "Copy/link to this API element")

```python
negative = UnaryUFunc(operator.neg, "negative")
```

variable positive[](https://docs.exaloop.io/libraries/api/modules/numpy/ndmath/#variable-positive "Copy/link to this API element")

```python
positive = UnaryUFunc(operator.pos, "positive")
```

variable power[](https://docs.exaloop.io/libraries/api/modules/numpy/ndmath/#variable-power "Copy/link to this API element")

```python
power = BinaryUFunc(operator.pow, "power")
```

variable remainder[](https://docs.exaloop.io/libraries/api/modules/numpy/ndmath/#variable-remainder "Copy/link to this API element")

```python
remainder = BinaryUFunc(_remainder, "remainder")
```

variable mod[](https://docs.exaloop.io/libraries/api/modules/numpy/ndmath/#variable-mod "Copy/link to this API element")

```python
mod = remainder
```

variable fmod[](https://docs.exaloop.io/libraries/api/modules/numpy/ndmath/#variable-fmod "Copy/link to this API element")

```python
fmod = BinaryUFunc(_fmod, "fmod")
```

variable absolute[](https://docs.exaloop.io/libraries/api/modules/numpy/ndmath/#variable-absolute "Copy/link to this API element")

```python
absolute = UnaryUFunc(operator.abs, "absolute")
```

variable abs[](https://docs.exaloop.io/libraries/api/modules/numpy/ndmath/#variable-abs "Copy/link to this API element")

```python
abs = absolute
```

variable fabs[](https://docs.exaloop.io/libraries/api/modules/numpy/ndmath/#variable-fabs "Copy/link to this API element")

```python
fabs = UnaryUFunc(_fabs, "fabs")
```

variable rint[](https://docs.exaloop.io/libraries/api/modules/numpy/ndmath/#variable-rint "Copy/link to this API element")

```python
rint = UnaryUFunc(_rint, "rint")
```

variable sign[](https://docs.exaloop.io/libraries/api/modules/numpy/ndmath/#variable-sign "Copy/link to this API element")

```python
sign = UnaryUFunc(_sign, "sign")
```

variable heaviside[](https://docs.exaloop.io/libraries/api/modules/numpy/ndmath/#variable-heaviside "Copy/link to this API element")

```python
heaviside = BinaryUFunc(_heaviside, "heaviside")
```

variable conjugate[](https://docs.exaloop.io/libraries/api/modules/numpy/ndmath/#variable-conjugate "Copy/link to this API element")

```python
conjugate = UnaryUFunc(_conj, "conjugate")
```

variable conj[](https://docs.exaloop.io/libraries/api/modules/numpy/ndmath/#variable-conj "Copy/link to this API element")

```python
conj = conjugate
```

variable exp[](https://docs.exaloop.io/libraries/api/modules/numpy/ndmath/#variable-exp "Copy/link to this API element")

```python
exp = UnaryUFunc(_exp, "exp")
```

variable exp2[](https://docs.exaloop.io/libraries/api/modules/numpy/ndmath/#variable-exp2 "Copy/link to this API element")

```python
exp2 = UnaryUFunc(_exp2, "exp2")
```

variable log[](https://docs.exaloop.io/libraries/api/modules/numpy/ndmath/#variable-log "Copy/link to this API element")

```python
log = UnaryUFunc(_log, "log")
```

variable log2[](https://docs.exaloop.io/libraries/api/modules/numpy/ndmath/#variable-log2 "Copy/link to this API element")

```python
log2 = UnaryUFunc(_log2, "log2")
```

variable log10[](https://docs.exaloop.io/libraries/api/modules/numpy/ndmath/#variable-log10 "Copy/link to this API element")

```python
log10 = UnaryUFunc(_log10, "log10")
```

variable expm1[](https://docs.exaloop.io/libraries/api/modules/numpy/ndmath/#variable-expm1 "Copy/link to this API element")

```python
expm1 = UnaryUFunc(_expm1, "expm1")
```

variable log1p[](https://docs.exaloop.io/libraries/api/modules/numpy/ndmath/#variable-log1p "Copy/link to this API element")

```python
log1p = UnaryUFunc(_log1p, "log1p")
```

variable sqrt[](https://docs.exaloop.io/libraries/api/modules/numpy/ndmath/#variable-sqrt "Copy/link to this API element")

```python
sqrt = UnaryUFunc(_sqrt, "sqrt")
```

variable square[](https://docs.exaloop.io/libraries/api/modules/numpy/ndmath/#variable-square "Copy/link to this API element")

```python
square = UnaryUFunc(_square, "square")
```

variable cbrt[](https://docs.exaloop.io/libraries/api/modules/numpy/ndmath/#variable-cbrt "Copy/link to this API element")

```python
cbrt = UnaryUFunc(_cbrt, "cbrt")
```

variable reciprocal[](https://docs.exaloop.io/libraries/api/modules/numpy/ndmath/#variable-reciprocal "Copy/link to this API element")

```python
reciprocal = UnaryUFunc(_reciprocal, "reciprocal")
```

variable gcd[](https://docs.exaloop.io/libraries/api/modules/numpy/ndmath/#variable-gcd "Copy/link to this API element")

```python
gcd = BinaryUFunc(_gcd, "gcd", 0)
```

variable lcm[](https://docs.exaloop.io/libraries/api/modules/numpy/ndmath/#variable-lcm "Copy/link to this API element")

```python
lcm = BinaryUFunc(_lcm, "lcm")
```

variable sin[](https://docs.exaloop.io/libraries/api/modules/numpy/ndmath/#variable-sin "Copy/link to this API element")

```python
sin = UnaryUFunc(_sin, "sin")
```

variable cos[](https://docs.exaloop.io/libraries/api/modules/numpy/ndmath/#variable-cos "Copy/link to this API element")

```python
cos = UnaryUFunc(_cos, "cos")
```

variable tan[](https://docs.exaloop.io/libraries/api/modules/numpy/ndmath/#variable-tan "Copy/link to this API element")

```python
tan = UnaryUFunc(_tan, "tan")
```

variable arcsin[](https://docs.exaloop.io/libraries/api/modules/numpy/ndmath/#variable-arcsin "Copy/link to this API element")

```python
arcsin = UnaryUFunc(_arcsin, "arcsin")
```

variable arccos[](https://docs.exaloop.io/libraries/api/modules/numpy/ndmath/#variable-arccos "Copy/link to this API element")

```python
arccos = UnaryUFunc(_arccos, "arccos")
```

variable arctan[](https://docs.exaloop.io/libraries/api/modules/numpy/ndmath/#variable-arctan "Copy/link to this API element")

```python
arctan = UnaryUFunc(_arctan, "arctan")
```

variable arctan2[](https://docs.exaloop.io/libraries/api/modules/numpy/ndmath/#variable-arctan2 "Copy/link to this API element")

```python
arctan2 = BinaryUFunc(_arctan2, "arctan2")
```

variable hypot[](https://docs.exaloop.io/libraries/api/modules/numpy/ndmath/#variable-hypot "Copy/link to this API element")

```python
hypot = BinaryUFunc(_hypot, "hypot")
```

variable sinh[](https://docs.exaloop.io/libraries/api/modules/numpy/ndmath/#variable-sinh "Copy/link to this API element")

```python
sinh = UnaryUFunc(_sinh, "sinh")
```

variable cosh[](https://docs.exaloop.io/libraries/api/modules/numpy/ndmath/#variable-cosh "Copy/link to this API element")

```python
cosh = UnaryUFunc(_cosh, "cosh")
```

variable tanh[](https://docs.exaloop.io/libraries/api/modules/numpy/ndmath/#variable-tanh "Copy/link to this API element")

```python
tanh = UnaryUFunc(_tanh, "tanh")
```

variable arcsinh[](https://docs.exaloop.io/libraries/api/modules/numpy/ndmath/#variable-arcsinh "Copy/link to this API element")

```python
arcsinh = UnaryUFunc(_arcsinh, "arcsinh")
```

variable arccosh[](https://docs.exaloop.io/libraries/api/modules/numpy/ndmath/#variable-arccosh "Copy/link to this API element")

```python
arccosh = UnaryUFunc(_arccosh, "arccosh")
```

variable arctanh[](https://docs.exaloop.io/libraries/api/modules/numpy/ndmath/#variable-arctanh "Copy/link to this API element")

```python
arctanh = UnaryUFunc(_arctanh, "arctanh")
```

variable deg2rad[](https://docs.exaloop.io/libraries/api/modules/numpy/ndmath/#variable-deg2rad "Copy/link to this API element")

```python
deg2rad = UnaryUFunc(_deg2rad, "deg2rad")
```

variable radians[](https://docs.exaloop.io/libraries/api/modules/numpy/ndmath/#variable-radians "Copy/link to this API element")

```python
radians = UnaryUFunc(_deg2rad, "radians")
```

variable rad2deg[](https://docs.exaloop.io/libraries/api/modules/numpy/ndmath/#variable-rad2deg "Copy/link to this API element")

```python
rad2deg = UnaryUFunc(_rad2deg, "rad2deg")
```

variable degrees[](https://docs.exaloop.io/libraries/api/modules/numpy/ndmath/#variable-degrees "Copy/link to this API element")

```python
degrees = UnaryUFunc(_rad2deg, "degrees")
```

variable bitwise\_and[](https://docs.exaloop.io/libraries/api/modules/numpy/ndmath/#variable-bitwise_and "Copy/link to this API element")

```python
bitwise_and = BinaryUFunc(operator.and_, "bitwise_and")
```

variable bitwise\_or[](https://docs.exaloop.io/libraries/api/modules/numpy/ndmath/#variable-bitwise_or "Copy/link to this API element")

```python
bitwise_or = BinaryUFunc(operator.or_, "bitwise_or")
```

variable bitwise\_xor[](https://docs.exaloop.io/libraries/api/modules/numpy/ndmath/#variable-bitwise_xor "Copy/link to this API element")

```python
bitwise_xor = BinaryUFunc(operator.xor, "bitwise_xor")
```

variable invert[](https://docs.exaloop.io/libraries/api/modules/numpy/ndmath/#variable-invert "Copy/link to this API element")

```python
invert = UnaryUFunc(operator.invert, "invert")
```

variable left\_shift[](https://docs.exaloop.io/libraries/api/modules/numpy/ndmath/#variable-left_shift "Copy/link to this API element")

```python
left_shift = BinaryUFunc(operator.lshift, "left_shift")
```

variable right\_shift[](https://docs.exaloop.io/libraries/api/modules/numpy/ndmath/#variable-right_shift "Copy/link to this API element")

```python
right_shift = BinaryUFunc(operator.rshift, "right_shift")
```

variable greater[](https://docs.exaloop.io/libraries/api/modules/numpy/ndmath/#variable-greater "Copy/link to this API element")

```python
greater = BinaryUFunc(operator.gt, "greater")
```

variable greater\_equal[](https://docs.exaloop.io/libraries/api/modules/numpy/ndmath/#variable-greater_equal "Copy/link to this API element")

```python
greater_equal = BinaryUFunc(operator.ge, "greater_equal")
```

variable less[](https://docs.exaloop.io/libraries/api/modules/numpy/ndmath/#variable-less "Copy/link to this API element")

```python
less = BinaryUFunc(operator.lt, "less")
```

variable less\_equal[](https://docs.exaloop.io/libraries/api/modules/numpy/ndmath/#variable-less_equal "Copy/link to this API element")

```python
less_equal = BinaryUFunc(operator.le, "less_equal")
```

variable not\_equal[](https://docs.exaloop.io/libraries/api/modules/numpy/ndmath/#variable-not_equal "Copy/link to this API element")

```python
not_equal = BinaryUFunc(operator.ne, "not_equal")
```

variable equal[](https://docs.exaloop.io/libraries/api/modules/numpy/ndmath/#variable-equal "Copy/link to this API element")

```python
equal = BinaryUFunc(operator.eq, "equal")
```

variable logical\_and[](https://docs.exaloop.io/libraries/api/modules/numpy/ndmath/#variable-logical_and "Copy/link to this API element")

```python
logical_and = BinaryUFunc(_logical_and, "logical_and")
```

variable logical\_or[](https://docs.exaloop.io/libraries/api/modules/numpy/ndmath/#variable-logical_or "Copy/link to this API element")

```python
logical_or = BinaryUFunc(_logical_or, "logical_or")
```

variable logical\_xor[](https://docs.exaloop.io/libraries/api/modules/numpy/ndmath/#variable-logical_xor "Copy/link to this API element")

```python
logical_xor = BinaryUFunc(_logical_xor, "logical_xor")
```

variable logical\_not[](https://docs.exaloop.io/libraries/api/modules/numpy/ndmath/#variable-logical_not "Copy/link to this API element")

```python
logical_not = UnaryUFunc(_logical_not, "logical_not")
```

variable maximum[](https://docs.exaloop.io/libraries/api/modules/numpy/ndmath/#variable-maximum "Copy/link to this API element")

```python
maximum = BinaryUFunc(_maximum, "maximum")
```

variable minimum[](https://docs.exaloop.io/libraries/api/modules/numpy/ndmath/#variable-minimum "Copy/link to this API element")

```python
minimum = BinaryUFunc(_minimum, "minimum")
```

variable fmax[](https://docs.exaloop.io/libraries/api/modules/numpy/ndmath/#variable-fmax "Copy/link to this API element")

```python
fmax = BinaryUFunc(_fmax, "fmax")
```

variable fmin[](https://docs.exaloop.io/libraries/api/modules/numpy/ndmath/#variable-fmin "Copy/link to this API element")

```python
fmin = BinaryUFunc(_fmin, "fmin")
```

variable isfinite[](https://docs.exaloop.io/libraries/api/modules/numpy/ndmath/#variable-isfinite "Copy/link to this API element")

```python
isfinite = UnaryUFunc(_isfinite, "isfinite")
```

variable isinf[](https://docs.exaloop.io/libraries/api/modules/numpy/ndmath/#variable-isinf "Copy/link to this API element")

```python
isinf = UnaryUFunc(_isinf, "isinf")
```

variable isnan[](https://docs.exaloop.io/libraries/api/modules/numpy/ndmath/#variable-isnan "Copy/link to this API element")

```python
isnan = UnaryUFunc(_isnan, "isnan")
```

variable isnat[](https://docs.exaloop.io/libraries/api/modules/numpy/ndmath/#variable-isnat "Copy/link to this API element")

```python
isnat = UnaryUFunc(_isnat, "isnat")
```

variable signbit[](https://docs.exaloop.io/libraries/api/modules/numpy/ndmath/#variable-signbit "Copy/link to this API element")

```python
signbit = UnaryUFunc(_signbit, "signbit")
```

variable copysign[](https://docs.exaloop.io/libraries/api/modules/numpy/ndmath/#variable-copysign "Copy/link to this API element")

```python
copysign = BinaryUFunc(_copysign, "copysign")
```

variable nextafter[](https://docs.exaloop.io/libraries/api/modules/numpy/ndmath/#variable-nextafter "Copy/link to this API element")

```python
nextafter = BinaryUFunc(_nextafter, "nextafter")
```

variable spacing[](https://docs.exaloop.io/libraries/api/modules/numpy/ndmath/#variable-spacing "Copy/link to this API element")

```python
spacing = UnaryUFunc(_spacing, "spacing")
```

variable modf[](https://docs.exaloop.io/libraries/api/modules/numpy/ndmath/#variable-modf "Copy/link to this API element")

```python
modf = UnaryUFunc2(_modf, "modf")
```

variable ldexp[](https://docs.exaloop.io/libraries/api/modules/numpy/ndmath/#variable-ldexp "Copy/link to this API element")

```python
ldexp = BinaryUFunc(_ldexp, "ldexp")
```

variable frexp[](https://docs.exaloop.io/libraries/api/modules/numpy/ndmath/#variable-frexp "Copy/link to this API element")

```python
frexp = UnaryUFunc2(_frexp, "frexp")
```

variable floor[](https://docs.exaloop.io/libraries/api/modules/numpy/ndmath/#variable-floor "Copy/link to this API element")

```python
floor = UnaryUFunc(_floor, "floor")
```

variable ceil[](https://docs.exaloop.io/libraries/api/modules/numpy/ndmath/#variable-ceil "Copy/link to this API element")

```python
ceil = UnaryUFunc(_ceil, "ceil")
```

variable trunc[](https://docs.exaloop.io/libraries/api/modules/numpy/ndmath/#variable-trunc "Copy/link to this API element")

```python
trunc = UnaryUFunc(_trunc, "trunc")
```

function i0[](https://docs.exaloop.io/libraries/api/modules/numpy/ndmath/#function-i0 "Copy/link to this API element")

```python
i0(x)
```

function sinc[](https://docs.exaloop.io/libraries/api/modules/numpy/ndmath/#function-sinc "Copy/link to this API element")

```python
sinc(x)
```

function nancumprod[](https://docs.exaloop.io/libraries/api/modules/numpy/ndmath/#function-nancumprod "Copy/link to this API element")

```python
nancumprod(a, axis = None, dtype: type = NoneType, out = None)
```

function nancumsum[](https://docs.exaloop.io/libraries/api/modules/numpy/ndmath/#function-nancumsum "Copy/link to this API element")

```python
nancumsum(a, axis = None, dtype: type = NoneType, out = None)
```

class ndarray[](https://docs.exaloop.io/libraries/api/modules/numpy/ndmath/#class-ndarray "Copy/link to this API element")

@extend

```python
class ndarray[dtype, ndim]
```

Extends [`numpy.ndarray.ndarray`](https://docs.exaloop.io/libraries/api/classes/numpy/ndarray/ndarray/) · [View complete class API →](https://docs.exaloop.io/libraries/api/classes/numpy/ndarray/ndarray/)

Methods 2

cumprod[](https://docs.exaloop.io/libraries/api/modules/numpy/ndmath/#class-ndarray-cumprod "Copy/link to this API element")

```python
cumprod(self: Self, axis = None, dtype: type = NoneType, out = None, Self: type)
```

cumsum[](https://docs.exaloop.io/libraries/api/modules/numpy/ndmath/#class-ndarray-cumsum "Copy/link to this API element")

```python
cumsum(self: Self, axis = None, dtype: type = NoneType, out = None, Self: type)
```

Special methods 1

\_\_abs\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/ndmath/#class-ndarray-__abs__ "Copy/link to this API element")

@inline

```python
__abs__(self: Self, Self: type)
```

Source: https://docs.exaloop.io/libraries/api/modules/numpy/ndmath/
