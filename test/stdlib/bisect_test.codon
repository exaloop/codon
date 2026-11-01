import bisect
import sys

@tuple
class MutatingSearch:
    values: List[int]

    def __lt__(self, value: int):
        self.values.clear()
        return False

    def __gt__(self, value: int):
        self.values.clear()
        return True


li = [1, 3, 4, 4, 4, 6, 7]
lst = [10, 20, 30, 40, 50]

str = ["a", "b", "b", "c", "d"]


@test
def bisect_left():
    assert bisect.bisect_left(li, 4, 0, len(li)) == 2
    assert bisect.bisect_left(li, 4) == 2
    assert bisect.bisect_left(str, "b", 0, len(str)) == 1
    assert bisect.bisect_left(lst, 25, 1, 3) == 2

    # precomputed cases
    assert bisect.bisect_left(List[int](), 1, 0, 0) == 0
    assert bisect.bisect_left([1], 0, 0, 1) == 0
    assert bisect.bisect_left([1], 1, 0, 1) == 0
    assert bisect.bisect_left([1], 2, 0, 1) == 1
    assert bisect.bisect_left([1, 1], 0, 0, 2) == 0
    assert bisect.bisect_left([1, 1], 1, 0, 2) == 0
    assert bisect.bisect_left([1, 1], 2, 0, 2) == 2
    assert bisect.bisect_left([1, 1, 1], 0, 0, 3) == 0
    assert bisect.bisect_left([1, 1, 1], 1, 0, 3) == 0
    assert bisect.bisect_left([1, 1, 1], 2, 0, 3) == 3
    assert bisect.bisect_left([1, 1, 1, 1], 0, 0, 4) == 0
    assert bisect.bisect_left([1, 1, 1, 1], 1, 0, 4) == 0
    assert bisect.bisect_left([1, 1, 1, 1], 2, 0, 4) == 4
    assert bisect.bisect_left([1, 2], 0, 0, 2) == 0
    assert bisect.bisect_left([1, 2], 1, 0, 2) == 0
    assert bisect.bisect_left([1, 2], 1.5, 0, 2) == 1
    assert bisect.bisect_left([1, 2], 2, 0, 2) == 1
    assert bisect.bisect_left([1, 2], 3, 0, 2) == 2
    assert bisect.bisect_left([1, 1, 2, 2], 0, 0, 4) == 0
    assert bisect.bisect_left([1, 1, 2, 2], 1, 0, 4) == 0
    assert bisect.bisect_left([1, 1, 2, 2], 1.5, 0, 4) == 2
    assert bisect.bisect_left([1, 1, 2, 2], 2, 0, 4) == 2
    assert bisect.bisect_left([1, 1, 2, 2], 3, 0, 4) == 4
    assert bisect.bisect_left([1, 2, 3], 0, 0, 3) == 0
    assert bisect.bisect_left([1, 2, 3], 1, 0, 3) == 0
    assert bisect.bisect_left([1, 2, 3], 1.5, 0, 3) == 1
    assert bisect.bisect_left([1, 2, 3], 2, 0, 3) == 1
    assert bisect.bisect_left([1, 2, 3], 2.5, 0, 3) == 2
    assert bisect.bisect_left([1, 2, 3], 3, 0, 3) == 2
    assert bisect.bisect_left([1, 2, 3], 4, 0, 3) == 3
    assert bisect.bisect_left([1, 2, 2, 3, 3, 3, 4, 4, 4, 4], 0, 0, 10) == 0
    assert bisect.bisect_left([1, 2, 2, 3, 3, 3, 4, 4, 4, 4], 1, 0, 10) == 0
    assert bisect.bisect_left([1, 2, 2, 3, 3, 3, 4, 4, 4, 4], 1.5, 0, 10) == 1
    assert bisect.bisect_left([1, 2, 2, 3, 3, 3, 4, 4, 4, 4], 2, 0, 10) == 1
    assert bisect.bisect_left([1, 2, 2, 3, 3, 3, 4, 4, 4, 4], 2.5, 0, 10) == 3
    assert bisect.bisect_left([1, 2, 2, 3, 3, 3, 4, 4, 4, 4], 3, 0, 10) == 3
    assert bisect.bisect_left([1, 2, 2, 3, 3, 3, 4, 4, 4, 4], 3.5, 0, 10) == 6
    assert bisect.bisect_left([1, 2, 2, 3, 3, 3, 4, 4, 4, 4], 4, 0, 10) == 6
    assert bisect.bisect_left([1, 2, 2, 3, 3, 3, 4, 4, 4, 4], 5, 0, 10) == 10


