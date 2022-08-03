from typing import Dict, List, Tuple

import codon

@codon.convert
class Foo:
    __slots__ = 'a', 'b', 'c'

    def __init__(self, n):
        self.a = n
        self.b = n**2
        self.c = n**3

    def __eq__(self, other):
        return (self.a == other.a and
                self.b == other.b and
                self.c == other.c)

    def __hash__(self):
        return hash((self.a, self.b, self.c))

    @codon.jit
    def total(self):
        return self.a + self.b + self.c

def test_convertible():
    assert Foo(10).total() == 1110

def test_many():
    @codon.jit
    def is_prime(n):
        if n <= 1:
            return False
        for i in range(2, n):
            if n % i == 0:
                return False
        return True

    assert sum(1 for i in range(100000, 200000) if is_prime(i)) == 8392

def test_roundtrip():
    @codon.jit
    def roundtrip(x):
        return x

    for _ in range(5):
        assert roundtrip(None) == None
        assert roundtrip(42) == 42
        assert roundtrip(3.14) == 3.14
        assert roundtrip(False) == False
        assert roundtrip(True) == True
        assert roundtrip('hello') == 'hello'
        assert roundtrip('') == ''
        assert roundtrip(2+3j) == 2+3j
        assert roundtrip(slice(1,2,3)) == slice(1,2,3)
        assert roundtrip([11,22,33]) == [11,22,33]
        assert roundtrip([[[42]]]) == [[[42]]]
        assert roundtrip({11,22,33}) == {11,22,33}
        assert roundtrip({11: 'one', 22: 'two', 33: 'three'}) == {11: 'one', 22: 'two', 33: 'three'}
        assert roundtrip((11,22,33)) == (11,22,33)
        assert Foo(roundtrip(Foo(123))[0]) == Foo(123)
        assert roundtrip(roundtrip) is roundtrip

def test_return_type():
    @codon.jit
    def run() -> Tuple[int, str, float, List[int], Dict[str, int]]:
        return (1, "str", 2.45, [1, 2, 3], {"a": 1, "b": 2})

    r = run()
    assert type(r) == tuple
    assert type(r[0]) == int
    assert type(r[1]) == str
    assert type(r[2]) == float
    assert type(r[3]) == list
    assert len(r[3]) == 3
    assert type(r[3][0]) == int
    assert type(r[4]) == dict
    assert len(r[4].items()) == 2
    assert type(next(iter(r[4].keys()))) == str
    assert type(next(iter(r[4].values()))) == int

def test_param_types():
    @codon.jit
    def run(a: int, b: Tuple[int, int], c: List[int], d: Dict[str, int]) -> int:
        s = 0
        for v in [a, *b, *c, *d.values()]:
            s += v
        return s

    r = run(1, (2, 3), [4, 5, 6], dict(a=7, b=8, c=9))
    assert type(r) == int
    assert r == 45

def test_error_handling():
    @codon.jit
    def type_error():
        return 1 + '1'

    try:
        type_error()
    except codon.JITError:
        pass
    except:
        assert False
    else:
        assert False

test_convertible()
test_many()
test_roundtrip()
test_return_type()
test_param_types()
test_error_handling()


@codon.jit
def foo(y):
    return f"{y.__class__.__name__}; {y}"


@codon.jit(debug=True)
def foo2(y):
    return f"{y.__class__.__name__}; {y}"

class Foo:
    def __init__(self):
        self.x = 1

@codon.jit
def a(x):
    return x+1

def b(x, z):
    y = a(x)
    return y * z

@codon.jit(pyvars=['b'])
def c(x, y):
    n = b(x,y) ** a(1)
    return n

def test_cross_calls():
    assert foo([None, 1]) == "List[Optional[int]]; [None, 1]"
    assert foo([1, None, 1]) == "List[Optional[int]]; [1, None, 1]"
    assert foo([1, None, 1.2]) == "List[pyobj]; [1, None, 1.2]"
    assert foo({None: 1}) == "Dict[pyobj,int]; {None: 1}"
    assert foo2([None, Foo()]).startswith("List[pyobj]; [None, <__main__.Foo object at")

    assert a(3) == 4
    assert b(3, 4) == 16
    assert round(c(5, 6.1), 2) == 1339.56

test_cross_calls()
