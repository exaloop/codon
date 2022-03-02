import sys
from io import StringIO
from time import time
from typing import Dict, List, Tuple

from extra.cython import JitError, codon


# test stdout


def test_stdout():
    @codon
    def run():
        print("hello world!")

    try:
        output = StringIO()
        sys.stdout = output
        run()
        assert output.getvalue() == "hello world!\n"
    finally:
        sys.stdout = sys.__stdout__


test_stdout()


# test error handling


def test_error_handling():
    @codon
    def run() -> int:
        return "not int"

    try:
        r = run()
    except JitError:
        assert True
    except BaseException:
        assert False
    else:
        assert False


test_error_handling()


# test type validity


def test_return_type():
    @codon
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


test_return_type()


def test_param_types():
    @codon
    def run(a: int, b: Tuple[int, int], c: List[int], d: Dict[str, int]) -> int:
        s = 0
        for v in [a, *b, *c, *d.values()]:
            s += v
        return s

    r = run(1, (2, 3), [4, 5, 6], dict(a=7, b=8, c=9))
    assert type(r) == int
    assert r == 45


test_param_types()
