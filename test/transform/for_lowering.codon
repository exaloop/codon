@extend
class range:
    def __iter__(self):
        yield 999

@test
def test_for_lowering():
    from random import randint
    x = 10
    v = []
    for i in range(x):
        v.append(i)
    assert v == [0,1,2,3,4,5,6,7,8,9]

    v = []
    for i in range(x - 11, x//10):
        v.append(i)
    assert v == [-1, 0]

    v = []
    for i in range(x, -x):
        v.append(i)
    assert v == []

    v = []
    for i in range(x//3, -x//3, -2):
        v.append(i)
    assert v == [3, 1, -1]

    v = []
    for i in range(-1, 7, 3):
        v.append(i)
    assert v == [-1, 2, 5]

    v = []
    for i in range(0, 1, randint(1,1)):  # no lowering for non-const step
        v.append(i)
    assert v == [999]

    v = []
    try:
        for i in range(0, 1, 0):  # no lowering for zero step
            v.append(i)
        v.append(-1)
    except:
        v.append(-2)
    assert v == [-2]

    v = []
    for i in range(5):
        if i == 1:
            continue
        if i == 3:
            break
        v.append(i)
    assert v == [0, 2]

test_for_lowering()
