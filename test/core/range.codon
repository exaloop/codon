# test adapted from https://github.com/python/cpython/blob/master/Lib/test/test_range.py

@test
def test_range():
    assert list(range(3)) == [0, 1, 2]
    assert list(range(1, 5)) == [1, 2, 3, 4]
    assert list(range(0)) == list[int]()
    assert list(range(-3)) == list[int]()
    assert list(range(1, 10, 3)) == [1, 4, 7]
    assert list(range(5, -5, -3)) == [5, 2, -1, -4]

    a = 10
    b = 100
    c = 50

    assert list(range(a, a+2)) == [a, a+1]
    assert list(range(a+2, a, -1)) == [a+2, a+1]
    assert list(range(a+4, a, -2)) == [a+4, a+2]

    s = list(range(a, b, c))
    assert a in s
    assert b not in s
    assert len(s) == 2

    s = list(range(b, a, -c))
    assert b in s
    assert a not in s
    assert len(s) == 2

    s = list(range(-a, -b, -c))
    assert -a in s
    assert -b not in s
    assert len(s) == 2

    val_err = False
    try:
        range(1, 2, 0)
    except ValueError:
        val_err = True
    assert val_err

@test
def test_index():
    u = range(2)
    assert u.index(0) == 0
    assert u.index(1) == 1
    val_err = False
    try:
        u.index(2)
    except ValueError:
        val_err = True
    assert val_err

    u = range(-2, 3)
    assert u.count(0) == 1
    assert u.index(0) == 2

    a = range(-2, 3)
    assert a.index(0) == 2

    assert range(1, 10, 3).index(4) == 1
    assert range(1, -10, -3).index(-5) == 2

@test
def test_count():
    assert range(3).count(-1) == 0
    assert range(3).count(0) == 1
    assert range(3).count(1) == 1
    assert range(3).count(2) == 1
    assert range(3).count(3) == 0
    assert range(3).index(1) == 1
    assert range(7, 0, -2).count(5) == 1
    assert range(7, 0, -2).count(-2) == 0
    assert range(7, 0, -1).count(0) == 0
    assert range(7, 0, -1).count(1) == 1

@test
def test_repr():
    assert str(range(1)) == 'range(0, 1)'
    assert str(range(1, 2)) == 'range(1, 2)'
    assert str(range(1, 2, 3)) == 'range(1, 2, 3)'

@test
def test_strided_limits():
    r = range(0, 101, 2)
    assert 0 in r
    assert 1 not in r
    assert 2 in r
    assert 99 not in r
    assert 100 in r
    assert 101 not in r

    r = range(0, -20, -1)
    assert 0 in r
    assert -1 in r
    assert -19 in r
    assert -20 not in r

    r = range(0, -20, -2)
    assert -18 in r
    assert -19 not in r
    assert -20 not in r

@test
def test_empty():
    r = range(0)
    assert 0 not in r
    assert 1 not in r

    r = range(0, -10)
    assert 0 not in r
    assert -1 not in r
    assert 1 not in r

@test
def test_slice():
    r = range(10)
    assert list(r[:2]) == list(r)[:2]
    assert list(r[0:2]) == list(r)[0:2]
    assert list(r[0:20]) == list(r)[0:20]
    assert list(r[:20]) == list(r)[:20]
    assert list(r[1:2]) == list(r)[1:2]
    assert list(r[20:30]) == list(r)[20:30]
    assert list(r[-30:-20]) == list(r)[-30:-20]
    assert list(r[-1:100:2]) == list(r)[-1:100:2]
    assert list(r[0:-1]) == list(r)[0:-1]
    assert list(r[-1:-3:-1]) == list(r)[-1:-3:-1]

    r = range(0)
    assert list(r[:2]) == list(r)[:2]
    assert list(r[0:2]) == list(r)[0:2]
    assert list(r[0:20]) == list(r)[0:20]
    assert list(r[:20]) == list(r)[:20]
    assert list(r[1:2]) == list(r)[1:2]
    assert list(r[20:30]) == list(r)[20:30]
    assert list(r[-30:-20]) == list(r)[-30:-20]
    assert list(r[-1:100:2]) == list(r)[-1:100:2]
    assert list(r[0:-1]) == list(r)[0:-1]
    assert list(r[-1:-3:-1]) == list(r)[-1:-3:-1]

    r = range(1, 9, 3)
    assert list(r[:2]) == list(r)[:2]
    assert list(r[0:2]) == list(r)[0:2]
    assert list(r[0:20]) == list(r)[0:20]
    assert list(r[:20]) == list(r)[:20]
    assert list(r[1:2]) == list(r)[1:2]
    assert list(r[20:30]) == list(r)[20:30]
    assert list(r[-30:-20]) == list(r)[-30:-20]
    assert list(r[-1:100:2]) == list(r)[-1:100:2]
    assert list(r[0:-1]) == list(r)[0:-1]
    assert list(r[-1:-3:-1]) == list(r)[-1:-3:-1]

    r = range(8, 0, -3)
    assert list(r[:2]) == list(r)[:2]
    assert list(r[0:2]) == list(r)[0:2]
    assert list(r[0:20]) == list(r)[0:20]
    assert list(r[:20]) == list(r)[:20]
    assert list(r[1:2]) == list(r)[1:2]
    assert list(r[20:30]) == list(r)[20:30]
    assert list(r[-30:-20]) == list(r)[-30:-20]
    assert list(r[-1:100:2]) == list(r)[-1:100:2]
    assert list(r[0:-1]) == list(r)[0:-1]
    assert list(r[-1:-3:-1]) == list(r)[-1:-3:-1]

@test
def test_contains():
    r = range(10)
    assert 0 in r
    assert 1 in r
    assert 5 in r
    assert -1 not in r
    assert 10 not in r

    r = range(9, -1, -1)
    assert 0 in r
    assert 1 in r
    assert 5 in r
    assert -1 not in r
    assert 10 not in r

    r = range(0, 10, 2)
    assert 0 in r
    assert 1 not in r
    assert 5 not in r
    assert -1 not in r
    assert 10 not in r

    r = range(9, -1, -2)
    assert 0 not in r
    assert 1 in r
    assert 5 in r
    assert -1 not in r
    assert 10 not in r

@test
def test_reverse_iteration():
    for r in [range(10),
              range(0),
              range(1, 9, 3),
              range(8, 0, -3),
              ]:
        assert list(reversed(r)) == list(r)[::-1]

test_range()
test_index()
test_count()
test_repr()
test_strided_limits()
test_empty()
test_slice()
test_contains()
test_reverse_iteration()
