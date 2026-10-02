import numpy as np

@noinline
def forwarding_branch(values, condition):
    temporary = values + 1
    if condition:
        values[0] = 100
    return temporary * 2

@noinline
def forwarding_reassignment(values, condition):
    factor = 2
    temporary = values * factor
    if condition:
        factor = 10
    return temporary + factor

@noinline
def forwarding_loop_before(values, count):
    temporary = values + 1
    while count > 0:
        values[0] += 10
        count -= 1
    return temporary * 2

@noinline
def forwarding_loop_consumer(values, count):
    temporary = values + 1
    result = np.zeros_like(values)
    for iteration in range(count):
        result[:] = temporary * 2
        values[0] += 10
    return result

@noinline
def forwarding_conditional_error(left, right, condition):
    temporary = left + right
    if condition:
        return temporary * 2.
    return np.zeros(1)

@noinline
def forwarding_early_return_error(left, right, condition):
    temporary = left + right
    if condition:
        return np.zeros(1)
    return temporary * 2.

@noinline
def forwarding_loop_error(left, right, count):
    temporary = left + right
    result = np.zeros(1)
    for iteration in range(count):
        result = temporary * 2.
    return result

@noinline
def forwarding_join(left, right, condition):
    temporary = left + right
    factor = 2. if condition else 3.
    return temporary * factor

@test
def test_forwarding_eager_errors():
    left = np.ones(2)
    right = np.ones(3)
    for condition in (False, True):
        caught = False
        try:
            forwarding_conditional_error(left, right, condition)
        except ValueError:
            caught = True
        assert caught
        caught = False
        try:
            forwarding_early_return_error(left, right, condition)
        except ValueError:
            caught = True
        assert caught
        caught = False
        try:
            forwarding_join(left, right, condition)
        except ValueError:
            caught = True
        assert caught
        assert np.array_equal(forwarding_conditional_error(left, left, condition),
                              np.full(2, 4.) if condition else np.zeros(1))
        assert np.array_equal(forwarding_early_return_error(left, left, condition),
                              np.zeros(1) if condition else np.full(2, 4.))
        assert np.array_equal(forwarding_join(left, left, condition),
                              np.full(2, 4. if condition else 6.))
    for count in (0, 1, 3):
        caught = False
        try:
            forwarding_loop_error(left, right, count)
        except ValueError:
            caught = True
        assert caught
        assert np.array_equal(forwarding_loop_error(left, left, count),
                              np.full(2, 4.) if count else np.zeros(1))

@test
def test_forwarding_control_flow():
    for condition in (False, True):
        values = np.array([1, 2, 3])
        assert np.array_equal(forwarding_branch(values, condition), [4, 6, 8])
        assert values[0] == (100 if condition else 1)
        values = np.array([1, 2, 3])
        expected = [12, 14, 16] if condition else [4, 6, 8]
        assert np.array_equal(forwarding_reassignment(values, condition), expected)
    for count in (0, 1, 3):
        values = np.array([1, 2, 3])
        assert np.array_equal(forwarding_loop_before(values, count), [4, 6, 8])
        assert values[0] == 1 + 10 * count
        values = np.array([1, 2, 3])
        expected = [4, 6, 8] if count else [0, 0, 0]
        assert np.array_equal(forwarding_loop_consumer(values, count), expected)
        assert values[0] == 1 + 10 * count

test_forwarding_eager_errors()
test_forwarding_control_flow()
