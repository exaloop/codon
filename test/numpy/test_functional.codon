import numpy as np

@test
def test_apply_along_axis():

    def my_func(a):
        return (a[0] + a[-1]) * 0.5

    b = np.array([[1, 2, 3], [4, 5, 6], [7, 8, 9]])
    assert np.array_equal(np.apply_along_axis(my_func, 0, b), [4., 5., 6.])
    assert np.array_equal(np.apply_along_axis(my_func, 1, b), [2., 5., 8.])

    b = np.array([[8, 1, 7], [4, 3, 9], [5, 2, 6]])
    assert np.array_equal(np.apply_along_axis(sorted, 1, b),
                          [[1, 7, 8], [3, 4, 9], [2, 5, 6]])

    b = np.array([[1, 2, 3], [4, 5, 6], [7, 8, 9]])
    assert np.array_equal(
        np.apply_along_axis(np.diag, -1, b),
        [[[1, 0, 0], [0, 2, 0], [0, 0, 3]], [[4, 0, 0], [0, 5, 0], [0, 0, 6]],
         [[7, 0, 0], [0, 8, 0], [0, 0, 9]]])

test_apply_along_axis()

@test
def test_apply_over_axes():
    a = np.arange(24).reshape(2, 3, 4)
    assert np.array_equal(np.apply_over_axes(np.sum, a, [0, 2]),
                          [[[60], [92], [124]]])
    assert np.array_equal(np.apply_over_axes(np.mean, a, [0, 2]),
                          [[[7.5], [11.5], [15.5]]])
    assert np.array_equal(np.apply_over_axes(np.sum, a, [0, 1, 2]), [[[276]]])
    assert np.array_equal(np.apply_over_axes(np.mean, a, [0, 1, 2]),
                          [[[11.5]]])

test_apply_over_axes()

@test
def test_vectorize():

    def f1(a):
        return 2 * a + 1

    def f3(a, b):
        if a > b:
            return a - b
        else:
            return a + b

    v1 = np.vectorize(f1)
    v3 = np.vectorize(f3)
    a = np.arange(24).reshape(2, 3, 4)

    assert np.array_equal(v1([1, 2, 3, 4]), [3, 5, 7, 9])

    assert np.array_equal(v3([1, 2, 3, 4], 2), [3, 4, 1, 2])
    a = np.arange(24).reshape(2, 3, 4)
    assert np.array_equal(v3(a, a), 2 * a)

test_vectorize()

@test
def test_frompyfunc():

    def f1(a):
        return 2 * a + 1

    def f2(a):
        return (a / 2, 3 * a + 1)

    def f3(a, b):
        if a > b:
            return a - b
        else:
            return a + b

    v1 = np.frompyfunc(f1, nin=1, nout=1, identity=None)
    v2 = np.frompyfunc(f2, nin=1, nout=2, identity=None)
    v3 = np.frompyfunc(f3, nin=2, nout=1, identity=None)
    a = np.arange(24).reshape(2, 3, 4)

    assert np.array_equal(v1([1, 2, 3, 4]), [3, 5, 7, 9])

    b1, b2 = v2(a)
    assert np.array_equal(b1, a / 2)
    assert np.array_equal(b2, 3 * a + 1)

    assert np.array_equal(v3([1, 2, 3, 4], 2), [3, 4, 1, 2])
    a = np.arange(24).reshape(2, 3, 4)
    assert np.array_equal(v3(a, a), 2 * a)

test_frompyfunc()
