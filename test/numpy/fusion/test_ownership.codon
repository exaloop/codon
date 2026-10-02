import numpy as np

from helpers import materialize, mutate_array

@noinline
def lifetime_update(values):
    values += 2.0 * values[::-1]
    temporary = values * values + 1.0
    values[:] = temporary
    values[:] = temporary
    temporary = values * 2.0
    values[:] = temporary
    temporary = temporary * 3.0
    values[:] = temporary

@noinline
def lifetime_escape(values):
    temporary = values * 2.0
    view = temporary[::-1]
    values[:] = temporary
    return view

@noinline
def lifetime_loop(values):
    temporary = values * 2.0
    for iteration in range(4):
        values[:] = temporary
        temporary = values + 1.0
    return temporary

@noinline
def lifetime_exception(values):
    temporary = values * 2.0
    try:
        values[:1] = temporary
    except ValueError:
        return temporary[::-1]
    return temporary

@noinline
def lifetime_local_loop(values, count):
    for iteration in range(count):
        temporary = values + 1.0
        values[:] = temporary
        values[:] = temporary

@noinline
def lifetime_transferred(values):
    temporary = np.exp(values)
    total = temporary.sum()
    return temporary / total

@noinline
def masked_update(values, mask, other):
    values[mask] = values[mask] ** 2 + other[mask]

@noinline
def gathered_view(values, mask, output):
    temporary = values[mask]
    view = temporary[::-1]
    output[:] = temporary
    return view

@noinline
def mutate_mask(mask):
    mask[:] = False
    return mask

@test
def test_masked_ownership():
    for original in (np.arange(12.).reshape(3, 4).astype(complex),
                     np.arange(12.).reshape(3, 4).astype(complex).T,
                     np.arange(12.).reshape(3, 4).astype(complex)[::-1, ::-1]):
        for mask in (original.real > 5, original.real > 100, original.real >= 0):
            values = original.copy()
            other = original + 1j
            selected = materialize(values[mask])
            expected = values.copy()
            expected[mask] = materialize(selected * selected) + materialize(other[mask])
            masked_update(values, mask, other)
            assert np.array_equal(values, expected)
            assert np.array_equal(other, original + 1j)
            output = np.empty(selected.shape, dtype=complex)
            escaped = gathered_view(original, mask, output)
            for repeat in range(16):
                replacement = np.ones(selected.shape, dtype=complex) * (repeat + 1)
                assert replacement.size == selected.size
            assert np.array_equal(escaped, selected[::-1])
            assert np.array_equal(output, selected)
    values = np.arange(8.)
    indices = np.array([6, 1, 6, 0])
    before = values.copy()
    values[indices] = values[indices] * 2 + values[indices]
    assert np.array_equal(values, [0., 3., 2., 3., 4., 5., 18., 7.])
    assert np.array_equal(indices, [6, 1, 6, 0])
    assert np.array_equal(before, np.arange(8.))
    values = np.array([1., 2., 3.])
    mask = np.array([True, True, True])
    result = values[mask] ** 2 + mutate_array(values)
    assert np.array_equal(result, [101., 6., 12.])
    mask[:] = True
    try:
        values[mask] ** 2 + values[mutate_mask(mask)]
        assert False
    except ValueError:
        assert not np.any(mask)

@noinline
def filtered_replacement(values, count, stop):
    for iteration in range(count):
        if iteration == stop:
            break
        if iteration % 2:
            continue
        mask = values > iteration
        values = values[mask]
    return values

@noinline
def filtered_aliases(values, count):
    saved = []
    for iteration in range(count):
        saved.append(values[::-1])
        values = values[values > iteration]
    return saved

@noinline
def filtered_exception(values, count):
    for iteration in range(count):
        try:
            values = values[values > iteration]
            if iteration == 2:
                raise ValueError('filtered')
        except ValueError:
            return values[::-1]
    return values[::-1]

@noinline
def filtered_pair(left, right, count):
    for iteration in range(count):
        if not len(left):
            break
        np.multiply(left, 1., left)
        mask = left > iteration
        np.logical_not(mask, mask)
        np.logical_not(mask, mask)
        left, right = left[mask], right[mask]
    return left, right

@noinline
def retained_filtered_tuple(values, count):
    saved = []
    for iteration in range(count):
        pair = values[values > iteration], values[values > iteration]
        values = pair[0]
        saved.append(pair)
    return saved

