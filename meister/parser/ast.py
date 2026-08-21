from ..bridge import *


@dataclass
class AST:
    @dataclass
    class SrcInfo:
        lineno: int
        col_offset: int
        end_lineno: int
        end_col_offset: int

    class Attribute:
        def __init__(self):
            pass

    class KeyValueAttribute(Attribute):
        key: str
        value: str

        def __init__(self, key, value):
            super().__init__()
            self.key = key
            self.value = value


    loc: AST.SrcInfo
    flags: Optional[Set[int]]
    attrs: Optional[Dict[int, Attribute]]

    @abstractmethod
    def __init__(
        self, lineno=0, col_offset=0, end_lineno=0, end_col_offset=0, **kwargs
    ):
        self.loc = AST.SrcInfo(lineno, col_offset, end_lineno, end_col_offset)
        self.flags = None
        self.attrs = None

    def __str__(self):
        return dump(self)
    def __repr__(self):
        return self.__str__()

    def has(self, flag: int):
        return (self.flags and flag in self.flags) or (self.attrs and flag in self.attrs)
    def set(self, flag: int, attr: Optional[Attribute] = None):
        if not attr:
            if not self.flags:
                self.flags = set()
            self.flags.add(flag)
        else:
            if not self.attrs:
                self.attrs = {}
            self.attrs[flag] = attr

    @property
    def lineno(self):
        return self.loc.lineno
    @property
    def col_offset(self):
        return self.loc.col_offset
    @property
    def end_lineno(self):
        return self.loc.end_lineno
    @property
    def end_col_offset(self):
        return self.loc.end_col_offset

    @abstractmethod
    def accept(self, visitor):
        pass


class BaseExpression(AST):
    @abstractmethod
    def __init__(self, **kwargs):
        super().__init__(**kwargs)

    @abstractmethod
    def accept(self, visitor):
        pass


class BaseStatement(AST):
    @abstractmethod
    def __init__(self, **kwargs):
        super().__init__(**kwargs)

    @abstractmethod
    def accept(self, visitor):
        pass


# Root nodes


class Module(AST):
    body: list[BaseStatement]

    def __init__(self, body=None, **kwargs):
        super().__init__(**kwargs)
        self.body = [cast(BaseStatement, b) for b in body] if body is not None else []

    def accept(self, visitor):
        return visitor.visit_Module(self)


class Interactive(AST):
    body: list[BaseStatement]

    def __init__(self, body=None, **kwargs):
        super().__init__(**kwargs)
        self.body = [cast(BaseStatement, i) for i in body] if body is not None else []

    def accept(self, visitor):
        return visitor.visit_Interactive(self)


# Literals


class Constant(BaseExpression):
    @abstractmethod
    def __init__(self, **kwargs):
        super().__init__(**kwargs)

    @abstractmethod
    def accept(self, visitor):
        pass


class Bool(Constant):  ## OLD: BoolExpr
    value: bool

    def __init__(self, value=False, **kwargs):
        super().__init__(**kwargs)
        self.value = value

    def accept(self, visitor):
        return visitor.visit_Bool(self)


class Str(Constant):  ## OLD: StringExpr
    value: str
    prefix: str

    def __init__(self, value="", prefix="", **kwargs):
        super().__init__(**kwargs)
        self.value = value
        self.prefix = prefix

    def accept(self, visitor):
        return visitor.visit_Str(self)


class Num(Constant):  ## OLD: IntExpr / FloatExpr
    value: str
    suffix: str
    ## TODO: StoredValue

    def __init__(self, value="", suffix="", **kwargs):
        super().__init__(**kwargs)
        self.value = value
        self.suffix = suffix

    def accept(self, visitor):
        return visitor.visit_Num(self)


class Ellipsis(Constant):  ## OLD: EllipsisExpr
    def __init__(self, **kwargs):
        super().__init__(**kwargs)

    def accept(self, visitor):
        return visitor.visit_Ellipsis(self)


class NoneValue(Constant):  ## OLD: NoneExpr
    def __init__(self, **kwargs):
        super().__init__(**kwargs)

    def accept(self, visitor):
        return visitor.visit_NoneValue(self)


class FormattedValue(BaseExpression):  ## OLD: StringExpr.String
    value: BaseExpression
    conversion: str
    format_spec: Optional[str]

    def __init__(
        self,
        value,
        conversion="",
        format_spec: Optional[str] = None,
        **kwargs,
    ):
        super().__init__(**kwargs)
        self.value = value
        self.conversion = conversion
        self.format_spec = format_spec

    def accept(self, visitor):
        return visitor.visit_FormattedValue(self)


class JoinedStr(BaseExpression):   ## OLD: StringExpr
    values: list[BaseExpression]  # FormattedValue | Constant

    def __init__(self, values=None, **kwargs):
        super().__init__(**kwargs)
        self.values = (
            [cast(BaseExpression, i) for i in values] if values is not None else []
        )

    def accept(self, visitor):
        return visitor.visit_JoinedStr(self)


class VariableCtx:
    Load = 1
    Store = 2
    Del = 3
    Update = 4


class ListEx(BaseExpression):
    elts: list[BaseExpression]
    ctx: int

    def __init__(self, elts=None, ctx=VariableCtx.Load, **kwargs):
        super().__init__(**kwargs)
        self.elts = [cast(BaseExpression, i) for i in elts] if elts is not None else []
        self.ctx = ctx

    def accept(self, visitor):
        return visitor.visit_ListEx(self)


class TupleEx(BaseExpression):
    elts: list[BaseExpression]
    ctx: int

    def __init__(self, elts=None, ctx=VariableCtx.Load, **kwargs):
        super().__init__(**kwargs)
        self.elts = [cast(BaseExpression, i) for i in elts] if elts is not None else []
        self.ctx = ctx

    def accept(self, visitor):
        return visitor.visit_TupleEx(self)


class SetEx(BaseExpression):
    elts: list[BaseExpression]

    def __init__(self, elts=None, **kwargs):
        super().__init__(**kwargs)
        self.elts = [cast(BaseExpression, i) for i in elts] if elts is not None else []

    def accept(self, visitor):
        return visitor.visit_SetEx(self)


