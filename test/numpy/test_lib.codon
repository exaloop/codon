import numpy as np
import numpy.lib.stride_tricks as str_tricks
from numpy import *

@test
def test_as_strided(x, expected, shape=None, strides=None):
    view = str_tricks.as_strided(x, shape=shape, strides=strides)
    assert np.array(view == expected).all()

test_as_strided(np.array([1, 2, 3, 4, 5, 6, 7, 8, 9]), np.array([1, 3, 5, 7]),
                (4, ), (16, ))
test_as_strided(np.array([[1, 2, 3], [4, 5, 6], [7, 8, 9]]),
                np.array([[1, 3], [3, 5]]), (2, 2), (16, 16))

@test
def test_sliding_window_view(x, window_shape, expected, axis=None):
    view = str_tricks.sliding_window_view(x, window_shape, axis=axis)
    assert (view == expected).all()

test_sliding_window_view(
    np.arange(6), 3, np.array([[0, 1, 2], [1, 2, 3], [2, 3, 4], [3, 4, 5]]))
test_sliding_window_view(np.array([[0, 1, 2, 3], [10, 11, 12, 13],
                                   [20, 21, 22, 23]]),
                         3,
                         np.array([[[0, 10, 20], [1, 11, 21], [2, 12, 22],
                                    [3, 13, 23]]]),
                         axis=0)

@test
def test_unique(ar,
                expected,
                return_index: Literal[bool] = False,
                return_inverse: Literal[bool] = False,
                return_counts: Literal[bool] = False,
                axis=None,
                equal_nan: bool = True):
    if return_index or return_counts or return_inverse:
        u = np.unique(ar, return_index, return_inverse, return_counts, axis,
                      equal_nan)
        for i in range(len(expected)):
            assert (u[i] == expected[i]).all()
    else:
        assert (np.unique(ar, return_index, return_inverse, return_counts,
                          axis, equal_nan) == expected).all()

test_unique(empty(0, float), empty(0, float))
test_unique(1, np.array([1]))
test_unique([1, 1, 2, 2, 3, 3], np.array([1, 2, 3]))
test_unique(np.array([1.1, 2.4, 6.1, 4.13, 2.65, 3.22, 2.9]),
            np.array([1.1, 2.4, 2.65, 2.9, 3.22, 4.13, 6.1]))
test_unique(np.array([1, 1, 2, 2, 3, 3]), np.array([1, 2, 3]))
test_unique(np.array([[1, 1], [2, 3]]), np.array([1, 2, 3]))
test_unique(np.array([[1, 0, 0], [1, 0, 0], [2, 3, 4]]),
            np.array([[1, 0, 0], [2, 3, 4]]),
            axis=0)
test_unique(np.array([1, 2, 6, 4, 2, 3, 2]),
            (np.array([1, 2, 3, 4, 6]), np.array([0, 1, 5, 3, 2])),
            return_index=True)
test_unique(np.array([1, 2, 6, 4, 2, 3, 2]),
            (np.array([1, 2, 3, 4, 6]), np.array([0, 1, 4, 3, 1, 2, 1])),
            return_inverse=True)
#test_unique(np.array([1, 2, 6, 4, 2, 3, 2]), (np.array([1, 2, 3, 4, 6]), np.array([1, 3, 1, 1, 1])), return_counts=True)
#test_unique(np.array(['a', 'b', 'b', 'c', 'a']), array(['a', 'b', 'c']))

@test
def test_in1d(ar1,
              ar2,
              expected,
              assume_unique: bool = False,
              invert: bool = False,
              kind: Optional[str] = None):
    assert (np.in1d(ar1, ar2, assume_unique, invert, kind) == expected).all()

test_in1d(np.array([0, 1, 2, 5, 0]), 0,
          np.array([True, False, False, False, True]))
test_in1d(0, 0, np.array([True]))
test_in1d(0, [0], np.array([True]))
test_in1d([0, 1, 2, 5, 0], [0, 2], np.array([True, False, True, False, True]))
test_in1d(np.array([0, 1, 2, 5, 0]), [0, 2],
          np.array([True, False, True, False, True]))
test_in1d(np.array([[0, 1, 5], [2, 5, 0]]), [0, 5],
          np.array([True, False, True, False, True, True]))
test_in1d(np.array([0, 1, 2, 5, 0]), [0, 2],
          np.array([False, True, False, True, False]),
          invert=True)
test_in1d(np.array([0, 1, 2, 5, 0]), [0],
          array([True, False, False, False, True]))
test_in1d(np.array([0, 1, 2, 5, 0]), [0],
          array([True, False, False, False, True]),
          kind='sort')
test_in1d(np.array([0, 1, 2, 5, 0]), [0],
          array([True, False, False, False, True]),
          kind='table')

