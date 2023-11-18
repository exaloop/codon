import math

NAN = math.nan
INF = math.inf
NINF = -math.inf


def close(a: float, b: float, epsilon: float = 1e-7):
    return abs(a - b) <= epsilon


@test
def test_isnan():
    assert math.isnan(float("nan")) == True
    assert math.isnan(4.0) == False


@test
def test_isinf():
    assert math.isinf(float("inf")) == True
    assert math.isinf(7.0) == False


@test
def test_isfinite():
    assert math.isfinite(1.4) == True
    assert math.isfinite(0.0) == True
    assert math.isfinite(NAN) == False
    assert math.isfinite(INF) == False
    assert math.isfinite(NINF) == False


@test
def test_ceil():
    assert isinstance(math.ceil(1.0), int)
    assert math.ceil(3.3) == 4
    assert math.ceil(0.5) == 1
    assert math.ceil(1.0) == 1
    assert math.ceil(1.5) == 2
    assert math.ceil(-0.5) == 0
    assert math.ceil(-1.0) == -1
    assert math.ceil(-1.5) == -1


@test
def test_floor():
    assert isinstance(math.floor(1.0), int)
    assert math.floor(3.3) == 3
    assert math.floor(0.5) == 0
    assert math.floor(1.0) == 1
    assert math.floor(1.5) == 1
    assert math.floor(-0.5) == -1
    assert math.floor(-1.0) == -1
    assert math.floor(-1.5) == -2


@test
def test_fabs():
    assert math.fabs(-1.0) == 1
    assert math.fabs(0.0) == 0
    assert math.fabs(1.0) == 1


@test
def test_fmod():
    assert math.fmod(10.0, 1.0) == 0.0
    assert math.fmod(10.0, 0.5) == 0.0
    assert math.fmod(10.0, 1.5) == 1.0
    assert math.fmod(-10.0, 1.0) == -0.0
    assert math.fmod(-10.0, 0.5) == -0.0
    assert math.fmod(-10.0, 1.5) == -1.0


@test
def test_exp():
    assert math.exp(0.0) == 1
    assert math.exp(-1.0) == 1 / math.e
    assert math.exp(1.0) == math.e


@test
def test_expm1():
    assert math.expm1(0.0) == 0
    assert close(math.expm1(1.0), 1.7182818284590453)
    assert close(math.expm1(3.0), 19.085536923187668)
    assert close(math.expm1(5.0), 147.4131591025766)
    assert math.expm1(INF) == INF
    assert math.expm1(NINF) == -1
    assert math.isnan(math.expm1(NAN)) == True


@test
def test_ldexp():
    assert math.ldexp(0.0, 1) == 0.0
    assert math.ldexp(1.0, 1) == 2.0
    assert math.ldexp(1.0, -1) == 0.5
    assert math.ldexp(-1.0, 1) == -2.0
    assert math.ldexp(0.0, 1) == 0.0
    assert math.ldexp(1.0, -1000000) == 0.0
    assert math.ldexp(-1.0, -1000000) == -0.0
    assert math.ldexp(INF, 30) == INF
    assert math.ldexp(NINF, -213) == NINF
    assert math.isnan(math.ldexp(NAN, 0)) == True


@test
def test_log():
    assert math.log(1.0 / math.e) == -1
    assert math.log(1.0) == 0
    assert math.log(math.e) == 1


@test
def test_log2():
    assert math.log2(1.0) == 0.0
    assert math.log2(2.0) == 1.0
    assert math.log2(4.0) == 2.0
    assert math.log2(2.0 ** 1023) == 1023.0
    assert math.isnan(math.log2(-1.5)) == True
    assert math.isnan(math.log2(NINF)) == True
    assert math.isnan(math.log2(NAN)) == True


@test
def test_log10():
    assert math.log10(0.1) == -1
    assert math.log10(1.0) == 0
    assert math.log10(10.0) == 1
    assert math.log10(10000.0) == 4


@test
def test_degrees():
    assert math.degrees(math.pi) == 180.0
    assert math.degrees(math.pi / 2) == 90.0
    assert math.degrees(-math.pi / 4) == -45.0
    assert math.degrees(0.0) == 0.0


@test
def test_radians():
    assert math.radians(180.0) == math.pi
    assert math.radians(90.0) == math.pi / 2
    assert math.radians(-45.0) == -math.pi / 4
    assert math.radians(0.0) == 0.0


@test
def test_sqrt():
    assert math.sqrt(4.0) == 2
    assert math.sqrt(0.0) == 0
    assert math.sqrt(1.0) == 1
    assert math.isnan(math.sqrt(-1.0)) == True


@test
def test_pow():
    assert math.pow(0.0, 1.0) == 0
    assert math.pow(1.0, 0.0) == 1
    assert math.pow(2.0, 1.0) == 2
    assert math.pow(2.0, -1.0) == 0.5
    assert math.pow(-0.0, 3.0) == -0.0
    assert math.pow(-0.0, 2.3) == 0.0
    assert math.pow(-0.0, 0.0) == 1
    assert math.pow(-0.0, -0.0) == 1
    assert math.pow(-2.0, 2.0) == 4.0
    assert math.pow(-2.0, 3.0) == -8.0
    assert math.pow(-2.0, -3.0) == -0.125
    assert math.pow(INF, 1.0) == INF
    assert math.pow(NINF, 1.0) == NINF
    assert math.pow(1.0, INF) == 1
    assert math.pow(1.0, NINF) == 1
    assert math.isnan(math.pow(NAN, 1.0)) == True
    assert math.isnan(math.pow(2.0, NAN)) == True
    assert math.isnan(math.pow(0.0, NAN)) == True
    assert math.pow(1.0, NAN) == 1


@test
def test_acos():
    assert math.acos(-1.0) == math.pi
    assert math.acos(0.0) == math.pi / 2
    assert math.acos(1.0) == 0
    assert math.isnan(math.acos(NAN)) == True


