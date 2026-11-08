import numpy as np


def kernel(A):
    A[:] = np.linalg.cholesky(A) + np.triu(A, k=1)
