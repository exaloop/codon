@test
def test_bool_match():
    T, F = True, False

    b = False
    match T:
        case True:
            b = True
        case False:
            assert False
        case _:
            assert False
    assert b

    b = False
    match F:
        case True:
            assert False
        case False:
            b = True
        case _:
            assert False
    assert b
test_bool_match()

@test
def test_str_match():
    s = 'abc'
    t = 'xyz'
    e = ''

    b = False
    match s:
        case '':
            assert False
        case 'abc':
            b = True
        case _:
            assert False
    assert b

    b = False
    match e:
        case '':
            b = True
        case 'abc':
            assert False
        case _:
            assert False
    assert b

    b = False
    match t:
        case '':
            assert False
        case 'abc':
            assert False
        case _:
            b = True
    assert b

    b = False
    match t:
        case '':
            assert False
        case x if len(x) >= 3:
            b = True
        case _:
            assert False
    assert b
test_str_match()

@test
def test_tuple_match():
    t = (42, 99)
    r = (12, 12)

    b = False
    match t:
        case (0, 0):
            assert False
        case (42, 99):
            b = True
        case _:
            assert False
    assert b

    b = False
    match t:
        case (0, 0):
            assert False
        case (42, 0):
            assert False
        case _:
            b = True
    assert b

    b = False
    match t:
        case (0, 0):
            assert False
        case (0, 99):
            assert False
        case _:
            b = True
    assert b

    b = False
    match t:
        case (0, 0):
            assert False
        case (a, _) if a == 42:
            b = True
        case _:
            assert False
    assert b

    b = False
    match t:
        case (0, 0):
            assert False
        case (a, bb) if 40 < a < bb < 100:
            b = True
        case _:
            assert False
    assert b

    b = False
    match t:
        case (0, 0):
            assert False
        case (a, bb) if a == bb:
            assert False
        case _:
            b = True
    assert b

    b = False
    match r:
        case (0, 0):
            assert False
        case (a, bb) if a == bb:
            b = True
        case _:
            assert False
    assert b

    b = False
    match t:
        case (0, 0):
            assert False
        case (41 ... 43, 98 ... 100):
            b = True
        case _:
            assert False
    assert b

    b = False
    match r:
        case (0, 0):
            assert False
        case (41 ... 43, 98 ... 100):
            assert False
        case _:
            b = True
    assert b

    b = False
    match t:
        case (0, 0):
            assert False
        case (41 ... 43, 99 | 10) | (11 ... 13, 11 ... 13) | (-1, -1):
            b = True
        case _:
            assert False
    assert b

    b = False
    match r:
        case (0, 0):
            assert False
        case (-1, -1) | (41 ... 43, 10 | 99) | (12 | 11, 9 | 11 ... 13):
            b = True
        case _:
            assert False
    assert b
test_tuple_match()

@test
def test_int_match():
    n = 42
    m = -99

    b = False
    match n:
        case 0:
            assert False
        case 1:
            assert False
        case _:
            b = True
    assert b

    b = False
    match n:
        case 0:
            assert False
        case 42:
            b = True
        case 99:
            assert False
        case _:
            assert False
    assert b

    b = False
    match n:
        case 0:
            assert False
        case 1:
            assert False
        case _:
            b = True
    assert b

    b = False
    match m:
        case 0:
            assert False
        case 42:
            assert False
        case -99:
            b = True
        case _:
            assert False
    assert b

    b = False
    match m:
        case 0 ... 10:
            assert False
        case 12 ... 42:
            assert False
        case 42:
            assert False
        case _:
            b = True
    assert b

    b = False
    match n:
        case 0 ... 10:
            assert False
        case 42 ... 100:
            b = True
        case 42:
            assert False
        case _:
            assert False
    assert b

    b = False
    match n:
        case t if t < 10:
            assert False
        case t if 41 < t < 43:
            b = True
        case _:
            assert False
    assert b
test_int_match()

@test
def test_list_match():
    v = [1, 2, 3, 4, 5]
    e = list[int]()

    b = False
    match v:
        case []:
            assert False
        case [1, 2, 3, 4]:
            assert False
        case [1, 2, 3, 4, 5]:
            b = True
        case [1, 2, 3, 4, 5, 6]:
            assert False
        case _:
            assert False
    assert b

    b = False
    match e:
        case []:
            b = True
        case [1, 2, 3, 4]:
            assert False
        case [1, 2, 3, 4, 5]:
            assert False
        case [1, 2, 3, 4, 5, 6]:
            assert False
        case _:
            assert False
    assert b

    b = False
    match e:
        case [...]:
            b = True
        case [1, 2, 3, 4]:
            assert False
        case [1, 2, 3, 4, 5]:
            assert False
        case [1, 2, 3, 4, 5, 6]:
            assert False
        case _:
            assert False
    assert b

    b = False
    match e:
        case [_]:
            assert False
        case [1, 2, 3, 4]:
            assert False
        case [1, 2, 3, 4, 5]:
            assert False
        case [1, 2, 3, 4, 5, 6]:
            assert False
        case _:
            b = True
    assert b

    b = False
    match v:
        case []:
            assert False
        case [1, ..., 4]:
            assert False
        case [1, ..., 5]:
            b = True
        case [1, ..., 6]:
            assert False
        case _:
            assert False
    assert b

    b = False
    match v:
        case []:
            assert False
        case [_, ..., 4]:
            assert False
        case [_, ..., 5]:
            b = True
        case [_, ..., 6]:
            assert False
        case _:
            assert False
    assert b

    b = False
    match [5]:
        case []:
            assert False
        case [_, ..., 4]:
            assert False
        case [_, ..., 5]:
            assert False
        case [_, ..., 6]:
            assert False
        case _:
            b = True
    assert b

    b = False
    match v:
        case []:
            assert False
        case [..., 4]:
            assert False
        case [..., 5]:
            b = True
        case [..., 6]:
            assert False
        case _:
            assert False
    assert b

    b = False
    match v:
        case []:
            assert False
        case [1, ...]:
            b = True
        case [2, ...]:
            assert False
        case [3, ...]:
            assert False
        case _:
            assert False
    assert b

    b = False
    match v:
        case []:
            assert False
        case [2, ..., a, bb] if (a,bb) == (4,5):
            assert False
        case [1, ..., a, bb] if (a,bb) == (4,5):
            b = True
        case [3, ..., a, bb] if (a,bb) == (4,5):
            assert False
        case _:
            assert False
    assert b

    b = False
    match v:
        case []:
            assert False
        case [2, ..., a, _, bb] if (a,bb) == (3,5):
            assert False
        case [1, ..., a, _, bb] if (a,bb) == (3,5):
            b = True
        case [3, ..., a, _, bb] if (a,bb) == (3,5):
            assert False
        case _:
            assert False
    assert b

    b = False
    match v:
        case []:
            assert False
        case [1, _, 3, _, 5]:
            b = True
        case [_, _]:
            assert False
        case [_]:
            assert False
        case _:
            assert False
    assert b

    b = False
    match [[v]]:
        case [[[]]]:
            assert False
        case [[[..., 4]]]:
            assert False
        case [[[..., 5]]]:
            b = True
        case [[[..., 6]]]:
            assert False
        case _:
            assert False
    assert b

    b = False
    match [[v]]:
        case [[[]]]:
            assert False
        case [[[1, _, 3, _, 5]]]:
            b = True
        case [[[_, _]]]:
            assert False
        case [[[_]]]:
            assert False
        case _:
            assert False
    assert b
test_list_match()
