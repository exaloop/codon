#%% bool,barebones
a = True
print a.__class__.__name__ #: bool

#%% int,barebones
i = 15
print i.__class__.__name__ #: int

#%% float,barebones
a = 1.11
print a.__class__.__name__ #: float

#%% str,barebones
a = 'hi'
print a.__class__.__name__ #: str

#%% none_unbound,barebones
a = None

#%% list_unbound,barebones
a = []
#! cannot typecheck the program


#%% id_static,barebones
def foo[N: Static[int]]():
    print N
foo(5) #: 5

def fox(N: Static[int]):
    print N
fox(6) #: 6

#%% if,barebones
y = 1 if True else 2
print y.__class__.__name__ #: int

a = None
b = 5
z = a if bool(True) else b # needs bool to prevent static evaluation
print z, z.__class__.__name__ #: None Optional[int]

zz = 1.11 if True else None
print zz, zz.__class__.__name__ #: 1.11 float

#%% binary,barebones
@extend
class float:
    def __add__(self, i: int): print 'add'; return 0
    def __sub__(self, i: int): print 'sub'; return 0
    def __mul__(self, i: int): print 'mul'; return 0
    def __pow__(self, i: int): print 'pow'; return 0
    def __truediv__(self, i: int): print 'truediv'; return 0
    def __floordiv__(self, i: int): print 'div'; return 0
    def __matmul__(self, i: int): print 'matmul'; return 0
    def __mod__(self, i: int): print 'mod'; return 0
    def __lt__(self, i: int): print 'lt'; return 0
    def __le__(self, i: int): print 'le'; return 0
    def __gt__(self, i: int): print 'gt'; return 0
    def __ge__(self, i: int): print 'ge'; return 0
    def __eq__(self, i: int): print 'eq'; return 0
    def __ne__(self, i: int): print 'ne'; return 0
    def __lshift__(self, i: int): print 'lshift'; return 0
    def __rshift__(self, i: int): print 'rshift'; return 0
    def __and__(self, i: int): print 'and'; return 0
    def __or__(self, i: int): print 'or'; return 0
    def __xor__(self, i: int): print 'xor'; return 0
# double assignment to disable propagation
def f(x): return x
a = f(1.0)
a = f(5.0)
a + f(1) #: add
# wrap in function to disable canonicalization
a - f(1) #: sub
a * f(2) #: mul
a ** f(2) #: pow
a // f(2) #: div
a / f(2) #: truediv
a @ f(1) #: matmul
a % f(1) #: mod
a < f(1) #: lt
a <= f(1) #: le
a > f(1) #: gt
a >= f(1) #: ge
a == f(1) #: eq
a != f(1) #: ne
a << f(1) #: lshift
a >> f(1) #: rshift
a & f(1) #: and
a | f(1) #: or
a ^ f(1) #: xor

#%% binary_rmagic,barebones
class Foo[T]:
    def __add__(self, other: T):
        print 'add'
        return self
    def __radd__(self, other: T):
        print 'radd'
        return self
foo = Foo[int]()
foo + 1 #: add
1 + foo #: radd

#%% binary_short_circuit,barebones
def moo():
    print 'moo'
    return True
print True or moo() #: True
print moo() or True #: moo
#: True
print False and moo() #: False
print moo() and False #: moo
#: False

#%% binary_is,barebones
print 5 is None #: False
print None is None #: True
print (None if bool(True) else 1) is None #: True
print (None if bool(False) else 1) is None #: False

print 5 is 5.0 #: False
print 5 is 6 #: False
print 5 is 5 #: True
print 5 is 1.12 #: False
class Foo:
    a: int
x = Foo(1)
y = Foo(1)
z = x
print x is x, x is y, x is z, z is x, z is y #: True False True True False

a, b, c, d = Optional(5), Optional[int](), Optional(5), Optional(4)
print a is a, a is b, b is b, a is c, a is d #: True False True True False
aa, bb, cc, dd = Optional(Foo(1)), Optional[Foo](), Optional(Foo(1)), Optional(Foo(2))
print aa is aa, aa is bb, bb is bb, aa is cc, aa is dd #: True False True False False


#%% pipe,barebones
def foo(a, b):
    return a+b
bar = lambda c, d: c+d
def hai(e):
    while e > 0:
        yield e
        e -= 2
