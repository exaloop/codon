import gpu

@test
def test_hello_world():
    @gpu.kernel
    def kernel(a, b, c):
        i = gpu.thread.x
        c[i] = a[i] + b[i]

    a = [i for i in range(16)]
    b = [2*i for i in range(16)]
    c = [0 for _ in range(16)]
    kernel(a, b, c, grid=1, block=16)

    assert c == [3*i for i in range(16)]

@test
def test_raw():
    @gpu.kernel
    def kernel(a, b, c):
        i = gpu.thread.x
        c[i] = a[i] + b[i]

    a = [i for i in range(16)]
    b = [2*i for i in range(16)]
    c = [0 for _ in range(16)]
    kernel(gpu.raw(a), gpu.raw(b), gpu.raw(c), grid=1, block=16)

    assert c == [3*i for i in range(16)]

@test
def test_conversions():
    @gpu.kernel
    def kernel(x, v):
        v[0] = x

    def empty_tuple(x):
        if staticlen(x) == 0:
            return ()
        else:
            T = type(x[0])
            return (T(),) + empty_tuple(x[1:])

    def check(x):
        if isinstance(x, Tuple):
            e = empty_tuple(x)
        else:
            e = type(x)()
        v = [e]
        kernel(x, v, grid=1, block=1)
        return v == [x]

    assert check(None)
    assert check(42)
    assert check(3.14)
    assert check(f32(2.718))
    assert check(byte(99))
    assert check(Int[128](123123))
    assert check(UInt[128](321321))
    assert check(Optional[int]())
    assert check(Optional(111))
    assert check((1, 2, 3))
    assert check(([1], [2], [3]))
    # assert check(())  # TODO: PTX can't handle this; why?
    assert check(DynamicTuple((1, 2, 3)))
    assert check(DynamicTuple(([1], [2], [3])))
    assert check(DynamicTuple[int]())
    assert check(DynamicTuple[List[List[List[str]]]]())
    assert check('hello world')
    assert check([1, 2, 3])
    assert check([[1], [2], [3]])
    assert check({1: [1.1], 2: [2.2]})
    assert check({'a', 'b', 'c'})
    assert check(Optional([1, 2, 3]))

@test
def test_user_classes():
    @dataclass(gpu=True, eq=True)
    class A:
       x: int
       y: List[int]

    @tuple
    class B:
        x: int
        y: List[int]

    @gpu.kernel
    def kernel(a, b, c):
        a.x += b.x + c[0]
        c[1][0][0] = 9999
        a.y[0] = c[0] + 1
        b.y[0] = c[0] + 2

    a = A(42, [-1])
    b = B(100, [-2])
    c = (1000, [[-1]])
    kernel(a, b, c, grid=1, block=1)

    assert a == A(1142, [1001])
    assert b == B(100, [1002])
    assert c == (1000, [[9999]])

    @gpu.kernel
    def kernel2(a, b, c):
        a[0].x += b[0].x + c[0][0]
        c[0][1][0][0] = 9999
        a[0].y[0] = c[0][0] + 1
        b[0].y[0] = c[0][0] + 2

    a = [A(42, [-1])]
    b = [B(100, [-2])]
    c = [(1000, [[-1]])]
    kernel2(a, b, c, grid=1, block=1)

    assert a == [A(1142, [1001])]
    assert b == [B(100, [1002])]
    assert c == [(1000, [[9999]])]

@test
def test_intrinsics():
    @gpu.kernel
    def kernel(v):
        block_id = (gpu.block.x + gpu.block.y*gpu.grid.dim.x +
                    gpu.block.z*gpu.grid.dim.x*gpu.grid.dim.y)
        thread_id = (block_id*gpu.block.dim.x*gpu.block.dim.y*gpu.block.dim.z +
                     gpu.thread.z*gpu.block.dim.x*gpu.block.dim.y +
                     gpu.thread.y*gpu.block.dim.x +
                     gpu.thread.x)
        v[thread_id] = thread_id
        gpu.syncthreads()

    grid = gpu.Dim3(3, 4, 5)
    block = gpu.Dim3(6, 7, 8)
    N = grid.x * grid.y * grid.z * block.x * block.y * block.z
    v = [0 for _ in range(N)]
    kernel(v, grid=grid, block=block)
    assert v == list(range(N))

