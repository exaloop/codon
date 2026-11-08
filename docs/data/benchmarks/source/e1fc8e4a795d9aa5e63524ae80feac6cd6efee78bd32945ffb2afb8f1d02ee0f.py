# https://numba.readthedocs.io/en/stable/user/5minguide.html

import numpy as np
import numba as nb


@nb.jit(nopython=True, parallel=False, fastmath=False)
def go_fast(a):
    trace = 0.0
    for i in range(a.shape[0]):
        trace += np.tanh(a[i, i])
    return a + trace
