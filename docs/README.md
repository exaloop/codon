# Codon Documentation

The documentation uses Docusaurus 3 and the shared `@exaloop/theme` package.
This directory contains both the documentation content and the site's
configuration, navigation, build tooling, and Codon-specific styles. The
Exaloop platform is the visual reference for typography, spacing, and
light/dark colors.

## Preview Locally

Use Node.js 22 or later, Python 3.11 or later, and a Codon installation. From the
repository root:

```sh
python3 -m venv build/docs-venv
build/docs-venv/bin/python -m pip install -r docs/requirements.txt
npm ci --prefix docs

CODON_STDLIB="$HOME/.codon/lib/codon/stdlib"
"$HOME/.codon/bin/codon" doc "$CODON_STDLIB" > build/docs-api.json
build/docs-venv/bin/python scripts/docgen.py \
  build/docs-api.json docs/libraries/api --stdlib

npm start --prefix docs -- --port 3040
```

Open `http://localhost:3040`. Guide edits update automatically. After regenerating
the API, restart the server to refresh its search index and staged assets.

The renderer tracks its outputs in `.codon-doc-manifest.json`. Subsequent runs
replace those files and remove stale generated pages, leaving unrelated files
alone. It refuses to overwrite an existing, untracked output file. When upgrading
from an older generator, move the old generated directory to a backup location
before the first run; do not remove authored pages to resolve a conflict.

## Document a Library

`codon doc` parses source without running the library. It accepts a `.codon` or
`.py` file, or discovers both extensions recursively in a directory. Python files
must use syntax supported by Codon. Hidden directories, `__pycache__`, and
`__init_test__.codon`/`__init_test__.py` are skipped automatically; data files and
other extensions are ignored.

For a project with an importable package at `src/mylib`, run the installed
compiler from the project directory:

```sh
"$HOME/.codon/bin/codon" doc src/mylib --root src \
  --exclude mylib/tests --exclude mylib/examples > api.json
```

`--root` is the directory containing importable packages. It controls qualified
module names and provides an additional import search location. Without it, the
root is inferred by walking above directories containing `__init__.codon` or
`__init__.py`. Repeat `--exclude` for exact files or directory subtrees relative to
the root; exclusions are paths, not glob patterns. Empty inputs, unsupported
explicit files, and filesystem failures produce a nonzero exit status.

Render the result with this repository's script and documentation dependencies:

```sh
build/docs-venv/bin/python scripts/docgen.py \
  /path/to/project/api.json /path/to/site/docs/reference \
  --project "My Library" --route /reference \
  --source-url 'https://github.com/example/mylib/blob/{revision}/src' \
  --source-revision v1.0
```

The output is Markdown containing HTML and Docusaurus frontmatter, plus an API
search index. This site's API styles and search integration provide its full
presentation; the script is not a standalone site builder. `--sidebar` optionally
selects a Docusaurus sidebar. Source links are omitted unless `--source-url` is
set. Its prefix corresponds to the extraction root, and `{revision}` is replaced
with the URL-encoded revision. `--project` defaults to `Library` and `--route` to
`/api`.

Only discovered input modules receive pages. Imported dependencies remain in
the JSON for reference resolution but do not become part of the project's API
catalog. Exclusions affect discovery, not import parsing: an excluded module can
still be read as a dependency. Imports must resolve to available Codon-compatible
source. Public names are those without a leading underscore; class pages also
include special methods. Re-exports and type aliases link to their definitions
when those definitions are in the documented project. Class extensions are
attached by identity, not by an unqualified class name.

Use `--stdlib` only when generating Codon's own reference. It selects the existing
stdlib title, route, sidebar, and source URL defaults. For legacy JSON it also
enables the older stdlib class-consolidation policy; versioned JSON uses explicit
identities instead.

### JSON Contract

The extractor emits an object with `schema_version: 2`, `module_root`, and a
`symbols` mapping. Mapping keys are local to that extraction; `symbol_id` supplies
a qualified identity, with ordinal suffixes for overloads or repeated names.
Modules include `name`, canonical `path`, `owned`, and `children`. Declarations
include `qualified_name`, their owning `module`, and a `source` object containing
`path`, `line`, `column`, and `length`. Builtin placeholders may have no source.

