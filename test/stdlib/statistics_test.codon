import statistics
import math


@test
def med():
    # Test median with even nuber of int data points.
    data = [1, 2, 3, 4, 5, 6]
    assert statistics.median(data) == 3.5

    # Test median with an odd number of int data points.
    data = [1, 2, 3, 4, 5, 6, 9]
    assert statistics.median(data) == 4

    # Test median works with an odd number of Fractions.
    fdata = [1 / 7, 2 / 7, 3 / 7, 4 / 7, 5 / 7]
    assert statistics.median(fdata) == 3 / 7

    # Test median works with an even number of Fractions.
    fdata = [1 / 7, 2 / 7, 3 / 7, 4 / 7, 5 / 7, 6 / 7]
    assert statistics.median(fdata) == 1 / 2

    # Test median works with an odd number of Decimals.
    ddata = [2.5, 3.1, 4.2, 5.7, 5.8]
    assert statistics.median(ddata) == 4.2


med()


@test
def med_low():
    # Test median_low with an even number of ints.
    data = [1, 2, 3, 4, 5, 6]
    assert statistics.median_low(data) == 3

    # Test median_low works with an even number of Fractions.
    fdata = [1 / 7, 2 / 7, 3 / 7, 4 / 7, 5 / 7, 6 / 7]
    assert statistics.median_low(fdata) == 3 / 7

    # Test median_low works with an even number of Decimals.
    ddata = [1.1, 2.2, 3.3, 4.4, 5.5, 6.6]
    assert statistics.median_low(ddata) == 3.3


med_low()


@test
def med_high():
    # Test median_high with an even number of ints.
    data = [1, 2, 3, 4, 5, 6]
    assert statistics.median_high(data) == 4

    # Test median_high works with an even number of Fractions.
    fdata = [1 / 7, 2 / 7, 3 / 7, 4 / 7, 5 / 7, 6 / 7]
    assert statistics.median_high(fdata) == 4 / 7

    # Test median_high works with an even number of Decimals.
    ddata = [1.1, 2.2, 3.3, 4.4, 5.5, 6.6]
    assert statistics.median_high(ddata) == 4.4


med_high()


@test
def med_grouped():
    # Test median_grouped with repeated median values.
    data = [12, 13, 14, 14, 14, 15, 15]
    assert statistics.median_grouped(data) == 14

    data = [12, 13, 14, 14, 14, 14, 15]
    assert statistics.median_grouped(data) == 13.875

    data = [5, 10, 10, 15, 20, 20, 20, 20, 25, 25, 30]
    assert statistics.median_grouped(data, 5) == 19.375

    # Test median_grouped with repeated median values.
    data = [2, 3, 3, 4, 4, 4, 5, 5, 5, 5, 6, 6]
    assert statistics.median_grouped(data) == 4.5

    data = [3, 4, 4, 4, 5, 5, 5, 5, 6, 6]
    assert statistics.median_grouped(data) == 4.75

    # Test median_grouped with repeated single values.
    ddata = [3.2]
    assert statistics.median_grouped(ddata) == 3.2

    # Test median_grouped works with an odd number of Fractions.
    fdata = [5 / 4, 9 / 4, 13 / 4, 13 / 4, 17 / 4]
    assert statistics.median_grouped(fdata) == 3.0

    # Test median_grouped works with an even number of Fractions.
    fdata = [5 / 4, 9 / 4, 13 / 4, 13 / 4, 17 / 4, 17 / 4]
    assert statistics.median_grouped(fdata) == 3.25

    # Test median_grouped works with an odd number of Decimals.
    ddata = [5.5, 6.5, 6.5, 7.5, 8.5]
    assert statistics.median_grouped(ddata) == 6.75

    # Test median_grouped works with an even number of Decimals.
    ddata = [5.5, 5.5, 6.5, 6.5, 7.5, 8.5]
    assert statistics.median_grouped(ddata) == 6.5


med_grouped()


