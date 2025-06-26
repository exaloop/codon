import numpy as np

@test
def test_average(a,
                 expected,
                 axis=None,
                 weights=None,
                 returned: Literal[bool] = False,
                 keepdims: Literal[bool] = False):
    if isinstance(expected, int) or isinstance(expected, float):
        assert np.average(a,
                          axis=axis,
                          weights=weights,
                          returned=returned,
                          keepdims=keepdims) == expected
    else:
        assert (np.average(a,
                           axis=axis,
                           weights=weights,
                           returned=returned,
                           keepdims=keepdims) == expected).all()

test_average([5], 5)
test_average([2, 2, 2, 2, 2], 2)
test_average(np.arange(-5, 0), -3)
test_average(np.arange(1, 5), 2.5)
test_average([-1, 2, -3, 4, -5], -0.6)
test_average(np.arange(1, 11), 4.0, weights=np.arange(10, 0, -1))
test_average(np.arange(6).reshape((3, 2)),
             np.array([0.75, 2.75, 4.75]),
             axis=1,
             weights=[1. / 4, 3. / 4])
test_average(np.arange(6).reshape((3, 2)),
             np.array([[0.5], [2.5], [4.5]]),
             axis=1,
             keepdims=True)
w1 = [0, 1, 1, 1, 1, 0]
w2 = [[0, 1, 1, 1, 1, 0], [1, 0, 0, 0, 0, 1]]
y = np.array([[0., 1., 2., 3., 4., 5.], [0., 2., 4., 6., 8., 10.]])
test_average(np.arange(6), 2.5, axis=0, weights=w1)
test_average(y, np.array([0., 1.5, 3., 4.5, 6., 7.5]), axis=0)
test_average(y, np.array([2.5, 5.]), axis=1)
test_average(y, 3.3333333333333335, weights=w2)
test_average(y, [0., 1., 2., 3., 4., 10.], axis=0, weights=w2)
y = np.arange(12).reshape(2, 2, 3)
w = np.array([0., 0., 1., .5, .5, 0., 0., .5, .5, 1., 0., 0.]).reshape(2, 2, 3)
test_average(y, np.array([7., 5.5, 4.]), axis=(0, 1), weights=w)

def test_average2():
    assert np.isnan(np.average(np.empty(0, float)))
    assert np.isnan(np.average([1, 2, np.nan, 4, 5]))
    assert np.isinf(np.average([1, 2, np.inf, 4, 5]))
    assert np.isnan(np.average([1, 2, np.nan, np.inf, 4, 5]))
    assert np.average(np.arange(1, 11),
                      weights=np.arange(10, 0, -1),
                      returned=True) == (4.0, 55.0)
    assert (np.average(np.array([[1, 2, 3], [4, 5, 6]]), axis=1,
                       returned=True)[0] == np.array([2., 5.])).all()
    assert (np.average(np.array([[1, 2, 3], [4, 5, 6]]), axis=1,
                       returned=True)[1] == np.array([3., 3.])).all()
    a = np.arange(6)
    b = np.arange(6) * 3
    assert (np.average([[a, b], [b, a]], axis=1, returned=True)[0] == np.array(
        [[0., 2., 4., 6., 8., 10.], [0., 2., 4., 6., 8., 10.]])).all()
    assert (np.average([[a, b], [b, a]], axis=1,
                       returned=True)[1] == np.array([[2., 2., 2., 2., 2., 2.],
                                                      [2., 2., 2., 2., 2.,
                                                       2.]])).all()
    assert (np.average([[a, b], [b, a]], axis=1, keepdims=True,
                       returned=True)[0] == np.array(
                           [[[0., 2., 4., 6., 8., 10.]],
                            [[0., 2., 4., 6., 8., 10.]]])).all()
    x = np.arange(6).reshape(2, 3)
    w2 = [[0, 0, 1], [1, 2, 3]]
    assert (np.round(
        np.average(x, axis=1, keepdims=True, returned=True, weights=w2)[0],
        8) == np.round(np.array([[2.], [4.33333333]]), 8)).all()
    assert (np.average(x, axis=1, keepdims=True, returned=True,
                       weights=w2)[1] == np.array([[1.], [6.]])).all()

test_average2()

@test
def test_cov(m,
             expected,
             y=None,
             rowvar=True,
             bias=False,
             ddof: Optional[int] = None,
             fweights=None,
             aweights=None,
             dtype: type = NoneType):
    assert np.isnan(np.cov(np.empty(0, float)))
    assert np.isnan(np.cov(np.array([2])))
    assert np.asarray(
        np.round(
            np.cov(m,
                   y=y,
                   rowvar=rowvar,
                   bias=bias,
                   ddof=ddof,
                   fweights=fweights,
                   aweights=aweights,
                   dtype=dtype), 8) == expected).all()

test_cov(
    np.array([[0, 2], [1, 1], [2, 0]]).T, np.array([[1., -1.], [-1., 1.]]))
test_cov(np.array([1, 2, 3, 4, 5]), np.array(2.5))
test_cov(np.array([1, 2, 3, 4, 5]), np.array(2.5), rowvar=False)
test_cov(np.array([1, 2, 3, 4, 5]), np.array(10.), ddof=4)
test_cov(np.array([[1, 2, 3], [4, 5, 6], [7, 8, 9]]),
         np.array([[1., 1., 1.], [1., 1., 1.], [1., 1., 1.]]))
test_cov(np.array([[1, 2, 3], [4, 5, 6], [7, 8, 9]]),
         np.array([[9., 9., 9.], [9., 9., 9.], [9., 9., 9.]]),
         rowvar=False)
test_cov(np.array([[1, 2, 3], [4, 5, 6], [7, 8, 9]]),
         np.array([[0.66666667, 0.66666667, 0.66666667],
                   [0.66666667, 0.66666667, 0.66666667],
                   [0.66666667, 0.66666667, 0.66666667]]),
         rowvar=True,
         bias=True)
test_cov(np.array([1, 2, 3, 4, 5]), np.array(2.), bias=True)
test_cov(np.array([1, 2, 3]),
         np.array([[1., 0.5], [0.5, 0.25]]),
         y=np.array([0.5, 1.0, 1.5]))
