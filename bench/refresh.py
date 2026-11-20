"""Reproduce the public benchmark results from the existing suite sources."""

import argparse
import ast
import hashlib
import importlib.util
import itertools
import json
import math
import os
import platform
import re
import shutil
import signal
import statistics
import subprocess
import sys
import time
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
SOURCES = ROOT / "bench" / "codon"
LIBRARY_PATH = "DYLD_LIBRARY_PATH" if sys.platform == "darwin" else "LD_LIBRARY_PATH"
APPLICATIONS = {
    "binary_trees": ["20"],
    "chaos": ["chaos.ppm"],
    "fannkuch": ["11"],
    "float": [],
    "go": [],
    "mandelbrot": [],
    "nbody": ["10000000"],
    "set_partition": ["15"],
    "spectral_norm": [],
    "primes": ["100000"],
    "taq": ["{taq}"],
    "word_count": ["{words}"],
}


def catalogue():
    cases = []
    for name, arguments in APPLICATIONS.items():
        python = SOURCES / f"{name}.py"
        codon = SOURCES / f"{name}.codon"
        cpp = SOURCES / f"{name}.cpp"
        assert python.is_file(), python
        implementations = {
            "python": str(python.relative_to(ROOT)),
            "pypy": str(python.relative_to(ROOT)),
            "codon": str((codon if codon.exists() else python).relative_to(ROOT)),
        }
        if cpp.exists():
            implementations["cpp"] = str(cpp.relative_to(ROOT))
        cases.append(dict(id=name, suite="applications", arguments=arguments,
                          implementations=implementations))
    names = set()
    for line in (SOURCES / "npbench.codon").read_text().splitlines():
        if not line.startswith("run("):
            continue
        call = ast.parse(line).body[0].value
        name = call.args[0].attr
        assert name not in names, f"Duplicate NPBench case: {name}"
        assert call.args[1].id == name, f"Mismatched initializer: {name}"
        names.add(name)
        cases.append(dict(id=name, suite="npbench", parameters={
            keyword.arg: ast.literal_eval(keyword.value) for keyword in call.keywords
        }))
    return cases


def write_json(filename, value):
    temporary = filename.with_suffix(filename.suffix + ".tmp")
    temporary.write_text(json.dumps(value, indent=2, allow_nan=False) + "\n")
    temporary.replace(filename)


def digest(filename):
    with filename.open("rb") as stream:
        return hashlib.file_digest(stream, "sha256").hexdigest()


def load_module(filename):
    spec = importlib.util.spec_from_file_location(filename.stem, filename)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def benchmark_environment(work):
    environment = os.environ.copy()
    for name in ("OMP_NUM_THREADS", "OPENBLAS_NUM_THREADS", "MKL_NUM_THREADS",
                 "VECLIB_MAXIMUM_THREADS", "NUMBA_NUM_THREADS", "BLIS_NUM_THREADS"):
        environment[name] = "1"
    environment["PYTHONHASHSEED"] = "0"
    environment["PYTHONPATH"] = str(work / "npbench")
    if sys.platform == "linux":
        environment["NUMPY_MADVISE_HUGEPAGE"] = "1"
    environment.pop("CODON_PATH", None)
    environment.pop("DYLD_LIBRARY_PATH", None)
    environment.pop("LD_LIBRARY_PATH", None)
    return environment


def toolchain_paths(work):
    configuration = work / "toolchains.json"
    if configuration.exists():
        return {key: Path(value).absolute() for key, value in json.loads(configuration.read_text()).items()}
    assert sys.platform == "darwin", "Provide toolchains.json with clang and pypy paths on Linux"
    return dict(clang=work / "toolchains" / "LLVM-23.1.5-macOS-ARM64" / "bin" / "clang++",
                pypy=work / "toolchains" / "pypy3.12-v8.0.1-macos_arm64" / "bin" / "pypy3")


def huge_page_policy():
    root = Path("/sys/kernel/mm/transparent_hugepage")
    return {name: (root / name).read_text().strip() for name in ("enabled", "defrag")}


def require_huge_pages():
    if sys.platform == "linux":
        assert "[always]" in huge_page_policy()["enabled"], "NPBench requires transparent huge pages explicitly enabled (always)"


def execute(command, directory, environment, log, timeout=3600):
    started = time.monotonic()
    try:
        with log.open("w") as stream:
            process = subprocess.Popen(command, cwd=directory, env=environment,
                                       stdout=stream, stderr=subprocess.STDOUT,
                                       start_new_session=True)
            try:
                returncode = process.wait(timeout=timeout)
            except subprocess.TimeoutExpired:
                os.killpg(process.pid, signal.SIGKILL)
                process.wait()
                return {"status": "timeout", "timeoutSeconds": timeout}
        return {"status": "ok" if returncode == 0 else "error",
                "returncode": returncode, "wallSeconds": time.monotonic() - started}
    except OSError as error:
        return {"status": "error", "reason": str(error)}


