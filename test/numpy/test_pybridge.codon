import numpy as np
import numpy.pybridge

def check_roundtrip(a):
    return (type(a).__from_py__(a.__to_py__()) == a).all()

@test
def test_pybridge():
    a = np.arange(9).reshape(3, 3)
    assert check_roundtrip(a)
    assert check_roundtrip(a.astype(float))
    assert check_roundtrip(a.astype(np.float32))
    assert check_roundtrip(a + a * 2j)
    assert check_roundtrip((a + a * 2j).astype(np.complex64))
    assert check_roundtrip(a.T)
    assert check_roundtrip(a[::2, ::2])
    assert check_roundtrip(np.array(['aa', 'bb', 'cc']))

test_pybridge()
