import numpy as np


def kernel(alpha, beta, C, A, B):

    temp2 = np.empty((C.shape[1], ), dtype=C.dtype)
    C *= beta
    for i in range(C.shape[0]):
        for j in range(C.shape[1]):
            C[:i, j] += alpha * B[i, j] * A[i, :i]
            temp2[j] = B[:i, j] @ A[i, :i]
        C[i, :] += alpha * B[i, :] * A[i, i] + alpha * temp2