@test
def test_asin():
    assert math.asin(-1.0) == -math.pi / 2
    assert math.asin(0.0) == 0
    assert math.asin(1.0) == math.pi / 2
    assert math.isnan(math.asin(NAN)) == True


@test
def test_atan():
    assert math.atan(-1.0) == -math.pi / 4
    assert math.atan(0.0) == 0
    assert math.atan(1.0) == math.pi / 4
    assert math.atan(INF) == math.pi / 2
    assert math.atan(NINF) == -math.pi / 2
    assert math.isnan(math.atan(NAN)) == True


@test
def test_atan2():
    assert math.atan2(-1.0, 0.0) == -math.pi / 2
    assert math.atan2(-1.0, 1.0) == -math.pi / 4
    assert math.atan2(0.0, 1.0) == 0
    assert math.atan2(1.0, 1.0) == math.pi / 4
    assert math.atan2(1.0, 0.0) == math.pi / 2
    assert math.atan2(-0.0, 0.0) == -0
    assert math.atan2(-0.0, 2.3) == -0
    assert math.atan2(0.0, -2.3) == math.pi
    assert math.atan2(INF, NINF) == math.pi * 3 / 4
    assert math.atan2(INF, 2.3) == math.pi / 2
    assert math.isnan(math.atan2(NAN, 0.0)) == True


@test
def test_cos():
    assert math.cos(0.0) == 1
    assert close(math.cos(math.pi / 2), 6.123233995736766e-17)
    assert close(math.cos(-math.pi / 2), 6.123233995736766e-17)
    assert math.cos(math.pi) == -1
    assert math.isnan(math.cos(INF)) == True
    assert math.isnan(math.cos(NINF)) == True
    assert math.isnan(math.cos(NAN)) == True


@test
def test_sin():
    assert math.sin(0.0) == 0
    assert math.sin(math.pi / 2) == 1
    assert math.sin(-math.pi / 2) == -1
    assert math.isnan(math.sin(INF)) == True
    assert math.isnan(math.sin(NINF)) == True
    assert math.isnan(math.sin(NAN)) == True


@test
def test_hypot():
    assert math.hypot(12.0, 5.0) == 13
    assert math.hypot(12.0 / 32.0, 5.0 / 32) == 13 / 32
    assert math.hypot(0.0, 0.0) == 0
    assert math.hypot(-3.0, 4.0) == 5
    assert math.hypot(3.0, 4.0) == 5


@test
def test_tan():
    assert math.tan(0.0) == 0
    assert close(math.tan(math.pi / 4), 0.9999999999999999)
    assert close(math.tan(-math.pi / 4), -0.9999999999999999)
    assert math.isnan(math.tan(INF)) == True
    assert math.isnan(math.tan(NINF)) == True
    assert math.isnan(math.tan(NAN)) == True


@test
def test_cosh():
    assert math.cosh(0.0) == 1
    assert math.cosh(2.0) - 2 * math.cosh(1.0) ** 2 == -1
    assert math.cosh(INF) == INF
    assert math.cosh(NINF) == INF
    assert math.isnan(math.cosh(NAN)) == True


@test
def test_sinh():
    assert math.sinh(0.0) == 0
    assert math.sinh(1.0) + math.sinh(-1.0) == 0
    assert math.sinh(INF) == INF
    assert math.sinh(NINF) == NINF
    assert math.isnan(math.sinh(NAN)) == True


@test
def test_tanh():
    assert math.tanh(0.0) == 0
    assert math.tanh(1.0) + math.tanh(-1.0) == 0
    assert math.tanh(INF) == 1
    assert math.tanh(NINF) == -1
    assert math.isnan(math.tanh(NAN)) == True


@test
def test_acosh():
    assert math.acosh(1.0) == 0
    assert close(math.acosh(2.0), 1.3169578969248166)
    assert math.acosh(INF) == INF
    assert math.isnan(math.acosh(NAN)) == True
    assert math.isnan(math.acosh(-1.0)) == True


@test
def test_asinh():
    assert math.asinh(0.0) == 0
    assert close(math.asinh(1.0), 0.881373587019543)
    assert close(math.asinh(-1.0), -0.881373587019543)
    assert math.asinh(INF) == INF
    assert math.isnan(math.asinh(NAN)) == True
    assert math.asinh(NINF) == NINF


@test
def test_atanh():
    assert math.atanh(0.0) == 0
    assert close(math.atanh(0.5), 0.5493061443340549)
    assert close(math.atanh(-0.5), -0.5493061443340549)
    assert math.isnan(math.atanh(INF)) == True
    assert math.isnan(math.atanh(NAN)) == True
    assert math.isnan(math.atanh(NINF)) == True


@test
def test_copysign():
    assert math.copysign(1.0, -0.0) == -1
    assert math.copysign(1.0, 42.0) == 1
    assert math.copysign(1.0, -42.0) == -1
    assert math.copysign(3.0, 0.0) == 3
    assert math.copysign(INF, 0.0) == INF
    assert math.copysign(INF, -0.0) == NINF
    assert math.copysign(NINF, 0.0) == INF
    assert math.copysign(NINF, -0.0) == NINF
    assert math.copysign(1.0, INF) == 1
    assert math.copysign(1.0, NINF) == -1
    assert math.copysign(INF, INF) == INF
    assert math.copysign(INF, NINF) == NINF
    assert math.copysign(NINF, INF) == INF
    assert math.copysign(NINF, NINF) == NINF
    assert math.isnan(math.copysign(NAN, 1.0)) == True
    assert math.isnan(math.copysign(NAN, INF)) == True
    assert math.isnan(math.copysign(NAN, NINF)) == True
    assert math.isnan(math.copysign(NAN, NAN)) == True


@test
def test_log1p():
    assert close(math.log1p(2.0), 1.0986122886681098)
    assert close(math.log1p(2.0 ** 90), 62.383246250395075)
    assert close(math.log1p(2.0 ** 300), 207.94415416798358)
    assert math.log1p(INF) == INF
    assert math.log1p(-1.0) == NINF


