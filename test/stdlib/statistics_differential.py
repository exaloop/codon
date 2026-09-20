"""Compare native statistics with CPython 3.14 and exact numerical references."""

import argparse
from decimal import Decimal, localcontext
from fractions import Fraction
import json
import math
from pathlib import Path
import random
import statistics
import subprocess
import sys
import tempfile


PROBE = '''import statistics as s
import json
import sys

def evaluate(operation, data, other, parameter, method):
    if operation == "sumprod": return [s._sumprod(iter(data), iter(other))]
    if operation == "mean": return [s.mean(data)]
    if operation == "fmean": return [s.fmean(data)]
    if operation == "weighted_fmean": return [s.fmean(data, weights=other)]
    if operation == "geometric_mean": return [s.geometric_mean(data)]
    if operation == "harmonic_mean": return [s.harmonic_mean(data)]
    if operation == "weighted_harmonic": return [s.harmonic_mean(data, weights=other)]
    if operation == "median": return [s.median(data)]
    if operation == "median_low": return [float(s.median_low(data))]
    if operation == "median_high": return [float(s.median_high(data))]
    if operation == "median_grouped": return [s.median_grouped(data, parameter)]
    if operation == "mode": return [float(s.mode(data))]
    if operation == "multimode": return [float(value) for value in s.multimode(data)]
    if operation == "quantiles": return s.quantiles(data, n=int(parameter), method=method)
    if operation == "pvariance": return [s.pvariance(data)]
    if operation == "variance": return [s.variance(data)]
    if operation == "pstdev": return [s.pstdev(data)]
    if operation == "stdev": return [s.stdev(data)]
    if operation == "centered_variance": return [s.variance(data, parameter)]
    if operation == "centered_pstdev": return [s.pstdev(data, parameter)]
    if operation == "covariance": return [s.covariance(data, other)]
    if operation == "correlation": return [s.correlation(data, other, method=method)]
    if operation == "regression":
        result = s.linear_regression(data, other, proportional=bool(parameter))
        return [result.slope, result.intercept]
    if operation == "kde": return [s.kde(data, parameter, method)(point) for point in other]
    if operation == "kde_cdf": return [s.kde(data, parameter, method, cumulative=True)(point) for point in other]
    if operation == "kde_random":
        sample = s.kde_random(data, parameter, method, seed=42)
        return [sample() for index in range(10)]
    raise ValueError(operation)

for line in sys.stdin:
    print(line.strip(), file=sys.stderr, flush=True)
    request = json.loads(line)
    operation = request["op"].as_str()
    parameter = request["parameter"].as_float()
    method = request["method"].as_str()
    try:
        if request["integer"].as_bool():
            data = [value.as_int() for value in request["data"].values()]
            other = [value.as_int() for value in request["other"].values()]
            result = evaluate(operation, data, other, parameter, method)
        else:
            data = [value.as_float() for value in request["data"].values()]
            other = [value.as_float() for value in request["other"].values()]
            result = evaluate(operation, data, other, parameter, method)
        print(json.dumps(("ok", result)))
    except Exception as error:
        print(json.dumps(("error", error.typename)))
'''

BENCHMARK = '''import statistics as s
import time

def measure(name, function, data):
    function(data)
    timings = []
    checksum = 0.0
    for repeat in range(5):
        start = time.perf_counter()
        for iteration in range(5):
            checksum += float(function(data))
        timings.append((time.perf_counter() - start) / 5)
    print(name, len(data), min(timings), checksum)

for count in (2000, 20000):
    values = [float(index % 997) for index in range(count)]
    measure("mode", s.mode, values)
values = [float(index % 997) / 7 + 1 for index in range(200000)]
measure("mean", s.mean, values)
measure("harmonic_mean", s.harmonic_mean, values)
measure("pvariance", s.pvariance, values)
measure("stdev", s.stdev, values)
'''