@test
def test_matmul():
    A = [[12, 7, 3],
         [4, 5, 6],
         [7, 8, 9]]

    B = [[5, 8, 1, 2],
         [6, 7, 3, 0],
         [4, 5, 9, 1]]

    def mmz(A, B):
        return [[0]*len(B[0]) for _ in range(len(A))]

    def matmul(A, B):
        result = mmz(A, B)
        for i in range(len(A)):
            for j in range(len(B[0])):
                for k in range(len(B)):
                    result[i][j] += A[i][k] * B[k][j]
        return result

    expected = matmul(A, B)

    @gpu.kernel
    def kernel(A, B, result):
        i = gpu.thread.x
        j = gpu.thread.y
        result[i][j] = sum(A[i][k]*B[k][j] for k in range(len(A[0])))

    result = mmz(A, B)
    kernel(A, B, result, grid=1, block=(len(result), len(result[0])))
    assert result == expected

MAX    = 1000  # maximum Mandelbrot iterations
N      = 256   # width and height of image

@test
def test_mandelbrot():
    pixels = [0 for _ in range(N * N)]

    def scale(x, a, b):
        return a + (x/N)*(b - a)

    expected = [0 for _ in range(N * N)]
    for i in range(N):
        for j in range(N):
            c = complex(scale(j, -2.00, 0.47), scale(i, -1.12, 1.12))
            z = 0j
            iteration = 0

            while abs(z) <= 2 and iteration < MAX:
                z = z**2 + c
                iteration += 1

            expected[N*i + j] = int(255 * iteration/MAX)

    @gpu.kernel
    def kernel(pixels):
        idx = (gpu.block.x * gpu.block.dim.x) + gpu.thread.x
        i, j = divmod(idx, N)
        c = complex(scale(j, -2.00, 0.47), scale(i, -1.12, 1.12))
        z = 0j
        iteration = 0

        while abs(z) <= 2 and iteration < MAX:
            z = z**2 + c
            iteration += 1

        pixels[idx] = int(255 * iteration/MAX)

    kernel(pixels, grid=(N*N)//1024, block=1024)
    assert pixels == expected

@test
def test_kitchen_sink():
    @gpu.kernel
    def kernel(x):
        i = gpu.thread.x
        d = {1: 2.1, 2: 3.5, 3: 4.2}
        s = {4, 5, 6}
        z = sum(
            d.get(x[i], j) + (j if i in s else -j)
            for j in range(i)
        )
        x[i] = int(z)

    x = [i for i in range(16)]
    kernel(x, grid=1, block=16)
    assert x == [0, 2, 6, 9, 12, 20, 30, 0, 0, 0, 0, 0, 0, 0, 0, 0]

@test
def test_auto_par():
    a = [i for i in range(16)]
    b = [2*i for i in range(16)]
    c = [0 for _ in range(16)]

    @par(gpu=True)
    for i in range(16):
        c[i] = a[i] + b[i]

    assert c == [3*i for i in range(16)]

    @par(gpu=True)
    for i in range(16):
        c[i] += a[i] + b[i]

    assert c == [6*i for i in range(16)]

    N = 200
    Z = 42
    x = [0] * (N*N)
    y = [0] * (N*N)

    for i in range(2, N - 1, 3):
        for j in range(3, N, 2):
            x[i*N + j] = i + j + Z

    @par(gpu=True, collapse=2)
    for i in range(2, N - 1, 3):
        for j in range(3, N, 2):
            y[i*N + j] = i + j + Z

    assert x == y

    @par(gpu=True)
    for i in range(1):
        pass

test_hello_world()
test_raw()
test_conversions()
test_user_classes()
test_intrinsics()
test_matmul()
test_mandelbrot()
test_kitchen_sink()
test_auto_par()