test_cov(np.array([1, 2, 3]),
         np.array(0.98095238),
         fweights=np.array([6, 1, 8]))
test_cov(np.array([1, 2, 3]),
         np.array(1.66129032),
         aweights=np.array([6, 1, 8]))
test_cov(np.array([[1, 2, 3], [4, 5, 6], [7, 8, 9]]),
         np.array([[0.475, 0.475, 0.475], [0.475, 0.475, 0.475],
                   [0.475, 0.475, 0.475]]),
         fweights=np.array([1, 2, 3]),
         aweights=np.array([0.5, 1.0, 1.5]))
test_cov(np.array([1, 2, 3, 4, 5]),
         np.array(1.74193548 + 0.j),
         aweights=np.array([0.1, 0.2, 0.3, 0.2, 0.1]),
         dtype=complex)
test_cov(np.array([[1 + 0j, 2 + 0j, 3 + 0j], [1j, 2j, 3j]]),
         np.array([[1. + 0.j, 0. - 1.j], [0. + 1.j, 1. + 0.j]]))
test_cov(np.array([[1, 2, 3]]),
         np.array([[1. + 0j, -1.j], [1.j, 1. + 0j]]),
         y=np.array([[1j, 2j, 3j]]))
test_cov(np.array([0.3942, 0.5969, 0.7730, 0.9918, 0.7964]),
         np.array(0.05084135))

@test
def test_corrcoef(x, expected, y=None, rowvar=True, dtype: type = NoneType):
    assert np.isnan(np.corrcoef(np.empty(0, float)))
    assert np.isnan(np.corrcoef(np.array([2])))
    if isinstance(expected, int) or isinstance(expected, float):
        assert np.corrcoef(x, y=y, rowvar=rowvar, dtype=dtype) == expected
    else:
        assert (np.round(np.corrcoef(x, y=y, rowvar=rowvar, dtype=dtype),
                         6) == np.round(expected, 6)).all()

test_corrcoef(
    np.array([[0.77395605, 0.43887844, 0.85859792],
              [0.69736803, 0.09417735, 0.97562235],
              [0.7611397, 0.78606431, 0.12811363]]),
    np.array([[1., 0.99256089, -0.68080986], [0.99256089, 1., -0.76492172],
              [-0.68080986, -0.76492172, 1.]]))
test_corrcoef(
    np.array([[0.77395605, 0.43887844, 0.85859792],
              [0.69736803, 0.09417735, 0.97562235],
              [0.7611397, 0.78606431, 0.12811363]]),
    np.array(
        [[1., 0.99256089, -0.68080986, 0.75008178, -0.934284, -0.99004057],
         [0.99256089, 1., -0.76492172, 0.82502011, -0.97074098, -0.99981569],
         [-0.68080986, -0.76492172, 1., -0.99507202, 0.89721355, 0.77714685],
         [0.75008178, 0.82502011, -0.99507202, 1., -0.93657855, -0.83571711],
         [-0.934284, -0.97074098, 0.89721355, -0.93657855, 1., 0.97517215],
         [-0.99004057, -0.99981569, 0.77714685, -0.83571711, 0.97517215, 1.]]),
    y=np.array([[0.45038594, 0.37079802, 0.92676499],
                [0.64386512, 0.82276161, 0.4434142],
                [0.22723872, 0.55458479, 0.06381726]]))
test_corrcoef(
    np.array([[0.77395605, 0.43887844, 0.85859792],
              [0.69736803, 0.09417735, 0.97562235],
              [0.7611397, 0.78606431, 0.12811363]]),
    np.array(
        [[1., 0.77598074, -0.47458546, -0.75078643, -0.9665554, 0.22423734],
         [0.77598074, 1., -0.92346708, -0.99923895, -0.58826587, -0.44069024],
         [-0.47458546, -0.92346708, 1., 0.93773029, 0.23297648, 0.75137473],
         [-0.75078643, -0.99923895, 0.93773029, 1., 0.55627469, 0.47536961],
         [-0.9665554, -0.58826587, 0.23297648, 0.55627469, 1., -0.46666491],
         [0.22423734, -0.44069024, 0.75137473, 0.47536961, -0.46666491, 1.]]),
    y=np.array([[0.45038594, 0.37079802, 0.92676499],
                [0.64386512, 0.82276161, 0.4434142],
                [0.22723872, 0.55458479, 0.06381726]]),
    rowvar=False)
test_corrcoef(np.array([[1 + 0j, 2 + 0j, 3 + 0j], [1j, 2j, 3j]]),
              np.array([[1. + 0.j, 0. - 1.j], [0. + 1.j, 1. + 0.j]]))
test_corrcoef(np.array([[1, 2, 3]]),
              np.array([[1. + 0j, -1.j], [1.j, 1. + 0j]]),
              y=np.array([[1j, 2j, 3j]]))
test_corrcoef([[1e-100, 1e100], [1e100, 1e-100]],
              np.array([[1., -1.], [-1., 1.]]))

@test
def test_correlate(a, b, expected, mode: Literal[str] = 'valid'):
    assert (np.round(np.correlate(a, b, mode=mode),
                     8) == np.round(expected, 8)).all()

test_correlate([1, 2, 3], [0., 1., 0.5], np.array(3.5))
test_correlate([0 + 0j, 4.5 + 2.1 - 9j, 1 + 0j, 0 + 0.5j],
               [1 + 1j, 2 + 0j, 3 - 1j], np.array([16.2 - 17.j, -0.9 - 14.1j]))
test_correlate([0.8, 4.6, 3.4], [19j, -8 + 5j, 2 + 5j, 13 - 1j, 9 - 10j],
               np.array([92. + 34.6j, 47. - 23.6j, -30. - 55.2j]))
test_correlate([1, 2, 3], [0., 1., 0.5], np.array([2., 3.5, 3.]), mode="same")
test_correlate([0 + 0j, 4.5 + 2.1 - 9j, 1 + 0j, 0 + 0.5j],
               [1 + 1j, 2 + 0j, 3 - 1j],
               np.array([28.8 - 20.4j, 16.2 - 17.j, -0.9 - 14.1j, 1. + 0.j]),
               mode='same')
