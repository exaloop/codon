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
    "taq": "Financial tick analysis", "word_count": "Word count",
}
LABELS = {"codon": "Codon", "python": "CPython", "pypy": "PyPy", "cpp": "Clang C++", "numba": "Numba"}
PYPERFORMANCE = {"binary_trees", "chaos", "fannkuch", "float", "go", "nbody", "spectral_norm"}


def published_suite(suite, name):
    if suite == "applications":
        if name == "sum":
            return None
        if name in PYPERFORMANCE:
            return "pyperformance"
    return suite


def sha256(filename):
    with filename.open("rb") as stream:
        return hashlib.file_digest(stream, "sha256").hexdigest()


def write_json(filename, value):
    filename.write_text(json.dumps(value, indent=2, allow_nan=False) + "\n")


def deduplicate_sources(output, url_prefix="/benchmarks"):
    output = output.resolve()
    url_prefix = url_prefix.rstrip("/")
    assert url_prefix.startswith("/") and not url_prefix.startswith("//")
    datasets = []
    assets = {}
    references = []
    originals = set()
    for filename in sorted(output.rglob("results.json")):
        data = json.loads(filename.read_text())
        datasets.append((filename, data))
        for case in data["cases"]:
            for result in case["results"].values():
                for field, hash_field in (("sourceUrl", "sourceSha256"),
                                          ("workerUrl", "workerSha256"),
                                          ("displaySourceUrl", None)):
                    if field not in result:
                        continue
                    url = result[field]
                    assert url.startswith(url_prefix + "/"), url
                    source = (output / url[len(url_prefix) + 1:]).resolve()
                    assert source.is_relative_to(output) and source.parent.name == "source", url
                    content = source.read_bytes()
                    digest = hashlib.sha256(content).hexdigest()
                    if hash_field:
                        assert digest == result[hash_field], url
                    asset = assets.setdefault(digest, {"content": content, "suffixes": set()})
                    assert asset["content"] == content, "Source hash collision"
                    asset["suffixes"].add(source.suffix)
                    references.append((result, field, digest))
                    originals.add(source)
    assert datasets, f"No benchmark datasets in {output}"
    for source in output.rglob("source/*"):
        if source.is_file() and re.fullmatch(r"[0-9a-f]{64}", source.stem):
            assert source.resolve().is_relative_to(output), source
            assert sha256(source) == source.stem, source
            originals.add(source)
    destinations = {}
    for digest, asset in assets.items():
        suffix = ".py" if ".py" in asset["suffixes"] else min(asset["suffixes"])
        destination = output / "source" / f"{digest}{suffix}"
        if destination.exists():
            assert destination.read_bytes() == asset["content"], destination
        destinations[digest] = destination
    for digest, destination in destinations.items():
        destination.parent.mkdir(parents=True, exist_ok=True)
        if not destination.exists():
            destination.write_bytes(assets[digest]["content"])
    for result, field, digest in references:
        result[field] = f"{url_prefix}/source/{destinations[digest].name}"
    for filename, data in datasets:
        write_json(filename, data)
    redundant = originals - set(destinations.values())
    for source in redundant:
        source.unlink()
    print(f"Stored {len(references)} source references in {len(assets)} unique files; "
          f"removed {len(redundant)} old paths")


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


def comparable_environment(environment):
    return dict(environment, blas=sorted(environment["blas"], key=lambda library: json.dumps(library, sort_keys=True)))


