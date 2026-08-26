# C++ comment: codon/parser/ast/error.h:1
# Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>
from __future__ import annotations

from ..bridge import List, dataclass
from . import ast


@dataclass(init=False)
class ErrorMessage:
    message: str = ""
    info: ast.Node.SrcInfo
    error_code: int = -1

    def __init__(
        self,
        message: str = "",
        info: ast.Node.SrcInfo | None = None,
        error_code: int = -1,
    ):
        self.message = message
        self.info = info or ast.Node.SrcInfo()
        self.error_code = error_code
        self.error_code = -1

    @property
    def file(self) -> str:
        return self.info.file

    @property
    def line(self) -> int:
        return self.info.line

    @property
    def column(self) -> int:
        return self.info.col

    def __str__(self):
        if self.file:
            return f"{self.file}:{self.line}:{self.column}: {self.message}"
        else:
            return self.message

    def __eq__(self, other):
        return self.message == other.message and self.info == other.info


@dataclass(init=False)
class Backtrace:
    trace: List[ErrorMessage]

    def __init__(self, trace: List[ErrorMessage] | None = None):
        self.trace = [] if trace is None else trace

    def __iter__(self):
        yield from self.trace

    def add(self, message: str, info: ast.Node.SrcInfo | None = None):
        self.trace.append(ErrorMessage(message, info or ast.Node.SrcInfo()))


@dataclass(init=False)
class ParserErrors:
    errors: List[Backtrace]

    def __init__(self, messages=None, errors: List[Backtrace] | None = None):
        self.errors = []
        if isinstance(messages, ErrorMessage):
            self.errors.append(Backtrace([messages]))
        elif isinstance(messages, list):
            for msg in messages:
                self.errors.append(Backtrace([msg]))
        if errors:
            self.errors += errors

    def __iter__(self):
        yield from self.errors

    def append(self, other: ParserErrors):
        for trace in other:
            self.add_error(trace)

    def add_error(self, trace: Backtrace):
        """Add an error message to the current backtrace"""
        if not self.errors or self.errors[-1] != trace:
            self.errors.append(trace)

    @property
    def message(self) -> str:
        return "" if not self.errors else self.errors[0].trace[0].message


@dataclass(init=False)
class ParserError(Exception):
    """Used for parsing, transformation and type-checking errors."""

    # These vectors (stacks) store an error stack-trace.
    errors: ParserErrors

    def __init__(self, errors: ParserErrors | None = None):
        self.errors = ParserErrors() if errors is None else errors
        Exception.__init__(self, self.errors.message)