@test
def test_mode():
    data = [12, 13, 14, 14, 14, 15, 15]
    assert statistics.mode(data) == 14

    data = list(range(20, 50, 3))
    assert statistics.mode(data) == 20

    # Test mode with nominal data.
    ndata = ["a", "b", "c", "b", "d", "b"]
    assert statistics.mode(ndata) == "b"

    ndata = ["fe", "fi", "fo", "fum", "fi", "fi"]
    assert statistics.mode(ndata) == "fi"

    # Test mode with bimodal data.
    data = [1, 1, 2, 2, 2, 2, 3, 4, 5, 6, 6, 6, 6, 7, 8, 9, 9]
    assert statistics.mode(data) == 2

    # Test mode when data points are all unique.
    data = list(range(10))
    assert statistics.mode(data) == 0


test_mode()


@test
def test_multimode():
    data = [1, 1, 2, 2, 2, 2, 3, 4, 5, 6, 6, 6, 6, 7, 8, 9, 9]
    assert statistics.multimode(data) == [2, 6]

    ndata = ["a", "a", "b", "b", "b", "b", "b", "b", "b", "b", "c", "c"]
    assert statistics.multimode(ndata) == ["b"]

    ndata = [
        "a",
        "a",
        "b",
        "b",
        "b",
        "b",
        "c",
        "c",
        "d",
        "d",
        "d",
        "d",
        "e",
        "e",
        "f",
        "f",
        "f",
        "f",
        "g",
        "g",
    ]
    assert statistics.multimode(ndata) == ["b", "d", "f"]


test_multimode()


@test
def test_mode_parity():
    assert statistics.multimode([3, 1, 3, 1]) == [3, 1]
    assert statistics.multimode("baba") == ["b", "a"]
    assert statistics.mode(iter([3, 1, 1, 3])) == 3
    assert statistics.multimode(iter([3, 1, 1, 3])) == [3, 1]
    assert statistics.multimode(List[int]()) == []
    assert math.isnan(statistics.mode([math.nan, 0.0]))
    assert statistics.mode([math.nan, 1.0, 1.0]) == 1.0
    assert len(statistics.multimode([math.nan, 0.0])) == 2
    assert statistics.mode({"first": 1, "second": 100}) == "first"
    assert statistics.multimode({"first": 1, "second": 100}) == ["first", "second"]
    try:
        statistics.mode(List[int]())
        assert False
    except statistics.StatisticsError as error:
        assert isinstance(error, ValueError)
        assert str(error) == "no mode for empty data"


test_mode_parity()


@test
def test_quantiles():
    for n in range(2, 10):
        data = [10.0] * n
        assert statistics.quantiles(data) == [10.0, 10.0, 10.0]
        assert statistics.quantiles(data, method="inclusive") == [10.0, 10.0, 10.0]

    data = [100, 200, 400, 800]
    for n, expected in [
        (2, [300.0]),
        (3, [200.0, 400.0]),
        (4, [175.0, 300.0, 500.0]),
        (5, [160.0, 240.0, 360.0, 560.0]),
        (6, [150.0, 200.0, 300.0, 400.0, 600.0]),
        (8, [137.5, 175.0, 225.0, 300.0, 375.0, 500.0, 650.0]),
        (10, [130.0, 160.0, 190.0, 240.0, 300.0, 360.0, 440.0, 560.0, 680.0]),
        (12, [125.0, 150.0, 175.0, 200.0, 250.0, 300.0, 350.0, 400.0, 500.0, 600.0, 700.0]),
        (15, [120.0, 140.0, 160.0, 180.0, 200.0, 240.0, 280.0, 320.0, 360.0, 400.0, 480.0, 560.0, 640.0, 720.0])
    ]:
        assert statistics.quantiles(data, n=n, method="inclusive") == expected


test_quantiles()


