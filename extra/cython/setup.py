from Cython.Build import cythonize
from Cython.Distutils import build_ext
from setuptools import setup
from setuptools.extension import Extension

extensions = [
    Extension("codon_jit",
        sources=[
            "extra/cython/jit.pyx"
        ],
        libraries=[
            "codonc",
            "codonrt"
        ],
        language="c++",
        extra_compile_args=[
            "-w",
            "-std=c++17"
        ],
        include_dirs=[
            "/usr/lib/llvm-12/include",
            "./build/include"
        ],
        library_dirs=[
            "/usr/lib/llvm-12/lib",
            "./build"
        ]
    )
]

setup(
    cmdclass = {'build_ext': build_ext},
    ext_modules = extensions
)