class DictEx(BaseExpression):
    keys: list[Optional[BaseExpression]]
    values: list[BaseExpression]

    def __init__(self, keys=None, values=None, **kwargs):
        super().__init__(**kwargs)
        self.keys = (
            [cast(Optional[BaseExpression], i) for i in keys]
            if keys is not None
            else []
        )
        self.values = (
            [cast(BaseExpression, i) for i in values] if values is not None else []
        )

    def accept(self, visitor):
        return visitor.visit_DictEx(self)


# Variables


class Name(BaseExpression):  ## OLD: IdExpr
    id: str
    ctx: int

    def __init__(self, id="", ctx=VariableCtx.Load, **kwargs):
        super().__init__(**kwargs)
        self.id = id
        self.ctx = ctx

    def accept(self, visitor):
        return visitor.visit_Name(self)


class Starred(BaseExpression):  ## OLD: StarExpr
    value: BaseExpression
    ctx: int

    def __init__(self, value, ctx=VariableCtx.Load, **kwargs):
        super().__init__(**kwargs)
        self.value = cast(BaseExpression, value)
        self.ctx = ctx

    def accept(self, visitor):
        return visitor.visit_Starred(self)


# Expressions


class UnaryOperator:
    UAdd = 1
    USub = 2
    Not = 3
    Invert = 4


class UnaryOp(BaseExpression):
    op: int
    operand: BaseExpression

    def __init__(self, op: int, operand: BaseExpression, **kwargs):
        super().__init__(**kwargs)
        self.op = op
        self.operand = operand

    def accept(self, visitor):
        return visitor.visit_UnaryOp(self)


class BinaryOperator:
    Add = 1
    Sub = 2
    Mult = 3
    Div = 4
    FloorDiv = 5
    Mod = 6
    Pow = 7
    LShift = 8
    RShift = 9
    BitOr = 10
    BitXor = 11
    BitAnd = 12
    MatMult = 13


class BinOp(BaseExpression):
    op: int
    values: list[BaseExpression]

    def __init__(self, op: int, values=None, **kwargs):
        super().__init__(**kwargs)
        self.op = op
        self.values = (
            [cast(BaseExpression, i) for i in values] if values is not None else []
        )

    def accept(self, visitor):
        return visitor.visit_BinOp(self)


class PipeOperator:
    Pipe = 1
    Parallel = 2


class PipeOp(BaseExpression):
    head: BaseExpression
    values: list[Tuple[int, BaseExpression]]

    def __init__(self, head, values, **kwargs):
        super().__init__(**kwargs)
        self.head = head
        self.values = [(o, cast(BaseExpression, i)) for o, i in values]

    def accept(self, visitor):
        return visitor.visit_PipeOp(self)


class BoolOperator:
    And = 1
    Or = 2


class BoolOp(BaseExpression):
    op: int
    values: list[BaseExpression]

    def __init__(self, op: int, values=None, **kwargs):
        super().__init__(**kwargs)
        self.op = op
        self.values = (
            [cast(BaseExpression, i) for i in values] if values is not None else []
        )

    def accept(self, visitor):
        return visitor.visit_BoolOp(self)


class CompareOperator:
    Eq = 1
    NotEq = 2
    Lt = 3
    LtE = 4
    Gt = 5
    GtE = 6
    Is = 7
    IsNot = 8
    In = 9
    NotIn = 10


class Compare(BaseExpression):
    left: BaseExpression
    ops: list[int]
    comparators: list[BaseExpression]

    def __init__(self, left: BaseExpression, ops=None, comparators=None, **kwargs):
        super().__init__(**kwargs)
        self.left = left
        self.ops = ops if ops is not None else []
        self.comparators = (
            [cast(BaseExpression, i) for i in comparators]
            if comparators is not None
            else []
        )

    def accept(self, visitor):
        return visitor.visit_Compare(self)


class Keyword(AST):
    arg: Optional[str]
    value: BaseExpression

    def __init__(self, arg: Optional[str], value, **kwargs):
        super().__init__(**kwargs)
        self.arg = arg
        self.value = value

    def accept(self, visitor):
        return visitor.visit_Keyword(self)


keyword = Keyword


class Attribute(BaseExpression):
    value: BaseExpression
    attr: str
    ctx: int

    def __init__(self, value: BaseExpression, attr="", ctx=VariableCtx.Load, **kwargs):
        super().__init__(**kwargs)
        self.value = value
        self.attr = attr
        self.ctx = ctx

    def accept(self, visitor):
        return visitor.visit_Attribute(self)


class Call(BaseExpression):
    func: BaseExpression  # Name | Attribute
    args: list[BaseExpression]
    keywords: list[Keyword]

    def __init__(self, func, args=None, keywords=None, **kwargs):
        super().__init__(**kwargs)
        self.func = func
        self.args = [cast(BaseExpression, i) for i in args] if args is not None else []
        self.keywords = [cast(Keyword, i) for i in keywords] if keywords is not None else []

    def accept(self, visitor):
        return visitor.visit_Call(self)


class PartialCall(Call):
    def __init__(self, func, args=None, keywords=None, **kwargs):
        super().__init__(func, args, keywords, **kwargs)

    def accept(self, visitor):
        return visitor.visit_PartialCall(self)


class IfExp(BaseExpression):
    test: BaseExpression
    body: BaseExpression
    orelse: BaseExpression

    def __init__(self, test, body, orelse, **kwargs):
        super().__init__(**kwargs)
        self.test = test
        self.body = body
        self.orelse = orelse

    def accept(self, visitor):
        return visitor.visit_IfExp(self)


class NamedExpr(BaseExpression):
    target: BaseExpression
    value: BaseExpression

    def __init__(self, target, value, **kwargs):
        super().__init__(**kwargs)
        self.target = target
        self.value = value

    def accept(self, visitor):
        return visitor.visit_NamedExpr(self)


class Subscript(BaseExpression):
    value: BaseExpression
    slice: list[BaseExpression]
    ctx: int

    def __init__(self, value, slice, ctx=VariableCtx.Load, **kwargs):
        super().__init__(**kwargs)
        self.value = value
        if slice is not None:
            self.slice = (
                [cast(BaseExpression, i) for i in slice]
                if isinstance(slice, list)
                else [cast(BaseExpression, slice)]
            )
        else:
            self.slice = []
        self.ctx = ctx

    def accept(self, visitor):
        return visitor.visit_Subscript(self)


