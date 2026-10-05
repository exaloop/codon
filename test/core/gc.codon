recorded = set[int]()
active_finalizers = 0
maximum_depth = 0
nested_collections = 0


class Finalized:
    index: int

    def __del__(self):
        global active_finalizers, maximum_depth, nested_collections
        active_finalizers += 1
        maximum_depth = max(maximum_depth, active_finalizers)
        recorded.add(self.index)
        if nested_collections == 0:
            nested_collections += 1
            from C import GC_gcollect()
            GC_gcollect()
        active_finalizers -= 1


@noinline
def discard_batch(start: int, count: int):
    values = [Finalized(start + index) for index in range(count)]
    assert len(values) == count


def collect_ready():
    from C import GC_gcollect()
    GC_gcollect()


@test
def test_allocating_finalizers():
    values = []
    for round_index in range(3):
        for index in range(100_000):
            values.append(Finalized(round_index * 100_000 + index))
        values.clear()
    assert len(recorded) > 0
    assert maximum_depth == 1
    assert nested_collections == 1


throwing_finalizers = 0


class UnprintableError(Exception):
    def __init__(self, message: str):
        super().__init__(message)

    def __str__(self):
        raise AssertionError("exception formatting must not call user code")


class ThrowingFinalizer:
    index: int

    def __del__(self):
        global throwing_finalizers
        throwing_finalizers += 1
        if throwing_finalizers == 1:
            raise ValueError("finalizer failure")
        elif throwing_finalizers == 2:
            raise BaseException("base finalizer failure")
        elif throwing_finalizers == 3:
            raise UnprintableError("stored finalizer failure")


@noinline
def discard_throwing_batch():
    values = [ThrowingFinalizer(index) for index in range(1000)]
    assert len(values) == 1000


@test
def test_throwing_finalizers():
    for batch in range(4):
        discard_throwing_batch()
        collect_ready()
    assert throwing_finalizers >= 3
    before = len(recorded)
    for batch in range(4):
        discard_batch(600_000 + batch * 1000, 1000)
        collect_ready()
    assert len(recorded) > before
    assert maximum_depth == 1
    assert nested_collections == 1


class ExplicitFinalizer:
    enabled: bool

    def __del__(self):
        if self.enabled:
            raise ValueError("explicit destructor failure")


@test
def test_explicit_destructor_call():
    value = ExplicitFinalizer(True)
    try:
        value.__del__()
        assert False
    except ValueError as error:
        assert error.message == "explicit destructor failure"
    finally:
        value.enabled = False


test_allocating_finalizers()
test_throwing_finalizers()
test_explicit_destructor_call()
print("finalizer regressions ok")
