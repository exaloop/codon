import numpy as np
import math

def check_binary_outer(ufunc, left, right, dtype: type = NoneType):
    from numpy import util

    first, second = util.op_types(left.dtype, right.dtype)
    result_dtype = type(ufunc._op(first, second)) if dtype is NoneType else dtype
    expected = np.empty(left.shape + right.shape, dtype=result_dtype)
    for left_index in util.multirange(left.shape):
        for right_index in util.multirange(right.shape):
            first = util.cast(left._ptr(left_index)[0], type(first))
            second = util.cast(right._ptr(right_index)[0], type(second))
            expected._ptr(left_index + right_index)[0] = util.cast(
                ufunc._op(first, second), result_dtype)

    actual = ufunc.outer(left, right, dtype=dtype)
    assert actual.shape == expected.shape
    assert actual.dtype is expected.dtype
    assert actual.strides == expected.strides
    if actual.dtype is np.float32 or actual.dtype is float or actual.dtype is complex:
        assert np.allclose(actual, expected, equal_nan=True)
    else:
        assert np.array_equal(actual, expected)

@test
def test_binary_outer_layouts():
    left = np.arange(24, dtype=np.int32).reshape(4, 6)
    right = np.arange(30, dtype=np.int32).reshape(5, 6)
    for first in (left[:, 2], left[::-1, 2], left[0, ::2], left[0, :1],
                  left[0, :0], np.broadcast_to(left[0, :1], (9,))):
        for second in (right[0], right[0, ::-1], right[0, ::2], right[0, :1],
                       right[0, :0], np.broadcast_to(right[0, :1], (7,))):
            check_binary_outer(np.add, first, second)
            check_binary_outer(np.subtract, first, second)
    for first in (left, left.T, left.copy(order='F'), left[::-1, ::2], left[:0]):
        for second in (right, right.T, right.copy(order='F'), right[::2, ::-1], right[:, :0]):
            check_binary_outer(np.multiply, first, second)
    check_binary_outer(np.add, np.array(3, dtype=np.int32), right)
    check_binary_outer(np.subtract, left, np.array(2, dtype=np.int32))
    check_binary_outer(np.add, np.array(3, dtype=np.int32), np.array(2, dtype=np.int32))
    check_binary_outer(np.add, left.reshape(2, 3, 4), right[0])
    assert np.array_equal(left, np.arange(24, dtype=np.int32).reshape(4, 6))
    assert np.array_equal(right, np.arange(30, dtype=np.int32).reshape(5, 6))
    result = np.add.outer(left[0], right[0])
    result[:] = -1
    assert left[0, 0] == 0 and right[0, 0] == 0
    assert np.array_equal(np.add.outer([1, 2], [3, 4]), np.array([[4, 5], [5, 6]]))
    assert np.add.outer(3, 4).shape == ()
    assert np.add.outer(3, 4).item() == 7

@test
def test_binary_outer_dtypes():
    check_binary_outer(np.add, np.array([127, -128], dtype=np.int8),
                       np.array([1, -1], dtype=np.int8), dtype=np.int64)
    check_binary_outer(np.add, np.array([255, 0], dtype=np.uint8),
                       np.array([1, 255], dtype=np.uint8), dtype=np.uint16)
    check_binary_outer(np.add, np.array([1.75, -2.5]), np.array([0.75, 1.5]), dtype=np.int32)
    check_binary_outer(np.subtract, np.array([1, 2], dtype=np.int16),
                       np.array([0.5, 1.5], dtype=np.float32))
    check_binary_outer(np.true_divide, np.array([1, 3], dtype=np.int32),
                       np.array([2, 4], dtype=np.int32))
    check_binary_outer(np.less, np.array([1, 3]), np.array([2, 4]))
    check_binary_outer(np.logical_and, np.array([True, False]), np.array([False, True]))
    check_binary_outer(np.multiply, np.array([1 + 2j, 3 - 4j]), np.array([2., -1.]))
    check_binary_outer(np.minimum, np.array([float('nan'), float('inf'), -0.]),
                       np.array([-float('inf'), 0., 1.]))
    zeros = np.multiply.outer(np.array([0., -0.]), np.array([-1., 1.]))
    assert np.array_equal(np.signbit(zeros), np.array([[True, False], [False, True]]))

