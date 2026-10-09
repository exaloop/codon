import numpy as np

from helpers import materialize, mutate_array, events, effectful_none, record

@noinline
def clip_reference(array, lower, upper):
    array = np.asarray(array)
    lower_shape = () if lower is None else np.asarray(lower).shape
    upper_shape = () if upper is None else np.asarray(upper).shape
    shape = np.broadcast_shapes(array.shape, lower_shape, upper_shape)
    output = np.empty(shape, dtype=array.dtype)
    return np.clip(array, lower, upper, out=output)

def check_clip_result(actual, expected):
    assert actual.dtype is expected.dtype
    assert actual.shape == expected.shape
    assert actual.strides == expected.strides, (actual.strides, expected.strides)
    assert np.allclose(actual, expected, rtol=0, atol=0, equal_nan=True)
    assert np.array_equal(np.signbit(actual), np.signbit(expected))

@noinline
def clip_into_destination(array):
    output = np.empty_like(array)
    output[:] = np.clip(array, 2., 10.) * 2.
    return output

@noinline
def clip_into_ufunc(array):
    output = np.empty_like(array)
    return np.add(np.clip(array, 2., 10.), 1., out=output)

@test
def test_clip_fusion():
    matrix = np.arange(24.).reshape(4, 6)
    check_clip_result(clip_into_destination(matrix), clip_reference(matrix, 2., 10.) * 2.)
    check_clip_result(clip_into_ufunc(matrix), clip_reference(matrix, 2., 10.) + 1.)
    for array in (matrix, matrix.T, matrix[::-1, ::2], matrix[:0, :], matrix[:1, :]):
        check_clip_result(np.clip(array + 1., 2., 10.) * 3. + array,
                          clip_reference(materialize(array + 1.), 2., 10.) * 3. + array)
        check_clip_result(array.clip(min=2.) * 2.,
                          clip_reference(array, 2., None) * 2.)
        check_clip_result(np.clip(array, None, 10.) * 2.,
                          clip_reference(array, None, 10.) * 2.)
        assert np.clip(array + 1., 2., 10.).sum() == clip_reference(
            materialize(array + 1.), 2., 10.).sum()
    lower = np.arange(6.)
    upper = np.arange(4.).reshape(4, 1) + 8.
    check_clip_result(np.clip(matrix + 1., lower + 1., upper * 2.) * 3.,
                      clip_reference(matrix + 1., lower + 1., upper * 2.) * 3.)
    check_clip_result(np.clip(matrix, lower[::-1], None) + 1.,
                      clip_reference(matrix, lower[::-1], None) + 1.)
    check_clip_result(np.clip(5., lower, upper) + 1.,
                      clip_reference(5., lower, upper) + 1.)
    check_clip_result(np.clip(matrix, np.array(2.), np.array(10.)) + 1.,
                      clip_reference(matrix, np.array(2.), np.array(10.)) + 1.)
    expensive = np.clip(np.arctan2(matrix, matrix + 1.), lower, upper) + 1.
    check_clip_result(expensive, clip_reference(
        materialize(np.arctan2(matrix, matrix + 1.)), lower, upper) + 1.)

def check_clip_dtype(dtype: type):
    array = np.array([0, 1, 127, 128, 255], dtype=dtype)
    check_clip_result(np.clip(array, 0.75, 130.75) * dtype(2),
                      clip_reference(array, 0.75, 130.75) * dtype(2))
    check_clip_result(np.clip(array, 10., 2.) * dtype(1),
                      clip_reference(array, 10., 2.) * dtype(1))

@test
def test_clip_edge_values():
    check_clip_dtype(np.int8)
    check_clip_dtype(np.uint8)
    check_clip_dtype(np.int64)
    check_clip_dtype(np.float32)
    check_clip_dtype(np.float64)
    array = np.array([float('nan'), float('inf'), -float('inf'), -0., 0., 1., -1.])
    for lower, upper in ((-0., 0.), (0., -0.), (float('nan'), 2.),
                         (-2., float('nan')), (10., 2.)):
        check_clip_result(np.clip(array, lower, upper) * 1.,
                          clip_reference(array, lower, upper) * 1.)
    check_clip_result(np.clip(array, None, -0.) * 1.,
                      clip_reference(array, None, -0.) * 1.)
    check_clip_result(np.clip(array, 0., None) * 1.,
                      clip_reference(array, 0., None) * 1.)

