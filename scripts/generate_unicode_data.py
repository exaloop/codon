#!/usr/bin/env python3
import argparse
import pathlib
import sys
import unicodedata

MAX_CPL = 0x10FFFF
PAGE_SHIFT = 8
PAGE_SIZE = 1 << PAGE_SHIFT
WORDS_PER_PAGE = PAGE_SIZE // 64
PAGE_COUNT = (MAX_CPL + 1) // PAGE_SIZE

PROPERTIES = {
    "alpha": lambda ch: ch.isalpha(),
    "decimal": lambda ch: ch.isdecimal(),
    "digit": lambda ch: ch.isdigit(),
    "numeric": lambda ch: ch.isnumeric(),
    "space": lambda ch: ch.isspace(),
    "printable": lambda ch: ch.isprintable(),
    "lower": lambda ch: ch.islower(),
    "upper": lambda ch: ch.isupper(),
    "title": lambda ch: unicodedata.category(ch) == "Lt",
    "cased": lambda ch: ch.islower() or ch.isupper() or unicodedata.category(ch) == "Lt",
    # Includes "_" exactly as Python permits it as an identifier start.
    "xid_start": lambda ch: ch.isidentifier(),
    # Tests whether ch can follow an ASCII identifier-start code point.
    "xid_continue": lambda ch: ("a" + ch).isidentifier(),
}

CASE_MAPPINGS = ("lower", "upper", "title", "casefold")

# A positive record encodes a one-code-point mapping as:
#     record = mapped_cp - source_cp + DIRECT_MAPPING_OFFSET
# Zero means identity. Negative records index the extended mapping table.
DIRECT_MAPPING_OFFSET = MAX_CPL + 1
EXTENDED_MAPPING_FLAG = -(1 << 31)


def build_pages(predicate):
    pages = []
    page_to_index = {}
    directory = []

    for page in range(PAGE_COUNT):
        words = [0] * WORDS_PER_PAGE
        first_cp = page << PAGE_SHIFT

        for offset in range(PAGE_SIZE):
            cp = first_cp + offset

            if predicate(chr(cp)):
                words[offset >> 6] |= 1 << (offset & 63)

        key = tuple(words)

        if not any(words):
            directory.append(-1)
        else:
            index = page_to_index.get(key)

            if index is None:
                index = len(pages)
                page_to_index[key] = index
                pages.append(key)

            directory.append(index)

    while directory and directory[-1] == -1:
        directory.pop()

    max_codepoint = (len(directory) << PAGE_SHIFT) - 1
    return directory, pages, max_codepoint


def llvm_array(values, llvm_type):
    return "        " + ", ".join(
        f"{llvm_type} {value}" for value in values
    )


def emit_property(out, name, directory, pages, max_codepoint):
    directory_count = len(directory)
    directory_values = llvm_array(directory, "i16")
    words = [word for page in pages for word in page]
    word_values = llvm_array(words, "i64")

    out.write(
        f"""
@pure
@llvm
def _unicode_{name}_page(page: int) -> i16:
    @data = private unnamed_addr constant [{directory_count} x i16] [{directory_values}]
    %p = getelementptr inbounds [{directory_count} x i16], ptr @data, i64 0, i64 %page
    %x = load i16, ptr %p, align 2
    ret i16 %x


@pure
@llvm
def _unicode_{name}_word(index: int) -> u64:
    @data = private unnamed_addr constant [{len(words)} x i64] [{word_values}]
    %p = getelementptr inbounds [{len(words)} x i64], ptr @data, i64 0, i64 %index
    %x = load i64, ptr %p, align 8
    ret i64 %x


def unicode_is_{name}(cp: int) -> bool:
    if cp < 0 or cp > {max_codepoint}:
        return False

    page = cp >> UNICODE_PAGE_SHIFT
    page_index = int(_unicode_{name}_page(page))

    if page_index < 0:
        return False

    word = _unicode_{name}_word(
        page_index * UNICODE_PAGE_WORDS + ((cp >> 6) & 3)
    )
    return ((word >> u64(cp & 63)) & u64(1)) != 0
"""
    )

def build_mapping_pages(method):
    pages = []
    page_to_index = {}
    directory = []
    extended_mappings = []
    extended_mapping_to_index = {}

    for page in range(PAGE_COUNT):
        records = [0] * PAGE_SIZE
        first_cp = page << PAGE_SHIFT

        for offset in range(PAGE_SIZE):
            cp = first_cp + offset
            mapped = getattr(chr(cp), method)()

            if mapped == chr(cp):
                continue

            codepoints = tuple(map(ord, mapped))
            assert 1 <= len(codepoints) <= 3, (method, cp, codepoints)

            if len(codepoints) == 1:
                records[offset] = (
                    codepoints[0] - cp + DIRECT_MAPPING_OFFSET
                )
            else:
                index = extended_mapping_to_index.get(codepoints)

                if index is None:
                    index = len(extended_mappings)
                    extended_mapping_to_index[codepoints] = index
                    extended_mappings.append(codepoints)

                records[offset] = EXTENDED_MAPPING_FLAG | index

        key = tuple(records)

        if not any(records):
            directory.append(-1)
        else:
            index = page_to_index.get(key)

            if index is None:
                index = len(pages)
                page_to_index[key] = index
                pages.append(key)

            directory.append(index)

    while directory and directory[-1] == -1:
        directory.pop()

    max_codepoint = (len(directory) << PAGE_SHIFT) - 1
    return directory, pages, extended_mappings, max_codepoint


