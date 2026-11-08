import numpy as np
import numba as nb


@nb.jit(nopython=True, parallel=False, fastmath=False)
def kernel(alpha, beta, C, A, B):

    C[:] = alpha * A @ B + beta * C
