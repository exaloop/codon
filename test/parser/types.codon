#%% basic,barebones
a = 5
b: float = 6.16
c: optional[str] = None
print a, b, c  #: 5 6.16 None

#%% late_unify,barebones
a = []
a.append(1)
print a  #: [1]
print [1]+[1]  #: [1, 1]

#%% late_unify_2,barebones
class XX[T]:
    y: T
a = XX()
def f(i: int) -> int:
    return i
print a.y.__class__.__name__ #: int
f(a.y)
print a.__class__.__name__ #: XX[int]
print XX[bool].__class__.__name__ #: XX[bool]

#%% nested_generic,barebones
x = Array[Array[int]](0)
f = Optional[Optional[Optional[int]]](Optional[Optional[int]](Optional[int](5)))
print x.len, f  #: 0 5

#%% map_unify
def map[T,S](l: List[T], f: Callable[[T], S]):
    return [f(x) for x in l]
e = 1
print map([1, 2, 3], lambda x: x+e)  #: [2, 3, 4]

def map2(l, f):
    return [f(x) for x in l]
print map2([1, 2, 3], lambda x: x+e)  #: [2, 3, 4]

#%% nested,barebones
def m4[TD](a: int, d: TD):
    def m5[TD,TE](a: int, d: TD, e: TE):
        print a, d, e
    m5(a, d, 1.12)
m4(1, 's')  #: 1 s 1.12
m4(1, True)  #: 1 True 1.12

#%% nested_class,barebones
class A[TA]:
    a: TA
    # lots of nesting:
    def m4[TD](self: A[TA], d: TD):
        def m5[TA,TD,TE](a: TA, d: TD, e: TE):
            print a, d, e
        m5(self.a, d, d)
ax = A(42)
ax.m4(1)  #: 42 1 1

#%% static_fn,barebones
class A[TA]:
    a: TA
    def dump(a, b, c):
        print a, b, c
    def m2():
        A.dump(1, 2, 's')
    def __str__(self):
        return 'A'
A.dump(1, 2, 3)  #: 1 2 3
A[int].m2()  #: 1 2 s
A.m2()  #: 1 2 s
c = A[str]('s')
c.dump('y', 1.1)  #: A y 1.1

#%% static_fn_overload,barebones
def foo(x: Static[int]):
    print('int', x)

@overload
def foo(x: Static[str]):
    print('str', x)

foo(10)
#: int 10
foo('s')
#: str s

#%% realization_big
class A[TA,TB,TC]:
    a: TA
    b: TB
    c: TC

    def dump(a, b, c):
        print a, b, c

    # non-generic method:
    def m0(self: A[TA,TB,TC], a: int):
        print a

    # basic generics:
    def m1[X](self: A[TA,TB,TC], other: A[X,X,X]):
        print other.a, other.b, other.c

    # non-generic method referencing outer generics:
    def m2(a: TA, b: TB, c: TC):
        A.dump(a, b, c)

    # generic args:
    def m3(self, other):
        return self.a

    # lots of nesting:
    def m4[TD](self: A[TA,TB,TC], d: TD):
        def m5[TA,TB,TC,TD,TE](a: TA, b: TB, c: TC, d: TD, e: TE):
            print a, b, c, d, e
        m5(self.a, self.b, self.c, d, d)

    # instantiating the type:
    def m5(self):
        x = A(self.a, self.b, self.c)
        A.dump(x.a, x.b, x.c)

    # deeply nested generic type:
    def m6[T](v: array[array[array[T]]]):
        return v[0][0][0]
a1 = A(42, 3.14, "hello")
a2 = A(1, 2, 3)
a1.m1(a2)                           #: 1 2 3
A[int,float,str].m2(1, 1.0, "one")  #: 1 1 one
A[int,int,int].m2(11, 22, 33)       #: 11 22 33
print a1.m3(a2)                     #: 42
print a1.m3(a2)                     #: 42
print a2.m3(a1)                     #: 1
a1.m4(True)                         #: 42 3.14 hello True True
a1.m4([1])                          #: 42 3.14 hello [1] [1]
a2.m4("x")                          #: 1 2 3 x x
a1.m5()                             #: 42 3.14 hello
a2.m5()                             #: 1 2 3

v1 = array[array[array[str]]](1)
v2 = array[array[str]](1)
v3 = array[str](1)
v1[0] = v2
v2[0] = v3
v3[0] = "world"
print A.m6(v1)                      #: world

f = a2.m0
f(99)                               #: 99

#%% realization_small,barebones
class B1[T]:
    a: T
    def foo[S](self: S) -> B1[int]:
        return B1[int](111)
b1 = B1[bool](True).foo()
print b1.foo().a                    #: 111

class B2[T]:
    a: T
    def foo[S](self: B2[S]):
        return B2[int](222)
b2 = B2[str]("x").foo()
print b2.foo().a                    #: 222

# explicit realization:
def m7[T,S]():
    print "works"
m7(str,float)                       #: works
m7(str,float)                       #: works
m7(float,str)                       #: works

#%% recursive,barebones
def foo(a):
    if not a:
        foo(True)
    print a
foo(0)
#: True
#: 0

def bar(a):
    def baz(x):
        if not x:
            bar(True)
        print (x)
    baz(a)
bar(0)
#: True
#: 0

def rec2(x, y):
    if x:
        return rec2(y, x)
    else:
        return 1.0
print rec2(1, False).__class__.__name__ #: float

def pq(x):
    return True
def rec3(x, y):
    if pq(x):
        return rec3(y, x)
    else:
        return y
print rec3('x', 's').__class__.__name__  #: str

# Nested mutually recursive function
def f[T](x: T) -> T:
    def g[T](z):
        return z(T())
    return g(f, T=T)
print f(1.2).__class__.__name__ #: float
print f('s').__class__.__name__ #: str

def f2[T](x: T):
    return f2(x - 1, T) if x else 1
print f2(1) #: 1
print f2(1.1).__class__.__name__ #: int


#%% recursive_error,barebones
def pq(x):
    return True
def rec3(x, y): #- ('a, 'b) -> 'b
    if pq(x):
        return rec3(y, x)
    else:
        return y
rec3(1, 's')
#! 'int' does not match expected type 'str'
#! during the realization of rec3(x: int, y: str)

#%% instantiate_function_2,barebones
def fx[T](x: T) -> T:
    def g[T](z):
        return z(T())
    return g(fx, T)
print fx(1.1).__class__.__name__, fx(1).__class__.__name__ #: float int

#%% optionals,barebones
y = None
print y  #: None
y = 5
print y  #: 5

def foo(x: optional[int], y: int):
    print 'foo', x, y
foo(y, 6)  #: foo 5 6
foo(5, 6)  #: foo 5 6
foo(5, y)  #: foo 5 5
y = None
try:
    foo(5, y)
except ValueError:
    print 'unwrap failed'  #: unwrap failed

class Cls:
    x: int
c = None
for i in range(2):
    if c: c.x += 1  # check for unwrap() dot access
    c = Cls(1)
print(c.x)  #: 1

#%% optional_methods,barebones
@extend
class int:
    def x(self):
        print 'x()!', self

y = None
z = 1 if y else None
print z  #: None

y = 6
z = 1 + y if y else None
print z  #: 7
z.x()  #: x()! 7
if 1: # otherwise compiler won't compile z.x() later
    z = None