@test
def test_trunc():
    assert isinstance(math.trunc(1.0), int)
    assert math.trunc(1.0) == 1
    assert math.trunc(-1.0) == -1
    assert math.trunc(1.5) == 1
    assert math.trunc(-1.5) == -1
    assert math.trunc(1.99999999) == 1
    assert math.trunc(-1.99999999) == -1
    assert math.trunc(0.99999999) == 0
    assert math.trunc(-100.999) == -100


@test
def test_erf():
    assert close(math.erf(1.0), 0.8427007929497148)
    assert math.erf(0.0) == 0
    assert close(math.erf(3.0), 0.9999779095030015)
    assert math.erf(256.0) == 1.0
    assert math.erf(INF) == 1.0
    assert math.erf(NINF) == -1.0
    assert math.isnan(math.erf(NAN)) == True


@test
def test_erfc():
    assert math.erfc(0.0) == 1.0
    assert close(math.erfc(1.0), 0.15729920705028516)
    assert close(math.erfc(2.0), 0.0046777349810472645)
    assert close(math.erfc(-1.0), 1.8427007929497148)
    assert math.erfc(INF) == 0.0
    assert math.erfc(NINF) == 2.0
    assert math.isnan(math.erfc(NAN)) == True


@test
def test_gamma():
    assert close(math.gamma(6.0), 120.0)
    assert close(math.gamma(1.0), 1.0)
    assert close(math.gamma(2.0), 1.0)
    assert close(math.gamma(3.0), 2.0)
    assert math.isnan(math.gamma(-1.0)) == True
    assert math.gamma(INF) == INF
    assert math.isnan(math.gamma(NINF)) == True
    assert math.isnan(math.gamma(NAN)) == True


@test
def test_lgamma():
    assert math.lgamma(1.0) == 0.0
    assert math.lgamma(2.0) == 0.0
    assert math.lgamma(-1.0) == INF
    assert math.lgamma(INF) == INF
    assert math.lgamma(NINF) == INF
    assert math.isnan(math.lgamma(NAN)) == True


@test
def test_remainder():
    assert math.remainder(2.0, 2.0) == 0.0
    assert math.remainder(-4.0, 1.0) == -0.0
    assert close(math.remainder(-3.8, 1.0), 0.20000000000000018)
    assert close(math.remainder(3.8, 1.0), -0.20000000000000018)
    assert math.isnan(math.remainder(INF, 1.0)) == True
    assert math.isnan(math.remainder(NINF, 1.0)) == True
    assert math.isnan(math.remainder(NAN, 1.0)) == True


@test
def test_gcd():
    assert math.gcd(0, 0) == 0
    assert math.gcd(1, 0) == 1
    assert math.gcd(-1, 0) == 1
    assert math.gcd(0, 1) == 1
    assert math.gcd(0, -1) == 1
    assert math.gcd(7, 1) == 1
    assert math.gcd(7, -1) == 1
    assert math.gcd(-23, 15) == 1
    assert math.gcd(120, 84) == 12
    assert math.gcd(84, -120) == 12
    assert math.gcd(Int[128]('1216342683557601535506311712'), Int[128]('436522681849110124616458784')) == Int[128](32)

    x = 43461
    y = 1064
    for c in (652560, 576559230):
        a = x * c
        b = y * c
        assert math.gcd(a, b) == c
        assert math.gcd(b, a) == c
        assert math.gcd(-a, b) == c
        assert math.gcd(b, -a) == c
        assert math.gcd(a, -b) == c
        assert math.gcd(-b, a) == c
        assert math.gcd(-a, -b) == c
        assert math.gcd(-b, -a) == c

    assert math.gcd() == 0
    assert math.gcd(120) == 120
    assert math.gcd(-120) == 120
    assert math.gcd(120, 84, 102) == 6
    assert math.gcd(120, 1, 84) == 1


@test
def test_lcm():
    assert math.lcm(0, 0) == 0
    assert math.lcm(1, 0) == 0
    assert math.lcm(-1, 0) == 0
    assert math.lcm(0, 1) == 0
    assert math.lcm(0, -1) == 0
    assert math.lcm(7, 1) == 7
    assert math.lcm(7, -1) == 7
    assert math.lcm(-23, 15) == 345
    assert math.lcm(120, 84) == 840
    assert math.lcm(84, -120) == 840
    #assert math.lcm(1216342683557601535506311712, 436522681849110124616458784) == 16592536571065866494401400422922201534178938447014944

    x = 4346103
    y = 1064501
    for c in (8171, 58657):
        a = x * c
        b = y * c
        d = x * y * c
        assert math.lcm(a, b) == d
        assert math.lcm(b, a) == d
        assert math.lcm(-a, b) == d
        assert math.lcm(b, -a) == d
        assert math.lcm(a, -b) == d
        assert math.lcm(-b, a) == d
        assert math.lcm(-a, -b) == d
        assert math.lcm(-b, -a) == d

    assert math.lcm() == 1
    assert math.lcm(120) == 120
    assert math.lcm(-120) == 120
    assert math.lcm(120, 84, 102) == 14280
    assert math.lcm(120, 0, 84) == 0


@test
def test_frexp():
    assert math.frexp(-2.0) == (-0.5, 2)
    assert math.frexp(-1.0) == (-0.5, 1)
    assert math.frexp(0.0) == (0.0, 0)
    assert math.frexp(1.0) == (0.5, 1)
    assert math.frexp(2.0) == (0.5, 2)
    assert math.frexp(INF)[0] == INF
    assert math.frexp(NINF)[0] == NINF
    assert math.isnan(math.frexp(NAN)[0]) == True


@test
def test_modf():
    assert math.modf(1.5) == (0.5, 1.0)
    assert math.modf(-1.5) == (-0.5, -1.0)
    assert math.modf(math.inf) == (0.0, INF)
    assert math.modf(-math.inf) == (-0.0, NINF)
    modf_nan = math.modf(NAN)
    assert math.isnan(modf_nan[0]) == True
    assert math.isnan(modf_nan[1]) == True


