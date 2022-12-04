import random as R
import time
import sys
from copy import copy

seed = int(time.time())
# sys.stderr.write('seed: ' + str(seed) + '\n')
R.seed(seed)


@test
def test_rnd_result(name, results, invariant):
    print(name)
    for a in results:
        assert invariant(a)


test_rnd_result(
    "randrange", [R.randrange(10) for _ in range(100)], range(10).__contains__
)
test_rnd_result(
    "randrange", [R.randrange(5, 20) for _ in range(100)], range(5, 20).__contains__
)
test_rnd_result(
    "randrange",
    [R.randrange(9, 99, 3) for _ in range(100)],
    range(9, 99, 3).__contains__,
)
test_rnd_result(
    "randint", [R.randint(5, 20) for _ in range(100)], range(5, 20 + 1).__contains__
)

population = list("ABCDEFGHIJKLMNOP")
test_rnd_result(
    "choice", [R.choice(population) for _ in range(100)], population.__contains__
)
test_rnd_result(
    "choice", [R.choice(population) for _ in range(100)], population.__contains__
)
test_rnd_result("choices", R.choices(population), population.__contains__)
test_rnd_result("choices", R.choices(population, k=100), population.__contains__)


@test
def test_shuffle(v):
    s = copy(v)
    R.shuffle(s)
    assert sorted(v) == sorted(s)


test_shuffle(list(range(100)))


@test
def test_sample(n: int, k: int):
    s = R.sample(list(range(n)), k=k)
    assert len(s) == k
    assert len(set(s)) == len(s)
    for a in s:
        assert a in range(n)


test_sample(100, 5)
test_sample(100, 100)
test_sample(100, 0)


from python import random as Rpy

@test
def test_vs_python(*args, seed, method: Static[str], T: type = float):
    print(seed, method, args)
    R1 = R.Random(seed)
    R2 = Rpy.Random(seed)

    N = 50
    A1 = [T(getattr(R1, method)(*args)) for _ in range(N)]
    A2 = [T.__from_py__(getattr(R2, method)(*args).p) for _ in range(N)]
    assert A1 == A2

test_vs_python(-10, 10, seed=22, method='randrange', T=int)
test_vs_python(-10, 10, 3, seed=33, method='randrange', T=int)
test_vs_python(-10, 10, seed=44, method='randint', T=int)
test_vs_python(20, seed=55, method='getrandbits', T=int)
test_vs_python(32, seed=55, method='getrandbits', T=int)
test_vs_python(40, seed=55, method='getrandbits', T=int)
test_vs_python(63, seed=55, method='getrandbits', T=int)
test_vs_python(seed=0, method='random')
test_vs_python(-12.5, 101.2, seed=1, method='uniform')
test_vs_python(-13, 5.5, 0, seed=2, method='triangular')
#test_vs_python(1.0, 2, seed=3, method='betavariate')  # different in older Python versions
test_vs_python(0.3, seed=4, method='expovariate')
#test_vs_python(1.0, 2, seed=5, method='gammavariate')  # different in older Python versions
test_vs_python(1.0, 2.0, seed=-101, method='gauss')
test_vs_python(1.0, 2.0, seed=-102, method='lognormvariate')
test_vs_python(1.0, 2.0, seed=-103, method='normalvariate')
test_vs_python(1.0, 2.0, seed=0xffffffff, method='vonmisesvariate')
test_vs_python(1.0, seed=0xffffffff-1, method='paretovariate')
test_vs_python(1.0, 2.0, seed=0, method='weibullvariate')


@test
def test_state():
    r = R.Random(1234)
    state = r.getstate()
    N = 100
    A1 = [r.random() for _ in range(N)]
    B1 = [r.gauss() for _ in range(N)]
    r.setstate(state)
    A2 = [r.random() for _ in range(N)]
    B2 = [r.gauss() for _ in range(N)]
    r.seed(1234)
    A3 = [r.random() for _ in range(N)]
    B3 = [r.gauss() for _ in range(N)]

    assert A1 == A2 == A3
    assert B1 == B2 == B3

test_state()