class Slice(BaseExpression):
    lower: Optional[BaseExpression]
    upper: Optional[BaseExpression]
    step: Optional[BaseExpression]

    def __init__(
        self,
        lower: Optional[BaseExpression] = None,
        upper: Optional[BaseExpression] = None,
        step: Optional[BaseExpression] = None,
        **kwargs,
    ):
        super().__init__(**kwargs)
        self.lower = lower
        self.upper = upper
        self.step = step

    def accept(self, visitor):
        return visitor.visit_Slice(self)


class Comprehension(AST):
    target: BaseExpression
    iter: BaseExpression
    ifs: list[BaseExpression]
    is_async: bool

    def __init__(self, target, iter, ifs=None, is_async=False, **kwargs):
        super().__init__(**kwargs)
        self.target = target
        self.iter = iter
        self.ifs = [cast(BaseExpression, i) for i in ifs] if ifs is not None else []
        self.is_async = is_async

    def accept(self, visitor):
        return visitor.visit_Comprehension(self)


comprehension = Comprehension


class ListComp(BaseExpression):
    elt: BaseExpression
    generators: list[Comprehension]

    def __init__(self, elt, generators=None, **kwargs):
        super().__init__(**kwargs)
        self.elt = elt
        self.generators = generators if generators is not None else []

    def accept(self, visitor):
        return visitor.visit_ListComp(self)


class SetComp(BaseExpression):
    elt: BaseExpression
    generators: list[Comprehension]

    def __init__(self, elt, generators=None, **kwargs):
        super().__init__(**kwargs)
        self.elt = elt
        self.generators = generators if generators is not None else []

    def accept(self, visitor):
        return visitor.visit_SetComp(self)


class GeneratorExp(BaseExpression):
    elt: BaseExpression
    generators: list[Comprehension]

    def __init__(self, elt, generators=None, **kwargs):
        super().__init__(**kwargs)
        self.elt = elt
        self.generators = generators if generators is not None else []

    def accept(self, visitor):
        return visitor.visit_GeneratorExp(self)


class DictComp(BaseExpression):
    key: BaseExpression
    value: BaseExpression
    generators: list[Comprehension]

    def __init__(self, key, value, generators=None, **kwargs):
        super().__init__(**kwargs)
        self.key = key
        self.value = value
        self.generators = generators if generators is not None else []

    def accept(self, visitor):
        return visitor.visit_DictComp(self)


# Statements


class Expr(BaseStatement):
    value: BaseExpression

    def __init__(self, value, **kwargs):
        super().__init__(**kwargs)
        self.value = value

    def accept(self, visitor):
        return visitor.visit_Expr(self)


class Assign(BaseStatement):
    targets: List[BaseExpression]
    value: Optional[BaseExpression]

    def __init__(self, targets, value: Optional[BaseExpression] = None, **kwargs):
        super().__init__(**kwargs)
        self.targets = (
            [cast(BaseExpression, i) for i in targets] if targets is not None else []
        )
        self.value = value

    def accept(self, visitor):
        return visitor.visit_Assign(self)


class AnnAssign(BaseStatement):
    target: BaseExpression
    annotation: Optional[BaseExpression]
    value: Optional[BaseExpression]
    simple: bool

    def __init__(self, target, annotation: Optional[BaseExpression] = None, value: Optional[BaseExpression] = None, simple=False, **kwargs):
        super().__init__(**kwargs)
        self.target = target
        self.annotation = annotation
        self.value = value
        self.simple = simple

    def accept(self, visitor):
        return visitor.visit_AnnAssign(self)


class AugAssign(BaseStatement):
    target: BaseExpression
    op: int
    value: BaseExpression

    def __init__(self, target, op, value, **kwargs):
        super().__init__(**kwargs)
        self.target = target
        self.op = op
        self.value = value

    def accept(self, visitor):
        return visitor.visit_AugAssign(self)


class Raise(BaseStatement):
    exc: Optional[BaseExpression]
    cause: Optional[BaseExpression]

    def __init__(self, exc=None, cause=None, **kwargs):
        super().__init__(**kwargs)
        self.exc = cast(Optional[BaseExpression], exc)
        self.cause = cast(Optional[BaseExpression], exc)

    def accept(self, visitor):
        return visitor.visit_Raise(self)


class Assert(BaseStatement):
    test: BaseExpression
    msg: Optional[BaseExpression]

    def __init__(self, test, msg: Optional[BaseExpression] = None, **kwargs):
        super().__init__(**kwargs)
        self.test = test
        self.msg = msg

    def accept(self, visitor):
        return visitor.visit_Assert(self)


class Delete(BaseStatement):
    targets: list[BaseExpression]

    def __init__(self, targets=None, **kwargs):
        super().__init__(**kwargs)
        self.targets = (
            [cast(BaseExpression, i) for i in targets] if targets is not None else []
        )

    def accept(self, visitor):
        return visitor.visit_Delete(self)


class Pass(BaseStatement):
    def __init__(self, **kwargs):
        super().__init__(**kwargs)

    def accept(self, visitor):
        return visitor.visit_Pass(self)


class Alias(AST):
    name: str
    asname: Optional[str]
    params: Optional[List[Arg]]
    ret: Optional[BaseExpression]

    def __init__(
        self,
        name="",
        asname: Optional[str] = None,
        params = None,
        ret: Optional[BaseExpression] = None,
        **kwargs
    ):
        super().__init__(**kwargs)
        self.name = name
        self.asname = asname
        self.params = [cast(BaseExpression, i) for i in params] if params is not None else []
        self.ret = ret

    def accept(self, visitor):
        return visitor.visit_Alias(self)


alias = Alias


class Import(BaseStatement):
    names: list[Alias]

    def __init__(self, names=None, **kwargs):
        super().__init__(**kwargs)
        self.names = names if names is not None else []

    def accept(self, visitor):
        return visitor.visit_Import(self)