NUMERICAL_BENCHMARK = '''
weights = [1.0 + float(index % 7) for index in range(len(values))]
other = [2.0 * value + float(index % 3) for index, value in enumerate(values)]
def weighted_mean(data):
    return s.fmean(data, weights=weights)
def covariance(data):
    return s.covariance(data, other)
def correlation(data):
    return s.correlation(data, other)
def regression(data):
    return s.linear_regression(data, other).slope
def centered_variance(data):
    return s.pvariance(data, 70.0)
measure("weighted_fmean", weighted_mean, values)
measure("covariance", covariance, values)
measure("correlation", correlation, values)
measure("regression", regression, values)
measure("centered_pvariance", centered_variance, values)
'''


def uses_exact_reference(request):
    if request.get("exact", False) or request["op"] == "sumprod":
        return True
    data, other = request["data"], request["other"]
    operation = request["op"]
    if not data or not all(math.isfinite(value) for value in data + other):
        return False
    if operation in ("centered_variance", "centered_pstdev", "weighted_fmean"):
        return max(abs(value) for value in data + other) >= 1e150
    if (operation == "regression" or operation == "correlation" and request["method"] == "linear"):
        magnitudes = [abs(value) for value in data + other if value]
        if magnitudes and (max(magnitudes) >= 1e150 or min(magnitudes) <= 1e-150):
            if not (operation == "regression" and request["parameter"]):
                try:
                    statistics.fmean(data)
                    statistics.fmean(other)
                except (ValueError, OverflowError):
                    return False
            return True
    return (request["integer"] and max(abs(value) for value in data + other) >= 2**53
            and (operation in ("covariance", "regression")
                 or operation == "correlation" and request["method"] == "linear"))


def rounded_fraction(value):
    try:
        return float(value)
    except OverflowError:
        return -math.inf if value < 0 else math.inf


def fraction_sqrt(value):
    with localcontext() as context:
        context.prec = 100
        return float((Decimal(value.numerator) / Decimal(value.denominator)).sqrt())


def exact_reference(request):
    data = list(map(Fraction, request["data"]))
    other = list(map(Fraction, request["other"]))
    operation, parameter = request["op"], request["parameter"]
    count = len(data)
    if operation in ("sumprod", "weighted_fmean"):
        result = sum(left * right for left, right in zip(data, other))
        if operation == "weighted_fmean":
            if not sum(other):
                raise statistics.StatisticsError("sum of weights must be non-zero")
            result /= sum(other)
        return [rounded_fraction(result)]
    if operation in ("centered_variance", "centered_pstdev"):
        correction = int(operation == "centered_variance")
        if count <= correction:
            raise statistics.StatisticsError("insufficient data")
        result = sum((value - Fraction(parameter)) ** 2 for value in data) / (count - correction)
        return [fraction_sqrt(result) if operation == "centered_pstdev" else rounded_fraction(result)]
    if count < 2 or len(other) != count:
        raise statistics.StatisticsError("insufficient data")
    proportional = operation == "regression" and bool(parameter)
    left_mean = Fraction(0) if proportional else sum(data) / count
    right_mean = Fraction(0) if proportional else sum(other) / count
    cross = sum((left - left_mean) * (right - right_mean) for left, right in zip(data, other))
    left_square = sum((value - left_mean) ** 2 for value in data)
    if operation == "covariance":
        return [rounded_fraction(cross / (count - 1))]
    if operation == "correlation":
        right_square = sum((value - right_mean) ** 2 for value in other)
        if not left_square or not right_square:
            raise statistics.StatisticsError("constant input")
        result = fraction_sqrt(cross * cross / (left_square * right_square))
        return [-result if cross < 0 else result]
    if not left_square:
        raise statistics.StatisticsError("constant input")
    slope = cross / left_square
    return [rounded_fraction(slope), 0.0 if proportional else rounded_fraction(right_mean - slope * left_mean)]


