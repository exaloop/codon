import os
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


from_cwd = lambda relpath: os.path.realpath(f"{os.getcwd()}/{relpath}")


llvm_config: str
llvm_config_candidates = ["llvm-config-12", "llvm-config", "llvm/bin/llvm-config"]
for candidate in llvm_config_candidates:
    if exists(candidate):
        llvm_config = candidate
        break
else:
    raise FileNotFoundError("Cannot find llvm-config; is llvm installed?")

llvm_include_dir = get_output(llvm_config, "--includedir")
llvm_lib_dir = get_output(llvm_config, "--libdir")

codon_include_dir = os.environ.get("CODON_INCLUDE_DIR", from_cwd("../../build/include"))
codon_lib_dir = os.environ.get("CODON_LIB_DIR", from_cwd("../../build"))

print(f"<llvm>  {llvm_include_dir}, {llvm_lib_dir}")
print(f"<codon> {codon_include_dir}, {codon_lib_dir}")

extensions = [
    Extension(
        "codon_jit",
        sources=["src/jit.pyx"],
        libraries=["codonc", "codonrt"],
        language="c++",
        extra_compile_args=["-w", "-std=c++17"],
        extra_link_args=[f"-Wl,-rpath,{codon_lib_dir}"],
        include_dirs=[llvm_include_dir, codon_include_dir],
        library_dirs=[llvm_lib_dir, codon_lib_dir],
    )
]

setup(
    name="codon",
    version="0.1.0",
    cmdclass={"build_ext": build_ext},
    ext_modules=extensions,
    packages=["codon"],
    package_dir={"codon": "src"}
)
