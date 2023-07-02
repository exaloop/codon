import itertools
from itertools import *
import math


def lzip(*args):
    return list(zip(*args))


def onearg(x):
    "Test function of one argument"
    return 2 * x


def errfunc(*args):
    "Test function that raises an error"
    raise ValueError()


def gen3():
    "Non-restartable source sequence"
    for i in (0, 1, 2):
        yield i


def isEven(x):
    "Test predicate"
    return x % 2 == 0


def isOdd(x):
    "Test predicate"
    return x % 2 == 1


def tupleize(*args):
    return args


def irange(n):
    for i in range(n):
        yield i


def take(n, seq):
    "Convenience function for partially consuming a long of infinite iterable"
    return list(islice(seq, n))


def testR(r):
    return r[0]


def testR2(r):
    return r[2]


def underten(x):
    return x < 10


@test
def test_combinations():
    f = lambda x: x  # hack to get non-static argument

    assert list(itertools.combinations("ABCD", f(2))) == [
        ["A", "B"],
        ["A", "C"],
        ["A", "D"],
        ["B", "C"],
        ["B", "D"],
        ["C", "D"],
    ]
    test_intermediate = itertools.combinations("ABCD", f(2))
    next(test_intermediate)
    assert list(test_intermediate) == [
        ["A", "C"],
        ["A", "D"],
        ["B", "C"],
        ["B", "D"],
        ["C", "D"],
    ]
    assert list(itertools.combinations(range(4), f(3))) == [
        [0, 1, 2],
        [0, 1, 3],
        [0, 2, 3],
        [1, 2, 3],
    ]
    test_intermediate = itertools.combinations(range(4), f(3))
    next(test_intermediate)
    assert list(test_intermediate) == [[0, 1, 3], [0, 2, 3], [1, 2, 3]]

    assert list(itertools.combinations("ABCD", 2)) == [
        ("A", "B"),
        ("A", "C"),
        ("A", "D"),
        ("B", "C"),
        ("B", "D"),
        ("C", "D"),
    ]
    test_intermediate = itertools.combinations("ABCD", 2)
    next(test_intermediate)
    assert list(test_intermediate) == [
        ("A", "C"),
        ("A", "D"),
        ("B", "C"),
        ("B", "D"),
        ("C", "D"),
    ]
    assert list(itertools.combinations(range(4), 3)) == [
        (0, 1, 2),
        (0, 1, 3),
        (0, 2, 3),
        (1, 2, 3),
    ]
    test_intermediate = itertools.combinations(range(4), 3)
    next(test_intermediate)
    assert list(test_intermediate) == [(0, 1, 3), (0, 2, 3), (1, 2, 3)]


@test
def test_combinations_with_replacement():
    f = lambda x: x  # hack to get non-static argument

    assert list(itertools.combinations_with_replacement(range(3), f(3))) == [
        [0, 0, 0],
        [0, 0, 1],
        [0, 0, 2],
        [0, 1, 1],
        [0, 1, 2],
        [0, 2, 2],
        [1, 1, 1],
        [1, 1, 2],
        [1, 2, 2],
        [2, 2, 2],
    ]
    assert list(itertools.combinations_with_replacement("ABC", f(2))) == [
        ["A", "A"],
        ["A", "B"],
        ["A", "C"],
        ["B", "B"],
        ["B", "C"],
        ["C", "C"],
    ]
    test_intermediate = itertools.combinations_with_replacement("ABC", f(2))
    next(test_intermediate)
    assert list(test_intermediate) == [
        ["A", "B"],
        ["A", "C"],
        ["B", "B"],
        ["B", "C"],
        ["C", "C"],
    ]

    assert list(itertools.combinations_with_replacement(range(3), 3)) == [
        (0, 0, 0),
        (0, 0, 1),
        (0, 0, 2),
        (0, 1, 1),
        (0, 1, 2),
        (0, 2, 2),
        (1, 1, 1),
        (1, 1, 2),
        (1, 2, 2),
        (2, 2, 2),
    ]
    assert list(itertools.combinations_with_replacement("ABC", 2)) == [
        ("A", "A"),
        ("A", "B"),
        ("A", "C"),
        ("B", "B"),
        ("B", "C"),
        ("C", "C"),
    ]
    test_intermediate = itertools.combinations_with_replacement("ABC", 2)
    next(test_intermediate)
    assert list(test_intermediate) == [
        ("A", "B"),
        ("A", "C"),
        ("B", "B"),
        ("B", "C"),
        ("C", "C"),
    ]

