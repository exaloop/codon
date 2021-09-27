@nonpure
def __outline_begin__():
    pass

@nonpure
def __outline_end__():
    pass

@nonpure
def __outline_successes__():
    return 0

@nonpure
def __outline_failures__():
    return 0

# use this to avoid const-folding
def N(n):
    return n

@test
def test_basic_outline():
    __outline_begin__()
    print 'hello world'
    __outline_end__()
test_basic_outline()
# EXPECT: hello world

@test
def test_private_vars_outline(x):
    b = N(10)
    __outline_begin__()
    a = N(42)
    print a + 1
    a = N(99)
    __outline_end__()
    b = N(13)
test_private_vars_outline('x')
# EXPECT: 43

@test
def test_shared_vars_outline(x):
    b = N(10)
    __outline_begin__()
    a = N(42)
    print a + b + x
    a = N(99)
    y = N(-42)
    __outline_end__()
    b = N(13)
    print a
    print b
    print y
test_shared_vars_outline(100)
# EXPECT: 152
# EXPECT: 99
# EXPECT: 13
# EXPECT: -42

f = 'f'
@test
def test_shared_vars_with_mod_outline(x):
    global f
    b = N(10)
    f = 'o'
    __outline_begin__()
    for i in range(1):
        a = N(42)
        print a + b + x
        a = N(99)
        x = N(-5)
        f = str(i)
    __outline_end__()
    b = N(13)
    print b, x
test_shared_vars_with_mod_outline(100)
print f
# EXPECT: 152
# EXPECT: 13 -5
# EXPECT: 0

g = 'g'
@test
def test_out_flow_return_outline(x):
    global g
    b = N(10)
    g = 'o'
    __outline_begin__()
    for i in range(1):
        a = N(42)
        print a + b + x
        a = N(99)
        x = N(-5)
        g = str(i)
        return a
        break
    __outline_end__()
    assert False
test_out_flow_return_outline(100)
print g
# EXPECT: 152
# EXPECT: 0

h = 'h'
@test
def test_out_flow_break_continue_outline(x):
    global h
    b = N(10)
    h = 'o'
    for i in range(3):
        __outline_begin__()
        if i == 0:
            continue
        if i == 2:
            break
        for j in range(2):
            if j == 0:
                continue

            a = N(42)
            print a + b + x + j
            a = N(99)
            x = N(-5)
            h = str(i)
            break
        __outline_end__()
    b = N(13)
    print b, x
test_out_flow_break_continue_outline(100)
print h
# EXPECT: 153
# EXPECT: 13 -5
# EXPECT: 1

@test
def test_generator_return_outline(x):
    for i in range(x):
        yield i
        __outline_begin__()
        if i == 2:
            continue
        if i == 4:
            return
        __outline_end__()
        yield i
print list(test_generator_return_outline(10))
# EXPECT: [0, 0, 1, 1, 2, 3, 3, 4]

@test
def test_normal_return_outline(x):
    for i in range(x):
        yield i
        __outline_begin__()
        if i == 2:
            continue
        if i == 4:
            return
        __outline_end__()
        yield i
print list(test_normal_return_outline(1))
# EXPECT: [0, 0]

@test
def test_invalid_outline_yield(x):
    b = N(10)
    __outline_begin__()
    a = N(42)
    yield a + b + x
    a = N(99)
    __outline_end__()
    b = N(13)
test_invalid_outline_yield(-123)

@test
def test_invalid_outline_yield_in(x):
    b = N(10)
    __outline_begin__()
    a = (yield) + b
    yield b
    __outline_end__()
    b = N(13)
test_invalid_outline_yield_in(-123)

@test
def test_invalid_outline_stack_alloc():
    __outline_begin__()
    a = __array__[int](1)
    a[0] = 42
    __outline_end__()
    print a[0]
test_invalid_outline_stack_alloc()
# EXPECT: 42

print __outline_successes__()
print __outline_failures__()
# EXPECT: 8
# EXPECT: 3
