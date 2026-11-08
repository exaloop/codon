import numpy as np
import numba as nb


@nb.jit(nopython=True, parallel=False, fastmath=False)
def relu(x):
    return np.maximum(x, 0)


# Numerically-stable version of softmax
@nb.jit(nopython=True, parallel=False, fastmath=False)
def softmax(x):
    new_shape = (x.shape[0], 1)
    # tmp_max = np.max(x, axis=-1, keepdims=True)
    tmp_max = np.empty(new_shape, dtype=x.dtype)
    for i in range(x.shape[0]):
        tmp_max[i, 0] = np.max(x[i, :])
    tmp_out = np.exp(x - tmp_max)
    # tmp_sum = np.sum(tmp_out, axis=-1, keepdims=True)
    tmp_sum = np.reshape(np.sum(tmp_out, axis=-1), new_shape)
    return tmp_out / tmp_sum


# 3-layer MLP
@nb.jit(nopython=True, parallel=False, fastmath=False)
def mlp(input, w1, b1, w2, b2, w3, b3):
    x = relu(input @ w1 + b1)
    x = relu(x @ w2 + b2)
    x = softmax(x @ w3 + b3)  # Softmax call can be omitted if necessary
    return x
