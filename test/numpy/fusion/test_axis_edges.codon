import numpy as np

from helpers import check_axis_result, materialize

def check_axis_dtype(dtype: type):
    array = np.array([120, 125, 1, 2, 3, 4], dtype=dtype).reshape(2, 3)
    for axis in range(2):
        check_axis_result((array + dtype(10)).sum(axis=axis),
                          materialize(array + dtype(10)).sum(axis=axis))
        check_axis_result((array + dtype(10)).prod(axis=axis),
                          materialize(array + dtype(10)).prod(axis=axis))
        check_axis_result((array + dtype(10)).min(axis=axis),
                          materialize(array + dtype(10)).min(axis=axis))
        check_axis_result((array + dtype(10)).max(axis=axis),
                          materialize(array + dtype(10)).max(axis=axis))
        check_axis_result(np.sum(array + dtype(10), axis=axis, dtype=np.complex128),
                          np.sum(materialize(array + dtype(10)), axis=axis,
                                 dtype=np.complex128))

@test
def test_axis_reduction_edges():
    check_axis_dtype(np.int8)
    check_axis_dtype(np.uint8)
    check_axis_dtype(np.float16)
    check_axis_dtype(np.float32)
    check_axis_dtype(np.complex64)
    check_axis_dtype(np.complex128)
    values = np.arange(24.).reshape(4, 6)
    for axis in (-3, 2):
        try:
            (values + 1).sum(axis=axis)
            assert False
        except np.AxisError:
            pass
    for axes in ((0, 0), (1, -1)):
        try:
            (values < 1).any(axis=axes)
            assert False
        except ValueError as error:
            assert 'repeated axis' in str(error)
    for empty in (values[:0], values[:, :0]):
        for axis in range(2):
            try:
                (empty + 1).min(axis=axis)
                assert False
            except ValueError as error:
                assert 'zero-size array' in str(error)
            try:
                (empty + 1).max(axis=axis, keepdims=True)
                assert False
            except ValueError as error:
                assert 'zero-size array' in str(error)
    special = np.array([[0., -0.], [float('nan'), float('inf')]])
    assert np.array_equal(np.isnan((special * 1).min(axis=1)), [False, True])
    assert np.array_equal(np.isnan((special * 1).max(axis=1)), [False, True])
    assert np.signbit((special * 1).min(axis=1)[0])
    assert not np.signbit((special * 1).max(axis=1)[0])
    check_axis_result((values > 3).sum(axis=0), materialize(values > 3).sum(axis=0))
    check_axis_result(np.sum(values + 1, axis=1, where=True),
                      np.sum(materialize(values + 1), axis=1, where=True))
    output = np.zeros(6)
    returned = np.sum(values + 1, axis=0, out=output)
    assert returned.data == output.data
    assert np.array_equal(output, materialize(values + 1).sum(axis=0))

    def mutate_axis(array):
        array[0, 0] = 100.
        return 0

    mutable = np.array([[1., 2.], [3., 4.]])
    assert np.array_equal(np.sum(mutable + 1, axis=mutate_axis(mutable)), [6., 8.])
    assert mutable[0, 0] == 100.
    independent = (values + 1).sum(axis=())
    independent[0, 0] = 100.
    assert values[0, 0] == 0.

def check_axis_gradual_identity(dtype: type, operation: Literal[str], initial):
    from numpy.fusion import _axis_gradual_expr

    def read_value(pointer, extra):
        return pointer[0]

    values = np.array([[2, 3], [4, 5]], dtype=dtype)
    partitions = (((values.strides[0],), (values.strides[1],)),)
    for length in (0, 2):
        output = np.full(2, 123, dtype=dtype)
        _axis_gradual_expr(output, (length,), partitions, (values.data,), read_value,
                           None, initial, False, operation)
        if length == 0:
            expected = np.full(2, initial, dtype=dtype)
        elif operation == 'sum':
            expected = np.array([6, 8], dtype=dtype) + initial
        else:
            expected = np.array([8, 15], dtype=dtype) * initial
        assert np.array_equal(output, expected)

def check_axis_default_identities(dtype: type):
    values = np.array([[2, 3], [4, 5]], dtype=dtype)
    for array in (values, values[::-1], values[:, ::-1], values.copy(order='F')):
        for length in (0, 2):
            view = array[:length]
            assert np.array_equal((view + dtype(1)).sum(axis=0),
                                  materialize(view + dtype(1)).sum(axis=0))
            assert np.array_equal((view + dtype(1)).prod(axis=0),
                                  materialize(view + dtype(1)).prod(axis=0))
    check_axis_gradual_identity(dtype, 'sum', 0)
    check_axis_gradual_identity(dtype, 'prod', 1)
    check_axis_gradual_identity(dtype, 'sum', 7)
    check_axis_gradual_identity(dtype, 'prod', 3)

@test
def test_axis_gradual_identities():
    check_axis_default_identities(np.int64)
    check_axis_default_identities(np.float32)
    check_axis_default_identities(np.float64)
    check_axis_default_identities(np.complex128)

@test
def test_axis_short_circuit_loop():
    from numpy.fusion import _loop_reduce_axis

    def counted(pointer, calls):
        calls[0] += 1
        return pointer[0]

    values = np.array([[1, 0, 0], [0, 0, 1], [0, 0, 0]])
    calls = 0
    result = _loop_reduce_axis((values,), counted, __ptr__(calls), (), values,
                               1, int, bool, 'any', 1)
    assert np.array_equal(result, [True, True, False])
    assert calls == 7
    calls = 0
    result = _loop_reduce_axis((values,), counted, __ptr__(calls), (), values,
                               1, int, bool, 'all', 1)
    assert np.array_equal(result, [False, False, False])
    assert calls == 4
    calls = 0
    cube = values.reshape(3, 1, 3)
    result = _loop_reduce_axis((cube,), counted, __ptr__(calls), (), cube,
                               (1, 2), int, bool, 'any', 1)
    assert np.array_equal(result, [True, True, False])
    assert calls == 7
    calls = 0
    empty = values[:, :0]
    result = _loop_reduce_axis((empty,), counted, __ptr__(calls), (), empty,
                               1, int, bool, 'all', 2)
    assert np.array_equal(result, [[True], [True], [True]])
    assert calls == 0

test_axis_reduction_edges()
test_axis_gradual_identities()
test_axis_short_circuit_loop()
