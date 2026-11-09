# internal.c\_stubs

Standard library module [View source](https://github.com/exaloop/codon/blob/master/stdlib/internal/c_stubs.codon)

function seq\_print[](https://docs.exaloop.io/libraries/api/modules/internal/c_stubs/#function-seq_print "Copy/link to this API element")

@C

```python
seq_print(_: str)
```

function seq\_print\_full[](https://docs.exaloop.io/libraries/api/modules/internal/c_stubs/#function-seq_print_full "Copy/link to this API element")

@C

```python
seq_print_full(_: str, _: cobj)
```

function linkname[](https://docs.exaloop.io/libraries/api/modules/internal/c_stubs/#function-linkname "Copy/link to this API element")

@\_\_attribute\_\_

```python
linkname(s: Literal[str])
```

function seq\_float\_from\_str[](https://docs.exaloop.io/libraries/api/modules/internal/c_stubs/#function-seq_float_from_str "Copy/link to this API element")

@nocapture @C

```python
seq_float_from_str(a: str, b: Ptr[cobj])
```

function seq\_strdup[](https://docs.exaloop.io/libraries/api/modules/internal/c_stubs/#function-seq_strdup "Copy/link to this API element")

@pure @C

```python
seq_strdup(a: cobj)
```

function seq\_stdin[](https://docs.exaloop.io/libraries/api/modules/internal/c_stubs/#function-seq_stdin "Copy/link to this API element")

@pure @llvm\_memory\_none @C

```python
seq_stdin()
```

function seq\_stdout[](https://docs.exaloop.io/libraries/api/modules/internal/c_stubs/#function-seq_stdout "Copy/link to this API element")

@pure @llvm\_memory\_none @C

```python
seq_stdout()
```

function seq\_stderr[](https://docs.exaloop.io/libraries/api/modules/internal/c_stubs/#function-seq_stderr "Copy/link to this API element")

@pure @llvm\_memory\_none @C

```python
seq_stderr()
```

function seq\_env[](https://docs.exaloop.io/libraries/api/modules/internal/c_stubs/#function-seq_env "Copy/link to this API element")

@no\_side\_effect @C

```python
seq_env()
```

function seq\_time[](https://docs.exaloop.io/libraries/api/modules/internal/c_stubs/#function-seq_time "Copy/link to this API element")

@no\_side\_effect @C

```python
seq_time()
```

function seq\_time\_monotonic[](https://docs.exaloop.io/libraries/api/modules/internal/c_stubs/#function-seq_time_monotonic "Copy/link to this API element")

@no\_side\_effect @C

```python
seq_time_monotonic()
```

function seq\_time\_highres[](https://docs.exaloop.io/libraries/api/modules/internal/c_stubs/#function-seq_time_highres "Copy/link to this API element")

@no\_side\_effect @C

```python
seq_time_highres()
```

function seq\_localtime[](https://docs.exaloop.io/libraries/api/modules/internal/c_stubs/#function-seq_localtime "Copy/link to this API element")

@no\_side\_effect @C

```python
seq_localtime(a: int, b: cobj)
```

function seq\_gmtime[](https://docs.exaloop.io/libraries/api/modules/internal/c_stubs/#function-seq_gmtime "Copy/link to this API element")

@no\_side\_effect @C

```python
seq_gmtime(a: int, b: cobj)
```

function seq\_mktime[](https://docs.exaloop.io/libraries/api/modules/internal/c_stubs/#function-seq_mktime "Copy/link to this API element")

@pure @C

```python
seq_mktime(a: cobj)
```

function seq\_sleep[](https://docs.exaloop.io/libraries/api/modules/internal/c_stubs/#function-seq_sleep "Copy/link to this API element")

@C

```python
seq_sleep(_: float)
```

function seq\_pid[](https://docs.exaloop.io/libraries/api/modules/internal/c_stubs/#function-seq_pid "Copy/link to this API element")

@pure @C

```python
seq_pid()
```

function seq\_lock\_new[](https://docs.exaloop.io/libraries/api/modules/internal/c_stubs/#function-seq_lock_new "Copy/link to this API element")

@pure @C

```python
seq_lock_new()
```

function seq\_lock\_acquire[](https://docs.exaloop.io/libraries/api/modules/internal/c_stubs/#function-seq_lock_acquire "Copy/link to this API element")

@nocapture @C

```python
seq_lock_acquire(a: cobj, b: bool, c: float)
```

function seq\_lock\_release[](https://docs.exaloop.io/libraries/api/modules/internal/c_stubs/#function-seq_lock_release "Copy/link to this API element")

@nocapture @C

```python
seq_lock_release(a: cobj)
```

function seq\_cond\_new[](https://docs.exaloop.io/libraries/api/modules/internal/c_stubs/#function-seq_cond_new "Copy/link to this API element")

@no\_side\_effect @C

```python
seq_cond_new()
```

function seq\_cond\_wait[](https://docs.exaloop.io/libraries/api/modules/internal/c_stubs/#function-seq_cond_wait "Copy/link to this API element")

@nocapture @C

```python
seq_cond_wait(condition: cobj, lock: cobj, timeout: float)
```

function seq\_cond\_notify[](https://docs.exaloop.io/libraries/api/modules/internal/c_stubs/#function-seq_cond_notify "Copy/link to this API element")

@nocapture @C

```python
seq_cond_notify(condition: cobj)
```

function seq\_rlock\_new[](https://docs.exaloop.io/libraries/api/modules/internal/c_stubs/#function-seq_rlock_new "Copy/link to this API element")

@pure @C

```python
seq_rlock_new()
```

function seq\_rlock\_acquire[](https://docs.exaloop.io/libraries/api/modules/internal/c_stubs/#function-seq_rlock_acquire "Copy/link to this API element")

@nocapture @C

```python
seq_rlock_acquire(a: cobj, b: bool, c: float)
```

function seq\_rlock\_release[](https://docs.exaloop.io/libraries/api/modules/internal/c_stubs/#function-seq_rlock_release "Copy/link to this API element")

@nocapture @C

```python
seq_rlock_release(a: cobj)
```

function seq\_i32\_to\_float[](https://docs.exaloop.io/libraries/api/modules/internal/c_stubs/#function-seq_i32_to_float "Copy/link to this API element")

@pure @C

```python
seq_i32_to_float(a: i32)
```

function isdigit[](https://docs.exaloop.io/libraries/api/modules/internal/c_stubs/#function-isdigit "Copy/link to this API element")

@pure @C

```python
isdigit(a: i32)
```

function isspace[](https://docs.exaloop.io/libraries/api/modules/internal/c_stubs/#function-isspace "Copy/link to this API element")

@pure @C

```python
isspace(a: i32)
```

function isupper[](https://docs.exaloop.io/libraries/api/modules/internal/c_stubs/#function-isupper "Copy/link to this API element")

@pure @C

```python
isupper(a: i32)
```

function islower[](https://docs.exaloop.io/libraries/api/modules/internal/c_stubs/#function-islower "Copy/link to this API element")

@pure @C

```python
islower(a: i32)
```

function toupper[](https://docs.exaloop.io/libraries/api/modules/internal/c_stubs/#function-toupper "Copy/link to this API element")

@pure @C

```python
toupper(a: i32)
```

function tolower[](https://docs.exaloop.io/libraries/api/modules/internal/c_stubs/#function-tolower "Copy/link to this API element")

@pure @C

```python
tolower(a: i32)
```

function isalnum[](https://docs.exaloop.io/libraries/api/modules/internal/c_stubs/#function-isalnum "Copy/link to this API element")

@pure @C

```python
isalnum(a: i32)
```

function isalpha[](https://docs.exaloop.io/libraries/api/modules/internal/c_stubs/#function-isalpha "Copy/link to this API element")

@pure @C

```python
isalpha(a: i32)
```

function ceil[](https://docs.exaloop.io/libraries/api/modules/internal/c_stubs/#function-ceil "Copy/link to this API element")

@pure @C

```python
ceil(a: float)
```

function floor[](https://docs.exaloop.io/libraries/api/modules/internal/c_stubs/#function-floor "Copy/link to this API element")

@pure @C

```python
floor(a: float)
```

function fabs[](https://docs.exaloop.io/libraries/api/modules/internal/c_stubs/#function-fabs "Copy/link to this API element")

@pure @C

```python
fabs(a: float)
```

function fmod[](https://docs.exaloop.io/libraries/api/modules/internal/c_stubs/#function-fmod "Copy/link to this API element")

@pure @C

```python
fmod(a: float, b: float)
```

function exp[](https://docs.exaloop.io/libraries/api/modules/internal/c_stubs/#function-exp "Copy/link to this API element")

@pure @C

```python
exp(a: float)
```

function expm1[](https://docs.exaloop.io/libraries/api/modules/internal/c_stubs/#function-expm1 "Copy/link to this API element")

@pure @C

```python
expm1(a: float)
```

function ldexp[](https://docs.exaloop.io/libraries/api/modules/internal/c_stubs/#function-ldexp "Copy/link to this API element")

@pure @C

```python
ldexp(a: float, b: i32)
```

function log[](https://docs.exaloop.io/libraries/api/modules/internal/c_stubs/#function-log "Copy/link to this API element")

@pure @C

```python
log(a: float)
```

function log10[](https://docs.exaloop.io/libraries/api/modules/internal/c_stubs/#function-log10 "Copy/link to this API element")

@pure @C

```python
log10(a: float)
```

function sqrt[](https://docs.exaloop.io/libraries/api/modules/internal/c_stubs/#function-sqrt "Copy/link to this API element")

@pure @C

```python
sqrt(a: float)
```

function cbrt[](https://docs.exaloop.io/libraries/api/modules/internal/c_stubs/#function-cbrt "Copy/link to this API element")

@pure @C

```python
cbrt(a: float)
```

function pow[](https://docs.exaloop.io/libraries/api/modules/internal/c_stubs/#function-pow "Copy/link to this API element")

@pure @C

```python
pow(a: float, b: float)
```

function round[](https://docs.exaloop.io/libraries/api/modules/internal/c_stubs/#function-round "Copy/link to this API element")

@pure @C

```python
round(a: float)
```

function nextafter[](https://docs.exaloop.io/libraries/api/modules/internal/c_stubs/#function-nextafter "Copy/link to this API element")

@pure @C

```python
nextafter(a: float, b: float)
```

function acos[](https://docs.exaloop.io/libraries/api/modules/internal/c_stubs/#function-acos "Copy/link to this API element")

@pure @C

```python
acos(a: float)
```

function asin[](https://docs.exaloop.io/libraries/api/modules/internal/c_stubs/#function-asin "Copy/link to this API element")

@pure @C

```python
asin(a: float)
```

function atan[](https://docs.exaloop.io/libraries/api/modules/internal/c_stubs/#function-atan "Copy/link to this API element")

@pure @C

```python
atan(a: float)
```

function atan2[](https://docs.exaloop.io/libraries/api/modules/internal/c_stubs/#function-atan2 "Copy/link to this API element")

@pure @C

```python
atan2(a: float, b: float)
```

function cos[](https://docs.exaloop.io/libraries/api/modules/internal/c_stubs/#function-cos "Copy/link to this API element")

@pure @C

```python
cos(a: float)
```

function sin[](https://docs.exaloop.io/libraries/api/modules/internal/c_stubs/#function-sin "Copy/link to this API element")

@pure @C

```python
sin(a: float)
```

function tan[](https://docs.exaloop.io/libraries/api/modules/internal/c_stubs/#function-tan "Copy/link to this API element")

@pure @C

```python
tan(a: float)
```

function cosh[](https://docs.exaloop.io/libraries/api/modules/internal/c_stubs/#function-cosh "Copy/link to this API element")

@pure @C

```python
cosh(a: float)
```

function sinh[](https://docs.exaloop.io/libraries/api/modules/internal/c_stubs/#function-sinh "Copy/link to this API element")

@pure @C

```python
sinh(a: float)
```

function tanh[](https://docs.exaloop.io/libraries/api/modules/internal/c_stubs/#function-tanh "Copy/link to this API element")

@pure @C

```python
tanh(a: float)
```

function acosh[](https://docs.exaloop.io/libraries/api/modules/internal/c_stubs/#function-acosh "Copy/link to this API element")

@pure @C

```python
acosh(a: float)
```

function asinh[](https://docs.exaloop.io/libraries/api/modules/internal/c_stubs/#function-asinh "Copy/link to this API element")

@pure @C

```python
asinh(a: float)
```

function atanh[](https://docs.exaloop.io/libraries/api/modules/internal/c_stubs/#function-atanh "Copy/link to this API element")

@pure @C

```python
atanh(a: float)
```

function copysign[](https://docs.exaloop.io/libraries/api/modules/internal/c_stubs/#function-copysign "Copy/link to this API element")

@pure @C

```python
copysign(a: float, b: float)
```

function log1p[](https://docs.exaloop.io/libraries/api/modules/internal/c_stubs/#function-log1p "Copy/link to this API element")

@pure @C

```python
log1p(a: float)
```

function trunc[](https://docs.exaloop.io/libraries/api/modules/internal/c_stubs/#function-trunc "Copy/link to this API element")

@pure @C

```python
trunc(a: float)
```

function log2[](https://docs.exaloop.io/libraries/api/modules/internal/c_stubs/#function-log2 "Copy/link to this API element")

@pure @C

```python
log2(a: float)
```

function erf[](https://docs.exaloop.io/libraries/api/modules/internal/c_stubs/#function-erf "Copy/link to this API element")

@pure @C

```python
erf(a: float)
```

function erfc[](https://docs.exaloop.io/libraries/api/modules/internal/c_stubs/#function-erfc "Copy/link to this API element")

@pure @C

```python
erfc(a: float)
```

function tgamma[](https://docs.exaloop.io/libraries/api/modules/internal/c_stubs/#function-tgamma "Copy/link to this API element")

@pure @C

```python
tgamma(a: float)
```

function lgamma[](https://docs.exaloop.io/libraries/api/modules/internal/c_stubs/#function-lgamma "Copy/link to this API element")

@pure @C

```python
lgamma(a: float)
```

function remainder[](https://docs.exaloop.io/libraries/api/modules/internal/c_stubs/#function-remainder "Copy/link to this API element")

@pure @C

```python
remainder(a: float, b: float)
```

function hypot[](https://docs.exaloop.io/libraries/api/modules/internal/c_stubs/#function-hypot "Copy/link to this API element")

@pure @C

```python
hypot(a: float, b: float)
```

function frexp[](https://docs.exaloop.io/libraries/api/modules/internal/c_stubs/#function-frexp "Copy/link to this API element")

@nocapture @C

```python
frexp(a: float, b: Ptr[Int[u128]])
```

function modf[](https://docs.exaloop.io/libraries/api/modules/internal/c_stubs/#function-modf "Copy/link to this API element")

@nocapture @C

```python
modf(a: float, b: Ptr[float])
```

function ceilf[](https://docs.exaloop.io/libraries/api/modules/internal/c_stubs/#function-ceilf "Copy/link to this API element")

@pure @C

```python
ceilf(a: float32)
```

function floorf[](https://docs.exaloop.io/libraries/api/modules/internal/c_stubs/#function-floorf "Copy/link to this API element")

@pure @C

```python
floorf(a: float32)
```

function fabsf[](https://docs.exaloop.io/libraries/api/modules/internal/c_stubs/#function-fabsf "Copy/link to this API element")

@pure @C

```python
fabsf(a: float32)
```

function fmodf[](https://docs.exaloop.io/libraries/api/modules/internal/c_stubs/#function-fmodf "Copy/link to this API element")

@pure @C

```python
fmodf(a: float32, b: float32)
```

function expf[](https://docs.exaloop.io/libraries/api/modules/internal/c_stubs/#function-expf "Copy/link to this API element")

@pure @C

```python
expf(a: float32)
```

function expm1f[](https://docs.exaloop.io/libraries/api/modules/internal/c_stubs/#function-expm1f "Copy/link to this API element")

@pure @C

```python
expm1f(a: float32)
```

function ldexpf[](https://docs.exaloop.io/libraries/api/modules/internal/c_stubs/#function-ldexpf "Copy/link to this API element")

@pure @C

```python
ldexpf(a: float32, b: i32)
```

function logf[](https://docs.exaloop.io/libraries/api/modules/internal/c_stubs/#function-logf "Copy/link to this API element")

@pure @C

```python
logf(a: float32)
```

function log2f[](https://docs.exaloop.io/libraries/api/modules/internal/c_stubs/#function-log2f "Copy/link to this API element")

@pure @C

```python
log2f(a: float32)
```

function log10f[](https://docs.exaloop.io/libraries/api/modules/internal/c_stubs/#function-log10f "Copy/link to this API element")

@pure @C

```python
log10f(a: float32)
```

function sqrtf[](https://docs.exaloop.io/libraries/api/modules/internal/c_stubs/#function-sqrtf "Copy/link to this API element")

@pure @C

```python
sqrtf(a: float32)
```

function cbrtf[](https://docs.exaloop.io/libraries/api/modules/internal/c_stubs/#function-cbrtf "Copy/link to this API element")

@pure @C

```python
cbrtf(a: float32)
```

function powf[](https://docs.exaloop.io/libraries/api/modules/internal/c_stubs/#function-powf "Copy/link to this API element")

@pure @C

```python
powf(a: float32, b: float32)
```

function roundf[](https://docs.exaloop.io/libraries/api/modules/internal/c_stubs/#function-roundf "Copy/link to this API element")

@pure @C

```python
roundf(a: float32)
```

function nextafterf[](https://docs.exaloop.io/libraries/api/modules/internal/c_stubs/#function-nextafterf "Copy/link to this API element")

@pure @C

```python
nextafterf(a: float32, b: float32)
```

function acosf[](https://docs.exaloop.io/libraries/api/modules/internal/c_stubs/#function-acosf "Copy/link to this API element")

@pure @C

```python
acosf(a: float32)
```

function asinf[](https://docs.exaloop.io/libraries/api/modules/internal/c_stubs/#function-asinf "Copy/link to this API element")

@pure @C

```python
asinf(a: float32)
```

function atanf[](https://docs.exaloop.io/libraries/api/modules/internal/c_stubs/#function-atanf "Copy/link to this API element")

@pure @C

```python
atanf(a: float32)
```

function atan2f[](https://docs.exaloop.io/libraries/api/modules/internal/c_stubs/#function-atan2f "Copy/link to this API element")

@pure @C

```python
atan2f(a: float32, b: float32)
```

function cosf[](https://docs.exaloop.io/libraries/api/modules/internal/c_stubs/#function-cosf "Copy/link to this API element")

@pure @C

```python
cosf(a: float32)
```

function sinf[](https://docs.exaloop.io/libraries/api/modules/internal/c_stubs/#function-sinf "Copy/link to this API element")

@pure @C

```python
sinf(a: float32)
```

function tanf[](https://docs.exaloop.io/libraries/api/modules/internal/c_stubs/#function-tanf "Copy/link to this API element")

@pure @C

```python
tanf(a: float32)
```

function coshf[](https://docs.exaloop.io/libraries/api/modules/internal/c_stubs/#function-coshf "Copy/link to this API element")

@pure @C

```python
coshf(a: float32)
```

function sinhf[](https://docs.exaloop.io/libraries/api/modules/internal/c_stubs/#function-sinhf "Copy/link to this API element")

@pure @C

```python
sinhf(a: float32)
```

function tanhf[](https://docs.exaloop.io/libraries/api/modules/internal/c_stubs/#function-tanhf "Copy/link to this API element")

@pure @C

```python
tanhf(a: float32)
```

function acoshf[](https://docs.exaloop.io/libraries/api/modules/internal/c_stubs/#function-acoshf "Copy/link to this API element")

@pure @C

```python
acoshf(a: float32)
```

function asinhf[](https://docs.exaloop.io/libraries/api/modules/internal/c_stubs/#function-asinhf "Copy/link to this API element")

@pure @C

```python
asinhf(a: float32)
```

function atanhf[](https://docs.exaloop.io/libraries/api/modules/internal/c_stubs/#function-atanhf "Copy/link to this API element")

@pure @C

```python
atanhf(a: float32)
```

function copysignf[](https://docs.exaloop.io/libraries/api/modules/internal/c_stubs/#function-copysignf "Copy/link to this API element")

@pure @C

```python
copysignf(a: float32, b: float32)
```

function log1pf[](https://docs.exaloop.io/libraries/api/modules/internal/c_stubs/#function-log1pf "Copy/link to this API element")

@pure @C

```python
log1pf(a: float32)
```

function truncf[](https://docs.exaloop.io/libraries/api/modules/internal/c_stubs/#function-truncf "Copy/link to this API element")

@pure @C

```python
truncf(a: float32)
```

function erff[](https://docs.exaloop.io/libraries/api/modules/internal/c_stubs/#function-erff "Copy/link to this API element")

@pure @C

```python
erff(a: float32)
```

function erfcf[](https://docs.exaloop.io/libraries/api/modules/internal/c_stubs/#function-erfcf "Copy/link to this API element")

@pure @C

```python
erfcf(a: float32)
```

function tgammaf[](https://docs.exaloop.io/libraries/api/modules/internal/c_stubs/#function-tgammaf "Copy/link to this API element")

@pure @C

```python
tgammaf(a: float32)
```

function lgammaf[](https://docs.exaloop.io/libraries/api/modules/internal/c_stubs/#function-lgammaf "Copy/link to this API element")

@pure @C

```python
lgammaf(a: float32)
```

function remainderf[](https://docs.exaloop.io/libraries/api/modules/internal/c_stubs/#function-remainderf "Copy/link to this API element")

@pure @C

```python
remainderf(a: float32, b: float32)
```

function hypotf[](https://docs.exaloop.io/libraries/api/modules/internal/c_stubs/#function-hypotf "Copy/link to this API element")

@pure @C

```python
hypotf(a: float32, b: float32)
```

function frexpf[](https://docs.exaloop.io/libraries/api/modules/internal/c_stubs/#function-frexpf "Copy/link to this API element")

@nocapture @C

```python
frexpf(a: float32, b: Ptr[Int[u128]])
```

function modff[](https://docs.exaloop.io/libraries/api/modules/internal/c_stubs/#function-modff "Copy/link to this API element")

@nocapture @C

```python
modff(a: float32, b: Ptr[float32])
```

function ferror[](https://docs.exaloop.io/libraries/api/modules/internal/c_stubs/#function-ferror "Copy/link to this API element")

@pure @C

```python
ferror(a: cobj)
```

function fgetc[](https://docs.exaloop.io/libraries/api/modules/internal/c_stubs/#function-fgetc "Copy/link to this API element")

@nocapture @C

```python
fgetc(a: cobj)
```

function ungetc[](https://docs.exaloop.io/libraries/api/modules/internal/c_stubs/#function-ungetc "Copy/link to this API element")

@nocapture @C

```python
ungetc(a: i32, b: cobj)
```

function fopen[](https://docs.exaloop.io/libraries/api/modules/internal/c_stubs/#function-fopen "Copy/link to this API element")

@nocapture @C

```python
fopen(a: cobj, b: cobj)
```

function fdopen[](https://docs.exaloop.io/libraries/api/modules/internal/c_stubs/#function-fdopen "Copy/link to this API element")

@nocapture @C

```python
fdopen(a: int, b: cobj)
```

function fclose[](https://docs.exaloop.io/libraries/api/modules/internal/c_stubs/#function-fclose "Copy/link to this API element")

@nocapture @C

```python
fclose(a: cobj)
```

function fread[](https://docs.exaloop.io/libraries/api/modules/internal/c_stubs/#function-fread "Copy/link to this API element")

@nocapture @C

```python
fread(a: cobj, b: int, c: int, d: cobj)
```

function fwrite[](https://docs.exaloop.io/libraries/api/modules/internal/c_stubs/#function-fwrite "Copy/link to this API element")

@nocapture @C

```python
fwrite(a: cobj, b: int, c: int, d: cobj)
```

function ftell[](https://docs.exaloop.io/libraries/api/modules/internal/c_stubs/#function-ftell "Copy/link to this API element")

@nocapture @C

```python
ftell(a: cobj)
```

function fseek[](https://docs.exaloop.io/libraries/api/modules/internal/c_stubs/#function-fseek "Copy/link to this API element")

@nocapture @C

```python
fseek(a: cobj, b: int, c: i32)
```

function fgets[](https://docs.exaloop.io/libraries/api/modules/internal/c_stubs/#function-fgets "Copy/link to this API element")

@nocapture @C

```python
fgets(a: cobj, b: int, c: cobj)
```

function fflush[](https://docs.exaloop.io/libraries/api/modules/internal/c_stubs/#function-fflush "Copy/link to this API element")

@nocapture @C

```python
fflush(a: cobj)
```

function getline[](https://docs.exaloop.io/libraries/api/modules/internal/c_stubs/#function-getline "Copy/link to this API element")

@nocapture @C

```python
getline(a: Ptr[cobj], b: Ptr[int], c: cobj)
```

function exit[](https://docs.exaloop.io/libraries/api/modules/internal/c_stubs/#function-exit "Copy/link to this API element")

@C

```python
exit(_: int)
```

function system[](https://docs.exaloop.io/libraries/api/modules/internal/c_stubs/#function-system "Copy/link to this API element")

@nocapture @C

```python
system(a: cobj)
```

function free[](https://docs.exaloop.io/libraries/api/modules/internal/c_stubs/#function-free "Copy/link to this API element")

@nocapture @C

```python
free(a: cobj)
```

function atoi[](https://docs.exaloop.io/libraries/api/modules/internal/c_stubs/#function-atoi "Copy/link to this API element")

@pure @C

```python
atoi(a: cobj)
```

function gzopen[](https://docs.exaloop.io/libraries/api/modules/internal/c_stubs/#function-gzopen "Copy/link to this API element")

@nocapture @C

```python
gzopen(a: cobj, b: cobj)
```

function gzerror[](https://docs.exaloop.io/libraries/api/modules/internal/c_stubs/#function-gzerror "Copy/link to this API element")

@nocapture @C

```python
gzerror(a: cobj, b: Ptr[i32])
```

function gzgetc[](https://docs.exaloop.io/libraries/api/modules/internal/c_stubs/#function-gzgetc "Copy/link to this API element")

@nocapture @C

```python
gzgetc(a: cobj)
```

function gzgets[](https://docs.exaloop.io/libraries/api/modules/internal/c_stubs/#function-gzgets "Copy/link to this API element")

@nocapture @C

```python
gzgets(a: cobj, b: cobj, c: i32)
```

function gzclose[](https://docs.exaloop.io/libraries/api/modules/internal/c_stubs/#function-gzclose "Copy/link to this API element")

@nocapture @C

```python
gzclose(a: cobj)
```

function gzread[](https://docs.exaloop.io/libraries/api/modules/internal/c_stubs/#function-gzread "Copy/link to this API element")

@nocapture @C

```python
gzread(a: cobj, b: cobj, c: u32)
```

function gzwrite[](https://docs.exaloop.io/libraries/api/modules/internal/c_stubs/#function-gzwrite "Copy/link to this API element")

@nocapture @C

```python
gzwrite(a: cobj, b: cobj, c: u32)
```

function gztell[](https://docs.exaloop.io/libraries/api/modules/internal/c_stubs/#function-gztell "Copy/link to this API element")

@nocapture @C

```python
gztell(a: cobj)
```

function gzseek[](https://docs.exaloop.io/libraries/api/modules/internal/c_stubs/#function-gzseek "Copy/link to this API element")

@nocapture @C

```python
gzseek(a: cobj, b: int, c: i32)
```

function gzflush[](https://docs.exaloop.io/libraries/api/modules/internal/c_stubs/#function-gzflush "Copy/link to this API element")

@nocapture @C

```python
gzflush(a: cobj, b: i32)
```

function BZ2\_bzCompressInit[](https://docs.exaloop.io/libraries/api/modules/internal/c_stubs/#function-bz2_bzcompressinit "Copy/link to this API element")

@nocapture @C

```python
BZ2_bzCompressInit(stream: cobj, compresslevel: i32, verbosity: i32, work_factor: i32)
```

function BZ2\_bzCompress[](https://docs.exaloop.io/libraries/api/modules/internal/c_stubs/#function-bz2_bzcompress "Copy/link to this API element")

@nocapture @C

```python
BZ2_bzCompress(stream: cobj, action: i32)
```

function BZ2\_bzCompressEnd[](https://docs.exaloop.io/libraries/api/modules/internal/c_stubs/#function-bz2_bzcompressend "Copy/link to this API element")

@nocapture @C

```python
BZ2_bzCompressEnd(stream: cobj)
```

function BZ2\_bzDecompressInit[](https://docs.exaloop.io/libraries/api/modules/internal/c_stubs/#function-bz2_bzdecompressinit "Copy/link to this API element")

@nocapture @C

```python
BZ2_bzDecompressInit(stream: cobj, verbosity: i32, small: i32)
```

function BZ2\_bzDecompress[](https://docs.exaloop.io/libraries/api/modules/internal/c_stubs/#function-bz2_bzdecompress "Copy/link to this API element")

@nocapture @C

```python
BZ2_bzDecompress(stream: cobj)
```

function BZ2\_bzDecompressEnd[](https://docs.exaloop.io/libraries/api/modules/internal/c_stubs/#function-bz2_bzdecompressend "Copy/link to this API element")

@nocapture @C

```python
BZ2_bzDecompressEnd(stream: cobj)
```

function strlen[](https://docs.exaloop.io/libraries/api/modules/internal/c_stubs/#function-strlen "Copy/link to this API element")

@pure @C

```python
strlen(a: Ptr[u8])
```

function memcmp[](https://docs.exaloop.io/libraries/api/modules/internal/c_stubs/#function-memcmp "Copy/link to this API element")

@pure @C

```python
memcmp(lhs: Ptr[u8], rhs: Ptr[u8], count: int)
```

function memchr[](https://docs.exaloop.io/libraries/api/modules/internal/c_stubs/#function-memchr "Copy/link to this API element")

@pure @C

```python
memchr(p: Ptr[u8], ch: i32, count: int)
```

Source: https://docs.exaloop.io/libraries/api/modules/internal/c_stubs/
