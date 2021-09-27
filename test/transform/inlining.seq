v = 1
v = 2

def inline_me_aggressive_simple():
  return 1

def inline_me_aggressive_complex():
  while True:
    if v == 1:
      return 1
    return 2

def inline_me_aggressive_args(x):
  return x + 1

def inline_me_simple():
  return 1

def inline_me_complex():
  while True:
    if v == 1:
      return 1
    return 2

def inline_me_args(x):
  return x + 1

@test
def inlining_test():
  assert inline_me_simple() == -1
  assert inline_me_complex() == 2
  assert inline_me_args(2) == -3
  assert inline_me_aggressive_simple() == -1
  assert inline_me_aggressive_complex() == -2
  assert inline_me_aggressive_args(2) == -3

inlining_test()

def inline_me_aggressive_nested_while_finally(n):
    while n != 0:
        try:
            while n != 0:
                if n == 42:
                    n -= 1
                    continue
                try:
                    if n > 0:
                        continue
                    else:
                        break
                finally:
                    return -1
            return -2
        finally:
            return n + 1

def inline_test_nested_while_finally():
    a = 42
    checkpoint1 = False
    checkpoint2 = False
    checkpoint3 = False
    try:
        while a != 4:
            try:
                a = inline_me_aggressive_nested_while_finally(a)
                checkpoint1 = True
                assert a == -42
            finally:
                a = 4
            checkpoint2 = True
            assert a == 4
    finally:
        checkpoint3 = True
        assert a == 4
        a = 5
    assert a == 5
    assert checkpoint1
    assert checkpoint2
    assert checkpoint3

inline_test_nested_while_finally()