test_correlate([0.8, 4.6, 3.4], [19j, -8 + 5j, 2 + 5j, 13 - 1j, 9 - 10j],
               np.array([
                   51.8 + 46.8j, 92. + 34.6j, 47. - 23.6j, -30. - 55.2j,
                   -27.2 - 104.4j
               ]),
               mode='same')
test_correlate([1, 2, 3], [0., 1., 0.5],
               np.array([0.5, 2., 3.5, 3., 0.]),
               mode="full")
test_correlate([0 + 0j, 4.5 + 2.1 - 9j, 1 + 0j, 0 + 0.5j],
               [1 + 1j, 2 + 0j, 3 - 1j],
               np.array([
                   0. + 0.j, 28.8 - 20.4j, 16.2 - 17.j, -0.9 - 14.1j, 1. + 0.j,
                   0.5 + 0.5j
               ]),
               mode='full')
test_correlate([0.8, 4.6, 3.4], [19j, -8 + 5j, 2 + 5j, 13 - 1j, 9 - 10j],
               np.array([
                   7.2 + 8.j, 51.8 + 46.8j, 92. + 34.6j, 47. - 23.6j,
                   -30. - 55.2j, -27.2 - 104.4j, 0. - 64.6j
               ]),
               mode='full')
test_correlate([1 + 1j, 2 + 0j, 3 - 1j], [0 + 0j, 1 + 0j, 0 + 0.5j],
               np.array(
                   [0.5 - 0.5j, 1.0 + 0.j, 1.5 - 1.5j, 3.0 - 1.j, 0.0 + 0.j]),
               mode='full')
test_correlate([0 + 0j, 1 + 0j, 0 + 0.5j], [1 + 1j, 2 + 0j, 3 - 1j],
               np.array([0.j, 3. + 1.j, 1.5 + 1.5j, 1. + 0.j, 0.5 + 0.5j]),
               mode='full')
test_correlate(np.array([1 + 0j, 2 + 0j, 3 + 0j, 4 + 1j]),
               np.array([-1 + 0j, -2j, 3 + 1j]),
               np.array([8. + 1.j, 11. + 5.j]))

@test
def test_correlate2():
    # Integer inputs
    a = np.array([1, 2, 3])
    b = np.array([4, 5, 6])
    assert np.allclose(np.correlate(a, b, mode="valid"), [32])
    assert np.allclose(np.correlate(a, b, mode="same"), [17, 32, 23])
    assert np.allclose(np.correlate(a, b, mode="full"), [6, 17, 32, 23, 12])

    # Floating-point inputs
    a = np.array([1.5, 2.5, 3.5])
    b = np.array([4.0, 5.0, 6.0])
    assert np.allclose(np.correlate(a, b, mode="valid"), [39.5])
    assert np.allclose(np.correlate(a, b, mode="same"), [22.5, 39.5, 27.5])
    assert np.allclose(np.correlate(a, b, mode="full"), [9.0, 22.5, 39.5, 27.5, 14.0])

    # Complex numbers
    a = np.array([1+2j, 3+4j])
    b = np.array([5+6j, 7+8j])
    assert np.allclose(np.correlate(a, b, mode="valid"), [70+8j])
    assert np.allclose(np.correlate(a, b, mode="same"), [23+6j, 70+8j])
    assert np.allclose(np.correlate(a, b, mode="full"), [23+6j, 70+8j, 39+2j])

    # Different-length arrays
    a = np.array([1, 2, 3, 4])
    b = np.array([0, 1])
    assert np.allclose(np.correlate(a, b, mode="valid"), [2, 3, 4])
    assert np.allclose(np.correlate(a, b, mode="same"), [1, 2, 3, 4])
    assert np.allclose(np.correlate(a, b, mode="full"), [1, 2, 3, 4, 0])
    a = np.array([0, 1])
    b = np.array([1, 2, 3, 4])
    assert np.allclose(np.correlate(a, b, mode="valid")[::-1], [2, 3, 4])
    assert np.allclose(np.correlate(a, b, mode="same")[::-1], [1, 2, 3, 4])
    assert np.allclose(np.correlate(a, b, mode="full")[::-1], [1, 2, 3, 4, 0])

    # Large array test
    a = np.arange(20)
    b = np.arange(10)
    expected_valid = np.array([np.sum(a[i : i + len(b)] * b) for i in range(len(a) - len(b) + 1)])
    expected_full = np.correlate(a, b, mode="full")
    expected_same = np.correlate(a, b, mode="same")
    assert np.allclose(np.correlate(a, b, mode="valid"), expected_valid)
    assert np.allclose(np.correlate(a, b, mode="full"), expected_full)
    assert np.allclose(np.correlate(a, b, mode="same"), expected_same)

    # Different dtypes (int and float)
    a = np.array([1, 2, 3], dtype=int)
    b = np.array([1.5, 2.5, 3.5], dtype=float)
    assert np.allclose(np.correlate(a, b, mode="valid"), [17.0])
    assert np.allclose(np.correlate(a, b, mode="same"), [9.5, 17.0, 10.5])
    assert np.allclose(np.correlate(a, b, mode="full"), [3.5, 9.5, 17.0, 10.5, 4.5])

    # Edge case: Single-element arrays
    a = np.array([5])
    b = np.array([10])
    assert np.allclose(np.correlate(a, b, mode="valid"), [50])
    assert np.allclose(np.correlate(a, b, mode="same"), [50])
    assert np.allclose(np.correlate(a, b, mode="full"), [50])

test_correlate2()

@test
def test_bincount(x, expected, weights=None, minlength=0):
    assert (np.bincount(x, weights=weights,
                        minlength=minlength) == expected).all()

test_bincount([1], np.array([0, 1]))
test_bincount(np.array([0, 1, 2, 3, 4]), np.array([1, 1, 1, 1, 1]))
test_bincount(
    np.array([0, 1, 1, 3, 2, 1, 7, 23]),
    np.array([
        1, 3, 1, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1
    ]))