@test
def test_order_statistics_parity():
    assert statistics.median(iter([1, 2, 3, 4])) == 2.5
    assert statistics.median_low(iter([4, 1, 3, 2])) == 2
    assert isinstance(statistics.median_low([1, 2]), int)
    assert isinstance(statistics.median_high([1, 2]), int)
    assert statistics.median_low("dcba") == "b"
    assert statistics.median_high("dcba") == "c"
    large = 9223372036854775807
    assert statistics.median([large, large]) == float(large)
    assert statistics.quantiles([large, large]) == [float(large)] * 3
    assert statistics.median_grouped([9007199254740993]) == 9007199254740992.0
    assert statistics.median_grouped(["1", "2", "3"], interval=" 2 ") == 2.0
    for text in ("", "invalid", "1tail"):
        try:
            statistics.median_grouped([text])
            assert False
        except TypeError as error:
            assert str(error) == "Value cannot be converted to a float"
        try:
            statistics.median_grouped([1], interval=text)
            assert False
        except TypeError as error:
            assert str(error) == "Value cannot be converted to a float"
    assert statistics.quantiles(iter([42])) == [42.0] * 3
    assert statistics.quantiles([42], n=1) == []
    for method in ("inclusive", "exclusive"):
        assert statistics.quantiles([1, 2], n=1, method=method) == []
        result = statistics.quantiles([1, 2], n=2, method=method)
        result.append(3.0)
        assert result == [1.5, 3.0]


test_order_statistics_parity()


@test
def test_mean():
    data = [100.0, 200.0, 400.0, 800.0]
    assert statistics.mean(data) == 375.0

    data = [17.25, 19.75, 20.0, 21.5, 21.75, 23.25, 25.125, 27.5]
    assert statistics.mean(data) == 22.015625

    data = [
        0.0,
        1.0,
        2.0,
        3.0,
        3.0,
        3.0,
        4.0,
        5.0,
        5.0,
        6.0,
        7.0,
        7.0,
        7.0,
        7.0,
        8.0,
        9.0,
    ]
    assert statistics.mean(data) == 4.8125


test_mean()


@test
def test_geometric_mean():
    PRECISION = 1e-6

    data = [54.0, 24.0, 36.0]
    assert math.fabs(statistics.geometric_mean(data) - 36) < PRECISION

    data = [4.0, 9.0]
    assert math.fabs(statistics.geometric_mean(data) - 6) < PRECISION

    data = [17.625]
    assert math.fabs(statistics.geometric_mean(data) - 17.625) < PRECISION

    data = [3.5, 4.0, 5.25]
    assert math.fabs(statistics.geometric_mean(data) - 4.18886) < PRECISION


test_geometric_mean()


@test
def test_harmonic_mean():
    data = [1.0, 0.0, 2.0]
    assert statistics.harmonic_mean(data) == 0

    data = [2.0, 4.0, 4.0, 8.0, 16.0, 16.0]
    assert statistics.harmonic_mean(data) == 6 * 4 / 5

    data = [1 / 8, 1 / 4, 1 / 4, 1 / 2, 1 / 2]
    assert statistics.harmonic_mean(data) == 1 / 4

    for x in range(1, 101):
        assert statistics.harmonic_mean([float(x)]) == float(x)


test_harmonic_mean()


@test
def test_means_parity():
    assert statistics.mean(range(5)) == 2.0
    assert statistics.mean(iter([1, 2, 3])) == 2.0
    assert statistics.mean({1: 100, 2: 200}) == 1.5
    assert statistics.mean([9007199254740993, -9007199254740992]) == 0.5
    assert statistics.mean([1e16, 1.0, -1e16]) == 1.0 / 3.0
    assert statistics.mean([1e308, 1e308]) == 1e308
    assert statistics.mean([math.inf, 1.0]) == math.inf
    assert math.isnan(statistics.mean([math.inf, -math.inf]))
    assert statistics.fmean(iter([1, 2, 3])) == 2.0
    assert statistics.fmean([10, 20], weights=[1, 3]) == 17.5
    assert statistics.fmean([10, 20], weights=[-1, 2]) == 30.0
    assert statistics.harmonic_mean(iter([40, 60]), weights=iter([5, 30])) == 56.0
    assert statistics.harmonic_mean([0, 10], weights=[0, 1]) == 10.0
    assert statistics.harmonic_mean([0, -1]) == 0.0
    assert statistics.harmonic_mean([0, -1], weights=[1, 1]) == 0.0
    assert statistics.harmonic_mean([1e-308, 1e-308, 0.0]) == 0.0
    assert statistics.harmonic_mean([1e-308, 1e-308, 0.0], weights=[1, 1, 1]) == 0.0
    assert statistics.harmonic_mean([math.nan, 0.0]) == 0.0
    assert statistics.harmonic_mean([math.inf, 2.0]) == 4.0
    assert statistics.geometric_mean(iter([0, 4])) == 0.0
    assert math.isnan(statistics.geometric_mean([0.0, math.inf]))
    assert math.isnan(statistics.geometric_mean([0.0, math.nan]))
    for data in ([-1.0], [1.0, -1.0], [-1.0, 0.0]):
        try:
            statistics.harmonic_mean(data)
            assert False
        except statistics.StatisticsError:
            pass
    try:
        statistics.geometric_mean([0, -1])
        assert False
    except statistics.StatisticsError:
        pass
    for weights in ([0, 0], [1]):
        try:
            statistics.fmean([1, 2], weights=weights)
            assert False
        except statistics.StatisticsError:
            pass


