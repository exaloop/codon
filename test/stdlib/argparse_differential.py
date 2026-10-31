"""Compare native argparse parsing and help with CPython 3.14."""

import argparse
import itertools
import json
import os
from pathlib import Path
import random
import subprocess
import sys
import tempfile


DEFINITIONS = r'''
import argparse

class PrefixChoices:
    def __iter__(self):
        yield "item-*"

    def __contains__(self, value: str) -> bool:
        return value.startswith("item-")

class CustomFormatter(argparse.HelpFormatter):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)

    def _format_action(self, action: argparse.Action) -> str:
        return super()._format_action(action) + "\n"

def make_parser(case):
    parser = argparse.ArgumentParser(prog="tool", exit_on_error=False, color=False)
    if case == 0:
        parser.add_argument("-a", action="store_true")
        parser.add_argument("-b", action="count")
        parser.add_argument("-n", "--number", type=int, default="3")
        parser.add_argument("--name", nargs="?", const="constant")
        parser.add_argument("rest", nargs="*")
    elif case == 1:
        parser.add_argument("first", nargs="?")
        parser.add_argument("middle", nargs="*")
        parser.add_argument("last")
        parser.add_argument("--flag", action="store_true")
    elif case == 2:
        parser.add_argument("--values", nargs="+", type=int)
        parser.add_argument("--pair", nargs=2, metavar=("LEFT", "RIGHT"))
        parser.add_argument("rest", nargs="*")
    elif case == 3:
        parser.add_argument("--foo")
        parser.add_argument("--food")
        parser.add_argument("-f")
        parser.add_argument("-foobar")
        parser.add_argument("rest", nargs="*")
    elif case == 4:
        parser.add_argument("--feature", action=argparse.BooleanOptionalAction, help="feature")
        parser.add_argument("-a", action="append", default=["start"])
        parser.add_argument("-e", action="extend", nargs="?")
        parser.add_argument("-c", action="append_const", const=2)
        parser.add_argument("--off", action="store_false")
    elif case == 5:
        group = parser.add_mutually_exclusive_group(required=True)
        group.add_argument("--one", action="store_true", help="one")
        group.add_argument("--two", nargs="?", const="two")
        parser.add_argument("rest", nargs="*")
    elif case == 6:
        parser.add_argument("first", nargs="?")
        parser.add_argument("remainder", nargs=argparse.REMAINDER)
        parser.add_argument("--flag", action="store_true")
    elif case == 7:
        parser.add_argument("--remainder", nargs=argparse.REMAINDER)
        parser.add_argument("rest", nargs="*")
    elif case == 8:
        parser.add_argument("-1", action="store_true")
        parser.add_argument("--number", type=float)
        parser.add_argument("rest", nargs="*")
    elif case == 9:
        parser.add_argument("--choice", choices=["apple", "pear", "peach"], help="choose %(choices)s")
        parser.add_argument("--integer", type=int, choices=[1, 2, 3])
        parser.add_argument("optional", nargs="?", choices=["a", "b"])
    elif case == 10:
        commands = parser.add_subparsers(dest="command", required=True, title="commands", help="command help")
        child = commands.add_parser("run", aliases=["r"], help="run inputs", exit_on_error=False)
        child.add_argument("files", nargs="+")
        child.add_argument("--count", type=int, default=1)
        other = commands.add_parser("stop", help="stop", exit_on_error=False)
        other.add_argument("--force", action="store_true")
        parser.add_argument("--verbose", action="store_true")
    elif case == 11:
        parser = argparse.ArgumentParser(prog="tool", prefix_chars="+-", exit_on_error=False, color=False)
        parser.add_argument("+a", action="count")
        parser.add_argument("++foo")
        parser.add_argument("-n", type=int)
        parser.add_argument("rest", nargs="*")
    elif case == 12:
        parser.allow_abbrev = False
        parser.add_argument("--long")
        parser.add_argument("-a", action="store_true")
        parser.add_argument("-b", action="store_true")
        parser.add_argument("rest", nargs="*")
    elif case == 13:
        parser = argparse.ArgumentParser(prog="tool", argument_default=argparse.SUPPRESS,
                                         exit_on_error=False, color=False)
        parser.add_argument("--name")
        parser.add_argument("rest", nargs="*")
        parser.add_argument("--count", type=int, default="3")
    elif case == 14:
        parser.add_argument("--required", required=True, help="required value")
        parser.add_argument("--secret", help=argparse.SUPPRESS)
        parser.add_argument("tail", nargs="+", metavar=("FIRST", "REST"))
    elif case == 15:
        parser.description = "A longer description with repeated   whitespace and a long-hyphenated-word."
        parser.epilog = "End of %(prog)s.\nMore words on the next line."
        parser.add_argument("--a-very-long-option-name", metavar="VALUE", help="A long help string which should wrap across multiple lines without losing words or indentation.")
        parser.add_argument("-n", "--number", type=int, default=3, help="%(type)s: %(default)04d; 100%%")
        parser.add_argument("input", help="Input file.")
    elif case == 16:
        parser = argparse.ArgumentParser(prog="tool", formatter_class=argparse.ArgumentDefaultsHelpFormatter,
                                         exit_on_error=False, color=False)
        parser.add_argument("--number", default=3, help="Number.")
        parser.add_argument("--flag", action="store_true", help="Flag.")
        parser.add_argument("rest", nargs="*", help="Files.")
    elif case == 17:
        parser = argparse.ArgumentParser(prog="tool", formatter_class=argparse.RawTextHelpFormatter,
                                         description="First line.\n  Indented second.\n\nLast.",
                                         epilog="Epilog.\n  Keep indentation.", exit_on_error=False, color=False)
        parser.add_argument("--name", help="First line.\n  Indented second.")
    elif case == 18:
        parser = argparse.ArgumentParser(prog="tool", usage="%(prog)s CUSTOM [args]", color=False, exit_on_error=False)
        parser.add_argument("--name", nargs="?")
    elif case == 19:
        group = parser.add_mutually_exclusive_group()
        group.add_argument("--one", action="store_true")
        group.add_argument("positional", nargs="?")
        parser.add_argument("--two", help="two")
    elif case == 20:
        parser.suggest_on_error = True
        parser.add_argument("--choice", choices=["apple", "pear", "peach", "appel"])
    elif case == 21:
        parser.add_argument("--first", dest="shared", default="1", type=int)
        parser.add_argument("--second", dest="shared", default="2", type=int)
        parser.add_argument("--choice", nargs="?", const="9", default="8", type=int, choices=[1, 2])
        parser.add_argument("rest", nargs="?", default=argparse.SUPPRESS, type=int)
    elif case == 22:
        parser.add_argument("--name")
        parser.add_argument("name", nargs="?", default="default")
    elif case == 23:
        parser.add_argument("-o", "--old", action="count", deprecated=True)
        parser.add_argument("--version", action="version", version="%(prog)s 1.2")
        commands = parser.add_subparsers(dest="command")
        commands.add_parser("old", deprecated=True, exit_on_error=False)
    elif case == 24:
        parser.description = "ab-cd very-long-hyphenated-word example--word a-b-c-d abc abcdefghijklmnopqrstuvwxyz0123456789"
        group = parser.add_argument_group("wrapping", description="Short words and a rather-long-compound-word with punctuation--inside.")
        group.add_argument("--value", help="long-word-with-hyphens --flag-like -x and abcdefghijklmnopqrstuvwxyz0123456789 followed by more.")
    elif case == 25:
        choices = ["first"]
        parser.add_argument("--value", choices=choices)
        choices.append("second")
        choices.pop(0)
        parser.add_argument("--substring", choices="abc")
        parser.add_argument("--range", type=int, choices=range(2, 20, 3))
    elif case == 26:
        parser.add_argument("value", choices=PrefixChoices())
    elif case == 27:
        parser = argparse.ArgumentParser(prog="tool", color=False, formatter_class=CustomFormatter, exit_on_error=False)
        parser.add_argument("path")
        parser.usage = "CUSTOM"
        commands = parser.add_subparsers()
        commands.add_parser("run", exit_on_error=False)
    elif case == 28:
        parser.add_argument("--count", action="count", type=int)
    elif case == 29:
        parser.add_argument("--flag", action="store_true", choices=None)
    elif case == 30:
        parser.add_argument("--value", nargs=0)
    elif case == 31:
        parser.add_argument("--value", action="append", nargs=0)
    elif case == 32:
        parser.add_argument("value", required=False)
    elif case == 33:
        parser.add_argument("--value", type="unregistered")
    elif case == 34:
        default = [1]
        parser.add_argument("--item", action="append", type=int, default=default)
        parser.set_defaults(shared=default)
        default.append(2)
    elif case == 35:
        parent = argparse.ArgumentParser(add_help=False)
        group = parent.add_mutually_exclusive_group(required=True)
        group.add_argument("--one", action="store_true")
        group.add_argument("--two", action="store_true")
        parser = argparse.ArgumentParser(prog="tool", parents=[parent], exit_on_error=False, color=False)
    return parser
'''

