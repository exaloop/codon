import numpy as np

from helpers import events, materialize, mutate_array, record

def check_mean_where_result(actual, expected):
    assert np.allclose(actual, expected, rtol=0, atol=0, equal_nan=True), (actual, expected)
    if hasattr(actual, 'shape'):
        assert actual.shape == expected.shape
        assert actual.strides == expected.strides, (actual.strides, expected.strides)
        assert actual.dtype is expected.dtype
    else:
        assert type(actual) is type(expected)

def check_fused_mean_dtype(dtype: type):
    values = np.arange(24, dtype=dtype).reshape(4, 6)
    for array in (values, values.copy(order='F'), values.T, values[::-1, ::-2],
                  values[:0], values[:, :0]):
        check_mean_where_result(np.mean(array + dtype(1)),
                                np.mean(materialize(array + dtype(1))))
        for axis in (0, 1, -1):
            check_mean_where_result((array + dtype(1)).mean(axis=axis),
                                    materialize(array + dtype(1)).mean(axis=axis))
            check_mean_where_result(np.mean(array + dtype(1), axis=axis, keepdims=True),
                                    np.mean(materialize(array + dtype(1)),
                                            axis=axis, keepdims=True))
        check_mean_where_result((array + dtype(1)).mean(axis=(), keepdims=True),
                                materialize(array + dtype(1)).mean(axis=(), keepdims=True))
        check_mean_where_result(np.mean(array + dtype(1), axis=(1, 0), dtype=np.complex128),
                                np.mean(materialize(array + dtype(1)),
                                        axis=(1, 0), dtype=np.complex128))
    for size in (0, 1, 17, 257):
        array = np.arange(size, dtype=dtype)[::-1]
        check_mean_where_result((array + dtype(1)).mean(),
                                materialize(array + dtype(1)).mean())

@test
def test_fused_mean():
    check_fused_mean_dtype(np.int8)
    check_fused_mean_dtype(np.uint8)
    check_fused_mean_dtype(np.float16)
    check_fused_mean_dtype(np.float32)
    check_fused_mean_dtype(np.float64)
    check_fused_mean_dtype(np.complex64)
    check_fused_mean_dtype(np.complex128)
    values = np.array([1e16, 1., -1e16, -1.] * 258).reshape(2, 3, 172)
    for array in (values, values.copy(order='F'), values[:, ::-1, ::-2]):
        check_mean_where_result((array + 0.).mean(), materialize(array + 0.).mean())
        check_mean_where_result(np.mean(array + 0., axis=(0, 2), keepdims=True),
                                np.mean(materialize(array + 0.), axis=(0, 2), keepdims=True))
        check_mean_where_result(np.mean(array + 0., dtype=np.float32, axis=1),
                                np.mean(materialize(array + 0.), dtype=np.float32, axis=1))
    values = np.array([[1.25, 2.75], [3.5, 4.5]])
    for axis in (0, 1):
        check_mean_where_result(np.mean(values + 0., axis=axis, dtype=np.int32),
                                np.mean(materialize(values + 0.), axis=axis, dtype=np.int32))
    check_mean_where_result(np.mean(values > 2), np.mean(materialize(values > 2)))
    check_mean_where_result(np.mean(values + 1, where=values > 2),
                            np.mean(materialize(values + 1), where=values > 2))
    output = np.zeros(2)
    returned = np.mean(values + 1, axis=0, out=output)
    assert returned.data == output.data
    check_mean_where_result(output, np.mean(materialize(values + 1), axis=0))
    for axis in (-3, 2):
        try:
            np.mean(values + 1, axis=axis)
            assert False
        except np.AxisError:
            pass
    try:
        np.mean(values + 1, axis=(0, 0))
        assert False
    except ValueError:
        pass
    special = np.array([0., -0., float('nan'), float('inf'), -float('inf')])
    check_mean_where_result(np.mean(special * 1), np.mean(materialize(special * 1)))

def check_fused_where_dtype(dtype: type):
    values = np.arange(24, dtype=dtype).reshape(4, 6)
    for array in (values, values.copy(order='F'), values.T, values[::-1, ::-2],
                  values[:0], values[:, :0]):
        condition = array.real > 3
        snapshot = array.copy()
        expected = np.where(condition, materialize(array * dtype(2)),
                            materialize(array + dtype(1)))
        check_mean_where_result(np.where(condition, array * dtype(2), array + dtype(1)), expected)
        check_mean_where_result(np.where(condition, array * dtype(2), array + dtype(1)) + dtype(3),
                                materialize(expected) + dtype(3))
        check_mean_where_result(np.mean(np.where(condition, array * dtype(2), array + dtype(1))),
                                np.mean(materialize(expected)))
        check_mean_where_result(np.where(condition, array * dtype(2), 3.),
                                np.where(condition, materialize(array * dtype(2)), 3.))
        check_mean_where_result(np.where(True, array + dtype(1), array),
                                np.where(True, materialize(array + dtype(1)), array))
        assert np.array_equal(array, snapshot)

@test
def test_fused_where():
    check_fused_where_dtype(np.int8)
    check_fused_where_dtype(np.float32)
    check_fused_where_dtype(np.float64)
    check_fused_where_dtype(np.complex64)
    left = np.arange(3.).reshape(3, 1)
    right = np.arange(4.).reshape(1, 4)
    condition = np.array([[0., float('nan'), 1., -2.]])
    expected = np.where(condition, materialize(left * 2), materialize(right + 1))
    check_mean_where_result(np.where(condition, left * 2, right + 1), expected)
    check_mean_where_result(np.where(condition + 0., np.array(3), np.array(4.)),
                            np.where(materialize(condition + 0.), np.array(3), np.array(4.)))
    check_mean_where_result(np.where(condition > 0, left + 1, np.where(right > 2, right, -right)),
                            np.where(materialize(condition > 0), materialize(left + 1),
                                     materialize(np.where(right > 2, right, -right))))
    check_mean_where_result(np.where(condition, np.sin(left) + np.exp(left), right * right),
                            np.where(condition, materialize(np.sin(left) + np.exp(left)),
                                     materialize(right * right)))
    special = np.array([0., -0., float('nan'), float('inf')])
    selected = np.where(special == 0, special * 1., special)
    reference = np.where(materialize(special == 0), materialize(special * 1.), special)
    assert np.array_equal(np.signbit(selected), np.signbit(reference))
    check_mean_where_result(selected, reference)
    assert np.array_equal(np.where(special == 0)[0], [0, 1])
    events.clear()
    result = np.where(record(special == 0, 1), record(special, 2), record(special, 3)) + 1
    assert events == [1, 2, 3]
    check_mean_where_result(result, special + 1)
    values = np.array([1., 2.])
    result = np.where(values < 2, values + 1, mutate_array(values))
    assert np.array_equal(result, [2., 2.])
    for condition in (False, True):
        try:
            np.where(condition, np.ones(2) + 1, np.ones(3) * 2)
            assert False
        except ValueError:
            pass
        events.clear()
        try:
            np.where(condition, np.ones(2) + np.ones(3), record(np.ones(2), 1))
            assert False
        except ValueError:
            assert events == []

test_fused_mean()
test_fused_where()