def emit_mapping(out, name, directory, pages, extended_mappings, max_codepoint):
    directory_count = len(directory)
    directory_values = llvm_array(directory, "i16")
    records = [record for page in pages for record in page]
    record_values = llvm_array(records, "i32")

    # Each entry is [length, codepoint_0, codepoint_1, codepoint_2].
    # Zero-fill unused code point slots.
    mapping_data = [
        value
        for mapping in extended_mappings
        for value in (len(mapping), *mapping, *([0] * (3 - len(mapping))))
    ]

    # LLVM does not permit an empty array initializer.
    if not mapping_data:
        mapping_data = [0, 0, 0, 0]

    mapping_values = llvm_array(mapping_data, "i32")

    out.write(
        f"""
@pure
@llvm
def _unicode_{name}_mapping_page(page: int) -> i16:
    @data = private unnamed_addr constant [{directory_count} x i16] [{directory_values}]
    %p = getelementptr inbounds [{directory_count} x i16], ptr @data, i64 0, i64 %page
    %x = load i16, ptr %p, align 2
    ret i16 %x


@pure
@llvm
def _unicode_{name}_mapping_record(index: int) -> i32:
    @data = private unnamed_addr constant [{len(records)} x i32] [{record_values}]
    %p = getelementptr inbounds [{len(records)} x i32], ptr @data, i64 0, i64 %index
    %x = load i32, ptr %p, align 4
    ret i32 %x


@pure
@llvm
def _unicode_{name}_mapping_data(index: int) -> i32:
    @data = private unnamed_addr constant [{len(mapping_data)} x i32] [{mapping_values}]
    %p = getelementptr inbounds [{len(mapping_data)} x i32], ptr @data, i64 0, i64 %index
    %x = load i32, ptr %p, align 4
    ret i32 %x


def _unicode_{name}_mapping_lookup(cp: int) -> int:
    if cp < 0 or cp > {max_codepoint}:
        return 0

    page_index = int(_unicode_{name}_mapping_page(cp >> UNICODE_PAGE_SHIFT))

    if page_index < 0:
        return 0

    return int(
        _unicode_{name}_mapping_record(
            page_index * 256 + (cp & 255)
        )
    )


def unicode_{name}_mapping_length(cp: int) -> int:
    record = _unicode_{name}_mapping_lookup(cp)

    if record >= 0:
        return 1

    return int(
        _unicode_{name}_mapping_data(
            (record & 0x7FFFFFFF) * 4
        )
    )


def unicode_{name}_mapping_codepoint(cp: int, index: int) -> int:
    record = _unicode_{name}_mapping_lookup(cp)

    if record == 0:
        return cp

    if record > 0:
        return cp + record - {DIRECT_MAPPING_OFFSET}

    return int(
        _unicode_{name}_mapping_data(
            (record & 0x7FFFFFFF) * 4 + 1 + index
        )
    )
"""
    )


def parse_property_codepoints(path, property_name):
    codepoints = set()

    with path.open(encoding="utf-8") as source:
        for line in source:
            line = line.partition("#")[0].strip()

            if not line:
                continue

            codepoint_range, name = (
                part.strip() for part in line.split(";", maxsplit=1)
            )

            if name != property_name:
                continue

            start, separator, end = codepoint_range.partition("..")
            first = int(start, 16)
            last = int(end, 16) if separator else first
            codepoints.update(range(first, last + 1))

    return codepoints


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--output",
        type=pathlib.Path,
        default=pathlib.Path("stdlib/internal/types/unicode/data.codon"),
    )
    parser.add_argument(
        "--derived-core-properties",
        type=pathlib.Path,
        required=True,
        help="Path to Unicode DerivedCoreProperties.txt",
    )
    args = parser.parse_args()

    case_ignorable = parse_property_codepoints(
        args.derived_core_properties,
        "Case_Ignorable",
    )

    args.output.parent.mkdir(parents=True, exist_ok=True)

    with args.output.open("w", encoding="utf-8") as out:
        out.write(
            f"""# AUTO-GENERATED by scripts/generate_unicode_data.py.
# Python Unicode database: {unicodedata.unidata_version}
# Do not edit manually.

UNICODE_MAX: Literal[int] = 0x10FFFF
UNICODE_PAGE_SHIFT: Literal[int] = 8
UNICODE_PAGE_WORDS: Literal[int] = 4

"""
        )

        for name, predicate in PROPERTIES.items():
            print(f"generating {name}", file=sys.stderr)
            directory, pages, max_codepoint = build_pages(predicate)
            emit_property(out, name, directory, pages, max_codepoint)

            print(
                f"  {name}: {len(pages)} unique non-empty pages, "
                f"{len(pages) * WORDS_PER_PAGE * 8} bytes of page data",
                file=sys.stderr,
            )

        directory, pages, max_codepoint = build_pages(
            lambda ch: ord(ch) in case_ignorable
        )
        emit_property(out, "case_ignorable", directory, pages, max_codepoint)

        print(
            f"  case_ignorable: {len(pages)} unique non-empty pages, "
            f"{len(pages) * WORDS_PER_PAGE * 8} bytes of page data",
            file=sys.stderr,
        )

        for name in CASE_MAPPINGS:
            print(f"generating {name} mappings", file=sys.stderr)
            directory, pages, extended_mappings, max_codepoint = build_mapping_pages(
                name
            )
            emit_mapping(
                out, name, directory, pages, extended_mappings, max_codepoint
            )

            print(
                f"  {name}: {len(pages)} unique non-empty pages, "
                f"{len(extended_mappings)} extended mappings",
                file=sys.stderr,
            )


if __name__ == "__main__":
    main()