test_means_parity()


@test
def test_pvariance():
    data = [float(i) for i in range(10000)]
    assert statistics.pvariance(data) == (10000 ** 2 - 1) / 12

    data = [4.0, 7.0, 13.0, 16.0]
    assert statistics.pvariance(data) == 22.5

    data = [1 / 4, 1 / 4, 3 / 4, 7 / 4]
    assert statistics.pvariance(data) == 3 / 8


test_pvariance()


@test
def test_pstdev():
    data = [float(i) for i in range(10000)]
    assert statistics.pstdev(data) == math.sqrt(statistics.pvariance(data))

    data = [4.0, 7.0, 13.0, 16.0]
    assert statistics.pstdev(data) == math.sqrt(statistics.pvariance(data))

    data = [1 / 4, 1 / 4, 3 / 4, 7 / 4]
    assert statistics.pstdev(data) == math.sqrt(statistics.pvariance(data))


test_pstdev()


@test
def test_variance():
    data = [4.0, 7.0, 13.0, 16.0]
    assert statistics.variance(data) == 30.0

    data = [1 / 4, 1 / 4, 3 / 4, 7 / 4]
    assert statistics.variance(data) == 1 / 2


test_variance()


@test
def test_stdev():
    data = [4.0, 7.0, 13.0, 16.0]
    assert statistics.stdev(data) == math.sqrt(statistics.variance(data))

    data = [1 / 4, 1 / 4, 3 / 4, 7 / 4]
    assert statistics.stdev(data) == math.sqrt(statistics.variance(data))


test_stdev()


@test
def test_dispersion_parity():
    assert statistics.pvariance(iter([1, 2, 3]), mu=0) == 14.0 / 3.0
    assert statistics.pvariance({1: 100, 2: 200}) == 0.25
    assert statistics.variance(iter([1, 2, 3]), xbar=0) == 7.0
    assert statistics.pstdev([1, 2, 3], mu=0) == math.sqrt(14.0 / 3.0)
    assert statistics.stdev([1, 2, 3], xbar=0) == math.sqrt(7.0)
    assert statistics.pvariance([9007199254740992, 9007199254740993]) == 0.25
    assert statistics.variance([9223372036854775806, 9223372036854775807]) == 0.5
    assert statistics.pstdev([1e308, -1e308]) == 1e308
    assert statistics.pvariance([1e154, 1e154], mu=0.0) == 1e308
    assert statistics.pstdev([1e154, 1e154], mu=0.0) == 1e154
    assert math.isclose(statistics.variance([1e154] * 3, xbar=0.0), 1.5e308)
    assert statistics.pstdev([1e308, -1e308], mu=0.0) == 1e308
    assert statistics.pstdev([1e-300, -1e-300], mu=0.0) == 1e-300
    assert math.isclose(statistics.pvariance([1e-160, -1e-160], mu=0.0), 1e-320, abs_tol=5e-324)
    assert math.isclose(statistics.stdev([1e-300, -1e-300]), math.sqrt(2.0) * 1e-300)
    assert statistics.pvariance([math.inf, 1.0]) == math.inf
    assert statistics.variance([1e308, -1e308], xbar=1.0) == math.inf
    assert statistics.pvariance([1.0, 2.0], mu=math.inf) == math.inf
    assert math.isnan(statistics.pvariance([1.0, 2.0], mu=math.nan))
    assert math.isnan(statistics.pvariance([math.inf, -math.inf]))
    try:
        statistics.pvariance([1e308, -1e308])
        assert False
    except OverflowError:
        pass
    try:
        statistics.pstdev([math.inf])
        assert False
    except ValueError as error:
        assert str(error) == "inf or nan encountered in data"
    try:
        statistics.stdev([1])
        assert False
    except statistics.StatisticsError as error:
        assert str(error) == "stdev requires at least two data points"


