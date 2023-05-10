import python

@test
def test_basic():
    from python import mymodule
    assert str(mymodule.multiply(3, 4)) == '12'
test_basic()

@test
def test_pybridge():
    @python
    def test_pydef(n: int) -> str:
        return ''.join(map(str, range(n)))
    assert test_pydef(5) == '01234'
test_pybridge()

from python import mymodule
@test
def test_conversions():
    T = tuple[dict[str,float],tuple[int,float]]
    t = mymodule.print_args(
        (4,5),
        {'a': 3.14, 'b': 2.123},
        True,
        {'ACGT'},
        [['abc'], ['1.1', '2.2'], list[str]()]
    )
    assert T.__from_py__(t.p) == ({'a': 3.14, 'b': 2.123}, (222, 3.14))
test_conversions()

@test
def test_pythrow():
    from python import mymodule.throw_exc() -> None as te
    try:
        te()
    except PyError as e:
        assert python.type(e.pytype)._getattr('__name__') + ":" + e.message == "ValueError:foo"
        return
    assert False
test_pythrow()

@test
def test_pyargs():
    from python import mymodule
    assert str(mymodule.print_args_var(1, 2, 3)) == "a=1, b=2, c=3, args=(), kwargs={}"
    assert str(mymodule.print_args_var(1, z=5, b=2)) == "a=1, b=2, c=1, args=(), kwargs={'z': 5}"
    assert str(mymodule.print_args_var(1, *(1,2,3,4,5), z=5)) == "a=1, b=1, c=2, args=(3, 4, 5), kwargs={'z': 5}"
test_pyargs()

@test
def test_roundtrip(x: T, T: type):
    assert T.__from_py__(x.__to_py__()) == x

test_roundtrip(42)
test_roundtrip(3.14)
test_roundtrip(True)
test_roundtrip(False)
test_roundtrip(byte(99))
test_roundtrip('hello world')
test_roundtrip('')
test_roundtrip(List[int]())
test_roundtrip([11, 22, 33])
test_roundtrip(Set[int]())
test_roundtrip({11, 22, 33})
test_roundtrip(Dict[str,int]())
test_roundtrip({'aa': 11, 'bb': 22, 'cc': 33})
test_roundtrip((11, 1.1, '11', [1, 1], {1}, {1: 1}))
test_roundtrip(())
test_roundtrip(DynamicTuple((111, 222, 333, 444)))
test_roundtrip(DynamicTuple('hello world'))
test_roundtrip(DynamicTuple[int]())
test_roundtrip(DynamicTuple[str]())
test_roundtrip(Optional(0))
test_roundtrip(Optional(111))
test_roundtrip(Optional[int]())
test_roundtrip(None)
test_roundtrip(...)

@test
def test_ops():
    from python import numpy as np
    a = np.array([1, 3])
    assert str(a + 1) == '[2 4]'
    assert str(a - 1) == '[0 2]'
    assert str(a * 2) == '[2 6]'
    assert str(a @ a) == '10'
    assert str(a // 2) == '[0 1]'
    assert str(a / 2) == '[0.5 1.5]'
    assert str(a % 2) == '[1 1]'
    assert str(a ** 2) == '[1 9]'
    assert str(-a) == '[-1 -3]'
    assert str(+a) == '[1 3]'
    assert str(~a) == '[-2 -4]'
    assert str(a << 1) == '[2 6]'
    assert str(a >> 1) == '[0 1]'
    assert str(a & 2) == '[0 2]'
    assert str(a ^ 2) == '[3 1]'
    assert str(a | 2) == '[3 3]'

    assert str(a < 3) == '[ True False]'
    assert str(a <= 3) == '[ True  True]'
    assert str(a == 3) == '[False  True]'
    assert str(a != 3) == '[ True False]'
    assert str(a > 3) == '[False False]'
    assert str(a >= 3) == '[False  True]'

    assert str(1 + a) == '[2 4]'
    assert str(1 - a) == '[ 0 -2]'
    assert str(2 * a) == '[2 6]'
    assert str(1 // a) == '[1 0]'
    assert str(2 & a) == '[0 2]'
    assert str(2 ^ a) == '[3 1]'
    assert str(2 | a) == '[3 3]'
    n = a[0]
    assert str(10 // n) == '10'
    assert str(10 / n) == '10.0'
    assert str(10 % n) == '0'
    assert str(10 ** n) == '10'
    assert str(4 << n) == '8'
    assert str(4 >> n) == '2'

    assert str(3 < a) == '[False False]'
    assert str(3 <= a) == '[False  True]'
    assert str(3 == a) == '[False  True]'
    assert str(3 != a) == '[ True False]'
    assert str(3 > a) == '[ True False]'
    assert str(3 >= a) == '[ True  True]'

    a = np.array([1, 3])
    b = a
    a += 1
    assert str(b) == '[2 4]'

    a = np.array([1, 3])
    b = a
    a -= 1
    assert str(b) == '[0 2]'

    a = np.array([1, 3])
    b = a
    a //= 2
    assert str(b) == '[0 1]'

    a = np.array([1.0, 3.0])
    b = a
    a /= 2
    assert str(b) == '[0.5 1.5]'

    a = np.array([1, 3])
    b = a
    a %= 2
    assert str(b) == '[1 1]'

    a = np.array([1, 3])
    b = a
    a **= 2
    assert str(b) == '[1 9]'

    a = np.array([1, 3])
    b = a
    a <<= 1
    assert str(b) == '[2 6]'

    a = np.array([1, 3])
    b = a
    a >>= 1
    assert str(b) == '[0 1]'

    a = np.array([1, 3])
    b = a
    a &= 2
    assert str(b) == '[0 2]'

    a = np.array([1, 3])
    b = a
    a ^= 2
    assert str(b) == '[3 1]'

    a = np.array([1, 3])
    b = a
    a |= 2
    assert str(b) == '[3 3]'

    a = np.array([1, 3])
    n = a[-1]
    assert str(n) == '3'
    assert int(n) == 3
    assert float(n) == 3.0
    from operator import index
    assert index(n) == 3
    assert hash(n) == 3
    assert len(a) == 2
    v = list(iter(a))
    assert len(v) == 2 and v[0] == 1 and v[1] == 3
    a[-1] = 99
    assert repr(a) == 'array([ 1, 99])'
    assert bool(a[0]) == True
    assert bool(a[0] - 1) == False

test_ops()
