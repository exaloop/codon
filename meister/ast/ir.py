# Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>

from __future__ import annotations

from dataclasses import field
from typing import ClassVar

from ..bridge import List, dataclass
from ..ir import Function, Type

__all__ = ["Function", "PyModule", "Type"]


@dataclass
class PyModule:
    types: List[PyType] = field(default_factory=list)
    functions: List[PyFunction] = field(default_factory=list)
    name: str = ""
    doc: str = ""


@dataclass
class PyFunction:
    TOPLEVEL: ClassVar[int] = 0
    METHOD: ClassVar[int] = 1
    CLASS: ClassVar[int] = 2
    STATIC: ClassVar[int] = 3

    name: str
    doc: str
    func: object = None
    kind: int = TOPLEVEL
    nargs: int = 0
    keywords: bool = False
    coexist: bool = False


@dataclass
class PyGetSet:
    name: str
    doc: str
    get: object
    set: object


@dataclass
class PyMember:
    name: str
    doc: str
    type: int
    indexes: tuple[int, ...]
    readonly: bool = False


@dataclass
class PyType:
    name: str
    doc: str
    type: object = None
    base: PyType | None = None
    type_ptr_hook: object = None
    slots: dict[str, object] = field(default_factory=dict)
    methods: List[PyFunction] = field(default_factory=list)
    members: List[PyMember] = field(default_factory=list)
    getset: List[PyGetSet] = field(default_factory=list)
