import numpy as np
from numpy import *

@test
def test_bartlett():
    assert (round(np.bartlett(4), 8) == array([0., 0.66666667, 0.66666667,
                                               0.])).all()
    assert (np.bartlett(0) == np.empty(0)).all()
    assert (np.bartlett(-7) == np.empty(0)).all()
    assert (np.bartlett(1) == np.array([1.])).all()

#    assert (round(np.bartlett(4.5), 8) == np.array([0., 0.57142857, 0.85714286, 0.28571429])).all()

test_bartlett()

@test
def test_blackman():
    assert (np.blackman(-8) == np.empty(0)).all()
    assert (np.blackman(1) == np.array([1.])).all()
    x = np.blackman(3)
    assert x[0] == -1.3877787807814457e-17

#    a = np.blackman(6.6)
#    assert a[6] == 0.01939475019748911

test_blackman()

@test
def test_hamming():
    assert (np.hamming(-9) == np.empty(0)).all()
    assert (np.hamming(1) == np.array([1.])).all()
    x = np.hamming(12)
    assert np.isclose(x[1], 0.15302337489765672)
    assert np.isclose(x[6], 0.9813667678626689)
    # a = np.hamming(6.6)
    # assert np.isclose(a[0], 0.08)

test_hamming()

@test
def test_hanning():
    assert (np.hanning(-9) == np.empty(0)).all()
    assert (np.hanning(1) == np.array([1.])).all()
    x = np.hanning(12)
    assert np.isclose(x[1], 0.07937323358440945)
    assert np.isclose(x[6], 0.9797464868072487)
    # a = np.hanning(6.3)
    # assert np.isclose(a[4], 0.4851833360887201)

test_hanning()

@test
def test_kaiser():
    assert (np.kaiser(0, 0) == np.empty(0)).all()
    assert (np.kaiser(0, 3) == np.empty(0)).all()
    assert (np.kaiser(0, -4) == np.empty(0)).all()
    assert (np.kaiser(1, -3) == np.array([1.])).all()
    assert (np.kaiser(8, 0) == np.array([1.])).all()
    assert (np.kaiser(1, 3) == np.array([1.])).all()
    assert (np.kaiser(1, -3) == np.array([1.])).all()
    assert (np.kaiser(-1, 0) == np.empty(0)).all()
    assert (np.kaiser(-5, 7) == np.empty(0)).all()
    assert (np.kaiser(-8, -4) == np.empty(0)).all()
    x = np.kaiser(8, 4)
    assert x[3] == 0.9652241821181469
    y = np.kaiser(8, -5)
    assert y[7] == 0.036710892271286676
    assert (round(np.kaiser(4, 5), 8) == np.array(
        [0.03671089, 0.77532210, 0.77532210, 0.03671089])).all()
    assert (round(np.kaiser(4, 6), 8) == np.array(
        [0.01487334, 0.73189542, 0.73189542, 0.01487334])).all()

#    assert (round(np.kaiser(4, 8.6), 8) == np.array([0.00133251, 0.63041193, 0.63041193, 0.00133251])).all()

test_kaiser()
