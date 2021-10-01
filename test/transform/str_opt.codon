cat_count = 0

# c/p from str.seq
def old_cat(*args):
    total = 0
    if staticlen(args) == 1 and hasattr(args[0], "__iter__") and hasattr(args[0], "__len__"):
        for s in args[0]:
            if not isinstance(s, str):
                compile_error("not a string")
            total += s.len
        p = cobj(total)
        n = 0
        for s in args[0]:
            str.memcpy(p + n, s.ptr, s.len)
            n += s.len
        return str(p, total)
    elif staticlen(args) == 1 and hasattr(args[0], "__iter__"):
        sz = 10
        p = cobj(sz)
        n = 0
        for s in args[0]:
            if not isinstance(s, str):
                compile_error("not a string")
            if n + s.len > sz:
                sz = 1 + 3 * (n + s.len) // 2
                pp = cobj(sz)
                str.memcpy(pp, p, n)
                p = pp
            str.memcpy(p + n, s.ptr, s.len)
            n += s.len
        return str(p, n)
    else:
        total = 0
        for i in args:
            if not isinstance(i, str):
                compile_error("not a string")
            total += i.len
        p = cobj(total)
        n = 0
        for i in args:
            str.memcpy(p + n, i.ptr, i.len)
            n += i.len
        return str(p, total)

@extend
class str:
    def cat(*args):
        global cat_count
        cat_count += 1
        return old_cat(*args)

@test
def test_str_optimization():
    assert 'hello ' + 'world' == "hello world"  # no opt: just adding 2 strs
    assert cat_count == 0
    # assert 'a' + 'b' + 'c' == 'abc' # superseded by string statics
    # assert cat_count == 1
    assert 'a' * 2 == 'aa'  # no opt: mul instead of add
    assert cat_count == 0
    # assert 'a' + ('b' + 'c') == 'abc'
    # assert cat_count == 2
    # assert 'a' + ('b' + ('c' + 'd')) == 'abcd'
    # assert cat_count == 3

    a = 'a'
    b = 'b'
    c = 'c'
    assert (a*2 + b*3 + c*4) == 'aabbbcccc'
    assert cat_count == 1
test_str_optimization()
