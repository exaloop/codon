#!/usr/bin/env python3
"""Build, verify, and unpack the reproducible standard-library table bundle."""

import argparse
import gzip
import hashlib
import io
import json
from pathlib import Path
import subprocess
import sys
import tarfile
import unicodedata
import urllib.request


ROOT = Path(__file__).resolve().parents[1]
MANIFEST = ROOT / "scripts/unicode/manifest.json"
VERSION = "16.0.0"
FILES = (
    "internal/unicode/generated/properties.codon",
    "internal/unicode/generated/single_byte_codecs.codon",
    "internal/unicode/generated/normalization.codon",
    "internal/unicode/generated/metadata.codon",
    "internal/unicode/generated/names.codon",
    "internal/numeric/generated/float_dtoa_tables.codon",
)
GENERATORS = (
    "scripts/unicode/inputs.json",
    "scripts/unicode_tables.py",
    "scripts/generate_unicode_data.py",
    "scripts/generate_unicode_normalization_data.py",
    "scripts/generate_single_byte_codecs.py",
    "scripts/generate_float_dtoa_tables.py",
)


def digest(data):
    return hashlib.sha256(data).hexdigest()


def write(path, data):
    path.parent.mkdir(parents=True, exist_ok=True)
    temporary = path.with_name(path.name + ".tmp")
    temporary.write_bytes(data)
    temporary.replace(path)


def pack(source):
    payloads = {name: (source / name).read_bytes() for name in FILES}
    output = io.BytesIO()
    with gzip.GzipFile(fileobj=output, mode="wb", filename="", mtime=0) as compressed:
        with tarfile.open(fileobj=compressed, mode="w", format=tarfile.USTAR_FORMAT) as archive:
            for name, data in payloads.items():
                member = tarfile.TarInfo(name)
                member.size = len(data)
                member.mode = 0o644
                archive.addfile(member, io.BytesIO(data))
    bundle = output.getvalue()
    archive_name = f"tables-{VERSION}.tar.gz"
    manifest = {
        "schema": 1,
        "unicode_version": VERSION,
        "archive": archive_name,
        "sha256": digest(bundle),
        "generators": {name: digest((ROOT / name).read_bytes()) for name in GENERATORS},
        "files": {
            name: {"sha256": digest(data), "size": len(data)}
            for name, data in payloads.items()
        },
    }
    write(MANIFEST.parent / archive_name, bundle)
    write(MANIFEST, (json.dumps(manifest, indent=2) + "\n").encode())
    print(f"Packed {len(payloads)} modules: {sum(map(len, payloads.values()))} -> {len(bundle)} bytes")


def verify(archive_path=None):
    manifest = json.loads(MANIFEST.read_text())
    if manifest["schema"] != 1 or manifest["unicode_version"] != VERSION:
        raise ValueError("unsupported Unicode table manifest")
    if set(manifest["files"]) != set(FILES):
        raise ValueError("Unicode table manifest has unexpected module paths")
    for name in GENERATORS:
        if digest((ROOT / name).read_bytes()) != manifest["generators"].get(name):
            raise ValueError(f"{name} changed; regenerate and repack the Unicode tables")
    archive_path = archive_path or MANIFEST.parent / manifest["archive"]
    bundle = archive_path.read_bytes()
    if digest(bundle) != manifest["sha256"]:
        raise ValueError(f"Unicode table archive checksum mismatch: {archive_path}")
    payloads = {}
    with tarfile.open(fileobj=io.BytesIO(bundle), mode="r:gz") as archive:
        for member in archive:
            if not member.isfile() or member.name not in FILES or member.name in payloads:
                raise ValueError(f"unexpected Unicode archive member: {member.name}")
            expected = manifest["files"][member.name]
            if member.size != expected["size"]:
                raise ValueError(f"Unicode table size mismatch: {member.name}")
            data = archive.extractfile(member).read()
            if digest(data) != expected["sha256"]:
                raise ValueError(f"Unicode table checksum mismatch: {member.name}")
            payloads[member.name] = data
    if set(payloads) != set(FILES):
        raise ValueError("incomplete Unicode table archive")
    return payloads


def extract(output, archive_path=None):
    payloads = verify(archive_path)
    for name, data in payloads.items():
        write(output / name, data)


def check(source, archive_path=None):
    for name, expected in verify(archive_path).items():
        if (source / name).read_bytes() != expected:
            raise ValueError(f"generated Unicode table differs from bundle: {name}")


def inputs_manifest():
    inputs = json.loads((MANIFEST.parent / "inputs.json").read_text())
    if inputs["unicode_version"] != VERSION:
        raise ValueError("UCD input manifest has a different Unicode version")
    return inputs


def fetch(ucd):
    inputs = inputs_manifest()
    for name, expected in inputs["files"].items():
        destination = ucd / name
        if destination.exists() and digest(destination.read_bytes()) == expected:
            continue
        with urllib.request.urlopen(inputs["base_url"] + name, timeout=60) as response:
            data = response.read()
        if digest(data) != expected:
            raise ValueError(f"downloaded UCD input checksum mismatch: {name}")
        write(destination, data)


def regenerate(output, ucd):
    if unicodedata.unidata_version != VERSION:
        raise ValueError(f"regeneration requires Python with Unicode {VERSION}, got {unicodedata.unidata_version}")
    inputs = inputs_manifest()
    for name, expected in inputs["files"].items():
        if digest((ucd / name).read_bytes()) != expected:
            raise ValueError(f"UCD input checksum mismatch: {name}")
    commands = (
        ("generate_unicode_data.py", "--output", output / FILES[0],
         "--unicodedata-output", output / FILES[3], "--names-output", output / FILES[4],
         "--derived-core-properties", ucd / "DerivedCoreProperties.txt",
         "--name-aliases", ucd / "NameAliases.txt", "--named-sequences", ucd / "NamedSequences.txt"),
        ("generate_single_byte_codecs.py", "--output", output / FILES[1]),
        ("generate_unicode_normalization_data.py", "--output", output / FILES[2]),
        ("generate_float_dtoa_tables.py", "--output", output / FILES[5]),
    )
    for script, *arguments in commands:
        subprocess.run([sys.executable, ROOT / "scripts" / script, *arguments], check=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    commands = parser.add_subparsers(dest="command", required=True)
    for name in ("verify", "extract", "check"):
        command = commands.add_parser(name)
        command.add_argument("--archive", type=Path)
        if name == "extract":
            command.add_argument("--output-dir", type=Path, required=True)
        elif name == "check":
            command.add_argument("--input-dir", type=Path, required=True)
    command = commands.add_parser("pack")
    command.add_argument("--input-dir", type=Path, required=True)
    command = commands.add_parser("regenerate")
    command.add_argument("--output-dir", type=Path, required=True)
    command.add_argument("--ucd-dir", type=Path, required=True)
    command = commands.add_parser("fetch")
    command.add_argument("--ucd-dir", type=Path, required=True)
    args = parser.parse_args()
    if args.command == "pack":
        pack(args.input_dir)
    elif args.command == "regenerate":
        regenerate(args.output_dir, args.ucd_dir)
    elif args.command == "fetch":
        fetch(args.ucd_dir)
    else:
        if args.command == "extract":
            extract(args.output_dir, args.archive)
        elif args.command == "check":
            check(args.input_dir, args.archive)
        else:
            verify(args.archive)
        print(f"Verified {len(FILES)} Unicode/numeric table modules ({VERSION})")


if __name__ == "__main__":
    main()