@test
def test_intersect1d(ar1,
                     ar2,
                     expected,
                     assume_unique: bool = False,
                     return_indices: Literal[bool] = False):
    if return_indices:
        u = np.intersect1d(ar1, ar2, assume_unique, return_indices)
        for i in range(len(expected)):
            assert (u[i] == expected[i]).all()
    else:
        assert (np.intersect1d(ar1, ar2, assume_unique,
                               return_indices) == expected).all()

test_intersect1d([1, 3, 4, 3], [3, 1, 2, 1], np.array([1, 3]))
test_intersect1d(
    np.array([1, 1, 2, 3, 4]),
    np.array([2, 1, 4, 6]),
    (np.array([1, 2, 4]), np.array([0, 2, 4]), np.array([1, 0, 2])),
    return_indices=True)
test_intersect1d(np.array([1, 1, 2, 3, 4]),
                 np.array([2, 1, 4, 6]),
                 np.array([1, 2, 4]),
                 assume_unique=True)

@test
def test_isin(element,
              test_elements,
              expected,
              assume_unique: bool = False,
              invert: bool = False,
              kind: Optional[str] = None):
    assert (np.isin(element, test_elements, assume_unique, invert,
                    kind) == expected).all()

test_isin(np.array([[0, 2], [4, 6]]), np.array([1, 2, 4, 8]),
          np.array([[False, True], [True, False]]))
test_isin([[0, 2], [4, 6]], [1, 2, 4, 8],
          np.array([[False, True], [True, False]]))
test_isin([[0, 2], [4, 6]], [1, 2, 4, 8],
          np.array([[True, False], [False, True]]),
          invert=True)
test_isin([[0, 2], [4, 6]], [1, 2, 4, 8],
          np.array([[False, True], [True, False]]),
          assume_unique=True)
test_isin([[0, 2], [4, 6]], [1, 2, 4, 8],
          np.array([[False, True], [True, False]]),
          kind='sort')
test_isin([[0, 2], [4, 6]], [1, 2, 4, 8],
          np.array([[False, True], [True, False]]),
          kind='table')

@test
def test_setdiff1d(ar1, ar2, expected, assume_unique: bool = False):
    assert (np.setdiff1d(ar1, ar2, assume_unique) == expected).all()

test_setdiff1d(np.array([1, 2, 3, 2, 4, 1]), np.array([3, 4, 5, 6]),
               np.array([1, 2]))
test_setdiff1d([1, 2, 3, 2, 4, 1], [3, 4, 5, 6], np.array([1, 2]))
test_setdiff1d([[0, 2], [4, 6]], np.array([2, 3, 5, 7, 5]), np.array([0, 4,
                                                                      6]))
test_setdiff1d(2, 1, np.array([2]))
test_setdiff1d(empty(0, float), [1, 2], empty(0, float))
test_setdiff1d(2, 2, empty(0, float))
test_setdiff1d(np.array([1, 2, 3, 2, 4, 1]),
               np.array([3, 4, 5, 6]),
               np.array([1, 1, 2, 2]),
               assume_unique=True)

@test
def test_setxor1d(ar1, ar2, expected, assume_unique: bool = False):
    assert (np.setxor1d(ar1, ar2, assume_unique) == expected).all()

test_setxor1d(np.array([1, 2, 3, 2, 4]), np.array([2, 3, 5, 7, 5]),
              np.array([1, 4, 5, 7]))
test_setxor1d([1, 2, 3, 2, 4], [2, 3, 5, 7, 5], np.array([1, 4, 5, 7]))
test_setxor1d([[0, 2], [4, 6]], np.array([2, 3, 5, 7, 5]),
              np.array([0, 3, 4, 5, 6, 7]))
test_setxor1d(2, 1, np.array([1, 2]))
test_setxor1d(empty(0, float), [1, 2], np.array([1., 2.]))
test_setxor1d(2, 2, empty(0, float))
test_setxor1d(np.array([1, 2, 3, 2, 4]),
              np.array([2, 3, 5, 7, 5]),
              np.array([1, 2, 4, 5, 5, 7]),
              assume_unique=True)

@test
def test_union1d(ar1, ar2, expected):
    assert (np.union1d(ar1, ar2) == expected).all()

test_union1d([-1, 0, 1], [-2, 0, 2], np.array([-2, -1, 0, 1, 2]))
test_union1d([[0, 2], [4, 6]], np.array([2, 3, 5, 7, 5]),
             array([0, 2, 3, 4, 5, 6, 7]))
test_union1d([[0, 2], [4, 6]], np.array([[2, 3, 5], [5, 7, 5]]),
             array([0, 2, 3, 4, 5, 6, 7]))
test_union1d(2, 1, np.array([1, 2]))
