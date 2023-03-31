import os
import pathlib

from setuptools import setup, Extension
from setuptools.command.build_ext import build_ext

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
        extension_path = pathlib.Path(self.get_ext_fullpath(ext.name))
        build_dir = pathlib.Path(self.build_temp)
        os.makedirs(build_dir, exist_ok=True)
        os.makedirs(extension_path.parent.absolute(), exist_ok=True)

        optimization = '-debug' if self.debug else '-release'
        self.spawn([
            '../../build/codon', 'build', optimization,
            '-pyext', '-o', str(extension_path) + ".o", '-module', ext.name, ext.source])

        print('-->', extension_path)
        ext.runtime_library_dirs = ["../../build"]
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