test_dispersion_parity()


@test
def test_bivariate_statistics():
    assert statistics.covariance(range(1, 10), [1, 2, 3] * 3) == 0.75
    assert statistics.covariance(range(1, 10), range(9, 0, -1)) == -7.5
    assert statistics.covariance([1e308, -1e308], [16.0, 18.0]) == -math.inf
    assert statistics.correlation([1e308, -1e308], [16.0, 18.0]) == -1.0
    assert math.isclose(statistics.fmean([1e308, -1e308], weights=[3.0, 19.0]), -1e308 * (16.0 / 22.0))
    assert statistics.correlation(range(1, 10), range(9, 0, -1)) == -1.0
    assert statistics.correlation([1, 2, 3], [1, 2, 3]) == 1.0
    assert math.isclose(statistics.correlation([1, 1, 2], [1, 2, 3], method="ranked"), math.sqrt(0.75))
    assert statistics.correlation(["low", "middle", "high"], [2, 3, 1], method="ranked") == 1.0
    result = statistics.linear_regression(iter([1, 2, 3]), iter([5, 8, 11]))
    assert isinstance(result, statistics.LinearRegression)
    assert len(result) == 2
    assert result[0] == 3.0 and result[1] == 2.0
    assert result.slope == 3.0 and result.intercept == 2.0
    slope, intercept = result
    assert slope == 3.0 and intercept == 2.0
    result = statistics.linear_regression([1, 2, 3], [3, 6, 9], proportional=True)
    assert result.slope == 3.0 and result.intercept == 0.0
    assert statistics.fmean([1e16, 1e16], weights=[1e16, -1e16 + 2.0]) == 1e16
    for left, right in (([1], [2]), ([1, 2], [3]), ([1, 1], [2, 3])):
        try:
            statistics.correlation(left, right)
            assert False
        except statistics.StatisticsError:
            pass
    try:
        statistics.linear_regression([1, 1], [2, 3])
        assert False
    except statistics.StatisticsError as error:
        assert str(error) == "x is constant"
    try:
        statistics.correlation([1, 2], [3, 4], method="unknown")
        assert False
    except ValueError:
        pass


test_bivariate_statistics()

@test
def test_integer_relational_precision():
    for origin in (2 ** 62, -(2 ** 62), 9223372036854775804, -9223372036854775808):
        values = [origin + 1, origin + 2, origin + 3]
        assert statistics.covariance(iter(values), iter(values)) == 1.0
        assert statistics.covariance(values, [2, 4, 6]) == 2.0
        assert statistics.correlation(values, values) == 1.0
        assert statistics.correlation(values, [6, 4, 2]) == -1.0
        assert statistics.correlation(values, [2.0, 4.0, 6.0]) == 1.0
        result = statistics.linear_regression(iter(values), iter(values))
        assert result.slope == 1.0 and result.intercept == 0.0
    for origin in (2 ** 60 + 1, -(2 ** 60) + 1):
        left = [origin + offset for offset in range(4)]
        right = [3 * value + 5 for value in left]
        result = statistics.linear_regression(left, right)
        assert result.slope == 3.0 and result.intercept == 5.0
        proportional = statistics.linear_regression(left, [3 * value for value in left], proportional=True)
        assert proportional.slope == 3.0 and proportional.intercept == 0.0
    try:
        statistics.correlation([2 ** 62] * 3, [1, 2, 3])
        assert False
    except statistics.StatisticsError:
        pass


