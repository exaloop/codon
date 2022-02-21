import sys
from time import time
from typing import Dict, List, Tuple

from extra.cython import codon, CodonError


# test error handling


def test_error_handling():
    @codon
    def get() -> int:
        return "not int"

    try:
        r = get()
        assert False
    except CodonError:
        assert True
    except BaseException:
        assert False


test_error_handling()


# test type validity


def test_return_type():
    @codon
    def get() -> Tuple[int, str, float, List[int], Dict[str, int]]:
        return (1, "str", 2.45, [1, 2, 3], {"a": 1, "b": 2})

    r = get()
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


test_return_type()


def test_param_types():
    @codon
    def get(a: int, b: Tuple[int, int], c: List[int], d: Dict[str, int]) -> int:
        s = 0
        for v in [a, *b, *c, *d.values()]:
            s += v
        return s

    r = get(1, (2, 3), [4, 5, 6], dict(a=7, b=8, c=9))
    assert type(r) == int
    assert r == 45


test_param_types()


# test exec time


def timed(label, *args):
    def timed_single(fn, *args):
        cpt = time()
        r = fn(*args)
        return r, round(time() - cpt, 3)

    print(f"{label}:")
    res = [(a[0], *timed_single(*a[1:])) for a in args]
    mxl = [0, 0, 0]
    for r in res:
        for i in range(len(r)):
            mxl[i] = max(mxl[i], len(str(r[i])))
    for r in res:
        l = " | ".join([str(r[i]).ljust(mxl[i], " ") for i in range(len(r))])
        print(f"  {l}")
    print()


def fib_python(a: int) -> int:
    return a if a < 2 else fib_python(a - 1) + fib_python(a - 2)


@codon
def fib_codon(a: int) -> int:
    return a if a < 2 else fib_codon(a - 1) + fib_codon(a - 2)


iters = 37
sys.setrecursionlimit(iters ** 2 + 1)
timed(f"fib({iters})", ("python", fib_python, iters), ("codon", fib_codon, iters))


def asum_python(x: int, m: int = 2) -> int:
    s = 0
    for i in range(x + 1):
        s += i * (-1 if i % m == 0 else 1)
    return s


@codon
def asum_codon(x: int, m: int = 2) -> int:
    s = 0
    for i in range(x + 1):
        s += i * (-1 if i % m == 0 else 1)
    return s


@codon
def asum_codon_par(x: int, m: int = 2) -> int:
    s = 0
    _@par
    for i in range(x + 1):
        s += i * (-1 if i % m == 0 else 1)
    return s


iters, step = 50_000_000, 3
timed(
    f"asum({iters}, {step})",
    ("python", asum_python, iters, step),
    ("codon", asum_codon, iters, step),
    ("codon @par", asum_codon_par, iters, step),
)
