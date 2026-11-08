# Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>

import argparse
import ctypes
import difflib
import os
import shutil
import subprocess
import sys
import tempfile
import traceback
from contextlib import contextmanager
from pathlib import Path

if __package__:
    from . import ast, cache, parser
else:
    sys.path.insert(0, str(Path(__file__).resolve().parent.parent))
    from meister import ast, cache, parser


class TestCase:
    def __init__(self, path, name="", code=None, line=0, barebones=False):
        self.path = path
        self.name = name
        self.code = code
        self.line = line
        self.barebones = barebones

    @property
    def label(self):
        if self.code is None:
            return str(self.path)
        name = self.name or f"line-{self.line + 1}"
        return f"{self.path}:{self.line + 1}:{name}"


class Result:
    def __init__(self, output=None, error=None):
        self.output = output
        self.error = error


class _NativeResult(ctypes.Structure):
    _fields_ = [("output", ctypes.c_void_p), ("error", ctypes.c_void_p)]


class NativeCodon:
    def __init__(self, library):
        self.library = ctypes.CDLL(str(library))

        self.dump_code = self.library.codon_ast_dump_code
        self.dump_code.argtypes = [
            ctypes.c_char_p,
            ctypes.c_char_p,
            ctypes.c_int,
            ctypes.c_uint8,
            ctypes.c_int,
            ctypes.c_int,
        ]
        self.dump_code.restype = _NativeResult

        self.dump_file = self.library.codon_ast_dump_file
        self.dump_file.argtypes = [ctypes.c_char_p, ctypes.c_uint8, ctypes.c_int, ctypes.c_int]
        self.dump_file.restype = _NativeResult

        self.free = self.library.codon_ast_dump_free
        self.free.argtypes = [ctypes.c_void_p]
        self.free.restype = None

    def get_early_defines(self):
        if not hasattr(self.library, "codon_ast_dump_defines"):
            return None
        get_defines = self.library.codon_ast_dump_defines
        get_defines.argtypes = []
        get_defines.restype = ctypes.c_void_p
        result = get_defines()
        try:
            return dict(
                line.split("=", 1) for line in ctypes.string_at(result).decode().splitlines()
            )
        finally:
            self.free(result)

    def _consume(self, result):
        try:
            output = (
                ctypes.string_at(result.output).decode("utf-8", errors="backslashreplace")
                if result.output
                else None
            )
            error = (
                ctypes.string_at(result.error).decode("utf-8", errors="backslashreplace")
                if result.error
                else None
            )
            if output is None and error is None:
                error = "native Codon returned neither an AST dump nor an error"
            return Result(output, error)
        finally:
            if result.output:
                self.free(result.output)
            if result.error:
                self.free(result.error)

    def run(self, case, typecheck: int = 0, indent=2):
        filename = os.fsencode(case.path)
        if case.code is None:
            return self._consume(self.dump_file(filename, True, indent, typecheck))
        return self._consume(
            self.dump_code(case.code.encode("utf-8"), filename, case.line, True, indent, typecheck)
        )


def split_test_file(path):
    text = path.read_text(encoding="utf-8")
    lines = text.splitlines()
    if not any(line.startswith("#%%") for line in lines):
        return [TestCase(path)]

    cases = []
    code = []
    name = ""
    start_line = 0
    saw_marker = False
    barebones = False

    for line_number, line in enumerate(lines):
        if line.startswith("#%%"):
            if saw_marker:
                cases.append(TestCase(path, name, "".join(code), start_line, barebones))
            elif not saw_marker and code:
                cases.append(TestCase(path, "", "".join(code), start_line))

            metadata = line[4:].split(",")
            name = metadata[0] if metadata else ""
            # C++: test/main.cpp:405 selects the stdlib from this marker.
            barebones = len(metadata) > 1 and metadata[1] == "barebones"
            code = [line + "\n"]
            start_line = line_number
            saw_marker = True
        else:
            code.append(line + "\n")

    if code:
        cases.append(TestCase(path, name, "".join(code), start_line, barebones))
    return cases