test_integer_relational_precision()

@test
def test_sumprod_overflow_recovery():
    for values in ([1e308, 1e308, -1e308, -1e308],
                   [-1e308, -1e308, 1e308, 1e308]):
        assert statistics._sumprod(iter(values), iter([1.0] * 4)) == 0.0
        assert statistics._sumprod(iter(values), iter([2.0] * 4)) == 0.0
        assert statistics.fmean(values, weights=[1.0] * 4) == 0.0
        assert statistics._sumprod(iter(values + [1e-300]), iter([1.0] * 5)) == 1e-300
        assert statistics._sumprod(iter([1e-300] + values), iter([1.0] * 5)) == 1e-300
    assert statistics.fmean([1e308, 1e308], weights=[1.0, 1.0]) == 1e308
    assert statistics.fmean([1e308, 1e308], weights=[-1.0, -1.0]) == 1e308
    assert statistics._sumprod(iter([1e308, -1e308]), iter([1e308, 1e308])) == 0.0
    assert statistics._sumprod(iter([1e308, -1e308, 5e-324]), iter([1e308, 1e308, 1.0])) == 5e-324
    for values in ([1e154, -1e154], [1e154, 1e154, -1e154, -1e154],
                   [1e308, -1e308], [1e-300, -1e-300]):
        assert statistics.correlation(values, values) == 1.0
        result = statistics.linear_regression(values, values)
        assert result.slope == 1.0 and result.intercept == 0.0
    assert math.isclose(statistics.covariance([1e154] * 2 + [-1e154] * 2,
                                            [1e154] * 2 + [-1e154] * 2), 1e308 * (4.0 / 3.0))
    result = statistics.linear_regression([1e308, -1e308], [16.0, 18.0])
    assert math.isclose(result.slope, -1e-308, rel_tol=1e-14, abs_tol=0.0)
    assert result.intercept == 17.0
    assert statistics._sumprod(iter([1e308, 1e308]), iter([1.0, 1.0])) == math.inf
    for special in (math.inf, -math.inf, math.nan):
        for values in ([special, 1e308, 1e308], [1e308, 1e308, special]):
            result = statistics._sumprod(iter(values), iter([1.0] * 3))
            assert math.isnan(result) if math.isnan(special) else result == special
    assert statistics._sumprod(iter([math.inf, -1e308]), iter([1.0, 2.0])) == math.inf
    assert statistics._sumprod(iter([-math.inf, 1e308]), iter([1.0, 2.0])) == -math.inf
    assert math.isnan(statistics._sumprod(iter([math.inf, -math.inf]), iter([1.0, 1.0])))
    assert math.isnan(statistics._sumprod(iter([1e308, 1e308, math.inf]), iter([1.0, 1.0, 0.0])))


test_sumprod_overflow_recovery()


@test
def test_mean_NormalDist():
    X = statistics.NormalDist(10000.0, 3.0)
    assert X.mean == 10000.0


test_mean_NormalDist()


@test
def test_stdev():
    X = statistics.NormalDist(10000.0, 3.0)
    assert X.stdev == 3.0


test_stdev()


@test
def test_variance():
    X = statistics.NormalDist(10000.0, 3.0)
    assert X.variance == 9.0


test_variance()


@test
def test_pdf():
    PRECISION = 1e-6
    X = statistics.NormalDist(100.0, 15.0)

    # verify peak around center
    assert X.pdf(99.0) < X.pdf(100.0)
    assert X.pdf(101.0) < X.pdf(100.0)

    for i in range(50):
        assert (
            math.fabs((X.pdf(100.0 - float(i)) - X.pdf(100.0 + float(i)))) < PRECISION
        )


test_pdf()


@test
def test_cdf():
    X = statistics.NormalDist(100.0, 15.0)
    # Verify center (should be exact)
    assert X.cdf(100.0) == 0.50


test_cdf()


