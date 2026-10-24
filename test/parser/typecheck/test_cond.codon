
#%% cond,barebones
a = 5
print (1 <= a <= 10), (1 >= a >= -5) #: True False

#%% if_expr,barebones
c = 5
a = 1 if c < 5 else 2
b = -(1 if c else 2)
print a, b #: 2 -1



#%% nullable_expression_join
# Conditional expressions and explicit returns use the same nullable target.
def select(flag: bool, left, right):
    return left if flag else right

for flag in (True, False):
    left_none = select(flag, NoneType(), 42)
    right_none = select(flag, 42, NoneType())
    assert type(left_none) is Optional[int]
    assert type(right_none) is Optional[int]
    if flag:
        assert left_none is None and right_none == 42
    else:
        assert left_none == 42 and right_none is None

    existing: Optional[int] = 7
    assert type(select(flag, NoneType(), existing)) is Optional[int]
    assert type(select(flag, existing, NoneType())) is Optional[int]
    assert select(flag, existing, 42) == (7 if flag else 42)
    assert select(flag, 42, existing) == (42 if flag else 7)
    assert type(select(flag, NoneType(), NoneType())) is NoneType

print('nullable expressions ok')  #: nullable expressions ok

#%% range_err,barebones
1 ... 3 #! unexpected range expression

#%% match
def foo(x):
    match x:
        case 1:
            print 'int'
        case 2 ... 10:
            print 'range'
        case 'ACGT':
            print 'string'
        case (a, 1):
            print 'tuple_wild', a
        case []:
            print 'list'
        case [[]]:
            print 'list list'
        case [1, 2]:
            print 'list 2'
        case [1, z, ...] if z < 5:
            print 'list 3', z
        case [1, _, ..., zz] | (1, zz):
            print 'list 4', zz
        case (1 ... 10, s := ('ACGT', 1 ... 4)):
            print 'complex', s
        case _:
            print 'else'
foo(1) #: int
foo(5) #: range
foo('ACGT') #: string
foo((9, 1)) #: tuple_wild 9
foo(List[int]()) #: list
foo([List[int]()]) #: list list
foo([1, 2]) #: list 2
foo([1, 3]) #: list 3 3
foo([1, 5]) #: else
foo([1, 5, 10]) #: list 4 10
foo((1, 33)) #: list 4 33
foo((9, ('ACGT', 3))) #: complex ('ACGT', 3)
foo(range(10)) #: else

for op in 'MI=DXSN':
    match op:
        case 'M' | '=' | 'X':
            print('case 1')
        case 'I' or 'S':
            print('case 2')
        case _:
            print('case 3')
#: case 1
#: case 2
#: case 1
#: case 3
#: case 1
#: case 2
#: case 3

v = i32(10)
match v:
    case i32(10): print("ok")  #: ok
    case _: print("nope")

@dataclass(eq=True)
class Foo:
    i: int
f = Foo(5)
match f:
    case 5: print("ok")
    case _: print("nope")  #: nope
match f:
    case 5: print("ok")
    case Foo(5): print("okish")  #: okish
    case _: print("nope")

class FooX:
    i: int
    def __match__(self, x):
        print("in match")
        return str(self.i) == x
f = FooX(5)
#: in match
match f:
    case "5": print("ok")  #: ok
    case _: print("nope")

#%% match_err_1,barebones
match [1, 2]:
    case [1, ..., 2, ..., 3]: pass
#! multiple ellipses in a pattern

#%% match_polymorphic_nodes,barebones
# Check derived types before reading their fields, including
# nested class/list patterns and an `as` binding with the narrowed type.
class Node:
    pass
class Expr(Node):
    pass
class IdExpr(Expr):
    value: str
class SpecialId(IdExpr):
    extra: int
class NoneExpr(Expr):
    pass
class Stmt(Node):
    pass
class ExprStmt(Stmt):
    expr: Expr
class SuiteStmt(Stmt):
    items: List[Stmt]
class ForStmt(Stmt):
    suite: Stmt

def block(first: Stmt, second: Optional[Stmt] = None):
    items = [first]
    if second is not None:
        items.append(unwrap(second))
    return SuiteStmt(items)

def describe(node: Node) -> str:
    match node:
        case SuiteStmt(items=[*_, ExprStmt(expr=IdExpr(value="x" | "y"))]) as suite:
            return "named " + str(len(suite.items))
        case SuiteStmt(items=[*_, ExprStmt(expr=e)]) as suite if len(suite.items) > 1:
            return "expression " + str(len(suite.items))
        case ForStmt(suite=s):
            return describe(s)
        case SuiteStmt(items=[*_, s]):
            return describe(s)
        case ExprStmt(expr=e):
            return describe(e)
        case IdExpr(value=value):
            return value
        case NoneExpr():
            return "none"
        case _:
            return "other"

assert describe(block(ExprStmt(IdExpr("x")))) == "named 1"
assert describe(block(ExprStmt(IdExpr("a")), ExprStmt(IdExpr("z")))) == "expression 2"
assert describe(ForStmt(block(ForStmt(ExprStmt(IdExpr("q")))))) == "q"
assert describe(SuiteStmt(List[Stmt]())) == "other"
assert describe(SpecialId(value="derived", extra=7)) == "derived"
assert describe(NoneExpr()) == "none"
assert describe(Node()) == "other"
print("polymorphic patterns ok")  #: polymorphic patterns ok

#%% match_class_pattern_effects,barebones
# Class patterns inspect objects rather than constructing them; failed fields and
# guards must short-circuit, and each attempted attribute is evaluated once.
events = List[str]()
class Base:
    pass
class Leaf(Base):
    value: int
    def __init__(self, value: int):
        events.append("init")
        self.value = value
    @property
    def payload(self):
        events.append("payload")
        return self.value
    @property
    def unreachable(self):
        events.append("unreachable")
        return self.value

