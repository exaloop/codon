import numpy as np
import numba as nb


# Numerically-stable version of softmax
@nb.jit(nopython=True, parallel=False, fastmath=True)
def softmax(x):
    new_shape = (x.shape[0], x.shape[1], x.shape[2], 1)
    # tmp_max = np.max(x, axis=-1, keepdims=True)
    tmp_max = np.empty(new_shape, dtype=x.dtype)
    for i in range(x.shape[3]):
        tmp_max[:, :, :, 0] = np.max(x[:, :, :, i])
    tmp_out = np.exp(x - tmp_max)
    # tmp_sum = np.sum(tmp_out, axis=-1, keepdims=True)
    tmp_sum = np.reshape(np.sum(tmp_out, axis=-1), new_shape)
    return tmp_out / tmp_sum
