from core.llvm import *

@test
def test_int_llvm_ops():
    assert add_int(42, 99) == 141
    assert add_int(-10, 10) == 0
    assert sub_int(12, 6) == 6
    assert sub_int(5, -5) == 10
    assert mul_int(22, 33) == 726
    assert mul_int(-3, 3) == -9
    assert div_int(10, 2) == 5
    assert div_int(10, 3) == 3
    assert div_int(10, -3) == -3
    assert mod_int(10, 2) == 0
    assert mod_int(10, 3) == 1

    assert eq_int(42, 42)
    assert not eq_int(10, -10)
    assert ne_int(0, 1)
    assert not ne_int(-3, -3)

    assert lt_int(2, 3)
    assert not lt_int(3, 2)
    assert not lt_int(3, 3)

    assert not gt_int(2, 3)
    assert gt_int(3, 2)
    assert not gt_int(3, 3)

    assert le_int(2, 3)
    assert not le_int(3, 2)
    assert le_int(3, 3)

    assert not ge_int(2, 3)
    assert ge_int(3, 2)
    assert ge_int(3, 3)

    assert inv_int(0b1010) == -11
    assert and_int(0b1010, 0b1101) == 0b1000
    assert or_int(0b1010, 0b1101) == 0b1111
    assert xor_int(0b1010, 0b1101) == 0b0111
    assert shr_int(0b1010, 3) == 0b1
    assert shl_int(0b1010, 3) == 0b1010000

    assert bitreverse_int(0b0111110111010110001001001000001010001000110001010110001101001101) == 0b1011001011000110101000110001000101000001001001000110101110111110
    assert bitreverse_int(0) == 0
    assert bitreverse_int(0xffffffffffffffff) == 0xffffffffffffffff
    assert ctpop_int(0x7dd6248288c5634d) == 29
    assert ctpop_int(0) == 0
    assert ctpop_int(0xffffffffffffffff) == 64
    assert bswap_int(0x7dd6248288c5634d) == 0x4d63c5888224d67d
    assert bswap_int(0) == 0
    assert bswap_int(0xffffffffffffffff) == 0xffffffffffffffff
    assert ctlz_int(0b0001110111010110001001001000001010001000110001010110001101001101) == 3
    assert ctlz_int(0b0011110111010110001001001000001010001000110001010110001101001101) == 2
    assert ctlz_int(0b0111110111010110001001001000001010001000110001010110001101001101) == 1
    assert ctlz_int(0b1111110111010110001001001000001010001000110001010110001101001101) == 0
    assert ctlz_int(0) == 64
    assert cttz_int(0b0001110111010110001001001000001010001000110001010110001101001000) == 3
    assert cttz_int(0b0001110111010110001001001000001010001000110001010110001101001100) == 2
    assert cttz_int(0b0001110111010110001001001000001010001000110001010110001101001110) == 1
    assert cttz_int(0b0001110111010110001001001000001010001000110001010110001101001111) == 0
    assert cttz_int(0) == 64

@test
def test_float_llvm_ops():
    def approx_eq(a: float, b: float, thresh: float = 1e-10):
        return -thresh <= a - b <= thresh
    PI = 3.1415926535897932384626433832795028841971693993751058209749445923078164062
    E = 2.718281828459045235360287471352662497757247093699959574966967627724076630353

    assert add_float(42., 99.) == 141.
    assert add_float(-10., 10.) == 0.
    assert sub_float(12., 6.) == 6.
    assert sub_float(5., -5.) == 10.
    assert mul_float(22., 33.) == 726.
    assert mul_float(-3., 3.) == -9.
    assert div_float(10., 2.) == 5.
    assert div_float(10., 4.) == 2.5
    assert div_float(10., -2.5) == -4.
    assert mod_float(10., 2.) == 0.
    assert mod_float(10., 3.) == 1.

    assert eq_float(42., 42.)
    assert not eq_float(10., -10.)
    assert ne_float(0., 1.)
    assert not ne_float(-3., -3.)

    assert lt_float(2., 3.)
    assert not lt_float(3., 2.)
    assert not lt_float(3., 3.)

    assert not gt_float(2., 3.)
    assert gt_float(3., 2.)
    assert not gt_float(3., 3.)

    assert le_float(2., 3.)
    assert not le_float(3., 2.)
    assert le_float(3., 3.)

    assert not ge_float(2., 3.)
    assert ge_float(3., 2.)
    assert ge_float(3., 3.)

    assert pow_float(10., 2.) == 100.
    assert sqrt_float(100.) == 10.
    assert sin_float(0.0) == 0.
    assert sin_float(PI/2) == 1.
    assert approx_eq(sin_float(PI), 0.)
    assert cos_float(0.0) == 1.
    assert approx_eq(cos_float(PI/2), 0.)
    assert cos_float(PI) == -1.
    assert exp_float(0.) == 1.
    assert exp_float(1.) == E
    assert exp2_float(0.) == 1.
    assert exp2_float(1.) == 2.
    assert log_float(1.) == 0.
    assert log_float(E) == 1.
    assert log10_float(1.) == 0.
    assert log10_float(10.) == 1.
    assert log2_float(1.) == 0.
    assert log2_float(2.) == 1.
    assert abs_float(1.5) == 1.5
    assert abs_float(-1.5) == 1.5
    assert pow_float(1., 0.) == 1.
    assert pow_float(3., 2.) == 9.
    assert pow_float(2., -2.) == 0.25
    assert pow_float(-2., 2.) == 4.
    assert min_float(1., 1.) == 1.
    assert min_float(-1., 1.) == -1.
    assert min_float(3., 2.) == 2.
    assert max_float(1., 1.) == 1.
    assert max_float(-1., 1.) == 1.
    assert max_float(3., 2.) == 3.
    assert copysign_float(100., 1.234) == 100.
    assert copysign_float(100., -1.234) == -100.
    assert copysign_float(-100., 1.234) == 100.
    assert copysign_float(-100., -1.234) == -100.
    assert fma_float(2., 3., 4.) == 10.

    assert floor_float(1.5) == 1.
    assert ceil_float(1.5) == 2.
    assert trunc_float(-1.5) == -1.
    assert rint_float(1.8) == 2.
    assert rint_float(1.3) == 1.
    assert nearbyint_float(2.3) == 2.
    assert nearbyint_float(-3.8) == -4.
    assert round_float(2.3) == 2.
    assert round_float(-2.3) == -2.

@test
def test_conversion_llvm_ops():
    assert int_to_float(42) == 42.0
    assert int_to_float(-100) == -100.0
    assert float_to_int(3.14) == 3
    assert float_to_int(-3.14) == -3

@test
def test_str_llvm_ops():
    N = 10
    p = ptr[byte](N)
    for i in range(N):
        p[i] = byte(i + 1)

    q = ptr[byte](N)
    for i in range(N):
        q[i] = byte(0)

    memcpy(q, p, N)
    for i in range(10):
        assert q[i] == byte(i + 1)

    memmove(p + 1, p, N - 1)
    assert p[1] == p[0]
    for i in range(1, N):
        assert p[i] == byte(i)

    memset(p, byte(42), N)
    for i in range(N):
        assert p[i] == byte(42)

test_int_llvm_ops()
test_float_llvm_ops()
test_conversion_llvm_ops()
test_str_llvm_ops()
