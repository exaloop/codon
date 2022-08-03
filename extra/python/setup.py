import os
import sys
import subprocess
import shutil
from pathlib import Path

from Cython.Distutils import build_ext
from setuptools import setup
from setuptools.extension import Extension
import distutils.dir_util

from config.config import *

def exists(executable):
    ps = subprocess.run(["which", executable], stdout=subprocess.PIPE)
    return ps.returncode == 0


def get_output(*args):
    ps = subprocess.run(args, stdout=subprocess.PIPE)
    return ps.stdout.decode("utf8").strip()


def package_files(directory):
    for (path, _, fns) in os.walk(directory):
        for fn in fns:
            yield os.path.join(path, fn)


from_root = lambda relpath: os.path.realpath(f"{os.getcwd()}/../../{relpath}")

llvm_config = ""
llvm_config_candidates = [
    os.environ.get("CODON_LLVM_CONFIG", from_root("llvm/bin/llvm-config")),
    "llvm-config-12",
    "llvm-config",
]
for candidate in llvm_config_candidates:
    if exists(candidate):
        llvm_config = candidate
        break
else:
    raise FileNotFoundError("Cannot find llvm-config; is llvm installed?")

llvm_include_dir = get_output(llvm_config, "--includedir")
llvm_lib_dir = get_output(llvm_config, "--libdir")

codon_dir = Path(os.environ.get("CODON_DIR", from_root("build")))
codon_include_dir = os.environ.get("CODON_INCLUDE_DIR", codon_dir / "include")
ext = "dylib" if sys.platform == "darwin" else "so"

root = Path(os.path.dirname(os.path.realpath(__file__)))
distutils.dir_util.copy_tree(str(codon_dir / ".." / "stdlib"), str(root / "codon" / "stdlib"))
shutil.copy(codon_dir / "lib" / "codon" / ("libcodonc." + ext), root / "codon")
shutil.copy(codon_dir / "lib" / "codon" / ("libcodonrt." + ext), root / "codon")
shutil.copy(codon_dir / "lib" / "codon" / ("libomp." + ext), root / "codon")

print(f"<llvm>  {llvm_include_dir}, {llvm_lib_dir}")
print(f"<codon> {codon_include_dir}")

if sys.platform == "darwin":
    linker_args = "-Wl,-rpath,@loader_path"
else:
    linker_args = "-Wl,-rpath=$ORIGIN"

jit_extension = Extension(
    "codon.codon_jit",
    sources=["codon/jit.pyx"],
    libraries=["codonc", "codonrt"],
    language="c++",
    extra_compile_args=["-w", "-std=c++17"],
    extra_link_args=[linker_args],
    include_dirs=[llvm_include_dir, str(codon_include_dir)],
    library_dirs=[llvm_lib_dir, str(root / "codon")],
)

setup(
    name="codon",
    version=CODON_VERSION,
    python_requires='>=3.6',
    description="Codon JIT decorator",
    url="https://exaloop.io",
    long_description="Please see https://exaloop.io for more details.",
    author="Exaloop Inc.",
    author_email="info@exaloop.io",
    license="Commercial",
    cmdclass={"build_ext": build_ext},
    ext_modules=[jit_extension],
    packages=["codon"],
    include_package_data=True
)
