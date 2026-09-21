import numpy as np

from helpers import check_matmul_add_result, events, materialize, matmul_add, matmul_add_borrowed, matmul_add_reference, mutate_array, record

def check_matmul_special_dtype(dtype: type, huge_value, tiny_value):
    right = np.array([[1., 2.], [-1., 1.]], dtype=dtype)
    for value in (float('nan'), float('inf'), -float('inf'), 0., -0., tiny_value):
        left = np.array([[value, 1.], [2., value]], dtype=dtype)
        bias = np.array([[value, 3.], [4., value]], dtype=dtype)
        for mode in range(4):
            check_matmul_add_result(matmul_add(left, right, bias, mode),
                                    matmul_add_reference(left, right, bias, mode))
            check_matmul_add_result(matmul_add(left, right[:, 0], bias[:, 0], mode),
                                    matmul_add_reference(left, right[:, 0], bias[:, 0], mode))
        for coefficient in (0., -0., 0.75):
            coefficient = dtype(coefficient)
            product = materialize(left @ right)
            expected = materialize(coefficient * product) + bias
            check_matmul_add_result(coefficient * (left @ right) + bias, expected)
            expected = product + materialize(coefficient * bias)
            check_matmul_add_result(left @ right + coefficient * bias, expected)
    huge = np.array([[huge_value, huge_value], [-huge_value, -huge_value]], dtype=dtype)
    bias = -huge
    right = np.ones((2, 2), dtype=dtype)
    for mode in range(4):
        check_matmul_add_result(matmul_add(huge, right, bias, mode),
                                matmul_add_reference(huge, right, bias, mode))

@test
def test_matmul_add_special_values():
    check_matmul_special_dtype(np.float32, 3e38, 1e-40)
    check_matmul_special_dtype(np.float64, 1e308, 1e-310)

@test
def test_matmul_add_errors_and_effects():
    left = np.ones((3, 4))
    right = np.ones((4, 5))
    output = np.full((3, 5), -7.)
    for bad_right in (np.ones((2, 5)), np.ones((4, 2))):
        for mode in range(4):
            snapshot = output.copy()
            try:
                matmul_add_borrowed(left, bad_right, output, mode)
                assert False
            except ValueError:
                assert np.array_equal(output, snapshot)
    events.clear()
    actual = record(left, 1) @ record(right, 2) + record(output, 3)
    assert events == [1, 2, 3]
    check_matmul_add_result(actual, matmul_add_reference(left, right, output))
    events.clear()
    actual = record(output, 3) + record(left, 1) @ record(right, 2)
    assert events == [3, 1, 2]
    check_matmul_add_result(actual, matmul_add_reference(left, right, output, 1))
    events.clear()
    actual = record(left, 1) @ record(right, 2) - record(output, 3)
    assert events == [1, 2, 3]
    check_matmul_add_result(actual, matmul_add_reference(left, right, output, 2))
    events.clear()
    actual = record(output, 3) - record(left, 1) @ record(right, 2)
    assert events == [3, 1, 2]
    check_matmul_add_result(actual, matmul_add_reference(left, right, output, 3))
    events.clear()
    try:
        left @ np.ones((2, 5)) + record(output, 4)
        assert False
    except ValueError:
        assert events == []
    events.clear()
    try:
        left @ np.ones((2, 5)) - record(output, 4)
        assert False
    except ValueError:
        assert events == []
    events.clear()
    try:
        record(output, 4) + left @ np.ones((2, 5))
        assert False
    except ValueError:
        assert events == [4]
    events.clear()
    try:
        record(output, 4) - left @ np.ones((2, 5))
        assert False
    except ValueError:
        assert events == [4]
    values = np.ones((3, 3))
    expected = materialize(values @ values)
    actual = values @ values + mutate_array(values)
    check_matmul_add_result(actual, expected + values)
    values = np.ones((3, 3))
    expected = materialize(values @ values)
    actual = values @ values - mutate_array(values)
    check_matmul_add_result(actual, expected - values)
    values = np.ones((3, 3))
    actual = mutate_array(values) + values @ values
    check_matmul_add_result(actual, values + materialize(values @ values))
    values = np.ones((3, 3))
    actual = mutate_array(values) - values @ values
    check_matmul_add_result(actual, values - materialize(values @ values))

test_matmul_add_special_values()
test_matmul_add_errors_and_effects()
