import subprocess

from Cython.Distutils import build_ext
from setuptools import setup
from setuptools.extension import Extension


def get_output(command):
    ps = subprocess.run(command.split(" "), stdout=subprocess.PIPE)
    return ps.stdout.decode("utf8").strip()


llvm_include_dir = get_output("llvm-config-12 --includedir")
llvm_lib_dir = get_output("llvm-config-12 --libdir")

extensions = [
    Extension(
        "codon_jit",
        sources=["extra/cython/jit.pyx"],
        libraries=["codonc", "codonrt"],
        language="c++",
        extra_compile_args=["-w", "-std=c++17"],
        include_dirs=[llvm_include_dir, "./build/include"],
        library_dirs=[llvm_lib_dir, "./build"],
    )
]

setup(cmdclass={"build_ext": build_ext}, ext_modules=extensions)
