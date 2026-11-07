import numpy as np
import numba as nb


@nb.jit(nopython=True, parallel=False, fastmath=True)
def kernel(A, p, r):

    return r @ A, A @ p