class ImportFrom(BaseStatement):
    module: Optional[str]
    names: list[Alias]
    level: int

    def __init__(self, module: Optional[str] = None, names=None, level=0, **kwargs):
        super().__init__(**kwargs)
        self.module = module
        self.names = names if names is not None else []
        self.level = level

    def accept(self, visitor):
        return visitor.visit_ImportFrom(self)


class If(BaseStatement):
    test: BaseExpression
    body: list[BaseStatement]
    orelse: list[BaseStatement]

    def __init__(self, test, body=None, orelse=None, **kwargs):
        super().__init__(**kwargs)
        self.test = test
        self.body = [cast(BaseStatement, i) for i in body] if body is not None else []
        self.orelse = (
            [cast(BaseStatement, i) for i in orelse] if orelse is not None else []
        )

    def accept(self, visitor):
        return visitor.visit_If(self)


class For(BaseStatement):
    target: BaseExpression
    iter: BaseExpression
    body: list[BaseStatement]
    orelse: list[BaseStatement]
    decorator_list: list[BaseExpression]

    def __init__(self, target, iter, body=None, orelse=None, decorator_list=None, **kwargs):
        super().__init__(**kwargs)
        self.target = target
        self.iter = iter
        self.body = [cast(BaseStatement, i) for i in body] if body is not None else []
        self.orelse = (
            [cast(BaseStatement, i) for i in orelse] if orelse is not None else []
        )
        self.decorator_list = (
            [cast(BaseExpression, i) for i in decorator_list]
            if decorator_list is not None
            else []
        )

    def accept(self, visitor):
        return visitor.visit_For(self)


class While(BaseStatement):
    test: BaseExpression
    body: list[BaseStatement]
    orelse: list[BaseStatement]

    def __init__(self, test, body=None, orelse=None, **kwargs):
        super().__init__(**kwargs)
        self.test = test
        self.body = [cast(BaseStatement, i) for i in body] if body is not None else []
        self.orelse = (
            [cast(BaseStatement, i) for i in orelse] if orelse is not None else []
        )

    def accept(self, visitor):
        return visitor.visit_While(self)


class Break(BaseStatement):
    def __init__(self, **kwargs):
        super().__init__(**kwargs)

    def accept(self, visitor):
        return visitor.visit_Break(self)


class Continue(BaseStatement):
    def __init__(self, **kwargs):
        super().__init__(**kwargs)

    def accept(self, visitor):
        return visitor.visit_Continue(self)


class ExceptHandler(AST):
    type: Optional[BaseExpression]
    name: Optional[str]
    body: list[BaseStatement]

    def __init__(
        self,
        typ: Optional[BaseExpression] = None,
        name: Optional[str] = None,
        body=None,
        **kwargs,
    ):
        super().__init__(**kwargs)
        self.type = typ
        self.name = name
        self.body = [cast(BaseStatement, i) for i in body] if body is not None else []

    def accept(self, visitor):
        return visitor.visit_ExceptHandler(self)


class Try(BaseStatement):
    body: list[BaseStatement]
    handlers: list[ExceptHandler]
    orelse: list[BaseStatement]
    finalbody: list[BaseStatement]

    def __init__(self, body=None, handlers=None, orelse=None, finalbody=None, **kwargs):
        super().__init__(**kwargs)
        self.body = [cast(BaseStatement, i) for i in body] if body is not None else []
        self.handlers = handlers if handlers is not None else []
        self.orelse = (
            [cast(BaseStatement, i) for i in orelse] if orelse is not None else []
        )
        self.finalbody = (
            [cast(BaseStatement, i) for i in finalbody] if finalbody is not None else []
        )

    def accept(self, visitor):
        return visitor.visit_Try(self)


class WithItem(AST):
    context_expr: BaseExpression
    optional_vars: Optional[BaseExpression]

    def __init__(
        self, context_expr, optional_vars: Optional[BaseExpression] = None, **kwargs
    ):
        super().__init__(**kwargs)
        self.context_expr = context_expr
        self.optional_vars = optional_vars

    def accept(self, visitor):
        return visitor.visit_WithItem(self)


withitem = WithItem


class With(BaseStatement):
    items: list[WithItem]
    body: list[BaseStatement]

    def __init__(self, items=None, body=None, **kwargs):
        super().__init__(**kwargs)
        self.items = items if items is not None else []
        self.body = [cast(BaseStatement, i) for i in body] if body is not None else []

    def accept(self, visitor):
        return visitor.visit_With(self)


class MatchPattern(AST):
    def __init__(self, **kwargs):
        super().__init__(**kwargs)

    def accept(self, visitor):
        return visitor.visit_MatchPattern(self)


class MatchValue(MatchPattern):
    value: BaseExpression

    def __init__(self, value, **kwargs):
        super().__init__(**kwargs)
        self.value = value

    def accept(self, visitor):
        return visitor.visit_MatchValue(self)


class MatchSingleton(MatchPattern):
    value: Optional[bool]  # None, True, False

    def __init__(self, value, **kwargs):
        super().__init__(**kwargs)
        self.value = None if value is None else value

    def accept(self, visitor):
        return visitor.visit_MatchSingleton(self)


class MatchSequence(MatchPattern):
    patterns: list[MatchPattern]

    def __init__(self, patterns=None, **kwargs):
        super().__init__(**kwargs)
        self.patterns = (
            [cast(MatchPattern, i) for i in patterns] if patterns is not None else []
        )

    def accept(self, visitor):
        return visitor.visit_MatchSequence(self)


class MatchStar(MatchPattern):
    name: Optional[str]

    def __init__(self, name: Optional[str] = None, **kwargs):
        super().__init__(**kwargs)
        self.name = name

    def accept(self, visitor):
        return visitor.visit_MatchStar(self)


class MatchMapping(MatchPattern):
    keys: list[BaseExpression]
    patterns: list[MatchPattern]
    rest: Optional[str]

    def __init__(self, keys=None, patterns=None, rest: Optional[str] = None, **kwargs):
        super().__init__(**kwargs)
        self.keys = [cast(BaseExpression, i) for i in keys] if keys is not None else []
        self.patterns = (
            [cast(MatchPattern, i) for i in patterns] if patterns is not None else []
        )
        self.rest = rest

    def accept(self, visitor):
        return visitor.visit_MatchMapping(self)