def reference(request):
    operation = request["op"]
    data, other = request["data"], request["other"]
    parameter, method = request["parameter"], request["method"]
    try:
        if uses_exact_reference(request):
            return ["ok", exact_reference(request)]
        if operation == "weighted_fmean":
            result = statistics.fmean(data, weights=other)
        elif operation == "weighted_harmonic":
            result = statistics.harmonic_mean(data, weights=other)
        elif operation == "quantiles":
            result = statistics.quantiles(data, n=int(parameter), method=method)
        elif operation == "median_grouped":
            result = statistics.median_grouped(data, parameter)
        elif operation == "centered_variance":
            result = statistics.variance(data, parameter)
        elif operation == "centered_pstdev":
            result = statistics.pstdev(data, parameter)
        elif operation == "covariance":
            result = statistics.covariance(data, other)
        elif operation == "correlation":
            result = statistics.correlation(data, other, method=method)
        elif operation == "regression":
            result = statistics.linear_regression(data, other, proportional=bool(parameter))
        elif operation in ("kde", "kde_cdf"):
            estimate = statistics.kde(data, parameter, method, cumulative=operation == "kde_cdf")
            result = [estimate(point) for point in other]
        elif operation == "kde_random":
            sample = statistics.kde_random(data, parameter, method, seed=42)
            result = [sample() for _ in range(10)]
        else:
            result = getattr(statistics, operation)(data)
        if not isinstance(result, (tuple, list)):
            result = [result]
        return ["ok", [float(value) for value in result]]
    except Exception as error:
        return ["error", type(error).__name__]


def requests(seed, cases):
    rng = random.Random(seed)
    datasets = [[], [0.0], [-1.0], [0.0, -1.0], [1e16, 1.0, -1e16],
                [1e308, 1e308], [1e308, -1e308], [1e-300, -1e-300],
                [math.inf], [math.inf, -math.inf], [math.inf, 0.0], [math.nan, 0.0],
                [2**53, 2**53 + 1], [2**63 - 2, 2**63 - 1],
                [-(2**63), 2**63 - 1], [2**53 + 1, -(2**53)],
                [1e308, 1e308, -1e308, -1e308, 1.0]]
    for _ in range(cases):
        count = rng.randrange(1, 50)
        scale = rng.choice([1.0, 1e-100, 1e100])
        offset = rng.choice([0.0, 1e12])
        datasets.append([offset + rng.uniform(-100, 100) * scale for _ in range(count)])
        datasets.append([rng.randrange(-100, 100) for _ in range(count)])
    for data in datasets:
        integer = bool(data) and isinstance(data[0], int)
        other = [rng.randrange(1, 20) for _ in data]
        if not integer:
            other = list(map(float, other))
        base = dict(data=data, other=other, parameter=1.0, method="linear", integer=integer)
        for operation in ("mean", "fmean", "geometric_mean", "harmonic_mean", "median",
                          "median_low", "median_high", "mode", "multimode", "median_grouped",
                          "pvariance", "variance", "pstdev", "stdev", "centered_variance",
                          "centered_pstdev", "covariance", "correlation", "regression",
                          "weighted_fmean", "weighted_harmonic"):
            yield dict(base, op=operation)
        for method in ("inclusive", "exclusive"):
            for count in (1, 4, 9):
                yield dict(base, op="quantiles", parameter=float(count), method=method)
        yield dict(base, op="correlation", method="ranked")
        yield dict(base, op="regression", parameter=0.0)
    for _ in range(cases):
        magnitude = rng.uniform(0.6, 0.9) * 1e308
        residual = rng.choice([5e-324, 1e-300, 1.0, 1e150])
        pairs = [(magnitude, 1.0), (magnitude, 2.0), (-magnitude, 1.0),
                 (-magnitude, 2.0), (residual, 1.0)]
        rng.shuffle(pairs)
        data, weights = map(list, zip(*pairs))
        for operation in ("sumprod", "weighted_fmean"):
            yield dict(op=operation, data=data, other=weights, parameter=1.0,
                       method="linear", integer=False, exact=True)
        origin = rng.choice([2**62, -(2**62), 2**63 - 1000, -(2**63) + 1000])
        offsets = [rng.randrange(100) for _ in range(10)]
        data = [origin + offset for offset in offsets]
        other = [origin + 3 * offset + 5 for offset in offsets]
        for operation in ("covariance", "correlation", "regression"):
            yield dict(op=operation, data=data, other=other, parameter=0.0,
                       method="linear", integer=True, exact=True)
        data = [rng.uniform(-1, 1) * rng.choice([1e154, 1e308, 1e-300]) for _ in range(10)]
        for operation in ("centered_variance", "centered_pstdev"):
            yield dict(op=operation, data=data, other=[], parameter=0.0,
                       method="linear", integer=False, exact=True)
    for kernel in ("normal", "gauss", "logistic", "sigmoid", "rectangular", "uniform",
                   "triangular", "parabolic", "epanechnikov", "quartic", "biweight",
                   "triweight", "cosine"):
        for bandwidth in (0.25, 1.0, 4.0):
            for operation in ("kde", "kde_cdf", "kde_random"):
                yield dict(op=operation, data=[-2.1, -1.3, -0.4, 1.9, 5.1, 6.2],
                           other=[-10.0, -2.0, 0.0, 0.25, 1.0, 3.0, 10.0],
                           parameter=bandwidth, method=kernel, integer=False)


