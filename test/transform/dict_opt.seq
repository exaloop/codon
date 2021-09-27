class DummyDict[K, V]:
    def __getitem__(self, k: K):
        raise ValueError('failed')
        return V()
    def get(self, k: K, d: V):
        raise ValueError('failed')
        return V()
    def __setitem__(self, k: K, v: V):
        assert False
    def __dict_do_op_throws__[F, Z](self, key: K, other: Z, op: F):
        pass
    def __dict_do_op__[F, Z](self, key: K, other: Z, dflt: V, op: F):
        pass

class WrappedDict[K, V]:
    d: Dict[K,V]
    do_op_throws_count: int
    do_op_count: int

    def __init__(self):
        self.d = {}
        self.do_op_throws_count = 0
        self.do_op_count = 0
    def __getitem__(self, k: K):
        return self.d.__getitem__(k)
    def get(self, k: K, d: V):
        return self.d.get(k, d)
    def __setitem__(self, k: K, v: V):
        self.d.__setitem__(k, v)
    def setdefault(self, k: K, v: V):
        return self.d.setdefault(k, v)

    def __dict_do_op_throws__[F, Z](self, key: K, other: Z, op: F):
        self.do_op_throws_count += 1
        self.d.__dict_do_op_throws__(key, other, op)
    def __dict_do_op__[F, Z](self, key: K, other: Z, dflt: V, op: F):
        self.do_op_count += 1
        self.d.__dict_do_op__(key, other, dflt, op)

@test
def test_dict_op():
    x = DummyDict[int, int]()
    x[1] = x[1] + 1
    x[1] = x.get(1, 1) + 1
test_dict_op()

@test
def test_dict_do_not_op():
    x = DummyDict[int, int]()
    try:
        x[1] = x[2] + 1
    except ValueError:
        return
    assert False
test_dict_do_not_op()

@test
def test_wrapped_dict():
    def my_op(a, b):
        return a * b
    def my_op_throws(a, b):
        raise ValueError('my_op_throws')
        return a * b

    x = WrappedDict[str, float]()
    x['a'] = x.get('a', 0.0) + 1.0        # invokes opt (do_op_count)
    x['a'] = x['a'] * 2                   # invokes opt (do_op_throws_count)
    x['b'] = x.setdefault('b', 4.5) + 99  # no opt (no getitem/get)
    x['a'] += x['b']                      # invokes opt (do_op_throws_count)
    x['b'] = my_op(x['b'], 2.0)           # no opt (not a int/float method)
    foo = x['a']
    x['a'] = x['b'] + foo                 # no opt (different keys)

    try:
        x['c'] += 1.0                     # invokes opt (do_op_throws_count)
        assert False
    except KeyError:
        pass

    try:
        x['d'] = my_op_throws(x['d'], 2.0)  # no opt (not a int/float method)
        assert False
    except KeyError:
        pass

    try:
        x['d'] = my_op_throws(x.get('d', 111.0), 2.0)  # no opt (not a int/float method)
        assert False
    except ValueError:
        pass

    assert x.do_op_throws_count == 3
    assert x.do_op_count == 1
    assert x.d == {'a': 312.5, 'b': 207}
test_wrapped_dict()
