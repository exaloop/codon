import numpy as np
import numba as nb


@nb.jit(nopython=True, parallel=False, fastmath=True)
def mgrid(xn, yn):
    Xi = np.empty((xn, yn), dtype=np.uint32)
    Yi = np.empty((xn, yn), dtype=np.uint32)
    for i in range(xn):
        Xi[i, :] = i
    for j in range(yn):
        Yi[:, j] = j
    return Xi, Yi


@nb.jit(nopython=True, parallel=False, fastmath=True)
def stockham_fft(N, R, K, x, y):

    # Generate DFT matrix for radix R.
    # Define transient variable for matrix.
    i_coord, j_coord = mgrid(R, R)
    dft_mat = np.empty((R, R), dtype=np.complex128)
    dft_mat = np.exp(-2.0j * np.pi * i_coord * j_coord / R)
    # Move input x to output y
    # to avoid overwriting the input.
    y[:] = x[:]

    # ii_coord, jj_coord = np.mgrid[0:R, 0:R**K]
    ii_coord, jj_coord = mgrid(R, R**K)

    # Main Stockham loop
    for i in range(K):

        # Stride permutation
        yv = np.reshape(y, (R**i, R, R**(K - i - 1)))
        # tmp_perm = np.transpose(yv, axes=(1, 0, 2))
        tmp_perm = np.transpose(yv, axes=(1, 0, 2)).copy()
        # Twiddle Factor multiplication
        D = np.empty((R, R**i, R**(K - i - 1)), dtype=np.complex128)
        tmp = np.exp(-2.0j * np.pi * ii_coord[:, :R**i] * jj_coord[:, :R**i] /
                     R**(i + 1))
        # D[:] = np.repeat(np.reshape(tmp, (R, R**i, 1)), R ** (K-i-1), axis=2)
        for k in range(R**(K - i - 1)):
            D[:, :, k] = tmp
        tmp_twid = np.reshape(tmp_perm, (N, )) * np.reshape(D, (N, ))
        # Product with Butterfly
        y[:] = np.reshape(dft_mat @ np.reshape(tmp_twid, (R, R**(K - 1))),
                          (N, ))
