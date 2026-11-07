import numpy as np
import numba as nb


@nb.jit(nopython=True, parallel=False, fastmath=True)
def kernel(NR, NQ, NP, A, C4):

    for r in range(NR):
        for q in range(NQ):
            tmp = A[r, q, :] @ C4
            A[r, q, :] = tmp
    # A[:] = np.reshape(np.reshape(A, (NR, NQ, 1, NP)) @ C4, (NR, NQ, NP))