def matches(expected, actual, probability=False, exact=False):
    if expected[0] != actual[0]:
        return False
    if expected[0] == "error":
        return expected == actual
    return len(expected[1]) == len(actual[1]) and all(
        (math.isnan(left) and math.isnan(right)) or left == right or
        math.isclose(left, right, rel_tol=2e-12, abs_tol=1e-14 if probability else 5e-324 if exact else 1e-300)
        for left, right in zip(expected[1], actual[1])
    )


def build(codon, directory, name, source, debug=False):
    source_path = directory / (name + ".codon")
    executable = directory / name
    source_path.write_text(source)
    subprocess.run([codon, "build", "-debug" if debug else "-release",
                    "-o", str(executable), str(source_path)], check=True)
    return executable


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--codon", default="./build/codon")
    parser.add_argument("--cases", type=int, default=100)
    parser.add_argument("--seed", type=int, default=1729)
    parser.add_argument("--debug", action="store_true")
    parser.add_argument("--benchmark", action="store_true")
    parser.add_argument("--baseline-ref", help="Git revision of the previous statistics module")
    args = parser.parse_args()
    if sys.version_info < (3, 14):
        parser.error("CPython 3.14 or newer is required")
    records = list(requests(args.seed, args.cases))
    expected = [reference(record) for record in records]
    with tempfile.TemporaryDirectory(prefix="codon-statistics-") as temporary:
        directory = Path(temporary)
        executable = build(args.codon, directory, "probe", PROBE, args.debug)
        try:
            result = subprocess.run([str(executable)], check=True, text=True, capture_output=True,
                                    input="".join(json.dumps(record) + "\n" for record in records),
                                    timeout=30)
        except subprocess.TimeoutExpired as error:
            print("Stalled input:", error.stderr.splitlines()[-1].decode(), flush=True)
            raise
        actual = [json.loads(line) for line in result.stdout.splitlines()]
        assert len(actual) == len(expected), (len(actual), len(expected), result.stderr)
        failures = [(record, wanted, got) for record, wanted, got in zip(records, expected, actual)
                    if not matches(wanted, got, record["op"] == "kde_cdf", uses_exact_reference(record))]
        for record, wanted, got in failures[:20]:
            oracle = "Exact" if uses_exact_reference(record) else "CPython"
            print(f"{record}\n{oracle}={wanted}\nCodon={got}", flush=True)
        exact_count = sum(uses_exact_reference(record) for record in records)
        print(f"{len(records)} comparisons ({exact_count} exact-reference), {len(failures)} failures "
              f"(seed {args.seed})", flush=True)
        if args.benchmark:
            source_path = directory / "benchmark.py"
            source_path.write_text(BENCHMARK + NUMERICAL_BENCHMARK)
            print("CPython: name, size, seconds/call, checksum", flush=True)
            subprocess.run([sys.executable, str(source_path)], check=True)
            print("Codon: name, size, seconds/call, checksum", flush=True)
            executable = build(args.codon, directory, "benchmark", BENCHMARK + NUMERICAL_BENCHMARK)
            subprocess.run([str(executable)], check=True)
            if args.baseline_ref:
                source = subprocess.check_output(
                    ["git", "show", f"{args.baseline_ref}:stdlib/statistics.codon"], text=True)
                (directory / "previous.codon").write_text(source)
                print(f"Codon baseline ({args.baseline_ref})", flush=True)
                executable = build(args.codon, directory, "baseline",
                                   BENCHMARK.replace("import statistics as s", "import previous as s"))
                subprocess.run([str(executable)], check=True)
        assert not failures


if __name__ == "__main__":
    main()