@test
def test_filtered_replacement():
    for original in (np.arange(16.), np.arange(16.)[::-1], np.arange(32.)[::2], np.arange(0.),
                     np.arange(63.), np.arange(64.), np.arange(65.), np.arange(96.)[::-1]):
        for count in (0, 1, 7):
            for stop in (-1, 0, 3):
                snapshot = original.copy()
                expected = original
                for iteration in range(count):
                    if iteration == stop:
                        break
                    if iteration % 2:
                        continue
                    expected = materialize(expected[expected > iteration])
                result = filtered_replacement(original, count, stop)
                for repeat in range(32):
                    replacement = np.ones(original.shape) * repeat
                    assert replacement.size == original.size
                assert np.array_equal(result, expected)
                assert np.array_equal(original, snapshot)
            saved = filtered_aliases(original, count)
            expected = original
            for iteration in range(count):
                assert np.array_equal(saved[iteration], expected[::-1])
                expected = materialize(expected[expected > iteration])
            expected = original
            for iteration in range(min(count, 3)):
                expected = materialize(expected[expected > iteration])
            assert np.array_equal(filtered_exception(original, count), expected[::-1])
            left, right = filtered_pair(original.copy(), original.copy() + 10., count)
            saved = retained_filtered_tuple(original, count)
            expected = original
            for iteration in range(count):
                expected = materialize(expected[expected > iteration])
                assert np.array_equal(saved[iteration][0], expected)
                assert np.array_equal(saved[iteration][1], expected)
            for repeat in range(32):
                replacement = np.ones(original.shape) * repeat
                assert replacement.size == original.size
            assert np.array_equal(left, expected)
            assert np.array_equal(right, expected + 10.)
    matrix = np.arange(12.).reshape(3, 4)
    rows, columns = np.array([2, 0, 2]), np.array([3, 1, 0])
    assert np.array_equal(matrix[rows, columns] ** 2 + matrix[rows, columns], [132., 2., 72.])
    assert np.array_equal(matrix[rows, 1] * 2 + matrix[rows, 1], [27., 3., 27.])

@test
def test_prompt_release():
    for original in (np.arange(17.), np.arange(34.)[::2], np.arange(17.)[::-1], np.arange(0.),
                     np.arange(63.), np.arange(64.), np.arange(65.)):
        for values in (original.copy(), original.copy()[::-1], np.repeat(original, 2)[::2]):
            snapshot = values.copy()
            expected = snapshot + 2.0 * snapshot[::-1]
            expected = (expected * expected + 1.0) * 6.0
            lifetime_update(values)
            assert np.array_equal(values, expected)
            for count in (0, 1, 7):
                before = values.copy()
                lifetime_local_loop(values, count)
                assert np.array_equal(values, before + count)
        escaped_input = original.copy()
        escaped = lifetime_escape(escaped_input)
        carried = lifetime_loop(original.copy())
        caught = lifetime_exception(np.arange(17.))
        borrowed = original.astype(float, copy=False)
        values = original.copy()
        values[:] = borrowed
        for repeat in range(32):
            replacement = np.ones(original.shape) * (repeat + 1.0)
            assert replacement.size == original.size
        assert np.array_equal(values, original)
        assert np.array_equal(escaped, (original * 2.0)[::-1])
        assert np.array_equal(carried, original * 2.0 + 4.0)
        assert np.array_equal(caught, (np.arange(17.) * 2.0)[::-1])
    values = np.arange(17.)
    expected = np.exp(values)
    expected /= expected.sum()
    transferred = lifetime_transferred(values)
    values[:] = np.conj(values)
    for repeat in range(32):
        replacement = np.ones(17) * (repeat + 1.0)
        assert replacement.size == 17
    assert np.allclose(transferred, expected)
    assert np.array_equal(values, np.arange(17.))

@test
def test_inner_loop_aliases():
    from numpy.fusion import _loop_basic

    def subtract(destination, left, right, extra):
        destination[0] = left[0] - right[0] + extra

    original = np.arange(63.).reshape(7, 9)
    for array in (original.copy(), original.copy(order='F'),
                  original[:, ::-1], original[::2, ::2], original[:0, :]):
        for other in (np.ones(array.shape), np.ones((1, array.shape[1])),
                      np.ones((array.shape[0], 1)), np.array(2.),
                      np.ones(array.shape, dtype=np.float32)):
            expected = array - other + 3.
            _loop_basic((array, array, other), subtract, 3.)
            assert np.array_equal(array, expected)
            expected = other - array + 3.
            _loop_basic((array, other, array), subtract, 3.)
            assert np.array_equal(array, expected)
            output = np.empty_like(array)
            expected = array - other + 3.
            _loop_basic((output, array, other), subtract, 3.)
            assert np.array_equal(output, expected)

    array = np.arange(9.).reshape(3, 3)
    reference = array.copy()
    other = np.ones((3, 3))
    np.ndarray._loop((reference, reference.T, other), subtract, extra=3.)
    _loop_basic((array, array.T, other), subtract, 3.)
    assert np.array_equal(array, reference)

test_masked_ownership()
test_filtered_replacement()
test_prompt_release()
test_inner_loop_aliases()