@test
def test_clip_effects_and_errors():
    array = np.array([-1., 1., 3.])
    output = np.zeros(3)
    check_clip_result(np.clip(array, 0., 2., out=output) + 1., np.array([1., 2., 3.]))
    assert np.array_equal(output, [0., 1., 2.])
    check_clip_result(np.clip(array, 0., 2.) + mutate_array(array),
                      np.array([100., 2., 5.]))
    events.clear()
    check_clip_result(np.clip(array, 0., 2., out=effectful_none()) + 1.,
                      clip_reference(array, 0., 2.) + 1.)
    assert events == [7]
    events.clear()
    check_clip_result(np.clip(array, effectful_none(), 2.) + 1.,
                      clip_reference(array, None, 2.) + 1.)
    assert events == [7]
    events.clear()
    check_clip_result(np.clip(array, 0., effectful_none()) + 1.,
                      clip_reference(array, 0., None) + 1.)
    assert events == [7]
    events.clear()
    try:
        np.clip(np.ones((2, 3)), np.ones(4), 10.) + record(1., 1)
        assert False
    except ValueError:
        pass
    assert events == []
    try:
        np.clip(np.ones((2, 3)), None, np.ones(4)) * 2.
        assert False
    except ValueError:
        pass

@test
def test_array_copies_and_casts():
    array = np.array([127, 128, 255, 256])
    assert np.array_equal(array.astype(np.int8) + np.int8(1), [-128, -127, 0, 1])
    assert np.sum(array.astype(np.int8) + np.int8(1), dtype=int) == -254
    assert np.array_equal((array + 1).astype(np.float32), [128., 129., 256., 257.])
    assert np.array_equal(array[::-1].copy() + 1, [257, 256, 129, 128])
    assert np.array_equal(array.copy(order='F') * 2, array * 2)
    assert np.array_equal(array.astype(int, copy=False) + 1, array + 1)
    mutable = np.array([1, 2])
    assert np.array_equal(mutable.copy() + mutate_array(mutable), [101, 4])
    fractional = np.array([16777217., 1.1, -1.1])
    assert np.array_equal(fractional.astype(np.float32).astype(float) + 0.,
                          materialize(fractional.astype(np.float32)).astype(float))
    expensive = np.arctan2(np.arctan2(array, array + 1), array + 2).astype(np.float32)
    expected = materialize(np.arctan2(np.arctan2(array, array + 1), array + 2))
    assert np.array_equal(expensive, expected.astype(np.float32))
    independent = (array + 1).copy()
    independent[0] = 0
    assert array[0] == 127
    vector = np.arange(6.)
    view = vector.astype(float, copy=False).astype(float, copy=False)
    assert view.data == vector.data
    view[0] = 123.
    assert vector[0] == 123.
    matrix = np.arange(12.).reshape(3, 4) * 0.125
    snapshot = matrix.copy()
    result = np.sin(matrix.astype(float, copy=False)) + 1.
    assert np.array_equal(matrix, snapshot)
    assert result.data != matrix.data
    assert np.allclose(result, np.sin(snapshot) + 1.)
    try:
        array.astype(float, order='bad') + 1
        assert False
    except ValueError:
        pass

def check_higher_rank_producer_order(array, order: Literal[str]):
    got = (array + 0.).copy(order=order)
    expected = materialize(array + 0.).copy(order=order)
    assert np.array_equal(got, expected)
    assert got.strides == expected.strides, (order, got.strides, expected.strides)
    assert ((array.copy(order=order) + 1).sum() ==
            (materialize(array.copy(order=order)) + 1).sum())
    got = (array + 0.).astype(np.float32, order=order)
    expected = materialize(array + 0.).astype(np.float32, order=order)
    assert np.array_equal(got, expected)
    assert got.strides == expected.strides, (order, got.strides, expected.strides)
    assert ((array.astype(np.float32, order=order) + 1).sum() ==
            (materialize(array.astype(np.float32, order=order)) + 1).sum())
    assert (array.astype(float, order=order, copy=False).sum() ==
            materialize(array.astype(float, order=order, copy=False)).sum())
    got = array.astype(float, order=order, copy=False).astype(
        float, order=order, copy=False)
    expected = materialize(array.astype(float, order=order, copy=False)).astype(
        float, order=order, copy=False)
    assert got.strides == expected.strides
    assert np.array_equal(got, expected)
    if order == 'K' or order == 'A':
        assert got.data == array.data
    got = np.zeros_like(array + 1, order=order)
    expected = np.zeros_like(materialize(array + 1), order=order)
    assert np.array_equal(got, expected)
    assert got.strides == expected.strides, (order, got.strides, expected.strides)
    got = (np.ones_like(array, dtype=np.int8, order=order) +
           np.zeros_like(array, dtype=np.int8, order=order))
    expected = (materialize(np.ones_like(array, dtype=np.int8, order=order)) +
                materialize(np.zeros_like(array, dtype=np.int8, order=order)))
    assert np.array_equal(got, expected)
    assert got.strides == expected.strides
    assert ((np.ones_like(array, order=order) + array).sum() ==
            (materialize(np.ones_like(array, order=order)) + array).sum())
    got = np.ones_like(array + 1, order=order)
    expected = np.ones_like(materialize(array + 1), order=order)
    assert np.array_equal(got, expected)
    assert got.strides == expected.strides, (order, got.strides, expected.strides)

