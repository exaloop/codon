import {cpSync, existsSync, mkdirSync, writeFileSync} from 'node:fs';
import {fileURLToPath} from 'node:url';
import {load} from 'cheerio';
import {icons} from 'lucide';

const docs = fileURLToPath(new URL('../', import.meta.url));
const output = fileURLToPath(new URL('../static/', import.meta.url));
const index = `${docs}/libraries/api/api-search-index.json`;
if (!existsSync(index)) {
  throw new Error('Generate the API reference first. See docs/README.md.');
}
mkdirSync(`${output}/libraries/api`, {recursive: true});
cpSync(`${docs}/img`, `${output}/img`, {recursive: true});
cpSync(index, `${output}/libraries/api/api-search-index.json`);

const sprite = load('<svg xmlns="http://www.w3.org/2000/svg"/>', {xmlMode: true});
const glyphs = {
  'arrow-right': 'ArrowRight', terminal: 'Terminal', 'book-open': 'BookOpen',
  braces: 'Braces', cpu: 'Cpu', layers: 'Layers', grid: 'Grid2X2', zap: 'Zap',
  blocks: 'Blocks', flask: 'FlaskConical', info: 'Info',
};
for (const [id, name] of Object.entries(glyphs)) {
  const symbol = sprite('<symbol/>').attr({id, viewBox: '0 0 24 24'});
  for (const [tag, attributes] of icons[name]) {
    symbol.append(sprite(`<${tag}/>`).attr(attributes));
  }
  sprite('svg').append(symbol);
}
writeFileSync(`${output}/img/doc-icons.svg`, `${sprite.xml()}\n`);
