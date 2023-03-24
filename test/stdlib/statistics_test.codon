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