@test
def test_higher_rank_producers():
    values = np.array([1e16, 1., -1e16, -1.] * 258).reshape(2, 3, 172)
    for array in (values, values.copy(order='F'), values.transpose((1, 2, 0)),
                  values[:, ::-1, ::2], values[:, :0, :], values[:, :1, :]):
        check_higher_rank_producer_order(array, 'C')
        check_higher_rank_producer_order(array, 'F')
        check_higher_rank_producer_order(array, 'A')
        check_higher_rank_producer_order(array, 'K')
    check_higher_rank_producer_order(values.reshape(6, 172).T, 'C')
    check_higher_rank_producer_order(values.reshape(2, 3, 4, 43), 'F')
    assert ((values.copy(order='F') + values.copy(order='C')).sum() ==
            (materialize(values.copy(order='F')) +
             materialize(values.copy(order='C'))).sum())
    matrix = np.arange(12.).reshape(3, 4)
    row = np.arange(4.)
    column = np.arange(3.).reshape(3, 1)
    got = np.ones_like((matrix + column).copy(order='F')) + row
    expected = materialize(np.ones_like(materialize(matrix + column).copy(order='F'))) + row
    assert np.array_equal(got, expected)
    assert got.strides == expected.strides
    assert ((matrix.astype(np.float32) + column + row).sum() ==
            materialize(matrix.astype(np.float32) + column + row).sum())
    narrowed = np.array([127, 128, 255, 256]).reshape(2, 2)
    assert np.array_equal(narrowed.astype(np.int8) + np.int8(1),
                          np.array([-128, -127, 0, 1]).reshape(2, 2))
    fractional = np.array([16777217., 1.1, -1.1, 0.]).reshape(2, 2)
    assert np.array_equal(fractional.astype(np.float32, order='F').astype(float) + 0.,
                          materialize(fractional.astype(np.float32, order='F')).astype(float))
    expensive = np.arctan2(np.arctan2(matrix, matrix + 1), matrix + 2).astype(
        np.float32, order='F')
    expected = materialize(np.arctan2(np.arctan2(matrix, matrix + 1), matrix + 2)).astype(
        np.float32, order='F')
    assert np.array_equal(expensive, expected)
    assert expensive.strides == expected.strides
    independent = (matrix + 1).copy(order='F')
    independent[0, 0] = 100.
    assert matrix[0, 0] == 0.
    try:
        np.ones_like(matrix + np.ones((2, 4))).sum()
        assert False
    except ValueError:
        pass
    try:
        matrix.copy(order='bad') + 1
        assert False
    except ValueError:
        pass

@test
def test_filled_producers():
    array = np.array([float('nan'), float('inf'), -2., 0., 3.])
    assert np.array_equal(np.zeros_like(array) + 1, [1., 1., 1., 1., 1.])
    assert np.array_equal(np.ones_like(array, dtype=np.int8) + 2, [3, 3, 3, 3, 3])
    assert np.sum(np.ones_like(array[::-2]) + 2) == 9.
    assert np.sum(np.ones_like(array[:0]) + 2) == 0.
    assert np.array_equal(np.ones_like(array) + np.arange(5), [1., 2., 3., 4., 5.])

@noinline
def roll_stencil(array, bias, shift: int, axis: Literal[int]):
    return np.roll(array, shift, axis) + 2 * np.roll(array, -shift, axis) + bias

