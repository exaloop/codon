import numpy as np

from helpers import eq

@test
def test_div_mod():
    xf64 = np.array([-4., -3., -2., -1., 0., 1., 2., 3., 4.], np.float64)
    xf32 = np.array([-4., -3., -2., -1., 0., 1., 2., 3., 4.], np.float32)
    xf16 = np.array([-4., -3., -2., -1., 0., 1., 2., 3., 4.], np.float16)

    xi64 = np.array([-4, -3, -2, -1, 0, 1, 2, 3, 4], np.int64)
    xi32 = np.array([-4, -3, -2, -1, 0, 1, 2, 3, 4], np.int32)
    xi16 = np.array([-4, -3, -2, -1, 0, 1, 2, 3, 4], np.int16)
    xi8 = np.array([-4, -3, -2, -1, 0, 1, 2, 3, 4], np.int8)

    xu64 = np.array([0, 1, 2, 3, 4], np.uint64)
    xu32 = np.array([0, 1, 2, 3, 4], np.uint32)
    xu16 = np.array([0, 1, 2, 3, 4], np.uint16)
    xu8 = np.array([0, 1, 2, 3, 4], np.uint8)

    assert eq(xf64 % 1, [0., 0., 0., 0., 0., 0., 0., 0., 0.], np.float64)
    assert eq(np.fmod(xf64, 1), [-0., -0., -0., -0., 0., 0., 0., 0., 0.],
              np.float64)
    assert eq(np.remainder(xf64, 1), [0., 0., 0., 0., 0., 0., 0., 0., 0.],
              np.float64)
    assert eq(xf64 % -1, [-0., -0., -0., -0., -0., -0., -0., -0., -0.], np.float64)
    assert eq(np.fmod(xf64, -1), [-0., -0., -0., -0., 0., 0., 0., 0., 0.],
              np.float64)
    assert eq(np.remainder(xf64, -1),
              [-0., -0., -0., -0., -0., -0., -0., -0., -0.], np.float64)

    assert eq(xf64 % 2, [0., 1., 0., 1., 0., 1., 0., 1., 0.], np.float64)
    assert eq(np.fmod(xf64, 2), [-0., -1., -0., -1., 0., 1., 0., 1., 0.],
              np.float64)
    assert eq(np.remainder(xf64, 2), [0., 1., 0., 1., 0., 1., 0., 1., 0.],
              np.float64)
    assert eq(xf64 % -2, [-0., -1., -0., -1., -0., -1., -0., -1., -0.], np.float64)
    assert eq(np.fmod(xf64, -2), [-0., -1., -0., -1., 0., 1., 0., 1., 0.],
              np.float64)
    assert eq(np.remainder(xf64, -2),
              [-0., -1., -0., -1., -0., -1., -0., -1., -0.], np.float64)

    assert eq(xf64 % 3, [2., 0., 1., 2., 0., 1., 2., 0., 1.], np.float64)
    assert eq(np.fmod(xf64, 3), [-1., -0., -2., -1., 0., 1., 2., 0., 1.],
              np.float64)
    assert eq(np.remainder(xf64, 3), [2., 0., 1., 2., 0., 1., 2., 0., 1.],
              np.float64)
    assert eq(xf64 % -3, [-1., -0., -2., -1., -0., -2., -1., -0., -2.], np.float64)
    assert eq(np.fmod(xf64, -3), [-1., -0., -2., -1., 0., 1., 2., 0., 1.],
              np.float64)
    assert eq(np.remainder(xf64, -3),
              [-1., -0., -2., -1., -0., -2., -1., -0., -2.], np.float64)

    assert eq(xf32 % 1, [0., 0., 0., 0., 0., 0., 0., 0., 0.], np.float32)
    assert eq(np.fmod(xf32, 1), [-0., -0., -0., -0., 0., 0., 0., 0., 0.],
              np.float32)
    assert eq(np.remainder(xf32, 1), [0., 0., 0., 0., 0., 0., 0., 0., 0.],
              np.float32)
    assert eq(xf32 % -1, [-0., -0., -0., -0., -0., -0., -0., -0., -0.], np.float32)
    assert eq(np.fmod(xf32, -1), [-0., -0., -0., -0., 0., 0., 0., 0., 0.],
              np.float32)
    assert eq(np.remainder(xf32, -1),
              [-0., -0., -0., -0., -0., -0., -0., -0., -0.], np.float32)

    assert eq(xf32 % 2, [0., 1., 0., 1., 0., 1., 0., 1., 0.], np.float32)
    assert eq(np.fmod(xf32, 2), [-0., -1., -0., -1., 0., 1., 0., 1., 0.],
              np.float32)
    assert eq(np.remainder(xf32, 2), [0., 1., 0., 1., 0., 1., 0., 1., 0.],
              np.float32)
    assert eq(xf32 % -2, [-0., -1., -0., -1., -0., -1., -0., -1., -0.], np.float32)
    assert eq(np.fmod(xf32, -2), [-0., -1., -0., -1., 0., 1., 0., 1., 0.],
              np.float32)
    assert eq(np.remainder(xf32, -2),
              [-0., -1., -0., -1., -0., -1., -0., -1., -0.], np.float32)

    assert eq(xf32 % 3, [2., 0., 1., 2., 0., 1., 2., 0., 1.], np.float32)
    assert eq(np.fmod(xf32, 3), [-1., -0., -2., -1., 0., 1., 2., 0., 1.],
              np.float32)
    assert eq(np.remainder(xf32, 3), [2., 0., 1., 2., 0., 1., 2., 0., 1.],
              np.float32)
    assert eq(xf32 % -3, [-1., -0., -2., -1., -0., -2., -1., -0., -2.], np.float32)
    assert eq(np.fmod(xf32, -3), [-1., -0., -2., -1., 0., 1., 2., 0., 1.],
              np.float32)
    assert eq(np.remainder(xf32, -3),
              [-1., -0., -2., -1., -0., -2., -1., -0., -2.], np.float32)

    assert eq(xf16 % 1, [0., 0., 0., 0., 0., 0., 0., 0., 0.], np.float16)
    assert eq(np.fmod(xf16, 1), [-0., -0., -0., -0., 0., 0., 0., 0., 0.],
              np.float16)
    assert eq(np.remainder(xf16, 1), [0., 0., 0., 0., 0., 0., 0., 0., 0.],
              np.float16)
    assert eq(xf16 % -1, [-0., -0., -0., -0., -0., -0., -0., -0., -0.], np.float16)
    assert eq(np.fmod(xf16, -1), [-0., -0., -0., -0., 0., 0., 0., 0., 0.],
              np.float16)
    assert eq(np.remainder(xf16, -1),
              [-0., -0., -0., -0., -0., -0., -0., -0., -0.], np.float16)

    assert eq(xf16 % 2, [0., 1., 0., 1., 0., 1., 0., 1., 0.], np.float16)
    assert eq(np.fmod(xf16, 2), [-0., -1., -0., -1., 0., 1., 0., 1., 0.],
              np.float16)
    assert eq(np.remainder(xf16, 2), [0., 1., 0., 1., 0., 1., 0., 1., 0.],
              np.float16)
    assert eq(xf16 % -2, [-0., -1., -0., -1., -0., -1., -0., -1., -0.], np.float16)
    assert eq(np.fmod(xf16, -2), [-0., -1., -0., -1., 0., 1., 0., 1., 0.],
              np.float16)
    assert eq(np.remainder(xf16, -2),
              [-0., -1., -0., -1., -0., -1., -0., -1., -0.], np.float16)

    assert eq(xf16 % 3, [2., 0., 1., 2., 0., 1., 2., 0., 1.], np.float16)
    assert eq(np.fmod(xf16, 3), [-1., -0., -2., -1., 0., 1., 2., 0., 1.],
              np.float16)
    assert eq(np.remainder(xf16, 3), [2., 0., 1., 2., 0., 1., 2., 0., 1.],
              np.float16)
    assert eq(xf16 % -3, [-1., -0., -2., -1., -0., -2., -1., -0., -2.], np.float16)
    assert eq(np.fmod(xf16, -3), [-1., -0., -2., -1., 0., 1., 2., 0., 1.],
              np.float16)
    assert eq(np.remainder(xf16, -3),
              [-1., -0., -2., -1., -0., -2., -1., -0., -2.], np.float16)

    assert eq(xi64 % 1, [0, 0, 0, 0, 0, 0, 0, 0, 0], np.int64)
    assert eq(np.fmod(xi64, 1), [0, 0, 0, 0, 0, 0, 0, 0, 0], np.int64)
    assert eq(np.remainder(xi64, 1), [0, 0, 0, 0, 0, 0, 0, 0, 0], np.int64)
    assert eq(xi64 % -1, [0, 0, 0, 0, 0, 0, 0, 0, 0], np.int64)
    assert eq(np.fmod(xi64, -1), [0, 0, 0, 0, 0, 0, 0, 0, 0], np.int64)
    assert eq(np.remainder(xi64, -1), [0, 0, 0, 0, 0, 0, 0, 0, 0], np.int64)

    assert eq(xi64 % 2, [0, 1, 0, 1, 0, 1, 0, 1, 0], np.int64)
    assert eq(np.fmod(xi64, 2), [0, -1, 0, -1, 0, 1, 0, 1, 0], np.int64)
    assert eq(np.remainder(xi64, 2), [0, 1, 0, 1, 0, 1, 0, 1, 0], np.int64)
    assert eq(xi64 % -2, [0, -1, 0, -1, 0, -1, 0, -1, 0], np.int64)
    assert eq(np.fmod(xi64, -2), [0, -1, 0, -1, 0, 1, 0, 1, 0], np.int64)
    assert eq(np.remainder(xi64, -2), [0, -1, 0, -1, 0, -1, 0, -1, 0], np.int64)

    assert eq(xi64 % 3, [2, 0, 1, 2, 0, 1, 2, 0, 1], np.int64)
    assert eq(np.fmod(xi64, 3), [-1, 0, -2, -1, 0, 1, 2, 0, 1], np.int64)
    assert eq(np.remainder(xi64, 3), [2, 0, 1, 2, 0, 1, 2, 0, 1], np.int64)
    assert eq(xi64 % -3, [-1, 0, -2, -1, 0, -2, -1, 0, -2], np.int64)
    assert eq(np.fmod(xi64, -3), [-1, 0, -2, -1, 0, 1, 2, 0, 1], np.int64)
    assert eq(np.remainder(xi64, -3), [-1, 0, -2, -1, 0, -2, -1, 0, -2], np.int64)

    assert eq(xi32 % 1, [0, 0, 0, 0, 0, 0, 0, 0, 0], np.int32)
    assert eq(np.fmod(xi32, 1), [0, 0, 0, 0, 0, 0, 0, 0, 0], np.int32)
    assert eq(np.remainder(xi32, 1), [0, 0, 0, 0, 0, 0, 0, 0, 0], np.int32)
    assert eq(xi32 % -1, [0, 0, 0, 0, 0, 0, 0, 0, 0], np.int32)
    assert eq(np.fmod(xi32, -1), [0, 0, 0, 0, 0, 0, 0, 0, 0], np.int32)
    assert eq(np.remainder(xi32, -1), [0, 0, 0, 0, 0, 0, 0, 0, 0], np.int32)

    assert eq(xi32 % 2, [0, 1, 0, 1, 0, 1, 0, 1, 0], np.int32)
    assert eq(np.fmod(xi32, 2), [0, -1, 0, -1, 0, 1, 0, 1, 0], np.int32)
    assert eq(np.remainder(xi32, 2), [0, 1, 0, 1, 0, 1, 0, 1, 0], np.int32)
    assert eq(xi32 % -2, [0, -1, 0, -1, 0, -1, 0, -1, 0], np.int32)
    assert eq(np.fmod(xi32, -2), [0, -1, 0, -1, 0, 1, 0, 1, 0], np.int32)
    assert eq(np.remainder(xi32, -2), [0, -1, 0, -1, 0, -1, 0, -1, 0], np.int32)

    assert eq(xi32 % 3, [2, 0, 1, 2, 0, 1, 2, 0, 1], np.int32)
    assert eq(np.fmod(xi32, 3), [-1, 0, -2, -1, 0, 1, 2, 0, 1], np.int32)
    assert eq(np.remainder(xi32, 3), [2, 0, 1, 2, 0, 1, 2, 0, 1], np.int32)
    assert eq(xi32 % -3, [-1, 0, -2, -1, 0, -2, -1, 0, -2], np.int32)
    assert eq(np.fmod(xi32, -3), [-1, 0, -2, -1, 0, 1, 2, 0, 1], np.int32)
    assert eq(np.remainder(xi32, -3), [-1, 0, -2, -1, 0, -2, -1, 0, -2], np.int32)

    assert eq(xi16 % 1, [0, 0, 0, 0, 0, 0, 0, 0, 0], np.int16)
    assert eq(np.fmod(xi16, 1), [0, 0, 0, 0, 0, 0, 0, 0, 0], np.int16)
    assert eq(np.remainder(xi16, 1), [0, 0, 0, 0, 0, 0, 0, 0, 0], np.int16)
    assert eq(xi16 % -1, [0, 0, 0, 0, 0, 0, 0, 0, 0], np.int16)
    assert eq(np.fmod(xi16, -1), [0, 0, 0, 0, 0, 0, 0, 0, 0], np.int16)
    assert eq(np.remainder(xi16, -1), [0, 0, 0, 0, 0, 0, 0, 0, 0], np.int16)

    assert eq(xi16 % 2, [0, 1, 0, 1, 0, 1, 0, 1, 0], np.int16)
    assert eq(np.fmod(xi16, 2), [0, -1, 0, -1, 0, 1, 0, 1, 0], np.int16)
    assert eq(np.remainder(xi16, 2), [0, 1, 0, 1, 0, 1, 0, 1, 0], np.int16)
    assert eq(xi16 % -2, [0, -1, 0, -1, 0, -1, 0, -1, 0], np.int16)
    assert eq(np.fmod(xi16, -2), [0, -1, 0, -1, 0, 1, 0, 1, 0], np.int16)
    assert eq(np.remainder(xi16, -2), [0, -1, 0, -1, 0, -1, 0, -1, 0], np.int16)

    assert eq(xi16 % 3, [2, 0, 1, 2, 0, 1, 2, 0, 1], np.int16)
    assert eq(np.fmod(xi16, 3), [-1, 0, -2, -1, 0, 1, 2, 0, 1], np.int16)
    assert eq(np.remainder(xi16, 3), [2, 0, 1, 2, 0, 1, 2, 0, 1], np.int16)
    assert eq(xi16 % -3, [-1, 0, -2, -1, 0, -2, -1, 0, -2], np.int16)
    assert eq(np.fmod(xi16, -3), [-1, 0, -2, -1, 0, 1, 2, 0, 1], np.int16)
    assert eq(np.remainder(xi16, -3), [-1, 0, -2, -1, 0, -2, -1, 0, -2], np.int16)

    assert eq(xi8 % 1, [0, 0, 0, 0, 0, 0, 0, 0, 0], np.int8)
    assert eq(np.fmod(xi8, 1), [0, 0, 0, 0, 0, 0, 0, 0, 0], np.int8)
    assert eq(np.remainder(xi8, 1), [0, 0, 0, 0, 0, 0, 0, 0, 0], np.int8)
    assert eq(xi8 % -1, [0, 0, 0, 0, 0, 0, 0, 0, 0], np.int8)
    assert eq(np.fmod(xi8, -1), [0, 0, 0, 0, 0, 0, 0, 0, 0], np.int8)
    assert eq(np.remainder(xi8, -1), [0, 0, 0, 0, 0, 0, 0, 0, 0], np.int8)

    assert eq(xi8 % 2, [0, 1, 0, 1, 0, 1, 0, 1, 0], np.int8)
    assert eq(np.fmod(xi8, 2), [0, -1, 0, -1, 0, 1, 0, 1, 0], np.int8)
    assert eq(np.remainder(xi8, 2), [0, 1, 0, 1, 0, 1, 0, 1, 0], np.int8)
    assert eq(xi8 % -2, [0, -1, 0, -1, 0, -1, 0, -1, 0], np.int8)
    assert eq(np.fmod(xi8, -2), [0, -1, 0, -1, 0, 1, 0, 1, 0], np.int8)
    assert eq(np.remainder(xi8, -2), [0, -1, 0, -1, 0, -1, 0, -1, 0], np.int8)

    assert eq(xi8 % 3, [2, 0, 1, 2, 0, 1, 2, 0, 1], np.int8)
    assert eq(np.fmod(xi8, 3), [-1, 0, -2, -1, 0, 1, 2, 0, 1], np.int8)
    assert eq(np.remainder(xi8, 3), [2, 0, 1, 2, 0, 1, 2, 0, 1], np.int8)
    assert eq(xi8 % -3, [-1, 0, -2, -1, 0, -2, -1, 0, -2], np.int8)
    assert eq(np.fmod(xi8, -3), [-1, 0, -2, -1, 0, 1, 2, 0, 1], np.int8)
    assert eq(np.remainder(xi8, -3), [-1, 0, -2, -1, 0, -2, -1, 0, -2], np.int8)

    assert eq(xu64 % 1, [0, 0, 0, 0, 0], np.uint64)
    assert eq(np.fmod(xu64, 1), [0, 0, 0, 0, 0], np.uint64)
    assert eq(np.remainder(xu64, 1), [0, 0, 0, 0, 0], np.uint64)

    assert eq(xu64 % 2, [0, 1, 0, 1, 0], np.uint64)
    assert eq(np.fmod(xu64, 2), [0, 1, 0, 1, 0], np.uint64)
    assert eq(np.remainder(xu64, 2), [0, 1, 0, 1, 0], np.uint64)

    assert eq(xu64 % 3, [0, 1, 2, 0, 1], np.uint64)
    assert eq(np.fmod(xu64, 3), [0, 1, 2, 0, 1], np.uint64)
    assert eq(np.remainder(xu64, 3), [0, 1, 2, 0, 1], np.uint64)

    assert eq(xu32 % 1, [0, 0, 0, 0, 0], np.uint32)
    assert eq(np.fmod(xu32, 1), [0, 0, 0, 0, 0], np.uint32)
    assert eq(np.remainder(xu32, 1), [0, 0, 0, 0, 0], np.uint32)

    assert eq(xu32 % 2, [0, 1, 0, 1, 0], np.uint32)
    assert eq(np.fmod(xu32, 2), [0, 1, 0, 1, 0], np.uint32)
    assert eq(np.remainder(xu32, 2), [0, 1, 0, 1, 0], np.uint32)

    assert eq(xu32 % 3, [0, 1, 2, 0, 1], np.uint32)
    assert eq(np.fmod(xu32, 3), [0, 1, 2, 0, 1], np.uint32)
    assert eq(np.remainder(xu32, 3), [0, 1, 2, 0, 1], np.uint32)

    assert eq(xu16 % 1, [0, 0, 0, 0, 0], np.uint16)
    assert eq(np.fmod(xu16, 1), [0, 0, 0, 0, 0], np.uint16)
    assert eq(np.remainder(xu16, 1), [0, 0, 0, 0, 0], np.uint16)

    assert eq(xu16 % 2, [0, 1, 0, 1, 0], np.uint16)
    assert eq(np.fmod(xu16, 2), [0, 1, 0, 1, 0], np.uint16)
    assert eq(np.remainder(xu16, 2), [0, 1, 0, 1, 0], np.uint16)

    assert eq(xu16 % 3, [0, 1, 2, 0, 1], np.uint16)
    assert eq(np.fmod(xu16, 3), [0, 1, 2, 0, 1], np.uint16)
    assert eq(np.remainder(xu16, 3), [0, 1, 2, 0, 1], np.uint16)

    assert eq(xu8 % 1, [0, 0, 0, 0, 0], np.uint8)
    assert eq(np.fmod(xu8, 1), [0, 0, 0, 0, 0], np.uint8)
    assert eq(np.remainder(xu8, 1), [0, 0, 0, 0, 0], np.uint8)

    assert eq(xu8 % 2, [0, 1, 0, 1, 0], np.uint8)
    assert eq(np.fmod(xu8, 2), [0, 1, 0, 1, 0], np.uint8)
    assert eq(np.remainder(xu8, 2), [0, 1, 0, 1, 0], np.uint8)

    assert eq(xu8 % 3, [0, 1, 2, 0, 1], np.uint8)
    assert eq(np.fmod(xu8, 3), [0, 1, 2, 0, 1], np.uint8)
    assert eq(np.remainder(xu8, 3), [0, 1, 2, 0, 1], np.uint8)


    def eq1(got, exp, dtype: type):
        return eq(got, np.asarray(exp, dtype) + 1, dtype)

    assert eq1(1 + xf64 % 1, [0., 0., 0., 0., 0., 0., 0., 0., 0.], np.float64)
    assert eq1(1 + np.fmod(xf64, 1), [-0., -0., -0., -0., 0., 0., 0., 0., 0.],
              np.float64)
    assert eq1(1 + np.remainder(xf64, 1), [0., 0., 0., 0., 0., 0., 0., 0., 0.],
              np.float64)
    assert eq1(1 + xf64 % -1, [-0., -0., -0., -0., -0., -0., -0., -0., -0.], np.float64)
    assert eq1(1 + np.fmod(xf64, -1), [-0., -0., -0., -0., 0., 0., 0., 0., 0.],
              np.float64)
    assert eq1(1 + np.remainder(xf64, -1),
              [-0., -0., -0., -0., -0., -0., -0., -0., -0.], np.float64)

    assert eq1(1 + xf64 % 2, [0., 1., 0., 1., 0., 1., 0., 1., 0.], np.float64)
    assert eq1(1 + np.fmod(xf64, 2), [-0., -1., -0., -1., 0., 1., 0., 1., 0.],
              np.float64)
    assert eq1(1 + np.remainder(xf64, 2), [0., 1., 0., 1., 0., 1., 0., 1., 0.],
              np.float64)
    assert eq1(1 + xf64 % -2, [-0., -1., -0., -1., -0., -1., -0., -1., -0.], np.float64)
    assert eq1(1 + np.fmod(xf64, -2), [-0., -1., -0., -1., 0., 1., 0., 1., 0.],
              np.float64)
    assert eq1(1 + np.remainder(xf64, -2),
              [-0., -1., -0., -1., -0., -1., -0., -1., -0.], np.float64)

    assert eq1(1 + xf64 % 3, [2., 0., 1., 2., 0., 1., 2., 0., 1.], np.float64)
    assert eq1(1 + np.fmod(xf64, 3), [-1., -0., -2., -1., 0., 1., 2., 0., 1.],
              np.float64)
    assert eq1(1 + np.remainder(xf64, 3), [2., 0., 1., 2., 0., 1., 2., 0., 1.],
              np.float64)
    assert eq1(1 + xf64 % -3, [-1., -0., -2., -1., -0., -2., -1., -0., -2.], np.float64)
    assert eq1(1 + np.fmod(xf64, -3), [-1., -0., -2., -1., 0., 1., 2., 0., 1.],
              np.float64)
    assert eq1(1 + np.remainder(xf64, -3),
              [-1., -0., -2., -1., -0., -2., -1., -0., -2.], np.float64)

    assert eq1(1 + xf32 % 1, [0., 0., 0., 0., 0., 0., 0., 0., 0.], np.float32)
    assert eq1(1 + np.fmod(xf32, 1), [-0., -0., -0., -0., 0., 0., 0., 0., 0.],
              np.float32)
    assert eq1(1 + np.remainder(xf32, 1), [0., 0., 0., 0., 0., 0., 0., 0., 0.],
              np.float32)
    assert eq1(1 + xf32 % -1, [-0., -0., -0., -0., -0., -0., -0., -0., -0.], np.float32)
    assert eq1(1 + np.fmod(xf32, -1), [-0., -0., -0., -0., 0., 0., 0., 0., 0.],
              np.float32)
    assert eq1(1 + np.remainder(xf32, -1),
              [-0., -0., -0., -0., -0., -0., -0., -0., -0.], np.float32)

    assert eq1(1 + xf32 % 2, [0., 1., 0., 1., 0., 1., 0., 1., 0.], np.float32)
    assert eq1(1 + np.fmod(xf32, 2), [-0., -1., -0., -1., 0., 1., 0., 1., 0.],
              np.float32)
    assert eq1(1 + np.remainder(xf32, 2), [0., 1., 0., 1., 0., 1., 0., 1., 0.],
              np.float32)
    assert eq1(1 + xf32 % -2, [-0., -1., -0., -1., -0., -1., -0., -1., -0.], np.float32)
    assert eq1(1 + np.fmod(xf32, -2), [-0., -1., -0., -1., 0., 1., 0., 1., 0.],
              np.float32)
    assert eq1(1 + np.remainder(xf32, -2),
              [-0., -1., -0., -1., -0., -1., -0., -1., -0.], np.float32)

    assert eq1(1 + xf32 % 3, [2., 0., 1., 2., 0., 1., 2., 0., 1.], np.float32)
    assert eq1(1 + np.fmod(xf32, 3), [-1., -0., -2., -1., 0., 1., 2., 0., 1.],
              np.float32)
    assert eq1(1 + np.remainder(xf32, 3), [2., 0., 1., 2., 0., 1., 2., 0., 1.],
              np.float32)
    assert eq1(1 + xf32 % -3, [-1., -0., -2., -1., -0., -2., -1., -0., -2.], np.float32)
    assert eq1(1 + np.fmod(xf32, -3), [-1., -0., -2., -1., 0., 1., 2., 0., 1.],
              np.float32)
    assert eq1(1 + np.remainder(xf32, -3),
              [-1., -0., -2., -1., -0., -2., -1., -0., -2.], np.float32)

    assert eq1(1 + xf16 % 1, [0., 0., 0., 0., 0., 0., 0., 0., 0.], np.float16)
    assert eq1(1 + np.fmod(xf16, 1), [-0., -0., -0., -0., 0., 0., 0., 0., 0.],
              np.float16)
    assert eq1(1 + np.remainder(xf16, 1), [0., 0., 0., 0., 0., 0., 0., 0., 0.],
              np.float16)
    assert eq1(1 + xf16 % -1, [-0., -0., -0., -0., -0., -0., -0., -0., -0.], np.float16)
    assert eq1(1 + np.fmod(xf16, -1), [-0., -0., -0., -0., 0., 0., 0., 0., 0.],
              np.float16)
    assert eq1(1 + np.remainder(xf16, -1),
              [-0., -0., -0., -0., -0., -0., -0., -0., -0.], np.float16)

    assert eq1(1 + xf16 % 2, [0., 1., 0., 1., 0., 1., 0., 1., 0.], np.float16)
    assert eq1(1 + np.fmod(xf16, 2), [-0., -1., -0., -1., 0., 1., 0., 1., 0.],
              np.float16)
    assert eq1(1 + np.remainder(xf16, 2), [0., 1., 0., 1., 0., 1., 0., 1., 0.],
              np.float16)
    assert eq1(1 + xf16 % -2, [-0., -1., -0., -1., -0., -1., -0., -1., -0.], np.float16)
    assert eq1(1 + np.fmod(xf16, -2), [-0., -1., -0., -1., 0., 1., 0., 1., 0.],
              np.float16)
    assert eq1(1 + np.remainder(xf16, -2),
              [-0., -1., -0., -1., -0., -1., -0., -1., -0.], np.float16)

    assert eq1(1 + xf16 % 3, [2., 0., 1., 2., 0., 1., 2., 0., 1.], np.float16)
    assert eq1(1 + np.fmod(xf16, 3), [-1., -0., -2., -1., 0., 1., 2., 0., 1.],
              np.float16)
    assert eq1(1 + np.remainder(xf16, 3), [2., 0., 1., 2., 0., 1., 2., 0., 1.],
              np.float16)
    assert eq1(1 + xf16 % -3, [-1., -0., -2., -1., -0., -2., -1., -0., -2.], np.float16)
    assert eq1(1 + np.fmod(xf16, -3), [-1., -0., -2., -1., 0., 1., 2., 0., 1.],
              np.float16)
    assert eq1(1 + np.remainder(xf16, -3),
              [-1., -0., -2., -1., -0., -2., -1., -0., -2.], np.float16)

    assert eq1(1 + xi64 % 1, [0, 0, 0, 0, 0, 0, 0, 0, 0], np.int64)
    assert eq1(1 + np.fmod(xi64, 1), [0, 0, 0, 0, 0, 0, 0, 0, 0], np.int64)
    assert eq1(1 + np.remainder(xi64, 1), [0, 0, 0, 0, 0, 0, 0, 0, 0], np.int64)
    assert eq1(1 + xi64 % -1, [0, 0, 0, 0, 0, 0, 0, 0, 0], np.int64)
    assert eq1(1 + np.fmod(xi64, -1), [0, 0, 0, 0, 0, 0, 0, 0, 0], np.int64)
    assert eq1(1 + np.remainder(xi64, -1), [0, 0, 0, 0, 0, 0, 0, 0, 0], np.int64)

    assert eq1(1 + xi64 % 2, [0, 1, 0, 1, 0, 1, 0, 1, 0], np.int64)
    assert eq1(1 + np.fmod(xi64, 2), [0, -1, 0, -1, 0, 1, 0, 1, 0], np.int64)
    assert eq1(1 + np.remainder(xi64, 2), [0, 1, 0, 1, 0, 1, 0, 1, 0], np.int64)
    assert eq1(1 + xi64 % -2, [0, -1, 0, -1, 0, -1, 0, -1, 0], np.int64)
    assert eq1(1 + np.fmod(xi64, -2), [0, -1, 0, -1, 0, 1, 0, 1, 0], np.int64)
    assert eq1(1 + np.remainder(xi64, -2), [0, -1, 0, -1, 0, -1, 0, -1, 0], np.int64)

    assert eq1(1 + xi64 % 3, [2, 0, 1, 2, 0, 1, 2, 0, 1], np.int64)
    assert eq1(1 + np.fmod(xi64, 3), [-1, 0, -2, -1, 0, 1, 2, 0, 1], np.int64)
    assert eq1(1 + np.remainder(xi64, 3), [2, 0, 1, 2, 0, 1, 2, 0, 1], np.int64)
    assert eq1(1 + xi64 % -3, [-1, 0, -2, -1, 0, -2, -1, 0, -2], np.int64)
    assert eq1(1 + np.fmod(xi64, -3), [-1, 0, -2, -1, 0, 1, 2, 0, 1], np.int64)
    assert eq1(1 + np.remainder(xi64, -3), [-1, 0, -2, -1, 0, -2, -1, 0, -2], np.int64)

    assert eq1(1 + xi32 % 1, [0, 0, 0, 0, 0, 0, 0, 0, 0], np.int32)
    assert eq1(1 + np.fmod(xi32, 1), [0, 0, 0, 0, 0, 0, 0, 0, 0], np.int32)
    assert eq1(1 + np.remainder(xi32, 1), [0, 0, 0, 0, 0, 0, 0, 0, 0], np.int32)
    assert eq1(1 + xi32 % -1, [0, 0, 0, 0, 0, 0, 0, 0, 0], np.int32)
    assert eq1(1 + np.fmod(xi32, -1), [0, 0, 0, 0, 0, 0, 0, 0, 0], np.int32)
    assert eq1(1 + np.remainder(xi32, -1), [0, 0, 0, 0, 0, 0, 0, 0, 0], np.int32)

    assert eq1(1 + xi32 % 2, [0, 1, 0, 1, 0, 1, 0, 1, 0], np.int32)
    assert eq1(1 + np.fmod(xi32, 2), [0, -1, 0, -1, 0, 1, 0, 1, 0], np.int32)
    assert eq1(1 + np.remainder(xi32, 2), [0, 1, 0, 1, 0, 1, 0, 1, 0], np.int32)
    assert eq1(1 + xi32 % -2, [0, -1, 0, -1, 0, -1, 0, -1, 0], np.int32)
    assert eq1(1 + np.fmod(xi32, -2), [0, -1, 0, -1, 0, 1, 0, 1, 0], np.int32)
    assert eq1(1 + np.remainder(xi32, -2), [0, -1, 0, -1, 0, -1, 0, -1, 0], np.int32)

    assert eq1(1 + xi32 % 3, [2, 0, 1, 2, 0, 1, 2, 0, 1], np.int32)
    assert eq1(1 + np.fmod(xi32, 3), [-1, 0, -2, -1, 0, 1, 2, 0, 1], np.int32)
    assert eq1(1 + np.remainder(xi32, 3), [2, 0, 1, 2, 0, 1, 2, 0, 1], np.int32)
    assert eq1(1 + xi32 % -3, [-1, 0, -2, -1, 0, -2, -1, 0, -2], np.int32)
    assert eq1(1 + np.fmod(xi32, -3), [-1, 0, -2, -1, 0, 1, 2, 0, 1], np.int32)
    assert eq1(1 + np.remainder(xi32, -3), [-1, 0, -2, -1, 0, -2, -1, 0, -2], np.int32)

    assert eq1(1 + xi16 % 1, [0, 0, 0, 0, 0, 0, 0, 0, 0], np.int16)
    assert eq1(1 + np.fmod(xi16, 1), [0, 0, 0, 0, 0, 0, 0, 0, 0], np.int16)
    assert eq1(1 + np.remainder(xi16, 1), [0, 0, 0, 0, 0, 0, 0, 0, 0], np.int16)
    assert eq1(1 + xi16 % -1, [0, 0, 0, 0, 0, 0, 0, 0, 0], np.int16)
    assert eq1(1 + np.fmod(xi16, -1), [0, 0, 0, 0, 0, 0, 0, 0, 0], np.int16)
    assert eq1(1 + np.remainder(xi16, -1), [0, 0, 0, 0, 0, 0, 0, 0, 0], np.int16)

    assert eq1(1 + xi16 % 2, [0, 1, 0, 1, 0, 1, 0, 1, 0], np.int16)
    assert eq1(1 + np.fmod(xi16, 2), [0, -1, 0, -1, 0, 1, 0, 1, 0], np.int16)
    assert eq1(1 + np.remainder(xi16, 2), [0, 1, 0, 1, 0, 1, 0, 1, 0], np.int16)
    assert eq1(1 + xi16 % -2, [0, -1, 0, -1, 0, -1, 0, -1, 0], np.int16)
    assert eq1(1 + np.fmod(xi16, -2), [0, -1, 0, -1, 0, 1, 0, 1, 0], np.int16)
    assert eq1(1 + np.remainder(xi16, -2), [0, -1, 0, -1, 0, -1, 0, -1, 0], np.int16)

    assert eq1(1 + xi16 % 3, [2, 0, 1, 2, 0, 1, 2, 0, 1], np.int16)
    assert eq1(1 + np.fmod(xi16, 3), [-1, 0, -2, -1, 0, 1, 2, 0, 1], np.int16)
    assert eq1(1 + np.remainder(xi16, 3), [2, 0, 1, 2, 0, 1, 2, 0, 1], np.int16)
    assert eq1(1 + xi16 % -3, [-1, 0, -2, -1, 0, -2, -1, 0, -2], np.int16)
    assert eq1(1 + np.fmod(xi16, -3), [-1, 0, -2, -1, 0, 1, 2, 0, 1], np.int16)
    assert eq1(1 + np.remainder(xi16, -3), [-1, 0, -2, -1, 0, -2, -1, 0, -2], np.int16)

    assert eq1(1 + xi8 % 1, [0, 0, 0, 0, 0, 0, 0, 0, 0], np.int8)
    assert eq1(1 + np.fmod(xi8, 1), [0, 0, 0, 0, 0, 0, 0, 0, 0], np.int8)
    assert eq1(1 + np.remainder(xi8, 1), [0, 0, 0, 0, 0, 0, 0, 0, 0], np.int8)
    assert eq1(1 + xi8 % -1, [0, 0, 0, 0, 0, 0, 0, 0, 0], np.int8)
    assert eq1(1 + np.fmod(xi8, -1), [0, 0, 0, 0, 0, 0, 0, 0, 0], np.int8)
    assert eq1(1 + np.remainder(xi8, -1), [0, 0, 0, 0, 0, 0, 0, 0, 0], np.int8)

    assert eq1(1 + xi8 % 2, [0, 1, 0, 1, 0, 1, 0, 1, 0], np.int8)
    assert eq1(1 + np.fmod(xi8, 2), [0, -1, 0, -1, 0, 1, 0, 1, 0], np.int8)
    assert eq1(1 + np.remainder(xi8, 2), [0, 1, 0, 1, 0, 1, 0, 1, 0], np.int8)
    assert eq1(1 + xi8 % -2, [0, -1, 0, -1, 0, -1, 0, -1, 0], np.int8)
    assert eq1(1 + np.fmod(xi8, -2), [0, -1, 0, -1, 0, 1, 0, 1, 0], np.int8)
    assert eq1(1 + np.remainder(xi8, -2), [0, -1, 0, -1, 0, -1, 0, -1, 0], np.int8)

    assert eq1(1 + xi8 % 3, [2, 0, 1, 2, 0, 1, 2, 0, 1], np.int8)
    assert eq1(1 + np.fmod(xi8, 3), [-1, 0, -2, -1, 0, 1, 2, 0, 1], np.int8)
    assert eq1(1 + np.remainder(xi8, 3), [2, 0, 1, 2, 0, 1, 2, 0, 1], np.int8)
    assert eq1(1 + xi8 % -3, [-1, 0, -2, -1, 0, -2, -1, 0, -2], np.int8)
    assert eq1(1 + np.fmod(xi8, -3), [-1, 0, -2, -1, 0, 1, 2, 0, 1], np.int8)
    assert eq1(1 + np.remainder(xi8, -3), [-1, 0, -2, -1, 0, -2, -1, 0, -2], np.int8)

    assert eq1(1 + xu64 % 1, [0, 0, 0, 0, 0], np.uint64)
    assert eq1(1 + np.fmod(xu64, 1), [0, 0, 0, 0, 0], np.uint64)
    assert eq1(1 + np.remainder(xu64, 1), [0, 0, 0, 0, 0], np.uint64)

    assert eq1(1 + xu64 % 2, [0, 1, 0, 1, 0], np.uint64)
    assert eq1(1 + np.fmod(xu64, 2), [0, 1, 0, 1, 0], np.uint64)
    assert eq1(1 + np.remainder(xu64, 2), [0, 1, 0, 1, 0], np.uint64)

    assert eq1(1 + xu64 % 3, [0, 1, 2, 0, 1], np.uint64)
    assert eq1(1 + np.fmod(xu64, 3), [0, 1, 2, 0, 1], np.uint64)
    assert eq1(1 + np.remainder(xu64, 3), [0, 1, 2, 0, 1], np.uint64)

    assert eq1(1 + xu32 % 1, [0, 0, 0, 0, 0], np.uint32)
    assert eq1(1 + np.fmod(xu32, 1), [0, 0, 0, 0, 0], np.uint32)
    assert eq1(1 + np.remainder(xu32, 1), [0, 0, 0, 0, 0], np.uint32)

    assert eq1(1 + xu32 % 2, [0, 1, 0, 1, 0], np.uint32)
    assert eq1(1 + np.fmod(xu32, 2), [0, 1, 0, 1, 0], np.uint32)
    assert eq1(1 + np.remainder(xu32, 2), [0, 1, 0, 1, 0], np.uint32)

    assert eq1(1 + xu32 % 3, [0, 1, 2, 0, 1], np.uint32)
    assert eq1(1 + np.fmod(xu32, 3), [0, 1, 2, 0, 1], np.uint32)
    assert eq1(1 + np.remainder(xu32, 3), [0, 1, 2, 0, 1], np.uint32)

    assert eq1(1 + xu16 % 1, [0, 0, 0, 0, 0], np.uint16)
    assert eq1(1 + np.fmod(xu16, 1), [0, 0, 0, 0, 0], np.uint16)
    assert eq1(1 + np.remainder(xu16, 1), [0, 0, 0, 0, 0], np.uint16)

    assert eq1(1 + xu16 % 2, [0, 1, 0, 1, 0], np.uint16)
    assert eq1(1 + np.fmod(xu16, 2), [0, 1, 0, 1, 0], np.uint16)
    assert eq1(1 + np.remainder(xu16, 2), [0, 1, 0, 1, 0], np.uint16)

    assert eq1(1 + xu16 % 3, [0, 1, 2, 0, 1], np.uint16)
    assert eq1(1 + np.fmod(xu16, 3), [0, 1, 2, 0, 1], np.uint16)
    assert eq1(1 + np.remainder(xu16, 3), [0, 1, 2, 0, 1], np.uint16)

    assert eq1(1 + xu8 % 1, [0, 0, 0, 0, 0], np.uint8)
    assert eq1(1 + np.fmod(xu8, 1), [0, 0, 0, 0, 0], np.uint8)
    assert eq1(1 + np.remainder(xu8, 1), [0, 0, 0, 0, 0], np.uint8)

    assert eq1(1 + xu8 % 2, [0, 1, 0, 1, 0], np.uint8)
    assert eq1(1 + np.fmod(xu8, 2), [0, 1, 0, 1, 0], np.uint8)
    assert eq1(1 + np.remainder(xu8, 2), [0, 1, 0, 1, 0], np.uint8)

    assert eq1(1 + xu8 % 3, [0, 1, 2, 0, 1], np.uint8)
    assert eq1(1 + np.fmod(xu8, 3), [0, 1, 2, 0, 1], np.uint8)
    assert eq1(1 + np.remainder(xu8, 3), [0, 1, 2, 0, 1], np.uint8)

test_div_mod()
