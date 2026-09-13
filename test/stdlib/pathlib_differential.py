"""Compare native pathlib lexical and glob behavior with CPython 3.14+."""

import argparse
import itertools
import fnmatch
import os
from pathlib import Path, PureWindowsPath
import random
import re
import subprocess
import sys
import tempfile


PROBE = '''from pathlib import Path
from pathlib import _pattern, _as_uri
from os._ntpath import splitdrive
import sys

for line in sys.stdin:
    fields = line.rstrip("\\n").split("\\t")
    path = Path(fields[0])
    if sys.argv[1] == "lexical":
        print("\\t".join([str(path), path.drive, path.root, path.anchor, path.name,
                         path.stem, path.suffix, repr(path.suffixes),
                         repr(list(path.parts)), str(path.parent),
                         repr([str(parent) for parent in path.parents]),
                         str(path.is_absolute())]))
    elif sys.argv[1] == "wildcard":
        print(_pattern(fields[1], fields[2] == "True").fullmatch(fields[0]))
    elif sys.argv[1] == "class":
        sensitive = fields[2] == "True"
        print(str(_pattern(fields[1], sensitive).fullmatch(fields[0])) + "\\t" + str(path.full_match(fields[1], case_sensitive=sensitive)))
    elif sys.argv[1] == "uri":
        print(_as_uri(fields[0].replace("\\\\", "/"), splitdrive(fields[0])[0]))
    elif sys.argv[1] == "match":
        sensitive: Optional[bool] = None
        if len(fields) > 2:
            sensitive = fields[2] == "True"
        print(str(path.match(fields[1], case_sensitive=sensitive)) + "\\t" + str(path.full_match(fields[1], case_sensitive=sensitive)))
    else:
        sensitive: Optional[bool] = None
        if len(fields) > 2:
            sensitive = fields[2] == "True"
        recurse = len(fields) > 3 and fields[3] == "True"
        if sys.argv[1] == "rglob":
            print(repr(sorted(str(found.relative_to(path)) for found in path.rglob(fields[1], case_sensitive=sensitive, recurse_symlinks=recurse))))
        else:
            print(repr(sorted(str(found.relative_to(path)) for found in path.glob(fields[1], case_sensitive=sensitive, recurse_symlinks=recurse))))
'''


