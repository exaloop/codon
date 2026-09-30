import numpy as np

from helpers import L, eq, f

@test
def test_forwarding():
    a = np.array([[1, 2, 3], [4, 5, 6], [7, 8, 9]])

    # Basic
    L.clear()
    expr1 = (f(a, 'a1') + 1) * (f(a, 'a2') + 2)
    expr2 = (f(1, '1') + expr1) ** 2
    assert eq(expr2, [[   49,   169,   441],
                      [  961,  1849,  3249],
                      [ 5329,  8281, 12321]], int)
    assert L == ['a1', 'a2', '1']

    # Multiple
    L.clear()
    expr1 = (f(a, 'a1') + 1) * (f(a, 'a2') + 2)
    expr2 = (f(a, 'a3') + 3) * (f(a, 'a4') + 4)
    expr3 = (expr2 * f(2, '1') + expr1) ** 2
    assert eq(expr3, [[  2116,   5184,  10816],
                      [ 20164,  34596,  55696],
                      [ 85264, 125316, 178084]], int)
    assert L == ['a1', 'a2', 'a3', 'a4', '1']

    # Chain
    L.clear()
    expr1 = (f(a, 'a1') + 1) * (f(a, 'a2') + 2)
    expr2 = (expr1 + 3) * (f(a, 'a4') + 4)
    expr3 = (expr2 * f(2, '1')) ** 2
    assert eq(expr3, [[   8100,   32400,  103684],
                      [ 278784,  656100, 1392400],
                      [2722500, 4981824, 8631844]], int)
    assert L == ['a1', 'a2', 'a4', '1']

    # Double Chain
    L.clear()
    expr1 = (f(a, 'a1') + 1) ^ (f(a, 'a2') + 2)
    expr2 = (expr1 + 3) ^ (f(a, 'a3') + 4)

    expr3 = (f(a, 'a4') + 3) ^ (f(a, 'a5') + 4)
    expr4 = (expr3 + 5) ^ (f(a, 'a6') + 6)

    expr5 = (expr2 * f(2, '1') + expr4) ** 2
    assert eq(expr5, [[   9,  576,  441],
                      [3364, 1521, 2704],
                      [1681,  484,  729]], int)
    assert L == ['a1', 'a2', 'a3', 'a4', 'a5', 'a6', '1']

    # No forwarding
    L.clear()
    expr1 = (f(a, 'a1') + 1) * (f(a, 'a2') + 2)
    L.append('break')
    expr2 = (f(1, '1') + expr1) ** 2
    assert eq(expr2, [[   49,   169,   441],
                      [  961,  1849,  3249],
                      [ 5329,  8281, 12321]], int)
    # expr1 is not forwarded, so order is as you'd expect
    assert L == ['a1', 'a2', 'break', '1']

test_forwarding()