test_binary_outer_layouts()
test_binary_outer_dtypes()

def check_axis_reduction_and_scan(ufunc, values):
    from numpy import util

    for axis in (0, 1, -1):
        normalized = axis % 2
        length = values.shape[normalized]
        width = values.shape[1 - normalized]
        expected = np.empty((width,), dtype=values.dtype)
        scan = np.empty(values.shape, dtype=values.dtype)
        for lane in range(width):
            total = util.cast(3, values.dtype)
            current = values[0, lane] if normalized == 0 else values[lane, 0]
            for step in range(length):
                element = values[step, lane] if normalized == 0 else values[lane, step]
                total = util.cast(ufunc._op(total, element), values.dtype)
                if step:
                    current = util.cast(ufunc._op(current, element), values.dtype)
                if normalized == 0:
                    scan[step, lane] = current
                else:
                    scan[lane, step] = current
            expected[lane] = total
        initial = util.cast(3, values.dtype)
        assert np.array_equal(ufunc.reduce(values, axis=axis, initial=initial), expected)
        assert np.array_equal(ufunc.accumulate(values, axis=axis), scan)
        kept = ufunc.reduce(values, axis=axis, initial=initial, keepdims=True)
        assert kept.shape == ((1, width) if normalized == 0 else (width, 1))
        assert np.array_equal(kept.ravel(), expected)

@test
def test_ordered_ufunc_axis_loops():
    backing = np.arange(6 * 37, dtype=np.int32).reshape(6, 37)
    for values in (backing, backing.T, backing.copy(order='F'), backing[::-1, ::-1],
                   backing[::2, ::3], np.broadcast_to(backing[:1], (6, 37))):
        check_axis_reduction_and_scan(np.add, values)
        check_axis_reduction_and_scan(np.subtract, values)
        check_axis_reduction_and_scan(np.maximum, values)
    floats = np.array([[1e16, 1e16, -0.], [1., -1e16, 0.], [-1e16, 1., -0.]])
    check_axis_reduction_and_scan(np.add, floats)
    check_axis_reduction_and_scan(np.subtract, floats)
    for column in range(3):
        lanes = np.broadcast_to(floats[:, column:column+1], (3, 37))
        check_axis_reduction_and_scan(np.add, lanes)
        check_axis_reduction_and_scan(np.subtract, lanes)
    assert np.array_equal(np.add.reduce(floats, axis=0), np.array([0., 1., 0.]))
    assert np.array_equal(np.maximum.reduce(backing, axis=0), backing[-1])
    assert np.array_equal(np.add.reduce(backing, axis=(), initial=np.int32(0)), backing)
    assert np.add.reduce(backing, axis=(0, 1), initial=np.int32(0)) == 24531
    cube = np.arange(24).reshape(2, 3, 4)
    assert np.array_equal(np.add.reduce(cube, axis=(0, 2)), np.array([60, 92, 124]))

test_ordered_ufunc_axis_loops()