PROBE = '''
import json
import sys

def native(value: argparse.ArgumentValue) -> json.JSON:
    if value.kind == argparse.VALUE_ARRAY: return json.JSON([native(item) for item in value.values()])
    if value.kind == argparse.VALUE_STR: return json.JSON(value.as_str())
    if value.kind == argparse.VALUE_BOOL: return json.JSON(value.as_bool())
    if value.kind == argparse.VALUE_INT: return json.JSON(value.as_int())
    if value.kind == argparse.VALUE_FLOAT: return json.JSON(value.as_float())
    return json.JSON(None)

for line in sys.stdin:
    print(line.strip(), file=sys.stderr, flush=True)
    request = json.loads(line)
    mode = request["mode"].as_str()
    arguments = [value.as_str() for value in request["args"].values()]
    try:
        parser = make_parser(request["case"].as_int())
        parser.color = request["color"].as_bool()
        result = json.JSON.object()
        result["status"] = "ok"
        if mode == "help": result["text"] = parser.format_help()
        elif mode == "usage": result["text"] = parser.format_usage()
        else:
            extras = List[str]()
            if mode == "parse": namespace = parser.parse_args(arguments)
            elif mode == "known": namespace, extras = parser.parse_known_args(arguments)
            elif mode == "intermixed": namespace = parser.parse_intermixed_args(arguments)
            else: namespace, extras = parser.parse_known_intermixed_args(arguments)
            result["namespace"] = {key: native(value) for key, value in namespace._data.items()}
            result["extras"] = extras
        print(json.dumps(result))
    except Exception as error:
        print(json.dumps({"status": "error", "type": error.typename, "message": str(error)}))
'''