@test
def test_islice():
    ra100 = range(100)
    ra = range(10)
    assert list(itertools.islice(iter("ABCDEFG"), 0, 2, 1)) == ["A", "B"]
    assert list(itertools.islice(iter(ra100), 10, 20, 3)) == [10, 13, 16, 19]
    assert list(itertools.islice(iter(ra100), 10, 3, 20)) == []
    assert list(itertools.islice(iter(ra100), 10, 20, 1)) == [
        10,
        11,
        12,
        13,
        14,
        15,
        16,
        17,
        18,
        19,
    ]
    assert list(itertools.islice(iter(ra100), 10, 10, 1)) == []
    assert list(itertools.islice(iter(ra100), 10, 3, 1)) == []
    # stop=len(iterable)
    assert list(itertools.islice(iter(ra), 0, 10, 1)) == [0, 1, 2, 3, 4, 5, 6, 7, 8, 9]
    assert list(itertools.islice(iter(ra), 2, 10, 1)) == [2, 3, 4, 5, 6, 7, 8, 9]
    assert list(itertools.islice(iter(ra), 1, 10, 2)) == [1, 3, 5, 7, 9]
    try:
        list(itertools.islice(iter(ra), -5, 10, 1))
        assert False
    except ValueError:
        pass


@test
def test_count():
    # infinite loop here
    assert take(3, itertools.count(3.25, 1.0)) == [3.25, 4.25, 5.25]
    assert take(3, zip("abc", itertools.count(3.25, 1.0))) == [
        ("a", 3.25),
        ("b", 4.25),
        ("c", 5.25),
    ]
    assert take(2, zip("abc", itertools.count(-1.0, 1.0))) == [("a", -1.0), ("b", 0.0)]
    assert take(2, zip("abc", itertools.count(-3.0, 1.0))) == [("a", -3.0), ("b", -2.0)]


@test
def test_repeat():
    assert list(itertools.repeat("a", 3)) == ["a", "a", "a"]
    assert list(itertools.repeat(1, 10)) == [1, 1, 1, 1, 1, 1, 1, 1, 1, 1]
    assert list(itertools.repeat("a", -1)) == []
    assert len(list(itertools.repeat("a", 0))) == 0


@test
def test_cycle():
    assert take(10, zip("zzzzzzzzzzz", itertools.cycle(iter("abc")))) == [
        ("z", "a"),
        ("z", "b"),
        ("z", "c"),
        ("z", "a"),
        ("z", "b"),
        ("z", "c"),
        ("z", "a"),
        ("z", "b"),
        ("z", "c"),
        ("z", "a"),
    ]
    assert take(10, zip("zzzzzzzzzzz", itertools.cycle(["a", "b"]))) == [
        ("z", "a"),
        ("z", "b"),
        ("z", "a"),
        ("z", "b"),
        ("z", "a"),
        ("z", "b"),
        ("z", "a"),
        ("z", "b"),
        ("z", "a"),
        ("z", "b"),
    ]


@test
def test_compress():
    assert list(itertools.compress("ABCDEF", [1, 0, 1, 0, 1, 1])) == [
        "A",
        "C",
        "E",
        "F",
    ]
    assert list(itertools.compress("ABCDEF", [1, 1, 1, 1, 1, 1])) == [
        "A",
        "B",
        "C",
        "D",
        "E",
        "F",
    ]
    assert list(itertools.compress("ABCDEF", [1, 0, 1])) == ["A", "C"]
    assert list(itertools.compress("ABC", [0, 1, 1, 1, 1, 1])) == ["B", "C"]


@test
def test_dropwhile():
    data = [1, 3, 5, 20, 2, 4, 6, 8]
    assert list(itertools.dropwhile(underten, data)) == [20, 2, 4, 6, 8]


@test
def test_takewhile():
    data = [1, 3, 5, 20, 2, 4, 6, 8]
    assert list(itertools.takewhile(underten, data)) == [1, 3, 5]


@test
def test_filterfalse():
    assert list(itertools.filterfalse(isEven, range(10))) == [1, 3, 5, 7, 9]
    assert list(itertools.filterfalse(lambda x: bool(x), [0, 1, 0, 2, 0])) == [0, 0, 0]


@test
def test_permutations():
    f = lambda x: x  # hack to get non-static argument

    assert list(itertools.permutations(range(3), f(2))) == [
        [0, 1],
        [0, 2],
        [1, 0],
        [1, 2],
        [2, 0],
        [2, 1],
    ]

    for n in range(3):
        values = [5 * x - 12 for x in range(n)]
        for r in range(n + 2):
            result = list(itertools.permutations(values, f(r)))
            if r > n:  # right number of perms
                assert len(result) == 0
            # factorial is not yet implemented in math
            # else: fact(n) / fact(n - r)

    assert list(itertools.permutations(range(3), 2)) == [
        (0, 1),
        (0, 2),
        (1, 0),
        (1, 2),
        (2, 0),
        (2, 1),
    ]

    for n in staticrange(3):
        values = [5 * x - 12 for x in range(n)]
        for r in staticrange(n + 2):
            result = list(itertools.permutations(values, r))
            if r > n:  # right number of perms
                assert len(result) == 0
            # factorial is not yet implemented in math
            # else: fact(n) / fact(n - r)


