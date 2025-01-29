import numpy as np

@test
def test_shares_memory():
    x = np.array([1, 2, 3, 4])
    assert np.shares_memory(x, x)
    assert not np.shares_memory(x, np.array([5, 6, 7]))
    assert np.shares_memory(x[::2], x)
    assert not np.shares_memory(x[::2], x[1::2])

    y = np.array([1])
    assert np.shares_memory(y, y)
    assert not np.shares_memory(y, y.copy())

test_shares_memory()

def check_may_share_memory_exact(a, b):
    got = np.may_share_memory(a, b, max_work=np.MAY_SHARE_EXACT)
    cond = np.may_share_memory(a, b) == np.may_share_memory(
        a, b, max_work=np.MAY_SHARE_BOUNDS)

    a.fill(0)
    b.fill(0)
    a.fill(1)
    exact = b.any()

    return cond and got == exact

@test
def test_may_share_memory_manual():
    # Manual test cases for may_share_memory
    import itertools

    # Base arrays
    xs0 = [
        np.zeros((13, 21, 23, 22), dtype=np.int8),
        np.zeros((13, 21, 23 * 2, 22), dtype=np.int8)[:, :, ::2, :]
    ]

    # Generate all negative stride combinations
    xs = []
    for x in xs0:
        for ss in itertools.product(
                *(([slice(None), slice(None, None, -1)], ) * 4)):
            xs.append(x[ss])

    for x in xs:
        # The default is a simple extent check
        assert np.may_share_memory(x[:, 0, :], x[:, 1, :])
        assert np.may_share_memory(x[:, 0, :], x[:, 1, :], max_work=None)

        # Exact checks
        assert check_may_share_memory_exact(x[:, 0, :], x[:, 1, :])
        assert check_may_share_memory_exact(x[:, ::7], x[:, 3::3])

        try:
            xp = x.ravel()
            if xp.flags.owndata:
                continue
            yp = xp.view(np.int16)
        except ValueError:
            continue

        # 0-size arrays cannot overlap
        assert check_may_share_memory_exact(x.ravel()[6:6],
                                            yp.reshape(13, 21, 23, 11)[:, ::7])

        # Test itemsize is dealt with
        assert check_may_share_memory_exact(x[:, ::7],
                                            yp.reshape(13, 21, 23, 11))
        assert check_may_share_memory_exact(
            x[:, ::7],
            yp.reshape(13, 21, 23, 11)[:, 3::3])
        assert check_may_share_memory_exact(x.ravel()[6:7],
                                            yp.reshape(13, 21, 23, 11)[:, ::7])

    # Check unit size
    x = np.zeros((1, ), dtype=np.int8)
    assert check_may_share_memory_exact(x, x)
    assert check_may_share_memory_exact(x, x.copy())

test_may_share_memory_manual()

@test
def test_shares_memory_api():
    x = np.zeros((4, 5, 6), dtype=np.int8)

    assert np.shares_memory(x, x)
    assert not np.shares_memory(x, x.copy())

    a = x[:, ::2, ::3]
    b = x[:, ::3, ::2]
    assert np.shares_memory(a, b)
    assert np.shares_memory(a, b, max_work=None)

test_shares_memory_api()

@test
def test_byte_bounds():
    a, b = np.byte_bounds(np.array(42))
    assert b - a == 8

    a, b = np.byte_bounds(np.array([1, 2, 3], np.int8))
    assert b - a == 3

    a, b = np.byte_bounds(np.ones((3, 3)))
    assert b - a == 72

    a, b = np.byte_bounds(np.ones((3, 3))[::-1, ::-1])
    assert b - a == 72

    a, b = np.byte_bounds(np.ones((3, 3))[1:1])
    assert b == a

test_byte_bounds()