def prepare_npbench(work, name, preset, directory):
    import numpy as np

    upstream = work / "npbench"
    metadata_file = upstream / "bench_info" / f"{name}.json"
    spec = json.loads(metadata_file.read_text())["benchmark"]
    parameters = spec["parameters"][preset]
    module_dir = upstream / "npbench" / "benchmarks" / spec["relative_path"]
    inputs = dict(parameters)
    if "init" in spec:
        module = load_module(module_dir / f"{spec['module_name']}.py")
        random_state = np.random.get_state() if name == "mlp" else None
        if random_state is not None:
            np.random.seed(42)
        try:
            values = getattr(module, spec["init"]["func_name"])(
                *(parameters[key] for key in spec["init"]["input_args"]))
        finally:
            if random_state is not None:
                np.random.set_state(random_state)
        if not isinstance(values, tuple):
            values = (values,)
        assert len(values) == len(spec["init"]["output_args"])
        inputs.update(zip(spec["init"]["output_args"], values))
    if name in ("doitgen", "scattering_self_energies"):
        spec["output_args"] = ["A" if name == "doitgen" else "Sigma"]
    descriptors = {}
    for argument in spec["input_args"]:
        value = inputs[argument]
        if isinstance(value, np.ndarray):
            value = np.ascontiguousarray(value)
            filename = directory / f"input-{argument}.npy"
            np.save(filename, value)
            with filename.open("rb") as stream:
                version = np.lib.format.read_magic(stream)
                assert version == (1, 0)
                np.lib.format.read_array_header_1_0(stream)
                payload_offset = stream.tell()
            descriptors[argument] = dict(dtype=value.dtype.name, ndim=value.ndim,
                                         shape=list(value.shape), bytes=value.nbytes,
                                         payloadOffset=payload_offset,
                                         sha256=digest(filename))
        else:
            descriptors[argument] = dict(value=value.item() if isinstance(value, np.generic) else value)
    result = dict(id=name, suite="npbench", title=spec["name"], preset=preset,
                  parameters=parameters, inputs=descriptors, metadata=spec,
                  moduleDirectory=str(module_dir))
    if name == "mlp":
        result["inputSeed"] = 42
    write_json(directory / "case.json", result)
    return result


def npbench_worker(case, backend):
    native = backend == "codon"
    spec = case["metadata"]
    module_dir = Path(case["moduleDirectory"])
    prefix = ["import numpy as np", "import sys", "import time"]
    if native:
        prefix += ["import npbench_lib as kernel_module",
                   "from C import GC_gcollect() -> None",
                   "from C import GC_get_total_bytes() -> int",
                   "from C import GC_get_heap_size() -> int",
                   "from C import GC_get_free_bytes() -> int"]
        function = f"kernel_module.{case['id']}"
        collector = "GC_gcollect()"
    else:
        suffix = "numpy" if backend == "python" else "numba_n"
        filename = Path(case["directory"]) / backend / "kernel.py"
        prefix += ["import gc", "import importlib.util", "import resource", "import json",
               "from threadpoolctl import threadpool_info, threadpool_limits",
               "thread_limits = threadpool_limits(limits=1)",
                   f"spec = importlib.util.spec_from_file_location('kernel_module', {str(filename)!r})",
                   "kernel_module = importlib.util.module_from_spec(spec)",
                   "spec.loader.exec_module(kernel_module)"]
        if backend == "numba" and case.get("numbaFastmath") == "disabled":
            prefix += ["from numba.core.registry import CPUDispatcher",
                       "dispatchers = {name: value for name, value in vars(kernel_module).items() if isinstance(value, CPUDispatcher)}",
                       "assert dispatchers, 'No Numba dispatchers found'",
                       "assert all(value.targetoptions.get('fastmath', False) is False for value in dispatchers.values()), 'Fast-math is still enabled'",
                       "print('BENCH_NUMBA_FASTMATH', json.dumps({name: False for name in sorted(dispatchers)}))"]
        function = f"kernel_module.{spec['func_name']}"
        collector = "gc.collect()"
    prefix += ["directory = sys.argv[1]", "samples = int(sys.argv[2])"]
    arrays = []
    for argument, descriptor in case["inputs"].items():
        if "dtype" in descriptor:
            arrays.append(argument)
            options = f", dtype=np.{descriptor['dtype']}, ndim={descriptor['ndim']}" if native else ""
            if native and descriptor["dtype"] in ("uint8", "int8", "bool"):
                shape = f".reshape({tuple(descriptor['shape'])!r})" if descriptor["ndim"] != 1 else ""
                prefix += [f"original_{argument} = np.fromfile(directory + '/input-{argument}.npy', dtype=np.{descriptor['dtype']}, offset={descriptor['payloadOffset']}){shape}"]
            else:
                prefix += [f"original_{argument} = np.load(directory + '/input-{argument}.npy'{options})"]
            prefix += [f"{argument} = np.empty_like(original_{argument})",
                       f"np.copyto({argument}, original_{argument})"]
        else:
            prefix += [f"{argument} = {descriptor['value']!r}"]
    if sys.platform == "linux":
        prefix += ["with open('/proc/self/smaps_rollup') as memory_status:",
                   "    for record in memory_status:",
                   "        if record.startswith('AnonHugePages:'):",
                   "            print('BENCH_HUGEPAGES_KIB', int(record.split()[1]))"]
    prefix += ["", "def emit(value, filename):"]
    if native:
        prefix += ["    if value is None:", "        return",
                   "    elif isinstance(value, tuple):",
                   "        index = 0", "        for item in value:",
                   "            emit(item, filename + '_' + str(index))",
                   "            index += 1",
                   "    else:", "        np.save(filename, np.asarray(value))"]
    else:
        prefix += ["    if value is None:", "        return",
                   "    if isinstance(value, tuple):",
                   "        for index, item in enumerate(value):",
                   "            emit(item, filename + '_' + str(index))",
                   "    else:", "        np.save(filename, np.asarray(value))"]
    arguments = ", ".join(spec["input_args"])
    prefix += [""] + (["@noinline"] if native else [])
    prefix += [f"def sample({arguments}):",
               "    started = time.perf_counter()",
               f"    result = {function}({arguments})",
               "    elapsed = time.perf_counter() - started"]
    if not spec.get("output_args"):
        prefix += ["    emit(result, directory + '/output-return')"]
    for argument in spec.get("output_args", []):
        prefix += [f"    emit({argument}, directory + '/output-{argument}')"]
    prefix += ["    return elapsed", "", "if samples == 0:"]
    if native:
        prefix += ["    GC_gcollect()", "    allocated_before = GC_get_total_bytes()",
                   "    heap_before = GC_get_heap_size()"]
    else:
        prefix += ["    gc.collect()", "    rss_before = resource.getrusage(resource.RUSAGE_SELF).ru_maxrss"]
    for argument in arrays:
        address = f"int({argument}.data)" if native else f"{argument}.ctypes.data"
        prefix += [f"    address_{argument} = {address}"]
    prefix += ["    for iteration in range(32):"]
    for argument in arrays:
        address = f"int({argument}.data)" if native else f"{argument}.ctypes.data"
        prefix += [f"        np.copyto({argument}, original_{argument})",
                   f"        assert {address} == address_{argument}"]
    if not arrays:
        prefix += ["        pass"]
    if native:
        prefix += ["    allocated = GC_get_total_bytes() - allocated_before",
                   "    heap_growth = GC_get_heap_size() - heap_before",
                   "    print('BENCH_MEMORY', allocated, heap_growth)",
                   "    assert allocated < 65536", "    assert heap_growth == 0"]
    else:
        prefix += ["    rss_growth = resource.getrusage(resource.RUSAGE_SELF).ru_maxrss - rss_before",
                   "    print('BENCH_MEMORY_RSS', rss_growth)"]
    prefix += ["elif samples == -1:", f"    sample({arguments})", "    print('BENCH_REFERENCE_ONLY')",
               "else:", "    for iteration in range(samples + 1):", f"        {collector}"]
    if native:
        prefix += ["        if iteration > 0:",
                   "            print('BENCH_GC', GC_get_heap_size(), GC_get_heap_size() - GC_get_free_bytes())"]
    for argument in arrays:
        prefix += [f"        np.copyto({argument}, original_{argument})"]
    prefix += [f"        elapsed = sample({arguments})",
               "        if iteration > 0:", "            print('BENCH_TIME', elapsed)"]
    if native:
        prefix += ["    GC_gcollect()",
                   "    print('BENCH_GC', GC_get_heap_size(), GC_get_heap_size() - GC_get_free_bytes())"]
    else:
        prefix += ["    print('BENCH_RUNTIME', json.dumps(threadpool_info()))"]
    return "\n".join(prefix) + "\n"