@test
def test_inv_cdf():
    PRECISION = 1e-6
    iq = statistics.NormalDist(100.0, 15.0)
    assert iq.inv_cdf(0.50) == iq.mean

    # One hundred ever smaller probabilities to test tails out to
    # extreme probabilities: 1 / 2**50 and (2**50-1) / 2 ** 50
    for e in range(1, 51):
        p = 2.0 ** (-e)
        assert math.fabs(iq.cdf(iq.inv_cdf(p)) - p) < PRECISION
        p = 1.0 - p
        assert math.fabs(iq.cdf(iq.inv_cdf(p)) - p) < PRECISION


test_inv_cdf()


@test
def test_ND_quartiles():
    PRECISION = 1e-6
    Z = statistics.NormalDist(0.0, 1.0)
    for n, expected in [
        (2, [0.0]),
        (3, [-0.430727, 0.430727]),
        (4, [-0.67449, 0.0, 0.67449]),
    ]:
        actual = Z.quantiles(n)
        for i in range(len(expected)):
            assert math.fabs(actual[i] - expected[i]) < PRECISION


test_ND_quartiles()


@test
def test_overlap():
    PRECISION = 1e-5
    for X1, X2, published_result in [
        (statistics.NormalDist(0.0, 2.0), statistics.NormalDist(1.0, 2.0), 0.80258),
        (statistics.NormalDist(0.0, 1.0), statistics.NormalDist(1.0, 2.0), 0.60993),
    ]:
        assert math.fabs(X1.overlap(X2) - published_result) < PRECISION
        assert math.fabs(X2.overlap(X1) - published_result) < PRECISION


test_overlap()


@test
def test_samples():
    mu, sigma = 10000.0, 3.0
    X = statistics.NormalDist(mu, sigma)
    n = 1000
    data = X.samples(n)
    assert len(data) == n


test_samples()


@test
def test_from_samples():
    data = [96.0, 107.0, 90.0, 92.0, 110.0]
    ND = statistics.NormalDist.from_samples(data)
    assert ND == statistics.NormalDist(99.0, 9.0)


test_from_samples()


@test
def test_normal_dist_parity():
    import random
    standard = statistics.NormalDist(sigma=1)
    assert standard == statistics.NormalDist(0, 1)
    assert standard != statistics.NormalDist(1, 1)
    assert statistics.NormalDist(1, 1) != standard
    assert standard != statistics.NormalDist(0, 2)
    assert 10 - statistics.NormalDist(2, 3) == statistics.NormalDist(8, 3)
    assert statistics.NormalDist(2, 3).zscore(8) == 2.0
    assert statistics.NormalDist(2, 0).inv_cdf(0.25) == 2.0
    assert math.isclose(standard.cdf(-10), 7.619853024160593e-24, rel_tol=1e-14)
    assert math.isclose(standard.overlap(statistics.NormalDist(20, 1)), 1.5239706048321186e-23, rel_tol=1e-14)
    expected = [0.35692707086842007, -1.9597799949641361, -0.5976722624865992]
    for actual, reference in zip(standard.samples(3, seed=42), expected):
        assert math.isclose(actual, reference, rel_tol=1e-14)
    random.seed(123)
    next_random = random.random()
    random.seed(123)
    standard.samples(3, seed=42)
    assert random.random() == next_random
    assert statistics.NormalDist.from_samples(iter([96, 107, 90, 92, 110])) == statistics.NormalDist(99, 9)
    try:
        statistics.NormalDist(1, 0).zscore(2)
        assert False
    except statistics.StatisticsError:
        pass
    assert statistics.NormalDist(2, 3) / -2.0 == statistics.NormalDist(-1, 1.5)
    for divisor in (0.0, -0.0):
        try:
            standard / divisor
            assert False
        except ZeroDivisionError:
            pass


test_normal_dist_parity()


