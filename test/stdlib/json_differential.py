"""Compare native JSON parsing and encoding with CPython."""

import argparse
import json
from pathlib import Path
import random
import subprocess
import tempfile


PROBE = '''import json
import sys

for line in sys.stdin:
    request = json.loads(line)
    source = request["source"].as_str()
    mode = request["mode"].as_int()
    try:
        value = json.loads(source)
        if mode == 0:
            output = json.dumps(value)
        elif mode == 1:
            output = json.dumps(value, ensure_ascii=False, sort_keys=True, separators=(",", ":"))
        elif mode == 2:
            output = json.dumps(value, indent=2, sort_keys=True)
        else:
            output = json.dumps(value, indent="\\t", separators=(", ", " : "))
        print(json.dumps(("ok", output)))
    except json.JSONDecodeError as error:
        print(json.dumps(("error", error.pos, error.lineno, error.colno)))
    except OverflowError:
        print('["overflow"]')
'''


def random_value(rng, depth=0):
    alphabet = "abc XYZ\n\t\x00\"\\" + "\u00e9\u4e2d\U0001f600\ud800"
    kind = rng.randrange(7 if depth < 4 else 5)
    if kind == 0:
        return None
    if kind == 1:
        return bool(rng.randrange(2))
    if kind == 2:
        return rng.randrange(-(2**63), 2**63)
    if kind == 3:
        return rng.uniform(-1e10, 1e10) * 10.0 ** rng.randrange(-280, 280)
    if kind == 4:
        return "".join(rng.choice(alphabet) for _ in range(rng.randrange(60)))
    if kind == 5:
        return [random_value(rng, depth + 1) for _ in range(rng.randrange(8))]
    return {
        "".join(rng.choice(alphabet) for _ in range(rng.randrange(10))): random_value(rng, depth + 1)
        for _ in range(rng.randrange(8))
    }


def reference(source, mode):
    try:
        value = json.loads(source)
    except json.JSONDecodeError as error:
        return ["error", error.pos, error.lineno, error.colno]
    options = ({},
               dict(ensure_ascii=False, sort_keys=True, separators=(",", ":")),
               dict(indent=2, sort_keys=True),
               dict(indent="\t", separators=(", ", " : ")))[mode]
    return ["ok", json.dumps(value, **options)]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--codon", default="./build/codon")
    parser.add_argument("--cases", type=int, default=2000)
    parser.add_argument("--seed", type=int, default=1729)
    parser.add_argument("--debug", action="store_true")
    args = parser.parse_args()
    rng = random.Random(args.seed)
    sources = [json.dumps(random_value(rng)) for _ in range(args.cases)]
    sources += [
        "NaN", "Infinity", "-Infinity", "-0.0", "1e400", "-1e400", "1e-400",
        "5e-324", "2.2250738585072014e-308", "1.7976931348623157e308",
        "9223372036854775807", "-9223372036854775808",
        '{"same":1,"same":2}', '"\\ud800\\ud800"', '"\\udc00\\ud800"',
        '"\\ud800\\u1234"', '"\\ud800\\udfff"', '"\\ud800\\u12zz"',
        '"\\u0000"', '"\\ud800"', '\ufeff{}',
        '"\\u00e9X\\u00e9', '"\\u1234',
        "", " ", "[", "{", "[1,]", '{"a":1,}', "01", "-01", "+1", "1.",
        "1e", "1e+", "1e-", "1.e2", "--1", "-", "[true false]", "true false",
        "[\n 1,\n bad]", '"unterminated', '"bad\\q"', '"bad\n"', '"\\u123z"',
    ]
    sources += ['"' + chr(code) + '"' for code in range(32)]
    for source in sources[:min(args.cases, 500)]:
        if source:
            offset = rng.randrange(len(source))
            sources.append(source[:offset] + source[offset + 1:])
    requests = [(source, mode) for source in sources for mode in range(4)]
    expected = [reference(source, mode) for source, mode in requests]
    with tempfile.TemporaryDirectory(prefix="codon-json-differential-") as temporary:
        source_path = Path(temporary) / "probe.codon"
        executable = Path(temporary) / "probe"
        source_path.write_text(PROBE)
        subprocess.run([args.codon, "build", "-debug" if args.debug else "-release",
                        "-o", str(executable), str(source_path)], check=True)
        result = subprocess.run(
            [str(executable)], check=True, text=True, capture_output=True,
            input="".join(json.dumps(dict(source=source, mode=mode)) + "\n"
                          for source, mode in requests),
        )
    actual = [json.loads(line) for line in result.stdout.splitlines()]
    assert len(actual) == len(expected), (len(actual), len(expected), result.stderr)
    failures = []
    for request, wanted, got in zip(requests, expected, actual):
        if wanted != got:
            failures.append((request, wanted, got))
    for request, wanted, got in failures[:12]:
        print(f"source={request!r}\nCPython={wanted!r}\nCodon={got!r}")
    print(f"{len(requests)} comparisons, {len(failures)} failures (seed {args.seed})")
    assert not failures


if __name__ == "__main__":
    main()
