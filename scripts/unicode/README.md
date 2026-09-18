# Standard-library table bundle

Normal builds need Python 3.8+ but do not need network access, UCD input files,
or a specific Python Unicode database. CMake verifies and unpacks
`tables-16.0.0.tar.gz` into the build tree, embeds the generated modules, and
installs them alongside the handwritten standard library. The archive is a
checked-in, offline bootstrap artifact; expanded generated sources are not tracked.
This reduces future source-tree growth without rewriting existing Git history.

## Layout

Handwritten algorithms:

- `stdlib/internal/unicode/normalization.codon`: Unicode normalization.
- `stdlib/internal/unicode/names.codon`: name search, decoding, and algorithmic names.
- `stdlib/internal/numeric/float_dtoa.codon`: floating-point conversion, unrelated to Unicode.

Generated modules, relative to `build/stdlib` or the installed stdlib:

- `internal/unicode/generated/properties.codon`: character predicates and case mappings.
- `internal/unicode/generated/single_byte_codecs.codon`: single-byte codec tables.
- `internal/unicode/generated/normalization.codon`: normalization tables.
- `internal/unicode/generated/metadata.codon`: categories, numeric values, and other metadata.
- `internal/unicode/generated/names.codon`: names, aliases, and named sequences.
- `internal/numeric/generated/float_dtoa_tables.codon`: floating-point conversion tables.

The hot string properties remain separate from metadata, names, and normalization.
Hangul names are algorithmic; CJK unified ideographs use exact assigned ranges
derived from the pinned database. Other names use a shared-word dictionary, and
byte pools use compact LLVM string constants. There is no runtime archive or
dictionary decompression.

## Maintenance

Regeneration uses CPython 3.14.3 (Unicode 16.0.0), also pinned in CI. The tool checks
the host Unicode version and the SHA-256 hashes of all external inputs. Ordinary
builds only consume the bundle. Only the explicit `fetch` command accesses the
network; it verifies downloads before accepting them.

From the repository root:

```sh
python3 scripts/unicode_tables.py fetch --ucd-dir build/unicode-16.0.0
python3 scripts/unicode_tables.py regenerate --ucd-dir build/unicode-16.0.0 --output-dir build/stdlib
python3 scripts/unicode_tables.py pack --input-dir build/stdlib
python3 scripts/unicode_tables.py verify
```

Review and commit generator changes together with the updated archive and
`manifest.json`. Generator hashes make stale bundles fail the build. Do not edit
expanded tables manually. To check regeneration without changing the bundle:

```sh
python3 scripts/unicode_tables.py regenerate --ucd-dir build/unicode-16.0.0 --output-dir build/unicode-check
python3 scripts/unicode_tables.py check --input-dir build/unicode-check
python3 scripts/unicode/test_tables.py
```

CMake targets `unicode_data`, `unicode_data_verify`, and `unicode_data_regenerate`
respectively extract, verify, and explicitly regenerate/repack the bundle.
Regeneration requires the matching Python interpreter selected by CMake and the
inputs in `CODON_UNICODE_UCD_DIR`; it updates the source archive and manifest.
`CODON_UNICODE_ARCHIVE` may point to another local copy of the same archive, but
its checksum must still match the checked-in manifest. Missing build outputs are
re-extracted, and bundle, manifest, or generator changes invalidate extraction.

For a Unicode upgrade, update the version in the bundle tool and CMake, pin the
new UCD URLs and checksums in `inputs.json`, select the corresponding CPython
release in CI, and regenerate. Review changed data and run the Unicode stdlib
tests, including normalization conformance and exhaustive name round trips.
