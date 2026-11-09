# numpy.fft.pocketfft

Standard library module [View source](https://github.com/exaloop/codon/blob/master/stdlib/numpy/fft/pocketfft.codon)

function ROT90[](https://docs.exaloop.io/libraries/api/modules/numpy/fft/pocketfft/#function-rot90 "Copy/link to this API element")

```python
ROT90(a)
```

function ROTX90[](https://docs.exaloop.io/libraries/api/modules/numpy/fft/pocketfft/#function-rotx90 "Copy/link to this API element")

```python
ROTX90(a, fwd: bool)
```

function ROTX45[](https://docs.exaloop.io/libraries/api/modules/numpy/fft/pocketfft/#function-rotx45 "Copy/link to this API element")

```python
ROTX45(a, fwd: bool)
```

function ROTX135[](https://docs.exaloop.io/libraries/api/modules/numpy/fft/pocketfft/#function-rotx135 "Copy/link to this API element")

```python
ROTX135(a, fwd: bool)
```

class arr[](https://docs.exaloop.io/libraries/api/modules/numpy/fft/pocketfft/#class-arr "Copy/link to this API element")

@tuple

```python
class arr[T]
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/numpy/fft/pocketfft/arr/)

Fields 3

`p` `Ptr[T]`

`sz` `int`

`T` `type`

Constructors 1

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/fft/pocketfft/#class-arr-__new__ "Copy/link to this API element")

```python
__new__(sz: int)
```

Methods 3

dealloc[](https://docs.exaloop.io/libraries/api/modules/numpy/fft/pocketfft/#class-arr-dealloc "Copy/link to this API element")

```python
dealloc(self: Self, Self: type)
```

size[](https://docs.exaloop.io/libraries/api/modules/numpy/fft/pocketfft/#class-arr-size "Copy/link to this API element")

```python
size(self: Self, Self: type)
```

data[](https://docs.exaloop.io/libraries/api/modules/numpy/fft/pocketfft/#class-arr-data "Copy/link to this API element")

```python
data(self: Self, Self: type)
```

Special methods 2

\_\_getitem\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/fft/pocketfft/#class-arr-__getitem__ "Copy/link to this API element")

```python
__getitem__(self: Self, idx: int, Self: type)
```

\_\_setitem\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/fft/pocketfft/#class-arr-__setitem__ "Copy/link to this API element")

```python
__setitem__(self: Self, idx: int, val: T, Self: type)
```

class sincos\_2pibyn[](https://docs.exaloop.io/libraries/api/modules/numpy/fft/pocketfft/#class-sincos_2pibyn "Copy/link to this API element")

@tuple

```python
class sincos_2pibyn[T]
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/numpy/fft/pocketfft/sincos_2pibyn/)

Fields 6

`N` `int`

`mask` `int`

`shift` `int`

`v1` `arr[complex]`

`v2` `arr[complex]`

`T` `type`

Constructors 1

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/fft/pocketfft/#class-sincos_2pibyn-__new__ "Copy/link to this API element")

```python
__new__(n: int)
```

Methods 3

calc[](https://docs.exaloop.io/libraries/api/modules/numpy/fft/pocketfft/#class-sincos_2pibyn-calc "Copy/link to this API element")

```python
calc(x: int, n: int, ang: float)
```

cmplx[](https://docs.exaloop.io/libraries/api/modules/numpy/fft/pocketfft/#class-sincos_2pibyn-cmplx "Copy/link to this API element")

```python
cmplx(self: Self, re, im, Self: type)
```

dealloc[](https://docs.exaloop.io/libraries/api/modules/numpy/fft/pocketfft/#class-sincos_2pibyn-dealloc "Copy/link to this API element")

```python
dealloc(self: Self, Self: type)
```

Special methods 1

\_\_getitem\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/fft/pocketfft/#class-sincos_2pibyn-__getitem__ "Copy/link to this API element")

```python
__getitem__(self: Self, idx: int, Self: type)
```

function PM[](https://docs.exaloop.io/libraries/api/modules/numpy/fft/pocketfft/#function-pm "Copy/link to this API element")

```python
PM(a, b)
```

function MP[](https://docs.exaloop.io/libraries/api/modules/numpy/fft/pocketfft/#function-mp "Copy/link to this API element")

```python
MP(a, b)
```

class fctdata[](https://docs.exaloop.io/libraries/api/modules/numpy/fft/pocketfft/#class-fctdata "Copy/link to this API element")

@tuple

```python
class fctdata[T]
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/numpy/fft/pocketfft/fctdata/)

Fields 4

`fct` `int`

`tw` `Ptr[T]`

`tws` `Ptr[T]`

`T` `type`

Methods 3

with\_fct[](https://docs.exaloop.io/libraries/api/modules/numpy/fft/pocketfft/#class-fctdata-with_fct "Copy/link to this API element")

```python
with_fct(self: Self, fct: int, Self: type)
```

with\_tw[](https://docs.exaloop.io/libraries/api/modules/numpy/fft/pocketfft/#class-fctdata-with_tw "Copy/link to this API element")

```python
with_tw(self: Self, tw: Ptr[T], Self: type)
```

with\_tws[](https://docs.exaloop.io/libraries/api/modules/numpy/fft/pocketfft/#class-fctdata-with_tws "Copy/link to this API element")

```python
with_tws(self: Self, tws: Ptr[T], Self: type)
```

class cfftp[](https://docs.exaloop.io/libraries/api/modules/numpy/fft/pocketfft/#class-cfftp "Copy/link to this API element")

```python
class cfftp[T, T0]
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/numpy/fft/pocketfft/cfftp/)

Fields 5

`length` `int`

`mem` `arr[T]`

`fact` `List[fctdata[T]]`

`T` `type`

`T0` `type`

Constructors 1

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/fft/pocketfft/#class-cfftp-__init__ "Copy/link to this API element")

```python
__init__(self: Self, length: int, Self: type)
```

Methods 15

add\_factor[](https://docs.exaloop.io/libraries/api/modules/numpy/fft/pocketfft/#class-cfftp-add_factor "Copy/link to this API element")

```python
add_factor(self: Self, factor: int, Self: type)
```

pass2[](https://docs.exaloop.io/libraries/api/modules/numpy/fft/pocketfft/#class-cfftp-pass2 "Copy/link to this API element")

```python
pass2(ido: int, l1: int, cc: Ptr[T], ch: Ptr[T], wa: Ptr[T], fwd: bool)
```

pass3[](https://docs.exaloop.io/libraries/api/modules/numpy/fft/pocketfft/#class-cfftp-pass3 "Copy/link to this API element")

```python
pass3(ido: int, l1: int, cc: Ptr[T], ch: Ptr[T], wa: Ptr[T], fwd: bool)
```

pass4[](https://docs.exaloop.io/libraries/api/modules/numpy/fft/pocketfft/#class-cfftp-pass4 "Copy/link to this API element")

```python
pass4(ido: int, l1: int, cc: Ptr[T], ch: Ptr[T], wa: Ptr[T], fwd: bool)
```

pass5[](https://docs.exaloop.io/libraries/api/modules/numpy/fft/pocketfft/#class-cfftp-pass5 "Copy/link to this API element")

```python
pass5(ido: int, l1: int, cc: Ptr[T], ch: Ptr[T], wa: Ptr[T], fwd: bool)
```

pass7[](https://docs.exaloop.io/libraries/api/modules/numpy/fft/pocketfft/#class-cfftp-pass7 "Copy/link to this API element")

```python
pass7(ido: int, l1: int, cc: Ptr[T], ch: Ptr[T], wa: Ptr[T], fwd: bool)
```

pass8[](https://docs.exaloop.io/libraries/api/modules/numpy/fft/pocketfft/#class-cfftp-pass8 "Copy/link to this API element")

```python
pass8(ido: int, l1: int, cc: Ptr[T], ch: Ptr[T], wa: Ptr[T], fwd: bool)
```

pass11[](https://docs.exaloop.io/libraries/api/modules/numpy/fft/pocketfft/#class-cfftp-pass11 "Copy/link to this API element")

```python
pass11(ido: int, l1: int, cc: Ptr[T], ch: Ptr[T], wa: Ptr[T], fwd: bool)
```

passg[](https://docs.exaloop.io/libraries/api/modules/numpy/fft/pocketfft/#class-cfftp-passg "Copy/link to this API element")

```python
passg(ido: int, ip: int, l1: int, cc: Ptr[T], ch: Ptr[T], wa: Ptr[T], csarr: Ptr[T], fwd: bool)
```

pass\_all[](https://docs.exaloop.io/libraries/api/modules/numpy/fft/pocketfft/#class-cfftp-pass_all "Copy/link to this API element")

```python
pass_all(self: Self, c: Ptr[T], fct: T0, fwd: bool, Self: type)
```

exec[](https://docs.exaloop.io/libraries/api/modules/numpy/fft/pocketfft/#class-cfftp-exec "Copy/link to this API element")

```python
exec(self: Self, c: Ptr[T], fct: T0, fwd: bool, Self: type)
```

factorize[](https://docs.exaloop.io/libraries/api/modules/numpy/fft/pocketfft/#class-cfftp-factorize "Copy/link to this API element")

```python
factorize(self: Self, Self: type)
```

twsize[](https://docs.exaloop.io/libraries/api/modules/numpy/fft/pocketfft/#class-cfftp-twsize "Copy/link to this API element")

```python
twsize(self: Self, Self: type)
```

comp\_twiddle[](https://docs.exaloop.io/libraries/api/modules/numpy/fft/pocketfft/#class-cfftp-comp_twiddle "Copy/link to this API element")

```python
comp_twiddle(self: Self, Self: type)
```

dealloc[](https://docs.exaloop.io/libraries/api/modules/numpy/fft/pocketfft/#class-cfftp-dealloc "Copy/link to this API element")

```python
dealloc(self: Self, Self: type)
```

function MULPM[](https://docs.exaloop.io/libraries/api/modules/numpy/fft/pocketfft/#function-mulpm "Copy/link to this API element")

```python
MULPM(c, d, e, f)
```

function REARRANGE[](https://docs.exaloop.io/libraries/api/modules/numpy/fft/pocketfft/#function-rearrange "Copy/link to this API element")

```python
REARRANGE(rx, ix, ry, iy)
```

class rfftp[](https://docs.exaloop.io/libraries/api/modules/numpy/fft/pocketfft/#class-rfftp "Copy/link to this API element")

```python
class rfftp[T, T0]
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/numpy/fft/pocketfft/rfftp/)

Fields 5

`length` `int`

`mem` `arr[T0]`

`fact` `List[fctdata[T0]]`

`T` `type`

`T0` `type`

Constructors 1

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/fft/pocketfft/#class-rfftp-__init__ "Copy/link to this API element")

```python
__init__(self: Self, length: int, Self: type)
```

Methods 17

add\_factor[](https://docs.exaloop.io/libraries/api/modules/numpy/fft/pocketfft/#class-rfftp-add_factor "Copy/link to this API element")

```python
add_factor(self: Self, factor: int, Self: type)
```

radf2[](https://docs.exaloop.io/libraries/api/modules/numpy/fft/pocketfft/#class-rfftp-radf2 "Copy/link to this API element")

```python
radf2(ido: int, l1: int, cc: Ptr[T0], ch: Ptr[T0], wa: Ptr[T0])
```

radf3[](https://docs.exaloop.io/libraries/api/modules/numpy/fft/pocketfft/#class-rfftp-radf3 "Copy/link to this API element")

```python
radf3(ido: int, l1: int, cc: Ptr[T0], ch: Ptr[T0], wa: Ptr[T0])
```

radf4[](https://docs.exaloop.io/libraries/api/modules/numpy/fft/pocketfft/#class-rfftp-radf4 "Copy/link to this API element")

```python
radf4(ido: int, l1: int, cc: Ptr[T0], ch: Ptr[T0], wa: Ptr[T0])
```

radf5[](https://docs.exaloop.io/libraries/api/modules/numpy/fft/pocketfft/#class-rfftp-radf5 "Copy/link to this API element")

```python
radf5(ido: int, l1: int, cc: Ptr[T0], ch: Ptr[T0], wa: Ptr[T0])
```

radfg[](https://docs.exaloop.io/libraries/api/modules/numpy/fft/pocketfft/#class-rfftp-radfg "Copy/link to this API element")

```python
radfg(ido: int, ip: int, l1: int, cc: Ptr[T0], ch: Ptr[T0], wa: Ptr[T0], csarr: Ptr[T0])
```

radb2[](https://docs.exaloop.io/libraries/api/modules/numpy/fft/pocketfft/#class-rfftp-radb2 "Copy/link to this API element")

```python
radb2(ido: int, l1: int, cc: Ptr[T0], ch: Ptr[T0], wa: Ptr[T0])
```

radb3[](https://docs.exaloop.io/libraries/api/modules/numpy/fft/pocketfft/#class-rfftp-radb3 "Copy/link to this API element")

```python
radb3(ido: int, l1: int, cc: Ptr[T0], ch: Ptr[T0], wa: Ptr[T0])
```

radb4[](https://docs.exaloop.io/libraries/api/modules/numpy/fft/pocketfft/#class-rfftp-radb4 "Copy/link to this API element")

```python
radb4(ido: int, l1: int, cc: Ptr[T0], ch: Ptr[T0], wa: Ptr[T0])
```

radb5[](https://docs.exaloop.io/libraries/api/modules/numpy/fft/pocketfft/#class-rfftp-radb5 "Copy/link to this API element")

```python
radb5(ido: int, l1: int, cc: Ptr[T0], ch: Ptr[T0], wa: Ptr[T0])
```

radbg[](https://docs.exaloop.io/libraries/api/modules/numpy/fft/pocketfft/#class-rfftp-radbg "Copy/link to this API element")

```python
radbg(ido: int, ip: int, l1: int, cc: Ptr[T0], ch: Ptr[T0], wa: Ptr[T0], csarr: Ptr[T0])
```

copy\_and\_norm[](https://docs.exaloop.io/libraries/api/modules/numpy/fft/pocketfft/#class-rfftp-copy_and_norm "Copy/link to this API element")

```python
copy_and_norm(self: Self, c: Ptr[T0], p1: Ptr[T0], fct: T0, Self: type)
```

exec[](https://docs.exaloop.io/libraries/api/modules/numpy/fft/pocketfft/#class-rfftp-exec "Copy/link to this API element")

```python
exec(self: Self, c: Ptr[T0], fct: T0, r2hc: bool, Self: type)
```

factorize[](https://docs.exaloop.io/libraries/api/modules/numpy/fft/pocketfft/#class-rfftp-factorize "Copy/link to this API element")

```python
factorize(self: Self, Self: type)
```

twsize[](https://docs.exaloop.io/libraries/api/modules/numpy/fft/pocketfft/#class-rfftp-twsize "Copy/link to this API element")

```python
twsize(self: Self, Self: type)
```

comp\_twiddle[](https://docs.exaloop.io/libraries/api/modules/numpy/fft/pocketfft/#class-rfftp-comp_twiddle "Copy/link to this API element")

```python
comp_twiddle(self: Self, Self: type)
```

dealloc[](https://docs.exaloop.io/libraries/api/modules/numpy/fft/pocketfft/#class-rfftp-dealloc "Copy/link to this API element")

```python
dealloc(self: Self, Self: type)
```

class fftblue[](https://docs.exaloop.io/libraries/api/modules/numpy/fft/pocketfft/#class-fftblue "Copy/link to this API element")

```python
class fftblue[T, T0]
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/numpy/fft/pocketfft/fftblue/)

Fields 8

`n` `int`

`n2` `int`

`plan` `cfftp[T, T0]`

`mem` `arr[T]`

`bk` `Ptr[T]`

`bkf` `Ptr[T]`

`T` `type`

`T0` `type`

Constructors 1

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/fft/pocketfft/#class-fftblue-__init__ "Copy/link to this API element")

```python
__init__(self: Self, length: int, Self: type)
```

Methods 4

fft[](https://docs.exaloop.io/libraries/api/modules/numpy/fft/pocketfft/#class-fftblue-fft "Copy/link to this API element")

```python
fft(self: Self, c: Ptr[T], fct: T0, fwd: bool, Self: type)
```

exec[](https://docs.exaloop.io/libraries/api/modules/numpy/fft/pocketfft/#class-fftblue-exec "Copy/link to this API element")

```python
exec(self: Self, c: Ptr[T], fct: T0, fwd: bool, Self: type)
```

exec\_r[](https://docs.exaloop.io/libraries/api/modules/numpy/fft/pocketfft/#class-fftblue-exec_r "Copy/link to this API element")

```python
exec_r(self: Self, c: Ptr[T0], fct: T0, fwd: bool, Self: type)
```

dealloc[](https://docs.exaloop.io/libraries/api/modules/numpy/fft/pocketfft/#class-fftblue-dealloc "Copy/link to this API element")

```python
dealloc(self: Self, Self: type)
```

class pocketfft\_c[](https://docs.exaloop.io/libraries/api/modules/numpy/fft/pocketfft/#class-pocketfft_c "Copy/link to this API element")

```python
class pocketfft_c[T, T0]
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/numpy/fft/pocketfft/pocketfft_c/)

Fields 5

`packplan` `Optional[cfftp[T, T0]]`

`blueplan` `Optional[fftblue[T, T0]]`

`length` `int`

`T` `type`

`T0` `type`

Constructors 1

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/fft/pocketfft/#class-pocketfft_c-__init__ "Copy/link to this API element")

```python
__init__(self: Self, length: int, Self: type)
```

Methods 1

exec[](https://docs.exaloop.io/libraries/api/modules/numpy/fft/pocketfft/#class-pocketfft_c-exec "Copy/link to this API element")

```python
exec(self: Self, c: Ptr[T], fct: T0, fwd: bool, Self: type)
```

class pocketfft\_r[](https://docs.exaloop.io/libraries/api/modules/numpy/fft/pocketfft/#class-pocketfft_r "Copy/link to this API element")

```python
class pocketfft_r[T, T0]
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/numpy/fft/pocketfft/pocketfft_r/)

Fields 5

`packplan` `Optional[rfftp[T, T0]]`

`blueplan` `Optional[fftblue[T, T0]]`

`length` `int`

`T` `type`

`T0` `type`

Constructors 1

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/fft/pocketfft/#class-pocketfft_r-__init__ "Copy/link to this API element")

```python
__init__(self: Self, length: int, Self: type)
```

Methods 1

exec[](https://docs.exaloop.io/libraries/api/modules/numpy/fft/pocketfft/#class-pocketfft_r-exec "Copy/link to this API element")

```python
exec(self: Self, c: Ptr[T0], fct: T0, fwd: bool, Self: type)
```

function fft[](https://docs.exaloop.io/libraries/api/modules/numpy/fft/pocketfft/#function-fft "Copy/link to this API element")

```python
fft(v: Ptr[T], n: int, norm: Optional[str], T: type)
```

function ifft[](https://docs.exaloop.io/libraries/api/modules/numpy/fft/pocketfft/#function-ifft "Copy/link to this API element")

```python
ifft(v: Ptr[T], n: int, norm: Optional[str], T: type)
```

function rfft[](https://docs.exaloop.io/libraries/api/modules/numpy/fft/pocketfft/#function-rfft "Copy/link to this API element")

```python
rfft(v: Ptr[T0], n: int, is_forward: bool, norm: Optional[str], o: Ptr[T], T: type, T0: type)
```

function irfft[](https://docs.exaloop.io/libraries/api/modules/numpy/fft/pocketfft/#function-irfft "Copy/link to this API element")

```python
irfft(v: Ptr[T], n: int, norm: Optional[str], o: Ptr[T0], T: type, T0: type)
```

Source: https://docs.exaloop.io/libraries/api/modules/numpy/fft/pocketfft/
