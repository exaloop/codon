import subprocess

from Cython.Distutils import build_ext
from setuptools import setup
from setuptools.extension import Extension


def exists(executable):
    ps = subprocess.run(["which", executable], stdout=subprocess.PIPE)
    return ps.returncode == 0


def get_output(*args):
    ps = subprocess.run(args, stdout=subprocess.PIPE)
    return ps.stdout.decode("utf8").strip()


llvm_config: str
if exists("llvm-config-12"):
    llvm_config = "llvm-config-12"
elif exists("llvm-config"):
    llvm_config = "llvm-config"
else:
    raise FileNotFoundError("Cannot find llvm-config; is llvm installed?")

llvm_include_dir = get_output(llvm_config, "--includedir")
llvm_lib_dir = get_output(llvm_config, "--libdir")

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
