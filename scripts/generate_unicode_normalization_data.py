#!/usr/bin/env python3
"""Generate Unicode normalization lookup tables."""

import argparse
import pathlib
import unicodedata


MAX_CODEPOINT = 0x10FFFF
DECOMPOSITION_TAGS = (
    "<circle>", "<compat>", "<final>", "<font>",
    "<fraction>", "<initial>", "<isolated>", "<medial>",
    "<narrow>", "<noBreak>", "<small>", "<square>",
    "<sub>", "<super>", "<vertical>", "<wide>",
)


def table_size(entry_count: int) -> int:
    size = 1
    while size < entry_count * 2:
        size <<= 1
    return size


def hash_codepoint(codepoint: int, mask: int) -> int:
    return (codepoint * 0x9E3779B1) & mask


def hash_pair(first: int, second: int, mask: int) -> int:
    return ((first * 0x9E3779B1) ^ (second * 0x85EBCA77)) & mask


def canonical_decomposition(codepoint: int) -> tuple[int, ...]:
    decomposition = unicodedata.decomposition(chr(codepoint))
    if not decomposition or decomposition.startswith("<"):
        return ()
    return tuple(int(value, 16) for value in decomposition.split())


def compatibility_decomposition(codepoint: int) -> tuple[int, ...]:
    decomposition = unicodedata.decomposition(chr(codepoint))
    if not decomposition:
        return ()
    values = decomposition.split()
    if values[0].startswith("<"):
        values = values[1:]
    return tuple(int(value, 16) for value in values)


def raw_decomposition(codepoint: int) -> tuple[int, tuple[int, ...]]:
    decomposition = unicodedata.decomposition(chr(codepoint))
    if not decomposition:
        return 0, ()
    values = decomposition.split()
    tag = 0
    if values[0].startswith("<"):
        tag = DECOMPOSITION_TAGS.index(values[0]) + 1
        values = values[1:]
    return tag, tuple(int(value, 16) for value in values)


def llvm_values(values: list[int], llvm_type: str) -> str:
    return ", ".join(f"{llvm_type} {value}" for value in values)


def build_decomposition_data(decomposition_for):
    records = []
    values = []
    for codepoint in range(MAX_CODEPOINT + 1):
        decomposition = decomposition_for(codepoint)
        if decomposition:
            offset = len(values)
            values.extend(decomposition)
            records.append((codepoint, offset, len(decomposition)))

    size = table_size(len(records))
    table = [0] * size
    mask = size - 1
    for codepoint, offset, length in records:
        index = hash_codepoint(codepoint, mask)
        while table[index]:
            index = (index + 1) & mask
        table[index] = (codepoint << 32) | (offset << 8) | length
    return table, values


def build_raw_decomposition_data():
    records = []
    values = []
    for codepoint in range(MAX_CODEPOINT + 1):
        tag, decomposition = raw_decomposition(codepoint)
        if decomposition:
            offset = len(values)
            values.extend(decomposition)
            records.append((codepoint, tag, offset, len(decomposition)))

    size = table_size(len(records))
    table = [0] * size
    mask = size - 1
    for codepoint, tag, offset, length in records:
        index = hash_codepoint(codepoint, mask)
        while table[index]:
            index = (index + 1) & mask
        table[index] = (codepoint << 32) | (tag << 27) | (offset << 8) | length
    return table, values


def build_combining_data():
    records = [
        (codepoint, unicodedata.combining(chr(codepoint)))
        for codepoint in range(MAX_CODEPOINT + 1)
        if unicodedata.combining(chr(codepoint))
    ]
    size = table_size(len(records))
    table = [0] * size
    mask = size - 1
    for codepoint, combining_class in records:
        index = hash_codepoint(codepoint, mask)
        while table[index]:
            index = (index + 1) & mask
        table[index] = (codepoint << 8) | combining_class
    return table


