import argparse
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import textwrap
import unittest


ROOT = Path(__file__).absolute().parents[1]
parser = argparse.ArgumentParser()
parser.add_argument('--codon', type=Path, default=ROOT / 'build/codon')
parser.add_argument('--renderer-python', type=Path, default=Path(sys.executable))
options, remaining = parser.parse_known_args()
CODON = str(options.codon.absolute())
RENDERER_PYTHON = str(options.renderer_python.absolute())


class DocumentationTest(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name).resolve()

    def source(self, name, content):
        target = self.root / name
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_text(textwrap.dedent(content), encoding='utf-8')
        return target

    def extract(self, target, *arguments):
        result = subprocess.run([CODON, 'doc', str(target), *map(str, arguments)],
                                text=True, capture_output=True, timeout=120)
        self.assertEqual(result.returncode, 0, result.stderr)
        document = json.loads(result.stdout)
        symbols = document['symbols']
        for value in symbols.values():
            if 'module' in value:
                self.assertIn(value['module'], symbols, value.get('qualified_name'))
                self.assertEqual(symbols[value['module']]['kind'], 'module')
        return document

    def render(self, document, output, *arguments, success=True):
        source = self.root / 'api.json'
        source.write_text(json.dumps(document) + '\n', encoding='utf-8')
        result = subprocess.run([RENDERER_PYTHON, str(ROOT / 'scripts/docgen.py'),
                                 str(source), str(output), *map(str, arguments)],
                                text=True, capture_output=True, timeout=120)
        self.assertEqual(result.returncode == 0, success, result.stdout + result.stderr)
        return result

    def test_schema_and_signatures(self):
        source = self.source('example.codon', '''\
            """Example library."""
            class Record:
                """A record."""
                pass
            def read(value: Int[64], fallback: int | str = 0) -> str:
                """Read \\x01 and quotes: \\"text\\"."""
                return str(value)
            def empty():
                pass
            def generic[T](value: T, /, *, flag: bool = True) -> T:
                pass
            def variadic(first: int, /, *args: int, flag: bool = True, **kwargs: str):
                pass
            @extend
            class Record:
                def read(self, /) -> str:
                    pass
            @extend
            class Record:
                @overload
                def read(self, value: str, /) -> str:
                    pass
        ''')
        document = self.extract(source)
        self.assertEqual(document['schema_version'], 2)
        symbols = document['symbols']
        module = next(value for value in symbols.values()
                      if value.get('kind') == 'module' and value['path'] == str(source))
        members = {symbols[key]['name']: symbols[key] for key in module['children']
               if symbols[key].get('type') != 'extension'}
        self.assertEqual(members['Record']['members'], [])
        self.assertEqual(members['empty']['args'], [])
        annotation = members['read']['args'][0]['type']
        self.assertEqual(annotation['args'][0], {'kind': 'literal', 'text': '64'})
        self.assertEqual(members['read']['return']['text'], 'str')
        self.assertIn('|', members['read']['args'][1]['type']['text'])
        self.assertIn('\x01', members['read']['doc'])
        self.assertEqual(members['read']['source']['path'], str(source))
        identities = [value['symbol_id'] for value in symbols.values()]
        self.assertEqual(len(identities), len(set(identities)))
        output = self.root / 'rendered'
        self.render(document, output)
        page = (output / 'modules/example/index.md').read_text()
        self.assertIn('generic[T: type](value: T, /, *, flag: bool = True) -&gt; T', page)
        self.assertIn('variadic(first: int, /, *args: int, flag: bool = True, **kwargs: str)', page)

    def test_module_scopes_and_imports(self):
        self.source('models.codon', '''\
            class Record:
                value: int
        ''')
        self.source('other.codon', '''\
            class Record:
                value: str
        ''')
        self.source('api.codon', '''\
            import models as model
            from other import Record as Other
            Alias = model.Record
            def read(value: model.Record, other: Other) -> Later:
                pass
            class Later:
                pass
        ''')
        document = self.extract(self.root)
        symbols = document['symbols']
        modules = {value['name']: value for value in symbols.values()
                   if value.get('kind') == 'module' and value.get('owned')}
        self.assertEqual(set(modules), {'api', 'models', 'other'})
        paths = [value['path'] for value in symbols.values() if value.get('kind') == 'module']
        self.assertEqual(len(paths), len(set(paths)))
        members = {symbols[key]['name']: symbols[key] for key in modules['api']['children']}
        first = members['read']['args'][0]['type']['target']
        second = members['read']['args'][1]['type']['target']
        self.assertNotEqual(first, second)
        self.assertEqual(symbols[first]['qualified_name'], 'models.Record')
        self.assertEqual(symbols[second]['qualified_name'], 'other.Record')
        self.assertEqual(members['Alias']['kind'], 'alias')
        self.assertEqual(members['Alias']['target'], first)
        self.assertFalse(members['Alias']['imported'])
        self.assertTrue(members['model']['imported'])
        self.assertTrue(members['Other']['imported'])
        self.assertEqual(members['read']['return']['target'],
                         next(key for key, value in symbols.items()
                              if value.get('qualified_name') == 'api.Later'))
        output = self.root / 'rendered'
        self.render(document, output)
        page = (output / 'modules/api/index.md').read_text()
        self.assertIn('id="alias-alias"', page)
        self.assertNotIn('id="alias-model"', page)
        self.assertNotIn('id="alias-other"', page)
        search = json.loads((output / 'api-search-index.json').read_text())
        qualified = {value['qualified'] for value in search}
        self.assertIn('api.Alias', qualified)
        self.assertNotIn('api.model', qualified)
        self.assertNotIn('api.Other', qualified)

    def test_package_discovery(self):
        package = self.source('lib/__init__.py', 'from .models import *\n').parent
        self.source('lib/models.py', 'class Record:\n    pass\n')
        self.source('lib/api.codon', '''\
            from . import models
            import lib.models
            def read(value: models.Record) -> lib.models.Record:
                raise RuntimeError("never executed")
        ''')
        self.source('lib/README.md', 'This is not source code.\n')
        self.source('lib/data.bin', '\x01\x02\n')
        self.source('lib/tests/broken.py', 'invalid syntax !\n')
        self.source('lib/.hidden/broken.py', 'invalid syntax !\n')
        self.source('lib/__init_test__.codon', 'invalid syntax !\n')
        document = self.extract(package, '--root', self.root, '--exclude', 'lib/tests')
        symbols = document['symbols']
        modules = {value['name']: value for value in symbols.values()
                   if value.get('kind') == 'module' and value.get('owned')}
        self.assertEqual(set(modules), {'lib', 'lib.models', 'lib.api'})
        exports = [symbols[key] for key in modules['lib']['children']]
        self.assertEqual([(value['kind'], value['name']) for value in exports],
                         [('alias', 'Record')])
        self.assertTrue(all(value['imported'] for value in exports))
        function = next(value for value in symbols.values()
                        if value.get('qualified_name') == 'lib.api.read')
        self.assertEqual(function['args'][0]['type']['target'], function['return']['target'])
        self.assertEqual(document, self.extract(package, '--root', self.root,
                                               '--exclude', 'lib/tests'))

    def test_invalid_inputs(self):
        for target in (self.root / 'missing', self.source('README.md', 'text\n'),
                       self.root):
            with self.subTest(target=target):
                result = subprocess.run([CODON, 'doc', '--root', str(self.root), str(target)],
                                        text=True, capture_output=True, timeout=120)
                self.assertNotEqual(result.returncode, 0)
                self.assertTrue(result.stderr)
                self.assertFalse(result.stdout)

    def test_cycles_and_extensions(self):
        self.source('lib/__init__.codon', '')
        self.source('lib/first.codon', '''\
            from .second import Other
            class Record[T]:
                value: T
                def peer(self) -> Other:
                    pass
            Integers = Record[int]
            def use(value: Record[int]) -> MissingExternal:
                pass
        ''')
        self.source('lib/second.codon', '''\
            from .first import Record
            from .first import min as minimum
            class Other:
                def peer(self) -> Record[int]:
                    pass
            @extend
            class Record:
                def added(self) -> T:
                    pass
        ''')
        document = self.extract(self.root / 'lib')
        symbols = document['symbols']
        first = next(value for value in symbols.values()
                     if value.get('qualified_name') == 'lib.first.use')
        self.assertEqual(first['return'], {'kind': 'expression', 'text': 'MissingExternal'})
        parent = first['args'][0]['type']['base']['target']
        extension = next(value for value in symbols.values()
                         if value.get('qualified_name') == 'lib.second.Record'
                         and value.get('type') == 'extension')
        minimum = next(value for value in symbols.values()
                   if value.get('qualified_name') == 'lib.second.minimum')
        self.assertEqual(symbols[minimum['target']]['name'], 'min')
        self.assertEqual(extension['parent'], parent)
        self.assertEqual(extension['generics'], ['T'])
        specialized = next(value for value in symbols.values()
                   if value.get('qualified_name') == 'lib.first.Integers')
        self.assertEqual(specialized['kind'], 'alias')
        self.assertEqual(specialized['target'], parent)
        self.assertEqual(specialized['type']['text'], 'Record[int]')
        output = self.root / 'rendered'
        self.render(document, output)
        self.assertIn('added(', (output / 'classes/lib/first/Record/index.md').read_text())
        dependency_document = self.extract(self.root / 'lib/first.codon')
        self.assertEqual(sum(value.get('owned', False)
                             for value in dependency_document['symbols'].values()), 1)

    def test_legacy_and_index_module(self):
        source = self.source('index.py', 'def read():\n    pass\n')
        document = {
            '1': {'kind': 'module', 'path': str(source), 'children': ['2', '3']},
            '2': {'kind': 'function', 'name': 'read', 'args': [], 'return': 'int'},
            '3': {'kind': 'function', 'name': 'old', 'args': [], 'ret': 'str'},
        }
        output = self.root / 'rendered'
        self.render(document, output, self.root)
        page = (output / 'modules/index/index.md').read_text()
        self.assertIn('read() -&gt; int', page)
        self.assertIn('old() -&gt; str', page)
        self.assertIn('Modules', (output / 'modules/index.md').read_text())
        self.assertNotIn('View source', page)
        outside = self.source('outside.json', '{}\n')
        unsafe = self.root / 'unsafe'
        unsafe.mkdir()
        (unsafe / '.codon-doc-manifest.json').symlink_to(outside)
        result = self.render(document, unsafe, self.root, success=False)
        self.assertIn('manifest symlink', result.stderr)
        self.assertEqual(outside.read_text(), '{}\n')

    def test_project_rendering_and_owned_output(self):
        package = self.source('lib/__init__.codon', '''\
            from .models import Record as ImportedRecord
            PublicRecord = ImportedRecord
        ''').parent
        self.source('lib/models.py', '''\
            class Base:
                pass
            class Record(Base):
                def read(self, value: Int[64]) -> str:
                    pass
        ''')
        self.source('lib/internal.codon', 'class Record:\n    pass\n')
        document = self.extract(package, '--root', self.root)
        output = self.root / 'rendered'
        output.mkdir()
        authored = output / 'guide.md'
        authored.write_text('Keep this page.\n', encoding='utf-8')
        arguments = ('--project', 'Example', '--route', '/reference/api',
                     '--source-url', 'https://example.com/src/{revision}',
                     '--source-revision', 'v1')
        self.render(document, output, *arguments)
        self.assertEqual(authored.read_text(), 'Keep this page.\n')
        self.assertIn('Example Reference', (output / 'index.md').read_text())
        first = output / 'classes/lib/models/Record/index.md'
        second = output / 'classes/lib/internal/Record/index.md'
        self.assertTrue(first.exists())
        self.assertTrue(second.exists())
        page = first.read_text()
        self.assertIn('class Record(Base)', page)
        self.assertIn('Int[64]', page)
        self.assertIn('-&gt; str', page)
        self.assertIn('https://example.com/src/v1/lib/models.py#L', page)
        self.assertIn('/reference/api/classes/lib/models/Record/', page)
        self.assertNotIn('exaloop/codon', page)
        alias_page = (output / 'modules/lib/index.md').read_text()
        self.assertIn('<article class="api-card api-card-alias"', alias_page)
        self.assertIn('<header class="api-card-header">', alias_page)
        self.assertIn('<span class="api-card-name">PublicRecord</span>', alias_page)
        self.assertIn('class="api-permalink"', alias_page)
        self.assertIn('<div class="api-card-body">', alias_page)
        self.assertIn('<div class="api-member-source">Alias of ', alias_page)
        self.assertIn('<code>ImportedRecord</code></a>', alias_page)
        self.assertNotIn('id="alias-importedrecord"', alias_page)
        search = json.loads((output / 'api-search-index.json').read_text())
        self.assertTrue(any(value['qualified'] == 'lib.PublicRecord' for value in search))
        self.assertFalse(any(value['qualified'] == 'lib.ImportedRecord' for value in search))
        self.assertFalse((output / 'modules/internal/types').exists())
        (package / 'internal.codon').unlink()
        self.render(self.extract(package, '--root', self.root), output, *arguments)
        self.assertFalse(second.exists())
        self.assertEqual(authored.read_text(), 'Keep this page.\n')
        conflict = self.root / 'conflict'
        conflict.mkdir()
        (conflict / 'index.md').write_text('Authored index\n', encoding='utf-8')
        result = self.render(document, conflict, success=False)
        self.assertIn('refusing to overwrite', result.stderr)
        self.assertEqual((conflict / 'index.md').read_text(), 'Authored index\n')


if __name__ == '__main__':
    unittest.main(argv=[sys.argv[0], *remaining])