def check_roll_stencil(array, bias, axis: Literal[int]):
    for shift in (-99, -1, 0, 1, 99, -9_223_372_036_854_775_808):
        opposite = -shift if shift != -9_223_372_036_854_775_808 else shift
        got = roll_stencil(array, bias, shift, axis)
        expected = (materialize(np.roll(array, shift, axis)) +
                    2 * materialize(np.roll(array, opposite, axis)) + bias)
        assert got.dtype is expected.dtype
        assert got.shape == expected.shape
        assert got.strides == expected.strides, (got.strides, expected.strides)
        assert np.array_equal(got, expected)
        if array.size:
            original = array.data[0]
            got.data[0] = array.dtype(42)
            assert array.data[0] == original

@test
def test_roll_producers():
    matrix = np.arange(24.).reshape(4, 6)
    for array in (matrix, np.asfortranarray(matrix), matrix.T,
                  matrix[::-1, ::-1], matrix[::2, ::2], matrix[:0], matrix[:, :0]):
        bias = np.ones((1, array.shape[1]))
        check_roll_stencil(array, bias, 0)
        check_roll_stencil(array, bias, 1)
        check_roll_stencil(array, bias, -1)
    check_roll_stencil(matrix[:1], np.ones((4, 6)), 0)
    check_roll_stencil(np.arange(6.), np.ones((4, 6)), 0)
    check_roll_stencil(np.array(3.) + matrix, np.array(2.), 1)
    check_roll_stencil(np.broadcast_to(matrix[:1], matrix.shape), matrix, 0)
    check_roll_stencil(np.arange(6, dtype=np.float32), np.ones(6, dtype=np.float32), 0)
    check_roll_stencil(np.array([0, 1, 127, -128], dtype=np.int8),
                       np.ones(4, dtype=np.int8), 0)
    check_roll_stencil(np.array([1j, 2j, 3j]), np.ones(3, dtype=complex), 0)

@noinline
def roll_laplacian(array):
    return (np.roll(array, 1, 0) + np.roll(array, -1, 0) +
            np.roll(array, 1, 1) + np.roll(array, -1, 1) +
            np.roll(array, 1, 2) + np.roll(array, -1, 2) - 6.0 * array)

@noinline
def roll_energy(left, right):
    left_gradient = ((np.roll(left, -1, 0) - left) ** 2 +
                     (np.roll(left, -1, 1) - left) ** 2 +
                     (np.roll(left, -1, 2) - left) ** 2)
    right_gradient = ((np.roll(right, -1, 0) - right) ** 2 +
                      (np.roll(right, -1, 1) - right) ** 2 +
                      (np.roll(right, -1, 2) - right) ** 2)
    return np.mean(left_gradient + right_gradient)

@noinline
def mixed_axis_roll(array, other, shift: int):
    return np.roll(array, shift, 0) + 2 * np.roll(other, shift, -1) + array

@noinline
def roll_axis_sum(array, shift: int):
    return np.sum(np.roll(array, shift, 0) + np.roll(array, shift, -1),
                  axis=1, dtype=float, keepdims=True, initial=3.)

@noinline
def roll_scalar_sum(array):
    return np.sum(np.roll(array, 1, 0) + np.roll(array, -1, 2))

def check_multiaxis_roll(array):
    expected = (materialize(np.roll(array, 1, 0)) + materialize(np.roll(array, -1, 0)) +
                materialize(np.roll(array, 1, 1)) + materialize(np.roll(array, -1, 1)) +
                materialize(np.roll(array, 1, 2)) + materialize(np.roll(array, -1, 2)) -
                6.0 * array)
    got = roll_laplacian(array)
    assert got.shape == expected.shape
    assert got.strides == expected.strides
    assert np.array_equal(got, expected)
    assert got.data != array.data
    other = array * 0.25
    left_gradient = ((materialize(np.roll(array, -1, 0)) - array) ** 2 +
                     (materialize(np.roll(array, -1, 1)) - array) ** 2 +
                     (materialize(np.roll(array, -1, 2)) - array) ** 2)
    right_gradient = ((materialize(np.roll(other, -1, 0)) - other) ** 2 +
                      (materialize(np.roll(other, -1, 1)) - other) ** 2 +
                      (materialize(np.roll(other, -1, 2)) - other) ** 2)
    expected_energy = np.mean(materialize(left_gradient + right_gradient))
    got_energy = roll_energy(array, other)
    if array.size:
        assert got_energy == expected_energy, (got_energy, expected_energy)
    else:
        assert np.isnan(got_energy) and np.isnan(expected_energy)
    expected_sum = np.sum(materialize(
        materialize(np.roll(array, 1, 0)) + materialize(np.roll(array, -1, 2))))
    assert roll_scalar_sum(array) == expected_sum
    for shift in (-99, -1, 0, 1, 99, -9_223_372_036_854_775_808):
        for other in (array, np.ones((array.shape[-1],), dtype=array.dtype)):
            got = mixed_axis_roll(array, other, shift)
            expected = (materialize(np.roll(array, shift, 0)) +
                        2 * materialize(np.roll(other, shift, -1)) + array)
            assert got.dtype is expected.dtype
            assert got.shape == expected.shape
            assert got.strides == expected.strides
            assert np.array_equal(got, expected)
        expected = np.sum(materialize(
            materialize(np.roll(array, shift, 0)) +
            materialize(np.roll(array, shift, -1))),
            axis=1, dtype=float, keepdims=True, initial=3.)
        got = roll_axis_sum(array, shift)
        assert got.shape == expected.shape
        assert np.array_equal(got, expected)