@test
def test_isclose():
    assert math.isclose(1.0 + 1.0, 1.000000000001 + 1.0) == True
    assert math.isclose(2.90909324093284, 2.909093240932844234234234234) == True
    assert math.isclose(2.90909324093284, 2.9) == False
    assert math.isclose(2.90909324093284, 2.90909324) == True
    assert math.isclose(2.90909324, 2.90909325) == False
    assert math.isclose(NAN, 2.9) == False
    assert math.isclose(2.9, NAN) == False
    assert math.isclose(INF, INF) == True
    assert math.isclose(NINF, NINF) == True
    assert math.isclose(NINF, INF) == False
    assert math.isclose(INF, NINF) == False


@test
def test_fsum():
    assert math.fsum((42,)) == 42.0
    assert math.fsum((1,2,3)) == 6.0
    assert math.fsum((1,2,-3)) == 0.0
    assert math.fsum(()) == 0.0
    assert math.fsum([.1] * 10) == 1.0

    mant_dig = 53
    etiny = -1074

    def sum_exact(p):
        n = len(p)
        hi = 0.0
        if n > 0:
            hi = p[n-1]
            n -= 1
            while n > 0:
                x = hi
                y = p[n-1]
                n -= 1
                hi = x + y
                yr = hi - x
                lo = y - yr
                if lo != 0.0:
                    break

            if n > 0 and ((lo < 0 and p[n-1] < 0) or (lo > 0 and p[n-1] > 0)):
                y = lo * 2
                x = hi + y
                yr = x - hi
                if y == yr:
                    hi = x
        return hi

    def msum(iterable):
        "Full precision summation using multiple floats for intermediate values"
        # Rounded x+y stored in hi with the round-off stored in lo.  Together
        # hi+lo are exactly equal to x+y.  The inner loop applies hi/lo summation
        # to each partial so that the list of partial sums remains exact.
        # Depends on IEEE-754 arithmetic guarantees.  See proof of correctness at:
        # www-2.cs.cmu.edu/afs/cs/project/quake/public/papers/robust-arithmetic.ps

        partials = []               # sorted, non-overlapping partial sums
        for x in iterable:
            i = 0
            for y in partials:
                if abs(x) < abs(y):
                    x, y = y, x
                hi = x + y
                lo = y - (hi - x)
                if lo:
                    partials[i] = lo
                    i += 1
                x = hi
            partials[i:] = [x]
        return sum_exact(partials)

    @pure
    @llvm
    def float1() -> float:
        ret double 0x401DF11F45F4E61A

    @pure
    @llvm
    def float2() -> float:
        ret double 0xBFE62A2AF1BD3624

    @pure
    @llvm
    def float3() -> float:
        ret double 0x7C95555555555555

    test_values = [
        ([], 0.0),
        ([0.0], 0.0),
        ([1e100, 1.0, -1e100, 1e-100, 1e50, -1.0, -1e50], 1e-100),
        ([2.0**53, -0.5, -2.0**-54], 2.0**53-1.0),
        ([2.0**53, 1.0, 2.0**-100], 2.0**53+2.0),
        ([2.0**53+10.0, 1.0, 2.0**-100], 2.0**53+12.0),
        ([2.0**53-4.0, 0.5, 2.0**-54], 2.0**53-3.0),
        ([1./n for n in range(1, 1001)], float1()),
        ([(-1.)**n/n for n in range(1, 1001)], float2()),
        ([1e16, 1., 1e-16], 10000000000000002.0),
        ([1e16-2., 1.-2.**-53, -(1e16-2.), -(1.-2.**-53)], 0.0),
        # exercise code for resizing partials array
        ([2.**n - 2.**(n+50) + 2.**(n+52) for n in range(-1074, 972, 2)] +
         [-2.**1022],
         float3()),
        ]

    # Telescoping sum, with exact differences (due to Sterbenz)
    terms = [1.7**i for i in range(1001)]
    test_values.append((
        [terms[i+1] - terms[i] for i in range(1000)] + [-terms[1000]],
        -terms[0]
    ))

    for i, (vals, expected) in enumerate(test_values):
        try:
            actual = math.fsum(vals)
        except OverflowError:
            # self.fail("test %d failed: got OverflowError, expected %r "
            #           "for math.fsum(%.100r)" % (i, expected, vals))
            assert False
        except ValueError:
            # self.fail("test %d failed: got ValueError, expected %r "
            #           "for math.fsum(%.100r)" % (i, expected, vals))
            assert False

        assert actual == expected

    from random import random, gauss, shuffle
    for j in range(10000):
        vals = [7, 1e100, -7, -1e100, -9e-20, 8e-20] * 10
        s = 0.
        for i in range(200):
            v = gauss(0, random()) ** 7 - s
            s += v
            vals.append(v)
        shuffle(vals)
        assert msum(vals) == math.fsum(vals)