def export(work, output, url_prefix="/benchmarks", report_file=None):
    url_prefix = url_prefix.rstrip("/")
    assert url_prefix.startswith("/") and not url_prefix.startswith("//")
    reports = {}
    inputs = (("npbench", report_file, None),) if report_file else (
        ("applications", work / "applications-final" / "report.json", 12),
        ("npbench", work / "npbench-final" / "report.json", 53))
    for suite, filename, count in inputs:
        report = json.loads(filename.read_text())
        assert report["status"] == "complete", f"{suite} is not complete"
        assert report["cases"]
        assert all(case["suite"] == suite for case in report["cases"])
        assert count is None or sum(published_suite(suite, case["id"]) is not None for case in report["cases"]) == count
        assert report["samples"] >= 3
        assert not report.get("quick", False)
        assert suite != "npbench" or report["preset"] == "L"
        for filename, expected in report["environment"]["fingerprints"].items():
            assert sha256(ROOT / filename) == expected, filename
        reports[suite] = report
    if "applications" in reports:
        assert comparable_environment(reports["applications"]["environment"]) == comparable_environment(reports["npbench"]["environment"]), "Different measurement environments"
    timeouts = {report.get("processTimeoutSeconds", 900) for report in reports.values()}
    assert len(timeouts) == 1, "Different process timeout limits"
    if report_file and (output / "results.json").exists():
        existing = json.loads((output / "results.json").read_text())
        assert {case["id"] for case in existing["cases"]} == {
            f"npbench-{case['id']}" for case in reports["npbench"]["cases"]
        }, "Export isolated reports into a separate staging directory"
    output.mkdir(parents=True, exist_ok=True)
    source_directory = output / "source"
    source_directory.mkdir(exist_ok=True)
    entries = []
    for suite, report in reports.items():
        for case in report["cases"]:
            group = published_suite(suite, case["id"])
            if group is None:
                continue
            identifier = f"{suite}-{case['id']}"
            entry = dict(id=identifier, name=case["id"], suite=group,
                         title=case.get("title", TITLES.get(case["id"], case["id"])),
                         measurementDate=report["environment"]["date"],
                         parameters=case.get("parameters", {}), arguments=case.get("arguments", []),
                         preset=case.get("preset", "Full"), policy=report["policy"], results={})
            for backend, result in case.get("results", {}).items():
                if result.get("referenceOnly"):
                    continue
                published = {key: value for key, value in result.items() if key in (
                    "status", "reason", "phase", "samples", "median", "validation", "gc",
                    "memoryAudit", "runtime", "adaptations", "compileCommand", "timeoutSeconds", "hugePagesKiB", "fastmath", "fastmathDispatchers")}
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
                    if case["id"] == "mandelbrot" and report.get("gpuMandelbrot"):
                        assert "@par(gpu=True" in text
                        published["execution"] = "gpu"
                        entry["acceleratorNote"] = (f"Codon uses the {report['environment']['gpu'].split(',')[0]} GPU via @par(gpu=True); "
                                                    "other runtimes use the CPU. The application timer includes GPU transfer and launch overhead.")
                download = source_directory / filename
                download.write_text(text if text.endswith("\n") else text + "\n")
                published["sourceUrl"] = f"{url_prefix}/source/{filename}"
                published["sourceSha256"] = sha256(download)
                published["executedSourceSha256"] = expected_code
                if not text.endswith("\n"):
                    published.setdefault("adaptations", []).append("The downloadable source adds a final newline; its original executed-source hash is retained in the report.")
                published["language"] = "cpp" if backend == "cpp" else "python"
                if suite == "npbench" and backend == "codon":
                    excerpt = focused_source(text, case["id"])
                    excerpt_name = f"{identifier}-{backend}-excerpt.codon"
                    (source_directory / excerpt_name).write_text(excerpt)
                    published["displaySourceUrl"] = f"{url_prefix}/source/{excerpt_name}"
                if suite == "npbench":
                    worker_name = f"{identifier}-{backend}-worker{source.suffix}"
                    (source_directory / worker_name).write_text(redact(source.read_text(), work))
                    published["workerUrl"] = f"{url_prefix}/source/{worker_name}"
                    published["workerSha256"] = sha256(source_directory / worker_name)
                    published["executedWorkerSha256"] = result["sourceSha256"]
                entry["results"][backend] = published
            if "reason" in case:
                entry["reason"] = case["reason"]
            entry["inputs"] = case.get("inputs", {})
            entries.append(entry)
    environment = next(iter(reports.values()))["environment"]
    payload = dict(schemaVersion=1, date=environment["date"], environment=environment,
                   datasets=reports.get("applications", {}).get("datasets", {}), cases=entries,
                   assetBase=url_prefix,
                   sampleCount=next(iter(reports.values()))["samples"], processTimeoutSeconds=timeouts.pop(),
                   versions={key: environment[key] for key in ("codon", "python", "pypy", "clang", "numpy", "numba", "scipy")})
    write_json(output / "results.json", redact(payload, work))
    write_json(output / "raw-results.json", redact(reports, work))
    shutil.copyfile(work / "npbench" / "LICENSE", output / "NPBench-LICENSE.txt")
    shutil.copyfile(ROOT / "bench" / "README.md", output / "reproduction.md")
    deduplicate_sources(output, url_prefix)
    print(f"Exported {len(entries)} workloads to {output}")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--work", type=Path, default=ROOT / "build" / "benchmarks-20261106")
    parser.add_argument("--output", type=Path, default=ROOT / "docs" / "data" / "benchmarks")
    parser.add_argument("--url-prefix", default="/benchmarks", help="Public asset directory for this machine's results")
    parser.add_argument("--report", type=Path, help="Export an isolated, completed NPBench L report to a separate staging directory")
    parser.add_argument("--deduplicate", action="store_true", help="Share identical sources across already-exported datasets without rerunning benchmarks")
    args = parser.parse_args()
    if args.deduplicate:
        if args.report:
            parser.error("--deduplicate cannot be combined with --report")
        deduplicate_sources(args.output.absolute(), args.url_prefix)
        return
    export(args.work.absolute(), args.output.absolute(), args.url_prefix, args.report)


if __name__ == "__main__":
    main()
