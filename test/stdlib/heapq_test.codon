import heapq
from random import random, randrange


@test
def check_invariant(heap):
    # Check the heap invariant.
    for pos, item in enumerate(heap):
        if pos:  # pos 0 has no parent
            parentpos = (pos - 1) >> 1
            assert heap[parentpos] <= item


@test
def test_heapify():
    for size in list(range(30)) + [20000]:
        heap = [random() for dummy in range(size)]
        heapq.heapify(heap)
        check_invariant(heap)


@test
def test_naive_nbest():
    data = [randrange(2000) for i in range(1000)]
    heap = []
    for item in data:
        heapq.heappush(heap, item)
        if len(heap) > 10:
            heapq.heappop(heap)
    heap.sort()
    assert heap == sorted(data)[-10:]


def heapiter(heap):
    # An iterator returning a heap's elements, smallest-first.
    while heap:
        yield heapq.heappop(heap)


@test
def test_nbest():
    # Less-naive "N-best" algorithm, much faster (if len(data) is big
    # enough <wink>) than sorting all of data.  However, if we had a max
    # heap instead of a min heap, it could go faster still via
    # heapify'ing all of data (linear time), then doing 10 heappops
    # (10 log-time steps).
    data = [randrange(2000) for i in range(1000)]
    heap = data[:10]
    heapq.heapify(heap)
    for item in data[10:]:
        if item > heap[0]:  # this gets rarer the longer we run
            heapq.heapreplace(heap, item)
    assert list(heapiter(heap)) == sorted(data)[-10:]


@test
def test_nbest_with_pushpop():
    data = [randrange(2000) for i in range(1000)]
    heap = data[:10]
    heapq.heapify(heap)
    for item in data[10:]:
        heapq.heappushpop(heap, item)
    assert list(heapiter(heap)) == sorted(data)[-10:]
    assert heapq.heappushpop(List[str](), "x") == "x"


@test
def test_heappushpop():
    h = List[int]()
    x = heapq.heappushpop(h, 10)
    assert (h, x) == (List[int](), 10)

    h = [10]
    x = heapq.heappushpop(h, 9)
    assert (h, x) == ([10], 9)

    h = [10]
    x = heapq.heappushpop(h, 11)
    assert (h, x) == ([11], 10)


@test
def test_heappop_max():
    # _heapop_max has an optimization for one-item lists which isn't
    # covered in other tests, so test that case explicitly here
    h = [3, 2]
    assert heapq._heappop_max(h) == 3
    assert heapq._heappop_max(h) == 2


@test
def test_heapsort():
    # Exercise everything with repeated heapsort checks
    for trial in range(100):
        size = randrange(50)
        data = [randrange(25) for i in range(size)]
        heap = None
        if trial & 1:  # Half of the time, use heapify
            heap = data[:]
            heapq.heapify(heap)
        else:  # The rest of the time, use heappush
            heap = []
            for item in data:
                heapq.heappush(heap, item)
        heap_sorted = [heapq.heappop(heap) for i in range(size)]
        assert heap_sorted == sorted(data)


"""
def test_merge(self):
    inputs = []
    for i in range(randrange(25)):
        row = []
        for j in range(randrange(100)):
            tup = choice('ABC'), randrange(-500, 500)
            row.append(tup)
        inputs.append(row)

    for key in [None, itemgetter(0), itemgetter(1), itemgetter(1, 0)]:
        for reverse in [False, True]:
            seqs = []
            for seq in inputs:
                seqs.append(sorted(seq, key=key, reverse=reverse))
            self.assertEqual(sorted(chain(*inputs), key=key, reverse=reverse),
                             list(heapq.merge(*seqs, key=key, reverse=reverse)))
            self.assertEqual(list(heapq.merge()), [])

def test_empty_merges(self):
    # Merging two empty lists (with or without a key) should produce
    # another empty list.
    self.assertEqual(list(heapq.merge([], [])), [])
    self.assertEqual(list(heapq.merge([], [], key=lambda: 6)), [])

def test_merge_does_not_suppress_index_error(self):
    # Issue 19018: Heapq.merge suppresses IndexError from user generator
    def iterable():
        s = list(range(10))
        for i in range(20):
            yield s[i]       # IndexError when i > 10
    with self.assertRaises(IndexError):
        list(heapq.merge(iterable(), iterable()))

def test_merge_stability(self):
    class Int(int):
        pass
    inputs = [[], [], [], []]
    for i in range(20000):
        stream = randrange(4)
        x = randrange(500)
        obj = Int(x)
        obj.pair = (x, stream)
        inputs[stream].append(obj)
    for stream in inputs:
        stream.sort()
    result = [i.pair for i in heapq.merge(*inputs)]
    self.assertEqual(result, sorted(result))
"""


