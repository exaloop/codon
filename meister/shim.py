from typing import (
    Any,
    Callable,
    ClassVar,
    Optional,
    Iterator,
    List,
    Literal,
    NoReturn,
    Tuple,
    TypeVar,
    Union,
    Dict,
    Set,
    cast,
)
from abc import abstractmethod
Generator = Iterator

class static:
    def vars(obj):
        yield from vars(obj).items()
    def len(*args):
        return len(*args)

class Codon:
    def unwrap(x, T=None):
        if x is None and T is None:
            raise ValueError("unexpected None")
        return x

    def return_type(*args):
        return Any

    def any_members(self):
        if isinstance(self, tuple):
            yield from enumerate(self)
        else:
            for i in self.__dict__.items():
                yield i

class TypeGetter:
    def __getitem__(self, i):
        return i
unrealized_type = TypeGetter()

import io
import argparse

CODON: Literal[bool] = False

def class_name(o):
    return o.__class__.__name__

def inline(f):
    return f

NoneType = type(None)

def _is_tuple(self):
    return isinstance(self, tuple)
Any.is_tuple = _is_tuple

def _is_list(self):
    return isinstance(self, list) or isinstance(self, tuple)
Any.is_list = _is_list

def _is_optional(self):
    return True
Any.is_optional = _is_optional


