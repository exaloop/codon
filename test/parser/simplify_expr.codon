#%% none,barebones
@extend
class Optional:
    def __repr__(self):
        return 'OPTIONAL: ' + ('-' if self is None else self.__val__().__repr__())
    def __str__(self):
        return 'OPTIONAL: ' + ('-' if self is None else self.__val__().__repr__())

a = None
print a #: OPTIONAL: -
if True:
    a = 5
print a #: OPTIONAL: 5

#%% bool,barebones
print True, False #: True False

#%% int,barebones
print 0b0000_1111 #: 15
print 0B101 #: 5
print 3 #: 3
print 18_446_744_073_709_551_000 #: -616
print 0b11111111_11111111_11111111_11111111_11111111_11111111_11111111_11111111 #: -1
print 0b11111111_11111111_11111111_11111111_11111111_11111111_11111111_11111111u #: 18446744073709551615
print 18_446_744_073_709_551_000u #: 18446744073709551000
print 65i7 #: -63
print -1u7 #: 127

@extend
class int:
    def __suffix_test__(s):
        return 'TEST: ' + str(s)
print 123_456test #: TEST: 123456

#%% int_error,barebones
print 1844674407_3709551999 #! integer '18446744073709551999' cannot fit into 64-bit integer

#%% float,barebones
print 5.15 #: 5.15
print 2e2 #: 200
print 2.e-2 #: 0.02

#%% float_suffix,barebones
@extend
class float:
    def __suffix_zoo__(x):
        return str(x) + '_zoo'

print 1.2e-1zoo #: 0.12_zoo

#%% string,barebones
print 'kthxbai', "kthxbai" #: kthxbai kthxbai
print """hi
hello""", '''hai
hallo'''
#: hi
#: hello hai
#: hallo

#%% fstring,barebones
a, b = 1, 2
print f"string {a}" #: string 1
print F"{b} string" #: 2 string
print f"str {a+b} end" #: str 3 end
print f"str {a+b=}" #: str a+b=3
c = f'and this is {a} followed by {b}'
print c, f'{b}{a}', f'. {1+a=} .. {b} ...' #: and this is 1 followed by 2 21 . 1+a=2 .. 2 ...

#%% fstring_error,barebones
f"a{b + 3}}" #! single '}' is not allowed in f-string

#%% fstring_error_2,barebones
f"a{{b + 3}" #! expecting '}' in f-string

#%% prefix_str,barebones
@extend
class str:
    def __prefix_pfx__[N: Static[int]](s: str):
        return 'PFX ' + s
print pfx'HELLO' #: PFX HELLO

@extend
class str:
    def __prefix_pxf__(s: str, N: Static[int]):
        return 'PXF ' + s + " " + str(N)
print pxf'HELLO' #: PXF HELLO 5

#%% raw_str,barebones
print 'a\\b' #: a\b
print r'a\tb' #: a\tb
print R'\n\r\t\\' #: \n\r\t\\

#%% id_fstring_error,barebones
f"a{b + 3}" #! name 'b' is not defined

#%% id_access,barebones
def foo():
    a = 5
    def bar():
        print a
    bar()  #: 5
    a = 4
    bar()  #: 5
foo()

z = {}
def fox():
    a = 5
    def goo():
        z['x'] = 'y'
        print a
    return goo
fox()()
print z
#: 5
#: {'x': 'y'}


#%% star_err,barebones
a = (1, 2, 3)
z = *a #! unexpected star expression

#%% list,barebones
a = [4, 5, 6]
print a #: [4, 5, 6]
b = [1, 2, 3, *a]
print b #: [1, 2, 3, 4, 5, 6]

#%% set,barebones
gs = {1.12}
print gs #: {1.12}
fs = {1, 2, 3, 1, 2, 3}
gs.add(1.12)
gs.add(1.13)
print fs, gs #: {1, 2, 3} {1.12, 1.13}
print {*fs, 5, *fs} #: {1, 2, 3, 5}

#%% dict,barebones
gd = {1: 'jedan', 2: 'dva', 2: 'two', 3: 'tri'}
fd = {}
fd['jedan'] = 1
fd['dva'] = 2
print gd, fd #: {1: 'jedan', 2: 'two', 3: 'tri'} {'jedan': 1, 'dva': 2}

#%% comprehension,barebones
l = [(i, j, f'i{i}/{j}')
     for i in range(50) if i % 2 == 0 if i % 3 == 0
     for j in range(2) if j == 1]
print l #: [(0, 1, 'i0/1'), (6, 1, 'i6/1'), (12, 1, 'i12/1'), (18, 1, 'i18/1'), (24, 1, 'i24/1'), (30, 1, 'i30/1'), (36, 1, 'i36/1'), (42, 1, 'i42/1'), (48, 1, 'i48/1')]

s = {i%3 for i in range(20)}
print s #: {0, 1, 2}

