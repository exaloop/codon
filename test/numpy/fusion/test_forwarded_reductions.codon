import numpy as np

from helpers import check_axis_result, events, materialize, record

@noinline
def forwarded_sum_epilogue(values, axis, keepdims: Literal[bool]):
    reduced = np.sum(values, axis=axis, keepdims=keepdims, initial=0.25)
    return reduced * 2

def check_forwarded_sum(values, axis, keepdims: Literal[bool]):
    expected = materialize(np.sum(values, axis=axis, keepdims=keepdims, initial=0.25)) * 2
    actual = forwarded_sum_epilogue(values, axis, keepdims)
    assert type(actual) is type(expected)
    check_axis_result(actual, expected)

@noinline
def forwarded_softmax(values):
    maximum = np.max(values, axis=-1, keepdims=True)
    exponentials = np.exp(values - maximum)
    total = np.sum(exponentials, axis=-1, keepdims=True)
    return exponentials / total

def check_forwarded_dtype(dtype: type):
    values = np.array([120, 125, 1, 2, 3, 4], dtype=dtype).reshape(2, 3)
    check_forwarded_sum(values, 1, False)
    check_forwarded_sum(values, 0, True)

@test
def test_forwarded_reduction_values():
    for size in (7, 8, 63, 64, 65, 127, 128, 129, 513):
        values = np.array([1e8, 1., -1e8, 3.] * (size + 1), dtype=np.float32)
        values = values[:3 * size].reshape(3, size)
        for array in (values, values.copy(order='F'), values[:, ::-1],
                      values[::2, ::2], values[:, :0]):
            for axis in range(2):
                check_forwarded_sum(array, axis, False)
                check_forwarded_sum(array, axis, True)
            check_forwarded_sum(array, (), False)
            check_forwarded_sum(array, (0, 1), True)
    check_forwarded_dtype(np.int8)
    check_forwarded_dtype(np.uint8)
    check_forwarded_dtype(np.float16)
    check_forwarded_dtype(np.float32)
    check_forwarded_dtype(np.complex64)
    check_forwarded_dtype(np.complex128)

    values = np.array([[16777216., 1.], [1., 1e-8]], dtype=np.float32)
    reduced = np.sum(values, axis=1, dtype=np.float64)
    actual = reduced.astype(np.float32) + np.float32(1)
    expected = materialize(np.sum(values, axis=1, dtype=np.float64)).astype(np.float32)
    expected = materialize(expected) + np.float32(1)
    check_axis_result(actual, expected)
    assert type(actual) is np.ndarray[np.float32, 1]
    reduced = np.sum(values, axis=1)
    actual = (reduced + np.float32(1)) - np.float32(16777216)
    expected = materialize(np.sum(values, axis=1)) + np.float32(1)
    expected = materialize(expected) - np.float32(16777216)
    check_axis_result(actual, expected)
    total = np.sum(values)
    check_axis_result(values / total, values / materialize(np.sum(values)))

    values = np.arange(2 * 3 * 4 * 129, dtype=np.float32).reshape(2, 3, 4, 129) * 0.001
    for array in (values, values.copy(order='F'), values[:, :, :, ::-1]):
        snapshot = array.copy()
        maximum = materialize(np.max(array, axis=-1, keepdims=True))
        centered = materialize(array - maximum)
        exponentials = materialize(np.exp(centered))
        total = materialize(np.sum(exponentials, axis=-1, keepdims=True))
        check_axis_result(forwarded_softmax(array), exponentials / total)
        assert np.array_equal(array, snapshot)

@noinline
def repeated_reduction_consumer(values):
    reduced = values.sum(axis=1)
    result = np.zeros(values.shape[0])
    for iteration in range(4):
        result = materialize(reduced * 2)
    return result

@test
def test_forwarded_reduction_lifetimes():
    values = np.array([[1., 2.], [3., 4.]])
    axis = 1
    initial = 0.25
    reduced = np.sum(values, axis=axis, initial=initial)
    axis = 0
    initial = 100.
    values[0, 0] = 100.
    assert np.array_equal(reduced * 2, [6.5, 14.5])
    assert axis == 0 and initial == 100.

    reduced = values.sum(axis=1)
    retained = reduced
    result = reduced * 2
    assert np.array_equal(retained, [102., 7.])
    assert np.array_equal(result, [204., 14.])
    assert result.data != retained.data
    check_axis_result(repeated_reduction_consumer(values), np.array([204., 14.]))

    reduced = values.sum(axis=())
    result = reduced * 2
    result[0, 0] = -1.
    assert values[0, 0] == 100.
    reduced = values.prod(axis=1)
    assert np.array_equal(reduced * 2, [400., 24.])
    reduced = values.min(axis=1)
    assert np.array_equal(-reduced, [-2., -3.])
    reduced = values.max(axis=1)
    assert np.array_equal(reduced + 1, [101., 5.])
    reduced = values.any(axis=1)
    assert np.array_equal(~reduced, [False, False])
    reduced = values.all(axis=1)
    assert np.array_equal(~reduced, [False, False])

    special = np.array([[0., -0.], [float('nan'), 1.]])
    reduced = special.min(axis=1)
    actual = -reduced
    expected = -materialize(special.min(axis=1))
    assert np.array_equal(np.isnan(actual), np.isnan(expected))
    assert np.array_equal(np.signbit(actual), np.signbit(expected))

    events.clear()
    reduced = np.sum(record(values, 1), axis=record(1, 2), initial=record(0.25, 3))
    result = reduced * record(2, 4)
    assert events == [1, 2, 3, 4]
    assert np.array_equal(result, [204.5, 14.5])
    events.clear()
    try:
        reduced = values[:, :0].min(axis=1)
        result = reduced * record(2, 5)
        assert False
    except ValueError:
        assert events == []

test_forwarded_reduction_values()
test_forwarded_reduction_lifetimes()
