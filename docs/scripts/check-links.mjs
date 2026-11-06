import assert from 'node:assert/strict';
import {existsSync, readFileSync, readdirSync, statSync, writeFileSync} from 'node:fs';
import {fileURLToPath} from 'node:url';
import {createRequire} from 'node:module';
import {load} from 'cheerio';

const require = createRequire(import.meta.url);
const redirects = require('../redirects.js');
const output = fileURLToPath(new URL('../build/', import.meta.url));
const origin = 'https://docs.exaloop.io';
const pages = new Map();
const failures = new Set();
let checked = 0;

for (const filename of readdirSync(output, {recursive: true}).filter(name => name.endsWith('.html'))) {
  const document = load(readFileSync(`${output}/${filename}`, 'utf8'));
  const route = `/${filename.replace(/index\.html$/, '')}`;
  const ids = new Set(document('[id], a[name]').toArray().map(element => element.attribs.id || element.attribs.name));
  pages.set(route, {document, ids});
}

function check(target, source) {
  if (!target || target.startsWith('data:')) return;
  const url = new URL(target, `${origin}${source}`);
  if (url.origin !== origin) return;
  checked++;
  const pathname = decodeURIComponent(url.pathname);
  const route = pathname.replace(/index\.html$/, '');
  const destination = pages.get(route) || pages.get(`${route}/`);
  if (!destination) {
    const filename = `${output}${pathname}`;
    if (!existsSync(filename) || !statSync(filename).isFile()) failures.add(`${source} -> ${target} (missing file)`);
    return;
  }
  const anchor = decodeURIComponent(url.hash.slice(1));
  if (anchor && !destination.ids.has(anchor)) failures.add(`${source} -> ${target} (missing anchor)`);
}

for (const [route, {document}] of pages) {
  if (document('.api-signature').length !== document('.api-signature pre').length) {
    failures.add(`${route} (API signatures lost their preformatted layout)`);
  }
  for (const element of document('a[href], img[src], script[src], link[href]').toArray()) {
    if (route === '/404.html' && ['canonical', 'alternate'].includes(element.attribs.rel)) continue;
    check(element.attribs.href || element.attribs.src, route);
  }
  if (route.includes('/Users/') || route.includes('/home/')) failures.add(`${route} (local source path leaked into URL)`);
}

const search = JSON.parse(readFileSync(`${output}/libraries/api/api-search-index.json`, 'utf8'));
for (const entry of search) check(entry.url, '/libraries/api/');
for (const redirect of redirects) {
  check(redirect.to, '/');
  for (const source of [redirect.from].flat()) check(source, '/');
}

const report = {pages: pages.size, apiSymbols: search.length, checkedLinks: checked, failures: [...failures]};
writeFileSync(`${output}/link-report.json`, `${JSON.stringify(report, null, 2)}\n`);
if (failures.size) console.error([...failures].slice(0, 40).join('\n'));
assert.equal(failures.size, 0, `${failures.size} broken links; see build/link-report.json`);
console.log(`Checked ${pages.size} pages, ${search.length} API symbols, and ${checked} local links and anchors.`);