test_bincount(np.array([0, 1, 1, 2, 2, 2]),
              np.array([0.3, 0.7, 1.1]),
              weights=np.array([0.3, 0.5, 0.2, 0.7, 1., -0.6]))
test_bincount(np.array([0, 1, 1, 3, 2, 1, 7]),
              np.array([1, 3, 1, 1, 0, 0, 0, 1]),
              minlength=2)
test_bincount(np.array([0, 1, 1, 3, 2, 1, 7]),
              np.array([
                  1, 3, 1, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                  0, 0
              ]),
              minlength=22)
test_bincount(np.array([1, 2, 4, 5, 2]), np.array([0, 0.2, 0.5, 0, 0.5, 0.1]),
              np.array([0.2, 0.3, 0.5, 0.1, 0.2]))
test_bincount(np.array([0, 1, 1, 2, 2, 3, 3]),
              np.array([1, 2, 2, 2]),
              minlength=2)
test_bincount(np.array([0, 1, 1, 2, 2, 3, 3]),
              np.array([1, 2, 2, 2]),
              minlength=0)
test_bincount(np.array([1, 2, 4, 5, 2]),
              np.array([0, 0.2, 0.5, 0, 0.5, 0.1, 0, 0]),
              np.array([0.2, 0.3, 0.5, 0.1, 0.2]), 8)
test_bincount(np.empty(0, int), np.empty(0, int))

@test
def test_digitize(x, bins, expected, right=False):
    assert (np.digitize(x, bins, right=right) == expected).all()

test_digitize(np.array([[0, 1, 1], [3, 2, 7]]), np.array([0, 1, 2, 4, 8]),
              np.array([[1, 2, 2], [3, 3, 4]]))
test_digitize(np.array([0.2, 6.4, 3.0, 1.6]),
              np.array([0.0, 1.0, 2.5, 4.0, 10.0]), np.array([1, 4, 3, 2]))
test_digitize(np.array([1.2, 10.0, 12.4, 15.5, 20.]),
              np.array([0, 5, 10, 15, 20]),
              np.array([1, 2, 3, 4, 4]),
              right=True)
test_digitize(np.array([1.2, 10.0, 12.4, 15.5, 20.]),
              np.array([0, 5, 10, 15, 20]), np.array([1, 3, 3, 4, 5]))
test_digitize(np.array([[0, 1, 1], [3, 2, 7]]),
              np.array([0.0, 1.0, 2.5, 4.0, 10.0]),
              np.array([[0, 1, 1], [3, 2, 4]]),
              right=True)
test_digitize(np.arange(-6, 5), np.arange(-5, 5), np.arange(11))
test_digitize(np.arange(5, -6, -1), np.arange(5, -5, -1), np.arange(11))
test_digitize(np.arange(-6, 5), np.arange(-6, 4), np.arange(11), True)
x = [-1, 0, 1, 2]
bins = [0, 0, 1]
test_digitize(x, bins, np.array([0, 2, 3, 3]), False)
test_digitize(x, bins, np.array([0, 0, 2, 3]), True)
bins = [1, 1, 0]
test_digitize(x, bins, np.array([3, 2, 0, 0]), False)
test_digitize(x, bins, np.array([3, 3, 2, 0]), True)
bins = [1, 1, 1, 1]
test_digitize(x, bins, np.array([0, 0, 4, 4]), False)
test_digitize(x, bins, np.array([0, 0, 0, 4]), True)

@test
def test_histogram_bin_edges(a, expected, bins=10, range=None, weights=None):
    assert (np.round(
        np.histogram_bin_edges(a, bins=bins, range=range, weights=weights),
        8) == np.round(expected, 8)).all()

test_histogram_bin_edges(
    np.empty(0, float),
    np.array([0., 0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 1.]))
test_histogram_bin_edges(
    [1], np.array([0.5, 0.6, 0.7, 0.8, 0.9, 1., 1.1, 1.2, 1.3, 1.4, 1.5]))
test_histogram_bin_edges(
    [1, 100],
    np.array([1., 10.9, 20.8, 30.7, 40.6, 50.5, 60.4, 70.3, 80.2, 90.1, 100.]))
test_histogram_bin_edges(
    np.array([0, 0, 0, 1, 2, 3, 3, 4, 5]),
    np.array([0., 0.5, 1., 1.5, 2., 2.5, 3., 3.5, 4., 4.5, 5.]))
test_histogram_bin_edges(np.array([0, 0, 0, 1, 2, 3, 3, 4, 5]),
                         np.array([
                             0., 0.38461538, 0.76923077, 1.15384615,
                             1.53846154, 1.92307692, 2.30769231, 2.69230769,
                             3.07692308, 3.46153846, 3.84615385, 4.23076923,
                             4.61538462, 5.
                         ]),
                         bins=13)
test_histogram_bin_edges(np.array([0, 0, 0, 1, 2, 3, 3, 4, 5]),
                         np.array([-1., -0.33333333, 0.33333333, 1.]),
                         bins=3,
                         range=(-1, 1))
test_histogram_bin_edges(np.array([0, 0, 1, 2, 3, 5]),
                         np.array([-5., 0., 5., 10.]),
                         bins=3,
                         range=(-5, 10),
                         weights=[8, 3, 1, 5, -9, 2])

test_histogram_bin_edges(np.empty(0, float), np.array([0., 1.]), bins='doane')
test_histogram_bin_edges([1], np.array([0.5, 1.5]), bins='doane')
test_histogram_bin_edges([1, 100], np.array([1., 100.]), bins='doane')
test_histogram_bin_edges(np.array([0, 0, 0, 1, 2, 3, 3, 4, 5]),
                         np.array([0., 1., 2., 3., 4., 5.]),
                         bins='doane')
test_histogram_bin_edges(
    np.array([0, 0, 0, 1, 2, 3, 3, 4, 5]),
    np.array([-1., -0.8, -0.6, -0.4, -0.2, 0., 0.2, 0.4, 0.6, 0.8, 1.]),
    bins='doane',
    range=(-1, 1))

