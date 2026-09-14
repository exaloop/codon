"""Compare POSIX path resolution and access modes with the host CPython.

Legacy CPython stops resolving after a symlink loop, unlike modern CPython and
Codon. Exclude those incomplete oracle results; os_test.codon covers post-loop
resolution independently, and modern CPython checks the entire generated corpus.
"""

import argparse
import os
from pathlib import Path
import random
import subprocess
import sys
import tempfile


PROBE = '''import os
import os.path
import sys

for line in sys.stdin:
    value = line.rstrip("\\n")
    if sys.argv[1] == "realpath":
        print(os.path.realpath(value))
    else:
        print(os.access(sys.argv[2], int(value)))
'''


def realpath_cases(root, seed, graphs):
    randomizer = random.Random(seed)
    cases = ["", ".", "..", "/", "//", "///"]
    for graph in range(graphs):
        base = root / f"graph-{graph}"
        base.mkdir()
        (base / "dir" / "child").mkdir(parents=True)
        (base / "file").write_text("data")
        targets = {
            "self": "self",
            "cycle-a": "cycle-b",
            "cycle-b": "cycle-a",
            "up": "dir/child/..",
            "abs": str(base / "dir"),
            "double": "/" + str(base / "dir"),
            "dangling": "missing/child",
            "notdir": "file/child",
            "back": ".",
            "dir/relative": "../up",
        }
        names = [f"link-{index}" for index in range(24)]
        pool = list(targets) + names + ["dir", "missing", "file", "dir/child/.."]
        for name in names:
            target = randomizer.choice(pool)
            if randomizer.randrange(3) == 0:
                target = str(base / target)
            targets[name] = target
        for name, target in targets.items():
            os.symlink(target, base / name)
        for name in [*targets, "dir", "missing", "file"]:
            for suffix in ["", "/", "/child", "/..", "/../up", "/../../self"]:
                value = str(base / name) + suffix
                cases.extend([value, "/" + value, os.path.relpath(value)])
        cases.extend([
            str(base) + "/back/back/up/../up",
            str(base) + "/up/../up/../up",
            str(base) + "/self/../cycle-a/../dir",
        ])
    return cases


def compare(binary, operation, values, expected, extra=()):
    result = subprocess.run(
        [str(binary), operation, *extra],
        input="".join(value + "\n" for value in values),
        text=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
        check=True, timeout=120,
    )
    actual = result.stdout.splitlines()
    if result.stderr:
        raise AssertionError(result.stderr)
    if len(actual) != len(expected):
        raise AssertionError(f"{operation}: expected {len(expected)} rows, got {len(actual)}")
    failures = [
        f"{value!r}: CPython={wanted!r}, Codon={got!r}"
        for value, wanted, got in zip(values, expected, actual)
        if wanted != got
    ]
    if failures:
        raise AssertionError(f"{operation}: {len(failures)} mismatches\n" + "\n".join(failures[:20]))
    return len(values)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--codon", type=Path, default=Path("build/codon"))
    parser.add_argument("--seed", type=int, default=1729)
    parser.add_argument("--graphs", type=int, default=12)
    args = parser.parse_args()
    if os.name != "posix":
        parser.error("this differential suite requires POSIX")
    with tempfile.TemporaryDirectory(prefix="codon-os-differential-") as temporary:
        root = Path(temporary)
        source = root / "probe.codon"
        binary = root / "probe"
        source.write_text(PROBE)
        subprocess.run(
            [str(args.codon.resolve()), "build", "-release", "-o", str(binary), str(source)],
            check=True, timeout=120,
        )
        cases = realpath_cases(root, args.seed, args.graphs)
        legacy_resolver = getattr(os.path, "_joinrealpath", None)
        if legacy_resolver is not None:
            compatible = [
                value for value in cases
                if legacy_resolver("", value, False, {})[1]
            ]
            skipped = len(cases) - len(compatible)
            if skipped:
                print(
                    f"SKIP: {skipped} realpath cases where CPython "
                    f"{sys.version.split()[0]} stops at a symlink loop"
                )
            cases = compatible
        count = compare(binary, "realpath", cases, [os.path.realpath(value) for value in cases])
        modes = [-2147483648, -8, -1, *range(16), 255, 2147483647]
        access_count = 0
        for path in [root, source, root / "missing"]:
            access_count += compare(
                binary, "access", [str(mode) for mode in modes],
                [str(os.access(path, mode)) for mode in modes], [str(path)],
            )
        print(f"PASS: {count} realpath cases, {access_count} access cases (seed={args.seed})")


if __name__ == "__main__":
    main()