def mykey(x: Tuple[int, int]):
    return (x[0] * 547 % 2000, x[1] * 547 % 2000)


@test
def test_nsmallest():
    data = [(randrange(2000), i) for i in range(1000)]
    for n in (10,):
        assert list(heapq.nsmallest(n, data)) == sorted(data)[: min(n, len(data))]
        assert (
            list(heapq.nsmallest(n, data, key=mykey))
            == sorted(data, key=mykey)[: min(n, len(data))]
        )
        assert list(heapq.nsmallest(n, data)) == sorted(data)[: min(n, len(data))]


@test
def test_nlargest():
    data = [(randrange(2000), i) for i in range(1000)]
    for n in (0, 1, 2, 10, 100, 400, 999, 1000, 1100):
        assert list(heapq.nlargest(n, data)) == sorted(data)[::-1][: min(n, len(data))]
        assert (
            list(heapq.nlargest(n, data, key=mykey))
            == sorted(data, key=mykey)[::-1][: min(n, len(data))]
        )
        assert list(heapq.nlargest(n, data)) == sorted(data)[::-1][: min(n, len(data))]


@test
def test_comparison_operator():
    def hsort(data: List[float], T: type):
        data2 = [T(x) for x in data]
        heapq.heapify(data2)
        return [heapq.heappop(data2).x for i in range(len(data))]

    class LT:
        x: float

        def __init__(self, x: float):
            self.x = x

        def __lt__(self, other: LT):
            return self.x > other.x

    data = [random() for i in range(100)]
    target = list(reversed(sorted(data)))
    assert hsort(data, LT) == target


@test
def test_merge():
    assert [0, 1, 2, 3, 4, 5, 5, 7, 8, 10, 15, 20, 25] == list(heapq.merge([1, 3, 5, 7], [0, 2, 4, 8], [5, 10, 15, 20], List[int](), [25]))
    assert ['dog', 'cat', 'fish', 'horse', 'kangaroo'] == list(heapq.merge(['dog', 'horse'], ['cat', 'fish', 'kangaroo'], key=len))

    assert [25, 20, 15, 10, 8, 7, 5, 5, 4, 3, 2, 1, 0] == list(heapq.merge([7, 5, 3, 1], [8, 4, 2, 0], [20, 15, 10, 5], List[int](), [25], reverse=True))
    assert ['kangaroo', 'horse', 'fish', 'dog', 'cat'] == list(heapq.merge(['horse', 'dog'], ['kangaroo', 'fish', 'cat'], key=len, reverse=True))


@test
def test_max_heap_api():
    for size in range(40):
        values = [(index * 17) % 23 for index in range(size)]
        heap = values.copy()
        heapq.heapify_max(heap)
        assert [heapq.heappop_max(heap) for _ in range(size)] == sorted(values, reverse=True)
        for value in values:
            heapq.heappush_max(heap, value)
        assert [heapq.heappop_max(heap) for _ in range(size)] == sorted(values, reverse=True)
    heap = [10]
    assert heapq.heappushpop_max(heap, 11) == 11
    assert heap == [10]
    assert heapq.heappushpop_max(heap, 9) == 10
    assert heap == [9]
    assert heapq.heapreplace_max(heap, 12) == 9
    assert heap == [12]
    assert heapq.heappushpop_max(List[int](), 7) == 7
    for operation in (heapq.heappop, heapq.heappop_max):
        try:
            operation(List[int]())
            assert False
        except IndexError as error:
            assert error.message == 'index out of range'
    for operation in (heapq.heapreplace, heapq.heapreplace_max):
        try:
            operation(List[int](), 7)
            assert False
        except IndexError as error:
            assert error.message == 'index out of range'