def discover_tests(path):
    if path.is_file():
        paths = [path]
    else:
        paths = sorted(path.rglob("*.codon"))
    cases = []
    for test_path in paths:
        # if "numpy" in str(test_path):
        #     continue
        cases.extend(split_test_file(test_path))
    return cases


def run_python(case, typecheck, early_defines=None, indent=2, prepared=None):
    recursion_limit = sys.getrecursionlimit()
    try:
        if typecheck:
            from .passes.typecheck import typecheck_program

            # One Codon realization uses many Python visitor frames.
            sys.setrecursionlimit(max(recursion_limit, cache.MAX_REALIZATION_DEPTH * 100))
        cc = cache.Cache("codon") if prepared is None else prepared[0]
        if case.code is None:
            node = parser.parse(file=str(case.path))
        else:
            node = parser.parse(file=str(case.path), code=case.code, line_offset=case.line)
        if typecheck:
            node = typecheck_program(
                cc,
                node,
                file=str(case.path),
                early_defines=(
                    cc.options.get_early_defines() if early_defines is None else early_defines
                ),
                barebones=typecheck > 1,
                _preamble=None if prepared is None else prepared[1],
            )

            out = ast.dump(node, indent=indent, include_attributes=True, types=True) + "\n"
            for _, fn_data in sorted(cc.functions.items()):
                for _, r in sorted(fn_data.realizations.items()):
                    if r.ast:
                        out += (
                            ast.dump(r.ast, indent=indent, include_attributes=True, types=True)
                            + "\n"
                        )
        else:
            node = cc.scope(node)
            out = ast.dump(node, indent=indent, include_attributes=True)
        return Result(out)
    except Exception as error:
        msg = ""
        if hasattr(error, "info"):
            msg = f" ({case.path}:{error.info.line}:{error.info.col})"
        return Result(error=f"{type(error).__name__}: {error}{msg}\n\n{traceback.format_exc()}")
    finally:
        sys.setrecursionlimit(recursion_limit)


def default_test_path(root):
    for path in (Path.cwd() / "tests", Path.cwd() / "test", root / "tests", root / "test"):
        if path.exists():
            return path
    return root / "tests"


def prepare_python(typecheck, early_defines):
    from .passes.typecheck import load_std_library

    cc = cache.Cache("codon")
    preamble = ast.SuiteStmt()
    recursion_limit = sys.getrecursionlimit()
    try:
        sys.setrecursionlimit(max(recursion_limit, cache.MAX_REALIZATION_DEPTH * 100))
        load_std_library(
            cc, preamble, early_defines or cc.options.get_early_defines(), typecheck > 1
        )
    finally:
        sys.setrecursionlimit(recursion_limit)
    return cc, preamble


def _isolated_case(connection, prepared, case, native, typecheck, early_defines, indent):
    try:
        connection.send(run_python(case, typecheck, early_defines, indent, prepared))
        connection.send(native.run(case, typecheck, indent))
    finally:
        connection.close()


def run_isolated_case(prepared, case, native, typecheck, early_defines, indent):
    import multiprocessing

    ctx = multiprocessing.get_context("fork")
    receiver, sender = ctx.Pipe(duplex=False)
    process = ctx.Process(
        target=_isolated_case,
        args=(sender, prepared, case, native, typecheck, early_defines, indent),
    )
    process.start()
    sender.close()
    results = []
    try:
        results.append(receiver.recv())
        results.append(receiver.recv())
    except EOFError:
        pass
    finally:
        receiver.close()
        process.join()
    failure = Result(error=f"test worker exited unexpectedly (status {process.exitcode})")
    return tuple(results + [failure] * (2 - len(results)))


def library_candidates(root):
    suffixes = (".dylib", ".so", ".dll")
    for build in ("build", "build-rel", "build-linux", "build-rel-linux"):
        for suffix in suffixes:
            yield root / build / f"libcodonc{suffix}"


