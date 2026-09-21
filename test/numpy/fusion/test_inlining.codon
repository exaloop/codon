import numpy as np

from helpers import events, materialize, mutate_array, mutate_initial, record

def square(array):
    return array * array

def reversed_arguments(first, second, unused):
    return second - first * first

def scalar_first(array, scalar):
    return scalar - array

def staged_square(array):
    squared = array * array
    shifted = squared + 1
    return shifted

def staged_rebind(array):
    array = array + 1
    result = array * 2
    return result

def staged_arguments(first, second, unused):
    squared = first * first
    result = second - squared
    return result

def staged_mean(array):
    squared = array * array
    result = np.mean(squared + 1)
    return result

def staged_where(array):
    condition = array > 2
    selected = np.where(condition, array * 2, array + 1)
    return selected

def staged_nested(array):
    return staged_square(array) + 2

def nested_staged(array):
    temporary = staged_nested(array)
    return temporary + 3

def nested_deep(array):
    return nested_staged(array) + 4

def nested_diamond(array):
    first = staged_square(array)
    second = staged_square(array)
    return second + first

def nested_arguments(first, second, unused):
    return staged_arguments(first, second, unused) + 1

def nested_effectful_arguments(array):
    return staged_arguments(record(array, 1), record(array * 2, 2), record(array, 3))

def nested_raising_argument(array):
    return staged_arguments(record(array, 1), record(array, 2), fail_argument())

def nested_shared_input(array):
    temporary = record(array + 1, 5)
    return staged_square(temporary) + 1

def nested_reduction(array):
    return array - staged_mean(array)

def staged_shared(array):
    temporary = record(array + 1, 5)
    return temporary * temporary + temporary

def staged_uncovered(array, other):
    unused = array + other
    return array + 1

def staged_mutation(array):
    temporary = array + 1
    mutate_array(array)
    return temporary * 2

def staged_reduction(array):
    temporary = array + 1
    total = temporary.mean()
    return temporary - total

def mutate_none(array):
    array[0] = 100

def staged_ufunc_effect(array, other):
    temporary = np.add(array, array, out=mutate_none(other))
    return other * 2 + temporary

def fail_argument():
    events.append(9)
    raise ValueError('argument')
    return np.arange(2)

def mutate_index(array):
    array[0] = 100
    return 0

@noinline
def indexed_callback(array, callback):
    return (array + 1) + array[callback(array):]

@noinline
def effectful_ufunc_receiver():
    events.append(8)
    return np.add

def receiver_expression(array):
    return effectful_ufunc_receiver()(array, array)

@test
def test_inline_arguments():
    array = np.arange(6)
    events.clear()
    result = record(10, 0) + reversed_arguments(
        record(array, 1), record(array * 2, 2), record(array, 3))
    assert np.array_equal(result, 10 + array * 2 - array * array)
    assert events == [0, 1, 2, 3]
    events.clear()
    assert np.array_equal(square(record(array, 4)) + 1, array * array + 1)
    assert events == [4]
    assert np.array_equal(scalar_first(array, 8) + 1, 9 - array)
    events.clear()
    try:
        reversed_arguments(record(array, 1), record(array, 2), fail_argument()) + 1
        assert False
    except ValueError:
        assert events == [1, 2, 9]
    mutable = np.array([1, 2])
    assert np.array_equal(square(mutable) + mutate_array(mutable), [101, 6])
    events.clear()
    result = receiver_expression(record(array, 6))
    assert np.array_equal(result, array + array)
    assert events == [6, 8]
    events.clear()
    try:
        receiver_expression(fail_argument())
        assert False
    except ValueError:
        assert events == [9]

def check_staged_layout(dtype: type):
    values = np.arange(24, dtype=dtype).reshape(4, 6)
    for array in (values, values.copy(order='F'), values.T, values[::-1, ::-2],
                  values[:0], values[:, :0]):
        expected = materialize(materialize(array * array) + 1) + 2
        result = staged_square(array) + 2
        assert np.array_equal(result, expected)
        assert result.dtype is expected.dtype
        assert result.strides == expected.strides
        assert np.array_equal(staged_nested(array), expected)
        deep_expected = materialize(expected + 3) + 4
        deep_result = nested_deep(array)
        assert np.array_equal(deep_result, deep_expected)
        assert deep_result.dtype is deep_expected.dtype
        assert deep_result.strides == deep_expected.strides
        assert np.array_equal(nested_diamond(array), materialize(array * array + 1) * 2)
        assert np.array_equal(staged_square(array + 1),
                              materialize((array + 1) * (array + 1)) + 1)
        assert np.array_equal(staged_rebind(array) + 2, materialize((array + 1) * 2) + 2)
        assert np.allclose(staged_mean(array),
                           np.mean(materialize(materialize(array * array) + 1)),
                           rtol=0, atol=0, equal_nan=True)
        assert np.array_equal(staged_where(array) + 1,
                              materialize(np.where(array > 2, array * 2, array + 1)) + 1)

