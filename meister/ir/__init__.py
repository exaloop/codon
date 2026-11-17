# Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>
# C++: codon/cir/cir.h. Public CIR backbone; LLVM code generation is not imported.
# ruff: noqa: F401
from .attribute import (
    Attribute,
    DictLiteralAttribute,
    DocstringAttribute,
    ExactTypeAttribute,
    IntValueAttribute,
    KeyValueAttribute,
    KeyValuePair,
    ListLiteralAttribute,
    LiteralElement,
    MemberAttribute,
    PartialFunctionAttribute,
    PythonWrapperAttribute,
    SetLiteralAttribute,
    SrcInfoAttribute,
    StringListAttribute,
    StringValueAttribute,
    TupleLiteralAttribute,
)
from .base import IdMixin, Node, ReplaceableNode, cast, is_a
from .const import BoolConst, BytesConst, Const, FloatConst, IntConst, StringConst, TemplatedConst
from .flow import (
    Catch,
    Flow,
    ForFlow,
    IfFlow,
    ImperativeForFlow,
    PipelineFlow,
    SeriesFlow,
    Stage,
    TryCatchFlow,
    WhileFlow,
)
from .func import BodiedFunc, ExternalFunc, Func, InternalFunc, Intrinsic, LLVMFunc
from .instr import (
    AssignInstr,
    AwaitInstr,
    BreakInstr,
    CallInstr,
    ContinueInstr,
    ControlFlowInstr,
    ExtractInstr,
    FlowInstr,
    InsertInstr,
    Instr,
    ReturnInstr,
    StackAllocInstr,
    TernaryInstr,
    ThrowInstr,
    YieldInInstr,
    YieldInstr,
)
from .module import Module
from .type import (
    BFloat16Type,
    BoolType,
    DerivedType,
    Field,
    Float16Type,
    Float32Type,
    Float128Type,
    FloatType,
    FuncType,
    GeneratorType,
    Generic,
    IntType,
    MemberedType,
    OptionalType,
    PointerType,
    PrimitiveType,
    RecordType,
    RefType,
    RuntimeInfo,
    Type,
    UnionType,
    VectorType,
)
from .value import Value
from .var import PointerValue, Var, VarValue

# The frontend previously exposed its function IR handle under this name.
Function = Func


def dump(node, annotate_fields=True, include_attributes=False, indent=None, types=False):
    """CIR counterpart of meister.ast.dump, including its formatting options."""
    from .util.format import dump as dump_ir

    return dump_ir(node, annotate_fields, include_attributes, indent, types)
