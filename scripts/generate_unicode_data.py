#!/usr/bin/env python3
import argparse
import pathlib
import struct
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

CATEGORY_CODES = (
    "Cn", "Cc", "Cf", "Co", "Cs", "Ll", "Lm", "Lo", "Lt", "Lu",
    "Mc", "Me", "Mn", "Nd", "Nl", "No", "Pc", "Pd", "Pe", "Pf",
    "Pi", "Po", "Ps", "Sc", "Sk", "Sm", "So", "Zl", "Zp", "Zs",
)
CATEGORY_INDEX = {category: index for index, category in enumerate(CATEGORY_CODES)}

BIDIRECTIONAL_CODES = (
    "", "AL", "AN", "B", "BN", "CS", "EN", "ES", "ET", "FSI",
    "L", "LRE", "LRI", "LRO", "NSM", "ON", "PDF", "PDI", "R",
    "RLE", "RLI", "RLO", "S", "WS",
)
BIDIRECTIONAL_INDEX = {
    direction: index for index, direction in enumerate(BIDIRECTIONAL_CODES)
}

EAST_ASIAN_WIDTH_CODES = ("F", "A", "H", "N", "Na", "W")
EAST_ASIAN_WIDTH_INDEX = {
    width: index for index, width in enumerate(EAST_ASIAN_WIDTH_CODES)
}

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


def build_value_pages(value_for):
    pages = []
    page_to_index = {}
    directory = []

    for page in range(PAGE_COUNT):
        first_codepoint = page << PAGE_SHIFT
        values = [value_for(first_codepoint + offset) for offset in range(PAGE_SIZE)]
        key = tuple(values)

        if not any(values):
            directory.append(-1)
            continue

        page_index = page_to_index.get(key)
        if page_index is None:
            page_index = len(pages)
            page_to_index[key] = page_index
            pages.append(key)
        directory.append(page_index)

    return directory, pages


def emit_value_index(out, name, directory, pages):
    directory_values = llvm_array(directory, "i16")
    values = [value for page in pages for value in page]
    value_values = llvm_array(values, "i8")

    out.write(
        f"""
@pure
@llvm
def _unicode_{name}_page(page: int) -> i16:
    @data = private unnamed_addr constant [{len(directory)} x i16] [{directory_values}]
    %p = getelementptr inbounds [{len(directory)} x i16], ptr @data, i64 0, i64 %page
    %x = load i16, ptr %p, align 2
    ret i16 %x


@pure
@llvm
def _unicode_{name}_value(index: int) -> u8:
    @data = private unnamed_addr constant [{len(values)} x i8] [{value_values}]
    %p = getelementptr inbounds [{len(values)} x i8], ptr @data, i64 0, i64 %index
    %x = load i8, ptr %p, align 1
    ret i8 %x


def unicode_{name}_index(cp: int) -> int:
    if cp < 0 or cp > UNICODE_MAX:
        return 0

    page_index = int(_unicode_{name}_page(cp >> UNICODE_PAGE_SHIFT))
    if page_index < 0:
        return 0

    return int(_unicode_{name}_value(page_index * 256 + (cp & 255)))
"""
    )


def llvm_f64_array(values):
    return "        " + ", ".join(
        "double 0x" + struct.pack(">d", value).hex().upper()
        for value in values
    )


def emit_numeric(out, directory, pages, values):
    directory_values = llvm_array(directory, "i16")
    page_values = [value for page in pages for value in page]
    page_value_data = llvm_array(page_values, "i16")
    numeric_values = llvm_f64_array(values)

    out.write(
        f"""
@pure
@llvm
def _unicode_numeric_page(page: int) -> i16:
    @data = private unnamed_addr constant [{len(directory)} x i16] [{directory_values}]
    %p = getelementptr inbounds [{len(directory)} x i16], ptr @data, i64 0, i64 %page
    %x = load i16, ptr %p, align 2
    ret i16 %x


@pure
@llvm
def _unicode_numeric_value_index(index: int) -> i16:
    @data = private unnamed_addr constant [{len(page_values)} x i16] [{page_value_data}]
    %p = getelementptr inbounds [{len(page_values)} x i16], ptr @data, i64 0, i64 %index
    %x = load i16, ptr %p, align 2
    ret i16 %x


@pure
@llvm
def _unicode_numeric_value(index: int) -> float:
    @data = private unnamed_addr constant [{len(values)} x double] [{numeric_values}]
    %p = getelementptr inbounds [{len(values)} x double], ptr @data, i64 0, i64 %index
    %x = load double, ptr %p, align 8
    ret double %x


def unicode_numeric_index(cp: int) -> int:
    if cp < 0 or cp > UNICODE_MAX:
        return 0

    page_index = int(_unicode_numeric_page(cp >> UNICODE_PAGE_SHIFT))
    if page_index < 0:
        return 0

    return int(_unicode_numeric_value_index(page_index * 256 + (cp & 255)))


def unicode_numeric_value(cp: int) -> float:
    return _unicode_numeric_value(unicode_numeric_index(cp) - 1)
"""
    )