def echo(s):
    print s
foo(1,2) |> bar(4) |> echo  #: 7
foo(1,2) |> bar(4) |> hai |> echo
#: 7
#: 5
#: 3
#: 1

#%% pipe_prepend,barebones
def foo(a: Optional[int]):
    print a
    return 1
5 |> foo #: 5
None |> foo #: None
print (None |> foo).__class__.__name__ #: int

def foo2(a: int):
    print a
    return 1
Optional(5) |> foo2 #: 5
try:
    Optional[int]() |> foo2
except ValueError as e:
    print e.message #: optional is None

#%% pipe_prepend_error,barebones
def foo2(a: int):
    print a
    return 1
try:
    None |> foo2
except ValueError:
    print 'exception' #: exception
# Explanation: None can also be Optional[Generator[int]]
# We cannot decide if this is a generator to be unrolled in a pipe,
# or just an argument to be passed to a function.
# So this will default to NoneType at the end.

#%% instantiate_err,barebones
def foo[N]():
    return N()
foo(int, float)  #! foo() takes 1 arguments (2 given)

#%% instantiate_err_2,barebones
def foo[N, T]():
    return N()
foo(int)  #! generic 'T' not provided

#%% instantiate_err_3,barebones
Ptr[int, float]()  #! Ptr takes 1 generics (2 given)

#%% slice,barebones
z = [1, 2, 3, 4, 5]
y = (1, 'foo', True)
print z[2], y[1]  #: 3 foo
print z[:1], z[1:], z[1:3], z[:4:2], z[::-1]  #: [1] [2, 3, 4, 5] [2, 3] [1, 3] [5, 4, 3, 2, 1]

#%% static_index,barebones
a = (1, '2s', 3.3)
print a[1] #: 2s
print a[0:2], a[:2], a[1:] #: (1, '2s') (1, '2s') ('2s', 3.3)
print a[0:3:2], a[-1:] #: (1, 3.3) (3.3,)

#%% static_index_side,barebones
def foo(a):
    print(a)
    return a

print (foo(2), foo(1))[::-1]
#: 2
#: 1
#: (1, 2)
print (foo(1), foo(2), foo(3), foo(4))[2]
#: 1
#: 2
#: 3
#: 4
#: 3

#%% static_index_lenient,barebones
a = (1, 2)
print a[3:5] #: ()

#%% static_index_err,barebones
a = (1, 2)
a[5] #! tuple index out of range (expected 0..1, got instead 5)

#%% static_index_err_2,barebones
a = (1, 2)
a[-3] #! tuple index out of range (expected 0..1, got instead -1)

#%% index_func_instantiate,barebones
class X:
    def foo[T](self, x: T):
        print x.__class__.__name__, x
x = X()
x.foo(5, int) #: int 5

#%% index,barebones
l = [1, 2, 3]
print l[2] #: 3

#%% index_two_rounds,barebones
l = []
print l[::-1] #: []
l.append(('str', 1, True, 5.15))
print l, l.__class__.__name__ #: [('str', 1, True, 5.15)] List[Tuple[str,int,bool,float]]

#%% dot_case_1,barebones
a = []
print a[0].loop()  #! 'int' object has no attribute 'loop'
a.append(5)

#%% dot_case_2,barebones
a = Optional(0)
print a.__bool__() #: False
print a.__add__(1) #: 1

#%% dot_case_4,barebones
a = [5]
print a.len #: 1

#%% dot_case_4_err,barebones
a = [5]
a.foo #! 'List[int]' object has no attribute 'foo'

#%% dot_case_6,barebones
# Did heavy changes to this testcase because
# of the automatic optional wraps/unwraps and promotions
class Foo:
    def bar(self, a):
        print 'generic', a, a.__class__.__name__
    def bar(self, a: Optional[float]):
        print 'optional', a
    def bar(self, a: int):
        print 'normal', a
f = Foo()
f.bar(1) #: normal 1
f.bar(1.1) #: optional 1.1
f.bar(Optional('s')) #: generic s Optional[str]
f.bar('hehe') #: generic hehe str


#%% dot_case_6b,barebones
class Foo:
    def bar(self, a, b):
        print '1', a, b
    def bar(self, a, b: str):
        print '2', a, b
    def bar(self, a: str, b):
        print '3', a, b