CLI_PROBE = '''
import sys
if len(sys.argv) > 1:
    if sys.argv[1] == "--terminal-width":
        print(argparse.HelpFormatter("tool", color=False)._width)
    else:
        parser = make_parser(int(sys.argv[1]))
        parser.exit_on_error = True
        parser.color = False
        parser.parse_args(sys.argv[2:])
'''

BENCHMARK = '''
import argparse
import time

parser = argparse.ArgumentParser(add_help=False, exit_on_error=False)
for index in range(200):
    parser.add_argument("--option-" + str(index), type=int)
parser.add_argument("-v", action="count", default=0)
parser.add_argument("files", nargs="*")
arguments = ["--option-" + str(index) + "=" + str(index) for index in range(200)]
arguments.extend(["file"] * 1000)
timings = []
for repeat in range(5):
    start = time.perf_counter()
    for iteration in range(100):
        parser.parse_args(arguments)
    timings.append((time.perf_counter() - start) / 100)
print(min(timings))
'''


def reference(make_parser, request):
    mode = request["mode"]
    try:
        parser = make_parser(request["case"])
        parser.color = request["color"]
        if mode in ("help", "usage"):
            return {"status": "ok", "text": getattr(parser, "format_" + mode)()}
        method = {"parse": "parse_args", "known": "parse_known_args",
                  "intermixed": "parse_intermixed_args", "known_intermixed": "parse_known_intermixed_args"}[mode]
        parsed = getattr(parser, method)(request["args"])
        namespace, extras = parsed if mode.startswith("known") else (parsed, [])
        return {"status": "ok", "namespace": vars(namespace), "extras": extras}
    except Exception as error:
        return {"status": "error", "type": type(error).__name__, "message": str(error)}