@test
def test_ufunc_axis_loop_boundaries():
    values = np.arange(5 * 19, dtype=np.int32).reshape(5, 19)
    mask = (values % 3) != 0
    expected = np.zeros(19, dtype=np.int64)
    for column in range(19):
        for row in range(5):
            if mask[row, column]:
                expected[column] += int(values[row, column])
    actual = np.add.reduce(values, axis=0, dtype=np.int64, where=mask)
    assert np.array_equal(actual, expected)
    output = np.full(38, -1, dtype=np.int64)
    np.add.reduce(values, axis=0, dtype=np.int64, where=mask, out=output[::2])
    assert np.array_equal(output[::2], expected)
    assert np.all(output[1::2] == -1)
    np.add.reduce(values, axis=0, dtype=np.int64, where=False, out=output[::2])
    assert np.array_equal(output[::2], expected)
    lane_mask = np.arange(19) % 2 == 0
    masked = np.add.reduce(values, axis=0, dtype=np.int64, where=lane_mask)
    for column in range(19):
        assert masked[column] == (5 * column + 190 if lane_mask[column] else 0)

    aliased = values.copy()
    np.add.reduce(aliased, axis=0, initial=np.int32(0), out=aliased[0])
    assert np.array_equal(aliased[0], np.arange(19, dtype=np.int32) * 5 + 190)
    for row in range(1, 5):
        assert np.array_equal(aliased[row], values[row])
    scan = np.add.accumulate(values, axis=0)
    aliased = values.copy()
    np.add.accumulate(aliased, axis=0, out=aliased)
    assert np.array_equal(aliased, scan)
    output_scan = np.full((5, 38), -1, dtype=np.int32)
    np.add.accumulate(values, axis=0, out=output_scan[:, ::2])
    assert np.array_equal(output_scan[:, ::2], scan)
    assert np.all(output_scan[:, 1::2] == -1)
    promoted = np.add.accumulate(values.astype(np.int64), axis=0, dtype=np.int64)
    assert promoted.dtype is np.int64
    assert np.array_equal(promoted, scan.astype(np.int64))
    reduced = np.add.reduce(values, axis=0, dtype=np.float64, initial=0.)
    assert np.array_equal(reduced, (np.arange(19) * 5 + 190).astype(float))

    for width in (0, 1, 3, 4, 7, 15, 16, 17, 33):
        array = np.arange(3 * width).reshape(3, width)
        assert np.array_equal(np.add.reduce(array, axis=0), np.arange(width) * 3 + 3 * width)
        assert np.add.accumulate(array, axis=0).shape == array.shape
    assert np.array_equal(np.add.reduce(np.empty((0, 17)), axis=0), np.zeros(17))
    cube = np.arange(2 * 3 * 19).reshape(2, 3, 19)
    expected_cube = np.arange(19) * 6 + 285
    assert np.array_equal(np.add.reduce(cube, axis=(0, 1)), expected_cube)
    assert np.array_equal(np.add.reduce(cube, axis=(1, 0)), expected_cube)
    assert np.add.reduce(cube, axis=(0, 1), keepdims=True).shape == (1, 1, 19)
    assert np.array_equal(np.add.reduce(cube, axis=0), cube[0] + cube[1])
    scan_cube = np.add.accumulate(cube, axis=0)
    assert np.array_equal(scan_cube[0], cube[0])
    assert np.array_equal(scan_cube[1], cube[0] + cube[1])
    scan_cube = np.add.accumulate(cube, axis=1)
    assert np.array_equal(scan_cube[:, 0], cube[:, 0])
    assert np.array_equal(scan_cube[:, 1], cube[:, 0] + cube[:, 1])
    assert np.array_equal(scan_cube[:, 2], cube[:, 0] + cube[:, 1] + cube[:, 2])
    check_axis_reduction_and_scan(np.add, np.array([[127, -128], [1, -1]], dtype=np.int8))
    check_axis_reduction_and_scan(np.add, np.array([[255, 0], [1, 255]], dtype=np.uint8))
    check_axis_reduction_and_scan(np.add, np.array([[1e8, 1e8], [1., -1e8], [-1e8, 1.]], dtype=np.float32))
    check_axis_reduction_and_scan(np.add, np.array([[1+2j, 3+4j], [5-6j, 7-8j]]))
    for width in (16, 63, 64, 65):
        check_axis_reduction_and_scan(np.add, np.broadcast_to(np.array([[127], [1]], dtype=np.int8), (2, width)))
        check_axis_reduction_and_scan(np.add, np.broadcast_to(np.array([[255], [1]], dtype=np.uint8), (2, width)))
    check_axis_reduction_and_scan(np.add, np.broadcast_to(np.array([[1e8], [1.], [-1e8]], dtype=np.float32), (3, 33)))
    check_axis_reduction_and_scan(np.add, np.broadcast_to(np.array([[1+2j], [5-6j]]), (2, 33)))
    zeros = np.array([[-0., 0.], [-0., -0.]])
    assert np.array_equal(np.signbit(np.add.accumulate(zeros, axis=0)),
                          np.array([[True, False], [True, False]]))
    nan_values = np.array([[1., float('nan')], [float('nan'), 2.]])
    assert np.isnan(np.minimum.reduce(nan_values, axis=0)).all()
    assert np.isnan(np.minimum.accumulate(nan_values, axis=0)[1]).all()
    zero_lanes = np.broadcast_to(zeros[:, :1], (2, 33))
    assert np.signbit(np.add.reduce(zero_lanes, axis=0, initial=None)).all()
    assert np.signbit(np.add.accumulate(zero_lanes, axis=0)).all()
    nan_lanes = np.broadcast_to(nan_values[:, :1], (2, 33))
    assert np.isnan(np.minimum.reduce(nan_lanes, axis=0)).all()
    assert np.isnan(np.minimum.accumulate(nan_lanes, axis=0)[1]).all()