test_histogram_bin_edges(np.empty(0, float), np.array([0., 1.]), bins='rice')
test_histogram_bin_edges([1], np.array([0.5, 1.5]), bins='rice')
test_histogram_bin_edges([1, 100], np.array([1., 34., 67., 100.]), bins='rice')
test_histogram_bin_edges(np.array([0, 0, 0, 1, 2, 3, 3, 4, 5]),
                         np.array([0., 1., 2., 3., 4., 5.]),
                         bins='rice')
test_histogram_bin_edges(np.array([0, 0, 0, 1, 2, 3, 3, 4, 5]),
                         np.array([
                             -1., -0.71428571, -0.42857143, -0.14285714,
                             0.14285714, 0.42857143, 0.71428571, 1.
                         ]),
                         bins='rice',
                         range=(-1, 1))

test_histogram_bin_edges(np.empty(0, float), np.array([0., 1.]), bins='scott')
test_histogram_bin_edges([7], np.array([6.5, 7.5]), bins='scott')
test_histogram_bin_edges([1, 100], np.array([1., 100.]), bins='scott')
test_histogram_bin_edges(np.array([0, 0, 0, 1, 2, 3, 3, 4, 5]),
                         np.array([0., 2.5, 5.]),
                         bins='scott')
test_histogram_bin_edges(np.array([0, 0, 0, 1, 2, 3, 3, 4, 5]),
                         np.array([-1., -0.33333333, 0.33333333, 1.]),
                         bins='scott',
                         range=(-1, 1))

test_histogram_bin_edges(np.empty(0, float), np.array([0., 1.]), bins='sqrt')
test_histogram_bin_edges([3], np.array([2.5, 3.5]), bins='sqrt')
test_histogram_bin_edges([1, 100], np.array([1., 50.5, 100.]), bins='sqrt')
test_histogram_bin_edges(np.array([0, 0, 0, 1, 2, 3, 3, 4, 5]),
                         np.array([0., 1.66666667, 3.33333333, 5.]),
                         bins='sqrt')
test_histogram_bin_edges(np.array([0, 0, 0, 1, 2, 3, 3, 4, 5]),
                         np.array([-1., -0.5, 0., 0.5, 1.]),
                         bins='sqrt',
                         range=(-1, 1))

test_histogram_bin_edges(np.empty(0, float),
                         np.array([0., 1.]),
                         bins='sturges')
test_histogram_bin_edges([3], np.array([2.5, 3.5]), bins='sturges')
test_histogram_bin_edges([1, 100], np.array([1., 50.5, 100.]), bins='sturges')
test_histogram_bin_edges(np.array([0, 0, 0, 1, 2, 3, 3, 4, 5]),
                         np.array([0., 1., 2., 3., 4., 5.]),
                         bins='sturges')
test_histogram_bin_edges(
    np.array([0, 0, 0, 1, 2, 3, 3, 4, 5]),
    np.array([-1., -0.66666667, -0.33333333, 0., 0.33333333, 0.66666667, 1.]),
    bins='sturges',
    range=(-1, 1))

test_histogram_bin_edges(np.empty(0, float), np.array([0., 1.]), bins='fd')
test_histogram_bin_edges([3], np.array([2.5, 3.5]), bins='fd')
test_histogram_bin_edges([1, 100], np.array([1., 50.5, 100.]), bins='fd')
test_histogram_bin_edges(np.array([0, 0, 0, 1, 2, 3, 3, 4, 5]),
                         np.array([0., 2.5, 5.]),
                         bins='fd')
test_histogram_bin_edges(
    np.array([0, 0, 0, 1, 2, 3, 3, 4, 5]),
    np.array([
        -12., -9.17857143, -6.35714286, -3.53571429, -0.71428571, 2.10714286,
        4.92857143, 7.75, 10.57142857, 13.39285714, 16.21428571, 19.03571429,
        21.85714286, 24.67857143, 27.5, 30.32142857, 33.14285714, 35.96428571,
        38.78571429, 41.60714286, 44.42857143, 47.25, 50.07142857, 52.89285714,
        55.71428571, 58.53571429, 61.35714286, 64.17857143, 67.
    ]),
    bins='fd',
    range=(-12, 67))

test_histogram_bin_edges(np.empty(0, float), np.array([0., 1.]), bins='auto')
test_histogram_bin_edges([3], np.array([2.5, 3.5]), bins='auto')
test_histogram_bin_edges([1, 100], np.array([1., 50.5, 100.]), bins='auto')
test_histogram_bin_edges(np.array([0, 0, 0, 1, 2, 3, 3, 4, 5]),
                         np.array([0., 1., 2., 3., 4., 5.]),
                         bins='auto')
test_histogram_bin_edges(
    np.array([0, 0, 2, 3, 3, 4]),
    np.array([
        -43., -41.88888889, -40.77777778, -39.66666667, -38.55555556,
        -37.44444444, -36.33333333, -35.22222222, -34.11111111, -33.,
        -31.88888889, -30.77777778, -29.66666667, -28.55555556, -27.44444444,
        -26.33333333, -25.22222222, -24.11111111, -23., -21.88888889,
        -20.77777778, -19.66666667, -18.55555556, -17.44444444, -16.33333333,
        -15.22222222, -14.11111111, -13., -11.88888889, -10.77777778,
        -9.66666667, -8.55555556, -7.44444444, -6.33333333, -5.22222222,
        -4.11111111, -3., -1.88888889, -0.77777778, 0.33333333, 1.44444444,
        2.55555556, 3.66666667, 4.77777778, 5.88888889, 7., 8.11111111,
        9.22222222, 10.33333333, 11.44444444, 12.55555556, 13.66666667,
        14.77777778, 15.88888889, 17.
    ]),
    bins='auto',
    range=(-43, 17))

test_histogram_bin_edges(np.empty(0, float), np.array([0., 1.]), bins='stone')
test_histogram_bin_edges([3], np.array([2.5, 3.5]), bins='stone')
test_histogram_bin_edges([1, 100], np.array([1., 100.]), bins='stone')
test_histogram_bin_edges(np.array([0, 0, 1, 2, 3, 3, 4, 5]),
                         np.array([0., 5.]),
                         bins='stone')