d = {i: j for i in range(10) if i < 1 for j in range(10)}
print d  #: {0: 9}

x = {t: lambda x: x * t for t in range(5)}
print(x[3](10))  #: 30

#%% comprehension_opt,barebones
@extend
class List:
    def __init__(self, cap: int):
        print 'optimize', cap
        self.arr = Array[T](cap)
        self.len = 0
def foo():
    yield 0
    yield 1
    yield 2
print [i for i in range(3)] #: optimize 3
#: [0, 1, 2]
print [i for i in foo()] #: [0, 1, 2]
print [i for i in range(3) if i%2 == 0] #: [0, 2]
print [i + j for i in range(1) for j in range(1)] #: [0]
print {i for i in range(3)} #: {0, 1, 2}

#%% comprehension_opt_clone
import sys
z = [i for i in sys.argv]

#%% generator,barebones
z = 3
g = (e for e in range(20) if e % z == 1)
print str(g)[:13] #: <generator at
print list(g) #: [1, 4, 7, 10, 13, 16, 19]

g1 = (a for a in range(3))
print list(g1) #: [0, 1, 2]
g2 = (a for a in range(z + 1))
print list(g2) #: [0, 1, 2, 3]

def nest(z):
    g1 = (a for a in range(3))
    print list(g1) #: [0, 1, 2]
    g2 = (a for a in range(z + 1))
    print list(g2) #: [0, 1, 2, 3, 4]
nest(4)

#%% cond,barebones
a = 5
print (1 <= a <= 10), (1 >= a >= -5) #: True False

#%% if,barebones
c = 5
a = 1 if c < 5 else 2
b = -(1 if c else 2)
print a, b #: 2 -1

#%% unary,barebones
a, b = False, 1
print not a, not b, ~b, +b, -b, -(+(-b)) #: True False -2 1 -1 1

#%% binary,barebones
x, y = 1, 0
c = [1, 2, 3]

print x and y, x or y #: False True
print x in c, x not in c #: True False
print c is c, c is not c #: True False

z: Optional[int] = None
print z is None, None is z, None is not z, None is None #: True True False True

#%% chain_binary,barebones
def foo():
    print 'foo'
    return 15
a = b = c = foo() #: foo
print a, b, c #: 15 15 15

x = y = []
x.append(1)
print x, y #: [1] [1]

print 1 <= foo() <= 10 #: foo
#: False
print 15 >= foo()+1 < 30 > 20 > foo()
#: foo
#: False
print 15 >= foo()-1 < 30 > 20 > foo()
#: foo
#: foo
#: True

print True == (b == 15) #: True

#%% pipe_error,barebones
def b(a, b, c, d):
    pass
1 |> b(1, ..., 2, ...)  #! multiple ellipsis expressions

#%% index_normal,barebones
t: tuple[int, int] = (1, 2)
print t #: (1, 2)

tt: Tuple[int] = (1, )
print tt #: (1,)

def foo(i: int) -> int:
    return i + 1
f: Callable[[int], int] = foo
print f(1) #: 2
fx: function[[int], int] = foo
print fx(2) #: 3
fxx: Function[[int], int] = foo
print fxx(3) #: 4

#%% index_special,barebones
class Foo:
    def __getitem__(self, foo):
        print foo
f = Foo()
f[0,0] #: (0, 0)
f[0,:] #: (0, slice(None, None, None))
f[:,:] #: (slice(None, None, None), slice(None, None, None))
f[:,0] #: (slice(None, None, None), 0)

#%% index_error,barebones
Ptr[9.99] #! expected type expression

#%% index_error_b,barebones
Ptr['s'] #! ''s'' does not match expected type 'T'

#%% index_error_static,barebones
Ptr[1] #! '1' does not match expected type 'T'

#%% index_error_2,barebones
Ptr[int, 's'] #! Ptr takes 1 generics (2 given)

#%% index_error_3,barebones
Ptr[1, 's'] #! Ptr takes 1 generics (2 given)

#%% call_ptr,barebones
v = 5
p = __ptr__(v)
print p[0] #: 5

#%% call_ptr_error,barebones
__ptr__(1) #! __ptr__() only takes identifiers as arguments

#%% call_ptr_error_3,barebones
v = 1
__ptr__(v, 1) #! __ptr__() takes 1 arguments (2 given)

#%% call_array,barebones
a = __array__[int](2)
a[0] = a[1] = 5
print a[0], a[1] #: 5 5

#%% call_array_error,barebones
a = __array__[int](2, 3) #! '__array__[int]' object has no method '__new__' with arguments (int, int)

#%% call_err_1,barebones
seq_print(1, name="56", 2) #! positional argument follows keyword argument

#%% call_err_2,barebones
x = (1, 2)
seq_print(1, name=*x) #! syntax error, unexpected '*'

#%% call_err_3,barebones
x = (1, 2)
seq_print(1, name=**x) #! syntax error, unexpected '*'

