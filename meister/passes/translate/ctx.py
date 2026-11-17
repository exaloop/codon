# Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>

from __future__ import annotations

from ...bridge import Dict, Enum, List, dataclass


@dataclass(init=False, eq=False)
class TranslateItem:
    """C++: codon/parser/visitors/translate/translate_ctx.h:21. Identifier: a function, class (type), or variable."""

    class Kind(Enum):
        Func = 0
        Type = 1
        Var = 2

    kind: Kind
    handle: object | None
    # Base function pointer.
    base: object

    # C++: codon/parser/visitors/translate/translate_ctx.h:32
    def __init__(self, kind, base, handle=None):
        self.kind, self.base, self.handle = kind, base, handle

    # C++: codon/parser/visitors/translate/translate_ctx.h:35
    @property
    def func(self):
        return self.handle if self.kind == self.Kind.Func else None

    # C++: codon/parser/visitors/translate/translate_ctx.h:36
    @property
    def type(self):
        return self.handle if self.kind == self.Kind.Type else None

    # C++: codon/parser/visitors/translate/translate_ctx.h:37
    @property
    def var(self):
        return self.handle if self.kind == self.Kind.Var else None


@dataclass(init=False, eq=False)
class TranslateContext:
    """C++: codon/parser/visitors/translate/translate_ctx.h:43, translate_ctx.cpp:16. IR translation variable table."""

    cache: object
    map: Dict[str, List[TranslateItem]]
    stack: List[List[str]]
    # Function bases, IR series (blocks), and sequence items for attributes.
    bases: List[object]
    series: List[object]
    seq_items: List[List[object]]
    # Main/current JIT cell, and first user statement after its runtime preamble.
    main_func: object | None
    main_user_start: object | None

    # C++: codon/parser/visitors/translate/translate_ctx.cpp:16
    def __init__(self, cache):
        self.cache = cache
        self.map, self.stack = {}, [[]]
        self.bases, self.series, self.seq_items = [], [], []
        self.main_func = self.main_user_start = None

    # C++: codon/parser/visitors/translate/translate_ctx.cpp:54
    def add(self, kind, name, handle):
        assert name, "adding an empty identifier"
        item = TranslateItem(kind, self.base, handle)
        self.map.setdefault(name, []).insert(0, item)
        self.stack[0].append(name)
        return item

    # C++ API: codon/parser/visitors/translate/translate_ctx.h:43
    def add_block(self):
        self.stack.insert(0, [])

    # C++ API: codon/parser/visitors/translate/translate_ctx.h:43
    def pop_block(self):
        for name in self.stack.pop(0):
            self.map[name].pop(0)
            if not self.map[name]:
                del self.map[name]

    # C++: codon/parser/visitors/translate/translate_ctx.cpp:19
    def find(self, name):
        from ..typecheck import utils

        if values := self.map.get(name):
            return values[0]
        item = self.cache.type_ctx.get(name)
        if item and item.is_type and item.type.can_realize():
            typ = item.type
            if name != typ.realized_name():  # type prefix
                typ = utils.extract_type(self.cache.type_ctx, typ)
            cls = self.cache.classes.get(typ.require_cls.name)
            if cls is None or name not in cls.realizations:
                return None
            return TranslateItem(TranslateItem.Kind.Type, self.bases[0], cls.realizations[name].ir)
        if item and item.type.func and item.type.can_realize():
            fn = self.cache.functions.get(item.type.require_func.ast.name)
            assert fn and name in fn.realizations, f"cannot find function realization {name}"
            return TranslateItem(TranslateItem.Kind.Func, self.bases[0], fn.realizations[name].ir)
        return None

    # C++: codon/parser/visitors/translate/translate_ctx.cpp:47
    def force_find(self, name):
        item = self.find(name)
        assert item is not None, f"cannot find '{name}'"
        return item

    # C++: codon/parser/visitors/translate/translate_ctx.cpp:69
    @property
    def module(self):
        return self.bases[0].module

    # C++: codon/parser/visitors/translate/translate_ctx.cpp:72
    @property
    def base(self):
        return self.bases[-1]

    # C++: codon/parser/visitors/translate/translate_ctx.cpp:73
    @property
    def current_series(self):
        return self.series[-1]

    # C++: codon/parser/visitors/translate/translate_ctx.cpp:66
    def add_series(self, series):
        self.series.append(series)

    # C++: codon/parser/visitors/translate/translate_ctx.cpp:67
    def pop_series(self):
        self.series.pop()