try:
    z.x()
except ValueError:
    print 'unwrap failed'  #: unwrap failed

print Optional(1) + Optional(2)  #: 3
print Optional(1) + 3  #: 4
print 1 + Optional(1)  #: 2

#%% optional_tuple,barebones
a = None
if True:
    a = ('x', 's')
print(a)  #: ('x', 's')
print(*a, (1, *a))  #: x s (1, 'x', 's')
x,y=a
print(x,y,[*a]) #: x s ['x', 's']

#%% global_none,barebones
a, b = None, None
def foo():
    global a, b
    a = [1, 2]
    b = 3
print a, b,
foo()
print a, b #: None None [1, 2] 3

#%% default_type_none
class Test:
    value: int
    def __init__(self, value: int):
        self.value = value
    def __repr__(self):
        return str(self.value)
def key_func(k: Test):
    return k.value
print sorted([Test(1), Test(3), Test(2)], key=key_func)  #: [1, 2, 3]
print sorted([Test(1), Test(3), Test(2)], key=lambda x: x.value)  #: [1, 2, 3]
print sorted([1, 3, 2])  #: [1, 2, 3]

#%% nested_map
print list(map(lambda i: i-2, map(lambda i: i+1, range(5))))
#: [-1, 0, 1, 2, 3]

def h(x: list[int]):
    return x
print h(list(map(lambda i: i-1, map(lambda i: i+2, range(5)))))
#: [1, 2, 3, 4, 5]

#%% func_unify_error,barebones
def foo(x:int):
    print x
z = 1 & foo #! unsupported operand type(s) for &: 'int' and 'foo[int]'

#%% tuple_type_late,barebones
coords = []
for i in range(2):
    coords.append( ('c', i, []) )
coords[0][2].append((1, 's'))
print(coords)  #: [('c', 0, [(1, 's')]), ('c', 1, [])]

#%% void,barebones
def foo():
    print 'foo'
def bar(x):
    print 'bar', x.__class__.__name__
a = foo()  #: foo
bar(a)  #: bar NoneType

def x():
  pass
b = lambda: x()
b()
x() if True else x()

#%% void_2,barebones
def foo():
    i = 0
    while i < 10:
        print i  #: 0
        yield
        i += 10
a = list(foo())
print(a)  #: [None]

#%% instantiate_swap,barebones
class Foo[T, U]:
    t: T
    u: U
    def __init__(self):
        self.t = T()
        self.u = U()
    def __str__(self):
        return f'{self.t} {self.u}'
print Foo[int, bool](), Foo[bool, int]() #: 0 False False 0

#%% static,barebones
class Num[N_: Static[int]]:
    def __str__(self):
        return f'[{N_}]'
    def __init__(self):
        pass
def foo[N: Static[int]]():
    print Num[N*2]()
foo(3) #: [6]

class XX[N_: Static[int]]:
    a: Num[N_*2]
    def __init__(self):
        self.a = Num()
y = XX[5]()
print y.a, y.__class__.__name__, y.a.__class__.__name__ #: [10] XX[5] Num[10]

@tuple
class FooBar[N: Static[int]]:
    x: Int[N]
z = FooBar(i32(5))
print z, z.__class__.__name__, z.x.__class__.__name__ #: (x: Int[32](5)) FooBar[32] Int[32]

@tuple
class Foo[N: Static[int]]:
    x: Int[2*N]
    def __new__(x: Int[2*N]) -> Foo[N]:
        return (x,)
foo = Foo[10](Int[20](0))
print foo.__class__.__name__, foo.x.__class__.__name__ #: Foo[10] Int[20]

#%% static_2,barebones
class Num[N: Static[int]]:
    def __str__(self):
        return f'~{N}'
    def __init__(self):
        pass
class Foo[T, A: Static[int], B: Static[int]]:
    a: Num[A+B]
    b: Num[A-B]
    c: Num[A if A > 3 else B]
    t: T
    def __init__(self):
        self.a = Num()
        self.b = Num()
        self.c = Num()
        self.t = T()
    def __str__(self):
        return f'<{self.a} {self.b} {self.c} :: {self.t}>'
print Foo[int, 3, 4](), Foo[int, 5, 4]()
#: <~7 ~-1 ~4 :: 0> <~9 ~1 ~5 :: 0>

#%% static_int,barebones
def foo(n: Static[int]):
    print n