def compare(binary, operation, rows, expected):
    result = subprocess.run(
        [str(binary), operation], input="\n".join(rows) + "\n",
        text=True, capture_output=True, check=True, timeout=120,
    )
    actual = result.stdout.splitlines()
    if len(actual) != len(expected):
        raise AssertionError(f"{operation}: expected {len(expected)} rows, got {len(actual)}")
    failures = [
        f"{row!r}: CPython={wanted!r}, Codon={got!r}"
        for row, wanted, got in zip(rows, expected, actual) if wanted != got
    ]
    if result.stderr or failures:
        raise AssertionError(result.stderr + "\n".join(failures[:20]) + f"\n{len(failures)} mismatches")
    return len(rows)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--codon", type=Path, default=Path("build/codon"))
    parser.add_argument("--seed", type=int, default=1729)
    args = parser.parse_args()
    if sys.version_info < (3, 14):
        parser.error("CPython 3.14+ is required for single-dot suffix and full_match semantics")
    with tempfile.TemporaryDirectory(prefix="codon-pathlib-") as temporary:
        root = Path(temporary)
        source, binary = root / "probe.codon", root / "probe"
        source.write_text(PROBE)
        subprocess.run([str(args.codon.resolve()), "build", "-release", "-o", str(binary), str(source)],
                       check=True, timeout=120)
        paths = ["", ".", "..", "...", "/", "//", "///"]
        for prefix, first, last in itertools.product(
            ["", "/", "//", "///"],
            ["a", ".", "..", ".hidden", "a.b", "café", "a\\b"],
            ["", ".", "..", "...", "file.txt", ".profile", "file.", ".a.", "a..b"],
        ):
            paths.append(prefix + first + "//" + last)
        expected = []
        for value in paths:
            path = Path(value)
            expected.append("\t".join([
                str(path), path.drive, path.root, path.anchor, path.name, path.stem,
                path.suffix, repr(path.suffixes), repr(list(path.parts)), str(path.parent),
                repr([str(parent) for parent in path.parents]), str(path.is_absolute()),
            ]))
        lexical_count = compare(binary, "lexical", paths, expected)
        names = [".", "/", "/a", "/a/b", "a", "a/b", "a/b.txt", "a/B.TXT",
                 "a/b/c", ".hidden", "a[", "a]", "café", "-", "z"]
        patterns = ["*", "**", "?", "*.txt", "a/*", "a/**", "**/b", "**/c",
                    "*/*", "**/*", "/", "/*", "/**", "/a/*", "a/?",
                    "[a-z]", "[!a]", "[z-a]", "[!z-a]", "[[]", "[]]", "[a-]",
                    "[", "*.*", "**/**", "a/**/c", "[a--!]", "[a-!!]", "[b-a!]"]
        pairs = list(itertools.product(names, patterns))
        match_count = compare(binary, "match", ["\t".join(pair) for pair in pairs], [
            f"{Path(value).match(pattern)}\t{Path(value).full_match(pattern)}"
            for value, pattern in pairs
        ])
        randomizer = random.Random(args.seed)
        alphabet = "abcxyz-]!^&~éÉİıſKσςµΜßẞÅÅιιﬅﬆ"
        classes = ["[a--b]", "[a-b-c]", "[]a]", "[!]]", "[z-a]", "[!z-a]", "[İ]", "[A-Z]",
               "[A-z]", "[À-ſ]", "[Α-Ͽ]", "[K-K]", "[", "[!", "[]", "[a-", "[-]", "[--]", "[!--]", "İ", "ς", "ẞ"]
        classes += ["[" + "".join(randomizer.choices(alphabet, k=randomizer.randrange(1, 9))) + "]"
                    for _ in range(500)]
        fuzz = [(character, pattern, sensitive) for pattern in classes
                for character in alphabet for sensitive in [True, False]]
        fuzz_count = compare(binary, "wildcard", [f"{value}\t{pattern}\t{sensitive}" for value, pattern, sensitive in fuzz], [
            str(re.compile(fnmatch.translate(pattern), 0 if sensitive else re.IGNORECASE).fullmatch(value) is not None)
            for value, pattern, sensitive in fuzz
        ])
        class_count = 0
        class_alphabet = "abcXYZ012-!]"
        for size in range(1, 5):
            rows, expected = [], []
            for body in itertools.product(class_alphabet, repeat=size):
                pattern = "[" + "".join(body) + "]"
                for sensitive in [True, False]:
                    matcher = re.compile(fnmatch.translate(pattern), 0 if sensitive else re.IGNORECASE)
                    for character in class_alphabet:
                        rows.append(f"{character}\t{pattern}\t{sensitive}")
                        expected.append(f"{matcher.fullmatch(character) is not None}\t{Path(character).full_match(pattern, case_sensitive=sensitive)}")
            class_count += compare(binary, "class", rows, expected)
        codepoints = sorted(set(itertools.chain(range(0xC0, 0x530), range(0x1E00, 0x2000), range(33, 0x10000, 31), map(ord, alphabet))))
        characters = [chr(codepoint) for codepoint in codepoints
                      if not 0xD800 <= codepoint <= 0xDFFF and not chr(codepoint).isspace()
                      and chr(codepoint) not in "/\\"]
        unicode_patterns = ["[a-z]", "[A-Z]", "[À-Ö]", "[Σ-ϵ]", "[!a-z]", "[K-K]", "[İ-İ]", "[ẞ-ẞ]"]
        unicode_patterns += [f"[{chr(start)}-{chr(end)}]" for start, end in
                             [sorted(randomizer.sample(range(0x20, 0xD800), 2)) for _ in range(24)]]
        rows, expected = [], []
        for pattern in unicode_patterns:
            matcher = re.compile(fnmatch.translate(pattern), re.IGNORECASE)
            for character in characters:
                rows.append(f"{character}\t{pattern}\tFalse")
                expected.append(f"{matcher.fullmatch(character) is not None}\t{Path(character).full_match(pattern, case_sensitive=False)}")
        unicode_count = compare(binary, "class", rows, expected)
        fuzz_paths = []
        for _ in range(5000):
            value = randomizer.choice(["", "/", "//"]) + "/".join(randomizer.choices(["a", "b", "É", "İ", "ς", "x.txt"], k=randomizer.randrange(1, 5)))
            pattern = randomizer.choice(["", "/", "//"]) + "/".join(randomizer.choices(["*", "**", "?", "[A-Z]", "a", "*.txt", *classes[:8]], k=randomizer.randrange(1, 5)))
            fuzz_paths.append((value, pattern, randomizer.choice([True, False])))
        recursive_patterns = ["a/*.py", "**/*.py", "a/**/b", "a/**/**/b", "/a/**", "**", "**/**", "a/**/**", "**/*", "", "."]
        recursive_paths = ["", ".", "/", "//", "a", "a/b", "a/x/b", "a/x/y/b", "a/test.py", "test.py", "/a", "/a/b", "//a/b"]
        fuzz_paths += list(itertools.product(recursive_paths, recursive_patterns[:-2], [True, False]))
        match_count += compare(binary, "match", [f"{value}\t{pattern}\t{sensitive}" for value, pattern, sensitive in fuzz_paths], [
            f"{Path(value).match(pattern, case_sensitive=sensitive)}\t{Path(value).full_match(pattern, case_sensitive=sensitive)}"
            for value, pattern, sensitive in fuzz_paths
        ])
        empty_rows = [f"{value}\t{pattern}\t{sensitive}" for value, pattern, sensitive in
                      itertools.product(recursive_paths, recursive_patterns[-2:], [True, False])]
        class_count += compare(binary, "class", empty_rows, [
            f"{re.fullmatch(fnmatch.translate(pattern), value, 0 if sensitive else re.IGNORECASE) is not None}\t{Path(value).full_match(pattern, case_sensitive=sensitive)}"
            for value, pattern, sensitive in itertools.product(recursive_paths, recursive_patterns[-2:], [True, False])
        ])
        windows_paths = [PureWindowsPath(prefix + tail) for prefix, tail in itertools.product(
            ["C:/", "z:/", "//server/share/", "//server/sh are/", "//?/C:/", "//?/UNC/server/share/"],
            ["foo:bar", "a b/#%", "café", "one/two:stream", ""],
        )]
        uri_count = compare(binary, "uri", [str(path) for path in windows_paths], [path.as_uri() for path in windows_paths])
        tree = root / "tree"
        (tree / "one" / "deep").mkdir(parents=True)
        (tree / "two").mkdir()
        for name in ["a.txt", "a.TXT", ".hidden", "one/b.txt", "one/deep/c.txt", "two/d.py"]:
            (tree / name).touch()
        if os.name == "posix":
            (tree / "loop").symlink_to(".", target_is_directory=True)
            (tree / "dangling").symlink_to("missing")
        patterns = ["*", ".*", "*/", "*.txt", "**", "**/", "**/*", "**/*.txt",
                    "**/**/c.txt", "one/../*.txt", "one/*", "one/**", "[ot]*/*",
                    "missing/*", "loop/*", "dangling", "**/loop/*"]
        glob_count = compare(binary, "glob", [f"{tree}\t{pattern}" for pattern in patterns], [
            repr(sorted(str(found.relative_to(tree)) for found in tree.glob(pattern)))
            for pattern in patterns
        ])
        glob_count += compare(binary, "rglob", [f"{tree}\t"], [
            repr(sorted(str(found.relative_to(tree)) for found in tree.rglob("")))
        ])
        if os.name == "posix":
            (tree / "loop").unlink()
            (tree / "alias").symlink_to("one", target_is_directory=True)
        for name in alphabet:
            (tree / name).touch()
        glob_cases = list(itertools.product(classes, [True, False], [False, True]))
        glob_cases += list(itertools.product(["**", "**/", "**/*.txt", "**/[A-Z]*", "alias/**"], [True, False], [False, True]))
        glob_count += compare(binary, "glob", [f"{tree}\t{pattern}\t{sensitive}\t{recurse}" for pattern, sensitive, recurse in glob_cases], [
            repr(sorted(str(found.relative_to(tree)) for found in tree.glob(pattern, case_sensitive=sensitive, recurse_symlinks=recurse)))
            for pattern, sensitive, recurse in glob_cases
        ])
        rglob_cases = list(itertools.product([tree, tree / "missing", tree / "a.txt"], ["", ".", "*.txt"], [False, True]))
        glob_count += compare(binary, "rglob", [f"{base}\t{pattern}\tTrue\t{recurse}" for base, pattern, recurse in rglob_cases], [
            repr(sorted(str(found.relative_to(base)) for found in base.rglob(pattern, case_sensitive=True, recurse_symlinks=recurse)))
            for base, pattern, recurse in rglob_cases
        ])
        print(f"PASS: {lexical_count} lexical, {match_count} matching, {fuzz_count} wildcard, {class_count} short-class, {unicode_count} Unicode-class, {uri_count} Windows URI, {glob_count} glob cases (seed={args.seed})")


if __name__ == "__main__":
    main()