@test
def test_prod():
    is_nan = lambda x: math.isnan(x)
    prod = math.prod
    assert prod(()) == 1
    assert prod((), start=5) == 5
    assert prod(list(range(2,8))) == 5040
    assert prod(iter(list(range(2,8)))) == 5040
    assert prod(range(1, 10), start=10) == 3628800

    assert prod([1, 2, 3, 4, 5]) == 120
    assert prod([1.0, 2.0, 3.0, 4.0, 5.0]) == 120.0
    assert prod([1, 2, 3, 4.0, 5.0]) == 120.0
    assert prod([1.0, 2.0, 3.0, 4, 5]) == 120.0

    # Test overflow in fast-path for integers
    assert prod([1, 1, 2**32, 1, 1]) == 2**32
    # Test overflow in fast-path for floats
    assert prod([1.0, 1.0, 2**32, 1, 1]) == float(2**32)

    # Some odd cases
    assert prod([2, 3], start='ab') == 'abababababab'
    assert prod([2, 3], start=[1, 2]) == [1, 2, 1, 2, 1, 2, 1, 2, 1, 2, 1, 2]
    assert prod((), start={2: 3}) == {2:3}

    #with self.assertRaises(TypeError):
    #    prod([10, 20], 1)     # start is a keyword-only argument

    assert prod([0, 1, 2, 3]) == 0
    assert prod([1, 0, 2, 3]) == 0
    assert prod([1, 2, 3, 0]) == 0

    def _naive_prod(iterable, start=1):
        for elem in iterable:
            start *= elem
        return start

    iterable = range(1, 13)
    assert prod(iterable) == _naive_prod(iterable)
    iterable = range(-12, -1)
    assert prod(iterable) == _naive_prod(iterable)
    iterable = range(-11, 5)
    assert prod(iterable) == 0

    # Big floats

    iterable = [float(x) for x in range(1, 123)]
    assert prod(iterable) == _naive_prod(iterable, 1.0)
    iterable = [float(x) for x in range(-123, -1)]
    assert prod(iterable) == _naive_prod(iterable, 1.0)
    iterable = [float(x) for x in range(-1000, 1000)]
    assert is_nan(prod(iterable))

    # Float tests

    assert is_nan(prod([1, 2, 3, float("nan"), 2, 3]))
    assert is_nan(prod([1, 0, float("nan"), 2, 3]))
    assert is_nan(prod([1, float("nan"), 0, 3]))
    assert is_nan(prod([1, float("inf"), float("nan"),3]))
    assert is_nan(prod([1, float("-inf"), float("nan"),3]))
    assert is_nan(prod([1, float("nan"), float("inf"),3]))
    assert is_nan(prod([1, float("nan"), float("-inf"),3]))

    assert prod([1, 2, 3, float('inf'),-3,4]) == float('-inf')
    assert prod([1, 2, 3, float('-inf'),-3,4]) == float('inf')

    assert is_nan(prod([1,2,0,float('inf'), -3, 4]))
    assert is_nan(prod([1,2,0,float('-inf'), -3, 4]))
    assert is_nan(prod([1, 2, 3, float('inf'), -3, 0, 3]))
    assert is_nan(prod([1, 2, 3, float('-inf'), -3, 0, 2]))

    # Type preservation

    assert type(prod([1, 2, 3, 4, 5, 6])) is int
    assert type(prod([1, 2.0, 3, 4, 5, 6])) is float
    assert type(prod(range(1, 10000))) is int
    assert type(prod(range(1, 10000), start=1.0)) is float
    #assert type(prod([1, decimal.Decimal(2.0), 3, 4, 5, 6])) is decimal.Decimal

    # Tuples

    assert prod((42,)) == 42
    assert prod((-3.5, 4.0)) == -14.0
    assert prod((3, 9, 3, 7, 1)) == 567
    assert prod((1, 2.5, 3)) == 7.5
    assert prod((2.5, 1, 3, 1.0, 1)) == 7.5
    assert prod((2, 3), start='ab') == 'abababababab'


test_isnan()
test_isinf()
test_isfinite()
test_ceil()
test_floor()
test_fabs()
test_fmod()
test_exp()
test_expm1()
test_ldexp()
test_log()
test_log2()
test_log10()
test_degrees()
test_radians()
test_sqrt()
test_pow()
test_acos()
test_asin()
test_atan()
test_atan2()
test_cos()
test_sin()
test_hypot()
test_tan()
test_cosh()
test_sinh()
test_tanh()
test_acosh()
test_asinh()
test_atanh()
test_copysign()
test_log1p()
test_trunc()
test_erf()
test_erfc()
test_gamma()
test_lgamma()
test_remainder()
test_gcd()
test_lcm()
test_frexp()
test_modf()
test_isclose()
test_fsum()
test_prod()


# 32-bit float ops

NAN32 = math.nan32
INF32 = math.inf32
NINF32 = -math.inf32


def close32(a: float32, b: float32, epsilon: float32 = 1e-5f32):
    return abs(a - b) <= epsilon


@test
def test_float32_isnan():
    assert math.isnan(float32(float("nan"))) == True
    assert math.isnan(4.0f32) == False


@test
def test_float32_isinf():
    assert math.isinf(float32(float("inf"))) == True
    assert math.isinf(7.0f32) == False


@test
def test_float32_isfinite():
    assert math.isfinite(1.4f32) == True
    assert math.isfinite(0.0f32) == True
    assert math.isfinite(NAN32) == False
    assert math.isfinite(INF32) == False
    assert math.isfinite(NINF32) == False


@test
def test_float32_ceil():
    assert isinstance(math.ceil(1.0f32), int)
    assert math.ceil(3.3f32) == 4
    assert math.ceil(0.5f32) == 1
    assert math.ceil(1.0f32) == 1
    assert math.ceil(1.5f32) == 2
    assert math.ceil(-0.5f32) == 0
    assert math.ceil(-1.0f32) == -1
    assert math.ceil(-1.5f32) == -1


@test
def test_float32_floor():
    assert isinstance(math.floor(1.0f32), int)
    assert math.floor(3.3f32) == 3
    assert math.floor(0.5f32) == 0
    assert math.floor(1.0f32) == 1
    assert math.floor(1.5f32) == 1
    assert math.floor(-0.5f32) == -1
    assert math.floor(-1.0f32) == -1
    assert math.floor(-1.5f32) == -2


@test
def test_float32_fabs():
    assert math.fabs(-1.0f32) == 1.0f32
    assert math.fabs(0.0f32) == 0.0f32
    assert math.fabs(1.0f32) == 1.0f32


@test
def test_float32_fmod():
    assert math.fmod(10.0f32, 1.0f32) == 0.0f32
    assert math.fmod(10.0f32, 0.5f32) == 0.0f32
    assert math.fmod(10.0f32, 1.5f32) == 1.0f32
    assert math.fmod(-10.0f32, 1.0f32) == -0.0f32
    assert math.fmod(-10.0f32, 0.5f32) == -0.0f32
    assert math.fmod(-10.0f32, 1.5f32) == -1.0f32


