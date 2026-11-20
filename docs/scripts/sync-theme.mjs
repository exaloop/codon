import {mkdirSync, rmSync} from 'node:fs';
import {fileURLToPath} from 'node:url';
import {resolve} from 'node:path';
import {execFileSync} from 'node:child_process';

const website = fileURLToPath(new URL('../', import.meta.url));
const source = resolve(process.argv[2] || resolve(website, '../../theme'));
const vendor = resolve(website, 'vendor');
mkdirSync(vendor, {recursive: true});
execFileSync('npm', ['test'], {cwd: source, stdio: 'inherit'});
const [packed] = JSON.parse(execFileSync('npm', ['pack', '--json', '--ignore-scripts', '--pack-destination', vendor], {cwd: source, encoding: 'utf8'}));
execFileSync('npm', ['install', '--save-exact', `./vendor/${packed.filename}`], {cwd: website, stdio: 'inherit'});
rmSync(resolve(website, 'node_modules/.cache'), {recursive: true, force: true});