f = Foo()
# Take the newest highest scoring method
f.bar('s', 't') #: 3 s t
f.bar(1, 't') #: 2 1 t
f.bar('s', 1) #: 3 s 1
f.bar(1, 2) #: 1 1 2

#%% dot,barebones
class Foo:
    def clsmethod():
        print 'foo'
    def method(self, a):
        print a
Foo.clsmethod() #: foo
Foo.method(Foo(), 1) #: 1
m1 = Foo.method
m1(Foo(), 's') #: s
m2 = Foo().method
m2(1.1) #: 1.1

#%% dot_error_static,barebones
class Foo:
    def clsmethod():
        print 'foo'
    def method(self, a):
        print a
Foo().clsmethod() #! 'Foo' object has no method 'clsmethod' with arguments (Foo)

#%% call,barebones
def foo(a, b, c='hi'):
    print 'foo', a, b, c
    return 1
class Foo:
    def __init__(self):
        print 'Foo.__init__'
    def foo(self, a):
        print 'Foo.foo', a
        return 's'
    def bar[T](self, a: T):
        print 'Foo.bar', a
        return a.__class__.__name__
    def __call__(self, y):
        print 'Foo.__call__'
        return foo(2, y)

foo(1, 2.2, True) #: foo 1 2.2 True
foo(1, 2.2) #: foo 1 2.2 hi
foo(b=2.2, a=1) #: foo 1 2.2 hi
foo(b=2.2, c=12u, a=1) #: foo 1 2.2 12

f = Foo() #: Foo.__init__
print f.foo(a=5) #: Foo.foo 5
#: s
print f.bar(a=1, T=int) #: Foo.bar 1
#: int
print Foo.bar(Foo(), 1.1, T=float) #: Foo.__init__
#: Foo.bar 1.1
#: float
print Foo.bar(Foo(), 's') #: Foo.__init__
#: Foo.bar s
#: str
print f('hahaha') #: Foo.__call__
#: foo 2 hahaha hi
#: 1

@tuple
class Moo:
    moo: int
    def __new__(i: int) -> Moo:
        print 'Moo.__new__'
        return (i,)
print Moo(1) #: Moo.__new__
#: (moo: 1)

#%% call_err_2,barebones
class A:
    a: A
a = A() #! argument 'a' has recursive default value

#%% call_err_3,barebones
class G[T]:
    t: T
class A:
    ga: G[A]
a = A() #! argument 'ga' has recursive default value

#%% call_err_4,barebones
seq_print_full(1, name="56", name=2) #! keyword argument repeated: name

#%% call_partial,barebones
def foo(i, j, k):
    return i + j + k
print foo(1.1, 2.2, 3.3)  #: 6.6
p = foo(6, ...)
print p.__class__.__name__ #: foo[int,...,...]
print p(2, 1)  #: 9
print p(k=3, j=6) #: 15
q = p(k=1, ...)
print q(3)  #: 10
qq = q(2, ...)
print qq()  #: 9
#
add_two = foo(3, k=-1, ...)
print add_two(42)  #: 44
print 3 |> foo(1, 2)  #: 6
print 42 |> add_two  #: 44
#
def moo(a, b, c=3):
    print a, b, c
m = moo(b=2, ...)
print m.__class__.__name__ #: moo[...,int,...]
m('s', 1.1) #: s 2 1.1
# #
n = m(c=2.2, ...)
print n.__class__.__name__ #: moo[...,int,float]
n('x') #: x 2 2.2
print n('y').__class__.__name__ #: NoneType

def ff(a, b, c):
    return a, b, c
print ff(1.1, 2, True).__class__.__name__ #: Tuple[float,int,bool]
print ff(1.1, ...)(2, True).__class__.__name__ #: Tuple[float,int,bool]
y = ff(1.1, ...)(c=True, ...)
print y.__class__.__name__ #: ff[float,...,bool]
print ff(1.1, ...)(2, ...)(True).__class__.__name__ #: Tuple[float,int,bool]
print y('hei').__class__.__name__ #: Tuple[float,str,bool]
z = ff(1.1, ...)(c='s', ...)
print z.__class__.__name__ #: ff[float,...,str]

def fx(*args, **kw):
    print(args, kw)