@test
def bisect_right():
    assert bisect.bisect_right(li, 4, 0, len(li)) == 5
    assert bisect.bisect_right(li, 4) == 5
    assert bisect.bisect_right(str, "b", 0, len(str)) == 3
    assert bisect.bisect_right(lst, 25, 1, 3) == 2

    # precomputed casesrightrt bisect.bisect_right(List[int](), 1, 0, 0) == 0
    assert bisect.bisect_right([1], 0, 0, 1) == 0
    assert bisect.bisect_right([1], 1, 0, 1) == 1
    assert bisect.bisect_right([1], 2, 0, 1) == 1
    assert bisect.bisect_right([1, 1], 0, 0, 2) == 0
    assert bisect.bisect_right([1, 1], 1, 0, 2) == 2
    assert bisect.bisect_right([1, 1], 2, 0, 2) == 2
    assert bisect.bisect_right([1, 1, 1], 0, 0, 3) == 0
    assert bisect.bisect_right([1, 1, 1], 1, 0, 3) == 3
    assert bisect.bisect_right([1, 1, 1], 2, 0, 3) == 3
    assert bisect.bisect_right([1, 1, 1, 1], 0, 0, 4) == 0
    assert bisect.bisect_right([1, 1, 1, 1], 1, 0, 4) == 4
    assert bisect.bisect_right([1, 1, 1, 1], 2, 0, 4) == 4
    assert bisect.bisect_right([1, 2], 0, 0, 2) == 0
    assert bisect.bisect_right([1, 2], 1, 0, 2) == 1
    assert bisect.bisect_right([1, 2], 1.5, 0, 2) == 1
    assert bisect.bisect_right([1, 2], 2, 0, 2) == 2
    assert bisect.bisect_right([1, 2], 3, 0, 2) == 2
    assert bisect.bisect_right([1, 1, 2, 2], 0, 0, 4) == 0
    assert bisect.bisect_right([1, 1, 2, 2], 1, 0, 4) == 2
    assert bisect.bisect_right([1, 1, 2, 2], 1.5, 0, 4) == 2
    assert bisect.bisect_right([1, 1, 2, 2], 2, 0, 4) == 4
    assert bisect.bisect_right([1, 1, 2, 2], 3, 0, 4) == 4
    assert bisect.bisect_right([1, 2, 3], 0, 0, 3) == 0
    assert bisect.bisect_right([1, 2, 3], 1, 0, 3) == 1
    assert bisect.bisect_right([1, 2, 3], 1.5, 0, 3) == 1
    assert bisect.bisect_right([1, 2, 3], 2, 0, 3) == 2
    assert bisect.bisect_right([1, 2, 3], 2.5, 0, 3) == 2
    assert bisect.bisect_right([1, 2, 3], 3, 0, 3) == 3
    assert bisect.bisect_right([1, 2, 3], 4, 0, 3) == 3
    assert bisect.bisect_right([1, 2, 2, 3, 3, 3, 4, 4, 4, 4], 0, 0, 10) == 0
    assert bisect.bisect_right([1, 2, 2, 3, 3, 3, 4, 4, 4, 4], 1, 0, 10) == 1
    assert bisect.bisect_right([1, 2, 2, 3, 3, 3, 4, 4, 4, 4], 1.5, 0, 10) == 1
    assert bisect.bisect_right([1, 2, 2, 3, 3, 3, 4, 4, 4, 4], 2, 0, 10) == 3
    assert bisect.bisect_right([1, 2, 2, 3, 3, 3, 4, 4, 4, 4], 2.5, 0, 10) == 3
    assert bisect.bisect_right([1, 2, 2, 3, 3, 3, 4, 4, 4, 4], 3, 0, 10) == 6
    assert bisect.bisect_right([1, 2, 2, 3, 3, 3, 4, 4, 4, 4], 3.5, 0, 10) == 6
    assert bisect.bisect_right([1, 2, 2, 3, 3, 3, 4, 4, 4, 4], 4, 0, 10) == 10
    assert bisect.bisect_right([1, 2, 2, 3, 3, 3, 4, 4, 4, 4], 5, 0, 10) == 10