test_ufunc_axis_loop_boundaries()

@noinline
def singleton_arctan2_expression(left, right):
    return np.arctan2(left, right) + 1.

def check_vectorized_singleton_broadcast(dtype: type):
    backing = np.full(513, 1000., dtype=dtype)
    backing[0] = 3.
    for size in (0, 1, 17, 257):
        values = np.linspace(0.5, 4., 2 * size, dtype=dtype)
        for vector in (values[:size], values[:size][::-1], values[::2]):
            for singleton in (backing[:1], np.array([3.], dtype=dtype)):
                for left, right in ((vector, singleton), (singleton, vector)):
                    expected_angle = np.empty_like(vector)
                    expected_norm = np.empty_like(vector)
                    for index in range(vector.size):
                        first = float(left[0] if left.size == 1 else left[index])
                        second = float(right[0] if right.size == 1 else right[index])
                        expected_angle[index] = math.atan2(first, second)
                        expected_norm[index] = math.hypot(first, second)
                    assert np.allclose(np.arctan2(left, right), expected_angle)
                    assert np.allclose(np.hypot(left, right), expected_norm)
                    assert np.allclose(singleton_arctan2_expression(left, right),
                                       expected_angle + 1.)
                    output = np.full(2 * vector.size, -1., dtype=dtype)
                    np.arctan2(left, right, out=output[::2])
                    assert np.allclose(output[::2], expected_angle)
                    assert np.all(output[1::2] == -1.)
    assert backing[0] == 3. and np.all(backing[1:] == 1000.)

@test
def test_vectorized_singleton_broadcast():
    check_vectorized_singleton_broadcast(np.float32)
    check_vectorized_singleton_broadcast(np.float64)

test_vectorized_singleton_broadcast()

@noinline
def standalone_absolute(array):
    return np.absolute(array)

def check_complex_absolute_strides(dtype: type):
    values = np.array([[3+4j, 5+12j], [8+15j, 7+24j]], dtype=dtype)
    expected = np.array([[5., 13.], [17., 25.]])
    for array in (values, values.copy(order='F'), values[::-1, ::-1],
                  values[:, ::2], values[:0, :]):
        reference = np.sqrt(array.real * array.real + array.imag * array.imag)
        assert np.allclose(standalone_absolute(array), reference)
        output = np.empty(array.shape, dtype=array.real.dtype)
        np.absolute(array, out=output)
        assert np.allclose(output, reference)
    assert np.array_equal(standalone_absolute(values), expected)

@test
def test_complex_absolute_strides():
    check_complex_absolute_strides(np.complex64)
    check_complex_absolute_strides(np.complex128)

