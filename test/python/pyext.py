import myext as m

def equal(v, a, b, tag):
    return v.a == a and v.b == b and v.tag == tag

# constructors #
################

x = m.Vec(3.14, 4.2, 'x')
y = m.Vec(100, 1000, tag='y')
z = m.Vec(b=2.2, a=1.1)
s = m.Vec(10)
t = m.Vec(b=11)

assert equal(x, 3.14, 4.2, 'x')
assert equal(y, 100, 1000, 'y')
assert equal(z, 1.1, 2.2, 'v0')
assert equal(s, 10, 0.0, 'v1')
assert equal(t, 0.0, 11, 'v2')

try:
    m.Vec(tag=10, a=1, b=2)
except:
    pass
else:
    assert False

# to-str #
##########

assert str(x) == 'x: <3.14, 4.2>'
assert repr(x) == "Vec(3.14, 4.2, 'x')"


# methods #
###########

assert x.foo(2.2, 3.3) == (3.14, 2.2, 3.3)
assert y.foo(3.3) == (100, 3.3, 2.22)
assert z.foo() == (1.1, 1.11, 2.22)
assert x.foo(a=2.2, b=3.3) == (3.14, 2.2, 3.3)
assert x.foo(2.2, b=3.3) == (3.14, 2.2, 3.3)
assert x.foo(b=3.3, a=2.2) == (3.14, 2.2, 3.3)
assert x.foo(a=2.2) == (3.14, 2.2, 2.22)
assert x.foo(b=3.3) == (3.14, 1.11, 3.3)

try:
    x.foo(1, a=1)
except:
    pass
else:
    assert False

try:
    x.foo(1, 2, b=2)
except:
    pass
else:
    assert False

assert equal(x.bar(), 3.14, 4.2, 'x')
assert equal(y.bar(), 100, 1000, 'y')
assert equal(z.bar(), 1.1, 2.2, 'v0')
assert equal(s.bar(), 10, 0.0, 'v1')
assert equal(t.bar(), 0.0, 11, 'v2')

try:
    x.bar(1)
except:
    pass
else:
    assert False

try:
    x.bar(z=1)
except:
    pass
else:
    assert False

assert m.Vec.baz(2.2, 3.3) == (2.2, 3.3)
assert x.baz(2.2, 3.3) == (2.2, 3.3)
assert m.Vec.baz(3.3) == (3.3, 2.22)
assert m.Vec.baz() == (1.11, 2.22)
assert m.Vec.baz(a=2.2, b=3.3) == (2.2, 3.3)
assert m.Vec.baz(2.2, b=3.3) == (2.2, 3.3)
assert m.Vec.baz(b=3.3, a=2.2) == (2.2, 3.3)
assert m.Vec.baz(a=2.2) == (2.2, 2.22)
assert m.Vec.baz(b=3.3) == (1.11, 3.3)

try:
    m.Vec.baz(1, a=1)
except:
    pass
else:
    assert False

try:
    m.Vec.baz(1, 2, b=2)
except:
    pass
else:
    assert False

assert m.Vec.nop() == 'nop'
assert x.nop() == 'nop'

# magics #
##########