class MatchClass(MatchPattern):
    cls: BaseExpression
    patterns: list[MatchPattern]
    kwd_attrs: list[str]
    kwd_patterns: list[MatchPattern]

    def __init__(self, cls, patterns=None, kwd_attrs=None, kwd_patterns=None, **kwargs):
        super().__init__(**kwargs)
        self.cls = cls
        self.patterns = (
            [cast(MatchPattern, i) for i in patterns] if patterns is not None else []
        )
        self.kwd_attrs = kwd_attrs if kwd_attrs is not None else []
        self.kwd_patterns = (
            [cast(MatchPattern, i) for i in kwd_patterns]
            if kwd_patterns is not None
            else []
        )

    def accept(self, visitor):
        return visitor.visit_MatchClass(self)


class MatchAs(MatchPattern):
    pattern: Optional[MatchPattern]
    name: Optional[str]

    def __init__(
        self,
        pattern: Optional[MatchPattern] = None,
        name: Optional[str] = None,
        **kwargs,
    ):
        super().__init__(**kwargs)
        self.pattern = pattern
        self.name = name

    def accept(self, visitor):
        return visitor.visit_MatchAs(self)


class MatchOr(MatchPattern):
    pattern: list[MatchPattern]

    def __init__(self, pattern=None, **kwargs):
        super().__init__(**kwargs)
        self.pattern = (
            [cast(MatchPattern, i) for i in pattern] if pattern is not None else []
        )

    def accept(self, visitor):
        return visitor.visit_MatchOr(self)


class MatchCase(AST):
    pattern: MatchPattern
    guard: Optional[BaseExpression]
    body: list[BaseStatement]

    def __init__(
        self, pattern, guard: Optional[BaseExpression] = None, body=None, **kwargs
    ):
        super().__init__(**kwargs)
        self.pattern = pattern
        self.guard = guard
        self.body = [cast(BaseStatement, i) for i in body] if body is not None else []

    def accept(self, visitor):
        return visitor.visit_MatchCase(self)


match_case = MatchCase


class Match(BaseStatement):
    subject: BaseExpression
    cases: list[MatchCase]

    def __init__(self, subject, cases=None, **kwargs):
        super().__init__(**kwargs)
        self.subject = subject
        self.cases = cases if cases is not None else []

    def accept(self, visitor):
        return visitor.visit_Match(self)


# Type parameters


class TypeVar(AST):
    name: str
    bound: Optional[BaseExpression]
    default_value: Optional[BaseExpression]

    def __init__(
        self,
        name="",
        bound: Optional[BaseExpression] = None,
        default_value: Optional[BaseExpression] = None,
        **kwargs,
    ):
        super().__init__(**kwargs)
        self.name = name
        self.bound = bound
        self.default_value = default_value

    def accept(self, visitor):
        return visitor.visit_TypeVar(self)


# Functions and Classes


class Arg(AST):
    arg: str
    annotation: Optional[BaseExpression]

    def __init__(self, arg="", annotation: Optional[BaseExpression] = None, **kwargs):
        super().__init__(**kwargs)
        self.arg = arg
        self.annotation = annotation

    def accept(self, visitor):
        return visitor.visit_Arg(self)


arg = Arg


class Arguments(AST):
    posonlyargs: list[Arg]
    args: list[Arg]
    vararg: Optional[Arg]
    kwonlyargs: list[Arg]
    kw_defaults: list[Optional[BaseExpression]]
    kwarg: Optional[Arg]
    defaults: list[BaseExpression]
    types: list[Arg]
    type_defaults: list[Optional[BaseExpression]]

    def __init__(
        self,
        posonlyargs=None,
        args=None,
        vararg: Optional[Arg] = None,
        kwonlyargs=None,
        kw_defaults=None,
        kwarg: Optional[Arg] = None,
        defaults=None,
        types=None,
        type_defaults=None,
        **kwargs,
    ):
        super().__init__(**kwargs)
        self.posonlyargs = posonlyargs if posonlyargs is not None else []
        self.args = args if args is not None else []
        self.vararg = vararg
        self.kwonlyargs = kwonlyargs if kwonlyargs is not None else []
        self.kw_defaults = (
            [cast(Optional[BaseExpression], i) for i in kw_defaults]
            if kw_defaults is not None
            else []
        )
        self.kwarg = kwarg
        self.defaults = (
            [cast(BaseExpression, i) for i in defaults]
            if defaults is not None
            else []
        )
        self.types = types if types is not None else []
        self.type_defaults = (
            [cast(Optional[BaseExpression], i) for i in type_defaults]
            if type_defaults is not None
            else []
        )

    def accept(self, visitor):
        return visitor.visit_Arguments(self)


arguments = Arguments


class FunctionDef(BaseStatement):
    name: str
    args: Arguments
    body: list[BaseStatement]
    returns: Optional[BaseExpression]
    decorator_list: list[BaseExpression]
    type_params: list[TypeVar]

    def __init__(
        self,
        name="",
        args=None,
        body=None,
        returns: Optional[BaseExpression] = None,
        type_params=None,
        decorator_list=None,
        **kwargs,
    ):
        super().__init__(**kwargs)
        self.name = name
        self.args = args if args is not None else Arguments()
        self.body = [cast(BaseStatement, i) for i in body] if body is not None else []
        self.returns = returns
        self.decorator_list = (
            [cast(BaseExpression, i) for i in decorator_list]
            if decorator_list is not None
            else []
        )
        self.type_params = type_params if type_params is not None else []

    def accept(self, visitor):
        return visitor.visit_FunctionDef(self)


class Lambda(BaseExpression):
    args: Arguments
    body: BaseExpression

    def __init__(self, args, body, **kwargs):
        super().__init__(**kwargs)
        self.args = args if args is not None else Arguments()
        self.body = body

    def accept(self, visitor):
        return visitor.visit_Lambda(self)


class Return(BaseStatement):
    value: Optional[BaseExpression]

    def __init__(self, value: Optional[BaseExpression] = None, **kwargs):
        super().__init__(**kwargs)
        self.value = value

    def accept(self, visitor):
        return visitor.visit_Return(self)


