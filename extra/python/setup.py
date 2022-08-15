import os
import sys
from pathlib import Path
from Cython.Distutils import build_ext
from setuptools import setup
from setuptools.extension import Extension
import tempfile
from config.config import *

codon_dir = Path(os.environ.get(
    "CODON_DIR", os.path.realpath(f"{os.getcwd()}/../../build")
))
ext = "dylib" if sys.platform == "darwin" else "so"

root = Path(__file__).resolve().parent
def symlink(target):
    tmp = tempfile.mktemp()
    os.symlink(str(target.resolve()), tmp)
    os.rename(tmp, str(root / "codon" / target.name))
symlink(codon_dir / ".." / "stdlib")
symlink(codon_dir / "lib" / "codon" / ("libcodonc." + ext))
symlink(codon_dir / "lib" / "codon" / ("libcodonrt." + ext))
symlink(codon_dir / "lib" / "codon" / ("libomp." + ext))

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
    library_dirs=[str(root / "codon")],
)

setup(
    name="codon",
    version=CODON_VERSION,
    install_requires=["astunparse"],
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