@test
def bisect1():
    assert bisect.bisect(li, 4, 0, len(li)) == 5
    assert bisect.bisect(str, "b", 0, len(str)) == 3
    assert bisect.bisect(lst, 25, 1, 3) == 2

    # precomputed casesrightrt bisect.bisect_right(List[int](), 1, 0, 0) == 0
    assert bisect.bisect([1], 0, 0, 1) == 0
    assert bisect.bisect([1], 1, 0, 1) == 1
    assert bisect.bisect([1], 2, 0, 1) == 1
    assert bisect.bisect([1, 1], 0, 0, 2) == 0
    assert bisect.bisect([1, 1], 1, 0, 2) == 2
    assert bisect.bisect([1, 1], 2, 0, 2) == 2
    assert bisect.bisect([1, 1, 1], 0, 0, 3) == 0
    assert bisect.bisect([1, 1, 1], 1, 0, 3) == 3
    assert bisect.bisect([1, 1, 1], 2, 0, 3) == 3
    assert bisect.bisect([1, 1, 1, 1], 0, 0, 4) == 0
    assert bisect.bisect([1, 1, 1, 1], 1, 0, 4) == 4
    assert bisect.bisect([1, 1, 1, 1], 2, 0, 4) == 4
    assert bisect.bisect([1, 2], 0, 0, 2) == 0
    assert bisect.bisect([1, 2], 1, 0, 2) == 1
    assert bisect.bisect([1, 2], 1.5, 0, 2) == 1
    assert bisect.bisect([1, 2], 2, 0, 2) == 2
    assert bisect.bisect([1, 2], 3, 0, 2) == 2
    assert bisect.bisect([1, 1, 2, 2], 0, 0, 4) == 0
    assert bisect.bisect([1, 1, 2, 2], 1, 0, 4) == 2
    assert bisect.bisect([1, 1, 2, 2], 1.5, 0, 4) == 2
    assert bisect.bisect([1, 1, 2, 2], 2, 0, 4) == 4
    assert bisect.bisect([1, 1, 2, 2], 3, 0, 4) == 4
    assert bisect.bisect([1, 2, 3], 0, 0, 3) == 0
    assert bisect.bisect([1, 2, 3], 1, 0, 3) == 1
    assert bisect.bisect([1, 2, 3], 1.5, 0, 3) == 1
    assert bisect.bisect([1, 2, 3], 2, 0, 3) == 2
    assert bisect.bisect([1, 2, 3], 2.5, 0, 3) == 2
    assert bisect.bisect([1, 2, 3], 3, 0, 3) == 3
    assert bisect.bisect([1, 2, 3], 4, 0, 3) == 3
    assert bisect.bisect([1, 2, 2, 3, 3, 3, 4, 4, 4, 4], 0, 0, 10) == 0
    assert bisect.bisect([1, 2, 2, 3, 3, 3, 4, 4, 4, 4], 1, 0, 10) == 1
    assert bisect.bisect([1, 2, 2, 3, 3, 3, 4, 4, 4, 4], 1.5, 0, 10) == 1
    assert bisect.bisect([1, 2, 2, 3, 3, 3, 4, 4, 4, 4], 2, 0, 10) == 3
    assert bisect.bisect([1, 2, 2, 3, 3, 3, 4, 4, 4, 4], 2.5, 0, 10) == 3
    assert bisect.bisect([1, 2, 2, 3, 3, 3, 4, 4, 4, 4], 3, 0, 10) == 6
    assert bisect.bisect([1, 2, 2, 3, 3, 3, 4, 4, 4, 4], 3.5, 0, 10) == 6
    assert bisect.bisect([1, 2, 2, 3, 3, 3, 4, 4, 4, 4], 4, 0, 10) == 10
    assert bisect.bisect([1, 2, 2, 3, 3, 3, 4, 4, 4, 4], 5, 0, 10) == 10


