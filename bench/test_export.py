import hashlib
import json
import tempfile
import unittest
from pathlib import Path

from export import deduplicate_sources, write_json


class SourceStorageTests(unittest.TestCase):
    def test_shared_sources_preserve_results_and_provenance(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            content = b"print(42)\n"
            digest = hashlib.sha256(content).hexdigest()
            snapshots = {}
            for machine, suffix in (("", ".codon"), ("aws", ".py")):
                output = root / machine
                source = output / "source" / f"example{suffix}"
                source.parent.mkdir(parents=True)
                source.write_bytes(content)
                url = f"/benchmarks/{source.relative_to(root).as_posix()}"
                result = {"sourceUrl": url, "sourceSha256": digest,
                          "workerUrl": url, "workerSha256": digest,
                          "displaySourceUrl": url, "executedSourceSha256": "original-source",
                          "executedWorkerSha256": "original-worker", "samples": [1, 2, 3], "median": 2}
                data = {"assetBase": "/benchmarks" + ("/aws" if machine else ""),
                        "cases": [{"id": "example", "results": {"codon": result}}]}
                write_json(output / "results.json", data)
                (output / "raw-results.json").write_bytes(b'{"provenance": "unchanged"}\n')
                snapshots[machine] = data
            deduplicate_sources(root)
            sources = list(root.rglob("source/*"))
            self.assertEqual(len(sources), 1)
            self.assertEqual(sources[0].read_bytes(), content)
            for machine, expected in snapshots.items():
                result = expected["cases"][0]["results"]["codon"]
                for field in ("sourceUrl", "workerUrl", "displaySourceUrl"):
                    result[field] = f"/benchmarks/source/{digest}.py"
                self.assertEqual(json.loads((root / machine / "results.json").read_text()), expected)
                self.assertEqual((root / machine / "raw-results.json").read_bytes(), b'{"provenance": "unchanged"}\n')
            before = {filename.relative_to(root): filename.read_bytes() for filename in root.rglob("*") if filename.is_file()}
            deduplicate_sources(root)
            self.assertEqual({filename.relative_to(root): filename.read_bytes() for filename in root.rglob("*") if filename.is_file()}, before)

            replacement = b"print(43)\n"
            source = root / "aws/source/refreshed.py"
            source.write_bytes(replacement)
            refreshed = json.loads((root / "aws/results.json").read_text())
            result = refreshed["cases"][0]["results"]["codon"]
            result["sourceUrl"] = "/benchmarks/aws/source/refreshed.py"
            result["sourceSha256"] = hashlib.sha256(replacement).hexdigest()
            write_json(root / "aws/results.json", refreshed)
            deduplicate_sources(root)
            self.assertEqual((root / f"source/{digest}.py").read_bytes(), content)
            self.assertEqual((root / "results.json").read_bytes(), before[Path("results.json")])
            self.assertEqual(len(list(root.rglob("source/*"))), 2)

    def test_invalid_hash_does_not_change_files(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            source = root / "source" / "example.py"
            source.parent.mkdir()
            source.write_bytes(b"print(42)\n")
            write_json(root / "results.json", {"cases": [{"results": {"python": {
                "sourceUrl": "/benchmarks/source/example.py", "sourceSha256": "incorrect"}}}]})
            before = {filename.relative_to(root): filename.read_bytes() for filename in root.rglob("*") if filename.is_file()}
            with self.assertRaises(AssertionError):
                deduplicate_sources(root)
            self.assertEqual({filename.relative_to(root): filename.read_bytes() for filename in root.rglob("*") if filename.is_file()}, before)

    def test_removes_only_unreferenced_managed_sources(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            source_directory = root / "source"
            source_directory.mkdir()
            active = b"print(42)\n"
            stale = b"print(41)\n"
            active_hash = hashlib.sha256(active).hexdigest()
            stale_hash = hashlib.sha256(stale).hexdigest()
            (source_directory / f"{active_hash}.py").write_bytes(active)
            (source_directory / f"{stale_hash}.py").write_bytes(stale)
            unrelated = source_directory / "notes.txt"
            unrelated.write_bytes(b"Retain this unrelated file.\n")
            write_json(root / "results.json", {"cases": [{"results": {"python": {
                "sourceUrl": f"/benchmarks/source/{active_hash}.py", "sourceSha256": active_hash}}}]})
            deduplicate_sources(root)
            self.assertEqual((source_directory / f"{active_hash}.py").read_bytes(), active)
            self.assertFalse((source_directory / f"{stale_hash}.py").exists())
            self.assertEqual(unrelated.read_bytes(), b"Retain this unrelated file.\n")

    def test_outside_source_is_rejected(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            published = root / "published"
            published.mkdir()
            source = root / "source/example.py"
            source.parent.mkdir()
            source.write_bytes(b"print(42)\n")
            write_json(published / "results.json", {"cases": [{"results": {"python": {
                "sourceUrl": "/benchmarks/../source/example.py",
                "sourceSha256": hashlib.sha256(source.read_bytes()).hexdigest()}}}]})
            with self.assertRaises(AssertionError):
                deduplicate_sources(published)
            self.assertTrue(source.is_file())
            self.assertFalse((published / "source").exists())


if __name__ == "__main__":
    unittest.main()