def find_library(root, requested):
    if requested:
        candidates = [Path(requested)]
    elif os.environ.get("CODON_LIBRARY"):
        candidates = [Path(os.environ["CODON_LIBRARY"])]
    else:
        candidates = [path for path in library_candidates(root) if path.exists()]

    errors = []
    for candidate in candidates:
        try:
            library = NativeCodon(candidate.resolve())
            return library, candidate.resolve()
        except (OSError, AttributeError) as error:
            errors.append(f"{candidate}: {error}")

    detail = "\n".join(errors) if errors else "no libcodonc shared library was found"
    raise RuntimeError(
        f"could not load a Codon library with the scoped-AST API:\n{detail}\n"
        "Build the codonc target or pass --library /path/to/libcodonc."
    )


def print_unified_diff(case, python_output, native_output, max_lines):
    lines = list(
        difflib.unified_diff(
            native_output.splitlines(),
            python_output.splitlines(),
            fromfile=f"C++ Codon: {case.label}",
            tofile=f"Python port: {case.label}",
            lineterm="",
        )
    )
    for line in lines[:max_lines]:
        print(line)
    if len(lines) > max_lines:
        print(f"... {len(lines) - max_lines} diff lines omitted")


def _clip_diff_line(line, width):
    line = line.expandtabs(4)
    if len(line) <= width:
        return line
    return line[: max(0, width - 1)] + "…"