test_histogram_bin_edges(
    np.array([0, 0, 2, 3, 3, 4]),
    np.array([
        -4., -3.43243243, -2.86486486, -2.2972973, -1.72972973, -1.16216216,
        -0.59459459, -0.02702703, 0.54054054, 1.10810811, 1.67567568,
        2.24324324, 2.81081081, 3.37837838, 3.94594595, 4.51351351, 5.08108108,
        5.64864865, 6.21621622, 6.78378378, 7.35135135, 7.91891892, 8.48648649,
        9.05405405, 9.62162162, 10.18918919, 10.75675676, 11.32432432,
        11.89189189, 12.45945946, 13.02702703, 13.59459459, 14.16216216,
        14.72972973, 15.2972973, 15.86486486, 16.43243243, 17.
    ]),
    bins='stone',
    range=(-4, 17))

@test
def test_histogram(a,
                   expected_hist,
                   expected_edges,
                   bins=10,
                   range=None,
                   density: Literal[bool] = False,
                   weights=None):
    hist, bin_edges = np.histogram(a,
                                   bins=bins,
                                   range=range,
                                   density=density,
                                   weights=weights)
    assert (np.round(hist, 8) == np.round(expected_hist, 8)).all()
    assert (np.round(bin_edges, 8) == np.round(expected_edges, 8)).all()

test_histogram(np.empty(0, float), np.array([0, 0, 0, 0, 0, 0, 0, 0, 0, 0]),
               np.array([0., 0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 1.]))
test_histogram([1, 2, 3, 4],
               np.array([3, 1]),
               np.array([0, 4, 7]),
               bins=[0, 4, 7],
               range=(-3, 3))
test_histogram([1, 2, 3, 4],
               np.array([-2.9, 12.]),
               np.array([0, 4, 7]),
               bins=[0, 4, 7],
               weights=[0.6, 0.5, -4, 12])
test_histogram([0, 1, 2, 3],
               np.array([1, 1, 1, 1]),
               np.array([0, 1, 2, 3, 4]),
               bins=[0, 1, 2, 3, 4])
test_histogram(
    [0], np.array([0, 0, 0, 0, 0, 1, 0, 0, 0, 0]),
    np.array([-0.5, -0.4, -0.3, -0.2, -0.1, 0., 0.1, 0.2, 0.3, 0.4, 0.5]))
test_histogram(np.linspace(0, 10, 100),
               np.array([10, 10, 10, 10, 10, 10, 10, 10, 10, 10]),
               np.array([0., 1., 2., 3., 4., 5., 6., 7., 8., 9., 10.]))
test_histogram([1, 2], np.array([2]), np.array([1., 2.]), bins=1)
test_histogram([1, 2, 3, 4, 5, 6, 7, 8],
               np.array([
                   0.17857143, 0.17857143, 0.17857143, 0., 0.17857143,
                   0.17857143, 0., 0.17857143, 0.17857143, 0.17857143
               ]),
               np.array([1., 1.7, 2.4, 3.1, 3.8, 4.5, 5.2, 5.9, 6.6, 7.3, 8.]),
               density=True)
test_histogram([1, 2, 3, 4, 5, 6, 7, 8],
               np.array([0., 0.125, 0.125, 0.09375]),
               np.array([0, 1, 3, 6, 10]),
               bins=[0, 1, 3, 6, 10],
               density=True)
test_histogram([1, 2, 3, 4, 5, 6, 7, 8],
               np.array([0, 2, 3, 3]),
               np.array([0, 1, 3, 6, 10]),
               bins=[0, 1, 3, 6, 10],
               density=False)
test_histogram([1, 2, 3, 4, 5, 6, 7, 8],
               np.array([0., 0.125, 0.125, 0.]),
               np.array([0, 1, 3, 6, np.inf]),
               bins=[0, 1, 3, 6, np.inf],
               density=True)
test_histogram([1, 2, 3, 4],
               np.array([0.25, 0.]),
               np.array([0.5, 1.5, np.inf]),
               bins=[0.5, 1.5, np.inf],
               density=True)
#Test outliers
test_histogram(np.arange(10) + .5,
               np.array([1, 1, 1, 1, 0, 1, 1, 1, 1, 1]),
               np.array([0., 0.9, 1.8, 2.7, 3.6, 4.5, 5.4, 6.3, 7.2, 8.1, 9.]),
               range=[0, 9])
test_histogram(np.arange(10) + .5,
               np.array([1, 1, 1, 1, 0, 1, 1, 1, 1, 1]),
               np.array([1., 1.9, 2.8, 3.7, 4.6, 5.5, 6.4, 7.3, 8.2, 9.1,
                         10.]),
               range=[1, 10])
test_histogram(np.arange(10) + .5,
               np.array([
                   0.12345679, 0.12345679, 0.12345679, 0.12345679, 0.,
                   0.12345679, 0.12345679, 0.12345679, 0.12345679, 0.12345679
               ]),
               np.array([0., 0.9, 1.8, 2.7, 3.6, 4.5, 5.4, 6.3, 7.2, 8.1, 9.]),
               range=[0, 9],
               density=True)
test_histogram(np.arange(10) + .5,
               np.array([
                   0.01371742, 0.04115226, 0.06858711, 0.09602195, 0.,
                   0.12345679, 0.15089163, 0.17832647, 0.20576132, 0.23319616
               ]),
               np.array([0., 0.9, 1.8, 2.7, 3.6, 4.5, 5.4, 6.3, 7.2, 8.1, 9.]),
               range=[0, 9],
               weights=np.arange(10) + .5,
               density=True)
test_histogram(np.arange(10) + .5,
               np.array([0.5, 1.5, 2.5, 3.5, 10., 6.5, 7.5, 8.5]),
               np.array([0., 1.125, 2.25, 3.375, 4.5, 5.625, 6.75, 7.875, 9.]),
               bins=8,
               range=[0, 9],
               weights=np.arange(10) + .5)
