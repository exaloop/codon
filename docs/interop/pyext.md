Codon includes a build mode called `pyext` for generating
[Python extensions](https://docs.python.org/3/extending/extending.html)
(which are traditionally written in C, C++ or Cython):

``` bash
codon build -pyext extension.codon  # add -release to enable optimizations
```

`codon build -pyext` accepts the following options:

- `-o <output object>`: Writes the compilation result to the specified file.
- `-module <module name>`: Specifies the generated Python module's name.

{% hint style="warning" %}
It is recommended to use the `pyext` build mode with Python versions 3.9
and up.
{% endhint %}

# Functions

Extension functions written in Codon should generally be fully typed:

``` python
def foo(a: int, b: float, c: str):  # return type will be deduced
    return a * b + float(c)
```

The `pyext` build mode will automatically generate all the necessary wrappers
and hooks for converting a function written in Codon into a function that's
callable from Python.

Function arguments that are not explicitly typed will be treated as generic
Python objects, and operated on through the CPython API.

Function overloads are also possible in Codon:

``` python
def bar(x: int):
    return x + 2

@overload
def bar(x: str):
    return x * 2
```

This will result in a single Python function `bar()` that dispatches to the
correct Codon `bar()` at runtime based on the argument's type (or raise a
`TypeError` on an invalid input type).

# Types

Codon class definitions can also be converted to Python extension types via
the `@dataclass(python=True)` decorator:

``` python
@dataclass(python=True)
class Vec:
    x: float
    y: float

    def __init__(self, x: float = 0.0, y: float = 0.0):
        self.x = x
        self.y = y

    def __add__(self, other: Vec):
        return Vec(self.x + other.x, self.y + other.y)

    def __add__(self, other: float):
        return Vec(self.x + other, self.y + other)

    def __repr__(self):
        return f'Vec({self.x}, {self.y})'
```

Now in Python (assuming we compile to a module `vec`):

``` python
from vec import Vec

a = Vec(x=3.0, y=4.0)  # Vec(3.0, 4.0)
b = a + Vec(1, 2)      # Vec(4.0, 6.0)
c = b + 10.0           # Vec(14.0, 16.0)
```

# Building with `setuptools`

Codon's `pyext` build mode can be used with `setuptools`. Here is a minimal example:

``` python
# setup.py
import os
import sys
import shutil
from pathlib import Path
from setuptools import setup, Extension
from setuptools.command.build_ext import build_ext

# Find Codon
codon_path = os.environ.get('CODON_DIR')
if not codon_path:
    c = shutil.which('codon')
    if c:
        codon_path = Path(c).parent / '..'
else:
    codon_path = Path(codon_path)
for path in [
    os.path.expanduser('~') + '/.codon',
    os.getcwd() + '/..',
]:
    path = Path(path)
    if not codon_path and path.exists():
        codon_path = path
        break

if (
    not codon_path
    or not (codon_path / 'include' / 'codon').exists()
    or not (codon_path / 'lib' / 'codon').exists()
):
    print(
        'Cannot find Codon.',
        'Please either install Codon (https://github.com/exaloop/codon),',
        'or set CODON_DIR if Codon is not in PATH.',
        file=sys.stderr,
    )
    sys.exit(1)
codon_path = codon_path.resolve()
print('Found Codon:', str(codon_path))

# Build with Codon
class CodonExtension(Extension):
    def __init__(self, name, source):
        self.source = source
        super().__init__(name, sources=[], language='c')

class BuildCodonExt(build_ext):
    def build_extensions(self):
        pass

    def run(self):
        inplace, self.inplace = self.inplace, False
        super().run()
        for ext in self.extensions:
            self.build_codon(ext)
        if inplace:
            self.copy_extensions_to_source()

    def build_codon(self, ext):
        extension_path = Path(self.get_ext_fullpath(ext.name))
        build_dir = Path(self.build_temp)
        os.makedirs(build_dir, exist_ok=True)
        os.makedirs(extension_path.parent.absolute(), exist_ok=True)

        codon_cmd = str(codon_path / 'bin' / 'codon')
        optimization = '-debug' if self.debug else '-release'
        self.spawn([codon_cmd, 'build', optimization, '--relocation-model=pic', '-pyext',
                    '-o', str(extension_path) + ".o", '-module', ext.name, ext.source])

        ext.runtime_library_dirs = [str(codon_path / 'lib' / 'codon')]
        self.compiler.link_shared_object(
            [str(extension_path) + '.o'],
            str(extension_path),
            libraries=['codonrt'],
            library_dirs=ext.runtime_library_dirs,
            runtime_library_dirs=ext.runtime_library_dirs,
            extra_preargs=['-Wl,-rpath,@loader_path'],
            debug=self.debug,
            build_temp=self.build_temp,
        )
        self.distribution.codon_lib = extension_path

setup(
    name='mymodule',
    version='0.1',
    packages=['mymodule'],
    ext_modules=[
        CodonExtension('mymodule', 'mymodule.codon'),
    ],
    cmdclass={'build_ext': BuildCodonExt}
)
```

Then, for example, we can build with:

``` bash
python3 setup.py build_ext --inplace
```

Finally, we can `import mymodule` in Python and use the module.