@test
def test_accumulate():
    # addition
    assert list(itertools.accumulate(range(10), int.__add__, initial=0)) == [
        0,
        0,
        1,
        3,
        6,
        10,
        15,
        21,
        28,
        36,
        45,
    ]
    assert list(itertools.accumulate([7], int.__add__, initial=0)) == [
        0,
        7,
    ]  # iterable of length 1
    assert list(itertools.accumulate(range(10), int.__add__)) == [
        0,
        1,
        3,
        6,
        10,
        15,
        21,
        28,
        36,
        45,
    ]
    assert list(itertools.accumulate([7], int.__add__)) == [7]  # iterable of length 1
    assert list(itertools.accumulate("abc", str.__add__, initial="")) == [
        "",
        "a",
        "ab",
        "abc",
    ]
    assert list(itertools.accumulate([""], str.__add__, initial=str(0))) == ["0", "0"]
    # multiply
    assert list(itertools.accumulate(range(10), int.__mul__, initial=0)) == [
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
    ]
    assert list(itertools.accumulate([1, 2, 3, 4, 5], int.__mul__, initial=1)) == [
        1,
        1,
        2,
        6,
        24,
        120,
    ]
    assert list(itertools.accumulate([7], int.__mul__)) == [7]
    # pass


@test
def test_chain():
    assert list(itertools.chain("abc", "def")) == ["a", "b", "c", "d", "e", "f"]
    assert list(itertools.chain("abc")) == ["a", "b", "c"]
    assert list(itertools.chain("a", "b", "c")) == ["a", "b", "c"]
    assert list(itertools.chain(["abc", "def"])) == ["abc", "def"]
    assert list(itertools.chain(["abc"])) == ["abc"]
    assert list(itertools.chain(["a", "b", "c"])) == ["a", "b", "c"]


@test
def test_starmap():
    assert list(itertools.starmap(math.pow, [(2.0, 5.0), (3.0, 2.0), (10.0, 3.0)])) == [
        32.0,
        9.0,
        1000.0,
    ]
    assert list(itertools.starmap(math.pow, [(0.0, 1.0), (1.0, 2.0), (2.0, 3.0)])) == [
        0.0 ** 1.0,
        1.0 ** 2.0,
        2.0 ** 3.0,
    ]


@test
def test_groupby():
    def key_str(s: str):
        return s

    assert list(itertools.groupby(iter("AAAABBBCCDAABBC"), key_str)) == [
        ("A", ["A", "A", "A", "A"]),
        ("B", ["B", "B", "B"]),
        ("C", ["C", "C"]),
        ("D", ["D"]),
        ("A", ["A", "A"]),
        ("B", ["B", "B"]),
        ("C", ["C"]),
    ]


@test
def test_zip_longest():
    assert list(itertools.zip_longest("ABCDE", "12345", fillvalue="-")) == [
        ("A", "1"),
        ("B", "2"),
        ("C", "3"),
        ("D", "4"),
        ("E", "5"),
    ]
    assert list(itertools.zip_longest("ABCDE", "123", fillvalue="-")) == [
        ("A", "1"),
        ("B", "2"),
        ("C", "3"),
        ("D", "-"),
        ("E", "-"),
    ]
    assert list(itertools.zip_longest("123", "ABCDE", fillvalue="-")) == [
        ("1", "A"),
        ("2", "B"),
        ("3", "C"),
        ("-", "D"),
        ("-", "E"),
    ]
    assert list(itertools.zip_longest("", "ABCDE", fillvalue="-")) == [
        ("-", "A"),
        ("-", "B"),
        ("-", "C"),
        ("-", "D"),
        ("-", "E"),
    ]
    assert list(itertools.zip_longest("ABCDE", "", fillvalue="-")) == [
        ("A", "-"),
        ("B", "-"),
        ("C", "-"),
        ("D", "-"),
        ("E", "-"),
    ]
    assert not list(itertools.zip_longest("", "", fillvalue="-"))


@test
def test_zip_test():
    assert list(zip()) == []
    assert list(zip((1, 2))) == [(1,), (2,)]
    assert list(zip([1, 2], ["a", "b"], (False, True))) == [
        (1, "a", False),
        (2, "b", True),
    ]


test_combinations()
test_combinations_with_replacement()
test_islice()
test_count()
test_repeat()
test_cycle()
test_compress()
test_dropwhile()
test_takewhile()
test_filterfalse()
test_permutations()
test_accumulate()
test_chain()
test_starmap()
test_groupby()
test_zip_longest()
test_zip_test()


# Updated tests lifted from CPython test suite


@test
def test_accumulate_from_cpython():
    assert list(accumulate(range(10))) == [0, 1, 3, 6, 10, 15, 21, 28, 36, 45]
    assert list(accumulate(iterable=range(10))) == [0, 1, 3, 6, 10, 15, 21, 28, 36, 45]
    assert list(accumulate("abc")) == ["a", "ab", "abc"]
    assert list(accumulate(List[float]())) == []
    assert list(accumulate([7])) == [7]

    s = [2, 8, 9, 5, 7, 0, 3, 4, 1, 6]
    assert list(accumulate(s, min)) == [2, 2, 2, 2, 2, 0, 0, 0, 0, 0]
    assert list(accumulate(s, max)) == [2, 8, 9, 9, 9, 9, 9, 9, 9, 9]
    mul = lambda a, b: a * b
    assert list(accumulate(s, mul)) == [2, 16, 144, 720, 5040, 0, 0, 0, 0, 0]

    # assert list(accumulate([10, 5, 1], initial=None)) == [10, 15, 16]
    assert list(accumulate([10, 5, 1], initial=100)) == [100, 110, 115, 116]
    assert list(accumulate([10, 5, 1], initial=100.5)) == [100.5, 110.5, 115.5, 116.5]
    assert list(accumulate(List[int](), initial=100)) == [100]