def requests(seed, count):
    rng = random.Random(seed)
    tokens = ["a", "b", "1", "2", "-1", "--", "--unknown", "-a", "-b", "-ab",
              "-n", "-n2", "--number=4", "--name", "--name=x", "--flag", "--one", "--two"]
    examples = {
        0: [["-abb", "--num=4", "a"], ["--name=", "--", "-a"], ["-abz"], ["-a=1"]],
        2: [["--values", "1", "2"], ["--pair", "--x", "a"], ["--pair=a", "b"]],
        3: [["--fo", "x"], ["-fo", "x"], ["-foobar", "x"], ["-fx"]],
        4: [["--no-feature"], ["-a", "x", "-c", "-e", "abc"]],
        5: [["--one"], ["--two"], ["--one", "--two"], ["--two=x"]],
        6: [["a", "--flag", "x"], ["--", "a", "--flag"], ["--flag", "a", "x"]],
        7: [["--remainder", "x", "--unknown", "a"], ["--remainder", "--", "a"]],
        9: [["--choice", "bad"], ["--integer", "2"], ["--integer", "9"], ["--choice", "pear", "a"]],
        10: [["run", "a"], ["--verbose", "r", "a", "--count", "2"], ["stop", "--force"], ["bad"], ["run", "a", "--unknown"]],
        11: [["+aa", "++foo=x"], ["++f", "a", "-n", "2"]],
        12: [["--lo", "x"], ["-ab"], ["--long=x"]],
        14: [["--required", "x", "a", "b"]],
        19: [["--one"], ["x"], ["--one", "x"]],
        20: [["--choice", word] for word in ("appl", "peah", "pear", "appeal", "bad", "pe", "")],
        21: [["--first", "4"], ["--second", "5"], ["--choice"], ["--choice", "9"]],
        22: [["--name", "given"], ["==SUPPRESS=="]],
        25: [["--value", "first"], ["--value", "second"], ["--substring", "ab"], ["--substring", ""], ["--range", "5"], ["--range", "6"]],
        26: [["item-12"], ["invalid"]],
        27: [["input", "run"]],
        34: [["--item", "3"], ["--item", "4", "--item", "5"]],
        35: [["--one"], ["--two"], ["--one", "--two"]],
    }
    for case in range(36):
        for mode in ("help", "usage"):
            yield {"case": case, "mode": mode, "args": []}
        arguments = [[], ["a"], ["a", "b"], ["--"], ["--unknown"], *examples.get(case, [])]
        arguments.extend([rng.choice(tokens) for unused in range(rng.randrange(7))] for unused in range(count))
        if case == 1:
            arguments.extend(list(values) for size in range(5) for values in itertools.product(["a", "--flag", "--unknown"], repeat=size))
        for argv in arguments:
            for mode in ("parse", "known", "intermixed", "known_intermixed"):
                yield {"case": case, "mode": mode, "args": argv}


