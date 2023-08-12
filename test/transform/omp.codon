import openmp as omp
import threading as thr

lock = thr.Lock()

@tuple
class A:
    n: int

    def __new__() -> A:
        return A(0)

    def __add__(self, other: A):
        return A(self.n + other.n)

    def __atomic_add__(a: Ptr[A], other: A):
        with lock:
            a[0] = A(a[0].n + other.n)

@test
def test_omp_api():
    thr.active_count()
    thr.get_native_id()
    omp.set_num_threads(4)
    omp.get_num_threads()
    omp.get_max_threads()
    omp.get_thread_num()
    omp.get_num_procs()
    omp.in_parallel()
    omp.set_dynamic(False)
    omp.get_dynamic()
    omp.get_cancellation()
    omp.set_schedule('static', 10)
    omp.get_schedule()
    omp.get_thread_limit()
    omp.set_max_active_levels(1)
    omp.get_max_active_levels()
    omp.get_level()
    omp.get_ancestor_thread_num(0)
    omp.get_team_size(0)
    omp.get_active_level()
    omp.in_final()
    omp.get_proc_bind()
    omp.set_default_device(0)
    omp.get_default_device()
    omp.get_num_devices()
    omp.get_num_teams()
    omp.get_team_num()
    omp.is_initial_device()
    omp.get_wtime()
    omp.get_wtick()

@test
def test_omp_schedules():
    omp.set_num_threads(4)
    N = 10001

    x = list(range(N))
    y = [0] * N
    @par
    for i in range(N):
        y[i] = x[i] ** 2
    assert all(y[i] == x[i]**2 for i in range(N))

    x = list(range(N))
    y = [0] * N
    @par(schedule='static')
    for i in range(N):
        y[i] = x[i] ** 2
    assert all(y[i] == x[i]**2 for i in range(N))

    x = list(range(N))
    y = [0] * N
    @par(schedule='static', chunk_size=1)
    for i in range(N):
        y[i] = x[i] ** 2
    assert all(y[i] == x[i]**2 for i in range(N))

    x = list(range(N))
    y = [0] * N
    chunk = 13
    @par(schedule='static', chunk_size=chunk)
    for i in range(N):
        y[i] = x[i] ** 2
    assert all(y[i] == x[i]**2 for i in range(N))

    x = list(range(N))
    y = [0] * N
    @par(schedule='static', chunk_size=N-1)
    for i in range(N):
        y[i] = x[i] ** 2
    assert all(y[i] == x[i]**2 for i in range(N))

    x = list(range(N))
    y = [0] * N
    @par(schedule='static', chunk_size=N)
    for i in range(N):
        y[i] = x[i] ** 2
    assert all(y[i] == x[i]**2 for i in range(N))

    x = list(range(N))
    y = [0] * N
    @par(schedule='static', chunk_size=N+1)
    for i in range(N):
        y[i] = x[i] ** 2
    assert all(y[i] == x[i]**2 for i in range(N))

    x = list(range(N))
    y = [0] * N
    @par(schedule='dynamic')
    for i in range(N):
        y[i] = x[i] ** 2
    assert all(y[i] == x[i]**2 for i in range(N))

    x = list(range(N))
    y = [0] * N
    @par(schedule='dynamic', chunk_size=1)
    for i in range(N):
        y[i] = x[i] ** 2
    assert all(y[i] == x[i]**2 for i in range(N))

    x = list(range(N))
    y = [0] * N
    chunk = 17
    @par(schedule='dynamic', chunk_size=chunk)
    for i in range(N):
        y[i] = x[i] ** 2
    assert all(y[i] == x[i]**2 for i in range(N))

    x = list(range(N))
    y = [0] * N
    @par(schedule='dynamic', chunk_size=N-1)
    for i in range(N):
        y[i] = x[i] ** 2
    assert all(y[i] == x[i]**2 for i in range(N))

    x = list(range(N))
    y = [0] * N
    @par(schedule='dynamic', chunk_size=N)
    for i in range(N):
        y[i] = x[i] ** 2
    assert all(y[i] == x[i]**2 for i in range(N))

    x = list(range(N))
    y = [0] * N
    @par(schedule='dynamic', chunk_size=N+1)
    for i in range(N):
        y[i] = x[i] ** 2
    assert all(y[i] == x[i]**2 for i in range(N))

