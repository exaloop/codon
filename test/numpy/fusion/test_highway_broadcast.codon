import numpy as np

def check_highway_broadcast(dtype: type):
    from numpy.fusion import _apply_vectorized_loop_binary
    from math import hypot, atan2

    values = (np.arange(7 * 37, dtype=dtype).reshape(7, 37) + dtype(1)) * dtype(0.01)
    for array in (values, values.copy(order='F'), values[:, ::-1]):
        for small in (array[:, :1], array[:1, :]):
            for left, right in ((small, array), (array, small)):
                output = np.empty_like(array)
                _apply_vectorized_loop_binary(left, right, output, 'hypot')
                expanded_left = np.broadcast_to(left, array.shape)
                expanded_right = np.broadcast_to(right, array.shape)
                for row in range(array.shape[0]):
                    for column in range(array.shape[1]):
                        expected = hypot(float(expanded_left[row, column]), float(expanded_right[row, column]))
                        assert abs(float(output[row, column]) - expected) < 1e-5
                _apply_vectorized_loop_binary(left, right, output, 'arctan2')
                for row in range(array.shape[0]):
                    for column in range(array.shape[1]):
                        expected = atan2(float(expanded_left[row, column]), float(expanded_right[row, column]))
                        assert abs(float(output[row, column]) - expected) < 1e-5

@test
def test_highway_broadcast():
    check_highway_broadcast(np.float32)
    check_highway_broadcast(np.float64)

test_highway_broadcast()
