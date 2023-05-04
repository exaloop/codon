import os
import sys
import shutil

from pathlib import Path
from setuptools import setup, Extension
from setuptools.command.build_ext import build_ext


codon_path = os.environ.get("CODON_DIR")
if not codon_path:
    c = shutil.which("codon")
    if c:
        codon_path = Path(c).parent / ".."
else:
    codon_path = Path(codon_path)
for path in [
    os.path.expanduser("~") + "/.codon",
    os.getcwd() + "/..",
]:
    path = Path(path)
    if not codon_path and path.exists():
        codon_path = path
        break

if (
    not codon_path
    or not (codon_path / "include" / "codon").exists()
    or not (codon_path / "lib" / "codon").exists()
):
    print(
        "Cannot find Codon.",
        'Please either install Codon (/bin/bash -c "$(curl -fsSL https://exaloop.io/install.sh)"),',
        "or set CODON_DIR if Codon is not in PATH or installed in ~/.codon",
        file=sys.stderr,
    )
    sys.exit(1)
codon_path = codon_path.resolve()
print("Codon: " + str(codon_path))


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

        optimization = '-debug' if self.debug else '-release'
        self.spawn([
            str(codon_path / "bin" / "codon"), 'build', optimization, "--relocation-model=pic",
            '-pyext', '-o', str(extension_path) + ".o", '-module', ext.name, ext.source])

        print('-->', extension_path)
        ext.runtime_library_dirs = [str(codon_path / "lib" / "codon")]
        self.compiler.link_shared_object(
            [str(extension_path) + ".o"],
            str(extension_path),
            libraries=["codonrt"],
            library_dirs=ext.runtime_library_dirs,
            runtime_library_dirs=ext.runtime_library_dirs,
            extra_preargs=['-Wl,-rpath,@loader_path'],
            # export_symbols=self.get_export_symbols(ext),
            debug=self.debug,
            build_temp=self.build_temp,
        )
        self.distribution.codon_lib = extension_path

setup(
    name='myext',
    version='0.1',
    packages=['myext'],
    ext_modules=[
        CodonExtension('myext', 'myextension.codon'),
    ],
    cmdclass={'build_ext': BuildCodonExt}
)

setup(
    name='myext2',
    version='0.1',
    packages=['myext2'],
    ext_modules=[
        CodonExtension('myext2', 'myextension2.codon'),
    ],
    cmdclass={'build_ext': BuildCodonExt}
)
