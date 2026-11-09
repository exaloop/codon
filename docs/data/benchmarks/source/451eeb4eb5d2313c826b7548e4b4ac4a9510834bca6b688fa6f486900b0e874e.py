import numpy as np


def kernel(alpha, beta, A, B, C, D):
    D[:] = alpha * A @ B @ C + beta * D
