# https://numba.readthedocs.io/en/stable/user/5minguide.html

import numpy as np


def go_fast(a):
    trace = 0.0
    for i in range(a.shape[0]):
        trace += np.tanh(a[i, i])
    return a + trace