def build_numeric_pages():
    values = []
    value_to_index = {}

    def value_for(codepoint):
        try:
            value = unicodedata.numeric(chr(codepoint))
        except ValueError:
            return 0

        index = value_to_index.get(value)
        if index is None:
            index = len(values)
            value_to_index[value] = index
            values.append(value)
        return index + 1

    directory, pages = build_value_pages(value_for)
    return directory, pages, values


def build_name_data():
    records = []
    values = []
    for codepoint in range(MAX_CPL + 1):
        name = unicodedata.name(chr(codepoint), "")
        if not name:
            continue
        offset = len(values)
        values.extend(name.encode("ascii"))
        records.append((codepoint, offset, len(name), name))

    name_order = sorted(range(len(records)), key=lambda index: records[index][3])
    return records, values, name_order


def emit_names(out, records, values, name_order):
    # A record packs [code point: 32 bits, byte offset: 24 bits, length: 8 bits].
    record_values = [
        (codepoint << 32) | (offset << 8) | length
        for codepoint, offset, length, _ in records
    ]

    out.write(
        f"""
@pure
@llvm
def _unicode_name_record(index: int) -> u64:
    @data = private unnamed_addr constant [{len(record_values)} x i64] [{llvm_array(record_values, 'i64')}]
    %p = getelementptr inbounds [{len(record_values)} x i64], ptr @data, i64 0, i64 %index
    %x = load i64, ptr %p, align 8
    ret i64 %x


@pure
@llvm
def _unicode_name_byte(index: int) -> u8:
    @data = private unnamed_addr constant [{len(values)} x i8] [{llvm_array(values, 'i8')}]
    %p = getelementptr inbounds [{len(values)} x i8], ptr @data, i64 0, i64 %index
    %x = load i8, ptr %p, align 1
    ret i8 %x


@pure
@llvm
def _unicode_name_order(index: int) -> i32:
    @data = private unnamed_addr constant [{len(name_order)} x i32] [{llvm_array(name_order, 'i32')}]
    %p = getelementptr inbounds [{len(name_order)} x i32], ptr @data, i64 0, i64 %index
    %x = load i32, ptr %p, align 4
    ret i32 %x


def unicode_name_record_index(codepoint: int) -> int:
    low = 0
    high = {len(records)}
    while low < high:
        middle = low + (high - low) // 2
        record = _unicode_name_record(middle)
        value = int(record >> u64(32))
        if value < codepoint:
            low = middle + 1
        else:
            high = middle
    if low == {len(records)}:
        return -1
    return low if int(_unicode_name_record(low) >> u64(32)) == codepoint else -1


def unicode_name_compare(value: str, record_index: int) -> int:
    record = _unicode_name_record(record_index)
    offset = int((record >> u64(8)) & u64(0xFFFFFF))
    length = int(record & u64(0xFF))
    shared = min(len(value), length)
    for index in range(shared):
        left = ord(value[index])
        right = int(_unicode_name_byte(offset + index))
        if left != right:
            return left - right
    return len(value) - length


def unicode_name_lookup_index(value: str) -> int:
    low = 0
    high = {len(name_order)}
    while low < high:
        middle = low + (high - low) // 2
        record_index = int(_unicode_name_order(middle))
        if unicode_name_compare(value, record_index) > 0:
            low = middle + 1
        else:
            high = middle
    if low == {len(name_order)}:
        return -1
    record_index = int(_unicode_name_order(low))
    return record_index if unicode_name_compare(value, record_index) == 0 else -1


def unicode_name_value(record_index: int) -> str:
    record = _unicode_name_record(record_index)
    offset = int((record >> u64(8)) & u64(0xFFFFFF))
    length = int(record & u64(0xFF))
    value = ""
    for index in range(length):
        value += chr(int(_unicode_name_byte(offset + index)))
    return value


def unicode_name_codepoint(record_index: int) -> int:
    return int(_unicode_name_record(record_index) >> u64(32))
"""
    )


