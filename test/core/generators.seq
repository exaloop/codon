v = range(5)
print len(v)  # EXPECT: 5
print 2 in v  # EXPECT: True
print 5 in v  # EXPECT: False
print [a for a in v]  # EXPECT: [0, 1, 2, 3, 4]
print list(reversed(v))  # EXPECT: [4, 3, 2, 1, 0]
print bool(v)  # EXPECT: True

v = range(10, 2, -3)
print len(v)  # EXPECT: 3
print 13 in v  # EXPECT: False
print 10 in v  # EXPECT: True
print 7 in v  # EXPECT: True
print 4 in v  # EXPECT: True
print 1 in v  # EXPECT: False
print [a for a in v]  # EXPECT: [10, 7, 4]
print list(reversed(v))  # EXPECT: [4, 7, 10]
print bool(v)  # EXPECT: True

v = range(10, 2, 3)
print len(v)  # EXPECT: 0
print 13 in v  # EXPECT: False
print 10 in v  # EXPECT: False
print 7 in v  # EXPECT: False
print 4 in v  # EXPECT: False
print 1 in v  # EXPECT: False
print [a for a in v]  # EXPECT: []
print list(reversed(v))  # EXPECT: []
print bool(v)  # EXPECT: False

def perms[T](elements: list[T]):
    if len(elements) <=1:
        yield elements
    else:
        for perm in perms(elements[1:]):
            for i in range(len(elements)):
                yield perm[:i] + elements[0:1] + perm[i:]

for a in perms([1,2,3,4]):
    print a

# EXPECT: [1, 2, 3, 4]
# EXPECT: [2, 1, 3, 4]
# EXPECT: [2, 3, 1, 4]
# EXPECT: [2, 3, 4, 1]
# EXPECT: [1, 3, 2, 4]
# EXPECT: [3, 1, 2, 4]
# EXPECT: [3, 2, 1, 4]
# EXPECT: [3, 2, 4, 1]
# EXPECT: [1, 3, 4, 2]
# EXPECT: [3, 1, 4, 2]
# EXPECT: [3, 4, 1, 2]
# EXPECT: [3, 4, 2, 1]
# EXPECT: [1, 2, 4, 3]
# EXPECT: [2, 1, 4, 3]
# EXPECT: [2, 4, 1, 3]
# EXPECT: [2, 4, 3, 1]
# EXPECT: [1, 4, 2, 3]
# EXPECT: [4, 1, 2, 3]
# EXPECT: [4, 2, 1, 3]
# EXPECT: [4, 2, 3, 1]
# EXPECT: [1, 4, 3, 2]
# EXPECT: [4, 1, 3, 2]
# EXPECT: [4, 3, 1, 2]
# EXPECT: [4, 3, 2, 1]


def mysum[T](start: T):
    m = start
    while True:
        a = (yield)
        if a == -1:
            break
        m += a
    yield m

iadder = mysum(0)
next(iadder)
for i in range(10):
    iadder.send(i)
print(iadder.send(-1))  # EXPECT: 45

fadder = mysum(0.0)
next(fadder)
for i in range(10):
    fadder.send(float(i))
print(fadder.send(-1.0))  # EXPECT: 45


@test
def test_generator_in_finally():
    def foo(n):
        if not n:
            raise ValueError('not n')
        return n

    b = False
    try:
        try:
            a = 1
            x = 2
            z = 0
            for a in (foo(i) * x for i in [a,2,3,4,z,5] * x):
                pass
        finally:
            b = True
    except ValueError as e:
        assert e.message == 'not n'
    assert b
test_generator_in_finally()