class Yield(BaseExpression):
    value: Optional[BaseExpression]

    def __init__(self, value: Optional[BaseExpression] = None, **kwargs):
        super().__init__(**kwargs)
        self.value = value

    def accept(self, visitor):
        return visitor.visit_Yield(self)


class YieldFrom(BaseExpression):
    value: BaseExpression

    def __init__(self, value, **kwargs):
        super().__init__(**kwargs)
        self.value = value

    def accept(self, visitor):
        return visitor.visit_YieldFrom(self)


class Global(BaseStatement):
    names: list[str]

    def __init__(self, names=None, **kwargs):
        super().__init__(**kwargs)
        self.names = names if names is not None else []

    def accept(self, visitor):
        return visitor.visit_Global(self)


class Nonlocal(BaseStatement):
    names: list[str]

    def __init__(self, names=None, **kwargs):
        super().__init__(**kwargs)
        self.names = names if names is not None else []

    def accept(self, visitor):
        return visitor.visit_Nonlocal(self)


class ClassDef(BaseStatement):
    name: str
    bases: list[BaseExpression]
    keywords: list[Keyword]
    body: list[BaseStatement]
    decorator_list: list[BaseExpression]
    type_params: list[TypeVar]

    def __init__(
        self,
        name="",
        bases: Optional[list[BaseExpression]] = None,
        keywords=None,
        body=None,
        decorator_list=None,
        type_params: Optional[list[TypeVar]] = None,
        **kwargs,
    ):
        super().__init__(**kwargs)
        self.name = name
        self.bases = (
            [cast(BaseExpression, i) for i in bases] if bases is not None else []
        )
        self.keywords = keywords if keywords is not None else []
        self.body = [cast(BaseStatement, i) for i in body] if body is not None else []
        self.decorator_list = (
            [cast(BaseExpression, i) for i in decorator_list]
            if decorator_list is not None
            else []
        )
        self.type_params = type_params if type_params is not None else []

    def accept(self, visitor):
        return visitor.visit_ClassDef(self)


class AsyncFunctionDef(FunctionDef):
    def __init__(
        self,
        name="",
        args=None,
        body=None,
        returns=None,
        type_params=None,
        decorator_list=None,
        **kwargs,
    ):
        super().__init__(
            name, args, body, returns, type_params, decorator_list, **kwargs
        )

    def accept(self, visitor):
        return visitor.visit_AsyncFunctionDef(self)


class LLVMFunctionDef(FunctionDef):
    def __init__(
        self,
        name="",
        args=None,
        body=None,
        returns=None,
        type_params=None,
        decorator_list=None,
        **kwargs,
    ):
        super().__init__(
            name, args, body, returns, type_params, decorator_list, **kwargs
        )

    def accept(self, visitor):
        return visitor.visit_LLVMFunctionDef(self)


class Await(BaseExpression):
    value: Optional[BaseExpression]

    def __init__(self, value=None, **kwargs):
        super().__init__(**kwargs)
        self.value = value

    def accept(self, visitor):
        return visitor.visit_Await(self)


class AsyncFor(For):
    def __init__(self, target=None, iter=None, body=None, orelse=None, **kwargs):
        super().__init__(target, iter, body, orelse, **kwargs)

    def accept(self, visitor):
        return visitor.visit_AsyncFor(self)


class AsyncWith(With):
    def __init__(self, items=None, body=None, **kwargs):
        super().__init__(items, body, **kwargs)

    def accept(self, visitor):
        return visitor.visit_AsyncWith(self)


class Custom(BaseStatement):
    name: str
    expr: Optional[BaseExpression]
    body: list[BaseStatement]

    def __init__(
        self,
        name,
        expr: Optional[BaseExpression] = None,
        body=None,
        **kwargs,
    ):
        super().__init__(**kwargs)
        self.name = name
        self.expr = expr
        self.body = [cast(BaseStatement, i) for i in body] if body is not None else []

    def accept(self, visitor):
        return visitor.visit_Custom(self)


def parse(code):
    print(code)
    raise NotImplementedError()


def literal_eval(node_or_string):
    """
    Evaluate an expression node or a string containing only a Python
    expression.  The string or node provided may only consist of the following
    Python literal structures: strings, bytes, numbers, tuples, lists, dicts,
    sets, booleans, and None.

    Caution: A complex expression can overflow the C stack and cause a crash.
    """
    if isinstance(node_or_string, str):
        node_or_string = parse(node_or_string.lstrip(" \t"))

    def _raise_malformed_node(node):
        msg = "malformed node or string"
        if hasattr(node, "lineno"):
            msg += f" on line {node.lineno}"
        raise ValueError(msg + f": {node!r}")

    def _convert_num(node):
        if not isinstance(node, Num):
            _raise_malformed_node(node)
        return int(node.value)

    def _convert_signed_num(node):
        if isinstance(node, UnaryOp) and (
            node.op == UnaryOperator.UAdd or node.op == UnaryOperator.USub
        ):
            operand = _convert_num(node.operand)
            if isinstance(node.op, UnaryOperator.UAdd):
                return +operand
            else:
                return -operand
        return _convert_num(node)

    def _convert(node: AST):
        if isinstance(node, Constant):
            node: Constant = node
            return node.value
        elif isinstance(node, TupleEx):
            node: TupleEx = node
            return tuple(map(_convert, node.elts))
        elif isinstance(node, ListEx):
            node: ListEx = node
            return list(map(_convert, node.elts))
        elif isinstance(node, SetEx):
            node: SetEx = node
            return set(map(_convert, node.elts))
        elif (
            isinstance(node, Call)
            and isinstance(node.func, Name)
            and node.func.id == "set"
            and node.args == node.keywords == []
        ):
            return set()
        elif isinstance(node, DictEx):
            node: DictEx = node
            if len(node.keys) != len(node.values):
                _raise_malformed_node(node)
            return dict(zip(map(_convert, node.keys), map(_convert, node.values)))
        elif isinstance(node, BinOp) and (
            node.op == BinaryOperator.Add or node.op == BinaryOperator.Sub
        ):
            left = _convert_signed_num(node.left)
            right = _convert_num(node.right)
            if isinstance(left, (int, float)) and isinstance(right, complex):
                if isinstance(node.op, BinaryOperator.Add):
                    return left + right
                else:
                    return left - right
        return _convert_signed_num(node)

    return _convert(node_or_string)