@test
def test_multiaxis_roll_producers():
    values = np.arange(120.).reshape(4, 5, 6) * 0.125
    for array in (values, values.copy(order='F'), values.transpose((2, 0, 1)),
                  values[::-1, ::-1, ::-1], values[:, ::2, ::2], values[:0],
                  values[:, :0], values[:, :, :0], values[:1],
                  np.broadcast_to(values[:1], values.shape)):
        check_multiaxis_roll(array)
    check_multiaxis_roll(values.astype(np.float32))
    cancellation = np.array([1e16, 1., -1e16, -1.] * 258).reshape(2, 3, 172)
    for array in (cancellation, cancellation.copy(order='F'),
                  cancellation.transpose((1, 2, 0)), cancellation[:, ::-1, ::2]):
        expected = np.sum(materialize(
            materialize(np.roll(array, 1, 0)) + materialize(np.roll(array, -1, 2))))
        assert roll_scalar_sum(array) == expected
    original = values.copy()
    expected = mixed_axis_roll(original, original, 1)
    values[:] = np.roll(values, 1, 0) + 2 * np.roll(values, 1, -1) + values
    assert np.array_equal(values, expected)
    np.add(np.roll(values, 1, 0), np.roll(values, 1, -1), out=values)
    assert np.array_equal(values, materialize(np.roll(expected, 1, 0)) +
                          materialize(np.roll(expected, 1, -1)))

@noinline
def dense_roll_stencil(array):
    return (np.roll(array, 1, 0) + np.roll(array, 2, 0) +
            np.roll(array, 3, 0) + np.roll(array, 4, 0) +
            np.roll(array, 1, 1) + np.roll(array, 2, 1) +
            np.roll(array, 3, 1) + np.roll(array, 4, 1) +
            np.roll(array, 1, 2) + np.roll(array, 2, 2) +
            np.roll(array, 3, 2) + np.roll(array, 4, 2))

def check_dense_roll_stencil(array):
    expected = np.zeros_like(array)
    for axis in range(3):
        for shift in (1, 2, 3, 4):
            expected = materialize(expected + materialize(np.roll(array, shift, axis)))
    got = dense_roll_stencil(array)
    assert got.dtype is expected.dtype
    assert got.shape == expected.shape
    assert got.strides == expected.strides
    assert np.array_equal(got, expected)
    if array.size:
        original = array.data[0]
        got.data[0] = array.dtype(42)
        assert array.data[0] == original

@test
def test_roll_region_dispatch():
    from numpy.fusion import _roll_region_size

    shifts = ((1, 0, 0), (2, 0, 0), (3, 0, 0), (4, 0, 0),
              (0, 1, 0), (0, 2, 0), (0, 3, 0), (0, 4, 0),
              (0, 0, 1), (0, 0, 2), (0, 0, 3), (0, 0, 4))
    assert _roll_region_size((5, 5, 5), shifts) == 1
    assert _roll_region_size((19, 20, 21), shifts) == 63
    assert _roll_region_size((20, 20, 20), shifts) == 64
    assert _roll_region_size((21, 20, 20), shifts) == 67
    assert _roll_region_size((20, 20, 20), shifts + shifts) == 64
    assert _roll_region_size((20, 20, 20), ((0, 0, 0),) + shifts) == 64
    assert _roll_region_size((5, 5, 5), ((0, 0, 0),)) == 125
    assert _roll_region_size((), ((),)) == 1
    assert _roll_region_size((0, 5, 5), ((0, 0, 0),)) == 0
    for shape in ((5, 5, 5), (19, 20, 21), (20, 20, 20), (21, 20, 20)):
        values = np.arange(shape[0] * shape[1] * shape[2], dtype=float).reshape(shape)
        for array in (values, values.copy(order='F'), values.transpose((2, 0, 1)),
                      values[::-1, ::-1, ::-1], values[:, ::2, ::2], values[:0],
                      values[:1], np.broadcast_to(values[:1], values.shape)):
            check_dense_roll_stencil(array)
    values = np.arange(125.).reshape(5, 5, 5) * 0.125
    check_dense_roll_stencil(values.astype(np.float32))
    check_dense_roll_stencil(values.astype(np.int8))

