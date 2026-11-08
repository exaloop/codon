import numpy as np


def kernel(A, b):

    x = np.zeros_like(b)
    y = np.zeros_like(b)

    for i in range(A.shape[0]):
        for j in range(i):
            A[i, j] -= A[i, :j] @ A[:j, j]
            A[i, j] /= A[j, j]
        for j in range(i, A.shape[0]):
            A[i, j] -= A[i, :i] @ A[:i, j]
    for i in range(A.shape[0]):
        y[i] = b[i] - A[i, :i] @ y[:i]
    for i in range(A.shape[0] - 1, -1, -1):
        x[i] = (y[i] - A[i, i + 1:] @ x[i + 1:]) / A[i, i]

    return x, y