@test
def test_key_semantics():
    def record_key(value: int, calls: List[int]):
        calls.append(value)
        return value

    values = [4, 1, 3, 2, 1]
    for select, expected in ((heapq.nsmallest, [1]), (heapq.nlargest, [4])):
        calls = []
        assert select(1, values, key=record_key(calls=calls, ...)) == expected
        assert calls == values
        assert select(-3, values, key=None) == List[int]()
        assert select(0, values, key=None) == List[int]()
        assert select(1, List[int](), key=None) == List[int]()
    assert heapq.nsmallest(n=2, iterable=values, key=None) == [1, 1]
    assert heapq.nlargest(n=2, iterable=values, key=None) == [4, 3]

    records = [(2, 'a'), (1, 'b'), (2, 'c'), (1, 'd')]
    for count in (1, 2, 3, 4, 6):
        assert heapq.nsmallest(count, records, key=lambda item: item[0]) == sorted(records, key=lambda item: item[0])[:count]
        assert heapq.nlargest(count, records, key=lambda item: item[0]) == sorted(records, key=lambda item: item[0], reverse=True)[:count]

    calls = []
    assert list(heapq.merge([1, 3], [2, 4], key=record_key(calls=calls, ...))) == [1, 2, 3, 4]
    assert calls == [1, 2, 3, 4]
    calls.clear()
    assert list(heapq.merge([1, 2, 3], key=record_key(calls=calls, ...))) == [1, 2, 3]
    assert calls == [1]
    assert list(heapq.merge([1, 3], [2, 4], key=None)) == [1, 2, 3, 4]
    assert not list(heapq.merge())
    assert not list(heapq.merge(List[int](), List[int](), key=None))
    left = [(1, 'a'), (2, 'b')]
    right = [(1, 'c'), (2, 'd')]
    assert list(heapq.merge(left, right, key=lambda item: item[0])) == [(1, 'a'), (1, 'c'), (2, 'b'), (2, 'd')]
    assert list(heapq.merge(left[::-1], right[::-1], key=lambda item: item[0], reverse=True)) == [(2, 'b'), (2, 'd'), (1, 'a'), (1, 'c')]


@tuple
class HeapMutationItem:
    value: int
    mutate: Callable[[], int]

    def __lt__(self, other):
        self.mutate()
        return self.value < other.value