def build_composition_data():
    records = []
    for codepoint in range(MAX_CODEPOINT + 1):
        decomposition = canonical_decomposition(codepoint)
        if len(decomposition) != 2:
            continue
        if unicodedata.normalize("NFC", "".join(map(chr, decomposition))) != chr(codepoint):
            continue
        records.append((decomposition[0], decomposition[1], codepoint))

    size = table_size(len(records))
    table = [0] * size
    mask = size - 1
    for first, second, result in records:
        index = hash_pair(first, second, mask)
        while table[index]:
            index = (index + 1) & mask
        key = (first << 21) | second
        table[index] = (key << 21) | result
    return table


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--output",
        type=pathlib.Path,
        default=pathlib.Path("stdlib/internal/unicode/normalization_data.codon"),
    )
    args = parser.parse_args()
    decomposition_table, decomposition_values = build_decomposition_data(
        canonical_decomposition
    )
    compatibility_table, compatibility_values = build_decomposition_data(
        compatibility_decomposition
    )
    raw_decomposition_table, raw_decomposition_values = build_raw_decomposition_data()
    combining_table = build_combining_data()
    composition_table = build_composition_data()

    args.output.parent.mkdir(parents=True, exist_ok=True)
    with args.output.open("w", encoding="utf-8") as out:
        out.write(
            "# AUTO-GENERATED by scripts/generate_unicode_normalization_data.py.\n"
            f"# Python Unicode database: {unicodedata.unidata_version}\n"
            "# Do not edit manually.\n\n"
            f"NORMALIZATION_DECOMPOSITION_MASK: Literal[int] = {len(decomposition_table) - 1}\n"
            f"NORMALIZATION_COMPATIBILITY_DECOMPOSITION_MASK: Literal[int] = {len(compatibility_table) - 1}\n"
            f"NORMALIZATION_RAW_DECOMPOSITION_MASK: Literal[int] = {len(raw_decomposition_table) - 1}\n"
            f"NORMALIZATION_COMBINING_MASK: Literal[int] = {len(combining_table) - 1}\n"
            f"NORMALIZATION_COMPOSITION_MASK: Literal[int] = {len(composition_table) - 1}\n\n"
        )
        out.write(
            "@pure\n@llvm\n"
            "def _normalization_decomposition_entry(index: int) -> u64:\n"
            f"    @data = private unnamed_addr constant [{len(decomposition_table)} x i64] [{llvm_values(decomposition_table, 'i64')}]\n"
            f"    %p = getelementptr inbounds [{len(decomposition_table)} x i64], ptr @data, i64 0, i64 %index\n"
            "    %value = load i64, ptr %p, align 8\n"
            "    ret i64 %value\n\n"
            "@pure\n@llvm\n"
            "def _normalization_decomposition_value(index: int) -> i32:\n"
            f"    @data = private unnamed_addr constant [{len(decomposition_values)} x i32] [{llvm_values(decomposition_values, 'i32')}]\n"
            f"    %p = getelementptr inbounds [{len(decomposition_values)} x i32], ptr @data, i64 0, i64 %index\n"
            "    %value = load i32, ptr %p, align 4\n"
            "    ret i32 %value\n\n"
            "@pure\n@llvm\n"
            "def _normalization_raw_decomposition_entry(index: int) -> u64:\n"
            f"    @data = private unnamed_addr constant [{len(raw_decomposition_table)} x i64] [{llvm_values(raw_decomposition_table, 'i64')}]\n"
            f"    %p = getelementptr inbounds [{len(raw_decomposition_table)} x i64], ptr @data, i64 0, i64 %index\n"
            "    %value = load i64, ptr %p, align 8\n"
            "    ret i64 %value\n\n"
            "@pure\n@llvm\n"
            "def _normalization_raw_decomposition_value(index: int) -> i32:\n"
            f"    @data = private unnamed_addr constant [{len(raw_decomposition_values)} x i32] [{llvm_values(raw_decomposition_values, 'i32')}]\n"
            f"    %p = getelementptr inbounds [{len(raw_decomposition_values)} x i32], ptr @data, i64 0, i64 %index\n"
            "    %value = load i32, ptr %p, align 4\n"
            "    ret i32 %value\n\n"
            "@pure\n@llvm\n"
            "def _normalization_compatibility_decomposition_entry(index: int) -> u64:\n"
            f"    @data = private unnamed_addr constant [{len(compatibility_table)} x i64] [{llvm_values(compatibility_table, 'i64')}]\n"
            f"    %p = getelementptr inbounds [{len(compatibility_table)} x i64], ptr @data, i64 0, i64 %index\n"
            "    %value = load i64, ptr %p, align 8\n"
            "    ret i64 %value\n\n"
            "@pure\n@llvm\n"
            "def _normalization_compatibility_decomposition_value(index: int) -> i32:\n"
            f"    @data = private unnamed_addr constant [{len(compatibility_values)} x i32] [{llvm_values(compatibility_values, 'i32')}]\n"
            f"    %p = getelementptr inbounds [{len(compatibility_values)} x i32], ptr @data, i64 0, i64 %index\n"
            "    %value = load i32, ptr %p, align 4\n"
            "    ret i32 %value\n\n"
            "@pure\n@llvm\n"
            "def _normalization_combining_entry(index: int) -> u64:\n"
            f"    @data = private unnamed_addr constant [{len(combining_table)} x i64] [{llvm_values(combining_table, 'i64')}]\n"
            f"    %p = getelementptr inbounds [{len(combining_table)} x i64], ptr @data, i64 0, i64 %index\n"
            "    %value = load i64, ptr %p, align 8\n"
            "    ret i64 %value\n\n"
            "@pure\n@llvm\n"
            "def _normalization_composition_entry(index: int) -> u64:\n"
            f"    @data = private unnamed_addr constant [{len(composition_table)} x i64] [{llvm_values(composition_table, 'i64')}]\n"
            f"    %p = getelementptr inbounds [{len(composition_table)} x i64], ptr @data, i64 0, i64 %index\n"
            "    %value = load i64, ptr %p, align 8\n"
            "    ret i64 %value\n"
        )


if __name__ == "__main__":
    main()
