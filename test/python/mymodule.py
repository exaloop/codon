def multiply(a,b):
    return a*b

def print_args(a,b,c,d,e):
    t = (a,b,c,d,e)
    if t != ((4, 5), {'a': 3.14, 'b': 2.123}, True, {'ACGT'}, [['abc'], ['1.1', '2.2'], []]):
        raise ValueError('TEST FAILED!')
    return ({'a': 3.14, 'b': 2.123}, (222, 3.14))

def print_args_var(a,b,c=1,*args,**kwargs):
    return 'a={}, b={}, c={}, args={}, kwargs={}'.format(a, b, c, args, kwargs)

def throw_exc():
    raise ValueError('foo')
    return 0
