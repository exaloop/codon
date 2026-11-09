# numpy.linalg.linalg

Standard library module [View source](https://github.com/exaloop/codon/blob/master/stdlib/numpy/linalg/linalg.codon)

class LinAlgError[](https://docs.exaloop.io/libraries/api/modules/numpy/linalg/linalg/#class-linalgerror "Copy/link to this API element")

```python
class LinAlgError
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/numpy/linalg/linalg/LinAlgError/)

Constructors 1

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/linalg/linalg/#class-linalgerror-__init__ "Copy/link to this API element")

```python
__init__(self: Self, message: str = "", Self: type)
```

class LinearizeData[](https://docs.exaloop.io/libraries/api/modules/numpy/linalg/linalg/#class-linearizedata "Copy/link to this API element")

@tuple

```python
class LinearizeData
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/numpy/linalg/linalg/LinearizeData/)

Fields 5

`rows` `int`

`cols` `int`

`row_strides` `int`

`col_strides` `int`

`out_lead_dim` `int`

Constructors 1

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/linalg/linalg/#class-linearizedata-__new__ "Copy/link to this API element")

```python
__new__(rows: int, cols: int, row_strides: int, col_strides: int)
```

Methods 5

linearize[](https://docs.exaloop.io/libraries/api/modules/numpy/linalg/linalg/#class-linearizedata-linearize "Copy/link to this API element")

```python
linearize(self: Self, dst: Ptr[T], src: Ptr[T], T: type, Self: type)
```

delinearize[](https://docs.exaloop.io/libraries/api/modules/numpy/linalg/linalg/#class-linearizedata-delinearize "Copy/link to this API element")

```python
delinearize(self: Self, dst: Ptr[T], src: Ptr[T], T: type, Self: type)
```

nan\_matrix[](https://docs.exaloop.io/libraries/api/modules/numpy/linalg/linalg/#class-linearizedata-nan_matrix "Copy/link to this API element")

```python
nan_matrix(self: Self, dst: Ptr[T], T: type, Self: type)
```

zero\_matrix[](https://docs.exaloop.io/libraries/api/modules/numpy/linalg/linalg/#class-linearizedata-zero_matrix "Copy/link to this API element")

```python
zero_matrix(self: Self, dst: Ptr[T], T: type, Self: type)
```

identity\_matrix[](https://docs.exaloop.io/libraries/api/modules/numpy/linalg/linalg/#class-linearizedata-identity_matrix "Copy/link to this API element")

```python
identity_matrix(dst: Ptr[T], n: int, T: type)
```

class SlogdetResult[](https://docs.exaloop.io/libraries/api/modules/numpy/linalg/linalg/#class-slogdetresult "Copy/link to this API element")

@tuple

```python
class SlogdetResult[A, B]
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/numpy/linalg/linalg/SlogdetResult/)

Fields 4

`sign` `A`

`logabsdet` `B`

`A` `type`

`B` `type`

Special methods 1

\_\_getitem\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/linalg/linalg/#class-slogdetresult-__getitem__ "Copy/link to this API element")

```python
__getitem__(self: Self, idx: Literal[int], Self: type)
```

function slogdet[](https://docs.exaloop.io/libraries/api/modules/numpy/linalg/linalg/#function-slogdet "Copy/link to this API element")

```python
slogdet(a)
```

function det[](https://docs.exaloop.io/libraries/api/modules/numpy/linalg/linalg/#function-det "Copy/link to this API element")

```python
det(a)
```

class EighParams[](https://docs.exaloop.io/libraries/api/modules/numpy/linalg/linalg/#class-eighparams "Copy/link to this API element")

```python
class EighParams[T, B]
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/numpy/linalg/linalg/EighParams/)

Fields 14

`A` `Ptr[T]`

`W` `Ptr[B]`

`WORK` `Ptr[T]`

`RWORK` `Ptr[B]`

`IWORK` `Ptr[fint]`

`N` `fint`

`LWORK` `fint`

`LRWORK` `fint`

`LIWORK` `fint`

`JOBZ` `u8`

`UPLO` `u8`

`LDA` `fint`

`T` `type`

`B` `type`

Constructors 1

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/linalg/linalg/#class-eighparams-__init__ "Copy/link to this API element")

```python
__init__(self: Self, JOBZ: u8, UPLO: u8, N: fint, Self: type)
```

Methods 2

release[](https://docs.exaloop.io/libraries/api/modules/numpy/linalg/linalg/#class-eighparams-release "Copy/link to this API element")

```python
release(self: Self, Self: type)
```

call[](https://docs.exaloop.io/libraries/api/modules/numpy/linalg/linalg/#class-eighparams-call "Copy/link to this API element")

```python
call(self: Self, Self: type)
```

class EighResult[](https://docs.exaloop.io/libraries/api/modules/numpy/linalg/linalg/#class-eighresult "Copy/link to this API element")

@tuple

```python
class EighResult[A, B]
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/numpy/linalg/linalg/EighResult/)

Fields 4

`eigenvalues` `A`

`eigenvectors` `B`

`A` `type`

`B` `type`

Special methods 1

\_\_getitem\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/linalg/linalg/#class-eighresult-__getitem__ "Copy/link to this API element")

```python
__getitem__(self: Self, idx: Literal[int], Self: type)
```

function eigh[](https://docs.exaloop.io/libraries/api/modules/numpy/linalg/linalg/#function-eigh "Copy/link to this API element")

```python
eigh(a, UPLO: str = "L")
```

function eigvalsh[](https://docs.exaloop.io/libraries/api/modules/numpy/linalg/linalg/#function-eigvalsh "Copy/link to this API element")

```python
eigvalsh(a, UPLO: str = "L")
```

class GesvParams[](https://docs.exaloop.io/libraries/api/modules/numpy/linalg/linalg/#class-gesvparams "Copy/link to this API element")

```python
class GesvParams[T]
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/numpy/linalg/linalg/GesvParams/)

Fields 8

`A` `Ptr[T]`

`B` `Ptr[T]`

`IPIV` `Ptr[fint]`

`N` `fint`

`NRHS` `fint`

`LDA` `fint`

`LDB` `fint`

`T` `type`

Constructors 1

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/linalg/linalg/#class-gesvparams-__init__ "Copy/link to this API element")

```python
__init__(self: Self, N: fint, NRHS: fint, Self: type)
```

Methods 2

release[](https://docs.exaloop.io/libraries/api/modules/numpy/linalg/linalg/#class-gesvparams-release "Copy/link to this API element")

```python
release(self: Self, Self: type)
```

call[](https://docs.exaloop.io/libraries/api/modules/numpy/linalg/linalg/#class-gesvparams-call "Copy/link to this API element")

```python
call(self: Self, Self: type)
```

function solve[](https://docs.exaloop.io/libraries/api/modules/numpy/linalg/linalg/#function-solve "Copy/link to this API element")

```python
solve(a, b)
```

function inv[](https://docs.exaloop.io/libraries/api/modules/numpy/linalg/linalg/#function-inv "Copy/link to this API element")

```python
inv(a)
```

class PotrfParams[](https://docs.exaloop.io/libraries/api/modules/numpy/linalg/linalg/#class-potrfparams "Copy/link to this API element")

```python
class PotrfParams[T]
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/numpy/linalg/linalg/PotrfParams/)

Fields 5

`A` `Ptr[T]`

`N` `fint`

`LDA` `fint`

`UPLO` `u8`

`T` `type`

Constructors 1

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/linalg/linalg/#class-potrfparams-__init__ "Copy/link to this API element")

```python
__init__(self: Self, UPLO: u8, N: fint, Self: type)
```

Methods 4

zero\_lower\_triangle[](https://docs.exaloop.io/libraries/api/modules/numpy/linalg/linalg/#class-potrfparams-zero_lower_triangle "Copy/link to this API element")

```python
zero_lower_triangle(self: Self, Self: type)
```

zero\_upper\_triangle[](https://docs.exaloop.io/libraries/api/modules/numpy/linalg/linalg/#class-potrfparams-zero_upper_triangle "Copy/link to this API element")

```python
zero_upper_triangle(self: Self, Self: type)
```

call[](https://docs.exaloop.io/libraries/api/modules/numpy/linalg/linalg/#class-potrfparams-call "Copy/link to this API element")

```python
call(self: Self, Self: type)
```

release[](https://docs.exaloop.io/libraries/api/modules/numpy/linalg/linalg/#class-potrfparams-release "Copy/link to this API element")

```python
release(self: Self, Self: type)
```

function cholesky[](https://docs.exaloop.io/libraries/api/modules/numpy/linalg/linalg/#function-cholesky "Copy/link to this API element")

```python
cholesky(a, upper: bool = False)
```

class GeevParams[](https://docs.exaloop.io/libraries/api/modules/numpy/linalg/linalg/#class-geevparams "Copy/link to this API element")

```python
class GeevParams[T, B]
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/numpy/linalg/linalg/GeevParams/)

Fields 18

`A` `Ptr[T]`

`WR` `Ptr[B]`

`WI` `Ptr[T]`

`VLR` `Ptr[T]`

`VRR` `Ptr[T]`

`WORK` `Ptr[T]`

`W` `Ptr[T]`

`VL` `Ptr[T]`

`VR` `Ptr[T]`

`N` `fint`

`LDA` `fint`

`LDVL` `fint`

`LDVR` `fint`

`LWORK` `fint`

`JOBVL` `u8`

`JOBVR` `u8`

`T` `type`

`B` `type`

Constructors 1

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/linalg/linalg/#class-geevparams-__init__ "Copy/link to this API element")

```python
__init__(self: Self, jobvl: u8, jobvr: u8, n: fint, Self: type)
```

Methods 3

call[](https://docs.exaloop.io/libraries/api/modules/numpy/linalg/linalg/#class-geevparams-call "Copy/link to this API element")

```python
call(self: Self, Self: type)
```

release[](https://docs.exaloop.io/libraries/api/modules/numpy/linalg/linalg/#class-geevparams-release "Copy/link to this API element")

```python
release(self: Self, Self: type)
```

process\_geev\_results[](https://docs.exaloop.io/libraries/api/modules/numpy/linalg/linalg/#class-geevparams-process_geev_results "Copy/link to this API element")

```python
process_geev_results(self: Self, Self: type)
```

class EigResult[](https://docs.exaloop.io/libraries/api/modules/numpy/linalg/linalg/#class-eigresult "Copy/link to this API element")

@tuple

```python
class EigResult[A, B]
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/numpy/linalg/linalg/EigResult/)

Fields 4

`eigenvalues` `A`

`eigenvectors` `B`

`A` `type`

`B` `type`

Special methods 1

\_\_getitem\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/linalg/linalg/#class-eigresult-__getitem__ "Copy/link to this API element")

```python
__getitem__(self: Self, idx: Literal[int], Self: type)
```

function eig[](https://docs.exaloop.io/libraries/api/modules/numpy/linalg/linalg/#function-eig "Copy/link to this API element")

```python
eig(a)
```

function eigvals[](https://docs.exaloop.io/libraries/api/modules/numpy/linalg/linalg/#function-eigvals "Copy/link to this API element")

```python
eigvals(a)
```

class GesddParams[](https://docs.exaloop.io/libraries/api/modules/numpy/linalg/linalg/#class-gesddparams "Copy/link to this API element")

```python
class GesddParams[T, B]
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/numpy/linalg/linalg/GesddParams/)

Fields 16

`A` `Ptr[T]`

`S` `Ptr[B]`

`U` `Ptr[T]`

`VT` `Ptr[T]`

`WORK` `Ptr[T]`

`RWORK` `Ptr[B]`

`IWORK` `Ptr[fint]`

`M` `fint`

`N` `fint`

`LDA` `fint`

`LDU` `fint`

`LDVT` `fint`

`LWORK` `fint`

`JOBZ` `u8`

`T` `type`

`B` `type`

Constructors 1

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/linalg/linalg/#class-gesddparams-__init__ "Copy/link to this API element")

```python
__init__(self: Self, jobz: u8, m: fint, n: fint, Self: type)
```

Methods 2

call[](https://docs.exaloop.io/libraries/api/modules/numpy/linalg/linalg/#class-gesddparams-call "Copy/link to this API element")

```python
call(self: Self, Self: type)
```

release[](https://docs.exaloop.io/libraries/api/modules/numpy/linalg/linalg/#class-gesddparams-release "Copy/link to this API element")

```python
release(self: Self, Self: type)
```

class SVDResult[](https://docs.exaloop.io/libraries/api/modules/numpy/linalg/linalg/#class-svdresult "Copy/link to this API element")

@tuple

```python
class SVDResult[A1, A2]
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/numpy/linalg/linalg/SVDResult/)

Fields 5

`U` `A1`

`S` `A2`

`Vh` `A1`

`A1` `type`

`A2` `type`

Special methods 1

\_\_getitem\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/linalg/linalg/#class-svdresult-__getitem__ "Copy/link to this API element")

```python
__getitem__(self: Self, idx: Literal[int], Self: type)
```

function svd[](https://docs.exaloop.io/libraries/api/modules/numpy/linalg/linalg/#function-svd "Copy/link to this API element")

```python
svd(a, full_matrices: bool = True, compute_uv: Literal[bool] = True, hermitian: bool = False)
```

function svdvals[](https://docs.exaloop.io/libraries/api/modules/numpy/linalg/linalg/#function-svdvals "Copy/link to this API element")

```python
svdvals(x)
```

class GeqrfParams[](https://docs.exaloop.io/libraries/api/modules/numpy/linalg/linalg/#class-geqrfparams "Copy/link to this API element")

```python
class GeqrfParams[T]
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/numpy/linalg/linalg/GeqrfParams/)

Fields 8

`M` `fint`

`N` `fint`

`A` `Ptr[T]`

`LDA` `fint`

`TAU` `Ptr[T]`

`WORK` `Ptr[T]`

`LWORK` `fint`

`T` `type`

Constructors 1

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/linalg/linalg/#class-geqrfparams-__init__ "Copy/link to this API element")

```python
__init__(self: Self, m: fint, n: fint, Self: type)
```

Methods 2

call[](https://docs.exaloop.io/libraries/api/modules/numpy/linalg/linalg/#class-geqrfparams-call "Copy/link to this API element")

```python
call(self: Self, Self: type)
```

release[](https://docs.exaloop.io/libraries/api/modules/numpy/linalg/linalg/#class-geqrfparams-release "Copy/link to this API element")

```python
release(self: Self, Self: type)
```

class GqrParams[](https://docs.exaloop.io/libraries/api/modules/numpy/linalg/linalg/#class-gqrparams "Copy/link to this API element")

```python
class GqrParams[T]
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/numpy/linalg/linalg/GqrParams/)

Fields 10

`M` `fint`

`MC` `fint`

`MN` `fint`

`A` `Ptr[T]`

`Q` `Ptr[T]`

`LDA` `fint`

`TAU` `Ptr[T]`

`WORK` `Ptr[T]`

`LWORK` `fint`

`T` `type`

Constructors 2

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/linalg/linalg/#class-gqrparams-__init__ "Copy/link to this API element")

```python
__init__(self: Self, m: fint, n: fint, mc: fint, Self: type)
```

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/linalg/linalg/#class-gqrparams-__init__-overload-2 "Copy/link to this API element")

```python
__init__(self: Self, m: fint, n: fint, Self: type)
```

Methods 2

call[](https://docs.exaloop.io/libraries/api/modules/numpy/linalg/linalg/#class-gqrparams-call "Copy/link to this API element")

```python
call(self: Self, Self: type)
```

release[](https://docs.exaloop.io/libraries/api/modules/numpy/linalg/linalg/#class-gqrparams-release "Copy/link to this API element")

```python
release(self: Self, Self: type)
```

class QRResult[](https://docs.exaloop.io/libraries/api/modules/numpy/linalg/linalg/#class-qrresult "Copy/link to this API element")

@tuple

```python
class QRResult[A]
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/numpy/linalg/linalg/QRResult/)

Fields 3

`Q` `A`

`R` `A`

`A` `type`

Special methods 1

\_\_getitem\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/linalg/linalg/#class-qrresult-__getitem__ "Copy/link to this API element")

```python
__getitem__(self: Self, idx: Literal[int], Self: type)
```

function qr[](https://docs.exaloop.io/libraries/api/modules/numpy/linalg/linalg/#function-qr "Copy/link to this API element")

```python
qr(a, mode: Literal[str] = "reduced")
```

class GelsdParams[](https://docs.exaloop.io/libraries/api/modules/numpy/linalg/linalg/#class-gelsdparams "Copy/link to this API element")

```python
class GelsdParams[T, B]
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/numpy/linalg/linalg/GelsdParams/)

Fields 16

`M` `fint`

`N` `fint`

`NRHS` `fint`

`A` `Ptr[T]`

`LDA` `fint`

`B_` `Ptr[T]`

`LDB` `fint`

`S` `Ptr[B]`

`RCOND` `B`

`RANK` `fint`

`WORK` `Ptr[T]`

`LWORK` `fint`

`RWORK` `Ptr[B]`

`IWORK` `Ptr[fint]`

`T` `type`

`B` `type`

Constructors 1

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/linalg/linalg/#class-gelsdparams-__init__ "Copy/link to this API element")

```python
__init__(self: Self, m: fint, n: fint, nrhs: fint, Self: type)
```

Methods 2

call[](https://docs.exaloop.io/libraries/api/modules/numpy/linalg/linalg/#class-gelsdparams-call "Copy/link to this API element")

```python
call(self: Self, Self: type)
```

release[](https://docs.exaloop.io/libraries/api/modules/numpy/linalg/linalg/#class-gelsdparams-release "Copy/link to this API element")

```python
release(self: Self, Self: type)
```

function lstsq[](https://docs.exaloop.io/libraries/api/modules/numpy/linalg/linalg/#function-lstsq "Copy/link to this API element")

```python
lstsq(a, b, rcond = None)
```

variable BLAS\_MAXSIZE[](https://docs.exaloop.io/libraries/api/modules/numpy/linalg/linalg/#variable-blas_maxsize "Copy/link to this API element")

```python
BLAS_MAXSIZE: Literal[int] = 2147483646
```

function matmul[](https://docs.exaloop.io/libraries/api/modules/numpy/linalg/linalg/#function-matmul "Copy/link to this API element")

```python
matmul(x1, x2)
```

function dot[](https://docs.exaloop.io/libraries/api/modules/numpy/linalg/linalg/#function-dot "Copy/link to this API element")

```python
dot(a, b, out = None)
```

function matrix\_power[](https://docs.exaloop.io/libraries/api/modules/numpy/linalg/linalg/#function-matrix_power "Copy/link to this API element")

```python
matrix_power(a, n: int)
```

@overload

```python
matrix_power(a, n: Literal[int])
```

function multi\_dot[](https://docs.exaloop.io/libraries/api/modules/numpy/linalg/linalg/#function-multi_dot "Copy/link to this API element")

```python
multi_dot(arrays, out = None)
```

function vdot[](https://docs.exaloop.io/libraries/api/modules/numpy/linalg/linalg/#function-vdot "Copy/link to this API element")

```python
vdot(a, b)
```

function outer[](https://docs.exaloop.io/libraries/api/modules/numpy/linalg/linalg/#function-outer "Copy/link to this API element")

```python
outer(a, b, out = None)
```

function inner[](https://docs.exaloop.io/libraries/api/modules/numpy/linalg/linalg/#function-inner "Copy/link to this API element")

```python
inner(a, b)
```

function tensordot[](https://docs.exaloop.io/libraries/api/modules/numpy/linalg/linalg/#function-tensordot "Copy/link to this API element")

```python
tensordot(a, b, axes)
```

@overload

```python
tensordot(a, b, axes: Literal[int] = 2)
```

function kron[](https://docs.exaloop.io/libraries/api/modules/numpy/linalg/linalg/#function-kron "Copy/link to this API element")

```python
kron(a, b)
```

function trace[](https://docs.exaloop.io/libraries/api/modules/numpy/linalg/linalg/#function-trace "Copy/link to this API element")

```python
trace(x, offset: int = 0, dtype: type = NoneType)
```

function tensorsolve[](https://docs.exaloop.io/libraries/api/modules/numpy/linalg/linalg/#function-tensorsolve "Copy/link to this API element")

```python
tensorsolve(a, b, axes = None)
```

function tensorinv[](https://docs.exaloop.io/libraries/api/modules/numpy/linalg/linalg/#function-tensorinv "Copy/link to this API element")

```python
tensorinv(a, ind: int = 2)
```

function norm[](https://docs.exaloop.io/libraries/api/modules/numpy/linalg/linalg/#function-norm "Copy/link to this API element")

```python
norm(x, ord = None, axis = None, keepdims: Literal[bool] = False)
```

function pinv[](https://docs.exaloop.io/libraries/api/modules/numpy/linalg/linalg/#function-pinv "Copy/link to this API element")

```python
pinv(a, rcond = 1e-15, hermitian: bool = False)
```

function matrix\_rank[](https://docs.exaloop.io/libraries/api/modules/numpy/linalg/linalg/#function-matrix_rank "Copy/link to this API element")

```python
matrix_rank(A, tol = None, hermitian: bool = False)
```

function cond[](https://docs.exaloop.io/libraries/api/modules/numpy/linalg/linalg/#function-cond "Copy/link to this API element")

```python
cond(x, p = None)
```

function matrix\_transpose[](https://docs.exaloop.io/libraries/api/modules/numpy/linalg/linalg/#function-matrix_transpose "Copy/link to this API element")

```python
matrix_transpose(x)
```

class ndarray[](https://docs.exaloop.io/libraries/api/modules/numpy/linalg/linalg/#class-ndarray "Copy/link to this API element")

@extend

```python
class ndarray[dtype, ndim]
```

Extends [`numpy.ndarray.ndarray`](https://docs.exaloop.io/libraries/api/classes/numpy/ndarray/ndarray/) · [View complete class API →](https://docs.exaloop.io/libraries/api/classes/numpy/ndarray/ndarray/)

Methods 2

dot[](https://docs.exaloop.io/libraries/api/modules/numpy/linalg/linalg/#class-ndarray-dot "Copy/link to this API element")

```python
dot(self: Self, other, Self: type)
```

trace[](https://docs.exaloop.io/libraries/api/modules/numpy/linalg/linalg/#class-ndarray-trace "Copy/link to this API element")

```python
trace(self: Self, offset: int = 0, axis1: int = 0, axis2: int = 1, dtype: type = NoneType, out = None, Self: type)
```

Special methods 1

\_\_matmul\_\_[](https://docs.exaloop.io/libraries/api/modules/numpy/linalg/linalg/#class-ndarray-__matmul__ "Copy/link to this API element")

```python
__matmul__(self: Self, other, Self: type)
```

Source: https://docs.exaloop.io/libraries/api/modules/numpy/linalg/linalg/