test_complex_absolute_strides()

@test
def test_add_at_repeated_indices_scalar_b():
    a = np.zeros((5,), dtype=float)
    idx = np.array([2, 2, 2, 4], dtype=int)
    np.add.at(a, idx, 3.0)
    # a[2] += 3 three times, a[4] += 3 once
    assert np.allclose(a, np.array([0, 0, 9, 0, 3], dtype=float))

@test
def test_add_at_repeated_indices_vector_b_aligned_by_iteration_not_idx():
    a = np.zeros((6,), dtype=float)
    idx = np.array([4, 4, 1, 4], dtype=int)
    b = np.array([10.0, 1.5, -2.0, 0.5], dtype=float)
    np.add.at(a, idx, b)
    # iteration-aligned:
    # a[4]+=10 ; a[4]+=1.5 ; a[1]+=-2 ; a[4]+=0.5  => a[4]=12.0, a[1]=-2
    assert np.allclose(a, np.array([0, -2, 0, 0, 12.0, 0], dtype=float))

@test
def test_add_at_rowwise():
    A = np.arange((27 * 16), dtype=float).reshape(27, 16)
    B = np.ones((7,), dtype=int) * 26
    C = np.zeros((7 * 16), dtype=float).reshape(7, 16)

    # Make C non-trivial so we can validate accumulation
    for i in range(7):
        C[i, :] = i + 1  # rows: all 1s, all 2s, ..., all 7s

    A_ref = A.copy()
    np.add.at(A, B, C)

    # Expected: only row 26 changes; it gets sum of all C rows
    A_ref[26, :] += C.sum(axis=0)
    assert np.allclose(A, A_ref)

@test
def test_add_at_preserves_other_rows():
    rng = np.random.default_rng(seed=0)
    A = rng.standard_normal((10, 4))
    B = np.array([3, 3, 3], dtype=int)
    C = np.ones((3, 4), dtype=float)
    A0 = A.copy()

    np.add.at(A, B, C)

    # rows != 3 unchanged
    mask = np.ones((10,), dtype=bool)
    mask[3] = False
    assert np.allclose(A[mask], A0[mask])

@test
def test_add_at_negative_indices():
    a = np.zeros((5,), dtype=float)
    idx = np.array([-1, -1, 0], dtype=int)
    b = np.array([1.0, 2.0, 3.0], dtype=float)
    np.add.at(a, idx, b)
    # a[-1]+=1 ; a[-1]+=2 ; a[0]+=3  => a[4]=3, a[0]=3
    assert np.allclose(a, np.array([3, 0, 0, 0, 3], dtype=float))

@test
def test_mul_at_repeated_indices_vector_b():
    a = np.ones((4,), dtype=float)
    idx = np.array([2, 2, 2], dtype=int)
    b = np.array([2.0, 3.0, 4.0], dtype=float)
    np.multiply.at(a, idx, b)
    # sequential: a[2] = 1*2*3*4 = 24
    assert np.allclose(a, np.array([1, 1, 24, 1], dtype=float))

@test
def test_at_sequential_semantics_noncommutative_like_true_divide():
    # This catches buffering bugs: if you accidentally buffer reads/writes,
    # repeated indices won’t behave like sequential application.
    a = np.array([100.0], dtype=float)
    idx = np.array([0, 0, 0], dtype=int)
    b = np.array([2.0, 5.0, 2.0], dtype=float)
    np.true_divide.at(a, idx, b)
    # sequential: ((100 / 2) / 5) / 2 = 5
    assert np.allclose(a, np.array([5.0], dtype=float))

test_add_at_repeated_indices_scalar_b()
test_add_at_repeated_indices_vector_b_aligned_by_iteration_not_idx()
test_add_at_rowwise()
test_add_at_preserves_other_rows()
test_add_at_negative_indices()
test_mul_at_repeated_indices_vector_b()
test_at_sequential_semantics_noncommutative_like_true_divide()
