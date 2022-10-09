import pickle
from copy import copy

@tuple
class MyType:
    a: i32
    b: str
    c: float

class A:
    a: int
    v: list[str]

    def __eq__(self: A, other: A):
        return self.a == other.a and self.v == other.v

    def __ne__(self: A, other: A):
        return not (self == other)

    def __hash__(self: A):
        return self.a

    def __copy__(self: A):
        return A(self.a, copy(self.v))

@test
def test_pickle[T](x: T):
    import gzip
    path = 'build/testjar.bin'
    jar = gzip.open(path, 'wb')
    pickle.dump(x, jar)
    jar.close()

    jar = gzopen(path, 'rb')
    y = pickle.load(jar, T)
    jar.close()

    assert x == y

@test
def test_non_atomic_list_pickle[T](x: list[list[T]]):
    import gzip
    copy = [copy(a) for a in x]
    path = 'build/testjar.bin'
    jar = gzip.open(path, 'wb')
    pickle.dump(x, jar)
    jar.close()

    for v in x:
        v.clear()

    jar = gzopen(path, 'rb')
    y = pickle.load(jar, list[list[T]])
    jar.close()

    assert y == copy

@test
def test_non_atomic_dict_pickle[T](x: dict[str, list[T]]):
    import gzip
    copy = {k: copy(v) for k,v in x.items()}
    path = 'build/testjar.bin'
    jar = gzip.open(path, 'wb')
    pickle.dump(x, jar)
    jar.close()

    for v in x.values():
        v.clear()

    jar = gzopen(path, 'rb')
    y = pickle.load(jar, dict[str, list[T]])
    jar.close()

    assert y == copy

@test
def test_non_atomic_set_pickle(x: set[A]):
    import gzip
    copy = {copy(a) for a in x}
    path = 'build/testjar.bin'
    jar = gzip.open(path, 'wb')
    pickle.dump(x, jar)
    jar.close()

    for a in x:
        a.v.clear()

    jar = gzopen(path, 'rb')
    y = pickle.load(jar, set[A])
    jar.close()

    assert y == copy

test_pickle(42)
test_pickle(3.14)
test_pickle(True)
test_pickle(byte(90))
test_pickle(UInt[123](123123123))
test_pickle(Int[123](-123123123))
test_pickle([11, 22, 33, 44])
test_pickle({11, 22, 33, 44})
test_pickle({11: 1.1, 22: 2.2, 33: 3.3, 44: 4.4})
test_pickle('hello world')
test_pickle('')
test_pickle(MyType(i32(-1001), 'xyz', 5.55))
test_pickle((A(1, ['x', 'abc', '1.1.1.1']), 42, A(1000, ['foo'])))
test_pickle(['ACGTAAGG', 'TATCTGTT'])
test_pickle(list[Int[8]]())
test_pickle({'ACGTAAGG', 'CATTTTTA'})
test_pickle({'ACGTAAGG'})
test_pickle({'ACGTAAGG', 'TTTTGGTT'})
test_pickle(set[Int[8]]())
test_pickle({'ACGTAAGG': 99, 'TTATTCTT': 42})
test_pickle(dict[Int[8],Int[8]]())
test_pickle({'ACGTAAGG': 'ACGTAAGG'})
test_pickle((42, 3.14, True, byte(90), 'ACGTAAGG', 'ACGTAAGG'))
test_pickle(DynamicTuple((111, 222, 333, 444)))
test_pickle(DynamicTuple('hello world'))
test_pickle(DynamicTuple[int]())
test_pickle(DynamicTuple[str]())
test_pickle({i32(42): [[{'ACG', 'ACGTAGCG', 'ACGTAGCG'}, {'ACG', 'ACGTAGCG', 'ACGTAGCG'}], list[set[str]](), [set[str]()], [{''}, {'', 'GCGC'}]]})

test_non_atomic_list_pickle([[3,2,1], [-1,-2,-3], [111,999,888,777], list[int]()])
test_non_atomic_dict_pickle({'first': [3,2,1], 'second': [-1,-2,-3], 'third': [111,999,888,777], 'fourth:': list[int]()})
test_non_atomic_set_pickle({A(42, ['fourty', 'two']), A(0, list[str]()), A(-99, ['negative', 'ninety', 'nine'])})
