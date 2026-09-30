import numpy as np

from helpers import check_axis_result, materialize

def check_axis_reducers(array, axis, keepdims: Literal[bool]):
    check_axis_result((array + 1).sum(axis=axis, keepdims=keepdims),
                      materialize(array + 1).sum(axis=axis, keepdims=keepdims))
    check_axis_result(np.sum(array * 2, axis=axis, keepdims=keepdims, initial=0.5),
                      np.sum(materialize(array * 2), axis=axis,
                             keepdims=keepdims, initial=0.5))
    check_axis_result((array * 0.01 + 1).prod(axis=axis, keepdims=keepdims, initial=2.),
                      materialize(array * 0.01 + 1).prod(axis=axis,
                                                       keepdims=keepdims, initial=2.))
    check_axis_result((array < 1).any(axis=axis, keepdims=keepdims),
                      materialize(array < 1).any(axis=axis, keepdims=keepdims))
    check_axis_result(np.all(array >= 1, axis=axis, keepdims=keepdims),
                      np.all(materialize(array >= 1), axis=axis, keepdims=keepdims))
    check_axis_result((array + 1).min(axis=axis, keepdims=keepdims, initial=0.),
                      materialize(array + 1).min(axis=axis, keepdims=keepdims, initial=0.))
    check_axis_result(np.amax(array + 1, axis=axis, keepdims=keepdims, initial=0.),
                      np.amax(materialize(array + 1), axis=axis,
                              keepdims=keepdims, initial=0.))

@test
def test_axis_reductions():
    values = np.array([1e16, 1., -1e16, -1.] * 258).reshape(2, 3, 172)
    for array in (values, values.copy(order='F'), values.transpose((1, 2, 0)),
                  values[:, ::-1, ::2], values[:, :0, :], values[:, :1, :]):
        for axis in range(-3, 3):
            check_axis_reducers(array, axis, False)
            check_axis_reducers(array, axis, True)
        for axes in ((0, 1), (2, 0), (-1, -2)):
            check_axis_reducers(array, axes, False)
            check_axis_reducers(array, axes, True)
        check_axis_reducers(array, (), False)
        check_axis_reducers(array, (), True)
        check_axis_reducers(array, (2, 1, 0), False)
        check_axis_reducers(array, (2, 1, 0), True)
        check_axis_reducers(array, None, True)
    matrix = np.arange(24.).reshape(4, 6)
    check_axis_reducers(matrix.T, 0, False)
    check_axis_reducers(matrix.T, -1, True)
    check_axis_reducers(matrix.reshape(2, 1, 3, 4), (0, 2), True)
    check_axis_reducers(np.arange(17.), -1, False)
    check_axis_reducers(np.arange(17.), 0, True)
    check_axis_result((matrix.astype(np.float32, order='F') + 1).sum(axis=1),
                      materialize(matrix.astype(np.float32, order='F') + 1).sum(axis=1))
    check_axis_result((matrix + np.arange(6.)).sum(axis=0),
                      materialize(matrix + np.arange(6.)).sum(axis=0))
    for axis in range(2):
        check_axis_result((matrix + 1).min(axis=axis),
                          materialize(matrix + 1).min(axis=axis))
        check_axis_result(np.amin(matrix * 2, axis=axis),
                          np.amin(materialize(matrix * 2), axis=axis))
        check_axis_result((matrix + 1).max(axis=axis, keepdims=True),
                          materialize(matrix + 1).max(axis=axis, keepdims=True))

@test
def test_axis_negative_strides():
    from numpy.fusion import _loop_reduce_axis

    def record_axis(pointer, context):
        trace, count = context
        trace[count[0]] = pointer[0]
        count[0] += 1
        return pointer[0]

    values = np.arange(12).reshape(3, 4)
    for array in (values[::-1], values[:, ::-1], values[::-1, ::-1]):
        trace = np.empty(12, dtype=int)
        count = 0
        result = _loop_reduce_axis((array,), record_axis, (trace.data, __ptr__(count)),
                                   0, array, 0, int, int, 'sum', 1)
        assert np.array_equal(result, array.sum(axis=0))
        assert count == 12
        assert np.array_equal(trace, array.flatten())

test_axis_reductions()
test_axis_negative_strides()