def source() -> Base:
    events.append("source")
    return Leaf(7)

match source():
    case Leaf(payload=1, unreachable=_):
        assert False
    case Leaf(payload=value) if value < 0:
        assert False
    case Leaf(payload=value) as leaf:
        assert leaf.value == value == 7
        events.append(str(value))
    case _:
        assert False
assert events == ["source", "init", "payload", "payload", "payload", "7"]

def missing(value: Base):
    match value:
        case Leaf(absent=_):
            return "absent"
        case Leaf():
            return "leaf"
        case _:
            return "base"
assert missing(Leaf(3)) == "leaf"
assert missing(Base()) == "base"
print("class pattern effects ok")  #: class pattern effects ok

#%% match_capture_scope,barebones
class Base:
    pass
class Number(Base):
    value: int
    suffix: str
class Text(Base):
    value: str
    suffix: str

def render(node: Base):
    match node:
        case Number(value=value, suffix=suffix):
            suffix = "" if not suffix else " " + suffix
            return str(value + 1) + suffix
        case Text(value=value, suffix=suffix):
            suffix = "" if not suffix else " " + suffix
            return value.upper() + suffix
        case _:
            return "other"
assert render(Number(3, "int")) == "4 int"
assert render(Text("hello", "str")) == "HELLO str"
assert render(Base()) == "other"

def captured_after(node: Base):
    match node:
        case Number(suffix=suffix):
            pass
        case Text(suffix=suffix):
            pass
    try:
        return suffix
    except NameError:
        return "unbound"
assert captured_after(Number(1, "number")) == "number"
assert captured_after(Text("a", "text")) == "text"
assert captured_after(Base()) == "unbound"
print("capture scope ok")  #: capture scope ok

#%% match_optional_nodes,barebones
class Expr:
    pass
class IdExpr(Expr):
    value: str
class IndexExpr(Expr):
    expr: Optional[Expr]

def inspect(value: Optional[Expr]):
    match value:
        case IndexExpr(expr=IdExpr(value="ClassVar")) as node:
            assert node.expr is not None
            return "class variable"
        case IdExpr(value=name) as node:
            assert node.value == name
            return name
        case None:
            return "none"
        case _:
            return "other"
assert inspect(IdExpr("name")) == "name"
assert inspect(IndexExpr(IdExpr("ClassVar"))) == "class variable"
assert inspect(IndexExpr(None)) == "other"
assert inspect(None) == "none"

def boxed(value: Any):
    match value:
        case IdExpr(value=name) as node:
            assert node.value == name
            return name
        case _:
            return "other"
assert boxed(IdExpr("boxed")) == "boxed"
assert boxed(1) == "other"
print("optional patterns ok")  #: optional patterns ok

#%% match_function_keyword_pattern_err,barebones
# Function-call patterns must be rejected during compilation.
def identity(value):
    return value
def compare(value):
    match value:
        case identity(value=expected):
            return True
        case _:
            return False
compare(7)
#! called match pattern must be a class
#! during the realization of compare(value: int)

#%% match_function_positional_pattern_err,barebones
def identity(value):
    return value
match 7:
    case identity(7): pass
#! called match pattern must be a class

#%% match_star_wildcard,barebones
def sequence(value):
    match value:
        case [1, *_, 3]:
            return "edges"
        case [*_, last]:
            return str(last)
        case [*_]:
            return "empty"
        case _:
            return "other"
assert sequence([1, 3]) == "edges"
assert sequence([1, 2, 2, 3]) == "edges"
assert sequence([8]) == "8"
assert sequence(List[int]()) == "empty"
assert sequence(1) == "other"

def tuple_sequence(value):
    match value:
        case (1, *_, last):
            return str(last)
        case (*_,):
            return "tuple"
        case _:
            return "other"
assert tuple_sequence((1, "tail")) == "tail"
assert tuple_sequence((1, 2, "tail")) == "tail"
assert tuple_sequence((1,)) == "tuple"
assert tuple_sequence(()) == "tuple"
assert tuple_sequence(1) == "other"
print("star wildcards ok")  #: star wildcards ok

#%% match_multi_star_err,barebones
match [1, 2]:
    case [*_, *_]: pass
#! multiple ellipses in a pattern

#%% if_expr_2,barebones
y = 1 if True else 2
print y.__class__.__name__ #: int

a = None
b = 5
z = a if bool(True) else b # needs bool to prevent static evaluation
print z, z.__class__.__name__ #: None Optional[int]

zz = 1.11 if True else None
print zz, zz.__class__.__name__ #: 1.11 float

#%% if,barebones
for a, b in [(1, 2), (3, 3), (5, 4)]:
    if a > b:
        print '1',
    elif a == b:
        print '=',
    else:
        print '2',
print '_'  #: 2 = 1 _

if 1:
    print '1' #: 1

#%% static_if,barebones
def foo(x, N: Literal[int]):
    if isinstance(x, int):
        return x + 1
    elif isinstance(x, float):
        return x.__pow__(.5)
    elif isinstance(x, Tuple[int, str]):
        return f'foo: {x[1]}'
    elif isinstance(x, Tuple) and (N >= 3 or static.len(x) > 2):
        return x[2:]
    elif hasattr(x, '__len__'):
        return 'len ' + str(x.__len__())
    else:
        compile_error('invalid type')
print foo(N=1, x=1) #: 2
print foo(N=1, x=2.0) #: 1.4142135623730951
print foo(N=1, x=(1, 'bar')) #: foo: bar
print foo(N=1, x=(1, 2)) #: len 2
print foo(N=3, x=(1, 2)) #: ()
print foo(N=1, x=(1, 2, 3)) #: (3,)

