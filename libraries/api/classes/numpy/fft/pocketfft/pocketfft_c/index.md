# pocketfft\_c

Standard library class `numpy.fft.pocketfft.pocketfft_c` [View source](https://github.com/exaloop/codon/blob/master/stdlib/numpy/fft/pocketfft.codon)

class pocketfft\_c[](https://docs.exaloop.io/libraries/api/classes/numpy/fft/pocketfft/pocketfft_c/#class-pocketfft_c "Copy/link to this API element")

```python
class pocketfft_c[T, T0]
```

Fields 5

`packplan` `Optional[cfftp[T, T0]]`

`blueplan` `Optional[fftblue[T, T0]]`

`length` `int`

`T` `type`

`T0` `type`

Constructors 1

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/classes/numpy/fft/pocketfft/pocketfft_c/#class-pocketfft_c-__init__ "Copy/link to this API element")

```python
__init__(self: Self, length: int, Self: type)
```

Defined in [`numpy.fft.pocketfft`](https://docs.exaloop.io/libraries/api/modules/numpy/fft/pocketfft/)

Methods 1

exec[](https://docs.exaloop.io/libraries/api/classes/numpy/fft/pocketfft/pocketfft_c/#class-pocketfft_c-exec "Copy/link to this API element")

```python
exec(self: Self, c: Ptr[T], fct: T0, fwd: bool, Self: type)
```

Defined in [`numpy.fft.pocketfft`](https://docs.exaloop.io/libraries/api/modules/numpy/fft/pocketfft/)

Source: https://docs.exaloop.io/libraries/api/classes/numpy/fft/pocketfft/pocketfft_c/
