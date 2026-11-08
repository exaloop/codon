import numpy as np
import numba as nb


@nb.jit(nopython=True, parallel=False, fastmath=False)
def kernel(A, B, C, D):

    return A @ B @ C @ D