Annotations are objects with `kind` and display `text`. References add `target`;
applications add `base` and `args`; literals are explicitly tagged, so `64` cannot
be confused with a symbol key. Unsupported or unresolved annotations retain
their source text as `expression` records. Functions use `return` and retain
argument kinds, defaults, generic parameters, and asynchronous status. Classes
include bases and member identities. Aliases have a `target`, and specialized
type aliases also retain their annotation in `type`.

The renderer accepts schema v2 and the old unversioned mapping. Legacy input
requires source roots as positional arguments and accepts both `return` and
`ret`. Unknown schema versions are rejected. Extraction is source-based: it does
not execute decorators, evaluate runtime-generated APIs, or replace type checking.

### Work on the Generator

For a compiler checkout, use `build/codon` and stage both `stdlib/` and the
generated tables in `build/stdlib/` into one SDK-style source tree containing the
installed `.codon` modules. Set `CODON_PATH` to that tree when running `doc`.
The schema carries its root, so no additional renderer roots are needed.

Run the extractor/renderer regressions with the same compiler and dependencies:

```sh
CODON_PATH=/path/to/staged/stdlib build/docs-venv/bin/python \
  test/docgen_test.py --codon build/codon -v
```

CI runs these tests against the installed SDK artifact before generating the
stdlib reference and checking the complete site.

## Verify the Production Site

```sh
npm run build --prefix docs
npm run serve --prefix docs -- --port 3040
```

The build checks Markdown links and routes, then crawls the rendered HTML for
local files and anchors. It also checks every API search target and all 25 legacy
redirects. Docusaurus's own anchor check cannot see IDs inside the generated API
HTML, so the final HTML check is mandatory instead. Its report is written to
`docs/build/link-report.json`. Search is indexed during the production build;
use `serve` to test search rather than the development server.

The build also exports each documentation page as `index.md` beside its HTML.
The page toolbar copies or downloads this rendered Markdown, copies the page URL,
and links to GitHub for editing authored pages. Generated API pages retain their
source links instead. Use the production preview to test Markdown downloads;
the development server does not generate them.

CI regenerates the API with its compiler artifact, runs these checks, and builds
`docs/build`. Each successful docs job uploads that directory as the
`codon-docs` artifact, including the link report and Markdown exports. Only pushes
to `master` deploy it to GitHub Pages at `docs.exaloop.io`; pull requests and
`develop` builds validate and retain the site without publishing it.

## Work on the Shared Theme

The standalone source lives in `../../theme`. It exports framework-independent
tokens and fonts plus a Docusaurus adapter; it contains no Codon content or logos.
Codon consumes `vendor/exaloop-theme-0.1.0.tgz`, a checked-in package snapshot, so a
fresh checkout and CI do not require the sibling project or a published package.

After changing the theme:

```sh
npm run theme:sync --prefix docs
npm run build --prefix docs
```

The sync command tests and packs the sibling theme, then updates the installed
package and lockfile. A different source path can be supplied after `--`. Include
the refreshed archive and lockfile with theme updates. Nothing is published by
this command. Once a public package exists, the dependency can move to a registry
version without changing the site's theme configuration.

The initial lockfile reports upstream low/moderate npm advisories, with no high
or critical findings as of November 3, 2026. Run `npm audit` when updating the
stack and review these before public release; do not apply forced major-version
changes without rebuilding and rechecking the site.

## Authoring

Use `.md` for Markdown and raw HTML, and `.mdx` for React components such as the
installation tabs. Use Docusaurus `:::note`, `:::tip`, and `:::warning` callouts.
Math uses KaTeX, diagrams use Mermaid, and code blocks support copy-to-clipboard.
Keep site-specific styles in `src/css`, and shared typography and tokens in the
theme package. Public URLs retain trailing slashes and the original docs paths.

Document discovery includes `index.md` and the guide directories listed in
`docusaurus.config.js`. This keeps the maintainer README, tooling, dependencies,
and generated site output out of the published documentation while allowing
underscore-prefixed API pages to remain visible. A new top-level content
directory should be added to that list as well as the sidebar where appropriate.
