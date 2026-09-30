def enumerate_values(count: int):
    for value in range(count):
        yield value * 3

@test
def test_enumerate_control_flow():
    result = []
    for index, value in enumerate(enumerate_values(5), -2):
        result.append((index, value))
        index = 99
        if value == 3:
            continue
        if value == 9:
            break
    else:
        assert False
    assert result == [(-2, 0), (-1, 3), (0, 6), (1, 9)]

    result = []
    for index, value in enumerate([3, 6, 9], 4):
        try:
            if value == 6:
                continue
            result.append((index, value))
        finally:
            result.append((index, -value))
    else:
        result.append((-1, -1))
    assert result == [(4, 3), (4, -3), (5, -6), (6, 9), (6, -9), (-1, -1)]
    assert index == 6 and value == 9

    for index, value in enumerate(enumerate_values(0), 100):
        assert False
    assert index == 6 and value == 9

    def first_value():
        for index, value in enumerate(enumerate_values(3), 8):
            return index + value
        return -1
    assert first_value() == 8

    result = []
    for index, value in enumerate(range(3), 9223372036854775807):
        result.append(index)
    assert result == [9223372036854775807, -9223372036854775808, -9223372036854775807]

test_enumerate_control_flow()

class EnumerateIterable:
    events: List[str]

    def __init__(self, events):
        self.events = events

    def __iter__(self, count: int = 2):
        self.events.append('iter')
        return enumerate_values(count)

@test
def test_enumerate_evaluation():
    def source(events):
        events.append('source')
        return EnumerateIterable(events)

    def start(events):
        events.append('start')
        return -7

    events = []
    result = []
    for index, value in enumerate(source(events), start(events)):
        result.append((index, value))
    assert events == ['source', 'start', 'iter']
    assert result == [(-7, 0), (-6, 3)]

    def fail(events):
        events.append('start')
        raise ValueError('start')
        return 0

    events = []
    try:
        for index, value in enumerate(source(events), fail(events)):
            assert False
        assert False
    except ValueError:
        pass
    assert events == ['source', 'start']

    def grow(values):
        values.append(30)
        return 5

    values = [10, 20]
    result = []
    for index, value in enumerate(values, grow(values)):
        result.append((index, value))
        values.append(99)
    assert result == [(5, 10), (6, 20), (7, 30)]

test_enumerate_evaluation()

@test
def test_enumerate_unpacking():
    result = []
    for index, (left, right) in enumerate([(2, 3), (4, 5)]):
        result.append((index, left + right))
    assert result == [(0, 5), (1, 9)]

    result = []
    for outer, left in enumerate([10, 20]):
        for inner, right in enumerate(enumerate_values(2), 3):
            result.append((outer, inner, left + right))
    assert result == [(0, 3, 10), (0, 4, 13), (1, 3, 20), (1, 4, 23)]

    result = []
    for outer, (inner, value) in enumerate(enumerate([4, 5], 8), 2):
        result.append((outer, inner, value))
    assert result == [(2, 8, 4), (3, 9, 5)]

    result = []
    for index, value in enumerate('abc', 2):
        result.append((index, value))
    assert result == [(2, 'a'), (3, 'b'), (4, 'c')]

    result = []
    for value, value in enumerate([4, 5]):
        result.append(value)
    assert result == [4, 5]

test_enumerate_unpacking()

@test
def test_enumerate_fallbacks():
    pairs = []
    for pair in enumerate([4, 5], 7):
        index = pair[0]
        value = pair[1]
        pairs.append(pair)
        assert index + value == pair[0] + pair[1]
    assert pairs == [(7, 4), (8, 5)]
    assert pair == (8, 5)

    iterator = enumerate(enumerate_values(2), 3)
    result = []
    for index, value in iterator:
        result.append((index, value))
    assert result == [(3, 0), (4, 3)]

test_enumerate_fallbacks()

@test
def test_enumerate_shadowing():
    def enumerate(values):
        return [(99, value) for value in values]

    result = []
    for index, value in enumerate([4, 5]):
        result.append((index, value))
    assert result == [(99, 4), (99, 5)]

test_enumerate_shadowing()

enumerate_global_pair = (-1, -1)

@noinline
def observe_enumerate_global_pair():
    return enumerate_global_pair

@test
def test_enumerate_global_target():
    global enumerate_global_pair
    observed = []
    for enumerate_global_pair in enumerate([10, 20], 3):
        index = enumerate_global_pair[0]
        value = enumerate_global_pair[1]
        observed.append(observe_enumerate_global_pair())
        assert observe_enumerate_global_pair() == (index, value)
    assert observed == [(3, 10), (4, 20)]
    assert observe_enumerate_global_pair() == (4, 20)

test_enumerate_global_target()

class EnumerateBase(object):
    events: List[str]

    def __init__(self, events: List[str]):
        self.events = events

    def __iter__(self) -> Generator[int]:
        self.events.append('base')
        yield 10

class EnumerateDerived(EnumerateBase):
    def __iter__(self) -> Generator[int]:
        self.events.append('derived')
        yield 20
        yield 30

@noinline
def enumerate_virtual_values(items: EnumerateBase, start: int):
    result = []
    for index, value in enumerate(items, start):
        result.append((index, value))
    return result

@test
def test_enumerate_virtual_iterator():
    def source(events):
        events.append('source')
        return EnumerateDerived(events)

    def start(events):
        events.append('start')
        return -3

    events = []
    assert enumerate_virtual_values(EnumerateBase(events), 2) == [(2, 10)]
    assert events == ['base']
    events.clear()
    assert enumerate_virtual_values(source(events), start(events)) == [(-3, 20), (-2, 30)]
    assert events == ['source', 'start', 'derived']

test_enumerate_virtual_iterator()
