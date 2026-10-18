import argparse
import fnmatch
import os
from pathlib import Path
import subprocess
import sys
import tempfile


SERIAL_TESTS = (
    "*transform_omp_codon*",
    "*stdlib_asyncio_test_codon*",
    "*stdlib_threading_test_codon*",
)
THREAD_LIMITS = (
    "OMP_NUM_THREADS", "OPENBLAS_NUM_THREADS", "MKL_NUM_THREADS",
    "VECLIB_MAXIMUM_THREADS", "NUMEXPR_NUM_THREADS",
)


def filter_listing(listing, serial):
    """Partition an already-filtered GTest listing without changing test names."""
    result = []
    group = ""
    emitted = False
    for line in listing.splitlines():
        if not line.strip():
            continue
        if not line[0].isspace():
            group = line
            emitted = False
            continue
        name = group.split("#", 1)[0].strip() + line.split("#", 1)[0].strip()
        if any(fnmatch.fnmatchcase(name, pattern) for pattern in SERIAL_TESTS) == serial:
            if not emitted:
                result.append(group)
                emitted = True
            result.append(line)
    return "\n".join(result) + "\n"


def phase_environment(environment, serial):
    """Limit nested library threads only in the concurrent test phase."""
    result = dict(environment, CODON_TEST_PHASE="serial" if serial else "parallel")
    if not serial:
        result.update({name: "1" for name in THREAD_LIMITS})
    return result


def run_worker(binary, arguments):
    """Give each test private writable paths while sharing read-only fixtures."""
    if "--gtest_list_tests" in arguments:
        listing = subprocess.check_output([str(binary), *arguments], text=True)
        print(filter_listing(listing, os.environ["CODON_TEST_PHASE"] == "serial"), end="")
        return 0

    workspace = Path(__file__).resolve().parents[2]
    with tempfile.TemporaryDirectory(prefix="codon-test-") as temporary:
        directory = Path(temporary)
        build = directory / "build"
        build.mkdir()
        for entry in binary.parent.iterdir():
            if entry.name in ("codon", "codon_test", "lib") or entry.name.startswith("libcodon"):
                (build / entry.name).symlink_to(entry)
        for name in ("test", "stdlib"):
            (directory / name).symlink_to(workspace / name, target_is_directory=True)
        for library in binary.parent.glob("libcodonrt.*"):
            (directory / library.name).symlink_to(library)
        return subprocess.run([str(binary), *arguments], cwd=directory).returncode


def main():
    parser = argparse.ArgumentParser(description="Run isolated Codon tests with gtest-parallel.")
    parser.add_argument("binary", type=Path)
    parser.add_argument("--gtest-parallel", required=True, type=Path)
    parser.add_argument("--output-dir", required=True, type=Path)
    parser.add_argument("--gtest-filter", default="*")
    parser.add_argument("--workers", type=int,
                        default=int(os.environ.get("CODON_TEST_WORKERS", min(4, os.cpu_count() or 1))))
    args = parser.parse_args()
    if args.workers < 1:
        parser.error("--workers must be positive")
    binary = args.binary.resolve(strict=True)
    runner = args.gtest_parallel.resolve(strict=True)
    output = args.output_dir.resolve()
    result = 0
    for serial in (False, True):
        phase = "serial" if serial else "parallel"
        directory = output / phase
        directory.mkdir(parents=True, exist_ok=True)
        command = [sys.executable, str(runner), sys.executable,
                   "--workers=" + str(1 if serial else args.workers),
                   "--gtest_filter=" + args.gtest_filter,
                   "--output_dir=" + str(directory),
                   "--dump_json_test_results=" + str(directory / "results.json"),
                   "--", str(Path(__file__).resolve()), "--worker", str(binary)]
        print("Running", phase, "tests with", 1 if serial else args.workers, "worker(s)", flush=True)
        completed = subprocess.run(command, env=phase_environment(os.environ, serial))
        result = result or completed.returncode
    return result


if __name__ == "__main__":
    if len(sys.argv) > 1 and sys.argv[1] == "--worker":
        sys.exit(run_worker(Path(sys.argv[2]), sys.argv[3:]))
    sys.exit(main())
