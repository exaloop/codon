import numpy as np

from helpers import eq

@test
def test_mixed_types():
    i8 = np.array([1, 2, 3, 4], np.int8)
    i16 = np.array([1, 2, 3, 4], np.int16)
    i32 = np.array([1, 2, 3, 4], np.int32)
    i64 = np.array([1, 2, 3, 4], np.int64)
    u8 = np.array([1, 2, 3, 4], np.uint8)
    u16 = np.array([1, 2, 3, 4], np.uint16)
    u32 = np.array([1, 2, 3, 4], np.uint32)
    u64 = np.array([1, 2, 3, 4], np.uint64)
    f16 = np.array([1., 2., 3., 4.], np.float16)
    f32 = np.array([1., 2., 3., 4.], np.float32)
    f64 = np.array([1., 2., 3., 4.], np.float64)
    c64 = np.array([1., 2., 3., 4.], np.complex64)
    c128 = np.array([1., 2., 3., 4.], np.complex128)

    assert eq(i8 + i16 + i32, [3, 6, 9, 12], np.int32)
    assert eq(i16 + i32 + i64, [3, 6, 9, 12], np.int64)
    assert eq(u8 + u16 + u32, [3, 6, 9, 12], np.uint32)
    assert eq(i8 + u16 + i32, [3, 6, 9, 12], np.int32)
    assert eq(i16 + u32 + i64, [3, 6, 9, 12], np.int64)
    assert eq(u8 + i32 + u64, [3, 6, 9, 12], np.float64)
    assert eq(i8 + u16 + u64, [3, 6, 9, 12], np.float64)
    assert eq(i16 + i64 + u64, [3, 6, 9, 12], np.float64)
    assert eq(i32 + u32 + i64, [3, 6, 9, 12], np.int64)
    assert eq(u16 + i64 + u32, [3, 6, 9, 12], np.int64)

    assert eq(f16 + f32 + f64, [3., 6., 9., 12.], np.float64)
    assert eq(f16 + f32 + f16, [3., 6., 9., 12.], np.float32)
    assert eq(f32 + f64 + f32, [3., 6., 9., 12.], np.float64)
    assert eq(f64 + f16 + f32, [3., 6., 9., 12.], np.float64)
    assert eq(f16 + f16 + f64, [3., 6., 9., 12.], np.float64)
    assert eq(f32 + f32 + f16, [3., 6., 9., 12.], np.float32)
    assert eq(f64 + f64 + f32, [3., 6., 9., 12.], np.float64)
    assert eq(f16 + f64 + f64, [3., 6., 9., 12.], np.float64)
    assert eq(f32 + f16 + f32, [3., 6., 9., 12.], np.float32)
    assert eq(f64 + f32 + f64, [3., 6., 9., 12.], np.float64)

    assert eq(i8 + i16 + f32, [3., 6., 9., 12.], np.float32)
    assert eq(i32 + f32 + c64, [3.+0.j, 6.+0.j, 9.+0.j, 12.+0.j], np.complex128)
    assert eq(f16 + u8 + u16, [3., 6., 9., 12.], np.float32)
    assert eq(i64 + f64 + c128, [3.+0.j, 6.+0.j, 9+0.j, 12.+0.j], np.complex128)
    assert eq(u16 + c64 + f32, [3.+0.j, 6.+0.j, 9.+0.j, 12.+0.j], np.complex64)
    assert eq(f32 + c128 + i32, [3.+0.j, 6.+0.j, 9.+0.j, 12.+0.j], np.complex128)
    assert eq(u32 + i16 + f64, [3., 6., 9., 12.], np.float64)
    assert eq(i8 + u64 + f64, [3., 6., 9., 12.], np.float64)
    assert eq(f64 + c64 + u16, [3.+0.j, 6.+0.j, 9.+0.j, 12.+0.j], np.complex128)
    assert eq(c128 + i32 + u32, [3.+0.j, 6.+0.j, 9.+0.j, 12.+0.j], np.complex128)
    assert eq(i8 + f16 + c64, [3.+0.j, 6.+0.j, 9.+0.j, 12.+0.j], np.complex64)
    assert eq(u8 + i64 + c128, [3.+0.j, 6.+0.j, 9.+0.j, 12.+0.j], np.complex128)
    assert eq(i16 + f32 + c64, [3.+0.j, 6.+0.j, 9.+0.j, 12.+0.j], np.complex64)

    assert eq((i8 == i16) + c64, [2.+0j, 3.+0j, 4.+0j, 5.+0j], np.complex64)
    assert eq(c64 + (i8 == i16), [2.+0j, 3.+0j, 4.+0j, 5.+0j], np.complex64)

    assert eq((i8 + u8) + (i16 + u16) + (u32 + u32) + (i64 + u64) + f16 + f32 + f64 + c64 + c128,
              [13.+0.j, 26.+0.j, 39.+0.j, 52.+0.j], np.complex128)

    # Mixing scalars and arrays
    assert eq((i8 + 100) << 1, [-54, -52, -50, -48], np.int8)
    assert eq((i8 + np.array(100)) << 1, [202, 204, 206, 208], np.int64)
    assert eq((100 + i8) << 1, [-54, -52, -50, -48], np.int8)
    assert eq((np.array(100) + i8) << 1, [202, 204, 206, 208], np.int64)
    assert eq((i32 + 1.5) * 2, [5., 7., 9., 11.], np.float64)
    assert eq((i32 + np.array(1.5)) * 2, [5., 7., 9., 11.], np.float64)
    assert eq((1.5 + i32) * 2, [5., 7., 9., 11.], np.float64)
    assert eq((np.array(1.5) + i32) * 2, [5., 7., 9., 11.], np.float64)
    assert eq((i32 + (1.5 + 0j)) * 2, [5., 7., 9., 11.], np.complex128)
    assert eq((i32 + np.array(1.5 + 0j)) * 2, [5., 7., 9., 11.], np.complex128)
    assert eq(((1.5 + 0j) + i32) * 2, [5., 7., 9., 11.], np.complex128)
    assert eq((np.array(1.5 + 0j) + i32) * 2, [5., 7., 9., 11.], np.complex128)

    # Complex -> Float conversions
    z = np.array([1+1j, 2+2j, 3+3j])
    assert eq(1 + abs(z), [2.41421356, 3.82842712, 5.24264069], np.float64)
    assert eq(1 + np.abs(z), [2.41421356, 3.82842712, 5.24264069], np.float64)
    z = np.array([1+1j, 2+2j, 3+3j], np.complex64)
    assert eq(1 + abs(z), [2.41421356, 3.82842712, 5.24264069], np.float32)
    assert eq(1 + np.abs(z), [2.41421356, 3.82842712, 5.24264069], np.float32)

test_mixed_types()