def main():
    cli = argparse.ArgumentParser(description=__doc__)
    cli.add_argument("--codon", default="./build/codon")
    cli.add_argument("--mode", choices=("debug", "release"), default="release")
    cli.add_argument("--seed", type=int, default=42)
    cli.add_argument("--cases", type=int, default=20)
    cli.add_argument("--width", type=int, default=80)
    cli.add_argument("--color", action="store_true")
    cli.add_argument("--benchmark", action="store_true")
    args = cli.parse_args()
    environment = dict(os.environ, COLUMNS=str(args.width), NO_COLOR="1", PYTHON_COLORS="1" if args.color else "0")
    os.environ["COLUMNS"] = str(args.width)
    os.environ["PYTHON_COLORS"] = environment["PYTHON_COLORS"]
    scope = {}
    exec(DEFINITIONS, scope)
    pending = list(requests(args.seed, args.cases))
    for request in pending: request["color"] = args.color
    with tempfile.TemporaryDirectory(prefix="codon-argparse-") as directory:
        source, binary = Path(directory) / "probe.codon", Path(directory) / "probe"
        source.write_text(DEFINITIONS + CLI_PROBE + PROBE)
        subprocess.run([args.codon, "build", "-" + args.mode, "-o", str(binary), str(source)], check=True, env=environment)
        process = subprocess.run([str(binary)], input="".join(json.dumps(request) + "\n" for request in pending),
                                 text=True, capture_output=True, timeout=120, env=environment)
        if process.returncode:
            raise AssertionError(f"Probe exited {process.returncode}; last request:\n{process.stderr[-2000:]}")
        cli_checks = [(0, ["--help"]), (0, ["-n", "bad"]), (0, ["--unknown"]),
                      (1, []), (2, ["--pair", "one"]), (5, []), (5, ["--one", "--two"]),
                      (10, ["--help"]), (10, ["run", "--help"]), (10, ["wrong"]),
                      (23, ["--version"]), (23, ["--old", "--old"]),
                      (23, ["-oo", "old"]), (23, ["-o", "--old", "old"]),
                      (27, ["input", "run", "--help"])]
        for case, argv in cli_checks:
            expected = subprocess.run([sys.executable, "-c", DEFINITIONS + CLI_PROBE, str(case), *argv],
                                      input="", text=True, capture_output=True, env=environment, timeout=10)
            actual = subprocess.run([str(binary), str(case), *argv], input="", text=True,
                                    capture_output=True, env=environment, timeout=10)
            expected_result = expected.returncode, expected.stdout, expected.stderr
            actual_result = actual.returncode, actual.stdout, actual.stderr
            if actual_result != expected_result:
                raise AssertionError(f"CLI {case} {argv!r}\nCPython: {expected_result!r}\nCodon: {actual_result!r}")
        terminal_checks = 0
        if os.name == "posix":
            import fcntl
            import pty
            import struct
            import termios

            for columns, expected_width in ((None, 55), ("invalid", 55), ("0", 55), ("-1", 55), (" +61 ", 59)):
                terminal_environment = dict(environment)
                terminal_environment.pop("COLUMNS", None)
                if columns is not None: terminal_environment["COLUMNS"] = columns
                master, slave = pty.openpty()
                try:
                    fcntl.ioctl(slave, termios.TIOCSWINSZ, struct.pack("HHHH", 24, 57, 0, 0))
                    subprocess.run([str(binary), "--terminal-width"], stdin=subprocess.DEVNULL, stdout=slave,
                                   stderr=subprocess.PIPE, check=True, env=terminal_environment, timeout=10)
                    actual_width = int(os.read(master, 4096).strip())
                    if actual_width != expected_width:
                        raise AssertionError(f"Terminal COLUMNS={columns!r}: expected {expected_width}, got {actual_width}")
                    terminal_checks += 1
                finally:
                    os.close(master)
                    os.close(slave)
        if args.benchmark:
            benchmark_source = Path(directory) / "benchmark.codon"
            benchmark_binary = Path(directory) / "benchmark"
            benchmark_source.write_text(BENCHMARK)
            subprocess.run([args.codon, "build", "-release", "-o", str(benchmark_binary), str(benchmark_source)],
                           check=True, env=environment)
            native = subprocess.check_output([str(benchmark_binary)], text=True, env=environment, timeout=60)
            python = subprocess.check_output([sys.executable, "-c", BENCHMARK], text=True, env=environment, timeout=60)
            print(f"200 options + 1000 positionals: Codon {float(native) * 1000:.3f} ms; CPython {float(python) * 1000:.3f} ms")
    output = process.stdout.splitlines()
    if len(output) != len(pending):
        raise AssertionError(f"Expected {len(pending)} responses, got {len(output)}: {process.stderr[-2000:]}")
    failures = 0
    for request, line in zip(pending, output):
        expected, actual = reference(scope["make_parser"], request), json.loads(line)
        if actual != expected:
            failures += 1
            if failures <= 20:
                print(json.dumps(request))
                print("CPython:", expected)
                print("Codon:  ", actual)
    print(f"{len(pending)} comparisons; {failures} mismatches; {len(cli_checks)} CLI and {terminal_checks} terminal checks passed ({args.mode}, width={args.width})")
    raise SystemExit(bool(failures))


if __name__ == "__main__":
    main()
