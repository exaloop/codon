"""Export completed, validated benchmark reports for the documentation site."""

import argparse
import ast
import hashlib
import json
import math
import re
import shutil
import statistics
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
TITLES = {
    "binary_trees": "Binary trees", "chaos": "Chaos fractal", "fannkuch": "Fannkuch",
    "float": "Floating-point objects", "go": "Go simulation", "mandelbrot": "Mandelbrot",
    "nbody": "N-body simulation", "set_partition": "Set partitions",
    "spectral_norm": "Spectral norm", "primes": "Prime counting",
    "sum": "Integer summation", "taq": "Financial tick analysis", "word_count": "Word count",
}
LABELS = {"codon": "Codon", "python": "CPython", "pypy": "PyPy", "cpp": "Clang C++", "numba": "Numba"}


def sha256(filename):
    with filename.open("rb") as stream:
        return hashlib.file_digest(stream, "sha256").hexdigest()


def write_json(filename, value):
    filename.write_text(json.dumps(value, indent=2, allow_nan=False) + "\n")


def redact(value, work):
    if isinstance(value, dict):
        return {redact(key, work): redact(item, work) for key, item in value.items()}
    if isinstance(value, list):
        return [redact(item, work) for item in value]
    if isinstance(value, str):
        return value.replace(str(work), "$BENCH_WORK").replace(str(ROOT), "$CODON_ROOT").replace(str(Path.home()), "$HOME")
    return value


def focused_source(text, name):
    matches = list(re.finditer(r"^def ([A-Za-z_]\w*)\(", text, re.MULTILINE))
    blocks = {}
    for index, match in enumerate(matches):
        end = matches[index + 1].start() if index + 1 < len(matches) else len(text)
        blocks[match.group(1)] = text[match.start():end].rstrip()
    if name not in blocks:
        return text
    needed = {name}
    pending = [name]
    while pending:
        current = pending.pop()
        for called in re.findall(r"(?<![.\w])([A-Za-z_]\w*)\(", blocks[current]):
            if called in blocks and called not in needed:
                needed.add(called)
                pending.append(called)
    prefix = text[:matches[0].start()].rstrip()
    return prefix + "\n\n" + "\n\n".join(block for key, block in blocks.items() if key in needed) + "\n"


def parallel_threads(text, default):
    counts = []
    for line in text.splitlines():
        if line.strip().startswith("@par("):
            call = ast.parse(line.strip()[1:], mode="eval").body
            options = {keyword.arg: ast.literal_eval(keyword.value) for keyword in call.keywords}
            if not options.get("gpu", False):
                counts.append(options.get("num_threads", default))
    return max(counts) if counts else None


