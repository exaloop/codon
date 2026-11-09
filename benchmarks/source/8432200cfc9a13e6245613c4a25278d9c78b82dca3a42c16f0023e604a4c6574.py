import numpy as np


def kernel(alpha, beta, A, u1, v1, u2, v2, w, x, y, z):

    A += np.outer(u1, v1) + np.outer(u2, v2)
    x += beta * y @ A + z
    w += alpha * A @ x
