import numpy as np

from helpers import L, eq, f

@test
def test_basic():
    # A few trivial cases
    a = np.array([1, 2, 3, 4])
    b = np.array([10, 11, 12, 13])

    assert eq((1 + a)/2, [1., 1.5, 2., 2.5], float)
    assert eq(a + b**2, [101, 123, 147, 173], int)
    assert eq(np.multiply(a, b) + np.subtract(a, b) + np.square(a) + (b**2), [102, 138, 180, 228], int)

@test
def test_order():
    # Make sure things are executed in the correct order after fusion
    L.clear()
    a = np.array([1, 2, 3, 4])
    b = np.array([10, 11, 12, 13])

    z = (f(1, '1') + f(a, 'a')) * (f(b, 'b') + f(2, '2'))
    assert eq(z, [24, 39, 56, 75], int)
    assert L == ['1', 'a', 'b', '2']

test_basic()
test_order()