def check_npbench_outputs(reference, candidate, metadata):
    import numpy as np

    rtol = metadata.get("rtol", 1e-5)
    atol = metadata.get("atol", 1e-8)
    norm_limit = metadata.get("norm_error", 1e-5)
    names = sorted(filename.name for filename in reference.glob("output-*.npy"))
    assert names, "Kernel produced no observable output"
    assert names == sorted(filename.name for filename in candidate.glob("output-*.npy"))
    checked = 0
    dtypes = {}
    errors = {}
    for name in names:
        expected = np.load(reference / name, mmap_mode="r")
        actual = np.load(candidate / name, mmap_mode="r")
        assert expected.shape == actual.shape, name
        assert expected.dtype.kind == actual.dtype.kind, f"Dtype kind differs: {name}"
        dtypes[name] = {"reference": str(expected.dtype), "actual": str(actual.dtype)}
        expected_flat, actual_flat = expected.reshape(-1), actual.reshape(-1)
        pointwise = True
        error_squared = reference_squared = 0.0
        for offset in range(0, expected.size, 1_000_000):
            wanted = expected_flat[offset:offset + 1_000_000]
            observed = actual_flat[offset:offset + 1_000_000]
            if expected.dtype.kind in "fc":
                assert np.isfinite(wanted).all() and np.isfinite(observed).all(), f"Nonfinite output: {name}"
                pointwise = bool(np.allclose(observed, wanted, rtol=rtol, atol=atol)) and pointwise
                difference = np.abs(observed - wanted).astype(np.float64)
                magnitude = np.abs(wanted).astype(np.float64)
                error_squared += float(np.sum(difference * difference))
                reference_squared += float(np.sum(magnitude * magnitude))
            else:
                np.testing.assert_array_equal(observed, wanted, err_msg=name)
        relative = math.sqrt(error_squared / reference_squared) if reference_squared > 0 else (0.0 if error_squared == 0 else math.inf)
        errors[name] = dict(pointwise=pointwise, relativeNorm=relative if math.isfinite(relative) else None)
        assert pointwise or relative < norm_limit, f"{name}: relative norm {relative:.6g} exceeds {norm_limit}"
        checked += expected.size
    return {"status": "passed", "elements": int(checked), "rtol": rtol, "atol": atol,
            "normLimit": norm_limit, "errors": errors, "dtypes": dtypes}


def disable_numba_fastmath(text):
    source = text.encode()
    offsets = list(itertools.accumulate([0] + [len(line) for line in source.splitlines(keepends=True)]))
    replacements = []
    for node in ast.walk(ast.parse(text)):
        if isinstance(node, ast.keyword) and node.arg == "fastmath":
            value = node.value
            replacements.append((offsets[value.lineno - 1] + value.col_offset,
                                 offsets[value.end_lineno - 1] + value.end_col_offset))
    assert replacements, "No explicit Numba fast-math options found"
    for start, end in sorted(replacements, reverse=True):
        source = source[:start] + b"False" + source[end:]
    return source.decode()


def uniform_histogram_source(text):
    removed = set()
    for node in ast.walk(ast.parse(text)):
        if isinstance(node, ast.Import) and all(alias.name == "numba" for alias in node.names):
            removed.update(range(node.lineno, node.end_lineno + 1))
        elif isinstance(node, ast.FunctionDef):
            for decorator in node.decorator_list:
                if (isinstance(decorator, ast.Call) and isinstance(decorator.func, ast.Attribute)
                        and isinstance(decorator.func.value, ast.Name)
                        and decorator.func.value.id == "nb" and decorator.func.attr in ("jit", "njit")):
                    removed.update(range(decorator.lineno, decorator.end_lineno + 1))
    return "".join(line for number, line in enumerate(text.splitlines(keepends=True), 1) if number not in removed)