test_accumulate_from_cpython()


@test
def test_chain_from_cpython():
    assert list(chain("abc", "def")) == list("abcdef")
    assert list(chain("abc")) == list("abc")
    assert list(chain("")) == []
    assert list(take(4, chain("abc", "def"))) == list("abcd")


test_chain_from_cpython()


@test
def test_chain_from_iterable_from_cpython():
    assert list(chain.from_iterable(["abc", "def"])) == list("abcdef")
    assert list(chain.from_iterable(["abc"])) == list("abc")
    assert list(chain.from_iterable([""])) == []
    assert take(4, chain.from_iterable(["abc", "def"])) == list("abcd")


test_chain_from_iterable_from_cpython()


@test
def test_combinations_from_cpython():
    f = lambda x: x  # hack to get non-static argument
    from math import factorial as fact

    err = False
    try:
        list(combinations("abc", f(-2)))
        assert False
    except ValueError:
        err = True
    assert err

    assert list(combinations("abc", f(32))) == []  # r > n
    assert list(combinations("ABCD", f(2))) == [
        ["A", "B"],
        ["A", "C"],
        ["A", "D"],
        ["B", "C"],
        ["B", "D"],
        ["C", "D"],
    ]
    assert list(combinations(range(4), f(3))) == [
        [0, 1, 2],
        [0, 1, 3],
        [0, 2, 3],
        [1, 2, 3],
    ]

    for n in range(7):
        values = [5 * x - 12 for x in range(n)]
        for r in range(n + 2):
            result = list(combinations(values, f(r)))

            assert len(result) == (0 if r > n else fact(n) // fact(r) // fact(n - r))
            assert len(result) == len(set(result))  # no repeats
            # assert result == sorted(result)                     # lexicographic order
            for c in result:
                assert len(c) == r  # r-length combinations
                assert len(set(c)) == r  # no duplicate elements
                assert list(c) == sorted(c)  # keep original ordering
                assert all(e in values for e in c)  # elements taken from input iterable
                assert list(c) == [
                    e for e in values if e in c
                ]  # comb is a subsequence of the input iterable


    assert list(combinations("abc", 32)) == []  # r > n
    assert list(combinations("ABCD", 2)) == [
        ("A", "B"),
        ("A", "C"),
        ("A", "D"),
        ("B", "C"),
        ("B", "D"),
        ("C", "D"),
    ]
    assert list(combinations(range(4), 3)) == [
        (0, 1, 2),
        (0, 1, 3),
        (0, 2, 3),
        (1, 2, 3),
    ]

    for n in staticrange(7):
        values = [5 * x - 12 for x in range(n)]
        for r in staticrange(n + 2):
            result = list(combinations(values, r))

            assert len(result) == (0 if r > n else fact(n) // fact(r) // fact(n - r))
            assert len(result) == len(set(result))  # no repeats
            # assert result == sorted(result)                     # lexicographic order
            for c in result:
                assert len(c) == r  # r-length combinations
                assert len(set(c)) == r  # no duplicate elements
                assert list(c) == sorted(c)  # keep original ordering
                assert all(e in values for e in c)  # elements taken from input iterable
                assert list(c) == [
                    e for e in values if e in c
                ]  # comb is a subsequence of the input iterable

test_combinations_from_cpython()


@test
def test_combinations_with_replacement_from_cpython():
    f = lambda x: x  # hack to get non-static argument
    cwr = combinations_with_replacement
    err = False
    try:
        list(combinations_with_replacement("abc", f(-2)))
        assert False
    except ValueError:
        err = True
    assert err

    assert list(combinations_with_replacement("ABC", f(2))) == [
        ["A", "A"],
        ["A", "B"],
        ["A", "C"],
        ["B", "B"],
        ["B", "C"],
        ["C", "C"],
    ]

    def numcombs(n, r):
        from math import factorial as fact

        if not n:
            return 0 if r else 1
        return fact(n + r - 1) // fact(r) // fact(n - 1)

    for n in range(7):
        values = [5 * x - 12 for x in range(n)]
        for r in range(n + 2):
            result = list(combinations_with_replacement(values, r))
            regular_combs = list(combinations(values, r))

            assert len(result) == numcombs(n, r)
            assert len(result) == len(set(result))  # no repeats
            # assert result == sorted(result)                     # lexicographic order

            if n == 0 or r <= 1:
                assert result == regular_combs  # cases that should be identical
            else:
                assert set(result) >= set(regular_combs)

            for c in result:
                assert len(c) == r  # r-length combinations
                noruns = [k for k, v in groupby(c)]  # combo without consecutive repeats
                assert len(noruns) == len(
                    set(noruns)
                )  # no repeats other than consecutive
                assert list(c) == sorted(c)  # keep original ordering
                assert all(e in values for e in c)  # elements taken from input iterable
                assert noruns == [
                    e for e in values if e in c
                ]  # comb is a subsequence of the input iterable


    assert list(combinations_with_replacement("ABC", 2)) == [
        ("A", "A"),
        ("A", "B"),
        ("A", "C"),
        ("B", "B"),
        ("B", "C"),
        ("C", "C"),
    ]

    for n in staticrange(7):
        values = [5 * x - 12 for x in range(n)]
        for r in staticrange(n + 2):
            result = list(combinations_with_replacement(values, r))
            regular_combs = list(combinations(values, r))

            assert len(result) == numcombs(n, r)
            assert len(result) == len(set(result))  # no repeats
            # assert result == sorted(result)                     # lexicographic order

            if n == 0 or r <= 1:
                assert result == regular_combs  # cases that should be identical
            else:
                assert set(result) >= set(regular_combs)

            for c in result:
                assert len(c) == r  # r-length combinations
                noruns = [k for k, v in groupby(c)]  # combo without consecutive repeats
                assert len(noruns) == len(
                    set(noruns)
                )  # no repeats other than consecutive
                assert list(c) == sorted(c)  # keep original ordering
                assert all(e in values for e in c)  # elements taken from input iterable
                assert noruns == [
                    e for e in values if e in c
                ]  # comb is a subsequence of the input iterable


test_combinations_with_replacement_from_cpython()


@test
def test_permutations_from_cpython():
    f = lambda x: x  # hack to get non-static argument
    from math import factorial as fact

    err = False
    try:
        list(permutations("abc", f(-2)))
        assert False
    except ValueError:
        err = True
    assert err

    assert list(permutations("abc", f(32))) == []
    assert list(permutations(range(3), f(2))) == [
        [0, 1],
        [0, 2],
        [1, 0],
        [1, 2],
        [2, 0],
        [2, 1],
    ]

    for n in range(7):
        values = [5 * x - 12 for x in range(n)]
        for r in range(n + 2):
            result = list(permutations(values, r))
            assert len(result) == (
                0 if r > n else fact(n) // fact(n - r)
            )  # right number of perms
            assert len(result) == len(set(result))  # no repeats
            # assert result == sorted(result)                # lexicographic order
            for p in result:
                assert len(p) == r  # r-length permutations
                assert len(set(p)) == r  # no duplicate elements
                assert all(e in values for e in p)  # elements taken from input iterable

            if r == n:
                assert result == list(permutations(values, None))  # test r as None
                assert result == list(permutations(values))  # test default r

    assert list(permutations("abc", 32)) == []
    assert list(permutations(range(3), 2)) == [
        (0, 1),
        (0, 2),
        (1, 0),
        (1, 2),
        (2, 0),
        (2, 1),
    ]

    for n in staticrange(7):
        values = [5 * x - 12 for x in range(n)]
        for r in staticrange(n + 2):
            result = list(permutations(values, r))
            assert len(result) == (
                0 if r > n else fact(n) // fact(n - r)
            )  # right number of perms
            assert len(result) == len(set(result))  # no repeats
            # assert result == sorted(result)                # lexicographic order
            for p in result:
                assert len(p) == r  # r-length permutations
                assert len(set(p)) == r  # no duplicate elements
                assert all(e in values for e in p)  # elements taken from input iterable

            if r == n:
                assert result == list(permutations(values, r))


test_permutations_from_cpython()


@extend
class List:
    def __lt__(self, other: List[T]):
        if len(self) != len(other):
            return len(self) < len(other)
        for a, b in zip(self, other):
            if a < b:
                return True
            if a > b:
                return False
        return False

    def __le__(self, other: List[T]):
        if len(self) != len(other):
            return len(self) < len(other)
        for a, b in zip(self, other):
            if a < b:
                return True
            if a > b:
                return False
        return True

    def __gt__(self, other: List[T]):
        if len(self) != len(other):
            return len(self) < len(other)
        for a, b in zip(self, other):
            if a > b:
                return True
            if a < b:
                return False
        return False

    def __ge__(self, other: List[T]):
        if len(self) != len(other):
            return len(self) < len(other)
        for a, b in zip(self, other):
            if a > b:
                return True
            if a < b:
                return False
        return True


@test
def test_combinatorics_from_cpython():
    # Test relationships between product(), permutations(),
    # combinations() and combinations_with_replacement().
    from math import factorial as fact

    for n in range(6):
        s = "ABCDEFG"[:n]
        for r in range(8):
            prod = list(product(s, repeat=r))
            cwr = list(combinations_with_replacement(s, r))
            perm = list(permutations(s, r))
            comb = list(combinations(s, r))

            # Check size
            assert len(prod) == n ** r
            assert len(cwr) == (
                (fact(n + r - 1) // fact(r) // fact(n - 1)) if n else (0 if r else 1)
            )
            assert len(perm) == (0 if r > n else fact(n) // fact(n - r))
            assert len(comb) == (0 if r > n else fact(n) // fact(r) // fact(n - r))

            # Check lexicographic order without repeated tuples
            assert prod == sorted(set(prod))
            assert cwr == sorted(set(cwr))
            assert perm == sorted(set(perm))
            assert comb == sorted(set(comb))

            # Check interrelationships
            assert cwr == [
                t for t in prod if sorted(t) == list(t)
            ]  # cwr: prods which are sorted
            assert perm == [
                t for t in prod if len(set(t)) == r
            ]  # perm: prods with no dups
            assert comb == [
                t for t in perm if sorted(t) == list(t)
            ]  # comb: perms that are sorted
            assert comb == [
                t for t in cwr if len(set(t)) == r
            ]  # comb: cwrs without dups
            assert comb == list(
                filter(set(cwr).__contains__, perm)
            )  # comb: perm that is a cwr
            assert comb == list(
                filter(set(perm).__contains__, cwr)
            )  # comb: cwr that is a perm
            assert comb == sorted(set(cwr) & set(perm))  # comb: both a cwr and a perm

    for n in staticrange(6):
        s = "ABCDEFG"[:n]
        for r in staticrange(8):
            prod = list(product(s, repeat=r))
            cwr = list(combinations_with_replacement(s, r))
            perm = list(permutations(s, r))
            comb = list(combinations(s, r))

            # Check size
            assert len(prod) == n ** r
            assert len(cwr) == (
                (fact(n + r - 1) // fact(r) // fact(n - 1)) if n else (0 if r else 1)
            )
            assert len(perm) == (0 if r > n else fact(n) // fact(n - r))
            assert len(comb) == (0 if r > n else fact(n) // fact(r) // fact(n - r))

            # Check lexicographic order without repeated tuples
            assert prod == sorted(set(prod))
            assert cwr == sorted(set(cwr))
            assert perm == sorted(set(perm))
            assert comb == sorted(set(comb))

            # Check interrelationships
            assert cwr == [
                t for t in prod if sorted(t) == list(t)
            ]  # cwr: prods which are sorted
            assert perm == [
                t for t in prod if len(set(t)) == r
            ]  # perm: prods with no dups
            assert comb == [
                t for t in perm if sorted(t) == list(t)
            ]  # comb: perms that are sorted
            assert comb == [
                t for t in cwr if len(set(t)) == r
            ]  # comb: cwrs without dups
            assert comb == list(
                filter(set(cwr).__contains__, perm)
            )  # comb: perm that is a cwr
            assert comb == list(
                filter(set(perm).__contains__, cwr)
            )  # comb: cwr that is a perm
            assert comb == sorted(set(cwr) & set(perm))  # comb: both a cwr and a perm


test_combinatorics_from_cpython()


@test
def test_compress_from_cpython():
    assert list(compress(data="ABCDEF", selectors=[1, 0, 1, 0, 1, 1])) == list("ACEF")
    assert list(compress("ABCDEF", [1, 0, 1, 0, 1, 1])) == list("ACEF")
    assert list(compress("ABCDEF", [0, 0, 0, 0, 0, 0])) == list("")
    assert list(compress("ABCDEF", [1, 1, 1, 1, 1, 1])) == list("ABCDEF")
    assert list(compress("ABCDEF", [1, 0, 1])) == list("AC")
    assert list(compress("ABC", [0, 1, 1, 1, 1, 1])) == list("BC")
    n = 10000
    data = chain.from_iterable(repeat(range(6), n))
    selectors = chain.from_iterable(repeat((0, 1)))
    assert list(compress(data, selectors)) == [1, 3, 5] * n


test_compress_from_cpython()


@test
def test_count_from_cpython():
    assert lzip("abc", count()) == [("a", 0), ("b", 1), ("c", 2)]
    assert lzip("abc", count(3)) == [("a", 3), ("b", 4), ("c", 5)]
    assert take(2, lzip("abc", count(3))) == [("a", 3), ("b", 4)]
    assert take(2, zip("abc", count(-1))) == [("a", -1), ("b", 0)]
    assert take(2, zip("abc", count(-3))) == [("a", -3), ("b", -2)]
    assert take(3, count(3.25)) == [3.25, 4.25, 5.25]


test_count_from_cpython()


@test
def test_count_with_stride_from_cpython():
    assert lzip("abc", count(2, 3)) == [("a", 2), ("b", 5), ("c", 8)]
    assert lzip("abc", count(start=2, step=3)) == [("a", 2), ("b", 5), ("c", 8)]
    assert lzip("abc", count(step=-1)) == [("a", 0), ("b", -1), ("c", -2)]
    assert lzip("abc", count(2, 0)) == [("a", 2), ("b", 2), ("c", 2)]
    assert lzip("abc", count(2, 1)) == [("a", 2), ("b", 3), ("c", 4)]
    assert lzip("abc", count(2, 3)) == [("a", 2), ("b", 5), ("c", 8)]
    assert take(3, count(2.0, 1.25)) == [2.0, 3.25, 4.5]


test_count_with_stride_from_cpython()


@test
def test_cycle_from_cpython():
    assert take(10, cycle("abc")) == list("abcabcabca")
    assert list(cycle("")) == []
    assert list(islice(cycle(gen3()), 10)) == [0, 1, 2, 0, 1, 2, 0, 1, 2, 0]


test_cycle_from_cpython()


@test
def test_groupby_from_cpython():
    # Check whether it accepts arguments correctly
    assert [] == list(groupby(List[int]()))
    assert [] == list(groupby(List[int](), key=lambda a: a))
    # Check normal input
    if 1:
        s = [
            (0, 10, 20),
            (0, 11, 21),
            (0, 12, 21),
            (1, 13, 21),
            (1, 14, 22),
            (2, 15, 22),
            (3, 16, 23),
            (3, 17, 23),
        ]

        if 1:
            dup = []
            for k, g in groupby(s, lambda r: r[0]):
                for elem in g:
                    assert k == elem[0]
                    dup.append(elem)
            assert s == dup

        # Check nested case
        if 1:
            dup = []
            for k, g in groupby(s, testR):
                for ik, ig in groupby(g, testR2):
                    for elem in ig:
                        assert k == elem[0]
                        assert ik == elem[2]
                        dup.append(elem)
            assert s == dup

        # Check case where inner iterator is not used
        keys = [k for k, g in groupby(s, testR)]
        expectedkeys = set([r[0] for r in s])
        assert set(keys) == expectedkeys
        assert len(keys) == len(expectedkeys)

    if 1:
        # Exercise pipes and filters style
        s = "abracadabra"
        if 1:
            # sort s | uniq
            r = [k for k, g in groupby(sorted(s))]
            assert r == ["a", "b", "c", "d", "r"]
        if 1:
            # sort s | uniq -d
            r = [k for k, g in groupby(sorted(s)) if list(islice(g, 1, 2))]
            assert r == ["a", "b", "r"]
        if 1:
            # sort s | uniq -c
            r = [(len(list(g)), k) for k, g in groupby(sorted(s))]
            assert r == [(5, "a"), (2, "b"), (1, "c"), (1, "d"), (2, "r")]
        if 1:
            # sort s | uniq -c | sort -rn | head -3
            r = sorted(
                [(len(list(g)), k) for k, g in groupby(sorted(s))], reverse=True
            )[:3]
            assert r == [(5, "a"), (2, "r"), (2, "b")]


test_groupby_from_cpython()


@test
def test_filter_from_cpython():
    assert list(filter(isEven, range(6))) == [0, 2, 4]
    # assert list(filter(None, [0,1,0,2,0])) == [1,2]  # TODO
    assert list(filter(lambda x: bool(x), [0, 1, 0, 2, 0])) == [1, 2]
    assert take(4, filter(isEven, count())) == [0, 2, 4, 6]


test_filter_from_cpython()


@test
def test_filterfalse_from_cpython():
    assert list(filterfalse(isEven, range(6))) == [1, 3, 5]
    # assert list(filter(None, [0,1,0,2,0])) == [0,0,0]  # TODO
    assert list(filterfalse(lambda x: bool(x), [0, 1, 0, 2, 0])) == [0, 0, 0]
    assert take(4, filterfalse(isEven, count())) == [1, 3, 5, 7]


test_filterfalse_from_cpython()


@test
def test_zip_from_cpython():
    ans = [(x, y) for x, y in zip("abc", count())]
    assert ans == [("a", 0), ("b", 1), ("c", 2)]
    assert list(zip("abc", range(6))) == lzip("abc", range(6))
    assert list(zip("abcdef", range(3))) == lzip("abcdef", range(3))
    assert take(3, zip("abcdef", count())) == lzip("abcdef", range(3))
    assert list(zip("abcdef")) == lzip("abcdef")
    assert list(zip()) == lzip()
    assert [pair for pair in zip("abc", "def")] == lzip("abc", "def")


test_zip_from_cpython()


@test
def test_ziplongest_from_cpython():
    for args in (
        (range(1000), range(2000, 2100), range(3000, 3050)),
        (range(1000), range(0), range(3000, 3050), range(1200), range(1500)),
        (range(1000), range(0), range(3000, 3050), range(1200), range(1500), range(0)),
    ):
        target = [
            tuple(arg[i] if i < len(arg) else None for arg in args)
            for i in range(max(map(len, args)))
        ]
        assert str(list(zip_longest(*args))) == str(target)
        target2 = [
            [(-999 if e is None else e.__val__()) for e in t] for t in target
        ]  # Replace None fills with 'X'
        assert list(zip_longest(*args, fillvalue=-999)) == target2

    assert (
        str(list(zip_longest("abc", range(6))))
        == "[('a', 0), ('b', 1), ('c', 2), (None, 3), (None, 4), (None, 5)]"
    )
    assert (
        str(list(zip_longest(range(6), "abc")))
        == "[(0, 'a'), (1, 'b'), (2, 'c'), (3, None), (4, None), (5, None)]"
    )


test_ziplongest_from_cpython()


@test
def test_product_from_cpython():
    for args, result in (
        # ((), [()]),                     # zero iterables  # TODO
        (("ab",), [("a",), ("b",)]),  # one iterable
        (
            (range(2), range(3)),
            [(0, 0), (0, 1), (0, 2), (1, 0), (1, 1), (1, 2)],
        ),  # two iterables
        (
            (range(0), range(2), range(3)),
            List[Tuple[int, int, int]](),
        ),  # first iterable with zero length
        (
            (range(2), range(0), range(3)),
            List[Tuple[int, int, int]](),
        ),  # middle iterable with zero length
        (
            (range(2), range(3), range(0)),
            List[Tuple[int, int, int]](),
        ),  # last iterable with zero length
    ):
        assert list(product(*args)) == result

    assert (
        len(list(product(range(7), range(7), range(7), range(7), range(7), range(7))))
        == 7 ** 6
    )


test_product_from_cpython()


@test
def test_repeat_from_cpython():
    assert list(repeat(object="a", times=3)) == ["a", "a", "a"]
    assert lzip(range(3), repeat("a")) == [(0, "a"), (1, "a"), (2, "a")]
    assert list(repeat("a", 3)) == ["a", "a", "a"]
    assert take(3, repeat("a")) == ["a", "a", "a"]
    assert list(repeat("a", 0)) == []
    assert list(repeat("a", -3)) == []


test_repeat_from_cpython()


@test
def test_map_from_cpython():
    power = lambda a, b: a ** b
    assert list(map(power, range(3), range(1, 7))) == [0 ** 1, 1 ** 2, 2 ** 3]
    assert list(map(tupleize, "abc", range(5))) == [("a", 0), ("b", 1), ("c", 2)]
    assert list(map(tupleize, "abc", count())) == [("a", 0), ("b", 1), ("c", 2)]
    assert take(2, map(tupleize, "abc", count())) == [("a", 0), ("b", 1)]
    assert list(map(tupleize, List[int]())) == []


test_map_from_cpython()


@test
def test_starmap_from_cpython():
    power = lambda a, b: a ** b
    assert list(starmap(power, zip(range(3), range(1, 7)))) == [0 ** 1, 1 ** 2, 2 ** 3]
    assert take(3, starmap(power, zip(count(), count(1)))) == [0 ** 1, 1 ** 2, 2 ** 3]
    # assert list(starmap(tupleize, List[int]())) == []  # TODO
    assert list(starmap(power, [(4, 5)])) == [4 ** 5]


test_starmap_from_cpython()


@test
def test_islice_from_cpython():
    for args in (  # islice(args) should agree with range(args)
        (10, 20, 3),
        (10, 3, 20),
        (10, 20),
        (10, 10),
        (10, 3),
        (20,),
    ):
        assert list(islice(range(100), *args)) == list(range(*args))

    for args, tgtargs in (  # Stop when seqn is exhausted
        ((10, 110, 3), ((10, 100, 3))),
        ((10, 110), ((10, 100))),
        ((110,), (100,)),
    ):
        assert list(islice(range(100), *args)) == list(range(*tgtargs))

    # Test stop=None
    assert list(islice(range(10), None)) == list(range(10))
    assert list(islice(range(10), None, None)) == list(range(10))
    assert list(islice(range(10), None, None, None)) == list(range(10))
    assert list(islice(range(10), 2, None)) == list(range(2, 10))
    assert list(islice(range(10), 1, None, 2)) == list(range(1, 10, 2))


test_islice_from_cpython()


@test
def test_takewhile_from_cpython():
    data = [1, 3, 5, 20, 2, 4, 6, 8]
    assert list(takewhile(underten, data)) == [1, 3, 5]
    assert list(takewhile(underten, List[int]())) == []
    t = takewhile(lambda x: bool(x), [1, 1, 1, 0, 0, 0])
    assert list(t) == [1, 1, 1]


test_takewhile_from_cpython()


@test
def test_dropwhile_from_cpython():
    data = [1, 3, 5, 20, 2, 4, 6, 8]
    assert list(dropwhile(underten, data)) == [20, 2, 4, 6, 8]
    assert list(dropwhile(underten, List[int]())) == []


test_dropwhile_from_cpython()


@test
def test_tee_from_cpython():
    import random

    n = 200

    a, b = tee(List[int]())  # test empty iterator
    assert list(a) == []
    assert list(b) == []

    a, b = tee(irange(n))  # test 100% interleaved
    assert lzip(a, b) == lzip(range(n), range(n))

    a, b = tee(irange(n))  # test 0% interleaved
    assert list(a) == list(range(n))
    assert list(b) == list(range(n))

    a, b = tee(irange(n))  # test dealloc of leading iterator
    for i in range(100):
        assert next(a) == i
    assert list(b) == list(range(n))

    a, b = tee(irange(n))  # test dealloc of trailing iterator
    for i in range(100):
        assert next(a) == i
    assert list(a) == list(range(100, n))

    for j in range(5):  # test randomly interleaved
        order = [0] * n + [1] * n
        random.shuffle(order)
        lists = ([], [])
        its = tee(irange(n))
        for i in order:
            value = next(its[i])
            lists[i].append(value)
        assert lists[0] == list(range(n))
        assert lists[1] == list(range(n))

    # test long-lagged and multi-way split
    a, b, c = tee(range(2000), 3)
    for i in range(100):
        assert next(a) == i
    assert list(b) == list(range(2000))
    assert [next(c), next(c)] == list(range(2))
    assert list(a) == list(range(100, 2000))
    assert list(c) == list(range(2, 2000))


test_tee_from_cpython()