@test
def test_heap_mutation():
    def clear_heap(heap: List[HeapMutationItem]) -> int:
        heap.clear()
        return 0

    def relocate_heap(heap: List[HeapMutationItem], changed: Ptr[bool]) -> int:
        if not changed[0]:
            changed[0] = True
            saved = heap.copy()
            heap.extend(saved * 128)
            heap.clear()
            heap.extend(saved)
        return 0

    for operation in (heapq.heapify, heapq.heapify_max, heapq.heappop, heapq.heappop_max):
        heap = List[HeapMutationItem]()
        mutate = clear_heap(heap, ...)
        heap.extend(HeapMutationItem(value, mutate) for value in (4, 3, 2, 1))
        try:
            operation(heap)
            assert False
        except RuntimeError as error:
            assert error.message == 'list changed size during iteration'
    for operation in (heapq.heappush, heapq.heappush_max, heapq.heapreplace, heapq.heapreplace_max):
        heap = List[HeapMutationItem]()
        mutate = clear_heap(heap, ...)
        heap.extend(HeapMutationItem(value, mutate) for value in (4, 3, 2, 1))
        try:
            operation(heap, HeapMutationItem(5, mutate))
            assert False
        except RuntimeError as error:
            assert error.message == 'list changed size during iteration'

    for operation, reverse in ((heapq.heappushpop, False), (heapq.heappushpop_max, True)):
        for replace in (False, True):
            heap = List[HeapMutationItem]()
            mutate = clear_heap(heap, ...)
            heap.append(HeapMutationItem(2, mutate))
            item = HeapMutationItem(1 if replace == reverse else 3, mutate)
            try:
                result = operation(heap, item)
                assert not replace
                assert result.value == item.value
            except IndexError as error:
                assert replace
                assert error.message == 'index out of range'
            assert not heap

    for initialize, pop, reverse in ((heapq.heapify, heapq.heappop, False), (heapq.heapify_max, heapq.heappop_max, True)):
        heap = List[HeapMutationItem]()
        changed = False
        mutate = relocate_heap(heap, __ptr__(changed), ...)
        values = [9, 1, 8, 2, 7, 3, 6, 4, 5]
        heap.extend(HeapMutationItem(value, mutate) for value in values)
        previous = heap._ptr
        initialize(heap)
        assert changed
        assert previous != heap._ptr
        assert [pop(heap).value for _ in range(len(heap))] == sorted(values, reverse=reverse)

    heap = [1, 3, 2]
    heapq.heapify_max(heap)
    heapq._heappush_max(heap, 4)
    assert heapq._heappushpop_max(heap, 5) == 5
    assert heapq.heappop_max(heap) == 4


@tuple
class MergeSource:
    values: List[int]
    events: List[int]

    def __iter__(self):
        self.events.append(-1)
        return self.items()

    def items(self):
        for value in self.values:
            self.events.append(value)
            yield value


@test
def test_merge_consumption():
    events = List[int]()
    source = MergeSource([1, 4, 7], events)
    merged = heapq.merge(source, (2, 5, 8), range(3, 10, 3))
    assert not events
    assert next(merged) == 1
    assert events == [-1, 1]
    assert next(merged) == 2
    assert events == [-1, 1, 4]
    assert list(merged) == [3, 4, 5, 6, 7, 8, 9]
    assert events == [-1, 1, 4, 7]

    def failing_key(value: int):
        raise ValueError('merge key')
        return value

    merged = heapq.merge([1], [2], key=failing_key)
    try:
        next(merged)
        assert False
    except ValueError as error:
        assert error.message == 'merge key'

    assert list(heapq.merge(List[int](), (1, 2), List[int]())) == [1, 2]
    assert not list(heapq.merge(key=None, reverse=True))


@test
def test_primitive_heaps():
    def check(Element: type):
        values = [Element(value) for value in (3, 1, 2, 1)]
        for initialize, push, pop, pushpop, reverse in (
            (heapq.heapify, heapq.heappush, heapq.heappop, heapq.heappushpop, False),
            (heapq.heapify_max, heapq.heappush_max, heapq.heappop_max, heapq.heappushpop_max, True),
        ):
            heap = values.copy()
            initialize(heap)
            assert [pop(heap) for _ in range(len(heap))] == sorted(values, reverse=reverse)
            for value in values:
                push(heap, value)
            ordered = sorted(values + [Element(2)], reverse=reverse)
            assert pushpop(heap, Element(2)) == ordered[0]
            assert [pop(heap) for _ in range(len(heap))] == ordered[1:]
    check(bool)
    check(int)
    check(Int[17])
    check(UInt[17])
    check(float)
    check(float32)
    check(float16)
    check(bfloat16)
    check(float128)
    check(str)
    values = [b'c', b'a', b'b', b'a']
    heapq.heapify(values)
    assert [heapq.heappop(values) for _ in range(len(values))] == [b'a', b'a', b'b', b'c']


test_primitive_heaps()
test_heap_mutation()
test_merge_consumption()
test_key_semantics()
test_max_heap_api()
test_heapify()
test_naive_nbest()
test_nsmallest()
test_nlargest()
test_nbest()
test_nbest_with_pushpop()
test_heappushpop()
test_heappop_max()
test_heapsort()
# test_comparison_operator()
test_merge()
