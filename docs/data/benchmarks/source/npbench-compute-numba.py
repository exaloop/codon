# https://cython.readthedocs.io/en/latest/src/userguide/numpy_tutorial.html

import numpy as np
import numba as nb


@nb.jit(nopython=True, parallel=False, fastmath=True)
def compute(array_1, array_2, a, b, c):
    # return np.clip(array_1, 2, 10) * a + array_2 * b + c
    return np.minimum(np.maximum(array_1, 2), 10) * a + array_2 * b + c
