# numpy.linalg.blas

Standard library module [View source](https://github.com/exaloop/codon/blob/master/stdlib/numpy/linalg/blas.codon)

variable fint[](https://docs.exaloop.io/libraries/api/modules/numpy/linalg/blas/#variable-fint "Copy/link to this API element")

```python
fint = i64
```

variable fintp[](https://docs.exaloop.io/libraries/api/modules/numpy/linalg/blas/#variable-fintp "Copy/link to this API element")

```python
fintp = Ptr[fint]
```

variable CBLAS\_ROW\_MAJOR[](https://docs.exaloop.io/libraries/api/modules/numpy/linalg/blas/#variable-cblas_row_major "Copy/link to this API element")

```python
CBLAS_ROW_MAJOR: Literal[int] = 101
```

variable CBLAS\_COL\_MAJOR[](https://docs.exaloop.io/libraries/api/modules/numpy/linalg/blas/#variable-cblas_col_major "Copy/link to this API element")

```python
CBLAS_COL_MAJOR: Literal[int] = 102
```

variable CBLAS\_NO\_TRANS[](https://docs.exaloop.io/libraries/api/modules/numpy/linalg/blas/#variable-cblas_no_trans "Copy/link to this API element")

```python
CBLAS_NO_TRANS: Literal[int] = 111
```

variable CBLAS\_TRANS[](https://docs.exaloop.io/libraries/api/modules/numpy/linalg/blas/#variable-cblas_trans "Copy/link to this API element")

```python
CBLAS_TRANS: Literal[int] = 112
```

variable CBLAS\_CONJ\_TRANS[](https://docs.exaloop.io/libraries/api/modules/numpy/linalg/blas/#variable-cblas_conj_trans "Copy/link to this API element")

```python
CBLAS_CONJ_TRANS: Literal[int] = 113
```

variable CBLAS\_UPPER[](https://docs.exaloop.io/libraries/api/modules/numpy/linalg/blas/#variable-cblas_upper "Copy/link to this API element")

```python
CBLAS_UPPER: Literal[int] = 121
```

variable CBLAS\_LOWER[](https://docs.exaloop.io/libraries/api/modules/numpy/linalg/blas/#variable-cblas_lower "Copy/link to this API element")

```python
CBLAS_LOWER: Literal[int] = 122
```

variable CBLAS\_NON\_UNIT[](https://docs.exaloop.io/libraries/api/modules/numpy/linalg/blas/#variable-cblas_non_unit "Copy/link to this API element")

```python
CBLAS_NON_UNIT: Literal[int] = 131
```

variable CBLAS\_UNIT[](https://docs.exaloop.io/libraries/api/modules/numpy/linalg/blas/#variable-cblas_unit "Copy/link to this API element")

```python
CBLAS_UNIT: Literal[int] = 132
```

variable CBLAS\_LEFT[](https://docs.exaloop.io/libraries/api/modules/numpy/linalg/blas/#variable-cblas_left "Copy/link to this API element")

```python
CBLAS_LEFT: Literal[int] = 141
```

variable CBLAS\_RIGHT[](https://docs.exaloop.io/libraries/api/modules/numpy/linalg/blas/#variable-cblas_right "Copy/link to this API element")

```python
CBLAS_RIGHT: Literal[int] = 142
```

variable LAPACK\_ROW\_MAJOR[](https://docs.exaloop.io/libraries/api/modules/numpy/linalg/blas/#variable-lapack_row_major "Copy/link to this API element")

```python
LAPACK_ROW_MAJOR: Literal[int] = 101
```

variable LAPACK\_COL\_MAJOR[](https://docs.exaloop.io/libraries/api/modules/numpy/linalg/blas/#variable-lapack_col_major "Copy/link to this API element")

```python
LAPACK_COL_MAJOR: Literal[int] = 102
```

function cblas\_dcopy[](https://docs.exaloop.io/libraries/api/modules/numpy/linalg/blas/#function-cblas_dcopy "Copy/link to this API element")

@C

```python
cblas_dcopy(n: fint, x: Ptr[float], incx: fint, y: Ptr[float], incy: fint)
```

function cblas\_scopy[](https://docs.exaloop.io/libraries/api/modules/numpy/linalg/blas/#function-cblas_scopy "Copy/link to this API element")

@C

```python
cblas_scopy(n: fint, x: Ptr[float32], incx: fint, y: Ptr[float32], incy: fint)
```

function cblas\_zcopy[](https://docs.exaloop.io/libraries/api/modules/numpy/linalg/blas/#function-cblas_zcopy "Copy/link to this API element")

@C

```python
cblas_zcopy(n: fint, x: Ptr[complex], incx: fint, y: Ptr[complex], incy: fint)
```

function cblas\_ccopy[](https://docs.exaloop.io/libraries/api/modules/numpy/linalg/blas/#function-cblas_ccopy "Copy/link to this API element")

@C

```python
cblas_ccopy(n: fint, x: Ptr[complex64], incx: fint, y: Ptr[complex64], incy: fint)
```

function cblas\_dgemm[](https://docs.exaloop.io/libraries/api/modules/numpy/linalg/blas/#function-cblas_dgemm "Copy/link to this API element")

@C

```python
cblas_dgemm(order: fint, transA: fint, transB: fint, M: fint, N: fint, K: fint, alpha: float, A: Ptr[float], lda: fint, B: Ptr[float], ldb: fint, beta: float, C: Ptr[float], ldc: fint)
```

function cblas\_sgemm[](https://docs.exaloop.io/libraries/api/modules/numpy/linalg/blas/#function-cblas_sgemm "Copy/link to this API element")

@C

```python
cblas_sgemm(order: fint, transA: fint, transB: fint, M: fint, N: fint, K: fint, alpha: float32, A: Ptr[float32], lda: fint, B: Ptr[float32], ldb: fint, beta: float32, C: Ptr[float32], ldc: fint)
```

function cblas\_zgemm[](https://docs.exaloop.io/libraries/api/modules/numpy/linalg/blas/#function-cblas_zgemm "Copy/link to this API element")

@C

```python
cblas_zgemm(order: fint, transA: fint, transB: fint, M: fint, N: fint, K: fint, alpha: cobj, A: Ptr[complex], lda: fint, B: Ptr[complex], ldb: fint, beta: cobj, C: Ptr[complex], ldc: fint)
```

function cblas\_cgemm[](https://docs.exaloop.io/libraries/api/modules/numpy/linalg/blas/#function-cblas_cgemm "Copy/link to this API element")

@C

```python
cblas_cgemm(order: fint, transA: fint, transB: fint, M: fint, N: fint, K: fint, alpha: cobj, A: Ptr[complex64], lda: fint, B: Ptr[complex64], ldb: fint, beta: cobj, C: Ptr[complex64], ldc: fint)
```

function cblas\_dsyrk[](https://docs.exaloop.io/libraries/api/modules/numpy/linalg/blas/#function-cblas_dsyrk "Copy/link to this API element")

@C

```python
cblas_dsyrk(order: fint, uplo: fint, trans: fint, N: fint, K: fint, alpha: float, A: Ptr[float], lda: fint, beta: float, C: Ptr[float], ldc: fint)
```

function cblas\_ssyrk[](https://docs.exaloop.io/libraries/api/modules/numpy/linalg/blas/#function-cblas_ssyrk "Copy/link to this API element")

@C

```python
cblas_ssyrk(order: fint, uplo: fint, trans: fint, N: fint, K: fint, alpha: float32, A: Ptr[float32], lda: fint, beta: float32, C: Ptr[float32], ldc: fint)
```

function cblas\_zsyrk[](https://docs.exaloop.io/libraries/api/modules/numpy/linalg/blas/#function-cblas_zsyrk "Copy/link to this API element")

@C

```python
cblas_zsyrk(order: fint, uplo: fint, trans: fint, N: fint, K: fint, alpha: cobj, A: Ptr[complex], lda: fint, beta: cobj, C: Ptr[complex], ldc: fint)
```

function cblas\_csyrk[](https://docs.exaloop.io/libraries/api/modules/numpy/linalg/blas/#function-cblas_csyrk "Copy/link to this API element")

@C

```python
cblas_csyrk(order: fint, uplo: fint, trans: fint, N: fint, K: fint, alpha: cobj, A: Ptr[complex64], lda: fint, beta: cobj, C: Ptr[complex64], ldc: fint)
```

function cblas\_ddot[](https://docs.exaloop.io/libraries/api/modules/numpy/linalg/blas/#function-cblas_ddot "Copy/link to this API element")

@C

```python
cblas_ddot(n: fint, x: cobj, incx: fint, y: cobj, incy: fint)
```

function cblas\_sdot[](https://docs.exaloop.io/libraries/api/modules/numpy/linalg/blas/#function-cblas_sdot "Copy/link to this API element")

@C

```python
cblas_sdot(n: fint, x: cobj, incx: fint, y: cobj, incy: fint)
```

function cblas\_zdotu\_sub[](https://docs.exaloop.io/libraries/api/modules/numpy/linalg/blas/#function-cblas_zdotu_sub "Copy/link to this API element")

@C

```python
cblas_zdotu_sub(n: fint, x: cobj, incx: fint, y: cobj, incy: fint, dotu: Ptr[complex])
```

function cblas\_cdotu\_sub[](https://docs.exaloop.io/libraries/api/modules/numpy/linalg/blas/#function-cblas_cdotu_sub "Copy/link to this API element")

@C

```python
cblas_cdotu_sub(n: fint, x: cobj, incx: fint, y: cobj, incy: fint, dotu: Ptr[complex64])
```

function cblas\_zdotc\_sub[](https://docs.exaloop.io/libraries/api/modules/numpy/linalg/blas/#function-cblas_zdotc_sub "Copy/link to this API element")

@C

```python
cblas_zdotc_sub(n: fint, x: cobj, incx: fint, y: cobj, incy: fint, dotc: cobj)
```

function cblas\_cdotc\_sub[](https://docs.exaloop.io/libraries/api/modules/numpy/linalg/blas/#function-cblas_cdotc_sub "Copy/link to this API element")

@C

```python
cblas_cdotc_sub(n: fint, x: cobj, incx: fint, y: cobj, incy: fint, dotc: cobj)
```

function cblas\_dgemv[](https://docs.exaloop.io/libraries/api/modules/numpy/linalg/blas/#function-cblas_dgemv "Copy/link to this API element")

@C

```python
cblas_dgemv(order: fint, trans: fint, M: fint, N: fint, alpha: float, A: cobj, lda: fint, x: cobj, incx: fint, beta: float, y: cobj, incy: fint)
```

function cblas\_sgemv[](https://docs.exaloop.io/libraries/api/modules/numpy/linalg/blas/#function-cblas_sgemv "Copy/link to this API element")

@C

```python
cblas_sgemv(order: fint, trans: fint, M: fint, N: fint, alpha: float32, A: cobj, lda: fint, x: cobj, incx: fint, beta: float32, y: cobj, incy: fint)
```

function cblas\_zgemv[](https://docs.exaloop.io/libraries/api/modules/numpy/linalg/blas/#function-cblas_zgemv "Copy/link to this API element")

@C

```python
cblas_zgemv(order: fint, trans: fint, M: fint, N: fint, alpha: cobj, A: cobj, lda: fint, x: cobj, incx: fint, beta: cobj, y: cobj, incy: fint)
```

function cblas\_cgemv[](https://docs.exaloop.io/libraries/api/modules/numpy/linalg/blas/#function-cblas_cgemv "Copy/link to this API element")

@C

```python
cblas_cgemv(order: fint, trans: fint, M: fint, N: fint, alpha: cobj, A: cobj, lda: fint, x: cobj, incx: fint, beta: cobj, y: cobj, incy: fint)
```

function dpotrf\_[](https://docs.exaloop.io/libraries/api/modules/numpy/linalg/blas/#function-dpotrf_ "Copy/link to this API element")

@C

```python
dpotrf_(uplo: cobj, n: fintp, a: cobj, lda: fintp, info: fintp)
```

function spotrf\_[](https://docs.exaloop.io/libraries/api/modules/numpy/linalg/blas/#function-spotrf_ "Copy/link to this API element")

@C

```python
spotrf_(uplo: cobj, n: fintp, a: cobj, lda: fintp, info: fintp)
```

function zpotrf\_[](https://docs.exaloop.io/libraries/api/modules/numpy/linalg/blas/#function-zpotrf_ "Copy/link to this API element")

@C

```python
zpotrf_(uplo: cobj, n: fintp, a: cobj, lda: fintp, info: fintp)
```

function cpotrf\_[](https://docs.exaloop.io/libraries/api/modules/numpy/linalg/blas/#function-cpotrf_ "Copy/link to this API element")

@C

```python
cpotrf_(uplo: cobj, n: fintp, a: cobj, lda: fintp, info: fintp)
```

function dgetrf\_[](https://docs.exaloop.io/libraries/api/modules/numpy/linalg/blas/#function-dgetrf_ "Copy/link to this API element")

@C

```python
dgetrf_(m: fintp, n: fintp, a: cobj, lda: fintp, ipiv: fintp, info: fintp)
```

function sgetrf\_[](https://docs.exaloop.io/libraries/api/modules/numpy/linalg/blas/#function-sgetrf_ "Copy/link to this API element")

@C

```python
sgetrf_(m: fintp, n: fintp, a: cobj, lda: fintp, ipiv: fintp, info: fintp)
```

function zgetrf\_[](https://docs.exaloop.io/libraries/api/modules/numpy/linalg/blas/#function-zgetrf_ "Copy/link to this API element")

@C

```python
zgetrf_(m: fintp, n: fintp, a: cobj, lda: fintp, ipiv: fintp, info: fintp)
```

function cgetrf\_[](https://docs.exaloop.io/libraries/api/modules/numpy/linalg/blas/#function-cgetrf_ "Copy/link to this API element")

@C

```python
cgetrf_(m: fintp, n: fintp, a: cobj, lda: fintp, ipiv: fintp, info: fintp)
```

function dgeqrf\_[](https://docs.exaloop.io/libraries/api/modules/numpy/linalg/blas/#function-dgeqrf_ "Copy/link to this API element")

@C

```python
dgeqrf_(m: fintp, n: fintp, a: cobj, lda: fintp, tau: cobj, work: cobj, lwork: fintp, info: fintp)
```

function sgeqrf\_[](https://docs.exaloop.io/libraries/api/modules/numpy/linalg/blas/#function-sgeqrf_ "Copy/link to this API element")

@C

```python
sgeqrf_(m: fintp, n: fintp, a: cobj, lda: fintp, tau: cobj, work: cobj, lwork: fintp, info: fintp)
```

function zgeqrf\_[](https://docs.exaloop.io/libraries/api/modules/numpy/linalg/blas/#function-zgeqrf_ "Copy/link to this API element")

@C

```python
zgeqrf_(m: fintp, n: fintp, a: cobj, lda: fintp, tau: cobj, work: cobj, lwork: fintp, info: fintp)
```

function cgeqrf\_[](https://docs.exaloop.io/libraries/api/modules/numpy/linalg/blas/#function-cgeqrf_ "Copy/link to this API element")

@C

```python
cgeqrf_(m: fintp, n: fintp, a: cobj, lda: fintp, tau: cobj, work: cobj, lwork: fintp, info: fintp)
```

function dorgqr\_[](https://docs.exaloop.io/libraries/api/modules/numpy/linalg/blas/#function-dorgqr_ "Copy/link to this API element")

@C

```python
dorgqr_(m: fintp, n: fintp, k: fintp, a: cobj, lda: fintp, tau: cobj, work: cobj, lwork: fintp, info: fintp)
```

function sorgqr\_[](https://docs.exaloop.io/libraries/api/modules/numpy/linalg/blas/#function-sorgqr_ "Copy/link to this API element")

@C

```python
sorgqr_(m: fintp, n: fintp, k: fintp, a: cobj, lda: fintp, tau: cobj, work: cobj, lwork: fintp, info: fintp)
```

function zungqr\_[](https://docs.exaloop.io/libraries/api/modules/numpy/linalg/blas/#function-zungqr_ "Copy/link to this API element")

@C

```python
zungqr_(m: fintp, n: fintp, k: fintp, a: cobj, lda: fintp, tau: cobj, work: cobj, lwork: fintp, info: fintp)
```

function cungqr\_[](https://docs.exaloop.io/libraries/api/modules/numpy/linalg/blas/#function-cungqr_ "Copy/link to this API element")

@C

```python
cungqr_(m: fintp, n: fintp, k: fintp, a: cobj, lda: fintp, tau: cobj, work: cobj, lwork: fintp, info: fintp)
```

function dgeev\_[](https://docs.exaloop.io/libraries/api/modules/numpy/linalg/blas/#function-dgeev_ "Copy/link to this API element")

@C

```python
dgeev_(jobvl: cobj, jobvr: cobj, n: fintp, a: cobj, lda: fintp, wr: cobj, wi: cobj, vl: cobj, ldvl: fintp, vr: cobj, ldvr: fintp, work: cobj, lwork: fintp, info: fintp)
```

function sgeev\_[](https://docs.exaloop.io/libraries/api/modules/numpy/linalg/blas/#function-sgeev_ "Copy/link to this API element")

@C

```python
sgeev_(jobvl: cobj, jobvr: cobj, n: fintp, a: cobj, lda: fintp, wr: cobj, wi: cobj, vl: cobj, ldvl: fintp, vr: cobj, ldvr: fintp, work: cobj, lwork: fintp, info: fintp)
```

function zgeev\_[](https://docs.exaloop.io/libraries/api/modules/numpy/linalg/blas/#function-zgeev_ "Copy/link to this API element")

@C

```python
zgeev_(jobvl: cobj, jobvr: cobj, n: fintp, a: cobj, lda: fintp, w: cobj, vl: cobj, ldvl: fintp, vr: cobj, ldvr: fintp, work: cobj, lwork: fintp, rwork: cobj, info: fintp)
```

function cgeev\_[](https://docs.exaloop.io/libraries/api/modules/numpy/linalg/blas/#function-cgeev_ "Copy/link to this API element")

@C

```python
cgeev_(jobvl: cobj, jobvr: cobj, n: fintp, a: cobj, lda: fintp, w: cobj, vl: cobj, ldvl: fintp, vr: cobj, ldvr: fintp, work: cobj, lwork: fintp, rwork: cobj, info: fintp)
```

function dsyevd\_[](https://docs.exaloop.io/libraries/api/modules/numpy/linalg/blas/#function-dsyevd_ "Copy/link to this API element")

@C

```python
dsyevd_(jobz: cobj, uplo: cobj, n: fintp, a: cobj, lda: fintp, w: cobj, work: cobj, lwork: fintp, iwork: fintp, liwork: fintp, info: fintp)
```

function ssyevd\_[](https://docs.exaloop.io/libraries/api/modules/numpy/linalg/blas/#function-ssyevd_ "Copy/link to this API element")

@C

```python
ssyevd_(jobz: cobj, uplo: cobj, n: fintp, a: cobj, lda: fintp, w: cobj, work: cobj, lwork: fintp, iwork: fintp, liwork: fintp, info: fintp)
```

function zheevd\_[](https://docs.exaloop.io/libraries/api/modules/numpy/linalg/blas/#function-zheevd_ "Copy/link to this API element")

@C

```python
zheevd_(jobz: cobj, uplo: cobj, n: fintp, a: cobj, lda: fintp, w: cobj, work: cobj, lwork: fintp, rwork: cobj, lrwork: fintp, iwork: fintp, liwork: fintp, info: fintp)
```

function cheevd\_[](https://docs.exaloop.io/libraries/api/modules/numpy/linalg/blas/#function-cheevd_ "Copy/link to this API element")

@C

```python
cheevd_(jobz: cobj, uplo: cobj, n: fintp, a: cobj, lda: fintp, w: cobj, work: cobj, lwork: fintp, rwork: cobj, lrwork: fintp, iwork: fintp, liwork: fintp, info: fintp)
```

function dgesv\_[](https://docs.exaloop.io/libraries/api/modules/numpy/linalg/blas/#function-dgesv_ "Copy/link to this API element")

@C

```python
dgesv_(n: fintp, nrhs: fintp, a: cobj, lda: fintp, ipiv: fintp, b: cobj, ldb: fintp, info: fintp)
```

function sgesv\_[](https://docs.exaloop.io/libraries/api/modules/numpy/linalg/blas/#function-sgesv_ "Copy/link to this API element")

@C

```python
sgesv_(n: fintp, nrhs: fintp, a: cobj, lda: fintp, ipiv: fintp, b: cobj, ldb: fintp, info: fintp)
```

function zgesv\_[](https://docs.exaloop.io/libraries/api/modules/numpy/linalg/blas/#function-zgesv_ "Copy/link to this API element")

@C

```python
zgesv_(n: fintp, nrhs: fintp, a: cobj, lda: fintp, ipiv: fintp, b: cobj, ldb: fintp, info: fintp)
```

function cgesv\_[](https://docs.exaloop.io/libraries/api/modules/numpy/linalg/blas/#function-cgesv_ "Copy/link to this API element")

@C

```python
cgesv_(n: fintp, nrhs: fintp, a: cobj, lda: fintp, ipiv: fintp, b: cobj, ldb: fintp, info: fintp)
```

function dgelsd\_[](https://docs.exaloop.io/libraries/api/modules/numpy/linalg/blas/#function-dgelsd_ "Copy/link to this API element")

@C

```python
dgelsd_(m: fintp, n: fintp, nrhs: fintp, a: cobj, lda: fintp, b: cobj, ldb: fintp, s: cobj, rcond: cobj, rank: fintp, work: cobj, lwork: fintp, iwork: fintp, info: fintp)
```

function sgelsd\_[](https://docs.exaloop.io/libraries/api/modules/numpy/linalg/blas/#function-sgelsd_ "Copy/link to this API element")

@C

```python
sgelsd_(m: fintp, n: fintp, nrhs: fintp, a: cobj, lda: fintp, b: cobj, ldb: fintp, s: cobj, rcond: cobj, rank: fintp, work: cobj, lwork: fintp, iwork: fintp, info: fintp)
```

function zgelsd\_[](https://docs.exaloop.io/libraries/api/modules/numpy/linalg/blas/#function-zgelsd_ "Copy/link to this API element")

@C

```python
zgelsd_(m: fintp, n: fintp, nrhs: fintp, a: cobj, lda: fintp, b: cobj, ldb: fintp, s: cobj, rcond: cobj, rank: fintp, work: cobj, lwork: fintp, rwork: cobj, lrwork: fintp, info: fintp)
```

function cgelsd\_[](https://docs.exaloop.io/libraries/api/modules/numpy/linalg/blas/#function-cgelsd_ "Copy/link to this API element")

@C

```python
cgelsd_(m: fintp, n: fintp, nrhs: fintp, a: cobj, lda: fintp, b: cobj, ldb: fintp, s: cobj, rcond: cobj, rank: fintp, work: cobj, lwork: fintp, rwork: cobj, lrwork: fintp, info: fintp)
```

function dgesdd\_[](https://docs.exaloop.io/libraries/api/modules/numpy/linalg/blas/#function-dgesdd_ "Copy/link to this API element")

@C

```python
dgesdd_(jobz: cobj, m: fintp, n: fintp, a: cobj, lda: fintp, s: cobj, u: cobj, ldu: fintp, vt: cobj, ldvt: fintp, work: cobj, lwork: fintp, iwork: fintp, info: fintp)
```

function sgesdd\_[](https://docs.exaloop.io/libraries/api/modules/numpy/linalg/blas/#function-sgesdd_ "Copy/link to this API element")

@C

```python
sgesdd_(jobz: cobj, m: fintp, n: fintp, a: cobj, lda: fintp, s: cobj, u: cobj, ldu: fintp, vt: cobj, ldvt: fintp, work: cobj, lwork: fintp, iwork: fintp, info: fintp)
```

function zgesdd\_[](https://docs.exaloop.io/libraries/api/modules/numpy/linalg/blas/#function-zgesdd_ "Copy/link to this API element")

@C

```python
zgesdd_(jobz: cobj, m: fintp, n: fintp, a: cobj, lda: fintp, s: cobj, u: cobj, ldu: fintp, vt: cobj, ldvt: fintp, work: cobj, lwork: fintp, rwork: cobj, iwork: fintp, info: fintp)
```

function cgesdd\_[](https://docs.exaloop.io/libraries/api/modules/numpy/linalg/blas/#function-cgesdd_ "Copy/link to this API element")

@C

```python
cgesdd_(jobz: cobj, m: fintp, n: fintp, a: cobj, lda: fintp, s: cobj, u: cobj, ldu: fintp, vt: cobj, ldvt: fintp, work: cobj, lwork: fintp, rwork: cobj, iwork: fintp, info: fintp)
```

Source: https://docs.exaloop.io/libraries/api/modules/numpy/linalg/blas/
