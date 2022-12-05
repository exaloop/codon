# entry point for validator
@nonpure
def expect_capture(return_captures: bool, extern_captures: bool, arg_captures):
    return False

g = [0]
h = ''

@nonpure
def make_sure_globals_arent_optd_out():
    g.append(1)
    print(h)

make_sure_globals_arent_optd_out()

@test
def test_1(a):
    global g
    g = a
    assert expect_capture(False, True, ())  # a
test_1([42])

@test
def test_2(a, b, c):
    x = c
    b[0] = a
    y = x
    assert expect_capture(False, False, (1,))  # a
    assert expect_capture(False, False, ())    # b
    assert expect_capture(False, False, ())    # c
test_2([42], [[1]], ['x'])

@test
def test_3(a):
    global g
    x = [1]
    p = __ptr__(x)
    p[0] = a
    q = p
    g = p[0]
    assert expect_capture(False, True, ())  # a
test_3([42])

@test
def test_4(a):
    global g
    v = [a]
    g = v[0]
    assert expect_capture(False, True, ())  # a
test_4([42])

@test
def test_5(a):
    global g
    v = [a]
    for i in v:
        g = i
    assert expect_capture(False, True, ())  # a
test_5([42])

@test
def test_6(a, b, c):
    a[0] = b
    c[0] = a
    assert expect_capture(False, False, (2,))    # a
    assert expect_capture(False, False, (0, 2))  # b
    assert expect_capture(False, False, ())      # c
test_6([[0]], [42], [[[0]]])

@test
def test_7(a, b, c):
    assert expect_capture(True, False, ())   # a
    assert expect_capture(False, False, ())  # b
    assert expect_capture(True, False, ())   # c
    return a if b else c
test_7([11], g, [22])

class X:
    v: List[List[int]]

@test
def test_8(a):
    x = X([])
    x.v.append(a)
    assert expect_capture(True, False, ())  # a
    return x
test_8([42])

@test
def test_9(a):
    a = [0]
    assert expect_capture(False, False, ())  # a
    return a
test_9([42])

@test
def test_10(a, b):
    if b:
        a = [0]
    assert expect_capture(True, False, ())   # a
    assert expect_capture(False, False, ())  # b
    return a
test_10([42], [99])

@test
def test_11(a):
    global g
    g = a
    assert expect_capture(True, True, ())  # a
    return g
test_11([42])

@test
def test_12(a, b):
    global g
    b[0] = a
    x = {2: b}
    y = [x]
    z = {'z': y}
    g = z['z'][0][2][0]
    assert expect_capture(True, True, (1,))  # a
    assert expect_capture(True, True, (0,))  # b
    return z
test_12([42], [[0]])

@test
def test_13(a, n):
    if n > 0:
        test_13(a, n - 1)
    assert expect_capture(True, True, ())    # a
    assert expect_capture(False, False, ())  # b
    return a
test_13([42], 3)

@test
def test_14(a):
    def assign_global(a):
        global g
        g = a

    assign_global(a)
    assert expect_capture(True, True, ())  # a
    return g
test_14([42])

class A:
    a: Optional[A]

@test
def test_15(a, b):
    a.a = b
    b.a = a
    assert expect_capture(False, False, (1,))  # a
    assert expect_capture(False, False, (0,))  # b
test_15(A(None), A(None))

@test
def test_16(a, b):
    a.a = b
    b.a = a
    assert expect_capture(True, False, (1,))  # a
    assert expect_capture(True, False, (0,))  # b
    return a
test_16(A(None), A(None))

@test
def test_17(a):
    global h
    h = a[1:-1]
    assert expect_capture(True, True, ())  # a
    return a[2:-2]
test_17('hello world')

@test
def test_18(a, b):
    if a:
        x = (b, b)
        raise ValueError(x[len(b)])
    assert expect_capture(False, False, ())  # a
    assert expect_capture(False, True, ())   # b
test_18([0 for _ in range(0)], 'b')

def assign1(x, y):
    x[0] = y

@test
def test_19(a, b, cond, elem):
    assign1(a if cond else b, elem)
    assert expect_capture(False, False, ())     # a
    assert expect_capture(True, False, ())      # b
    assert expect_capture(False, False, ())     # cond
    assert expect_capture(True, False, (0, 1))  # elem
    return b
test_19(['a'], ['b'], [True], 'x')

@test
def test_20(x):
    a = ['']
    p = a
    p[0] = x
    assert expect_capture(True, False, ())  # x
    return a
test_20('x')

@test
def test_21(x):
    a = ''
    p = __ptr__(a)
    p[0] = x
    assert expect_capture(True, False, ())  # x
    return a
test_21('x')

class A:
    a: List[str]

@test
def test_22(x):
    a = ['']
    p = A([])
    p.a = a
    p.a[0] = x
    assert expect_capture(True, False, ())  # x
    return a
test_22('x')

def assign(p, a):
    p.a = a

def test_23(x):
    a = ['']
    p = A([])
    assign(p, a)
    p.a[0] = x
    assert expect_capture(True, False, ())  # x
    return a
test_23('x')

class S:
    s: str

@test
def test_24(a, b, cond):
    q = S('')
    if cond:
        q = a
    q.s = b
    assert expect_capture(False, False, ())    # a
    assert expect_capture(False, False, (0,))  # b
    assert expect_capture(False, False, ())    # cond
test_24(S('s'), 'b', True)

@test
def test_25(a, b, v):
    q = S('')
    for i in v:
        if i:
            q = a
        else:
            q = S('q')
    q.s = b
    assert expect_capture(False, False, ())    # a
    assert expect_capture(False, False, (0,))  # b
    assert expect_capture(False, False, ())    # v
test_25(S('s'), 'b', [0,1,0,1])

@test
def test_26(a, b, v):
    q = S('')
    for i in v:
        if i:
            q = a
        else:
            q.s = b
    return q
    assert expect_capture(True, False, ())    # a
    assert expect_capture(True, False, (0,))  # b
    assert expect_capture(False, False, ())   # v
    return q
test_26(S('s'), 'b', [0,1,0,1])

@test
def test_27(x):
    a = ''
    p = __ptr__(a)
    q = __ptr__(p)
    q[0][0] = x
    assert expect_capture(True, False, ())  # x
    return a
test_27('x')
