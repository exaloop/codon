# Codon Documentation

The documentation uses Docusaurus 3 and the shared `@exaloop/theme` package.
Content stays in `../docs`; this directory owns site configuration, navigation,
build tooling, and Codon-specific styles. The Exaloop platform is the visual
reference for typography, spacing, and light/dark colors.

## Preview Locally

Use Node.js 22 or later, Python 3.11 or later, and a Codon installation. From the
repository root:

```sh
python3 -m venv build/docs-venv
build/docs-venv/bin/python -m pip install -r website/requirements.txt
npm ci --prefix website

CODON_STDLIB="$HOME/.codon/lib/codon/stdlib"
"$HOME/.codon/bin/codon" doc "$CODON_STDLIB" > build/docs-api.json
build/docs-venv/bin/python scripts/docgen.py \
  build/docs-api.json docs/libraries/api "$CODON_STDLIB"

npm start --prefix website -- --port 3040
```

Open `http://localhost:3040`. Guide edits update automatically. After regenerating
the API, restart the server to refresh its search index and staged assets.

For a compiler checkout, use `build/codon` and stage both `stdlib/` and the
generated tables in `build/stdlib/` into one clean tree containing only `.codon`
files. Set `CODON_PATH` to that tree when running `doc`. Supply every source root
appearing in the emitted JSON to `docgen.py` (the staged tree, `stdlib/`, and
`build/stdlib/`) so paths remain relative.
Regeneration replaces Markdown in `docs/libraries/api`; keep authored guides
outside that generated directory.

## Verify the Production Site

```sh
npm run build --prefix website
npm run serve --prefix website -- --port 3040
```

The build checks Markdown links and routes, then crawls the rendered HTML for
local files and anchors. It also checks every API search target and all 25 legacy
redirects. Docusaurus's own anchor check cannot see IDs inside the generated API
HTML, so the final HTML check is mandatory instead. Its report is written to
`website/build/link-report.json`. Search is indexed during the production build;
use `serve` to test search rather than the development server.

The build also exports each documentation page as `index.md` beside its HTML.
The page toolbar copies or downloads this rendered Markdown, copies the page URL,
and links to GitHub for editing authored pages. Generated API pages retain their
source links instead. Use the production preview to test Markdown downloads;
the development server does not generate them.

CI regenerates the API with its compiler artifact, runs these checks, and builds
`website/build`. Each successful docs job uploads that directory as the
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
npm run theme:sync --prefix website
npm run build --prefix website
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