a: Static[int] = 5
foo(a < 1)   #: 0
foo(a <= 1)  #: 0
foo(a > 1)   #: 1
foo(a >= 1)  #: 1
foo(a == 1)  #: 0
foo(a != 1)  #: 1
foo(a and 1) #: 1
foo(a or 1)  #: 1
foo(a + 1)   #: 6
foo(a - 1)   #: 4
foo(a * 1)   #: 5
foo(a // 2)  #: 2
foo(a % 2)   #: 1
foo(a & 2)   #: 0
foo(a | 2)   #: 7
foo(a ^ 1)   #: 4

#%% static_str,barebones
class X:
    s: Static[str]
    i: Int[1 + (s == "abc")]
    def __init__(self: X[s], s: Static[str]):
        i = Int[1+(s=="abc")]()
        print s, self.s, self.i.__class__.__name__
def foo(x: Static[str], y: Static[str]):
    print x+y
z: Static[str] = "woo"
foo("he", z)  #: hewoo
X(s='lolo') #: lolo lolo Int[1]
X('abc') #: abc abc Int[2]


def foo2(x: Static[str]):
    print(x, x.__is_static__)
s: Static[str] = "abcdefghijkl"
foo2(s)  #: abcdefghijkl True
foo2(s[1])  #: b True
foo2(s[1:5])  #: bcde True
foo2(s[10:50])  #: kl True
foo2(s[1:30:3])  #: behk True
foo2(s[::-1])  #: lkjihgfedcba True


#%% static_getitem
print Int[staticlen("ee")].__class__.__name__ #: Int[2]

y = [1, 2]
print getattr(y, "len") #: 2
print y.len #: 2
getattr(y, 'append')(1)
print y #: [1, 2, 1]

@extend
class Dict:
    def __getitem2__(self, attr: Static[str]):
        if hasattr(self, attr):
            return getattr(self, attr)
        else:
            return self[attr]
    def __getitem1__(self, attr: Static[int]):
        return self[attr]

d = {'s': 3.19}
print d.__getitem2__('_upper_bound') #: 3
print d.__getitem2__('s') #: 3.19
e = {1: 3.33}
print e.__getitem1__(1) #: 3.33

#%% static_fail,barebones
def test(i: Int[32]):
    print int(i)
test(Int[5](1)) #! 'Int[5]' does not match expected type 'Int[32]'

#%% static_fail_2,barebones
zi = Int[32](6)
def test3[N](i: Int[N]):
    print int(i)
test3(zi) #! 'N' does not match expected type 'N'
# TODO: nicer error message!

#%% static_fail_3,barebones
zi = Int[32](6)
def test3[N: Static[int]](i: Int[N]):
    print int(i)
test3(1, int) #! expected static expression
# TODO: nicer error message!

#%% nested_fn_generic,barebones
def f(x):
    def g(y):
        return y
    return g(x)
print f(5), f('s') #: 5 s

def f2[U](x: U, y):
    def g[T, U](x: T, y: U):
        return (x, y)
    return g(y, x)
x, y = 1, 'haha'
print f2(x, y).__class__.__name__ #: Tuple[str,int]
print f2('aa', 1.1, U=str).__class__.__name__ #: Tuple[float,str]

#%% nested_fn_generic_error,barebones
def f[U](x: U, y): # ('u, 'a) -> tuple['a, 'u]
    def g[T, U](x: T, y: U): # ('t, 'u) -> tuple['t, 'u]
        return (x, y)
    return g(y, x)
print f(1.1, 1, int).__class__.__name__ #! 'float' does not match expected type 'int'

#%% fn_realization,barebones
def ff[T](x: T, y: tuple[T]):
      print ff(T=str,...).__class__.__name__ #: ff[str,Tuple[str],str]
      return x
x = ff(1, (1,))
print x, x.__class__.__name__ #: 1 int
# print f.__class__.__name__  # TODO ERRORS

def fg[T](x:T):
    def g[T](y):
        z = T()
        return z
    print fg(T=str,...).__class__.__name__  #: fg[str,str]
    print g(1, T).__class__.__name__ #: int
fg(1)
print fg(1).__class__.__name__ #: NoneType

def f[T](x: T):
    print f(x, T).__class__.__name__  #: int
    print f(x).__class__.__name__      #: int
    print f(x, int).__class__.__name__ #: int
    return x
print f(1), f(1).__class__.__name__ #: 1 int
print f(1, int).__class__.__name__ #: int

#%% fn_realization_error,barebones
def f[T](x: T):
    print f(x, int).__class__.__name__
    return x
f('s')
#! 'str' does not match expected type 'int'
#! during the realization of f(x: str, T: str)

#%% nested_class_error,barebones
class X:
    def foo(self, x):
        return x
    class Y:
        def bar(self, x):
            return x
y = X.Y()
y.foo(1) #! 'X.Y' object has no attribute 'foo'

#%% nested_deep_class,barebones
class A[T]:
    a: T
    class B[U]:
        b: U
        class C[V]:
            c: V
            def foo[W](t: V, u: V, v: V, w: W):
                return (t, u, v, w)

print A.B.C[bool].foo(W=str, ...).__class__.__name__ #: foo[bool,bool,bool,str,str]
print A.B.C.foo(1,1,1,True) #: (1, 1, 1, True)
print A.B.C.foo('x', 'x', 'x', 'x') #: ('x', 'x', 'x', 'x')
print A.B.C.foo('x', 'x', 'x', 'x') #: ('x', 'x', 'x', 'x')
print A.B.C.foo('x', 'x', 'x', 'x') #: ('x', 'x', 'x', 'x')

x = A.B.C[bool]()
print x.__class__.__name__ #: A.B.C[bool]

#%% nested_deep_class_error,barebones
class A[T]:
    a: T
    class B[U]:
        b: U
        class C[V]:
            c: V
            def foo[W](t: V, u: V, v: V, w: W):
                return (t, u, v, w)

print A.B.C[str].foo(1,1,1,True) #! 'A.B.C[str]' object has no method 'foo' with arguments (int, int, int, bool)

#%% nested_deep_class_error_2,barebones
class A[T]:
    a: T
    class B[U]:
        b: U
        class C[V]:
            c: V
            def foo[W](t: V, u: V, v: V, w: W):
                return (t, u, v, w)
print A.B[int].C[float].foo(1,1,1,True) #! 'A.B[int]' object has no attribute 'C'

#%% nested_class_function,barebones
def f(x):
    def g(y):
        return y
    a = g(1)
    b = g('s')
    c = g(x)
    return a, b, c
print f(1.1).__class__.__name__ #: Tuple[int,str,float]
print f(False).__class__.__name__ #: Tuple[int,str,bool]

class A[T]:
    a: T
    class B[U]:
        b: U
        class C[V]:
            c: V
            def f(x):
                def g(y):
                    return y
                a = g(1)
                b = g('s')
                c = g(x)
                return a, b, c
print A.B.C.f(1.1).__class__.__name__ #: Tuple[int,str,float]
print A.B.C[Optional[int]].f(False).__class__.__name__ #: Tuple[int,str,bool]

#%% rec_class_1,barebones
class A:
    y: A
    def __init__(self): pass  # necessary to prevent recursive instantiation!
x = A()
print x.__class__.__name__, x.y.__class__.__name__ #: A A

#%% rec_class_2,barebones
class A[T]:
    a: T
    b: A[T]
    c: A[str]
    def __init__(self): pass
a = A[int]()
print a.__class__.__name__, a.b.__class__.__name__, a.c.__class__.__name__, a.b.b.__class__.__name__, a.b.c.__class__.__name__
#: A[int] A[int] A[str] A[int] A[str]
print a.c.b.__class__.__name__, a.c.c.__class__.__name__, a.b.b.b.b.b.b.b.b.b.b.b.b.b.b.b.b.b.b.__class__.__name__
#: A[str] A[str] A[int]

#%% rec_class_3,barebones
class X:
    x: int
    rec: X
    def __init__(self): pass
    def foo(x: X, y: int):
        return y
    class Y:
        y: int
        def bar(self, y):
            print y
            return self.y
x, y = X(), X.Y()
print x.__class__.__name__, y.__class__.__name__
#: X X.Y
print X.foo(x, 4), x.foo(5)
#: 4 5
print y.bar(1), y.bar('s'), X.Y.bar(y, True)
#: 1
#: s
#: True
#: 0 0 0

#%% rec_class_4,barebones
class A[T]:
    a: T
    b: A[T]
    c: A[str]
    def __init__(self): pass
class B[T]:
    a: T
    b: A[T]
    c: B[T]
    def __init__(self): pass
    class Nest1[U]:
        n: U
    class Nest2[T, U]:
        m: T
        n: U
b = B[float]()
print b.__class__.__name__, b.a.__class__.__name__, b.b.__class__.__name__, b.c.__class__.__name__, b.c.b.c.a.__class__.__name__
#: B[float] float A[float] B[float] str

n1 = B.Nest1[int]()
print n1.n, n1.__class__.__name__, n1.n.__class__.__name__ #: 0 B.Nest1[int] int

n1: B.Nest2 = B.Nest2[float, int]()
print (n1.m, n1.n), n1.__class__.__name__, n1.m.__class__.__name__, n1.n.__class__.__name__ #: (0, 0) B.Nest2[float,int] float int

#%% func_arg_instantiate,barebones
class A[T]:
    y: T
    def foo(self, y: T):
        self.y = y
        return y
    def bar(self, y):
        return y
a = A()
print a.__class__.__name__ #: A[int]
a.y = 5
print a.__class__.__name__ #: A[int]

b = A()
print b.foo(5) #: 5
print b.__class__.__name__, b.y #: A[int] 5
print b.bar('s'), b.bar('s').__class__.__name__ #: s str
print b.bar(5), b.bar(5).__class__.__name__ #: 5 int

aa = A()
print aa.foo('s') #: s
print aa.__class__.__name__, aa.y, aa.bar(5.1).__class__.__name__ #: A[str] s float

#%% no_func_arg_instantiate_err,barebones
# TODO: allow unbound self?
class A[T]:
    y: T
    def foo(self, y): self.y = y
a = A()
a.foo(1) #! cannot typecheck the program

#%% return_deduction,barebones
def fun[T, R](x, y: T) -> R:
   	def ffi[T, R, Z](x: T, y: R, z: Z):
   		return (x, y, z)
   	yy = ffi(False, byte(2), 's', T=bool, Z=str, R=R)
   	yz = ffi(1, byte(2), 's', T=int, Z=str, R=R)
   	return byte(1)
print fun(2, 1.1, float, byte).__class__.__name__ #: byte

#%% return_auto_deduction_err,barebones
def fun[T, R](x, y: T) -> R:
   	return byte(1)
print fun(2, 1.1).__class__.__name__ #! cannot typecheck the program

#%% random
# shuffle used to fail before for some reason (sth about unbound variables)...
def foo():
    from random import shuffle
    v = list(range(10))
    shuffle(v)
    print sorted(v) #: [0, 1, 2, 3, 4, 5, 6, 7, 8, 9]
foo()

#%% function_type,barebones
class F:
    f: Function[[int], int]
    g: function[[int], None]
    x: int
def foo(x: int):
    return x+1
def goo(x: int):
    print x+2
f = F(foo, goo, 2)
print f.f(f.x) #: 3
f.g(f.x) #: 4

def hoo(z):
    print z+3
f.g = hoo
f.g(f.x)  #: 5

def hai(x, y, z):
    print f'hai({x},{y},{z})'
fn = Function[[int, int, int], None](hai)
fn(1, 2, 3) #: hai(1,2,3)
print str(fn)[:12] #: <function at
z = fn(2, 3, ...)
z(3) #: hai(2,3,3)

#%% int_float,barebones
l = [1., 2, 3, 4]
print l, l.__class__.__name__ #: [1, 2, 3, 4] List[float]

#%% forward_fn,barebones
def test(name, sort, key):
    v1 = [1, 2, 3, 4]
    sp = sort(v1, key)
    print name, sp
def foo(l, f):
    return [f(i) for i in l]
test('hi', foo, lambda x: x+1) #: hi [2, 3, 4, 5]

def foof(l: List[int], x, f: Callable[[int], int]):
    return [f(i)+x for i in l]
test('qsort', foof(x=3, ...), lambda x: x+1) #: qsort [5, 6, 7, 8]

#%% class_fn_access,barebones
class X[T]:
    def foo[U](self, x: T, y: U):
        return (x+x, y+y)
y = X[X[int]]()
print y.__class__.__name__ #: X[X[int]]
print X[float].foo(U=int, ...).__class__.__name__ #: foo[X[float],float,int,int]
print X[int]().foo(1, 's') #: (2, 'ss')

#%% class_partial_access,barebones
class X[T]:
    def foo[U](self, x, y: U):
        return (x+x, y+y)
y = X[X[int]]()
# TODO: should this even be the case?
# print y.foo(U=float,...).__class__.__name__  ->  X.foo[X[X[int]],...,...]
print y.foo(1, 2.2, float) #: (2, 4.4)

#%% forward,barebones
def foo(f, x):
    f(x, type(x))
    print f.__class__.__name__
def bar[T](x):
    print x, T.__class__.__name__
foo(bar, 1)
#: 1 int
#: bar[...]
foo(bar(...), 's')
#: s str
#: bar[...]
z = bar
z('s', int)
#: s int
z(1, T=str)
#: 1 str

zz = bar(T=int,...)
zz(1)
#: 1 int

#%% forward_error,barebones
def foo(f, x):
    f(x, type(x))
    print f.__class__.__name__
def bar[T](x):
    print x, T.__class__.__name__
foo(bar(T=int,...), 1)
#! bar() takes 2 arguments (2 given)
#! during the realization of foo(f: bar[...], x: int)
# TODO fix this error message

#%% sort_partial
def foo(x, y):
    return y**x
print sorted([1,2,3,4,5], key=foo(y=2, ...))
print sorted([1,2,3,4,5], key=foo(y=-2, ...))
#: [1, 2, 3, 4, 5]
#: [5, 3, 1, 2, 4]

#%% mutually_recursive_error,barebones
def bl(x):
    return True
def frec(x, y):
    def grec(x, y):
        return frec(y, x)
    return grec(x, y) if bl(y) else 2
print frec(1, 2).__class__.__name__, frec('s', 1).__class__.__name__
#! 'NoneType' does not match expected type 'int'
#! during the realization of frec(x: int, y: int)

#%% return_fn,barebones
def retfn(a):
    def inner(b, *args, **kwargs):
        print a, b, args, kwargs
    print inner.__class__.__name__ #: inner[...,...,int,...]
    return inner(15, ...)
f = retfn(1)
print f.__class__.__name__ #: inner[int,...,int,...]
f(2,3,foo='bar') #: 1 15 (2, 3) (foo: 'bar')

#%% decorator_manual,barebones
def foo(x, *args, **kwargs):
    print x, args, kwargs
    return 1
def dec(fn, a):
    print 'decorating', fn.__class__.__name__ #: decorating foo[...,...,...]
    def inner(*args, **kwargs):
        print 'decorator', args, kwargs #: decorator (5.5, 's') (z: True)
        return fn(a, *args, **kwargs)
    return inner(...)
ff = dec(foo(...), 10)
print ff(5.5, 's', z=True)
#: 10 (5.5, 's') (z: True)
#: 1


#%% decorator,barebones
def foo(x, *args, **kwargs):
    print x, args, kwargs
    return 1
def dec(a):
    def f(fn):
        print 'decorating', fn.__class__.__name__
        def inner(*args, **kwargs):
            print 'decorator', args, kwargs
            return fn(a, *args, **kwargs)
        return inner
    return f
ff = dec(10)(foo)
print ff(5.5, 's', z=True)
#: decorating foo[...,...,...]
#: decorator (5.5, 's') (z: True)
#: 10 (5.5, 's') (z: True)
#: 1

@dec(a=5)
def zoo(e, b, *args):
    return f'zoo: {e}, {b}, {args}'
print zoo(2, 3)
print zoo('s', 3)
#: decorating zoo[...,...,...]
#: decorator (2, 3) ()
#: zoo: 5, 2, (3,)
#: decorator ('s', 3) ()
#: zoo: 5, s, (3,)

def mydecorator(func):
    def inner():
        print("before")
        func()
        print("after")
    return inner
@mydecorator
def foo2():
    print("foo")
foo2()
#: before
#: foo
#: after

def timeme(func):
    def inner(*args, **kwargs):
        begin = 1
        end = func(*args, **kwargs) - begin
        print('time needed for', func.__class__.__name__, 'is', end)
    return inner
@timeme
def factorial(num):
    n = 1
    for i in range(1,num + 1):
        n *= i
    print(n)
    return n
factorial(10)
#: 3628800
#: time needed for factorial[...] is 3628799

def dx1(func):
    def inner():
        x = func()
        return x * x
    return inner
def dx2(func):
    def inner():
        x = func()
        return 2 * x
    return inner
@dx1
@dx2
def num():
    return 10
print(num()) #: 400

def dy1(func):
    def inner(*a, **kw):
        x = func(*a, **kw)
        return x * x
    return inner
def dy2(func):
    def inner(*a, **kw):
        x = func(*a, **kw)
        return 2 * x
    return inner
@dy1
@dy2
def num2(a, b):
    return a+b
print(num2(10, 20)) #: 3600

#%% hetero_iter,barebones
e = (1, 2, 3, 'foo', 5, 'bar', 6)
for i in e:
    if isinstance(i, int):
        if i == 1: continue
    if isinstance(i, str):
        if i == 'bar': break
    print i

#%% type_loc,barebones
a = 1
T = type(a)
print T.__class__.__name__  #: int

#%% empty_tuple,barebones
T = type(())  # only errors with empty tuple type
p = Ptr[T](cobj())
print p.__class__.__name__  #: Ptr[Tuple]

print [a for a in ()]  #: []

def foo(*args):
    return [a for a in args]
args, result = ((), [()])
print list(foo(*args))  #: []
print result  #: [()]


#%% type_error_reporting
# TODO: improve this certainly
def tee(iterable, n=2):
    from collections import deque
    it = iter(iterable)
    deques = [deque() for i in range(n)]
    def gen(mydeque):
        while True:
            if not mydeque:             # when the local deque is empty
                if it.done():
                    return
                newval = it.next()
                for d in deques:        # load it to all the deques
                    d.append(newval)
            yield mydeque.popleft()
    return list(gen(d) for d in deques)
it = [1,2,3,4]
a, b = tee(it)
#! cannot typecheck the program
#! during the realization of tee(iterable: List[int], n: int)

#%% new_syntax,barebones
def foo[T,U](x: type, y, z: Static[int] = 10):
    print T.__class__.__name__, U.__class__.__name__, x.__class__.__name__, y.__class__.__name__, Int[z+1].__class__.__name__
    return List[x]()
print foo(T=int,U=str,...).__class__.__name__ #: foo[T1,x,z,int,str]
print foo(T=int,U=str,z=5,x=bool,...).__class__.__name__ #: foo[T1,bool,5,int,str]
print foo(float,3,T=int,U=str,z=5).__class__.__name__ #: List[float]
foo(float,1,10,str,int) #: str int float int Int[11]


class Foo[T,U: Static[int]]:
    a: T
    b: Static[int]
    c: Int[U]
    d: type
    e: List[d]
    f: UInt[b]
print Foo[5,int,float,6].__class__.__name__ #: Foo[5,int,float,6]
print Foo(1.1, 10i32, [False], 10u66).__class__.__name__ #: Foo[66,bool,float,32]


def foo2[N: Static[int]]():
    print Int[N].__class__.__name__, N
x: Static[int] = 5
y: Static[int] = 105 - x * 2
foo2(y-x) #: Int[90] 90

if 1.1+2.2 > 0:
    z: Static[int] = 88
    print z #: 88
print x #: 5
x : Static[int] = 3
print x #: 3

def fox(N: Static[int] = 4):
    print Int[N].__class__.__name__, N
fox(5) #: Int[5] 5
fox() #: Int[4] 4

#%% new_syntax_err,barebones
class Foo[T,U: Static[int]]:
    a: T
    b: Static[int]
    c: Int[U]
    d: type
    e: List[d]
    f: UInt[b]
print Foo[float,6].__class__.__name__ #! Foo takes 4 generics (2 given)

#%% partial_star_pipe_args,barebones
iter(['A', 'C']) |> print
#: A
#: C
iter(range(4)) |> print('x', ..., 1)
#: x 0 1
#: x 1 1
#: x 2 1
#: x 3 1

#%% type_arg_transform,barebones
print list(map(str, range(5)))
#: ['0', '1', '2', '3', '4']


#%% traits,barebones
def t[T](x: T, key: Optional[Callable[[T], S]] = None, S: type = NoneType):
    if isinstance(S, NoneType):
        return x
    else:
        return (key.__val__())(x)
print t(5) #: 5
print t(6, lambda x: f'#{x}') #: #6

z: Callable[[int],int] = lambda x: x+1
print z(5) #: 6

def foo[T](x: T, func: Optional[Callable[[], T]] = None) -> T:
    return x
print foo(1) #: 1

#%% trait_callable
foo = [1,2,11]
print(sorted(foo, key=str))
#: [1, 11, 2]

foo = {1: "a", 2: "a", 11: "c"}
print(sorted(foo.items(), key=str))
#: [(1, 'a'), (11, 'c'), (2, 'a')]

def call(f: Callable[[int,int], Tuple[str,int]]):
    print(f(1, 2))

def foo(*x): return f"{x}_{x.__class__.__name__}",1
call(foo)
#: ('(1, 2)_Tuple[int,int]', 1)

def foo(a:int, *b: float): return f"{a}_{b}", a+b[0]
call(foo)
#: ('1_(2,)', 3)

def call(f: Callable[[int,int],str]):
    print(f(1, 2))
def foo(a: int, *b: int, **kw): return f"{a}_{b}_{kw}"
call(foo(zzz=1.1, ...))
#: 1_(2,)_(zzz: 1.1)

#%% traits_error,barebones
def t[T](x: T, key: Optional[Callable[[T], S]] = None, S: type = NoneType):
    if isinstance(S, NoneType):
        return x
    else:
        return (key.__val__())(x)
print t(6, Optional(1)) #! 'Optional[int]' does not match expected type 'Optional[Callable[[int],S]]'

#%% traits_error_2,barebones
z: Callable[[int],int] = 4 #! 'Callable[[int],int]' does not match expected type 'int'

#%% assign_wrappers,barebones
a = 1.5
print a #: 1.5
if 1:
    a = 1
print a, a.__class__.__name__ #: 1 float

a: Optional[int] = None
if 1:
    a = 5
print a.__class__.__name__, a #: Optional[int] 5

b = 5
c = Optional(6)
if 1:
    b = c
print b.__class__.__name__, c.__class__.__name__, b, c #: int Optional[int] 6 6

z: Generator[int] = [1, 2]
print z.__class__.__name__ #: Generator[int]

zx: float = 1
print zx.__class__.__name__, zx #: float 1

def test(v: Optional[int]):
    v: int = v if v is not None else 3
    print v.__class__.__name__
test(5) #: int
test(None) #: int

#%% methodcaller,barebones
def foo():
    def bar(a, b):
        print 'bar', a, b
    return bar
foo()(1, 2) #: bar 1 2

def methodcaller(foo: Static[str]):
    def caller(foo: Static[str], obj, *args, **kwargs):
        if isinstance(getattr(obj, foo)(*args, **kwargs), None):
            getattr(obj, foo)(*args, **kwargs)
        else:
            return getattr(obj, foo)(*args, **kwargs)
    return caller(foo=foo, ...)
v = [1]
methodcaller('append')(v, 42)
print v #: [1, 42]
print methodcaller('index')(v, 42) #: 1

#%% fn_overloads,barebones
def foo(x):
    return 1, x

print(foo(''))  #: (1, '')

@overload
def foo(x, y):
    def foo(x, y):
        return f'{x}_{y}'
    return 2, foo(x, y)

@overload
def foo(x):
    if x == '':
        return 3, 0
    return 3, 1 + foo(x[1:])[1]

print foo('hi') #: (3, 2)
print foo('hi', 1) #: (2, 'hi_1')


def fox(a: int, b: int, c: int, dtype: type = int):
    print('fox 1:', a, b, c)

@overload
def fox(a: int, b: int, dtype: type = int):
    print('fox 2:', a, b, dtype.__class__.__name__)

fox(1, 2, float)
#: fox 2: 1 2 float
fox(1, 2)
#: fox 2: 1 2 int
fox(1, 2, 3)
#: fox 1: 1 2 3

#%% fn_shadow,barebones
def foo(x):
    return 1, x
print foo('hi') #: (1, 'hi')

def foo(x):
    return 2, x
print foo('hi') #: (2, 'hi')

#%% fn_overloads_error,barebones
def foo(x):
    return 1, x
@overload
def foo(x, y):
    return 2, x, y
foo('hooooooooy!', 1, 2) #! no function 'foo' with arguments (str, int, int)

#%% c_void_return,barebones
from C import seq_print(str)
x = seq_print("not ")
print x  #: not None


#%% static_for,barebones
def foo(i: Static[int]):
    print('static', i, Int[i].__class__.__name__)

for i in statictuple(1, 2, 3, 4, 5):
    foo(i)
    if i == 3: break
#: static 1 Int[1]
#: static 2 Int[2]
#: static 3 Int[3]
for i in staticrange(9, 4, -2):
    foo(i)
    if i == 3:
        break
#: static 9 Int[9]
#: static 7 Int[7]
#: static 5 Int[5]
for i in statictuple("x", 1, 3.3, 2):
    print(i)
#: x
#: 1
#: 3.3
#: 2

print tuple(Int[i+10](i) for i in statictuple(1, 2, 3)).__class__.__name__
#: Tuple[Int[11],Int[12],Int[13]]

for i in staticrange(0, 10):
    if i % 2 == 0: continue
    if i > 8: break
    print('xyz', Int[i].__class__.__name__)
print('whoa')
#: xyz Int[1]
#: xyz Int[3]
#: xyz Int[5]
#: xyz Int[7]
#: whoa

for i in staticrange(15):
    if i % 2 == 0: continue
    if i > 8: break
    print('xyz', Int[i].__class__.__name__)
print('whoa')
#: xyz Int[1]
#: xyz Int[3]
#: xyz Int[5]
#: xyz Int[7]
#: whoa

print tuple(Int[i-10](i) for i in staticrange(30,33)).__class__.__name__
#: Tuple[Int[20],Int[21],Int[22]]

for i in statictuple(0, 2, 4, 7, 11, 12, 13):
    if i % 2 == 0: continue
    if i > 8: break
    print('xyz', Int[i].__class__.__name__)
print('whoa')
#: xyz Int[7]
#: whoa

for i in staticrange(10):  # TODO: large values are too slow!
    pass
print('done')
#: done

tt = (5, 'x', 3.14, False, [1, 2])
for i, j in staticenumerate(tt):
    print(foo(i * 2 + 1), j)
#: static 1 Int[1]
#: None 5
#: static 3 Int[3]
#: None x
#: static 5 Int[5]
#: None 3.14
#: static 7 Int[7]
#: None False
#: static 9 Int[9]
#: None [1, 2]

print tuple((Int[i+1](i), j) for i, j in staticenumerate(tt)).__class__.__name__
#: Tuple[Tuple[Int[1],int],Tuple[Int[2],str],Tuple[Int[3],float],Tuple[Int[4],bool],Tuple[Int[5],List[int]]]

#%% static_range_error,barebones
for i in staticrange(1000, -2000, -2):
    pass
#! staticrange too large (expected 0..1024, got instead 1500)

#%% trait_defdict
class dd(Static[Dict[K,V]]):
    fn: S
    K: type
    V: type
    S: TypeVar[Callable[[], V]]

    def __init__(self: dd[K, VV, Function[[], V]], VV: TypeVar[V]):
        self.fn = lambda: VV()

    def __init__(self, f: S):
        self.fn = f

    def __getitem__(self, key: K) -> V:
        if key not in self:
            self.__setitem__(key, self.fn())
        return super().__getitem__(key)


x = dd(list)
x[1] = [1, 2]
print(x[2])
#: []
print(x)
#: {1: [1, 2], 2: []}

z = 5
y = dd(lambda: z+1)
y.update({'a': 5})
print(y['b'])
#: 6
z = 6
print(y['c'])
#: 7
print(y)
#: {'a': 5, 'b': 6, 'c': 7}

xx = dd(lambda: 'empty')
xx.update({1: 's', 2: 'b'})
print(xx[1], xx[44])
#: s empty
print(xx)
#: {44: 'empty', 1: 's', 2: 'b'}

s = 'mississippi'
d = dd(int)
for k in s:
    d[k] = d["x" + k]
print(sorted(d.items()))
#: [('i', 0), ('m', 0), ('p', 0), ('s', 0), ('xi', 0), ('xm', 0), ('xp', 0), ('xs', 0)]


#%% kwargs_getattr,barebones
def foo(**kwargs):
    print kwargs['foo'], kwargs['bar']

foo(foo=1, bar='s')
#: 1 s



#%% union_types,barebones
def foo_int(x: int):
    print(f'{x} {x.__class__.__name__}')
def foo_str(x: str):
    print(f'{x} {x.__class__.__name__}')
def foo(x):
    print(f'{x} {int(__internal__.union_get_tag(x))} {x.__class__.__name__}')

a: Union[int, str] = 5
foo_int(a)  #: 5 int
foo(a)  #: 5 0 Union[int,str]
print(staticlen(a))  #: 2
print(staticlen(Union[int, int]), staticlen(Tuple[int, float, int]))  #: 1 3

@extend
class str:
    def __add__(self, i: int):
        return int(self) + i

a += 6  ## this is U.__new__(a.__getter__(__add__)(59))
b = a + 59
print(a, b, a.__class__.__name__, b.__class__.__name__)  #: 11 70 Union[int,str] int

if True:
    a = 'hello'
    foo_str(a)  #: hello str
    foo(a)  #: hello 1 Union[int,str]
    b = a[1:3]
    print(b)  #: el
print(a)  #: hello

a: Union[Union[Union[str], int], Union[int, int, str]] = 9
foo(a)  #: 9 0 Union[int,str]

def ret(x):
    z : Union = x
    if x < 1: z = 1
    elif x < 10: z = False
    else: z = 'oops'
    return z
r = ret(2)
print(r, r.__class__.__name__)  #: False Union[bool,int,str]
r = ret(33.3)
print(r, r.__class__.__name__)  #: oops Union[bool,float,int,str]

def ret2(x) -> Union:
    if x < 1: return 1
    elif x < 10: return 2.2
    else: return ['oops']
r = ret2(20)
print(r, r.__class__.__name__)  #: ['oops'] Union[List[str],float,int]

class A:
    x: int
    def foo(self):
        return f"A: {self.x}"
class B:
    y: str
    def foo(self):
        return f"B: {self.y}"
x : Union[A,B] = A(5)  # TODO: just Union does not work in test mode :/
print(x.foo())  #: A: 5
print(x.x)  #: 5
if True:
    x = B("bee")
print(x.foo())  #: B: bee
print(x.y)  #: bee
try:
    print(x.x)
except TypeError as e:
    print(e.message)  #: invalid union call 'x'

def do(x: A):
    print('do', x.x)
try:
    do(x)
except TypeError:
    print('error') #: error

def do2(x: B):
    print('do2', x.y)
do2(x)  #: do2 bee

z: Union[int, str] = 1
print isinstance(z, int), isinstance(z, str), isinstance(z, float), isinstance(z, Union[int, float]), isinstance(z, Union[int, str])
#: True False False False True

print isinstance(z, Union[int]), isinstance(z, Union[int, float, str])
#: False False

if True:
    z = 's'
print isinstance(z, int), isinstance(z, str), isinstance(z, float), isinstance(z, Union[int, float]), isinstance(z, Union[int, str])
#: False True False False True

class A:
    def foo(self): return 1
class B:
    def foo(self): return 's'
class C:
    def foo(self): return [True, False]
x : Union[A,B,C] = A()
print x.foo(), x.foo().__class__.__name__
#: 1 Union[List[bool],int,str]

xx = Union[int, str](0)
print(xx)  #: 0

#%% union_error,barebones
a: Union[int, str] = 123
print(123 == a)  #: True
print(a == 123)  #: True
try:
    a = "foo"
    print(a == 123)
except TypeError:
    print("oops", a)  #: oops foo


#%% generator_capture_nonglobal,barebones
# Issue #49
def foo(iter):
    print(iter.__class__.__name__, list(iter))

for x in range(2):
    foo(1 for _ in range(x))
#: Generator[int] []
#: Generator[int] [1]
for x in range(2):
    for y in range(x):
        foo('z' for _ in range(y))
#: Generator[str] []

#%% nonlocal_capture_loop,barebones
# Issue #51
def kernel(fn):
    def wrapper(*args, grid, block):
        print(grid, block, fn(*args))
    return wrapper
def test_mandelbrot():
    MAX    = 10  # maximum Mandelbrot iterations
    N      = 2   # width and height of image
    pixels = [0 for _ in range(N)]
    def scale(x, a, b):
        return a + (x/N)*(b - a)
    @kernel
    def k(pixels):
        i = 0
        while i < MAX: i += 1  # this is needed for test to make sense
        return (MAX, N, pixels, scale(N, -2, 0.4))
    k(pixels, grid=(N*N)//1024, block=1024)
test_mandelbrot()  #: 0 1024 (10, 2, [0, 0], 0.4)

#%% delayed_lambda_realization,barebones
x = []
for i in range(2):
    print(all(x[j] < 0 for j in range(i)))
    x.append(i)
#: True
#: False

#%% constructor_passing
class A:
    s: str
    def __init__(self, x):
        self.s = str(x)[::-1]
    def __lt__(self, o): return self.s < o.s
    def __eq__(self, o): return self.s == o.s
    def __ge__(self, o): return self.s >= o.s
foo = [1,2,11,30]
print(sorted(foo, key=str))
#: [1, 11, 2, 30]
print(sorted(foo, key=A))
#: [30, 1, 11, 2]


#%% polymorphism,barebones
class A:
    a: int
    def foo(self, a: int): return (f'A({self.a})', a)
    def bar(self): return 'A.bar'
    def aaz(self): return 'A.aaz'
class B(A):
    b: int
    def foo(self, a): return (f'B({self.a},{self.b})', a + self.b)
    def bar(self): return 'B.bar'
    def baz(self): return 'B.baz'
class M[T]:
    m: T
    def moo(self): return (f'M_{T.__class__.__name__}', self.m)
class X(B,M[int]):
    def foo(self, a): return (f'X({self.a},{self.b},{self.m})', a + self.b + self.m)
    def bar(self): return 'X.bar'

def foo(i):
    x = i.foo(1)
    y = i.bar()
    z = i.aaz()
    print(*x, y, z)
a = A(1)
l = [a, B(2,3), X(2,3,-1)]
for i in l: foo(i)
#: A(1) 1 A.bar A.aaz
#: B(2,3) 4 B.bar A.aaz
#: X(2,3,-1) 3 X.bar A.aaz

def moo(m: M):
    print(m.moo())
moo(M[float](5.5))
moo(X(1,2,3))
#: ('M_float', 5.5)
#: ('M_int', 3)


class A[T]:
    def __init__(self):
        print("init A", T.__class__.__name__)
class Ho:
    def __init__(self):
        print("init Ho")
# TODO: this throws and error: B[U](U)
class B[U](A[U], Ho):
    def __init__(self):
        super().__init__()
        print("init B", U.__class__.__name__)
B[Ho]()
#: init A Ho
#: init B Ho


class Vehicle:
    def drive(self):
        return "I'm driving a vehicle"

class Car(Vehicle):
    def drive(self):
        return "I'm driving a car"

class Truck(Vehicle):
    def drive(self):
        return "I'm driving a truck"

class SUV(Car, Truck):
    def drive(self):
        return "I'm driving an SUV"

suv = SUV()
def moo(s):
    print(s.drive())
moo(suv)
moo(Truck())
moo(Car())
moo(Vehicle())
#: I'm driving an SUV
#: I'm driving a truck
#: I'm driving a car
#: I'm driving a vehicle


#%% polymorphism_error_1,barebones
class M[T]:
    m: T
class X(M[int]):
    pass
l = [M[float](1.1), X(2)]
#! 'List[M[float]]' object has no method 'append' with arguments (List[M[float]], X)

#%% polymorphism_2
class Expr:
    def __init__(self):
        pass
    def eval(self):
        raise ValueError('invalid expr')
        return 0.0
    def __str__(self):
        return "Expr"
class Const(Expr):
    x: float
    def __init__(self, x):
        self.x=x
    def __str__(self):
        return f"{self.x}"
    def eval(self):
        return self.x
class Add(Expr):
    lhs: Expr
    rhs: Expr
    def __init__(self, lhs, rhs):
        self.lhs=lhs
        self.rhs=rhs
        # print(f'ctr: {self}')
    def eval(self):
        return self.lhs.eval()+self.rhs.eval()
    def __str__(self):
        return f"({self.lhs}) + ({self.rhs})"
class Mul(Expr):
    lhs: Expr
    rhs: Expr
    def __init__(self, lhs, rhs):
        self.lhs=lhs
        self.rhs=rhs
    def eval(self):
        return self.lhs.eval()*self.rhs.eval()
    def __str__(self):
        return f"({self.lhs}) * ({self.rhs})"

c1 = Const(5)
c2 = Const(4)
m = Add(c1, c2)
c3 = Const(2)
a : Expr = Mul(m, c3)
print(f'{a} = {a.eval()}')
#: ((5) + (4)) * (2) = 18

from random import random, seed
seed(137)
def random_expr(depth) -> Expr:
    if depth<=0:
        return Const(int(random()*42.0))
    else:
        lhs=random_expr(depth-1)
        rhs=random_expr(depth-1)
        ctorid = int(random()*3)
        if ctorid==0:
            return Mul(lhs,rhs)
        else:
            return Add(lhs,rhs)
for i in range(11):
    print(random_expr(i).eval())
#: 17
#: 71
#: 1760
#: 118440
#: 94442
#: 8.02435e+15
#: 1.07463e+13
#: 1.43017e+19
#: 2.40292e+34
#: 6.1307e+28
#: 5.16611e+49


#%% collection_common_type,barebones
l = [1, 2, 3]
print(l, l.__class__.__name__)
#: [1, 2, 3] List[int]

l = [1.1, 2, 3]
print(l, l.__class__.__name__)
#: [1.1, 2, 3] List[float]

l = [1, 2, 3.3]
print(l, l.__class__.__name__)
#: [1, 2, 3.3] List[float]

l = [1, None]
print(l, l.__class__.__name__)
#: [1, None] List[Optional[int]]

l = [None, 2.2]
print(l, l.__class__.__name__)
#: [None, 2.2] List[Optional[float]]

class A:
    def __repr__(self): return 'A'
class B(A):
    def __repr__(self): return 'B'
class C(B):
    def __repr__(self): return 'C'
class D(A):
    def __repr__(self): return 'D'

l = [A(), B(), C(), D()]
print(l, l.__class__.__name__)
#: [A, B, C, D] List[A]

l = [D(), C(), B(), A()]
print(l, l.__class__.__name__)
#: [D, C, B, A] List[A]

l = [C(), B()]
print(l, l.__class__.__name__)
#: [C, B] List[B]

l = [C(), A(), B()]
print(l, l.__class__.__name__)
#: [C, A, B] List[A]

l = [None, *[1, 2], None]
print(l, l.__class__.__name__)
#: [None, 1, 2, None] List[Optional[int]]

# l = [C(), D(), B()] # does not work (correct behaviour)
# print(l, l.__class__.__name__)

d = {1: None, 2.2: 's'}
print(d, d.__class__.__name__)
#: {1: None, 2.2: 's'} Dict[float,Optional[str]]

#%% polymorphism_3
import operator

class Expr:
    def eval(self):
        return 0

class Const(Expr):
    value: int

    def __init__(self, value):
        self.value = value

    def eval(self):
        return self.value

class BinOp(Expr):
    lhs: Expr
    rhs: Expr

    def __init__(self, lhs, rhs):
        self.lhs = lhs
        self.rhs = rhs

    def eval_from_fn(self, fn):
        return fn(self.lhs.eval(), self.rhs.eval())

class Add(BinOp):
    def eval(self):
        return self.eval_from_fn(operator.add)

class Sub(BinOp):
    def eval(self):
        return self.eval_from_fn(operator.sub)

class Mul(BinOp):
    def eval(self):
        return self.eval_from_fn(operator.mul)

class Div(BinOp):
    def eval(self):
        return self.eval_from_fn(operator.floordiv)

# TODO: remove Expr requirement
expr : Expr = Mul(Const(3), Add(Const(10), Const(5)))
print(expr.eval()) #: 45


#%% polymorphism_4
class A(object):
    a: int
    def __init__(self, a: int):
        self.a = a

    def test_a(self, n: int):
        print("test_a:A", n)

    def test(self, n: int):
        print("test:A", n)

    def test2(self, n: int):
        print("test2:A", n)

class B(A):
    b: int
    def __init__(self, a: int, b: int):
        super().__init__(a)
        self.b = b

    def test(self, n: int):
        print("test:B", n)

    def test2(self, n: int):
        print("test2:B", n)

class C(B):
    pass

b = B(1, 2)
b.test_a(1)
b.test(1)
#: test_a:A 1
#: test:B 1

a: A = b
a.test(1)
a.test2(2)
#: test:B 1
#: test2:B 2



class AX(object):
    value: u64

    def __init__(self):
        print('init/AX')
        self.value = 15u64

    def get_value(self) -> u64:
        return self.value

class BX(object):
    a: AX
    def __init__(self):
        print('init/BX')
        self.a = AX()
    def hai(self):
        return f"hai/BX: {self.a.value}"

class CX(BX):
    def __init__(self):
        print('init/CX')
        super().__init__()

    def getsuper(self):
        return super()

    def test(self):
        print('test/CX:', self.a.value)
        return self.a.get_value()

    def hai(self):
        return f"hai/CX: {self.a.value}"

table = CX()
#: init/CX
#: init/BX
#: init/AX
print table.test()
#: test/CX: 15
#: 15

s = table.getsuper()
print(s.hai())
#: hai/BX: 15
s.a.value += 1u64
print(s.hai())
#: hai/BX: 16
table.a.value += 1u64
print(s.hai())
#: hai/BX: 17
table.test()
#: test/CX: 17

c: List[BX] = [s, table]
print(c[0].hai())  #: hai/BX: 17
print(c[1].hai())  #: hai/CX: 17



#%% no_generic,barebones
def foo(a, b: Static[int]):
    pass
foo(5)  #! generic 'b' not provided


#%% no_generic_2,barebones
def f(a, b, T: type):
    print(a, b)
f(1, 2)  #! generic 'T' not provided

#%% variardic_tuples,barebones

class Foo[N: Static[int]]:
    x: Tuple[N, str]

    def __init__(self):
        self.x = ("hi", ) * N

f = Foo[5]()
print(f.__class__.__name__)
#: Foo[5]
print(f.x.__class__.__name__)
#: Tuple[str,str,str,str,str]
print(f.x)
#: ('hi', 'hi', 'hi', 'hi', 'hi')

print(Tuple[int, int].__class__.__name__)
#: Tuple[int,int]
print(Tuple[3, int].__class__.__name__)
#: Tuple[int,int,int]
print(Tuple[0].__class__.__name__)
#: Tuple
print(Tuple[-5, int].__class__.__name__)
#: Tuple
print(Tuple[5, int, str].__class__.__name__)
#: Tuple[int,str,int,str,int,str,int,str,int,str]


#%% domination_nested,barebones
def correlate(a, b, mode = 'valid'):
    if mode == 'valid':
        if isinstance(a, List):
            xret = '1'
        else:
            xret = '2'
        for i in a:
            for j in b:
                xret += 'z'
    elif mode == 'same':
        if isinstance(a, List):
            xret = '3'
        else:
            xret = '4'
        for i in a:
            for j in b:
                xret += 'z'
    elif mode == 'full':
        if isinstance(a, List):
            xret = '5'
        else:
            xret = '6'
        for i in a:
            for j in b:
                xret += 'z'
    else:
        raise ValueError(f"mode must be one of 'valid', 'same', or 'full' (got {repr(mode)})")
    return xret
print(correlate([1], [2], 'full'))  #: 5z

def foo(x, y):
    a = 5
    if isinstance(a, int):
        if staticlen(y) == 0:
            a = 0
        elif staticlen(y) == 1:
            a = 1
        else:
            for i in range(10):
                a = 40
            return a
    return a
print foo(5, (1, 2, 3))  #: 40

#%% union_hasattr,barebones
class A:
    def foo(self):
        print('foo')
    def bar(self):
        print('bar')
class B:
    def foo(self):
        print('foo')
    def baz(self):
        print('baz')

a = A()
print(hasattr(a, 'foo'), hasattr(a, 'bar'), hasattr(a, 'baz'))
#: True True False
b = B()
print(hasattr(b, 'foo'), hasattr(b, 'bar'), hasattr(b, 'baz'))
#: True False True

c: Union[A, B] = A()
print(hasattr(c, 'foo'), hasattr(c, 'bar'), hasattr(c, 'baz'))
#: True True False

c = B()
print(hasattr(c, 'foo'), hasattr(c, 'bar'), hasattr(c, 'baz'))
#: True False True


#%% delayed_dispatch
import math
def fox(a, b, key=None): # key=None delays it!
    return a if a <= b else b

a = 1.0
b = 2.0
c = fox(a, b)
print(math.log(c) / 2) #: 0