test_histogram(np.array([0., 0., 0., 1., 2., 3., 3., 4., 5.]),
               np.array([
                   0, 0, 3, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 2,
                   0, 0, 0, 0, 1, 0, 0, 0, 0, 1
               ]),
               np.array([
                   -0.5, -0.31666667, -0.13333333, 0.05, 0.23333333,
                   0.41666667, 0.6, 0.78333333, 0.96666667, 1.15, 1.33333333,
                   1.51666667, 1.7, 1.88333333, 2.06666667, 2.25, 2.43333333,
                   2.61666667, 2.8, 2.98333333, 3.16666667, 3.35, 3.53333333,
                   3.71666667, 3.9, 4.08333333, 4.26666667, 4.45, 4.63333333,
                   4.81666667, 5.
               ]),
               bins=30,
               range=(-0.5, 5))
test_histogram(np.linspace(0, 10, 10),
               np.array([0, 0, 0, 0, 0, 1, 1, 1, 1, 1]),
               np.array([0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10]),
               bins=np.arange(11),
               weights=[0, 0, 0, 0, 0, 1, 1, 1, 1, 1])
test_histogram(np.arange(9),
               np.array([0.2, 0.1, 0.1, 0.075]),
               np.array([0, 1, 3, 6, 10]),
               bins=[0, 1, 3, 6, 10],
               weights=[2, 1, 1, 1, 1, 1, 1, 1, 1],
               density=True)
test_histogram(np.array([1.3, 2.5, 2.3]),
               np.array([1. + 2.j, 1. + 3.j]),
               np.array([0, 2, 3]),
               bins=[0, 2, 3],
               weights=np.array([1. + 2.j, -1. + 1.j, 2. + 2.j]))
test_histogram(np.array([1.3, 2.5, 2.3]),
               np.array([1. + 2.j, 1. + 3.j]),
               np.array([1., 2., 3.]),
               bins=2,
               range=[1, 3],
               weights=np.array([1. + 2.j, -1. + 1.j, 2. + 2.j]))
test_histogram(
    np.array([1.3, 2.5, 2.3]),
    np.array([
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0
    ]),
    np.array([
        -10., -9.8, -9.6, -9.4, -9.2, -9., -8.8, -8.6, -8.4, -8.2, -8., -7.8,
        -7.6, -7.4, -7.2, -7., -6.8, -6.6, -6.4, -6.2, -6., -5.8, -5.6,
        -5.4, -5.2, -5., -4.8, -4.6, -4.4, -4.2, -4., -3.8, -3.6,
        -3.4, -3.2, -3., -2.8, -2.6, -2.4, -2.2, -2., -1.8, -1.6,
        -1.4, -1.2, -1., -0.8, -0.6, -0.4, -0.2, 0., 0.2, 0.4, 0.6, 0.8, 1.,
        1.2, 1.4, 1.6, 1.8, 2., 2.2, 2.4, 2.6, 2.8, 3., 3.2, 3.4, 3.6, 3.8, 4.,
        4.2, 4.4, 4.6, 4.8, 5., 5.2, 5.4, 5.6, 5.8, 6., 6.2, 6.4, 6.6, 6.8, 7.,
        7.2, 7.4, 7.6, 7.8, 8., 8.2, 8.4, 8.6, 8.8, 9., 9.2, 9.4, 9.6, 9.8, 10.
    ]),
    range=[-10, 10],
    bins=100)
test_histogram(np.array([0., 0., 0., 1., 2., 3., 3., 4., 5.]),
               np.array([
                   0, 0, 3, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 2,
                   0, 0, 0, 0, 1, 0, 0, 0, 0, 1
               ]),
               np.array([
                   -0.5, -0.31666667, -0.13333333, 0.05, 0.23333333,
                   0.41666667, 0.6, 0.78333333, 0.96666667, 1.15, 1.33333333,
                   1.51666667, 1.7, 1.88333333, 2.06666667, 2.25, 2.43333333,
                   2.61666667, 2.8, 2.98333333, 3.16666667, 3.35, 3.53333333,
                   3.71666667, 3.9, 4.08333333, 4.26666667, 4.45, 4.63333333,
                   4.81666667, 5.
               ]),
               bins=30,
               range=(-0.5, 5))
test_histogram(np.ones(100),
               np.array([100]),
               np.array([0.5, 1.5]),
               bins='scott')
test_histogram(np.ones(100), np.array([100]), np.array([0.5, 1.5]), bins='fd')
xcenter = np.linspace(-10, 10, 50)
a = np.hstack((np.linspace(-110, -100, 5), xcenter))
test_histogram(a,
               np.array([
                   3, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 8, 13,
                   15, 14
               ]),
               np.array([
                   -110., -104.28571429, -98.57142857, -92.85714286,
                   -87.14285714, -81.42857143, -75.71428571, -70.,
                   -64.28571429, -58.57142857, -52.85714286, -47.14285714,
                   -41.42857143, -35.71428571, -30., -24.28571429,
                   -18.57142857, -12.85714286, -7.14285714, -1.42857143,
                   4.28571429, 10.
               ]),
               bins='fd')
test_histogram(a,
               np.array([5, 0, 0, 0, 50]),
               np.array([-110., -86., -62., -38., -14., 10.]),
               bins='scott')
test_histogram(a,
               np.array([5, 0, 0, 0, 0, 0, 0, 0, 0, 23, 27]),
               np.array([
                   -110., -99.09090909, -88.18181818, -77.27272727,
                   -66.36363636, -55.45454545, -44.54545455, -33.63636364,
                   -22.72727273, -11.81818182, -0.90909091, 10.
               ]),
               bins='doane')
test_histogram(a,
               np.array([5, 0, 0, 0, 0, 50]),
               np.array([-110., -90., -70., -50., -30., -10., 10.]),
               bins='stone')

