add_count = 0

@extend
class List:
    def __add__(self, other: List[T]) -> List[T]:
        global add_count
        add_count += 1
        n = self.len + other.len
        v = List[T](n)
        v.len = n
        p = v.arr.ptr
        str.memcpy(p.as_byte(),
                   self.arr.ptr.as_byte(),
                   self.len * gc.sizeof(T))
        str.memcpy((p + self.len).as_byte(),
                   other.arr.ptr.as_byte(),
                   other.len * gc.sizeof(T))
        return v

@test
def test_list_optimization():
    add_count0 = add_count
    A = list(range(3))
    B = list(range(10))
    assert [0] + [1] == [0, 1]
    assert A + B == [0, 1, 2, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9]
    assert (A + B[:] + B[7:] + B[:3] + B[3:7] + B[7:3:-1] + A[::-1] + [11, 22, 33] ==
            [0, 1, 2, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 7, 8, 9, 0, 1, 2, 3, 4, 5, 6, 7, 6, 5, 4, 2, 1, 0, 11, 22, 33])

    def f(a, tag, order):
        order.append(tag)
        return a

    order = []
    X = (f([1, 2], 'a', order) +
         [f(3, 'b', order), f(4, 'c', order)] +
         f(list(range(10)), 'd', order)[f(5, 'e', order):f(2, 'f', order):f(-1, 'g', order)])
    assert X == [1, 2, 3, 4, 5, 4, 3]
    assert order == ['a', 'b', 'c', 'd', 'e', 'f', 'g']
    assert add_count == add_count0

test_list_optimization()
