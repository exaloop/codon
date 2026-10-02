import numpy as np

from helpers import effectful_none, events, materialize, mutate_array, record

@noinline
def into_slice(values, increment, order):
    output = np.empty_like(values, order=order)
    output[:] = values * values + increment
    return output

@noinline
def into_ufunc(values, increment, order):
    output = np.empty_like(values, order=order)
    result = np.add(values * values, increment, out=output)
    assert result.data == output.data
    assert result.shape == output.shape and result.strides == output.strides
    return output

def check_destination_dtype(dtype: type):
    values = np.array([127, -128, 33, -17, 1, 0], dtype=dtype).reshape(2, 3)
    increment = dtype(3)
    for array in (values, values.copy(order='F'), values[:, ::-1],
                  values[:, ::2], values[:, :0], values[:0, :]):
        snapshot = array.copy()
        expected = materialize(array * array) + increment
        for order in ('C', 'F'):
            assigned = into_slice(array, increment, order)
            returned = into_ufunc(array, increment, order)
            layout = np.empty_like(array, order=order)
            assert np.array_equal(assigned, expected)
            assert np.array_equal(returned, expected)
            assert assigned.strides == layout.strides
            assert returned.strides == layout.strides
            assert np.array_equal(array, snapshot)

@noinline
def into_broadcast(left, right, shape):
    output = np.empty(shape)
    output[:] = left * left + right
    return output

@noinline
def into_self():
    output = np.full((2, 3), 2.0)
    output[:] = output * output + 1.0
    return output

@noinline
def into_ufunc_self():
    output = np.full((2, 3), 2.0)
    return np.add(output * output, 1.0, out=output)

@noinline
def into_simple_ufunc(values, increment):
    output = np.empty_like(values)
    return np.add(values, increment, out=output)

@test
def test_destination_values():
    check_destination_dtype(np.float64)
    check_destination_dtype(np.float32)
    check_destination_dtype(np.float16)
    check_destination_dtype(np.int64)
    check_destination_dtype(np.int8)
    values = np.array([1.0000001192092896, 4097., 8193., -0.], dtype=np.float32)
    expected = materialize(values * values) + np.float32(-16785408)
    assert np.array_equal(into_slice(values, np.float32(-16785408), 'C'), expected)
    assert np.array_equal(into_ufunc(values, np.float32(-16785408), 'F'), expected)
    left = np.array([[1.], [2.]])
    right = np.array([[10., 20., 30.]])
    assert np.array_equal(into_broadcast(left, right, (2, 3)), [[11., 21., 31.], [14., 24., 34.]])
    assert np.array_equal(into_broadcast(left, right, (4, 2, 3)),
                          np.broadcast_to(materialize(left * left) + right, (4, 2, 3)))
    assert np.array_equal(into_self(), np.full((2, 3), 5.0))
    assert np.array_equal(into_ufunc_self(), np.full((2, 3), 5.0))
    flags = np.array([True, False])
    assert np.array_equal(into_simple_ufunc(flags, flags), flags)

@noinline
def into_unknown(output, values):
    output[:] = values * values + 1.0
    return output

@noinline
def into_unknown_ufunc(output, values):
    return np.add(values * values, 1.0, out=output)

@noinline
def into_retained_view():
    output = np.ones(6)
    reverse = output[::-1]
    reverse[:] = np.arange(6, dtype=float)
    output[:] = reverse * reverse + 1.0
    return output

@noinline
def recorded_slice():
    events.append(8)
    return slice(None, None, None)

@noinline
def into_recorded_slice(values):
    output = np.empty_like(values)
    output[recorded_slice()] = values * values + 1.0
    return output

@noinline
def recorded_add():
    events.append(9)
    return np.add

@noinline
def into_recorded_add(values):
    output = np.empty_like(values)
    return recorded_add()(values * values, 1.0, out=output)

@noinline
def into_effectful_none(values):
    output = np.empty_like(values)
    output[:] = np.add(values * values, 1.0, out=effectful_none())
    return output

@test
def test_destination_fallbacks():
    values = np.arange(6, dtype=float)
    expected = materialize(values[::-1] * values[::-1]) + 1.0
    actual = into_unknown(values, values[::-1])
    assert actual.data == values.data and np.array_equal(actual, expected)
    values = np.arange(6, dtype=float)
    expected = materialize(values[:-1] * values[:-1]) + 1.0
    actual = into_unknown_ufunc(values[1:], values[:-1])
    assert np.array_equal(actual, expected)
    assert np.array_equal(into_retained_view(), np.arange(6, dtype=float) ** 2 + 1.0)
    values = np.arange(6, dtype=float)
    values[1:] = values[:-1] * values[:-1] + 1.0
    assert np.array_equal(values, [0., 1., 2., 5., 10., 17.])

    values = np.array([1.25, 2.75])
    output = np.empty(2, dtype=np.int64)
    result = np.add(values * values, 0.25, out=output)
    assert result.data == output.data and np.array_equal(output, [1, 7])
    output = np.full(2, -1.)
    np.add(values * values, 1., out=output, where=False)
    assert np.array_equal(output, [-1., -1.])
    np.add(values * values, 1., out=output, where=np.array([True, False]))
    assert np.array_equal(output, [2.5625, -1.])
    output = np.empty(2)
    np.add(values * values, 1., out=output, dtype=np.float32)
    assert np.array_equal(output, [2.5625, 8.5625])

    events.clear()
    output = np.empty(2)
    np.add(record(values, 1) * record(values, 2), record(1., 3),
           out=record(output, 4))
    assert events == [1, 2, 3, 4]
    assert np.array_equal(output, [2.5625, 8.5625])
    output = np.ones(2)
    output[:] = output * output + mutate_array(output)
    assert np.array_equal(output, [101., 2.])
    events.clear()
    assert np.array_equal(into_recorded_slice(values), [2.5625, 8.5625])
    assert events == [8]
    events.clear()
    assert np.array_equal(into_recorded_add(values), [2.5625, 8.5625])
    assert events == [9]
    events.clear()
    assert np.array_equal(into_effectful_none(values), [2.5625, 8.5625])
    assert events == [7]
    output = np.empty(2)
    result = np.add(values * values, 1., out=output) * 2.
    assert np.array_equal(output, [2.5625, 8.5625])
    assert np.array_equal(result, [5.125, 17.125])

@noinline
def into_bad_shape(values, shape):
    output = np.full(shape, -7.)
    try:
        output[:] = values * values + 1.0
    except ValueError:
        return output
    assert False
    return output

def destination_add(output, left, right, extra):
    output[0] = left[0] + right[0]

@test
def test_destination_errors():
    from numpy.fusion import _loop_into
    values = np.arange(6, dtype=float).reshape(2, 3)
    assert np.array_equal(into_bad_shape(values, (2, 2)), np.full((2, 2), -7.))
    output = np.full((2, 3), -7.)
    try:
        _loop_into((values, np.ones((2, 2))), destination_add, (), output)
        assert False
    except ValueError:
        assert np.array_equal(output, np.full((2, 3), -7.))
    events.clear()
    output = np.full((2, 3), -7.)
    try:
        np.add(values * np.ones((2, 2)), record(1., 1), out=record(output, 2))
        assert False
    except ValueError:
        assert events == []
        assert np.array_equal(output, np.full((2, 3), -7.))

test_destination_values()
test_destination_fallbacks()
test_destination_errors()