def prepare_npbench_plan(args, names):
    require_huge_pages()
    work = args.work.absolute()
    environment = benchmark_environment(work)
    sys.path.insert(0, str(work / "npbench"))
    args.output.mkdir(parents=True, exist_ok=True)
    refresh_cases = args.refresh_cases.split(",") if args.refresh_cases else []
    assert set(refresh_cases) <= set(names), "Refreshed cases must be included in this run"
    plan = {"status": "prepared", "preset": args.preset, "samples": args.samples,
              "processTimeoutSeconds": args.timeout,
              "numbaFastmath": args.numba_fastmath,
              "numbaOnly": args.numba_only,
              "refreshCases": refresh_cases,
              "environment": provenance(work),
              "policy": "Fixed input buffers, in-place reset and GC outside timer; one warm-up; kernel-only wall time.",
              "cases": []}
    baseline = None
    if args.baseline_report:
        baseline = json.loads(args.baseline_report.read_text())
        assert baseline["status"] == "complete" and baseline["preset"] == args.preset
        assert baseline["samples"] == args.samples
        expected = dict(baseline["environment"])
        current = dict(plan["environment"])
        expected.setdefault("platform", "darwin" if "macosBuild" in expected else "linux")
        for metadata in (expected, current):
            for key in ("date", "fingerprints", "codonCommit", "codonSourceDiffSha256"):
                metadata.pop(key, None)
            metadata["blas"] = sorted(metadata["blas"], key=lambda library: json.dumps(library, sort_keys=True))
        assert expected == current, "Baseline software or hardware differs"
        for filename, expected_hash in baseline["environment"]["fingerprints"].items():
            if filename != "bench/refresh.py":
                assert plan["environment"]["fingerprints"][filename] == expected_hash, filename
        plan["baselineReport"] = str(args.baseline_report.absolute())
        plan["baselineReportSha256"] = digest(args.baseline_report)
        baseline = {case["id"]: case for case in baseline["cases"]}
    for name in names:
        print(f"Preparing {name} ({args.preset})", flush=True)
        directory = args.output.absolute() / name
        directory.mkdir(parents=True, exist_ok=True)
        try:
            case = prepare_npbench(work, name, args.preset, directory)
            if baseline is not None:
                for key in ("inputs", "parameters", "metadata"):
                    expected, actual = baseline[name][key], case[key]
                    if key == "inputs" and name in refresh_cases:
                        expected = {argument: {field: value for field, value in descriptor.items() if field != "sha256"}
                                    for argument, descriptor in expected.items()}
                        actual = {argument: {field: value for field, value in descriptor.items() if field != "sha256"}
                                  for argument, descriptor in actual.items()}
                    assert actual == expected, f"Baseline {key} differs for {name}"
                if name in refresh_cases:
                    case["previousInputs"] = baseline[name]["inputs"]
            case["directory"] = str(directory)
            case["numbaFastmath"] = args.numba_fastmath
            case["results"] = {}
            numba_only = args.numba_only and name not in refresh_cases
            for backend in (("python", "numba") if numba_only else ("codon", "python", "numba")):
                backend_dir = directory / backend
                backend_dir.mkdir(exist_ok=True)
                for filename in directory.glob("input-*.npy"):
                    link = backend_dir / filename.name
                    if not link.exists():
                        link.symlink_to(filename)
                source = backend_dir / ("worker.codon" if backend == "codon" else "worker.py")
                source.write_text(npbench_worker(case, backend))
                runtime_environment = dict(environment)
                result = dict(status="prepared", source=str(source), sourceSha256=digest(source))
                if numba_only and backend == "python":
                    result["referenceOnly"] = True
                if backend == "codon":
                    if name == "azimint_hist":
                        original = Path(case["moduleDirectory"]) / f"{case['metadata']['module_name']}_numba_n.py"
                        library = uniform_histogram_source(original.read_text())
                        result["adaptations"] = ["Uses the same portable weighted-histogram helpers across Codon, Python, and Numba."]
                    else:
                        library = (SOURCES / "npbench_lib.codon").read_text()
                        assert library.count("import numpy.pybridge\n") == 1
                        library = library.replace("import numpy.pybridge\n", "")
                    (backend_dir / "npbench_lib.codon").write_text(library)
                    runtime_environment[LIBRARY_PATH] = str(ROOT / "build")
                    command = [str(ROOT / "build" / "codon"), "build", "-release", str(source), "-o", str(backend_dir / "worker")]
                    if sys.platform == "darwin":
                        command.append("--linker-flags=-framework Accelerate")
                    result["compileCommand"] = command
                    compiled = execute(command, backend_dir, runtime_environment, backend_dir / "compile.log")
                    result["compile"] = compiled
                    if compiled["status"] != "ok":
                        result.update(status="compile-error", phase="compile")
                    else:
                        result["executableSha256"] = digest(backend_dir / "worker")
                    command = [str(backend_dir / "worker")]
                    code = backend_dir / "npbench_lib.codon"
                else:
                    suffix = "numba_n" if backend == "numba" or name == "azimint_hist" else "numpy"
                    original = Path(case["moduleDirectory"]) / f"{case['metadata']['module_name']}_{suffix}.py"
                    code = backend_dir / "kernel.py"
                    text = original.read_text()
                    if name == "azimint_hist" and backend == "python":
                        text = uniform_histogram_source(text)
                        result["adaptations"] = ["Uses the same portable weighted-histogram helpers as Codon and Numba; the Python loop is interpreted."]
                    if name == "mlp" and backend == "numba":
                        text = replace_once(text,
                            "for i in range(x.shape[1]):\n        tmp_max[:, 0] = np.max(x[:, i])",
                            "for i in range(x.shape[0]):\n        tmp_max[i, 0] = np.max(x[i, :])")
                        result["adaptations"] = ["Corrects the upstream softmax maximum axis to prevent overflow; matches NumPy's row-wise maximum."]
                    if backend == "numba":
                        result["fastmath"] = False
                        text = disable_numba_fastmath(text)
                        result.setdefault("adaptations", []).append("Disables fast-math in Numba kernels and helpers; preserves the original algorithm.")
                    code.write_text(text)
                    command = [sys.executable, str(source)]
                result.update(command=command, code=str(code), codeSha256=digest(code))
                case["results"][backend] = result
                print("Prepared", name, backend, result["status"], flush=True)
            plan["cases"].append(case)
        except Exception as error:
            plan["cases"].append({"id": name, "status": "error", "reason": str(error)})
        write_json(args.output.absolute() / "plan.json", plan)
    return plan