@noinline
def dynamic_axis_roll(array, shift: int, axis: int):
    return np.roll(array, shift, axis) + 1

@noinline
def forwarded_roll(array, shift: int):
    rolled = np.roll(array, shift, 0)
    shift += 1
    return rolled + array + shift

@test
def test_roll_fallbacks():
    array = np.arange(12.).reshape(3, 4)
    for axis in (0, 1, -1):
        assert np.array_equal(dynamic_axis_roll(array, 1, axis),
                              materialize(np.roll(array, 1, axis)) + 1)
    assert np.array_equal(np.roll(array, 1) + 1, materialize(np.roll(array, 1)) + 1)
    assert np.array_equal(np.roll(array, (1, 2), (0, 1)) + 1,
                          materialize(np.roll(array, (1, 2), (0, 1))) + 1)
    assert np.array_equal(np.roll(array, 1, 0) + np.roll(array, 1, 1),
                          materialize(np.roll(array, 1, 0)) + materialize(np.roll(array, 1, 1)))
    assert np.array_equal(np.roll(array + 1, 1, 0) + 2,
                          materialize(np.roll(materialize(array + 1), 1, 0)) + 2)
    assert np.array_equal(np.roll(np.roll(array, 1, 0), 1, 0) + 1,
                          materialize(np.roll(materialize(np.roll(array, 1, 0)), 1, 0)) + 1)
    assert (np.roll(array, 1, 0) + array).sum() == (
        materialize(np.roll(array, 1, 0)) + array).sum()
    assert np.array_equal(np.arctan2(np.arctan2(np.roll(array, 1, 0), array + 1), array + 2),
                          np.arctan2(np.arctan2(materialize(np.roll(array, 1, 0)), array + 1), array + 2))
    copy = np.roll(array, 0, 0)
    copy[0, 0] = -1
    assert array[0, 0] == 0
    original = array.copy()
    array[:] = np.roll(array, 1, 0) + 1
    assert np.array_equal(array, materialize(np.roll(original, 1, 0)) + 1)
    output = np.empty_like(array)
    np.add(np.roll(array, 1, 0), 1, out=output)
    assert np.array_equal(output, materialize(np.roll(array, 1, 0)) + 1)

@test
def test_roll_effects_and_errors():
    array = np.array([1., 2., 3.])
    assert np.array_equal(forwarded_roll(array, 1), [6., 5., 7.])
    indices = np.array([2, 0, 1])
    assert np.array_equal(np.roll(array[indices], 1, 0) + 1, [3., 4., 2.])
    assert np.array_equal(np.roll(array, 1, 0) + mutate_array(array), [103., 3., 5.])
    array = np.array([1., 2., 3.])
    rolled = np.roll(array, 1, 0)
    array[0] = 100.
    assert np.array_equal(rolled + array, [103., 3., 5.])
    events.clear()
    assert np.array_equal(np.roll(array, record(1, 5), 0) + 1, [4., 101., 3.])
    assert events == [5]
    events.clear()
    try:
        np.roll(array, 1, 2) + record(1., 9)
        assert False
    except np.AxisError:
        pass
    assert events == []
    try:
        np.roll(array, 1, 0) + np.ones(4)
        assert False
    except ValueError:
        pass

test_roll_producers()
test_multiaxis_roll_producers()
test_roll_region_dispatch()
test_roll_fallbacks()
test_roll_effects_and_errors()
test_array_copies_and_casts()
test_higher_rank_producers()
test_filled_producers()
test_clip_fusion()
test_clip_edge_values()
test_clip_effects_and_errors()