@test
def test_multistatement_helpers():
    check_staged_layout(np.int64)
    check_staged_layout(np.float32)
    check_staged_layout(np.float64)
    array = np.arange(6.)
    events.clear()
    result = record(10., 0) + staged_arguments(
        record(array, 1), record(array * 2, 2), record(array, 3))
    assert events == [0, 1, 2, 3]
    assert np.array_equal(result, 10 + array * 2 - array * array)
    events.clear()
    try:
        staged_arguments(record(array, 1), record(array, 2), fail_argument())
        assert False
    except ValueError:
        assert events == [1, 2, 9]
    events.clear()
    assert np.array_equal(staged_shared(array), (array + 1) * (array + 1) + array + 1)
    assert events == [5]
    for other in (np.ones(2), np.ones(3)):
        try:
            staged_uncovered(array, other) + 1
            assert False
        except ValueError:
            pass
    original = np.array([1., 2.])
    rebound = staged_rebind(original)
    assert np.array_equal(original, [1., 2.])
    assert np.array_equal(rebound, [4., 6.])
    assert np.array_equal(staged_mutation(original), [4., 6.])
    assert np.array_equal(original, [100., 2.])
    original = np.array([1., 2.])
    result = staged_square(original) + mutate_array(original)
    assert np.array_equal(result, [102., 7.])
    result = staged_reduction(array)
    expected = materialize(array + 1)
    assert np.array_equal(result, expected - expected.mean())
    other = np.array([1., 2.])
    assert np.array_equal(staged_ufunc_effect(np.array([1., 2.]), other), [202., 8.])
    assert np.array_equal(other, [100., 2.])

@test
def test_nested_helper_arguments():
    array = np.arange(6.)
    events.clear()
    result = nested_arguments(record(array, 1), record(array * 2, 2), record(array, 3))
    assert events == [1, 2, 3]
    assert np.array_equal(result, array * 2 - array * array + 1)
    events.clear()
    result = nested_effectful_arguments(array)
    assert events == [1, 2, 3]
    assert np.array_equal(result, array * 2 - array * array)
    events.clear()
    try:
        nested_arguments(record(array, 1), record(array, 2), fail_argument())
        assert False
    except ValueError:
        assert events == [1, 2, 9]
    events.clear()
    try:
        nested_raising_argument(array)
        assert False
    except ValueError:
        assert events == [1, 2, 9]
    events.clear()
    assert np.array_equal(nested_shared_input(array), (array + 1) * (array + 1) + 2)
    assert events == [5]
    assert np.array_equal(nested_reduction(array), array - np.mean(array * array + 1))
    mutable = np.array([1., 2.])
    result = nested_staged(mutable) + mutate_array(mutable)
    assert np.array_equal(result, [107., 12.])

@test
def test_safe_fusion_leaves():
    matrix = np.arange(12).reshape(3, 4)
    result = (matrix[1:, :-1] + matrix[:-1, 1:]) * matrix[1:, 1:]
    expected = materialize(matrix[1:, :-1] + matrix[:-1, 1:]) * matrix[1:, 1:]
    assert np.array_equal(result, expected)
    result = (np.clip(matrix, 2, 10) * 3 + matrix) * 2
    assert np.array_equal(result, (materialize(np.clip(matrix, 2, 10)) * 3 + matrix) * 2)
    mutable = np.array([1, 2, 3])
    result = (mutable + 1) + mutable[mutate_index(mutable):]
    assert np.array_equal(result, [102, 5, 7])
    mutable = np.array([1, 2, 3])
    result = (mutable + 1) + np.clip(mutable, mutate_initial(mutable), 10)
    assert np.array_equal(result, [12, 5, 7])
    mutable = np.array([1, 2, 3])
    result = (mutable + 1) + np.clip(mutable, 0, 2, out=mutable)
    assert np.array_equal(result, [3, 5, 6])
    mutable = np.array([1, 2, 3])
    assert np.array_equal(indexed_callback(mutable, mutate_index), [102, 5, 7])
    mutable = np.array([1, 2, 3])
    result = (mutable + 1) / (2 * mutate_initial(mutable))
    assert np.array_equal(result, [0.5, 0.75, 1.])
    dx, dy = 0.5, 2.
    result = (matrix[1:, :-1] / (2 * dx) + matrix[:-1, 1:] * (1 / dy)) / (2 * (dx ** 2 + dy ** 2))
    left = materialize(matrix[1:, :-1] / (2 * dx))
    right = materialize(matrix[:-1, 1:] * (1 / dy))
    assert np.array_equal(result, (left + right) / (2 * (dx ** 2 + dy ** 2)))

test_multistatement_helpers()
test_nested_helper_arguments()
test_safe_fusion_leaves()
test_inline_arguments()