f1 = fx(1, x=1, ...)
f2 = f1(2, y=2, ...)
f3 = f2(3, z=3, ...)
f3()
#: (1, 2, 3) (x: 1, y: 2, z: 3)

#%% call_arguments_partial,barebones
def doo[R, T](a: Callable[[T], R], b: Generator[T], c: Optional[T], d: T):
    print R.__class__.__name__, T.__class__.__name__
    print a.__class__.__name__[:8], b.__class__.__name__
    for i in b:
        print a(i)
    print c, c.__class__.__name__
    print d, d.__class__.__name__

l = [1, 2, 3]
doo(b=l, d=Optional(5), c=l[0], a=lambda x: x+1)
#: int int
#: ._lambda Generator[int]
#: 2
#: 3
#: 4
#: 1 Optional[int]
#: 5 int

l = [1]
def adder(a, b): return a+b
doo(b=l, d=Optional(5), c=l[0], a=adder(b=4, ...))
#: int int
#: adder[.. Generator[int]
#: 5
#: 1 Optional[int]
#: 5 int

#%% call_partial_star,barebones
def foo(x, *args, **kwargs):
    print x, args, kwargs
p = foo(...)
p(1, z=5) #: 1 () (z: 5)
p('s', zh=65) #: s () (zh: 65)
q = p(zh=43, ...)
q(1) #: 1 () (zh: 43)
r = q(5, 38, ...)
r() #: 5 (38,) (zh: 43)
r(1, a=1) #: 5 (38, 1) (zh: 43, a: 1)

#%% call_args_kwargs_type,barebones
def foo(*args: float, **kwargs: int):
    print(args, kwargs, args.__class__.__name__)

foo(1, f=1)  #: (1,) (f: 1) Tuple[float]
foo(1, 2.1, 3, z=2)  #: (1, 2.1, 3) (z: 2) Tuple[float,float,float]

def sum(x: Generator[int]):
    a = 0
    for i in x:
        a += i
    return a

def sum_gens(*x: Generator[int]) -> int:
    a = 0
    for i in x:
        a += sum(i)
    return a
print sum_gens([1, 2, 3])  #: 6
print sum_gens({1, 2, 3})  #: 6
print sum_gens(iter([1, 2, 3]))  #: 6

#%% call_kwargs,barebones
def kwhatever(**kwargs):
    print 'k', kwargs
def whatever(*args):
    print 'a', args
def foo(a, b, c=1, *args, **kwargs):
    print a, b, c, args, kwargs
    whatever(a, b, *args, c)
    kwhatever(x=1, **kwargs)
foo(1, 2, 3, 4, 5, arg1='s', kwa=2)
#: 1 2 3 (4, 5) (arg1: 's', kwa: 2)
#: a (1, 2, 4, 5, 3)
#: k (arg1: 's', kwa: 2, x: 1)
foo(1, 2)
#: 1 2 1 () ()
#: a (1, 2, 1)
#: k (x: 1)
foo(1, 2, 3)
#: 1 2 3 () ()
#: a (1, 2, 3)
#: k (x: 1)
foo(1, 2, 3, 4)
#: 1 2 3 (4,) ()
#: a (1, 2, 4, 3)
#: k (x: 1)
foo(1, 2, zamboni=3)
#: 1 2 1 () (zamboni: 3)
#: a (1, 2, 1)
#: k (x: 1, zamboni: 3)

#%% call_unpack,barebones
def foo(*args, **kwargs):
    print args, kwargs

@tuple
class Foo:
    x: int = 5
    y: bool = True

t = (1, 's')
f = Foo(6)
foo(*t, **f) #: (1, 's') (x: 6, y: True)
foo(*(1,2)) #: (1, 2) ()
foo(3, f) #: (3, (x: 6, y: True)) ()
foo(k = 3, **f) #: () (k: 3, x: 6, y: True)

#%% call_partial_args_kwargs,barebones
def foo(*args):
    print(args)
a = foo(1, 2, ...)
b = a(3, 4, ...)
c = b(5, ...)
c('zooooo')
#: (1, 2, 3, 4, 5, 'zooooo')

def fox(*args, **kwargs):
    print(args, kwargs)