def run_npbench(args):
    require_huge_pages()
    work, output = args.work.absolute(), args.output.absolute()
    environment = benchmark_environment(work)
    names = args.cases.split(",") if args.cases else [case["id"] for case in catalogue() if case["suite"] == "npbench"]
    report_file = output / "report.json"
    if args.resume and report_file.exists():
        report = json.loads(report_file.read_text())
    elif args.prepared:
        report = json.loads((output / "plan.json").read_text())
    else:
        report = prepare_npbench_plan(args, names)
    if args.prepare_only:
        return
    assert args.samples == report["samples"], "Sample count differs from preparation"
    assert args.numba_fastmath == report.get("numbaFastmath", "upstream"), "Fast-math policy differs from preparation"
    assert args.numba_only == report.get("numbaOnly", False), "Backend selection differs from preparation"
    assert (args.refresh_cases.split(",") if args.refresh_cases else []) == report.get("refreshCases", []), "Refreshed cases differ from preparation"
    if "baselineReport" in report:
        assert digest(Path(report["baselineReport"])) == report["baselineReportSha256"]
    check_fingerprints(report["environment"])
    report["status"] = "running"
    for case_index, case in enumerate(report["cases"]):
        if case.get("complete") or "results" not in case:
            continue
        directory = Path(case["directory"])
        backends = list(case["results"])
        order = backends[case_index % len(backends):] + backends[:case_index % len(backends)]
        for backend in order:
            result = case["results"][backend]
            if result["status"] != "prepared":
                continue
            backend_dir = directory / backend
            runtime_environment = dict(environment)
            if backend == "codon":
                runtime_environment[LIBRARY_PATH] = str(ROOT / "build")
            assert digest(Path(result["source"])) == result["sourceSha256"]
            assert digest(Path(result["code"])) == result["codeSha256"]
            if "executableSha256" in result:
                assert digest(Path(result["command"][0])) == result["executableSha256"]
            command = result["command"]
            if result.get("referenceOnly"):
                reference = execute(command + [str(backend_dir), "-1"], backend_dir, runtime_environment,
                                    backend_dir / "reference.log", args.timeout)
                result.update(reference, phase="validation-reference")
                if result["status"] == "ok":
                    assert "BENCH_REFERENCE_ONLY" in (backend_dir / "reference.log").read_text()
                    result["status"] = "reference"
                print(case["id"], backend, result["status"], flush=True)
                write_json(report_file, report)
                continue
            audit = execute(command + [str(backend_dir), "0"], backend_dir, runtime_environment, backend_dir / "memory.log", args.timeout)
            if audit["status"] != "ok":
                result.update(audit, phase="memory-audit")
                write_json(report_file, report)
                continue
            measured = execute(command + [str(backend_dir), str(args.samples)], backend_dir,
                               runtime_environment, backend_dir / "run.log", args.timeout)
            result.update(measured, memoryAudit=(backend_dir / "memory.log").read_text().strip())
            if measured["status"] == "ok":
                lines = (backend_dir / "run.log").read_text().splitlines()
                if backend == "numba" and report.get("numbaFastmath") == "disabled":
                    options = [json.loads(line.removeprefix("BENCH_NUMBA_FASTMATH ")) for line in lines if line.startswith("BENCH_NUMBA_FASTMATH ")]
                    assert len(options) == 1 and options[0] and all(value is False for value in options[0].values())
                    result["fastmathDispatchers"] = options[0]
                samples = [float(line.split()[1]) for line in lines if line.startswith("BENCH_TIME ")]
                assert len(samples) == args.samples and all(math.isfinite(value) and value > 0 for value in samples)
                result.update(samples=samples, median=statistics.median(samples))
                if sys.platform == "linux":
                    huge_pages = [int(line.split()[1]) for line in lines if line.startswith("BENCH_HUGEPAGES_KIB ")]
                    assert len(huge_pages) == 1, "Missing huge-page observation"
                    result["hugePagesKiB"] = huge_pages[0]
                if backend == "codon":
                    records = [list(map(int, line.split()[1:])) for line in lines if line.startswith("BENCH_GC ")]
                    assert len(records) == args.samples + 1
                    growth = records[-1][1] - records[0][1]
                    budget = max(16 * 1024 * 1024, sum(item.get("bytes", 0) for item in case["inputs"].values()) // 20)
                    result["gc"] = dict(heapAndLiveBytes=records, liveGrowthBytes=growth, budgetBytes=budget)
                    if growth > budget:
                        result.update(status="memory-review", reason="Post-collection live heap did not stabilize within the declared budget.")
                else:
                    result["runtime"] = [json.loads(line.removeprefix("BENCH_RUNTIME ")) for line in lines if line.startswith("BENCH_RUNTIME ")][0]
                    assert all(pool["num_threads"] == 1 for pool in result["runtime"]), result["runtime"]
            print(case["id"], backend, result["status"], result.get("median"), flush=True)
            write_json(report_file, report)
        for backend, result in case["results"].items():
            if result["status"] not in ("ok", "memory-review"):
                continue
            try:
                assert case["results"]["python"]["status"] in ("ok", "reference"), "Python reference failed"
                result["validation"] = check_npbench_outputs(directory / "python", directory / backend, case["metadata"])
            except (AssertionError, OSError, ValueError) as error:
                result.update(status="invalid", reason=str(error)[:2000])
        case["complete"] = True
        write_json(report_file, report)
        if not args.keep_arrays:
            for backend in case["results"]:
                for filename in (directory / backend).glob("*.npy"):
                    filename.unlink()
            for filename in directory.glob("input-*.npy"):
                filename.unlink()
    check_fingerprints(report["environment"])
    report["status"] = "complete"
    write_json(report_file, report)
    print(report_file, flush=True)


def command_output(command):
    return subprocess.check_output(command, text=True, stderr=subprocess.STDOUT).strip()


def provenance(work):
    import numpy as np
    import numba
    import scipy
    import scipy.linalg
    from threadpoolctl import threadpool_info, threadpool_limits

    tools = toolchain_paths(work)
    suffix = ".dylib" if sys.platform == "darwin" else ".so"
    files = [ROOT / "build" / name for name in ("codon", f"libcodonc{suffix}", f"libcodonrt{suffix}", f"libomp{suffix}")]
    files.append(Path(__file__))
    if (work / "toolchains.json").exists():
        files.append(work / "toolchains.json")
    files += sorted(SOURCES.glob("*.py")) + sorted(SOURCES.glob("*.codon")) + sorted(SOURCES.glob("*.cpp"))
    source_stamp = ROOT / "benchmark-source.json"
    if source_stamp.exists():
        source_metadata = json.loads(source_stamp.read_text())
        files.append(source_stamp)
    else:
        source_metadata = dict(codonCommit=command_output(["git", "-C", str(ROOT), "rev-parse", "HEAD"]),
                               codonSourceDiffSha256=hashlib.sha256(command_output(["git", "-C", str(ROOT), "diff", "HEAD", "--", "codon", "stdlib"]).encode()).hexdigest())
    if sys.platform == "darwin":
        host = dict(os=command_output(["sw_vers", "-productVersion"]),
                    cpu=command_output(["sysctl", "-n", "machdep.cpu.brand_string"]),
                    memoryBytes=int(command_output(["sysctl", "-n", "hw.memsize"])),
                    logicalCpus=int(command_output(["sysctl", "-n", "hw.logicalcpu"])),
                    macosBuild=command_output(["sw_vers", "-buildVersion"]),
                    sdk=command_output(["xcrun", "--show-sdk-version"]),
                    runtimeLibraries=["build/libcodonrt.dylib", "build/libomp.dylib", "Apple Accelerate"])
    else:
        assert sys.platform == "linux", sys.platform
        cpu = next(line.partition(":")[2].strip() for line in Path("/proc/cpuinfo").read_text().splitlines() if line.startswith("model name"))
        host = dict(os=platform.freedesktop_os_release()["PRETTY_NAME"], kernel=platform.release(), cpu=cpu,
                    memoryBytes=os.sysconf("SC_PHYS_PAGES") * os.sysconf("SC_PAGE_SIZE"), logicalCpus=os.cpu_count(),
                    transparentHugePages=huge_page_policy(), numpyMadviseHugepage=1,
                    runtimeLibraries=["build/libcodonrt.so", "build/libomp.so", "OpenBLAS ILP64 (linked into Codon runtime)"])
        if shutil.which("nvidia-smi"):
            host["gpu"] = command_output(["nvidia-smi", "--query-gpu=name,memory.total,driver_version", "--format=csv,noheader"])
    with threadpool_limits(limits=1):
        blas = threadpool_info()
    return dict(date=time.strftime("%Y-%m-%d"), machine=platform.machine(), platform=sys.platform,
                codon=command_output([str(ROOT / "build" / "codon"), "--version"]),
                npbenchCommit=command_output(["git", "-C", str(work / "npbench"), "rev-parse", "HEAD"]),
                python=platform.python_version(), numpy=np.__version__, numba=numba.__version__,
                scipy=scipy.__version__, pypy=command_output([str(tools["pypy"]), "--version"]),
                clang=command_output([str(tools["clang"]), "--version"]),
                blas=blas, threads=1, **host, **source_metadata,
                fingerprints={str(filename.relative_to(ROOT)): digest(filename) for filename in files})


def check_fingerprints(metadata):
    if "transparentHugePages" in metadata:
        assert huge_page_policy() == metadata["transparentHugePages"], "Huge-page policy changed during benchmark"
    for filename, expected in metadata["fingerprints"].items():
        assert digest(ROOT / filename) == expected, f"Changed during benchmark: {filename}"


def replace_once(text, before, after):
    assert text.count(before) == 1, before
    return text.replace(before, after)


def application_source(name, backend, quick, gpu_mandelbrot=False):
    original = SOURCES / f"{name}.py"
    if backend == "cpp":
        original = SOURCES / f"{name}.cpp"
    elif backend == "codon" and (name != "mandelbrot" or gpu_mandelbrot) and (SOURCES / f"{name}.codon").exists():
        original = SOURCES / f"{name}.codon"
    source = original.read_text()
    adaptations = []
    if backend != "cpp":
        source = source.replace("time.time()", "time.perf_counter()")
        adaptations.append("Uses a monotonic high-resolution wall timer.")
    if name == "chaos" and backend == "codon":
        source = replace_once(source, '    with open(filename, "w") as fp:',
                              '    with open(filename, "w", encoding="latin1", newline="") as fp:')
        adaptations.append("Uses Latin-1 PPM output, matching the existing Python implementation.")
    if name == "mandelbrot" and backend == "codon":
        adaptations.append("Uses the existing CUDA implementation with @par(gpu=True, collapse=2)." if gpu_mandelbrot
                           else "Uses the existing CPU Python implementation; the CUDA version requires NVIDIA hardware.")
    if name == "go":
        source = replace_once(source, "def versus_cpu():\n", "def versus_cpu():\n    checksum = 0\n")
        source = replace_once(source, "        computer_move(board)\n", "        checksum += computer_move(board)\n    return checksum\n")
        source = replace_once(source, "    versus_cpu()\n", "    print(versus_cpu())\n")
        adaptations.append("Prints the sum of selected moves to validate observable work.")
    if backend == "cpp":
        source = "#include <iomanip>\n" + source
        source = source.replace("std::chrono::high_resolution_clock", "std::chrono::steady_clock")
        source = replace_once(source, "int main(int argc, char *argv[]) {", "int main(int argc, char *argv[]) {\n  std::cout << std::setprecision(17);")
        source = source.replace("(duration_cast<milliseconds>(clock::now() - t).count() / 1e3)",
                                "std::chrono::duration<double>(clock::now() - t).count()")
        adaptations.append("Prints full-precision values and sub-millisecond elapsed time.")
    if quick:
        if name == "float":
            source = replace_once(source, "POINTS = 10000000", "POINTS = 10000")
        elif name == "mandelbrot":
            source = replace_once(source, "N      = 4096", "N      = 64")
        elif name == "go":
            source = replace_once(source, "for i in range(100):", "for i in range(2):")
        adaptations.append("Small validation-only input; not a published performance result.")
    return source, str(original.relative_to(ROOT)), adaptations


def application_output(stdout):
    lines = stdout.strip().splitlines()
    elapsed = float(lines[-1])
    assert math.isfinite(elapsed) and elapsed > 0
    return elapsed, sorted(line.strip() for line in lines[:-1] if line.strip())


def compare_application(expected, actual):
    numbers = r"[-+]?(?:\d+(?:\.\d*)?|\.\d+)(?:[eE][-+]?\d+)?"
    assert len(expected) == len(actual), "Different number of output lines"
    assert expected, "No observable result"
    for wanted, observed in zip(expected, actual):
        assert re.sub(numbers, "#", wanted) == re.sub(numbers, "#", observed), (wanted, observed)
        for first, second in zip(re.findall(numbers, wanted), re.findall(numbers, observed)):
            if re.fullmatch(r"[-+]?\d+", first) and re.fullmatch(r"[-+]?\d+", second):
                assert int(first) == int(second), (first, second)
            else:
                assert math.isclose(float(first), float(second), rel_tol=1e-8, abs_tol=1e-10), (first, second)


def prepare_applications(args, names):
    work = args.work.absolute()
    output = args.output.absolute()
    output.mkdir(parents=True, exist_ok=True)
    environment = benchmark_environment(work)
    tools = toolchain_paths(work)
    cpp_flags = ["-O3", "-std=c++17", "-fuse-ld=lld"]
    if sys.platform == "darwin":
        sdk = command_output(["xcrun", "--show-sdk-path"])
        cpp_headers = Path(sdk) / "usr" / "include" / "c++" / "v1"
        assert cpp_headers.is_dir(), cpp_headers
        cpp_flags += ["-mcpu=native", "-isysroot", sdk, "-nostdinc++", "-isystem", str(cpp_headers)]
    else:
        cpp_flags += ["-march=native"]
    datasets = {}
    data_directory = output / "data"
    data_directory.mkdir(exist_ok=True)
    if "taq" in names:
        assert args.taq_source, "--taq-source is required"
        filename = data_directory / "taq.txt"
        limit = 20_000 if args.quick else 10_000_000
        with args.taq_source.open("rb") as source, filename.open("wb") as target:
            target.writelines(itertools.islice(source, limit))
        datasets["taq"] = dict(path=str(filename), source=args.taq_source.name,
                                lineLimit=limit, bytes=filename.stat().st_size, sha256=digest(filename))
    if "word_count" in names:
        assert args.word_source, "--word-source is required"
        filename = data_directory / "words.txt"
        with args.word_source.open("rb") as source:
            data = source.read(64 * 1024 if args.quick else 16 * 1024 * 1024)
        filename.write_bytes(data.rsplit(b"\n", 1)[0] + b"\n")
        datasets["words"] = dict(path=str(filename), source=args.word_source.name,
                                  bytes=filename.stat().st_size, sha256=digest(filename))
    plan = dict(status="prepared", quick=args.quick, gpuMandelbrot=args.gpu_mandelbrot, samples=args.samples,
                environment=provenance(work), datasets=datasets, cases=[])
    plan["policy"] = "Existing application-internal wall timer; fresh process per sample; first process excluded; allocation and PyPy JIT within the application are included; rotating backend order."
    small_arguments = {"binary_trees": ["10"], "fannkuch": ["7"], "nbody": ["10000"],
                       "set_partition": ["10"], "primes": ["1000"]}
    for name in names:
        case = dict(id=name, suite="applications", arguments=small_arguments.get(name, APPLICATIONS[name]) if args.quick else APPLICATIONS[name], results={})
        for backend in ("codon", "python", "pypy", "cpp"):
            if backend == "cpp" and not (SOURCES / f"{name}.cpp").exists():
                continue
            directory = output / name / backend
            directory.mkdir(parents=True, exist_ok=True)
            text, original, adaptations = application_source(name, backend, args.quick, args.gpu_mandelbrot)
            suffix = ".cpp" if backend == "cpp" else ".codon" if backend == "codon" else ".py"
            source = directory / ("worker" + suffix)
            source.write_text(text)
            result = dict(status="prepared", source=str(source), original=original,
                          sourceSha256=digest(source), adaptations=adaptations)
            backend_environment = dict(environment)
            if backend in ("codon", "cpp"):
                executable = directory / "worker"
                if backend == "codon":
                    backend_environment[LIBRARY_PATH] = str(ROOT / "build")
                    command = [str(ROOT / "build" / "codon"), "build", "-release", str(source), "-o", str(executable)]
                else:
                    command = [str(tools["clang"]), *cpp_flags, str(source), "-o", str(executable)]
                result["compileCommand"] = command
                compiled = execute(command, directory, backend_environment, directory / "compile.log")
                result["compile"] = compiled
                if compiled["status"] != "ok":
                    result["status"] = "compile-error"
                else:
                    result["executableSha256"] = digest(executable)
                result["command"] = [str(executable)]
            elif backend == "python":
                result["command"] = [sys.executable, str(source)]
            else:
                result["command"] = [str(tools["pypy"]), str(source)]
            case["results"][backend] = result
            print("Prepared", name, backend, result["status"], flush=True)
        plan["cases"].append(case)
        write_json(output / "plan.json", plan)
    return plan


def run_applications(args):
    work, output = args.work.absolute(), args.output.absolute()
    names = args.cases.split(",") if args.cases else list(APPLICATIONS)
    plan_file = output / "plan.json"
    report_file = output / "report.json"
    if args.resume and report_file.exists():
        plan = json.loads(report_file.read_text())
    else:
        plan = json.loads(plan_file.read_text()) if args.prepared else prepare_applications(args, names)
    if args.prepare_only:
        return
    check_fingerprints(plan["environment"])
    assert args.samples == plan["samples"], "Sample count differs from preparation"
    plan["status"] = "running"
    environment = benchmark_environment(work)
    for case in plan["cases"]:
        if case.get("complete"):
            continue
        backends = list(case["results"])
        references = []
        for result in case["results"].values():
            assert digest(Path(result["source"])) == result["sourceSha256"]
            if "executableSha256" in result:
                assert digest(Path(result["command"][0])) == result["executableSha256"]
            if args.resume and result["status"] == "ok":
                result.update(status="prepared")
                result.pop("samples", None)
                result.pop("outputs", None)
        for iteration in range(args.samples + 1):
            order = backends[iteration % len(backends):] + backends[:iteration % len(backends)]
            for backend in order:
                result = case["results"][backend]
                if result["status"] not in ("prepared", "ok"):
                    continue
                directory = output / case["id"] / backend
                arguments = [argument.format(**{key: value["path"] for key, value in plan["datasets"].items()}) for argument in case["arguments"]]
                backend_environment = dict(environment)
                if backend == "codon":
                    backend_environment[LIBRARY_PATH] = str(ROOT / "build")
                log = directory / f"run-{iteration}.log"
                execution = execute(result["command"] + arguments, directory, backend_environment, log, args.timeout)
                if execution["status"] != "ok":
                    result.update(execution)
                    continue
                try:
                    elapsed, observed = application_output(log.read_text())
                    if case["id"] == "chaos":
                        observed = [digest(directory / "chaos.ppm")]
                    result.setdefault("outputs", []).append(observed)
                    if backend == "python":
                        references.append(observed)
                    result["status"] = "ok"
                    if iteration > 0:
                        result.setdefault("samples", []).append(elapsed)
                    print(case["id"], backend, iteration, elapsed, flush=True)
                except (AssertionError, ValueError, OSError) as error:
                    result.update(status="invalid", reason=str(error))
        for backend, result in case["results"].items():
            if result["status"] != "ok":
                continue
            try:
                assert references, "Python reference failed"
                assert len(result["samples"]) == args.samples
                for observed in result["outputs"]:
                    if case["id"] == "chaos":
                        assert observed == references[0], "Rendered PPM differs from Python"
                    else:
                        compare_application(references[0], observed)
                result.update(median=statistics.median(result["samples"]), validation={"status": "passed"})
            except (AssertionError, KeyError) as error:
                result.update(status="invalid", reason=str(error))
        case["complete"] = True
        write_json(report_file, plan)
    check_fingerprints(plan["environment"])
    plan["status"] = "complete"
    write_json(report_file, plan)
    print(report_file, flush=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--list", action="store_true", help="Print source inventory")
    parser.add_argument("--suite", choices=["npbench", "applications"])
    parser.add_argument("--work", type=Path, default=ROOT / "build" / "benchmarks-20261106")
    parser.add_argument("--output", type=Path, default=ROOT / "build" / "benchmarks-20261106" / "pilot")
    parser.add_argument("--cases")
    parser.add_argument("--preset", choices=["S", "M", "L"], default="L")
    parser.add_argument("--samples", type=int, default=3)
    parser.add_argument("--timeout", type=int, default=3600)
    parser.add_argument("--numba-fastmath", choices=["disabled"], default="disabled", help="Numba kernels and helpers always use fastmath=False")
    parser.add_argument("--numba-only", action="store_true", help="Measure only Numba; run Python once solely for output validation")
    parser.add_argument("--baseline-report", type=Path, help="Verify software, hardware, and input hashes against an existing NPBench report")
    parser.add_argument("--refresh-cases", default="", help="Comma-separated NPBench cases to reinitialize and measure on all three runtimes")
    parser.add_argument("--quick", action="store_true", help="Small application correctness pilot")
    parser.add_argument("--gpu-mandelbrot", action="store_true", help="Use the existing Codon CUDA Mandelbrot implementation")
    parser.add_argument("--prepare-only", action="store_true")
    parser.add_argument("--prepared", action="store_true")
    parser.add_argument("--resume", action="store_true")
    parser.add_argument("--keep-arrays", action="store_true")
    parser.add_argument("--taq-source", type=Path)
    parser.add_argument("--word-source", type=Path)
    args = parser.parse_args()
    if args.samples < 1 or args.timeout < 1:
        parser.error("--samples and --timeout must be positive")
    if args.suite != "npbench" and (args.numba_only or args.baseline_report or args.refresh_cases):
        parser.error("--numba-only, --baseline-report and --refresh-cases require --suite npbench")
    if args.list:
        print(json.dumps(catalogue(), indent=2))
    elif args.suite == "npbench":
        run_npbench(args)
    elif args.suite == "applications":
        run_applications(args)
    else:
        parser.error("choose --list or --suite")


if __name__ == "__main__":
    main()