@test
def test_omp_ranges():
    nt = 4
    lock = thr.Lock()
    seen = set()

    @omp.critical
    def add(seen, i):
        seen.add(i)

    @par
    for i in range(3, 123, 7):
        add(seen, i)
    assert seen == set(range(3, 123, 7))
    seen.clear()

    @par
    for i in range(-3, -123, 7):
        with lock:
            seen.add(i)
    assert seen == set(range(-3, -123, 7))
    seen.clear()

    @par(num_threads=nt)
    for i in range(-3, -123, -7):
        add(seen, i)
    assert seen == set(range(-3, -123, -7))
    seen.clear()

    @par(chunk_size=12)
    for i in range(3, 123, 7):
        with lock:
            seen.add(i)
    assert seen == set(range(3, 123, 7))
    seen.clear()

    @par(chunk_size=12)
    for i in range(-3, -123, 7):
        add(seen, i)
    assert seen == set(range(-3, -123, 7))
    seen.clear()

    @par(chunk_size=12, num_threads=nt)
    for i in range(-3, -123, -7):
        with lock:
            seen.add(i)
    assert seen == set(range(-3, -123, -7))
    seen.clear()

    @par(chunk_size=10000)
    for i in range(3, 123, 7):
        add(seen, i)
    assert seen == set(range(3, 123, 7))
    seen.clear()

    @par(chunk_size=10000)
    for i in range(-3, -123, 7):
        with lock:
            seen.add(i)
    assert seen == set(range(-3, -123, 7))
    seen.clear()

    @par(chunk_size=10000, num_threads=nt)
    for i in range(-3, -123, -7):
        add(seen, i)
    assert seen == set(range(-3, -123, -7))
    seen.clear()

    @par(schedule='dynamic', num_threads=nt)
    for i in range(-3, -123, -7):
        with lock:
            seen.add(i)
    assert seen == set(range(-3, -123, -7))
    seen.clear()

    @par(schedule='dynamic', chunk_size=12)
    for i in range(3, 123, 7):
        add(seen, i)
    assert seen == set(range(3, 123, 7))
    seen.clear()

    @par(schedule='dynamic', chunk_size=12)
    for i in range(-3, -123, 7):
        with lock:
            seen.add(i)
    assert seen == set(range(-3, -123, 7))
    seen.clear()

    @par(schedule='dynamic', chunk_size=12, num_threads=nt)
    for i in range(-3, -123, -7):
        add(seen, i)
    assert seen == set(range(-3, -123, -7))
    seen.clear()

my_global = 42

class Vector:
    x: float
    y: float

    def __init__(self):
        self.x = 0.0
        self.y = 0.0

    def __add__(self, other: Vector):
        return Vector(self.x + other.x, self.y + other.y)

    def __str__(self):
        return f'<{self.x}, {self.y}>'

