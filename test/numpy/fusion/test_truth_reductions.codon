import numpy as np

from helpers import effectful_none, events, materialize, mutate_initial

def check_truth_and_extrema(array):
    assert (array < 1).any() == materialize(array < 1).any()
    assert np.any(array >= 2) == np.any(materialize(array >= 2))
    assert (array < 3).all() == materialize(array < 3).all()
    assert np.all(array >= 0) == np.all(materialize(array >= 0))
    assert (array * 2).any() == materialize(array * 2).any()
    assert (array * 2).all() == materialize(array * 2).all()
    assert (array + 1).min() == materialize(array + 1).min()
    assert (array + 1).max() == materialize(array + 1).max()
    assert np.amin(array * 2) == np.min(materialize(array * 2))
    assert np.amax(array * 2) == np.max(materialize(array * 2))
    assert np.min(array + 1, initial=-3) == np.min(materialize(array + 1), initial=-3)
    assert np.max(array + 1, initial=99) == np.max(materialize(array + 1), initial=99)

def check_truth_and_extrema_dtype(dtype: type):
    array = np.array([0, 1, 2, 3, 4, 5], dtype=dtype)
    check_truth_and_extrema(array)
    check_truth_and_extrema(array.reshape(2, 3))
    check_truth_and_extrema(array[::-2])
    assert not (array[:0] < 1).any()
    assert (array[:0] < 1).all()
    assert (array[:0] + 1).min(initial=3) == dtype(3)
    assert (array[:0] + 1).max(initial=3) == dtype(3)

@test
def test_truth_and_extrema_reductions():
    check_truth_and_extrema_dtype(np.float64)
    check_truth_and_extrema_dtype(np.float32)
    check_truth_and_extrema_dtype(np.float16)
    check_truth_and_extrema_dtype(np.int8)
    check_truth_and_extrema_dtype(np.uint8)
    check_truth_and_extrema_dtype(np.int64)
    check_truth_and_extrema_dtype(np.complex128)
    check_truth_and_extrema_dtype(np.complex64)
    values = np.arange(120, dtype=float).reshape(2, 3, 20)
    for array in (values, values.copy(order='F'), values.transpose((1, 2, 0)),
                  values[:, ::-1, ::2]):
        check_truth_and_extrema(array)
    matrix = values.reshape(6, 20)
    row = np.arange(20)
    assert np.any(matrix < row + 1) == np.any(materialize(matrix < row + 1))
    assert np.all(matrix >= row) == np.all(materialize(matrix >= row))
    assert np.amin(matrix + row) == np.min(materialize(matrix + row))
    assert np.amax(matrix.T + 1) == np.max(materialize(matrix.T + 1))
    assert not np.any(matrix[:0] < row)
    assert np.all(matrix[:0] < row)
    assert np.amin(matrix[:0] + row, initial=7.) == 7.
    assert np.amax(matrix[:0] + row, initial=-7.) == -7.
    for reduction in (np.min, np.max, np.amin, np.amax):
        try:
            reduction(matrix[:0] + 1)
            assert False
        except ValueError as error:
            assert 'zero-size array to reduction operation' in str(error)
    try:
        np.any(matrix < np.arange(19))
        assert False
    except ValueError:
        pass

    booleans = np.array([True, False])
    assert not np.min(~booleans)
    assert np.max(~booleans)
    zeros = np.array([0., -0.])
    assert np.signbit((zeros * 1.).min())
    assert not np.signbit((zeros * 1.).max())
    special = np.array([0., float('inf'), float('-inf'), float('nan')])
    assert np.isnan(np.min(special * 1.))
    assert np.isnan(np.max(special * 1.))
    assert np.any(special * 1.)
    assert not np.all(special * 1.)
    assert np.all(special[1:] * 1.)
    narrow = np.array([120, 125], dtype=np.int8)
    assert np.min(narrow + np.int8(10), dtype=float) == -126.
    assert np.max(narrow + np.int8(10), dtype=float) == -121.

    assert np.any(matrix < 1, where=True)
    assert not np.all(matrix < 1, where=True)
    assert np.min(matrix + 1, where=True, initial=1000.) == 1.
    assert np.max(matrix + 1, where=True, initial=-1000.) == 120.
    assert np.array_equal(np.any(matrix < 1, axis=0), materialize(matrix < 1).any(axis=0))
    assert np.array_equal(np.all(matrix < 1, keepdims=True), [[False]])
    output = np.array(0.)
    assert np.max(matrix + 1, out=output) is output
    assert output == 120.
    events.clear()
    assert np.any(matrix < 1, axis=effectful_none())
    assert events == [7]
    mutable = np.array([1, 2, 3])
    assert np.max(mutable + 1, initial=mutate_initial(mutable)) == 4

test_truth_and_extrema_reductions()