def dump(
    node: AST,
    annotate_fields=True,
    include_attributes=False,
    indent: Optional[int] = None,
):
    """
    Return a formatted dump of the tree in node.  This is mainly useful for
    debugging purposes.  If annotate_fields is true (by default),
    the returned string will show the names and the values for fields.
    If annotate_fields is false, the result string will be more compact by
    omitting unambiguous field names.  Attributes such as line
    numbers and column offsets are not dumped by default.  If this is wanted,
    include_attributes can be set to true.  If indent is a non-negative
    integer or string, then the tree will be pretty-printed with that indent
    level. None (the default) selects the single line representation.
    """

    def _format(node: Any, level=0):
        if indent:
            level += 1
            prefix = "\n" + (" " * indent) * level
            sep = ",\n" + (" " * indent) * level
        else:
            prefix = ""
            sep = ", "
        if isinstance(node, AST):
            args = []
            allsimple = True
            for name, value in Codon.any_members(node):
                if name in ['attrs', 'loc']:
                    continue
                value, simple = _format(value, level)
                allsimple = allsimple and simple
                if annotate_fields and value:
                    args.append(f"{name}={value}")
                elif value:
                    args.append(value)
            if include_attributes and node.attrs:
                raise NotImplementedError("include_attributes")
            args = [arg for arg in args if arg]
            if allsimple and len(args) <= 3:
                return f"{class_name(node)}({', '.join(args)})", not args
            return f"{class_name(node)}({prefix}{sep.join(args)})", False
        if isinstance(node, str):
            if node == "":
                return "", True
            return f"'{string_escape(node)}'", True
        if isinstance(node, int):
            return f"{node}", True
        if isinstance(node, bool):
            return f"{node}", True
        else:
            if Any.is_tuple(node):
                return f"({prefix}{sep.join(_format(x, level)[0] for x in cast(List[Any], node))})", False
            elif Any.is_list(node):
                n = cast(List[Any], node)
                if not n:
                    return "", True
                return f"[{prefix}{sep.join(_format(x, level)[0] for x in n)}]", False
            elif Any.is_optional(node):
                n = cast(Optional[Any], node)
                if n is None:
                    return "", True
                else:
                    return _format(Codon.unwrap(n), level)
            return repr(node), True

    return _format(node)[0]


def fix_missing_locations(node: AST):
    """
    When you compile a node tree with compile(), the compiler expects lineno and
    col_offset attributes for every node that supports them.  This is rather
    tedious to fill in for generated nodes, so this helper adds these attributes
    recursively where not already set, by setting them to the values of the
    parent node.  It works recursively starting at *node*.
    """

    def _fix(node, lineno, col_offset, end_lineno, end_col_offset):
        if node.lineno != 0:
            lineno, col_offset, end_lineno, end_col_offset = tuple(node.loc)
        for child in iter_child_nodes(node):
            _fix(child, lineno, col_offset, end_lineno, end_col_offset)

    _fix(node, 1, 0, 1, 0)
    return node


def increment_lineno(node, n=1):
    """
    Increment the line number and end line number of each node in the tree
    starting at *node* by *n*. This is useful to "move code" to a different
    location in a file.
    """
    for child in walk(node):
        if "lineno" in child._attributes:
            child.lineno = getattr(child, "lineno", 0) + n
        if (
            "end_lineno" in child._attributes
            and (end_lineno := getattr(child, "end_lineno", 0)) is not None
        ):
            child.end_lineno = end_lineno + n
    return node


def iter_fields(node: AST) -> Generator[Tuple[str, Any]]:
    """
    Yield a tuple of ``(fieldname, value)`` for each field in ``node._fields``
    that is present on *node*.
    """

    for name, value in Codon.any_members(node):
        node = value
        if isinstance(value, AST):
            yield name, node
        elif Any.is_tuple(node) or Any.is_list(node) or Any.is_optional(node):
            yield name, node


def iter_child_nodes(node) -> Generator[AST]:
    """
    Yield all direct child nodes of *node*, that is, all fields that are nodes
    and all items of fields that are lists of nodes.
    """
    def _get_node(a: Any) -> Any:
        if Any.is_optional(a):
            o = cast(Optional[Any], a)
            if o: return Codon.unwrap(o)
        return a
    for _, field in iter_fields(node):
        f = _get_node(field)
        if isinstance(f, AST):
            yield f
        elif Any.is_list(f):
            for item in cast(List[Any], f):
                i = _get_node(item)
                if isinstance(i, AST):
                    yield i

def walk(node: AST):
    """
    Recursively yield all descendant nodes in the tree starting at *node*
    (including *node* itself), in no specified order.  This is useful if you
    only want to modify nodes in place and don't care about the context.
    """
    from collections import deque

    todo = deque([node])
    while todo:
        node = todo.popleft()
        todo.extend(iter_child_nodes(node))
        yield node


def string_escape(s):
    # Escape string as in repr.
    # See https://github.com/python/cpython/blob/ebe02e4f393bc0bd2263c43da313b28012f82af9/Objects/bytesobject.c#L1484

    res = []
    for c in s:
        if c == "'" or c == "\\":
            res.append(f"\\{c}")
        elif c == "\t":
            res.append("\\t")
        elif c == "\n":
            res.append("\\n")
        elif c == "\r":
            res.append("\\r")
        elif ord(c) < ord(" ") or ord(c) >= 0x7F:
            res.append(f"\\x{chr((ord(c) & 0xF0) >> 4)}{chr(ord(c) & 0xF)}")
        else:
            res.append(c)
    return "".join(res)


