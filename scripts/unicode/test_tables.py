import contextlib
import io
import json
from pathlib import Path
import shutil
import sys
import tarfile
import tempfile
import unittest
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import unicode_tables as tables


class TableBundleTest(unittest.TestCase):
    def setUp(self):
        temporary = tempfile.TemporaryDirectory()
        self.addCleanup(temporary.cleanup)
        self.root = Path(temporary.name)
        for name in tables.GENERATORS:
            target = self.root / name
            target.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(tables.ROOT / name, target)
        self.manifest_path = self.root / "scripts/unicode/manifest.json"
        for attribute, value in (("ROOT", self.root), ("MANIFEST", self.manifest_path)):
            replacement = patch.object(tables, attribute, value)
            replacement.start()
            self.addCleanup(replacement.stop)
        self.source = self.root / "source"
        self.payloads = {name: ("# " + name + "\n").encode() for name in tables.FILES}
        for name, data in self.payloads.items():
            tables.write(self.source / name, data)
        self.pack()
        self.manifest = json.loads(self.manifest_path.read_text())
        self.archive = self.manifest_path.parent / self.manifest["archive"]

    def pack(self):
        with contextlib.redirect_stdout(io.StringIO()):
            tables.pack(self.source)

    def save_manifest(self):
        tables.write(self.manifest_path, json.dumps(self.manifest).encode())

    def replace_archive(self, members):
        output = io.BytesIO()
        with tarfile.open(fileobj=output, mode="w:gz") as archive:
            for name, data, kind in members:
                member = tarfile.TarInfo(name)
                member.type = kind
                member.size = len(data)
                archive.addfile(member, io.BytesIO(data))
        bundle = output.getvalue()
        tables.write(self.archive, bundle)
        self.manifest["sha256"] = tables.digest(bundle)
        self.save_manifest()

    def test_round_trip_and_determinism(self):
        archive = self.archive.read_bytes()
        manifest = self.manifest_path.read_bytes()
        self.pack()
        self.assertEqual(archive, self.archive.read_bytes())
        self.assertEqual(manifest, self.manifest_path.read_bytes())
        self.assertEqual(self.payloads, tables.verify())
        output = self.root / "output"
        tables.extract(output)
        tables.check(output)
        tables.write(output / tables.FILES[0], b"changed")
        with self.assertRaisesRegex(ValueError, "differs from bundle"):
            tables.check(output)

    def test_archive_corruption_does_not_extract(self):
        tables.write(self.archive, self.archive.read_bytes()[:-1])
        output = self.root / "output"
        with self.assertRaisesRegex(ValueError, "archive checksum mismatch"):
            tables.extract(output)
        self.assertFalse(output.exists())

    def test_generator_changes_require_regeneration(self):
        tables.write(self.root / tables.GENERATORS[0], b"changed")
        with self.assertRaisesRegex(ValueError, "regenerate and repack"):
            tables.verify()

    def test_manifest_version_and_paths(self):
        self.manifest["unicode_version"] = "0.0.0"
        self.save_manifest()
        with self.assertRaisesRegex(ValueError, "unsupported"):
            tables.verify()
        self.manifest["unicode_version"] = tables.VERSION
        self.manifest["files"]["../unexpected.codon"] = {}
        self.save_manifest()
        with self.assertRaisesRegex(ValueError, "unexpected module paths"):
            tables.verify()

    def test_unsafe_and_duplicate_members(self):
        name = tables.FILES[0]
        member = (name, self.payloads[name], tarfile.REGTYPE)
        for members in (
            [("../escape.codon", b"", tarfile.REGTYPE)],
            [("/absolute.codon", b"", tarfile.REGTYPE)],
            [(name, b"", tarfile.SYMTYPE)],
            [(name, b"", tarfile.LNKTYPE)],
            [member, member],
        ):
            with self.subTest(members=members):
                self.replace_archive(members)
                with self.assertRaisesRegex(ValueError, "unexpected Unicode archive member"):
                    tables.verify()

    def test_missing_member(self):
        self.replace_archive([
            (name, data, tarfile.REGTYPE)
            for name, data in list(self.payloads.items())[1:]
        ])
        with self.assertRaisesRegex(ValueError, "incomplete"):
            tables.verify()

    def test_member_size_and_hash(self):
        name = tables.FILES[0]
        for data, message in ((b"", "size mismatch"), (b"?" * len(self.payloads[name]), "checksum mismatch")):
            with self.subTest(message=message):
                self.replace_archive([(name, data, tarfile.REGTYPE)])
                with self.assertRaisesRegex(ValueError, message):
                    tables.verify()

    def test_bad_inputs_rejected_before_generation(self):
        inputs = self.root / "ucd"
        inputs.mkdir()
        for name in tables.inputs_manifest()["files"]:
            tables.write(inputs / name, b"invalid")
        with patch.object(tables.unicodedata, "unidata_version", tables.VERSION):
            with self.assertRaisesRegex(ValueError, "input checksum mismatch"):
                tables.regenerate(self.root / "output", inputs)
        self.assertFalse((self.root / "output").exists())

    def test_wrong_host_database_rejected(self):
        with patch.object(tables.unicodedata, "unidata_version", "0.0.0"):
            with self.assertRaisesRegex(ValueError, "regeneration requires"):
                tables.regenerate(self.root / "output", self.root / "ucd")


if __name__ == "__main__":
    unittest.main()