@test
def insort_left():
    bisect.insort_left(li, 5)
    assert li == [1, 3, 4, 4, 4, 5, 6, 7]


@test
def insort_right():
    li = [1, 3, 4, 4, 4, 6, 7]
    bisect.insort_right(li, 0, 0, len(li))
    assert li == [0, 1, 3, 4, 4, 4, 6, 7]
    bisect.insort_right(li, 10)
    assert li == [0, 1, 3, 4, 4, 4, 6, 7, 10]


@test
def insort():
    li = [1, 3, 4, 4, 4, 6, 7]
    bisect.insort(li, 0, 0, len(li))
    assert li == [0, 1, 3, 4, 4, 4, 6, 7]
    bisect.insort(li, 10)
    assert li == [0, 1, 3, 4, 4, 4, 6, 7, 10]


@test
def bisect_key():
    records = [(1, 'a'), (3, 'b'), (3, 'c'), (7, 'd')]
    key = lambda record: record[0]
    assert bisect.bisect_left(records, 3, key=key) == 1
    assert bisect.bisect_right(records, 3, key=key) == 3
    assert bisect.bisect(a=records, x=3, lo=2, hi=3, key=key) == 3
    assert bisect.bisect_left([1, 3, 3, 7], 3, key=None) == 1
    bisect.insort_left(records, (3, 'left'), key=key)
    bisect.insort_right(records, (3, 'right'), key=key)
    bisect.insort(records, (3, 'alias'), key=key)
    assert records == [(1, 'a'), (3, 'left'), (3, 'b'), (3, 'c'),
                       (3, 'right'), (3, 'alias'), (7, 'd')]


@test
def bisect_bounds_and_mutation():
    def clear_key(value: int, values: List[int]):
        values.clear()
        return value

    def grow_key(value: int, values: List[int]):
        if len(values) == 3:
            values.extend([7] * 128)
        return value

    for search, expected in ((bisect.bisect_left, 2), (bisect.bisect_right, 3)):
        assert search([1, 3, 5], 5, hi=-1) == expected
        assert search([1, 3, 5], 5, hi=-2) == 0
        assert search([1, 3, 5], 5, lo=5, hi=3) == 5
        for lower, upper in ((0, 4), (sys.maxsize - 1, sys.maxsize)):
            try:
                search([1, 2], 3, lower, upper)
                assert False
            except IndexError as error:
                assert error.message == 'list index out of range'
        values = [1, 3, 5]
        try:
            search(values, 4, key=clear_key(values=values, ...))
            assert False
        except IndexError as error:
            assert error.message == 'list index out of range'
        values = [1, 3, 5]
        assert search(values, 5, key=grow_key(values=values, ...)) == expected
        assert len(values) == 131


@test
def bisect_primitives():
    def check(Element: type):
        values = [Element(value) for value in (1, 2, 2, 3)]
        query = Element(2)
        assert bisect.bisect_left(values, query) == sum(value < query for value in values)
        assert bisect.bisect_right(values, query) == sum(not (query < value) for value in values)
    check(bool)
    check(int)
    check(Int[17])
    check(UInt[17])
    check(float)
    check(float32)
    check(float16)
    check(bfloat16)
    check(float128)
    check(type(''))
    assert bisect.bisect_left([b'a', b'b', b'b'], b'b') == 1
    assert bisect.bisect_right([b'a', b'b', b'b'], b'b') == 3
    assert bisect.bisect_left([1, 2, 3], 2.5) == 2
    assert bisect.bisect_right([1.0, 2.0, 3.0], 2) == 2
    for search in (bisect.bisect_left, bisect.bisect_right):
        assert search([1, 2], 0, hi=3) == 0
        values = [1, 3, 5]
        try:
            search(values, MutatingSearch(values))
            assert False
        except IndexError as error:
            assert error.message == 'list index out of range'
        assert not values


bisect_primitives()
bisect_bounds_and_mutation()
bisect_key()
bisect_left()
bisect_right()
bisect1()
insort_left()
insort_right()
insort()
