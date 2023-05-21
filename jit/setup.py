# Copyright (C) 2022-2023 Exaloop Inc. <https://exaloop.io>

import os
import sys
import shutil
import subprocess
from pathlib import Path
from Cython.Distutils import build_ext
from setuptools import setup
from setuptools.extension import Extension

exec(open("codon/version.py").read())

ext = "dylib" if sys.platform == "darwin" else "so"

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


if sys.platform == "darwin":
    libraries=["codonrt", "codonc"]
    linker_args = ["-Wl,-rpath," + str(codon_path / "lib" / "codon")]
else:
    libraries=["codonrt"]
    linker_args = [
        "-Wl,-rpath=" + str(codon_path / "lib" / "codon"),
        "-Wl,--no-as-needed",
        "-lcodonc",
    ]

    # TODO: handle ABI changes better
    out = subprocess.check_output(["nm", "-g", str(codon_path / "lib" / "codon" / "libcodonc.so")])
    out = [i for i in out.decode(sys.stdout.encoding).split("\n") if "jitExecuteSafe" in i]
    if out and "cxx11" not in out[0]:
        print("CXX11 ABI not detected")
        os.environ["CFLAGS"] = os.environ.get("CFLAGS", "") + " -D_GLIBCXX_USE_CXX11_ABI=0"

jit_extension = Extension(
    "codon.codon_jit",
    sources=["codon/jit.pyx", "codon/jit.pxd"],
    libraries=libraries,
    language="c++",
    extra_compile_args=["-w"],
    extra_link_args=linker_args,
    include_dirs=[str(codon_path / "include")],
    library_dirs=[str(codon_path / "lib" / "codon")],
)

setup(
    name="codon-jit",
    version=__version__,
    install_requires=["cython", "astunparse"],
    python_requires=">=3.6",
    description="Codon JIT decorator",
    url="https://exaloop.io",
    long_description="Please see https://exaloop.io for more details.",
    author="Exaloop Inc.",
    author_email="info@exaloop.io",
    license="Commercial",
    ext_modules=[jit_extension],
    packages=["codon"],
    include_package_data=True,
    cmdclass={
        "build_ext": build_ext,
    },
)