@test
def test_float32_exp():
    assert math.exp(0.0f32) == 1.0f32
    assert math.exp(-1.0f32) == 1.0f32 / math.e32
    assert math.exp(1.0f32) == math.e32


@test
def test_float32_expm1():
    assert math.expm1(0.0f32) == 0.0f32
    assert close32(math.expm1(1.0f32), 1.7182818284590453f32)
    assert close32(math.expm1(3.0f32), 19.085536923187668f32)
    assert close32(math.expm1(5.0f32), 147.4131591025766f32)
    assert math.expm1(INF32) == INF32
    assert math.expm1(NINF32) == -1.0f32
    assert math.isnan(math.expm1(NAN32)) == True


@test
def test_float32_ldexp():
    assert math.ldexp(0.0f32, 1) == 0.0f32
    assert math.ldexp(1.0f32, 1) == 2.0f32
    assert math.ldexp(1.0f32, -1) == 0.5f32
    assert math.ldexp(-1.0f32, 1) == -2.0f32
    assert math.ldexp(0.0f32, 1) == 0.0f32
    assert math.ldexp(1.0f32, -1000000) == 0.0f32
    assert math.ldexp(-1.0f32, -1000000) == -0.0f32
    assert math.ldexp(INF32, 30) == INF32
    assert math.ldexp(NINF32, -213) == NINF32
    assert math.isnan(math.ldexp(NAN32, 0)) == True


@test
def test_float32_log():
    assert math.log(1.0f32 / math.e32) == -1.0f32
    assert math.log(1.0f32) == 0.0f32
    assert close32(math.log(math.e32), 1.0f32)


@test
def test_float32_log2():
    assert math.log2(1.0f32) == 0.0f32
    assert math.log2(2.0f32) == 1.0f32
    assert math.log2(4.0f32) == 2.0f32
    assert math.log2(2.0f32 ** 50.0f32) == 50.0f32
    assert math.isnan(math.log2(-1.5f32)) == True
    assert math.isnan(math.log2(NINF32)) == True
    assert math.isnan(math.log2(NAN32)) == True


@test
def test_float32_log10():
    assert math.log10(0.1f32) == -1.0f32
    assert math.log10(1.0f32) == 0.0f32
    assert math.log10(10.0f32) == 1.0f32
    assert math.log10(10000.0f32) == 4.0f32


@test
def test_float32_degrees():
    assert math.degrees(math.pi32) == 180.0f32
    assert math.degrees(math.pi32 / 2.0f32) == 90.0f32
    assert math.degrees(-math.pi32 / 4.0f32) == -45.0f32
    assert math.degrees(0.0f32) == 0.0f32


@test
def test_float32_radians():
    assert math.radians(180.0f32) == math.pi32
    assert math.radians(90.0f32) == math.pi32 / 2.0f32
    assert math.radians(-45.0f32) == -math.pi32 / 4.0f32
    assert math.radians(0.0f32) == 0.0f32


@test
def test_float32_sqrt():
    assert math.sqrt(4.0f32) == 2.0f32
    assert math.sqrt(0.0f32) == 0.0f32
    assert math.sqrt(1.0f32) == 1.0f32
    assert math.isnan(math.sqrt(-1.0f32)) == True


@test
def test_float32_pow():
    assert math.pow(0.0f32, 1.0f32) == 0.0f32
    assert math.pow(1.0f32, 0.0f32) == 1.0f32
    assert math.pow(2.0f32, 1.0f32) == 2.0f32
    assert math.pow(2.0f32, -1.0f32) == 0.5f32
    assert math.pow(-0.0f32, 3.0f32) == -0.0f32
    assert math.pow(-0.0f32, 2.3f32) == 0.0f32
    assert math.pow(-0.0f32, 0.0f32) == 1.0f32
    assert math.pow(-0.0f32, -0.0f32) == 1.0f32
    assert math.pow(-2.0f32, 2.0f32) == 4.0f32
    assert math.pow(-2.0f32, 3.0f32) == -8.0f32
    assert math.pow(-2.0f32, -3.0f32) == -0.125f32
    assert math.pow(INF32, 1.0f32) == INF32
    assert math.pow(NINF32, 1.0f32) == NINF32
    assert math.pow(1.0f32, INF32) == 1.0f32
    assert math.pow(1.0f32, NINF32) == 1.0f32
    assert math.isnan(math.pow(NAN32, 1.0f32)) == True
    assert math.isnan(math.pow(2.0f32, NAN32)) == True
    assert math.isnan(math.pow(0.0f32, NAN32)) == True
    assert math.pow(1.0f32, NAN32) == 1.0f32


@test
def test_float32_acos():
    assert close32(math.acos(-1.0f32), math.pi32)
    assert close32(math.acos(0.0f32), math.pi32 / 2.0f32)
    assert math.acos(1.0f32) == 0.0f32
    assert math.isnan(math.acos(NAN32)) == True


@test
def test_float32_asin():
    assert close32(math.asin(-1.0f32), -math.pi32 / 2.0f32)
    assert math.asin(0.0f32) == 0.0f32
    assert close32(math.asin(1.0f32), math.pi32 / 2.0f32)
    assert math.isnan(math.asin(NAN32)) == True


@test
def test_float32_atan():
    assert math.atan(-1.0f32) == -math.pi32 / 4.0f32
    assert math.atan(0.0f32) == 0.0f32
    assert close32(math.atan(1.0f32), math.pi32 / 4.0f32)
    assert close32(math.atan(INF32), math.pi32 / 2.0f32)
    assert close32(math.atan(NINF32), -math.pi32 / 2.0f32)
    assert math.isnan(math.atan(NAN32)) == True