xa = fox(1, 2, x=5, ...)
xb = xa(3, 4, q=6, ...)
xc = xb(5, ...)
xd = xc(z=5.1, ...)
xd('zooooo', w='lele')
#: (1, 2, 3, 4, 5, 'zooooo') (x: 5, q: 6, z: 5.1, w: 'lele')

class Foo:
    i: int
    def __str__(self):
        return f'#{self.i}'
    def foo(self, a):
        return f'{self}:generic'
    def foo(self, a: float):
        return f'{self}:float'
    def foo(self, a: int):
        return f'{self}:int'
f = Foo(4)

def pacman(x, f):
    print f(x, '5')
    print f(x, 2.1)
    print f(x, 4)
pacman(f, Foo.foo)
#: #4:generic
#: #4:float
#: #4:int

def macman(f):
    print f('5')
    print f(2.1)
    print f(4)
macman(f.foo)
#: #4:generic
#: #4:float
#: #4:int

class Fox:
    i: int
    def __str__(self):
        return f'#{self.i}'
    def foo(self, a, b):
        return f'{self}:generic b={b}'
    def foo(self, a: float, c):
        return f'{self}:float, c={c}'
    def foo(self, a: int):
        return f'{self}:int'
    def foo(self, a: int, z, q):
        return f'{self}:int z={z} q={q}'
ff = Fox(5)
def maxman(f):
    print f('5', b=1)
    print f(2.1, 3)
    print f(4)
    print f(5, 1, q=3)
maxman(ff.foo)
#: #5:generic b=1
#: #5:float, c=3
#: #5:int
#: #5:int z=1 q=3


#%% call_static,barebones
print isinstance(1, int), isinstance(2.2, float), isinstance(3, bool)
#: True True False
print isinstance((1, 2), Tuple), isinstance((1, 2), Tuple[int, int]), isinstance((1, 2), Tuple[float, int])
#: True True False
print isinstance([1, 2], List), isinstance([1, 2], List[int]), isinstance([1, 2], List[float])
#: True True False
print isinstance({1, 2}, List), isinstance({1, 2}, Set[float])
#: False False
print isinstance(Optional(5), Optional[int]), isinstance(Optional(), Optional)
#: True True
print isinstance(Optional(), Optional[int]), isinstance(Optional('s'), Optional[int])
#: False False
print isinstance(None, Optional), isinstance(None, Optional[int])
#: True False
print isinstance(None, Optional[NoneType])
#: True
print isinstance({1, 2}, List)
#: False

print staticlen((1, 2, 3)), staticlen((1, )), staticlen('hehe')
#: 3 1 2

print hasattr([1, 2], "__getitem__")
#: True
print hasattr(type([1, 2]), "__getitem__")
#: True
print hasattr(int, "__getitem__")
#: False
print hasattr([1, 2], "__getitem__", str)
#: False

#%% isinstance_inheritance,barebones
class AX[T]:
    a: T
    def __init__(self, a: T):
        self.a = a
class Side:
    def __init__(self):
        pass
class BX[T,U](Static[AX[T]], Static[Side]):
    b: U
    def __init__(self, a: T, b: U):
        super().__init__(a)
        self.b = b
class CX[T,U](Static[BX[T,U]]):
    c: int
    def __init__(self, a: T, b: U):
        super().__init__(a, b)
        self.c = 1
c = CX('a', False)
print isinstance(c, CX), isinstance(c, BX), isinstance(c, AX), isinstance(c, Side)
#: True True True True
print isinstance(c, BX[str, bool]), isinstance(c, BX[str, str]), isinstance(c, AX[int])
#: True False False

#%% staticlen_err,barebones
print staticlen([1, 2]) #! expected tuple type

#%% compile_error,barebones
compile_error("woo-hoo") #! woo-hoo

#%% stack_alloc,barebones
a = __array__[int](2)
print a.__class__.__name__ #: Array[int]

#%% typeof,barebones
a = 5
z = []
z.append(6)
print z.__class__.__name__, z, type(1.1).__class__.__name__  #: List[int] [6] float

#%% ptr,barebones
v = 5
c = __ptr__(v)
print c.__class__.__name__ #: Ptr[int]

#%% yieldexpr,barebones
def mysum(start):
    m = start
    while True:
        a = (yield)
        print a.__class__.__name__ #: int
        if a == -1:
            break
        m += a
    yield m