#%% call_collections
from collections import namedtuple as nt

ee = nt('Foo', ['x', 'y'])
f = ee(1, 2)
print f #: (x: 1, y: 2)

ee = nt('FooX', [('x', str), 'y'])
fd = ee('s', 2)
print fd #: (x: 's', y: 2)

#%% call_partial_functools
from functools import partial
def foo(x, y, z):
    print x,y,z
f1 = partial(foo, 1, z=3)
f1(2) #: 1 2 3
f2 = partial(foo, y=2)
f2(1, 2) #: 1 2 2

#%% lambda,barebones
l = lambda a, b: a + b
print l(1, 2) #: 3

e = 5
lp = lambda x: x + e
print lp(1) #: 6

e = 7
print lp(2) #: 9

def foo[T](a: T, l: Callable[[T], T]):
    return l(a)
print foo(4, lp) #: 11

def foox(a, l):
    return l(a)
print foox(4, lp) #: 11

#%% nested_lambda,barebones
def foo():
    print list(a*a for a in range(3))
foo()  #: [0, 1, 4]

#%% walrus,barebones
def foo(x):
    return x * x
if x := foo(4):
    pass
if (x := foo(4)) and False:
    print 'Nope'
print x #: 16

a = [y := foo(1), y+1, y+2]
print a #: [1, 2, 3]

print {y: b for y in [1,2,3] if (b := (y - 1))} #: {2: 1, 3: 2}
print list(b for y in [1,2,3] if (b := (y // 3))) #: [1]

#%% walrus_update,barebones
def foo(x):
    return x * x
x = 5
if x := foo(4):
    pass
print x #: 16

#%% walrus_cond_1,barebones
def foo(x):
    return x * x
if False or (x := foo(4)):
    pass
print(x) #: 16

y = (z := foo(5)) if True else 0
print(z) #: 25

#%% walrus_err,barebones
def foo(x):
    return x * x
if False and (x := foo(4)):
    pass
try:
    print(x)
except NameError:
    print("Error") #: Error

t = True
y = 0 if t else (z := foo(4))
try:
    print(z)
except NameError:
    print("Error") #: Error

#%% range_err,barebones
1 ... 3 #! unexpected range expression

#%% callable_error,barebones
def foo(x: Callable[[]]): pass  #! Callable takes 2 generics (1 given)

#%% unpack_specials,barebones
x, = 1,
print x  #: 1

a = (2, 3)
b = (1, *a[1:])
print a, b  #: (2, 3) (1, 3)

#%% nonlocal,barebones
def goo(ww):
  z = 0
  def foo(x):
    f = 10
    def bar(y):
      nonlocal z
      f = x + y
      z += y
      print('goo.foo.bar', f, z)
    bar(5)
    print('goo.foo', f)
    return bar
  b = foo(10)
  print('goo', z)
  return b
b = goo('s')
# goo.foo.bar 15 5
# goo.foo 10
# goo 5
b(11)
# goo.foo.bar 21 16
b(12)
# goo.foo.bar 22 28
b = goo(1)  # test another instantiation
# goo.foo.bar 15 5
# goo.foo 10
# goo 5
b(11)
# goo.foo.bar 21 16
b(13)
# goo.foo.bar 23 29

#%% nonlocal_error,barebones
def goo():
  z = 0
  def foo():
    z += 1
goo()  #! local variable 'z' referenced before assignment

#%% new_scoping,barebones
try:
    if True and (x := (True or (y := 1 + 2))):
        pass
    try:
        print(x)  #: True
        print(y)
    except NameError:
        print("Error")  #: Error
    print(x) #: True
    if len("s") > 0:
        print(x)  #: True
        print(y)
    print(y)  # TODO: test for __used__ usage
    print(y)  # (right now manual inspection is needed)
except NameError as e:
    print(e.message)  #: variable 'y' not yet defined

t = True
y = 0 if t else (xx := 1)
try:
    print(xx)
except NameError:
    print("Error")  #: Error

#%% new_scoping_weird,barebones
def foo():
    if len("s") == 3:
        x = 3
    def bar(y):
        print(x+y)
    x=5
    return bar
try:
    f = foo()
    f(5)
except NameError:
    print('error') #: error
    # TODO: Python works here.
    # Need to capture these vars conditionally?

#%% new_scoping_loops_try,barebones
for i in range(10):
    pass
print(i) #: 9

j = 6
for j in range(0):
    pass
print(j) #: 6

for j in range(1):
    pass
print(j) #: 0

z = 6
for z in []:
    pass
print(z) #: 6

for z in [1, 2]:
    pass
print(z) #: 2

try:
    raise ValueError("hi")
except ValueError as e:
    pass
print(e.message) #: hi

try:
    pass
except ValueError as f:
    pass
try:
    print(f.message)
except NameError:
    print('error') #: error