@test
def test_float32_atan2():
    assert math.atan2(-1.0f32, 0.0f32) == -math.pi32 / 2.0f32
    assert math.atan2(-1.0f32, 1.0f32) == -math.pi32 / 4.0f32
    assert math.atan2(0.0f32, 1.0f32) == 0.0f32
    assert math.atan2(1.0f32, 1.0f32) == math.pi32 / 4.0f32
    assert math.atan2(1.0f32, 0.0f32) == math.pi32 / 2.0f32
    assert math.atan2(-0.0f32, 0.0f32) == -0.0f32
    assert math.atan2(-0.0f32, 2.3f32) == -0.0f32
    assert close32(math.atan2(0.0f32, -2.3f32), math.pi32)
    assert math.atan2(INF32, NINF32) == math.pi32 * 3.0f32 / 4.0f32
    assert math.atan2(INF32, 2.3f32) == math.pi32 / 2.0f32
    assert math.isnan(math.atan2(NAN32, 0.0f32)) == True


@test
def test_float32_cos():
    assert math.cos(0.0f32) == 1.0f32
    assert close32(math.cos(math.pi32 / 2.0f32), 6.123233995736766e-17f32)
    assert close32(math.cos(-math.pi32 / 2.0f32), 6.123233995736766e-17f32)
    assert math.cos(math.pi32) == -1.0f32
    assert math.isnan(math.cos(INF32)) == True
    assert math.isnan(math.cos(NINF32)) == True
    assert math.isnan(math.cos(NAN32)) == True


@test
def test_float32_sin():
    assert math.sin(0.0f32) == 0.0f32
    assert math.sin(math.pi32 / 2.0f32) == 1.0f32
    assert math.sin(-math.pi32 / 2.0f32) == -1.0f32
    assert math.isnan(math.sin(INF32)) == True
    assert math.isnan(math.sin(NINF32)) == True
    assert math.isnan(math.sin(NAN32)) == True


@test
def test_float32_hypot():
    assert math.hypot(12.0f32, 5.0f32) == 13.0f32
    assert math.hypot(12.0f32 / 32.0f32, 5.0f32 / 32.0f32) == 13.0f32 / 32.0f32
    assert math.hypot(0.0f32, 0.0f32) == 0.0f32
    assert math.hypot(-3.0f32, 4.0f32) == 5.0f32
    assert math.hypot(3.0f32, 4.0f32) == 5.0f32


@test
def test_float32_tan():
    assert math.tan(0.0f32) == 0.0f32
    assert close32(math.tan(math.pi32 / 4.0f32), 0.9999999999999999f32)
    assert close32(math.tan(-math.pi32 / 4.0f32), -0.9999999999999999f32)
    assert math.isnan(math.tan(INF32)) == True
    assert math.isnan(math.tan(NINF32)) == True
    assert math.isnan(math.tan(NAN32)) == True


@test
def test_float32_cosh():
    assert math.cosh(0.0f32) == 1.0f32
    assert close32(math.cosh(2.0f32) - 2.0f32 * math.cosh(1.0f32) ** 2.0f32, -1.0f32)
    assert math.cosh(INF32) == INF32
    assert math.cosh(NINF32) == INF32
    assert math.isnan(math.cosh(NAN32)) == True


@test
def test_float32_sinh():
    assert math.sinh(0.0f32) == 0.0f32
    assert math.sinh(1.0f32) + math.sinh(-1.0f32) == 0.0f32
    assert math.sinh(INF32) == INF32
    assert math.sinh(NINF32) == NINF32
    assert math.isnan(math.sinh(NAN32)) == True


@test
def test_float32_tanh():
    assert math.tanh(0.0f32) == 0.0f32
    assert math.tanh(1.0f32) + math.tanh(-1.0f32) == 0.0f32
    assert math.tanh(INF32) == 1.0f32
    assert math.tanh(NINF32) == -1.0f32
    assert math.isnan(math.tanh(NAN32)) == True


@test
def test_float32_acosh():
    assert math.acosh(1.0f32) == 0.0f32
    assert close32(math.acosh(2.0f32), 1.3169578969248166f32)
    assert math.acosh(INF32) == INF32
    assert math.isnan(math.acosh(NAN32)) == True
    assert math.isnan(math.acosh(-1.0f32)) == True


@test
def test_float32_asinh():
    assert math.asinh(0.0f32) == 0.0f32
    assert close32(math.asinh(1.0f32), 0.881373587019543f32)
    assert close32(math.asinh(-1.0f32), -0.881373587019543f32)
    assert math.asinh(INF32) == INF32
    assert math.isnan(math.asinh(NAN32)) == True
    assert math.asinh(NINF32) == NINF32


@test
def test_float32_atanh():
    assert math.atanh(0.0f32) == 0.0f32
    assert close32(math.atanh(0.5f32), 0.5493061443340549f32)
    assert close32(math.atanh(-0.5f32), -0.5493061443340549f32)
    assert math.isnan(math.atanh(INF32)) == True
    assert math.isnan(math.atanh(NAN32)) == True
    assert math.isnan(math.atanh(NINF32)) == True


@test
def test_float32_copysign():
    assert math.copysign(1.0f32, -0.0f32) == -1.0f32
    assert math.copysign(1.0f32, 42.0f32) == 1.0f32
    assert math.copysign(1.0f32, -42.0f32) == -1.0f32
    assert math.copysign(3.0f32, 0.0f32) == 3.0f32
    assert math.copysign(INF32, 0.0f32) == INF32
    assert math.copysign(INF32, -0.0f32) == NINF32
    assert math.copysign(NINF32, 0.0f32) == INF32
    assert math.copysign(NINF32, -0.0f32) == NINF32
    assert math.copysign(1.0f32, INF32) == 1.0f32
    assert math.copysign(1.0f32, NINF32) == -1.0f32
    assert math.copysign(INF32, INF32) == INF32
    assert math.copysign(INF32, NINF32) == NINF32
    assert math.copysign(NINF32, INF32) == INF32
    assert math.copysign(NINF32, NINF32) == NINF32
    assert math.isnan(math.copysign(NAN32, 1.0f32)) == True
    assert math.isnan(math.copysign(NAN32, INF32)) == True
    assert math.isnan(math.copysign(NAN32, NINF32)) == True
    assert math.isnan(math.copysign(NAN32, NAN32)) == True


