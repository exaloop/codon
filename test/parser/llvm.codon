#%% ptr,barebones
import internal.gc as gc
print gc.sizeof(Ptr[int]) #: 8
print gc.atomic(Ptr[int]) #: False
y = Ptr[int](1)
y[0] = 11
print y[0] #: 11
_y = y.as_byte()
print int(_y[0]) #: 11
y = Ptr[int](5)
y[0] = 1; y[1] = 2; y[2] = 3; y[3] = 4; y[4] = 5
z = Ptr[int](y)
print y[1], z[2] #: 2 3
z = Ptr[int](y.as_byte())
print y[1], z[2] #: 2 3
print z.__bool__() #: True
z.__int__() # big number...
zz = z.__copy__() # this is not a deep copy!
zz[2] = 10
print z[2], zz[2] #: 10 10
print y.__getitem__(1) #: 2
y.__setitem__(1, 3)
print y[1] #: 3
print y.__add__(1)[0] #: 3
print (y + 3).__sub__(y + 1) #: 2
print y.__eq__(z) #: True
print y.__eq__(zz) #: True
print y.as_byte().__eq__('abc'.ptr) #: False
print y.__ne__(z) #: False
print y.__lt__(y+1) #: True
print y.__gt__(y+1) #: False
print (y+1).__le__(y) #: False
print y.__ge__(y) #: True
y.__prefetch_r0__()
y.__prefetch_r1__()
y.__prefetch_r2__()
y.__prefetch_r3__()
y.__prefetch_w0__()
y.__prefetch_w1__()
y.__prefetch_w2__()
y.__prefetch_w3__()

#%% int,barebones
a = int()
b = int(5)
c = int(True)
d = int(byte(1))
e = int(1.1)
print a, b, c, d, e #: 0 5 1 1 1
print a.__repr__() #: 0
print b.__copy__() #: 5
print b.__hash__() #: 5
print a.__bool__(), b.__bool__() #: False True
print a.__pos__() #: 0
print b.__neg__() #: -5
print (-b).__abs__() #: 5
print c.__truediv__(5) #: 0.2
print b.__lshift__(1) #: 10
print b.__rshift__(1) #: 2
print b.__truediv__(5.15) #: 0.970874
print a.__add__(1) #: 1
print a.__add__(1.1) #: 1.1
print a.__sub__(1) #: -1
print a.__sub__(1.1) #: -1.1
print b.__mul__(1) #: 5
print b.__mul__(1.1) #: 5.5
print b.__floordiv__(2) #: 2
print b.__floordiv__(1.1) #: 4
print b.__mod__(2) #: 1
print b.__mod__(1.1) #: 0.6
print a.__eq__(1) #: False
print a.__eq__(1.1) #: False
print a.__ne__(1) #: True
print a.__ne__(1.1) #: True
print a.__lt__(1) #: True
print a.__lt__(1.1) #: True
print a.__le__(1) #: True
print a.__le__(1.1) #: True
print a.__gt__(1) #: False
print a.__gt__(1.1) #: False
print a.__ge__(1) #: False
print a.__ge__(1.1) #: False

#%% uint,barebones
au = Int[123](15)
a = UInt[123]()
b = UInt[123](a)
a = UInt[123](15)
a = UInt[123](au)
print a.__copy__() #: 15
print a.__hash__() #: 15
print a.__bool__() #: True
print a.__pos__() #: 15
print a.__neg__() #: 10633823966279326983230456482242756593
print a.__invert__() #: 10633823966279326983230456482242756592
m = UInt[123](4)
print a.__add__(m), a.__sub__(m), a.__mul__(m), a.__floordiv__(m), a.__truediv__(m) #: 19 11 60 3 3.75
print a.__mod__(m), a.__lshift__(m), a.__rshift__(m) #: 3 240 0
print a.__eq__(m), a.__ne__(m), a.__lt__(m), a.__gt__(m), a.__le__(m), a.__ge__(m) #: False True False True False True
print a.__and__(m), a.__or__(m), a.__xor__(m) #: 4 15 11
print a, a.popcnt() #: 15 4
ac = Int[128](5)
bc = Int[32](5)
print ac, bc, int(ac), int(bc) #: 5 5 5 5

print int(Int[12](12)) #: 12
print int(Int[122](12)) #: 12
print int(Int[64](12)) #: 12
print int(UInt[12](12)) #: 12
print int(UInt[122](12)) #: 12
print int(UInt[64](12)) #: 12

print Int[32](212) #: 212
print Int[64](212) #: 212
print Int[66](212) #: 212
print UInt[32](112) #: 112
print UInt[64](112) #: 112
print UInt[66](112) #: 112


#%% float,barebones
z = float.__new__()
z = 5.5
print z.__repr__() #: 5.5
print z.__copy__() #: 5.5
print z.__bool__(), z.__pos__(), z.__neg__() #: True 5.5 -5.5
f = 1.3
print z.__floordiv__(f), z.__floordiv__(2) #: 4 2
print z.__truediv__(f), z.__truediv__(2) #: 4.23077 2.75
print z.__pow__(2.2), z.__pow__(2) #: 42.54 30.25
print z.__add__(2) #: 7.5
print z.__sub__(2) #: 3.5
print z.__mul__(2) #: 11
print z.__truediv__(2) #: 2.75
print z.__mod__(2) #: 1.5
print z.__eq__(2) #: False
print z.__ne__(2) #: True
print z.__lt__(2) #: False
print z.__gt__(2) #: True
print z.__le__(2) #: False
print z.__ge__(2) #: True

#%% bool,barebones
z = bool.__new__()
print z.__repr__() #: False
print z.__copy__() #: False
print z.__bool__(), z.__invert__() #: False True
print z.__eq__(True) #: False
print z.__ne__(True) #: True
print z.__lt__(True) #: True
print z.__gt__(True) #: False
print z.__le__(True) #: True
print z.__ge__(True) #: False
print z.__and__(True), z.__or__(True), z.__xor__(True) #: False True True

#%% byte,barebones
z = byte.__new__()
z = byte(65)
print z.__repr__() #: byte('A')
print z.__bool__() #: True
print z.__eq__(byte(5)) #: False
print z.__ne__(byte(5)) #: True
print z.__lt__(byte(5)) #: False
print z.__gt__(byte(5)) #: True
print z.__le__(byte(5)) #: False
print z.__ge__(byte(5)) #: True

#%% array,barebones
a = Array[float](5)
pa = Ptr[float](3)
z = Array[float](pa, 3)
z.__copy__()
print z.__len__() #: 3
print z.__bool__() #: True
z.__setitem__(0, 1.0)
z.__setitem__(1, 2.0)
z.__setitem__(2, 3.0)
print z.__getitem__(1) #: 2
print z.slice(0, 2).len #: 2

#%% optional,barebones
a = Optional[float]()
print bool(a) #: False
a = Optional[float](0.0)
print bool(a) #: False
a = Optional[float](5.5)
print a.__bool__(), a.__val__() #: True 5.5

#%% generator,barebones
def foo():
    yield 1
    yield 2
    yield 3
z = foo()
y = z.__iter__()
print str(y.__raw__())[:2] #: 0x
print y.__done__() #: False
print y.__promise__()[0] #: 0
y.__resume__()
print y.__repr__()[:16] #: <generator at 0x
print y.next() #: 1
print y.done() #: False
y.send(1)
y.destroy()
print y.done() #: True