@test
def test_histogram_fast(dtype: type):
    data = np.array([1, 1, 2, 2, 3, 3, 4, 4, 4, 5], dtype=dtype)

    if dtype is float or dtype is np.float32 or dtype is np.float16:
        dt = dtype()
    else:
        dt = float()

    bins_dtype = type(dt)

    hist, bins = np.histogram(data)
    assert np.array_equal(hist, [2, 0, 2, 0, 0, 2, 0, 3, 0, 1])
    assert np.allclose(bins, [1. , 1.4, 1.8, 2.2, 2.6, 3. , 3.4, 3.8, 4.2, 4.6, 5. ])
    assert hist.dtype is int
    assert bins.dtype is bins_dtype

    hist, bins = np.histogram(data, bins=1)
    assert np.array_equal(hist, [10])
    assert np.allclose(bins, [1., 5.])
    assert hist.dtype is int
    assert bins.dtype is bins_dtype

    hist, bins = np.histogram(data, range=(1.5, 3.5))
    assert np.array_equal(hist, [0, 0, 2, 0, 0, 0, 0, 2, 0, 0])
    assert np.allclose(bins, [1.5, 1.7, 1.9, 2.1, 2.3, 2.5, 2.7, 2.9, 3.1, 3.3, 3.5])
    assert hist.dtype is int
    assert bins.dtype is bins_dtype

    hist, bins = np.histogram(data, range=(5, 6))
    assert np.array_equal(hist, [1, 0, 0, 0, 0, 0, 0, 0, 0, 0])
    assert np.allclose(bins, [5. , 5.1, 5.2, 5.3, 5.4, 5.5, 5.6, 5.7, 5.8, 5.9, 6.])
    assert hist.dtype is int
    assert bins.dtype is bins_dtype

    hist, bins = np.histogram(data, range=(1.5, 3.5), bins=5)
    assert np.array_equal(hist, [0, 2, 0, 2, 0])
    assert np.allclose(bins, [1.5, 1.9, 2.3, 2.7, 3.1, 3.5])
    assert hist.dtype is int
    assert bins.dtype is bins_dtype

    hist, bins = np.histogram(data, bins=[-100., 2., 4., 4.2, 4.7, 5.0, 5.2])
    assert np.array_equal(hist, [2, 4, 3, 0, 0, 1])
    assert np.allclose(bins, [-100., 2., 4., 4.2, 4.7, 5., 5.2])
    assert hist.dtype is int
    assert bins.dtype is float

    bins = np.array([-100., 2., 4., 4.2, 4.7, 5.0, 5.2], np.float32)
    hist, bins = np.histogram(data, bins=bins)
    assert np.array_equal(hist, [2, 4, 3, 0, 0, 1])
    assert np.allclose(bins, [-100., 2., 4., 4.2, 4.7, 5., 5.2])
    assert hist.dtype is int
    assert bins.dtype is np.float32

    hist, bins = np.histogram(data, bins=[-100., 2., 4., 4.2, 4.7, 5.0, 5.2], range=(2, 4))
    assert np.array_equal(hist, [2, 4, 3, 0, 0, 1])
    assert np.allclose(bins, [-100., 2., 4., 4.2, 4.7, 5., 5.2])
    assert hist.dtype is int
    assert bins.dtype is float

    hist, bins = np.histogram(data, bins=[-100, 2, 4, 5, 100], range=(-1, 100))
    assert np.array_equal(hist, [2, 4, 3, 1])
    assert np.array_equal(bins, [-100,    2,    4,    5,  100])
    assert hist.dtype is int
    assert bins.dtype is int

    data = np.array([2, 2, 2], dtype=dtype)

    hist, bins = np.histogram(data)
    assert np.array_equal(hist, [0, 0, 0, 0, 0, 3, 0, 0, 0, 0])
    assert np.allclose(bins, [1.5, 1.6, 1.7, 1.8, 1.9, 2. , 2.1, 2.2, 2.3, 2.4, 2.5])
    assert hist.dtype is int
    assert bins.dtype is bins_dtype

    hist, bins = np.histogram(data, bins=5)
    assert np.array_equal(hist, [0, 0, 3, 0, 0])
    assert np.allclose(bins, [1.5, 1.7, 1.9, 2.1, 2.3, 2.5])
    assert hist.dtype is int
    assert bins.dtype is bins_dtype

    hist, bins = np.histogram(data, bins=5, range=(3, 4))
    assert np.array_equal(hist, [0, 0, 0, 0, 0])
    assert np.allclose(bins, [3. , 3.2, 3.4, 3.6, 3.8, 4.])
    assert hist.dtype is int
    assert bins.dtype is bins_dtype

    data = np.empty(0, dtype=dtype)

    hist, bins = np.histogram(data)
    assert np.array_equal(hist, [0, 0, 0, 0, 0, 0, 0, 0, 0, 0])
    assert np.allclose(bins, [0. , 0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 1.])
    assert hist.dtype is int
    assert bins.dtype is bins_dtype

    hist, bins = np.histogram(data, bins=1)
    assert np.array_equal(hist, [0])
    assert np.allclose(bins, [0., 1.])
    assert hist.dtype is int
    assert bins.dtype is bins_dtype

    data = np.arange(81, dtype=dtype) ** 0.2
    data = data.reshape((3, 3, 3, 3))

    hist, bins = np.histogram(data)
    assert np.array_equal(hist, [1,  0,  0,  0,  2,  4,  7, 13, 21, 33])
    assert np.allclose(bins, [0.        , 0.24022489, 0.48044977, 0.72067466, 0.96089955,
                              1.20112443, 1.44134932, 1.68157421, 1.92179909, 2.16202398,
                              2.40224887])
    assert hist.dtype is int
    assert bins.dtype is bins_dtype

    hist, bins = np.histogram(data[::2,::2,::2,::2])
    assert np.array_equal(hist, [1, 0, 0, 0, 1, 1, 1, 4, 0, 8])
    assert np.allclose(bins, [0.        , 0.24022489, 0.48044977, 0.72067466, 0.96089955,
                              1.20112443, 1.44134932, 1.68157421, 1.92179909, 2.16202398,
                              2.40224887])
    assert hist.dtype is int
    assert bins.dtype is bins_dtype

    hist, bins = np.histogram(data[::2,::2,::2,::2], bins=[0, 0.2, 0.8, 3], range=(0, 2.1))
    assert np.array_equal(hist, [1, 0, 15])
    assert np.allclose(bins, [0., 0.2, 0.8, 3.])
    assert hist.dtype is int
    assert bins.dtype is float

test_histogram_fast(int)
test_histogram_fast(np.int8)
test_histogram_fast(np.float64)
test_histogram_fast(np.float32)