iadder = mysum(0)
next(iadder)
for i in range(10):
    iadder.send(i)
#: int
#: int
#: int
#: int
#: int
#: int
#: int
#: int
#: int
#: int
print iadder.send(-1)  #: 45

#%% function_typecheck_level,barebones
def foo(x):
    def bar(z):  # bar has a parent foo(), however its unbounds must not be generalized!
        print z
    bar(x)
    bar('x')
foo(1)
#: 1
#: x
foo('s')
#: s
#: x

#%% tuple_generator,barebones
a = (1, 2)
b = ('f', 'g')
print a, b #: (1, 2) ('f', 'g')
c = (*a, True, *b)
print c #: (1, 2, True, 'f', 'g')
print a + b + c #: (1, 2, 'f', 'g', 1, 2, True, 'f', 'g')
print () + (1, ) + ('a', 'b') #: (1, 'a', 'b')

t = tuple(i+1 for i in (1,2,3))
print t #: (2, 3, 4)
print tuple((j, i) for i, j in ((1, 'a'), (2, 'b'), (3, 'c')))
#: (('a', 1), ('b', 2), ('c', 3))

#%% tuple_fn,barebones
@tuple
class unpackable_plain:
  a: int
  b: str

u = unpackable_plain(1, 'str')
a, b = tuple(u)
print a, b #: 1 str

@tuple
class unpackable_gen:
  a: int
  b: T
  T: type

u2 = unpackable_gen(1, 'str')
a2, b2 = tuple(u2)
print a2,b2  #: 1 str

class plain:
  a: int
  b: str

c = plain(3, 'heh')
z = tuple(c)
print z, z.__class__.__name__  #: (3, 'heh') Tuple[int,str]

#%% static_unify,barebones
def foo(x: Callable[[1,2], 3]): pass  #! '2' does not match expected type 'T1'

#%% static_unify_2,barebones
def foo(x: List[1]): pass  #! '1' does not match expected type 'T'

#%% super,barebones
class A[T]:
    a: T
    def __init__(self, t: T):
        self.a = t
    def foo(self):
        return f'A:{self.a}'
class B(Static[A[str]]):
    b: int
    def __init__(self):
        super().__init__('s')
        self.b = 6
    def baz(self):
        return f'{super().foo()}::{self.b}'
b = B()
print b.foo() #: A:s
print b.baz() #: A:s::6

class AX[T]:
    a: T
    def __init__(self, a: T):
        self.a = a
    def foo(self):
        return f'[AX:{self.a}]'
class BX[T,U](Static[AX[T]]):
    b: U
    def __init__(self, a: T, b: U):
        print super().__class__.__name__
        super().__init__(a)
        self.b = b
    def foo(self):
        return f'[BX:{super().foo()}:{self.b}]'
class CX[T,U](Static[BX[T,U]]):
    c: int
    def __init__(self, a: T, b: U):
        print super().__class__.__name__
        super().__init__(a, b)
        self.c = 1
    def foo(self):
        return f'CX:{super().foo()}:{self.c}'
c = CX('a', False)
print c.__class__.__name__, c.foo()
#: BX[str,bool]
#: AX[str]
#: CX[str,bool] CX:[BX:[AX:a]:False]:1

#%% super_vtable_2
class Base:
    def test(self):
        print('base.test')
class A(Base):
    def test(self):
        super().test()
        Base.test(self)
        print('a.test')
a = A()
a.test()
def moo(x: Base):
    x.test()
moo(a)
Base.test(a)
#: base.test
#: base.test
#: a.test
#: base.test
#: base.test
#: a.test
#: base.test

#%% super_tuple,barebones
@tuple
class A[T]:
    a: T
    x: int
    def __new__(a: T) -> A[T]:
        return (a, 1)
    def foo(self):
        return f'A:{self.a}'
@tuple
class B(Static[A[str]]):
    b: int
    def __new__() -> B:
        return (*(A('s')), 6)
    def baz(self):
        return f'{super().foo()}::{self.b}'

b = B()
print b.foo() #: A:s
print b.baz() #: A:s::6


#%% super_error,barebones
class A:
    def __init__(self):
        super().__init__()
a = A()
#! no super methods found
#! during the realization of __init__(self: A)

#%% super_error_2,barebones
super().foo(1) #! no super methods found