@test
def test_kernel_density():
    kernels = ["normal", "gauss", "logistic", "sigmoid", "rectangular", "uniform",
               "triangular", "parabolic", "epanechnikov", "quartic", "biweight",
               "triweight", "cosine"]
    for kernel in kernels:
        density = statistics.kde([0.0], 1.0, kernel)
        cumulative = statistics.kde([0.0], 1.0, kernel, cumulative=True)
        assert cumulative(0.0) == 0.5
        assert math.isclose(cumulative(-0.25) + cumulative(0.25), 1.0)
        assert density(0.0) > 0.0
        assert density(-0.25) == density(0.25)
        assert 0.0 <= cumulative(-10.0) < cumulative(10.0) <= 1.0
        sampler = statistics.kde_random([0.0], 1.0, kernel, seed=42)
        repeat = statistics.kde_random([0.0], 1.0, kernel, seed=42)
        for index in range(10):
            value = sampler()
            assert value == repeat()
            assert math.isfinite(value)
    assert statistics.kde([0.0], 1.0, "uniform")(0.5) == 0.5
    assert statistics.kde([0.0], 1.0, "uniform")(1.5) == 0.0
    assert statistics.kde([0.0], 1.0, "triangular")(0.5) == 0.5
    assert statistics.kde([0.0], 1.0, "parabolic")(0.0) == 0.75
    assert statistics.kde([0.0], 1.0, "quartic")(0.0) == 15.0 / 16.0
    assert statistics.kde([0.0], 1.0, "triweight")(0.0) == 35.0 / 32.0
    for kernel in ("normal", "uniform"):
        data = [0.0]
        density = statistics.kde(data, 1.0, kernel)
        original = density(0.0)
        data.append(3.0)
        assert density(0.0) < original
        data.clear()
        try:
            density(0.0)
            assert False
        except ZeroDivisionError:
            pass
        data.extend([0.0, 0.0])
        assert density(0.0) == original
    def estimators(center: float, kernel: str):
        data = [center]
        return statistics.kde(data, 1.0, kernel), statistics.kde_random(data, 1.0, kernel, seed=42)
    for kernel in ("normal", "triangular"):
        first_density, first_sampler = estimators(1.0, kernel)
        second_density, second_sampler = estimators(10.0, kernel)
        assert first_density(1.0) == second_density(10.0)
        assert first_density(1.0) > first_density(10.0)
        for index in range(10):
            assert math.isclose(second_sampler() - first_sampler(), 9.0)
    for data, bandwidth, kernel in ((List[float](), 1.0, "normal"),
                                     ([1.0], 0.0, "normal"),
                                     ([1.0], 1.0, "unknown")):
        try:
            statistics.kde(data, bandwidth, kernel)
            assert False
        except statistics.StatisticsError:
            pass


test_kernel_density()


@test
def test_statistics_errors():
    for function, message in (
        (statistics.mean, "mean requires at least one data point"),
        (statistics.fmean, "fmean requires at least one data point"),
        (statistics.pvariance, "pvariance requires at least one data point"),
        (statistics.pstdev, "pstdev requires at least one data point"),
        (statistics.variance, "variance requires at least two data points"),
        (statistics.stdev, "stdev requires at least two data points"),
    ):
        try:
            function(List[float]())
            assert False
        except statistics.StatisticsError as error:
            assert str(error) == message
    for weights in ([-1, 2], [0, 0], [1]):
        try:
            statistics.harmonic_mean([2, 4], weights=weights)
            assert False
        except statistics.StatisticsError:
            pass
    for data, weights in (([-1, 0], [0, 1]), ([0, 1], [1, -1])):
        try:
            statistics.harmonic_mean(data, weights=weights)
            assert False
        except statistics.StatisticsError:
            pass
    for function in (statistics.fmean, statistics.geometric_mean):
        try:
            function(iter(List[float]()))
            assert False
        except statistics.StatisticsError:
            pass
    for count in (0, -1):
        try:
            statistics.quantiles([1, 2], n=count)
            assert False
        except statistics.StatisticsError:
            pass
    try:
        statistics.quantiles([1, 2], method="unknown")
        assert False
    except ValueError as error:
        assert str(error) == "Unknown method: 'unknown'"
    for data in (List[float](), [1.0]):
        try:
            statistics.NormalDist.from_samples(iter(data))
            assert False
        except statistics.StatisticsError as error:
            assert str(error) == "stdev requires at least two data points"


test_statistics_errors()