@test
def test_omp_reductions():
    def expected(N, a, op):
        for i in range(N):
            a = op(a, type(a)(i))
        return a

    from math import inf
    omp.set_num_threads(4)
    N = 10001
    L = list(range(N))

    # static
    a = 0
    @par
    for i in L:
        a += i
    assert a == expected(N, 0, int.__add__)

    a = 0
    @par
    for i in L:
        a |= i
    assert a == expected(N, 0, int.__or__)

    a = 0
    @par
    for i in L:
        a ^= i
    assert a == expected(N, 0, int.__xor__)

    a = 0xffffffff
    @par
    for i in L:
        a &= i
    assert a == expected(N, 0xffffffff, int.__and__)

    a = 1
    @par
    for i in L:
        a *= i
    assert a == expected(N, 1, int.__mul__)

    a = 0
    @par
    for i in L:
        b = N+1 if i == N//2 else i
        a = max(a, b)
    assert a == N+1

    a = 0
    @par
    for i in L:
        b = -1 if i == N//2 else i
        a = min(a, b)
    assert a == -1

    x = A(0)
    @par
    for i in L:
        x += A(i)
    assert x.n == expected(N, 0, int.__add__)

    # static chunked
    a = 0
    @par(chunk_size=3)
    for i in L:
        a += i
    assert a == expected(N, 0, int.__add__)

    a = 0
    @par(chunk_size=3)
    for i in L:
        a |= i
    assert a == expected(N, 0, int.__or__)

    a = 0
    @par(chunk_size=3)
    for i in L:
        a ^= i
    assert a == expected(N, 0, int.__xor__)

    a = 0xffffffff
    @par(chunk_size=3)
    for i in L:
        a &= i
    assert a == expected(N, 0xffffffff, int.__and__)

    a = 1
    @par(chunk_size=3)
    for i in L[1:10]:
        a *= i
    assert a == 1*2*3*4*5*6*7*8*9

    a = 0
    @par(chunk_size=3)
    for i in L:
        b = N+1 if i == N//2 else i
        a = max(a, b)
    assert a == N+1

    a = 0
    @par(chunk_size=3)
    for i in L:
        b = -1 if i == N//2 else i
        a = min(a, b)
    assert a == -1

    x = A(0)
    @par(chunk_size=3)
    for i in L:
        x += A(i)
    assert x.n == expected(N, 0, int.__add__)

    # dynamic
    a = 0
    @par(schedule='dynamic')
    for i in L:
        a += i
    assert a == expected(N, 0, int.__add__)

    a = 0
    @par(schedule='dynamic')
    for i in L:
        a |= i
    assert a == expected(N, 0, int.__or__)

    a = 0
    @par(schedule='dynamic')
    for i in L:
        a ^= i
    assert a == expected(N, 0, int.__xor__)

    a = 0xffffffff
    @par(schedule='dynamic')
    for i in L:
        a &= i
    assert a == expected(N, 0xffffffff, int.__and__)

    a = 1
    @par(schedule='dynamic')
    for i in L[1:10]:
        a *= i
    assert a == 1*2*3*4*5*6*7*8*9

    a = 0
    @par(schedule='dynamic')
    for i in L:
        b = N+1 if i == N//2 else i
        a = max(a, b)
    assert a == N+1

    a = 0
    @par(schedule='dynamic')
    for i in L:
        b = -1 if i == N//2 else i
        a = min(a, b)
    assert a == -1

    x = A(0)
    @par(schedule='dynamic')
    for i in L:
        x += A(i)
    assert x.n == expected(N, 0, int.__add__)

    # floats
    c = 0.
    @par
    for i in L:
        c += float(i)
    assert c == expected(N, 0., float.__add__)

    c = 1.
    @par
    for i in L[1:10]:
        c *= float(i)
    assert c == float(1*2*3*4*5*6*7*8*9)

    c = 0.
    @par
    for i in L:
        b = float(N+1 if i == N//2 else i)
        c = max(b, c)
    assert c == float(N+1)

    c = 0.
    @par
    for i in L:
        b = float(-1 if i == N//2 else i)
        c = min(b, c)
    assert c == -1.

    # float32s
    c = f32(0.)
    # this one can give different results due to
    # non-commutativity of floats; so limit to 1001
    @par
    for i in L[1:1001]:
        c += f32(i)
    assert c == sum((f32(i) for i in range(1001)), f32(0))

    c = f32(1.)
    @par
    for i in L[1:10]:
        c *= f32(i)
    assert c == f32(1*2*3*4*5*6*7*8*9)

    c = f32(0.)
    @par
    for i in L:
        b = f32(N+1 if i == N//2 else i)
        c = max(b, c)
    assert c == f32(N+1)

    c = f32(0.)
    @par
    for i in L:
        b = f32(-1 if i == N//2 else i)
        c = min(b, c)
    assert c == f32(-1.)

    x_add = 10.
    x_min = inf
    x_max = -inf
    @par
    for i in L:
        x_i = float(i)
        x_add += x_i
        x_min = min(x_min, x_i)
        x_max = max(x_i, x_max)
    assert x_add == expected(N, 10., float.__add__)
    assert x_min == expected(N, inf, min)
    assert x_max == expected(N, -inf, max)

    x_mul = 2.
    @par
    for i in L[:10]:
        x_i = float(i)
        x_mul *= x_i
    assert x_mul == expected(10, 2., float.__mul__)

    # multiple reductions
    global my_global
    g = my_global
    a = 0
    b = 0
    @par(schedule='dynamic', num_threads=3)
    for i in L:
        a += i
        b ^= i
        my_global += i
    assert a == expected(N, 0, int.__add__)
    assert b == expected(N, 0, int.__xor__)
    assert my_global == g + expected(N, 0, int.__add__)

    # custom reductions
    vectors = [Vector(i, i) for i in range(10)]
    v = Vector()
    @par
    for vv in vectors:
        v += vv
    assert v.x == 45.0
    assert v.y == 45.0

another_global = 0
@test
def test_omp_critical():
    @omp.critical
    def foo(i):
        global another_global
        another_global += i

    @omp.critical
    def bar(i):
        global another_global
        another_global += i

    global another_global
    for n in (99999, 100000, 100001):
        another_global = 0
        @par(schedule='dynamic')
        for i in range(n):
            foo(i)
            bar(i)
        assert another_global == 2*sum(range(n))

@test
def test_omp_non_imperative():
    def squares(N):
        for i in range(N):
            yield i*i

    N = 10001
    v = [0] * N

    @par
    for i,s in enumerate(squares(N)):
        v[i] = s

    assert all(s == i*i for i,s in enumerate(v))

test_generator_based_loops_global = 0.7

@test
def test_omp_non_imperative_reductions():
    def squares(n):
        for i in range(n):
            yield i*i

    @omp.critical
    def add(v, x):
        v.add(x)

    @nonpure
    def foo(x):
        return x

    global test_generator_based_loops_global
    N = 1001

    # no reductions
    v = set()
    @par
    for i in squares(N):
        x = i - 1
        add(v, x)
    assert v == {i**2 - 1 for i in range(N)}

    # one reduction
    a = 0
    @par
    for i in squares(N):
        a += i
    assert a == 333833500

    # kitchen sink
    a = 7
    b = 0
    c = 0.5
    d = foo(0)
    e = foo(1)
    f = foo(-1)
    g = -1
    h = Vector(1.5, 1.25)
    @par
    for i in squares(N):
        x = foo(i) + d
        y = foo(x) + e + f
        a = x + a + d
        b ^= y - d
        c += x + d
        test_generator_based_loops_global += y - d
        f = foo(-1)
        g = max(i, g)
        h += Vector(i, i)
    assert a == 333833507
    assert b == 332752
    assert c == 333833500.5
    assert test_generator_based_loops_global == 333833500.7
    assert g == (N - 1) ** 2
    assert h.x == 333833501.5
    assert h.y == 333833501.25

@test
def test_omp_transform(a, b, c):
    a0, b0, c0 = a, b, c
    d = a + b + c
    v = list(range(int(d*d)))
    ids = set()

    @par('schedule(static, 5) num_threads(3) ordered')
    for i in v:
        a += type(a)(i)
        z = i * i
        c = type(c)(z)
        b += type(b)(z)
        with lock:
            ids.add(omp.get_thread_num())

    for i in v:
        a0 += type(a0)(i)
        z = i * i
        c0 = type(c0)(z)
        b0 += type(b0)(z)

    assert ids == {0, 1, 2}
    assert int(a) == int(a0)
    assert abs(b - b0) < b/1e6
    assert c == v[-1] ** 2

@test
def test_omp_nested():
    def squares(n):
        for i in range(n):
            yield i*i

    N = 100
    v = []

    v.clear()
    @par
    for i in range(N):
        @par
        for j in range(i):
            with lock:
                v.append(i + j)
    assert set(v) == {i + j for i in range(N) for j in range(i)}

    v.clear()
    @par
    for i in range(N):
        @par
        for j in squares(i):
            with lock:
                v.append(i + j)
    assert set(v) == {i + j for i in range(N) for j in squares(i)}

    v.clear()
    @par
    for i in squares(N):
        @par
        for j in range(i):
            with lock:
                v.append(i + j)
    assert set(v) == {i + j for i in squares(N) for j in range(i)}

    v.clear()
    @par
    for i in squares(N):
        @par
        for j in squares(i):
            with lock:
                v.append(i + j)
    assert set(v) == {i + j for i in squares(N) for j in squares(i)}

@test
def test_omp_corner_cases():
    def squares(n):
        for i in range(n):
            yield i*i

    @nonpure
    def foo(x):
        return x

    v = list(range(10))

    @par
    for i in range(10):
        pass

    @par
    for i in v:
        pass

    @par(num_threads=2)
    for i in range(10):
        pass

    @par(schedule='dynamic')
    for i in range(10):
        pass

    @par(num_threads=2, schedule='dynamic')
    for i in range(10):
        pass

    @par
    for i in squares(10):
        pass

    @par(num_threads=2)
    for i in squares(10):
        pass

    @par
    for i in range(10):
        foo(i)

    @par
    for i in squares(10):
        foo(i)

    @par
    for i in range(10):
        a = foo(i)

    @par
    for i in squares(10):
        a = foo(i)

    @par
    for i in range(10):
        i += i

    @par
    for i in squares(10):
        i += i

@test
def test_omp_collapse():
    # trivial
    A0 = []
    B0 = []

    for i in range(10):
        A0.append(i)

    @par(num_threads=4, collapse=1)
    for i in range(10):
        with lock:
            B0.append(i)

    assert sorted(A0) == sorted(B0)

    # basic
    A1 = []
    B1 = []

    for i in range(10):
        for j in range(10):
            A1.append((i,j))

    @par(num_threads=4, collapse=2)
    for i in range(10):
        for j in range(10):
            with lock:
                B1.append((i,j))

    assert sorted(A1) == sorted(B1)

    # deep
    A2 = []
    B2 = []

    for a in range(3):
        for b in range(4):
            for c in range(5):
                for d in range(6):
                    A2.append((a,b,c,d))

    @par(num_threads=4, collapse=4)
    for a in range(3):
        for b in range(4):
            for c in range(5):
                for d in range(6):
                    with lock:
                        B2.append((a,b,c,d))

    assert sorted(A2) == sorted(B2)

    # ranges 1
    A3 = []
    B3 = []

    for a in range(-5,5,2):
        for b in range(5,-7,-2):
            for c in range(0,17,3):
                for d in range(5):
                    A3.append((a,b,c,d))

    @par(num_threads=4, collapse=4)
    for a in range(-5,5,2):
        for b in range(5,-7,-2):
            for c in range(0,17,3):
                for d in range(5):
                    with lock:
                        B3.append((a,b,c,d))

    assert sorted(A3) == sorted(B3)

    # ranges 2
    A4 = []
    B4 = []

    for i in range(10):
        for j in range(7,-5,-2):
            for k in range(-5,10,3):
                A4.append((i,j,k))

    @par(num_threads=4, collapse=3)
    for i in range(10):
        for j in range(7,-5,-2):
            for k in range(-5,10,3):
                with lock:
                    B4.append((i,j,k))

    assert sorted(A4) == sorted(B4)

    # zero
    B5 = []

    @noinline
    def zstart():
        return 5

    @noinline
    def zstop():
        return -5

    start = zstart()
    stop = zstop()

    @par(num_threads=4, collapse=3)
    for i in range(10):
        for j in range(start, stop, 1):
            for k in range(-5,10,3):
                with lock:
                    B5.append((i,j,k))

    assert len(B5) == 0

    # order
    A6 = []
    B6 = []

    for a in range(-5,5,2):
        for b in range(5,-7,-2):
            for c in range(0,17,3):
                for d in range(5):
                    A6.append((a,b,c,d))

    @par(num_threads=1, collapse=4)
    for a in range(-5,5,2):
        for b in range(5,-7,-2):
            for c in range(0,17,3):
                for d in range(5):
                    B6.append((a,b,c,d))  # no lock since threads=1

    assert A6 == B6

@test
def test_omp_ordered(N: int = 1000):
    @omp.ordered
    def f(A, i):
        A.append(i)

    A = []

    @par(schedule='dynamic', chunk_size=1, num_threads=2, ordered=True)
    for i in range(N):
        f(A, i)

    assert A == list(range(N))

test_omp_api()
test_omp_schedules()
test_omp_ranges()
test_omp_reductions()
test_omp_critical()
test_omp_non_imperative()
test_omp_non_imperative_reductions()
test_omp_transform(111, 222, 333)
test_omp_transform(111.1, 222.2, 333.3)
test_omp_nested()
test_omp_corner_cases()
test_omp_collapse()
test_omp_ordered()
