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
from pathlib import Path

if __package__:
    from .converted import ast, cache, parser
else:
    sys.path.insert(0, str(Path(__file__).resolve().parent.parent))
    from meister.converted import ast, cache, parser


class TestCase:
    def __init__(self, path, name="", code=None, line=0):
        self.path = path
        self.name = name
        self.code = code
        self.line = line

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

        self.dump_code = self.library.codon_ast_parse_scope_dump_code
        self.dump_code.argtypes = [
            ctypes.c_char_p,
            ctypes.c_char_p,
            ctypes.c_int,
            ctypes.c_uint8,
            ctypes.c_int,
        ]
        self.dump_code.restype = _NativeResult

        self.dump_file = self.library.codon_ast_parse_scope_dump_file
        self.dump_file.argtypes = [ctypes.c_char_p, ctypes.c_uint8, ctypes.c_int]
        self.dump_file.restype = _NativeResult

        self.free = self.library.codon_ast_dump_free
        self.free.argtypes = [ctypes.c_void_p]
        self.free.restype = None

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

    def run(self, case):
        filename = os.fsencode(case.path)
        if case.code is None:
            return self._consume(self.dump_file(filename, True, 2))
        return self._consume(
            self.dump_code(case.code.encode("utf-8"), filename, case.line, True, 2)
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

    for line_number, line in enumerate(lines):
        if line.startswith("#%%"):
            if saw_marker and name != "__ignore__":
                cases.append(TestCase(path, name, "".join(code), start_line))
            elif not saw_marker and code:
                cases.append(TestCase(path, "", "".join(code), start_line))

            metadata = line[4:].split(",")
            name = metadata[0] if metadata else ""
            code = [line + "\n"]
            start_line = line_number
            saw_marker = True
        else:
            code.append(line + "\n")

    if code and name != "__ignore__":
        cases.append(TestCase(path, name, "".join(code), start_line))
    return cases


def discover_tests(path):
    if path.is_file():
        paths = [path]
    else:
        paths = sorted(path.rglob("*.codon"))
    cases = []
    for test_path in paths:
        cases.extend(split_test_file(test_path))
    return cases


def run_python(case):
    try:
        cc = cache.Cache("codon")
        if case.code is None:
            node = parser.parse(file=str(case.path))
        else:
            node = parser.parse(file=None, code=case.code)
        node = cc.scope(node)
        return Result(ast.dump(node, indent=2, include_attributes=True))
    except Exception as error:
        msg = ""
        if hasattr(error, "info"):
            msg = f" ({case.path}:{error.info.line}:{error.info.col})"
        return Result(error=f"{type(error).__name__}: {error}{msg}\n\n{traceback.format_exc()}")


def default_test_path(root):
    for path in (Path.cwd() / "tests", Path.cwd() / "test", root / "tests", root / "test"):
        if path.exists():
            return path
    return root / "tests"


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

    print(
        f"  {'C++ Codon':<{side_width}} | "
        f"{'Python port':<{side_width}}"
    )
    print(f"  {'-' * side_width}-+-{'-' * side_width}")

    for marker, native_index, native_line, python_index, python_line in rows[:max_lines]:
        native_number = "" if native_index is None else str(native_index + 1)
        python_number = "" if python_index is None else str(python_index + 1)
        native_side = (
            f"{native_number:>{number_width}} "
            f"{_clip_diff_line(native_line, text_width)}"
        )
        python_side = (
            f"{python_number:>{number_width}} "
            f"{_clip_diff_line(python_line, text_width)}"
        )
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
        if value == "ClassDeduce," or (
            value.startswith("ClassDeduce=[") and value.endswith("],")
        ):
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


def main(argv=None):
    root = Path(__file__).resolve().parent.parent
    argument_parser = argparse.ArgumentParser(
        description="Compare Python and C++ Codon AST dumps immediately after scoping."
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
        "--max-diffs", type=int, default=20, help="maximum mismatching cases to print"
    )
    argument_parser.add_argument(
        "--max-diff-lines", type=int, default=200, help="maximum lines per printed diff"
    )
    argument_parser.add_argument(
        "--diff",
        "--diff-mode",
        dest="diff_mode",
        choices=("side-by-side", "code", "unified"),
        default="side-by-side",
        help="diff display (default: side-by-side)",
    )
    argument_parser.add_argument("--verbose", action="store_true")
    args = argument_parser.parse_args(argv)

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

    matched = 0
    mismatched = 0
    rejected = 0
    printed = 0
    print(f"Comparing {len(cases)} scoped ASTs using {library_path}")

    for case in cases:
        python_result = run_python(case)
        native_result = native.run(case)

        if python_result.output is not None and native_result.output is not None:
            python_output = ignore_class_deduce(python_result.output)
            native_output = ignore_class_deduce(native_result.output)
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
                # break
        elif python_result.error is not None and native_result.error is not None:
            rejected += 1
            print(f"MATCH-ERROR {case.label}")
        else:
            mismatched += 1
            print(f"MISMATCH {case.label}")
            if printed < args.max_diffs:
                print(f"  C++: {native_result.error or 'produced an AST'}")
                print(f"  Python: {python_result.error or 'produced an AST'}")
                printed += 1
            # break

    print(
        f"Scoped ASTs: {matched} matched, {mismatched} mismatched, "
        f"{rejected} rejected by both"
    )
    return 1 if mismatched else 0


if __name__ == "__main__":
    raise SystemExit(main())