@test
def test_float32_log1p():
    assert close32(math.log1p(2.0f32), 1.0986122886681098f32)
    assert close32(math.log1p(2.0f32 ** 90.0f32), 62.383246250395075f32)
    assert math.log1p(INF32) == INF32
    assert math.log1p(-1.0f32) == NINF32


@test
def test_float32_trunc():
    assert isinstance(math.trunc(1.0f32), int)
    assert math.trunc(1.0f32) == 1
    assert math.trunc(-1.0f32) == -1
    assert math.trunc(1.5f32) == 1
    assert math.trunc(-1.5f32) == -1
    assert math.trunc(1.99999f32) == 1
    assert math.trunc(-1.99999f32) == -1
    assert math.trunc(0.99999f32) == 0
    assert math.trunc(-100.999f32) == -100


@test
def test_float32_erf():
    assert close32(math.erf(1.0f32), 0.8427007929497148f32)
    assert math.erf(0.0f32) == 0.0f32
    assert close32(math.erf(3.0f32), 0.9999779095030015f32)
    assert math.erf(256.0f32) == 1.0f32
    assert math.erf(INF32) == 1.0f32
    assert math.erf(NINF32) == -1.0f32
    assert math.isnan(math.erf(NAN32)) == True


@test
def test_float32_erfc():
    assert math.erfc(0.0f32) == 1.0f32
    assert close32(math.erfc(1.0f32), 0.15729920705028516f32)
    assert close32(math.erfc(2.0f32), 0.0046777349810472645f32)
    assert close32(math.erfc(-1.0f32), 1.8427007929497148f32)
    assert math.erfc(INF32) == 0.0f32
    assert math.erfc(NINF32) == 2.0f32
    assert math.isnan(math.erfc(NAN32)) == True


@test
def test_float32_gamma():
    assert close32(math.gamma(6.0f32), 120.0f32)
    assert close32(math.gamma(1.0f32), 1.0f32)
    assert close32(math.gamma(2.0f32), 1.0f32)
    assert close32(math.gamma(3.0f32), 2.0f32)
    assert math.isnan(math.gamma(-1.0f32)) == True
    assert math.gamma(INF32) == INF32
    assert math.isnan(math.gamma(NINF32)) == True
    assert math.isnan(math.gamma(NAN32)) == True


@test
def test_float32_lgamma():
    assert math.lgamma(1.0f32) == 0.0f32
    assert math.lgamma(2.0f32) == 0.0f32
    assert math.lgamma(-1.0f32) == INF32
    assert math.lgamma(INF32) == INF32
    assert math.lgamma(NINF32) == INF32
    assert math.isnan(math.lgamma(NAN32)) == True


@test
def test_float32_remainder():
    assert math.remainder(2.0f32, 2.0f32) == 0.0f32
    assert math.remainder(-4.0f32, 1.0f32) == -0.0f32
    assert close32(math.remainder(-3.8f32, 1.0f32), 0.20000000000000018f32)
    assert close32(math.remainder(3.8f32, 1.0f32), -0.20000000000000018f32)
    assert math.isnan(math.remainder(INF32, 1.0f32)) == True
    assert math.isnan(math.remainder(NINF32, 1.0f32)) == True
    assert math.isnan(math.remainder(NAN32, 1.0f32)) == True


@test
def test_float32_frexp():
    assert math.frexp(-2.0f32) == (-0.5f32, 2)
    assert math.frexp(-1.0f32) == (-0.5f32, 1)
    assert math.frexp(0.0f32) == (0.0f32, 0)
    assert math.frexp(1.0f32) == (0.5f32, 1)
    assert math.frexp(2.0f32) == (0.5f32, 2)
    assert math.frexp(INF32)[0] == INF32
    assert math.frexp(NINF32)[0] == NINF32
    assert math.isnan(math.frexp(NAN32)[0]) == True


@test
def test_float32_modf():
    assert math.modf(1.5f32) == (0.5f32, 1.0f32)
    assert math.modf(-1.5f32) == (-0.5f32, -1.0f32)
    assert math.modf(math.inf32) == (0.0f32, INF32)
    assert math.modf(-math.inf32) == (-0.0f32, NINF32)
    modf_nan = math.modf(NAN32)
    assert math.isnan(modf_nan[0]) == True
    assert math.isnan(modf_nan[1]) == True


@test
def test_float32_isclose():
    assert math.isclose(1.0f32 + 1.0f32, 1.000000000001f32 + 1.0f32) == True
    assert math.isclose(2.90909324093284f32, 2.909093240932844234234234234f32) == True
    assert math.isclose(2.90909324093284f32, 2.9f32) == False
    assert math.isclose(2.90909324093284f32, 2.90909324f32) == True
    assert math.isclose(2.909094f32, 2.909095f32) == False
    assert math.isclose(NAN32, 2.9f32) == False
    assert math.isclose(2.9f32, NAN32) == False
    assert math.isclose(INF32, INF32) == True
    assert math.isclose(NINF32, NINF32) == True
    assert math.isclose(NINF32, INF32) == False
    assert math.isclose(INF32, NINF32) == False


test_float32_isnan()
test_float32_isinf()
test_float32_isfinite()
test_float32_ceil()
test_float32_floor()
test_float32_fabs()
test_float32_fmod()
test_float32_exp()
test_float32_expm1()
test_float32_ldexp()
test_float32_log()
test_float32_log2()
test_float32_log10()
test_float32_degrees()
test_float32_radians()
test_float32_sqrt()
test_float32_pow()
test_float32_acos()
test_float32_asin()
test_float32_atan()
test_float32_atan2()
test_float32_cos()
test_float32_sin()
test_float32_hypot()
test_float32_tan()
test_float32_cosh()
test_float32_sinh()
test_float32_tanh()
test_float32_acosh()
test_float32_asinh()
test_float32_atanh()
test_float32_copysign()
test_float32_log1p()
test_float32_trunc()
test_float32_erf()
test_float32_erfc()
test_float32_gamma()
test_float32_lgamma()
test_float32_remainder()
test_float32_frexp()
test_float32_modf()
test_float32_isclose()