def print_side_by_side_diff(python_output, native_output, max_lines):
    native_lines = native_output.splitlines()
    python_lines = python_output.splitlines()
    matcher = difflib.SequenceMatcher(None, native_lines, python_lines)
    rows = []

    for group_number, group in enumerate(matcher.get_grouped_opcodes(3)):
        if group_number:
            rows.append(("…", None, "", None, ""))
        for tag, native_start, native_end, python_start, python_end in group:
            if tag == "equal":
                for offset in range(native_end - native_start):
                    rows.append(
                        (
                            " ",
                            native_start + offset,
                            native_lines[native_start + offset],
                            python_start + offset,
                            python_lines[python_start + offset],
                        )
                    )
            elif tag == "replace":
                count = max(native_end - native_start, python_end - python_start)
                for offset in range(count):
                    native_index = native_start + offset
                    python_index = python_start + offset
                    rows.append(
                        (
                            "!",
                            native_index if native_index < native_end else None,
                            native_lines[native_index] if native_index < native_end else "",
                            python_index if python_index < python_end else None,
                            python_lines[python_index] if python_index < python_end else "",
                        )
                    )
            elif tag == "delete":
                for native_index in range(native_start, native_end):
                    rows.append(("-", native_index, native_lines[native_index], None, ""))
            elif tag == "insert":
                for python_index in range(python_start, python_end):
                    rows.append(("+", None, "", python_index, python_lines[python_index]))

    terminal_width = shutil.get_terminal_size((180, 24)).columns
    number_width = max(4, len(str(max(len(native_lines), len(python_lines), 1))))
    text_width = max(20, (terminal_width - 2 * number_width - 9) // 2)
    side_width = number_width + 1 + text_width

    print(f"  {'C++ Codon':<{side_width}} | {'Python port':<{side_width}}")
    print(f"  {'-' * side_width}-+-{'-' * side_width}")

    for marker, native_index, native_line, python_index, python_line in rows[:max_lines]:
        native_number = "" if native_index is None else str(native_index + 1)
        python_number = "" if python_index is None else str(python_index + 1)
        native_side = f"{native_number:>{number_width}} {_clip_diff_line(native_line, text_width)}"
        python_side = f"{python_number:>{number_width}} {_clip_diff_line(python_line, text_width)}"
        print(f"{marker} {native_side:<{side_width}} | {python_side:<{side_width}}")

    if len(rows) > max_lines:
        print(f"... {len(rows) - max_lines} side-by-side diff lines omitted")


def launch_code_diff(case, python_output, native_output):
    if shutil.which("code") is None:
        print("VS Code command 'code' was not found; showing a side-by-side diff instead.")
        return False

    with tempfile.TemporaryDirectory(prefix="codon-ast-diff-") as directory:
        directory = Path(directory)
        native_path = directory / "cpp-codon.ast.txt"
        python_path = directory / "python-port.ast.txt"
        native_path.write_text("cpp\n" + native_output + "\n", encoding="utf-8")
        python_path.write_text("python\n" + python_output + "\n", encoding="utf-8")
        print(f"Opening VS Code diff for {case.label}")
        result = subprocess.run(
            ["code", "--wait", "--diff", str(native_path), str(python_path)],
            check=False,
        )
    if result.returncode:
        print(f"VS Code exited with status {result.returncode}; showing terminal diff.")
        return False
    return True


def show_diff(case, python_output, native_output, mode, max_lines):
    if mode == "first":
        size = min(len(python_output), len(native_output))
        offset = 0
        for offset in range(0, size, 65536):
            left = native_output[offset : offset + 65536]
            right = python_output[offset : offset + 65536]
            if left != right:
                offset += next(
                    (i for i, (a, b) in enumerate(zip(left, right)) if a != b),
                    min(len(left), len(right)),
                )
                break
        else:
            offset = size
        start, end = max(0, offset - 180), offset + 360
        print(f"First AST difference at character {offset}")
        print(f"  C++: {native_output[start:end]!r}")
        print(f"  Python: {python_output[start:end]!r}")
        return
    if mode == "code":
        if launch_code_diff(case, python_output, native_output):
            return
        mode = "side-by-side"
    if mode == "side-by-side":
        print_side_by_side_diff(python_output, native_output, max_lines)
    else:
        print_unified_diff(case, python_output, native_output, max_lines)


def ignore_class_deduce(output):
    lines = output.splitlines(keepends=True)
    filtered = []
    index = 0

    while index < len(lines):
        line = lines[index]
        value = line.strip()
        if value == "ClassDeduce," or (value.startswith("ClassDeduce=[") and value.endswith("],")):
            index += 1
            continue
        if value == "ClassDeduce=[":
            end = index + 1
            while end < len(lines):
                if lines[end].strip().endswith("],"):
                    break
                end += 1
            if end < len(lines):
                index = end + 1
                continue
        filtered.append(line)
        index += 1

    return "".join(filtered)


@contextmanager
def use_build_stdlib(root, library_path):
    """Give both frontends the source stdlib and its build-generated tables."""
    stdlib = Path(os.environ.get("CODON_PATH", root / "stdlib"))
    generated = library_path.parent / "stdlib"
    if not (stdlib / "internal/core.codon").is_file() or not generated.is_dir():
        yield
        return
    if (stdlib / "internal/numeric/generated/float_dtoa_tables.codon").is_file():
        yield
        return
    directory = Path(tempfile.mkdtemp(prefix="meister-stdlib-"))
    owner_pid = os.getpid()
    previous = os.environ.get("CODON_PATH")
    try:
        merged = directory / "stdlib"
        shutil.copytree(stdlib, merged)
        shutil.copytree(generated, merged, dirs_exist_ok=True)
        os.environ["CODON_PATH"] = str(merged.resolve())
        yield
    finally:
        # An inherited generator can be finalized in a forked test worker. Only
        # its owner may restore the environment or remove the shared snapshot.
        if os.getpid() == owner_pid:
            if previous is None:
                os.environ.pop("CODON_PATH", None)
            else:
                os.environ["CODON_PATH"] = previous
            shutil.rmtree(directory)


def main(argv=None):
    root = Path(__file__).resolve().parent.parent
    if (root / "stdlib").is_dir():
        os.environ.setdefault("CODON_PATH", str(root / "stdlib"))
    argument_parser = argparse.ArgumentParser(
        description="Compare Python and C++ Codon AST dumps after scoping or typechecking."
    )
    argument_parser.add_argument(
        "tests",
        nargs="?",
        type=Path,
        default=default_test_path(root),
        help="Codon test file or directory (default: tests/, falling back to test/)",
    )
    argument_parser.add_argument("--library", help="path to libcodonc")
    argument_parser.add_argument(
        "--scope-only", action="store_true", help="compare parsing/scoping without typechecking"
    )
    argument_parser.add_argument(
        "--max-diffs", type=int, default=20, help="maximum mismatching cases to print"
    )
    argument_parser.add_argument(
        "--max-diff-lines", type=int, default=200, help="maximum lines per printed diff"
    )
    argument_parser.add_argument(
        "--diff",
        "--diff-mode",
        dest="diff_mode",
        choices=("side-by-side", "code", "unified", "first"),
        default="side-by-side",
        help="diff display (default: side-by-side)",
    )
    argument_parser.add_argument("--verbose", action="store_true")
    argument_parser.add_argument(
        "--barebones", action="store_true", help="force the barebones stdlib for all cases"
    )
    argument_parser.add_argument(
        "--strict", action="store_true", help="compare all AST fields without normalization"
    )
    argument_parser.add_argument(
        "--indent", type=int, default=2, help="AST dump indentation (0 for compact dumps)"
    )
    argument_parser.add_argument(
        "--reuse-stdlib",
        action="store_true",
        help="fork isolated workers from a preloaded Python stdlib (requires --indent 0)",
    )
    argument_parser.add_argument("--stop-on-error", action="store_true")
    args = argument_parser.parse_args(argv)
    if args.reuse_stdlib and (args.indent or args.scope_only or not hasattr(os, "fork")):
        argument_parser.error("--reuse-stdlib requires --indent 0, typechecking and fork support")

    tests = args.tests.resolve()
    if not tests.exists():
        argument_parser.error(f"test path does not exist: {tests}")

    try:
        native, library_path = find_library(root, args.library)
    except RuntimeError as error:
        argument_parser.error(str(error))

    cases = discover_tests(tests)
    if not cases:
        argument_parser.error(f"no .codon tests found under {tests}")

    with use_build_stdlib(root, library_path):
        return compare_tests(args, cases, native, library_path)


def compare_tests(args, cases, native, library_path):
    matched = 0
    mismatched = 0
    rejected = 0
    printed = 0
    stage = "Scoped" if args.scope_only else "Typechecked"
    early_defines = native.get_early_defines() if not args.scope_only else None
    print(f"Comparing {len(cases)} {stage.lower()} ASTs using {library_path}")
    prepared = {}

    for case in cases:
        typecheck = 0 if args.scope_only else 2 if args.barebones or case.barebones else 1
        if args.reuse_stdlib:
            if typecheck not in prepared:
                prepared[typecheck] = prepare_python(typecheck, early_defines)
            python_result, native_result = run_isolated_case(
                prepared[typecheck], case, native, typecheck, early_defines, args.indent
            )
        else:
            python_result = run_python(
                case, typecheck=typecheck, early_defines=early_defines, indent=args.indent
            )
            native_result = native.run(case, typecheck=typecheck, indent=args.indent)

        if args.verbose and (python_result.error or native_result.error):
            print(f"  C++: {(native_result.error or 'produced an AST').splitlines()[0]}")
            print(f"  Python: {(python_result.error or 'produced an AST').splitlines()[0]}")

        if python_result.output is not None and native_result.output is not None:
            raw_match = python_result.output == native_result.output
            python_output = (
                python_result.output
                if args.scope_only or args.strict or raw_match
                else ignore_class_deduce(python_result.output)
            )
            native_output = (
                native_result.output
                if args.scope_only or args.strict or raw_match
                else ignore_class_deduce(native_result.output)
            )
            if python_output == native_output:
                matched += 1
                print(f"MATCH {case.label}")
            else:
                mismatched += 1
                print(f"MISMATCH {case.label}")
                if printed < args.max_diffs:
                    show_diff(
                        case,
                        python_output,
                        native_output,
                        args.diff_mode,
                        args.max_diff_lines,
                    )
                    printed += 1
                if args.stop_on_error:
                    break
        elif (
            python_result.error is not None
            and native_result.error is not None
            and not native_result.error.startswith("test worker exited unexpectedly")
        ):
            rejected += 1
            print(f"MATCH-ERROR {case.label}")
        else:
            mismatched += 1
            print(f"MISMATCH {case.label}")
            if printed < args.max_diffs:
                print(f"  C++: {native_result.error or 'produced an AST'}")
                print(f"  Python: {python_result.error or 'produced an AST'}")
                printed += 1
            if args.stop_on_error:
                break

    print(f"{stage} ASTs: {matched} matched, {mismatched} mismatched, {rejected} rejected by both")
    return 1 if mismatched else 0


if __name__ == "__main__":
    raise SystemExit(main())