def parse_named_codepoints(path, name_first):
    records = []
    with path.open(encoding="utf-8") as source:
        for line in source:
            line = line.partition("#")[0].strip()
            if not line:
                continue
            first, second, *_ = (part.strip() for part in line.split(";"))
            name, values = (first, second) if name_first else (second, first)
            records.append((name, tuple(int(value, 16) for value in values.split())))
    return records


def build_lookup_alias_data(alias_path, sequence_path):
    entries = {}
    if alias_path:
        entries.update(parse_named_codepoints(alias_path, False))
    if sequence_path:
        entries.update(parse_named_codepoints(sequence_path, True))

    records = []
    name_values = []
    codepoint_values = []
    for name, codepoints in sorted(entries.items()):
        name_offset = len(name_values)
        name_values.extend(name.encode("ascii"))
        codepoint_offset = len(codepoint_values)
        codepoint_values.extend(codepoints)
        records.append((name_offset, len(name), codepoint_offset, len(codepoints)))
    return records, name_values, codepoint_values


def emit_lookup_aliases(out, records, name_values, codepoint_values):
    # A record packs [name byte offset: 24 bits, name length: 8 bits,
    # code point offset: 24 bits, code point length: 8 bits].
    record_values = [
        (name_offset << 40) | (name_length << 32) | (codepoint_offset << 8) | codepoint_length
        for name_offset, name_length, codepoint_offset, codepoint_length in records
    ]
    # LLVM does not permit empty array initializers.
    if not record_values:
        record_values = [0]
        name_values = [0]
        codepoint_values = [0]

    out.write(
        f"""
@pure
@llvm
def _unicode_lookup_alias_record(index: int) -> u64:
    @data = private unnamed_addr constant [{len(record_values)} x i64] [{llvm_array(record_values, 'i64')}]
    %p = getelementptr inbounds [{len(record_values)} x i64], ptr @data, i64 0, i64 %index
    %x = load i64, ptr %p, align 8
    ret i64 %x


@pure
@llvm
def _unicode_lookup_alias_byte(index: int) -> u8:
    @data = private unnamed_addr constant [{len(name_values)} x i8] [{llvm_array(name_values, 'i8')}]
    %p = getelementptr inbounds [{len(name_values)} x i8], ptr @data, i64 0, i64 %index
    %x = load i8, ptr %p, align 1
    ret i8 %x


@pure
@llvm
def _unicode_lookup_alias_codepoint(index: int) -> i32:
    @data = private unnamed_addr constant [{len(codepoint_values)} x i32] [{llvm_array(codepoint_values, 'i32')}]
    %p = getelementptr inbounds [{len(codepoint_values)} x i32], ptr @data, i64 0, i64 %index
    %x = load i32, ptr %p, align 4
    ret i32 %x


def unicode_lookup_alias_compare(value: str, record_index: int) -> int:
    record = _unicode_lookup_alias_record(record_index)
    offset = int(record >> u64(40))
    length = int((record >> u64(32)) & u64(0xFF))
    shared = min(len(value), length)
    for index in range(shared):
        left = ord(value[index])
        right = int(_unicode_lookup_alias_byte(offset + index))
        if left != right:
            return left - right
    return len(value) - length


def unicode_lookup_alias_index(value: str) -> int:
    low = 0
    high = {len(records)}
    while low < high:
        middle = low + (high - low) // 2
        if unicode_lookup_alias_compare(value, middle) > 0:
            low = middle + 1
        else:
            high = middle
    if low == {len(records)}:
        return -1
    return low if unicode_lookup_alias_compare(value, low) == 0 else -1


def unicode_lookup_alias_length(record_index: int) -> int:
    return int(_unicode_lookup_alias_record(record_index) & u64(0xFF))


def unicode_lookup_alias_codepoint(record_index: int, index: int) -> int:
    record = _unicode_lookup_alias_record(record_index)
    offset = int((record >> u64(8)) & u64(0xFFFFFF))
    return int(_unicode_lookup_alias_codepoint(offset + index))
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


def unicode_{name}_mapping_record(cp: int) -> int:
    return _unicode_{name}_mapping_lookup(cp)


def unicode_{name}_mapping_record_length(record: int) -> int:
    if record >= 0:
        return 1

    return int(
        _unicode_{name}_mapping_data(
            (record & 0x7FFFFFFF) * 4
        )
    )


def unicode_{name}_mapping_record_codepoint(
    cp: int,
    record: int,
    index: int,
) -> int:
    if record == 0:
        return cp

    if record > 0:
        return cp + record - {DIRECT_MAPPING_OFFSET}

    return int(
        _unicode_{name}_mapping_data(
            (record & 0x7FFFFFFF) * 4 + 1 + index
        )
    )


def unicode_{name}_mapping_length(cp: int) -> int:
    return unicode_{name}_mapping_record_length(
        unicode_{name}_mapping_record(cp)
    )


def unicode_{name}_mapping_codepoint(cp: int, index: int) -> int:
    return unicode_{name}_mapping_record_codepoint(
        cp,
        unicode_{name}_mapping_record(cp),
        index,
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
        default=pathlib.Path("stdlib/internal/unicode/data.codon"),
    )
    parser.add_argument(
        "--derived-core-properties",
        type=pathlib.Path,
        required=True,
        help="Path to Unicode DerivedCoreProperties.txt",
    )
    parser.add_argument(
        "--unicodedata-output",
        type=pathlib.Path,
        default=pathlib.Path(
            "stdlib/internal/unicode/unicodedata_data.codon"
        ),
    )
    parser.add_argument("--name-aliases", type=pathlib.Path)
    parser.add_argument("--named-sequences", type=pathlib.Path)
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

    args.unicodedata_output.parent.mkdir(parents=True, exist_ok=True)
    with args.unicodedata_output.open("w", encoding="utf-8") as out:
        out.write(
            f"""# AUTO-GENERATED by scripts/generate_unicode_data.py.
# Python Unicode database: {unicodedata.unidata_version}
# Do not edit manually.

UNICODE_MAX: Literal[int] = 0x10FFFF
UNICODE_PAGE_SHIFT: Literal[int] = 8
UNICODEDATA_VERSION: str = "{unicodedata.unidata_version}"

"""
        )

        metadata_tables = (
            (
                "category",
                CATEGORY_INDEX,
                lambda codepoint: unicodedata.category(chr(codepoint)),
            ),
            (
                "bidirectional",
                BIDIRECTIONAL_INDEX,
                lambda codepoint: unicodedata.bidirectional(chr(codepoint)),
            ),
            (
                "east_asian_width",
                EAST_ASIAN_WIDTH_INDEX,
                lambda codepoint: unicodedata.east_asian_width(chr(codepoint)),
            ),
            (
                "mirrored",
                {False: 0, True: 1},
                lambda codepoint: bool(unicodedata.mirrored(chr(codepoint))),
            ),
        )
        for name, index, value_for in metadata_tables:
            directory, pages = build_value_pages(
                lambda codepoint: index[value_for(codepoint)]
            )
            emit_value_index(out, name, directory, pages)
            print(
                f"  {name}: {len(pages)} unique non-default pages, "
                f"{len(pages) * PAGE_SIZE} bytes of page data",
                file=sys.stderr,
            )

        for name, value_for in (
            (
                "decimal",
                lambda codepoint: (
                    unicodedata.decimal(chr(codepoint), -1) + 1
                ),
            ),
            (
                "digit",
                lambda codepoint: unicodedata.digit(chr(codepoint), -1) + 1,
            ),
        ):
            directory, pages = build_value_pages(value_for)
            emit_value_index(out, name, directory, pages)
            print(
                f"  {name}: {len(pages)} unique non-default pages, "
                f"{len(pages) * PAGE_SIZE} bytes of page data",
                file=sys.stderr,
            )

        directory, pages, values = build_numeric_pages()
        emit_numeric(out, directory, pages, values)
        print(
            f"  numeric: {len(pages)} unique non-default pages, "
            f"{len(values)} values, {len(pages) * PAGE_SIZE * 2} bytes of page data",
            file=sys.stderr,
        )

        records, values, name_order = build_name_data()
        emit_names(out, records, values, name_order)
        print(
            f"  names: {len(records)} records, {len(values)} bytes",
            file=sys.stderr,
        )

        alias_records, alias_names, alias_codepoints = build_lookup_alias_data(
            args.name_aliases, args.named_sequences
        )
        emit_lookup_aliases(out, alias_records, alias_names, alias_codepoints)
        print(
            f"  lookup aliases: {len(alias_records)} records",
            file=sys.stderr,
        )


if __name__ == "__main__":
    main()