def export(work, output):
    reports = {}
    for suite, directory, count in (("applications", "applications-final", 13), ("npbench", "npbench-final", 53)):
        report = json.loads((work / directory / "report.json").read_text())
        assert report["status"] == "complete", f"{suite} is not complete"
        assert len(report["cases"]) == count
        assert report["samples"] >= 3
        assert not report.get("quick", False)
        assert suite != "npbench" or report["preset"] == "L"
        for filename, expected in report["environment"]["fingerprints"].items():
            assert sha256(ROOT / filename) == expected, filename
        reports[suite] = report
    assert reports["applications"]["environment"] == reports["npbench"]["environment"], "Different measurement environments"
    output.mkdir(parents=True, exist_ok=True)
    source_directory = output / "source"
    source_directory.mkdir(exist_ok=True)
    entries = []
    for suite, report in reports.items():
        for case in report["cases"]:
            identifier = f"{suite}-{case['id']}"
            entry = dict(id=identifier, name=case["id"], suite=suite,
                         title=case.get("title", TITLES.get(case["id"], case["id"])),
                         parameters=case.get("parameters", {}), arguments=case.get("arguments", []),
                         preset=case.get("preset", "Full"), policy=report["policy"], results={})
            for backend, result in case.get("results", {}).items():
                published = {key: value for key, value in result.items() if key in (
                    "status", "reason", "phase", "samples", "median", "validation", "gc",
                    "memoryAudit", "runtime", "adaptations", "compileCommand", "timeoutSeconds")}
                published["label"] = LABELS[backend]
                if result["status"] == "ok":
                    assert result["validation"]["status"] == "passed", identifier
                if result["status"] in ("ok", "invalid", "memory-review") and len(result.get("samples", [])) == report["samples"]:
                    assert len(result["samples"]) == report["samples"], identifier
                    assert all(math.isfinite(sample) and sample > 0 for sample in result["samples"])
                    median = statistics.median(result["samples"])
                    if "median" in result:
                        assert result["median"] == median
                    published.update(median=median, minimum=min(result["samples"]), maximum=max(result["samples"]))
                    if result["status"] == "invalid":
                        entry["outputNote"] = ("Output differs between runtimes. Floating-point rounding and the order of operations can accumulate "
                                               "in iterative calculations and change the final output.")
                else:
                    published.pop("median", None)
                source = Path(result["source"])
                assert sha256(source) == result["sourceSha256"]
                if "executableSha256" in result:
                    assert sha256(Path(result["command"][0])) == result["executableSha256"]
                code = Path(result.get("code", result["source"]))
                expected_code = result.get("codeSha256", result["sourceSha256"])
                assert sha256(code) == expected_code
                filename = f"{identifier}-{backend}{code.suffix}"
                text = code.read_text()
                if suite == "applications" and backend == "codon":
                    threads = parallel_threads(text, report["environment"]["threads"])
                    if threads is not None:
                        published["parallelThreads"] = threads
                        entry["threadingNote"] = (f"Codon uses {threads} OpenMP threads (explicit in the source); other runtimes use one."
                                                  if threads > 1 else "Codon's parallel loop is configured with one OpenMP thread in this run.")
                download = source_directory / filename
                download.write_text(text if text.endswith("\n") else text + "\n")
                published["sourceUrl"] = f"/benchmarks/source/{filename}"
                published["sourceSha256"] = sha256(download)
                published["executedSourceSha256"] = expected_code
                if not text.endswith("\n"):
                    published.setdefault("adaptations", []).append("The downloadable source adds a final newline; its original executed-source hash is retained in the report.")
                published["language"] = "cpp" if backend == "cpp" else "python"
                if suite == "npbench" and backend == "codon":
                    excerpt = focused_source(text, case["id"])
                    excerpt_name = f"{identifier}-{backend}-excerpt.codon"
                    (source_directory / excerpt_name).write_text(excerpt)
                    published["displaySourceUrl"] = f"/benchmarks/source/{excerpt_name}"
                if suite == "npbench":
                    worker_name = f"{identifier}-{backend}-worker{source.suffix}"
                    (source_directory / worker_name).write_text(redact(source.read_text(), work))
                    published["workerUrl"] = f"/benchmarks/source/{worker_name}"
                    published["workerSha256"] = sha256(source_directory / worker_name)
                    published["executedWorkerSha256"] = result["sourceSha256"]
                entry["results"][backend] = published
            if "reason" in case:
                entry["reason"] = case["reason"]
            entry["inputs"] = case.get("inputs", {})
            entries.append(entry)
    environment = reports["applications"]["environment"]
    payload = dict(schemaVersion=1, date=environment["date"], environment=environment,
                   datasets=reports["applications"]["datasets"], cases=entries,
                   sampleCount=reports["applications"]["samples"], processTimeoutSeconds=900,
                   versions={key: environment[key] for key in ("codon", "python", "pypy", "clang", "numpy", "numba", "scipy")})
    write_json(output / "results.json", redact(payload, work))
    write_json(output / "raw-results.json", redact(reports, work))
    shutil.copyfile(work / "npbench" / "LICENSE", output / "NPBench-LICENSE.txt")
    shutil.copyfile(ROOT / "bench" / "README.md", output / "reproduction.md")
    print(f"Exported {len(entries)} workloads to {output}")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--work", type=Path, default=ROOT / "build" / "benchmarks-20261106")
    parser.add_argument("--output", type=Path, default=ROOT / "docs" / "data" / "benchmarks")
    args = parser.parse_args()
    export(args.work.absolute(), args.output.absolute())


if __name__ == "__main__":
    main()