class _NodeVisitor[T]:
    def generic_visit(self, node) -> T:
        return T()

    def visit(self, node) -> T:
        return node.accept(self)

    def visit_AST(self, node): return self.generic_visit(node)
    def visit_BaseExpression(self, node): return self.generic_visit(node)
    def visit_BaseStatement(self, node): return self.generic_visit(node)
    def visit_Module(self, node): return self.generic_visit(node)
    def visit_Interactive(self, node): return self.generic_visit(node)
    def visit_Constant(self, node): return self.generic_visit(node)
    def visit_Bool(self, node): return self.generic_visit(node)
    def visit_Str(self, node): return self.generic_visit(node)
    def visit_Num(self, node): return self.generic_visit(node)
    def visit_Ellipsis(self, node): return self.generic_visit(node)
    def visit_NoneValue(self, node): return self.generic_visit(node)
    def visit_FormattedValue(self, node): return self.generic_visit(node)
    def visit_JoinedStr(self, node): return self.generic_visit(node)
    def visit_ListEx(self, node): return self.generic_visit(node)
    def visit_TupleEx(self, node): return self.generic_visit(node)
    def visit_SetEx(self, node): return self.generic_visit(node)
    def visit_DictEx(self, node): return self.generic_visit(node)
    def visit_Name(self, node): return self.generic_visit(node)
    def visit_Starred(self, node): return self.generic_visit(node)
    def visit_UnaryOp(self, node): return self.generic_visit(node)
    def visit_BinOp(self, node): return self.generic_visit(node)
    def visit_PipeOp(self, node): return self.generic_visit(node)
    def visit_BoolOp(self, node): return self.generic_visit(node)
    def visit_Compare(self, node): return self.generic_visit(node)
    def visit_Keyword(self, node): return self.generic_visit(node)
    def visit_Attribute(self, node): return self.generic_visit(node)
    def visit_Call(self, node): return self.generic_visit(node)
    def visit_PartialCall(self, node): return self.generic_visit(node)
    def visit_IfExp(self, node): return self.generic_visit(node)
    def visit_NamedExpr(self, node): return self.generic_visit(node)
    def visit_Subscript(self, node): return self.generic_visit(node)
    def visit_Slice(self, node): return self.generic_visit(node)
    def visit_Comprehension(self, node): return self.generic_visit(node)
    def visit_ListComp(self, node): return self.generic_visit(node)
    def visit_SetComp(self, node): return self.generic_visit(node)
    def visit_GeneratorExp(self, node): return self.generic_visit(node)
    def visit_DictComp(self, node): return self.generic_visit(node)
    def visit_Expr(self, node): return self.generic_visit(node)
    def visit_Assign(self, node): return self.generic_visit(node)
    def visit_AnnAssign(self, node): return self.generic_visit(node)
    def visit_AugAssign(self, node): return self.generic_visit(node)
    def visit_Raise(self, node): return self.generic_visit(node)
    def visit_Assert(self, node): return self.generic_visit(node)
    def visit_Delete(self, node): return self.generic_visit(node)
    def visit_Pass(self, node): return self.generic_visit(node)
    def visit_Alias(self, node): return self.generic_visit(node)
    def visit_Import(self, node): return self.generic_visit(node)
    def visit_ImportFrom(self, node): return self.generic_visit(node)
    def visit_If(self, node): return self.generic_visit(node)
    def visit_For(self, node): return self.generic_visit(node)
    def visit_While(self, node): return self.generic_visit(node)
    def visit_Break(self, node): return self.generic_visit(node)
    def visit_Continue(self, node): return self.generic_visit(node)
    def visit_ExceptHandler(self, node): return self.generic_visit(node)
    def visit_Try(self, node): return self.generic_visit(node)
    def visit_WithItem(self, node): return self.generic_visit(node)
    def visit_With(self, node): return self.generic_visit(node)
    def visit_MatchPattern(self, node): return self.generic_visit(node)
    def visit_MatchValue(self, node): return self.generic_visit(node)
    def visit_MatchSingleton(self, node): return self.generic_visit(node)
    def visit_MatchSequence(self, node): return self.generic_visit(node)
    def visit_MatchStar(self, node): return self.generic_visit(node)
    def visit_MatchMapping(self, node): return self.generic_visit(node)
    def visit_MatchClass(self, node): return self.generic_visit(node)
    def visit_MatchAs(self, node): return self.generic_visit(node)
    def visit_MatchOr(self, node): return self.generic_visit(node)
    def visit_MatchCase(self, node): return self.generic_visit(node)
    def visit_Match(self, node): return self.generic_visit(node)
    def visit_TypeVar(self, node): return self.generic_visit(node)
    def visit_Arg(self, node): return self.generic_visit(node)
    def visit_Arguments(self, node): return self.generic_visit(node)
    def visit_FunctionDef(self, node): return self.generic_visit(node)
    def visit_Lambda(self, node): return self.generic_visit(node)
    def visit_Return(self, node): return self.generic_visit(node)
    def visit_Yield(self, node): return self.generic_visit(node)
    def visit_YieldFrom(self, node): return self.generic_visit(node)
    def visit_Global(self, node): return self.generic_visit(node)
    def visit_Nonlocal(self, node): return self.generic_visit(node)
    def visit_ClassDef(self, node): return self.generic_visit(node)
    def visit_AsyncFunctionDef(self, node): return self.generic_visit(node)
    def visit_LLVMFunctionDef(self, node): return self.generic_visit(node)
    def visit_Await(self, node): return self.generic_visit(node)
    def visit_AsyncFor(self, node): return self.generic_visit(node)
    def visit_AsyncWith(self, node): return self.generic_visit(node)
    def visit_Custom(self, node): return self.generic_visit(node)


class NodeVisitor(_NodeVisitor[NoneType]):
    def generic_visit(self, node):
        for child in iter_child_nodes(node):
            child.accept(self)


# class ReplacementNodeVisitor(_NodeVisitor[Optional[AST]]):
#     def generic_visit(self, node) -> Optional[AST]:
#         for field, old_value in iter_fields(node):
#             if isinstance(old_value, unrealized_type[list]):
#                 new_values = []
#                 for value in old_value.list():
#                     new_value = self.visit(value)
#                     if new_value is None:
#                         continue
#                     elif isinstance(new_value, unrealized_type[list]):
#                         new_values.extend(new_value)
#                         continue
#                     new_values.append(new_value)
#                 setattr(node, field, new_values)
#             elif isinstance(old_value, AST):
#                 new_node = self.visit(old_value)
#                 setattr(node, field, new_node)
#         return node
