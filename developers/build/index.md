# Build from source

tip

The [prebuilt installation](https://docs.exaloop.io/start/install/) is usually sufficient for compiling applications. Building from source is useful for compiler development, custom toolchains or platforms that need a locally configured build.

## Dependencies

Codon uses an LLVM fork based on LLVM 20. To build it, you can do the following:

```bash
git clone --depth 1 -b codon https://github.com/exaloop/llvm-project
cmake -S llvm-project/llvm -B llvm-project/build \
    -DCMAKE_BUILD_TYPE=Release \
    -DLLVM_INCLUDE_TESTS=OFF \
    -DLLVM_ENABLE_RTTI=ON \
    -DLLVM_ENABLE_ZLIB=OFF \
    -DLLVM_ENABLE_ZSTD=OFF \
    -DLLVM_ENABLE_PROJECTS="openmp" \
    -DLLVM_TARGETS_TO_BUILD=all \
    -DLLVM_ENABLE_RUNTIMES="compiler-rt" \
    -DCOMPILER_RT_BUILD_BUILTINS=ON \
    -DCOMPILER_RT_BUILD_SANITIZERS=OFF \
    -DCOMPILER_RT_BUILD_XRAY=OFF \
    -DCOMPILER_RT_BUILD_LIBFUZZER=OFF \
    -DCOMPILER_RT_BUILD_PROFILE=OFF \
    -DCOMPILER_RT_BUILD_MEMPROF=OFF \
    -DCOMPILER_RT_BUILD_CTX_PROFILE=OFF \
    -DCOMPILER_RT_BUILD_ORC=OFF \
    -DCOMPILER_RT_BUILD_CRT=OFF \
    -DCOMPILER_RT_DEFAULT_TARGET_ONLY=ON \
    -DCOMPILER_RT_INCLUDE_TESTS=OFF
cmake --build llvm-project/build
cmake --install llvm-project/build --prefix="$PWD/llvm-project/install"
export LLVM_DIR="$PWD/llvm-project/install/lib/cmake/llvm"
```

You can use `-DLLVM_ENABLE_PROJECTS="clang;openmp"` if you do not have `clang` installed on your system. We also recommend setting a local prefix during installation to avoid clashes with the system LLVM.

## Build

Python 3.8+ is required to verify and unpack the bundled standard-library tables. This step is offline and does not depend on Python's Unicode database version. Generated tables live in the build tree and are embedded and installed automatically. See the [table maintenance guide](https://github.com/exaloop/codon/blob/master/scripts/unicode/README.md) for the separate, version-pinned regeneration workflow.

Codon requires `libgfortran`, the parent directory of which must be specified via the `CODON_SYSTEM_LIBRARIES` environment variable. For example, on macOS, with a `brew`\-installed `libgfortran` (obtainable via `brew install gcc`):

```bash
export CODON_SYSTEM_LIBRARIES=/opt/homebrew/opt/gcc/lib/gcc/current
```

On Linux:

```bash
export CODON_SYSTEM_LIBRARIES=/usr/lib/x86_64-linux-gnu
```

With that LLVM installation selected, obtain the Codon source and configure its build. An existing checkout can be used in place of the clone command. The build process downloads and builds several smaller dependencies.

```bash
git clone https://github.com/exaloop/codon.git
cd codon
cmake -S . -B build \
    -DCMAKE_BUILD_TYPE=Release \
    -DLLVM_DIR="$LLVM_DIR" \
    -DCMAKE_C_COMPILER=clang \
    -DCMAKE_CXX_COMPILER=clang++
cmake --build build --config Release
cmake --install build --prefix="$PWD/install"
```

This will produce the `codon` executable in the `install/bin` directory, as well as `codon_test` in the `build` directory which runs the test suite. Additionally, a number of shared libraries are produced in `install/lib/codon`:

-   `libcodonc`: The compiler library used by the `codon` command-line tool.
-   `libcodonrt`: The runtime library used during execution.
-   `libomp`: OpenMP runtime used to execute parallel code.

warning

`LLVM_DIR` needs to refer to Codon's LLVM fork, not an unrelated system LLVM installation. The absolute path exported above remains valid after changing into the Codon checkout.

## Jupyter support

To enable Jupyter support, you will need to build the Jupyter plugin:

```bash
cmake -S jupyter -B jupyter/build \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_C_COMPILER=clang \
    -DCMAKE_CXX_COMPILER=clang++ \
    -DLLVM_DIR="$LLVM_DIR" \
    -DCODON_PATH="$PWD/install"
cmake --build jupyter/build
cmake --install jupyter/build
```

On Linux, add `-DXEUS_USE_DYNAMIC_UUID=ON` to the configuration command. If CMake cannot discover OpenSSL, its installation prefix can be supplied with `-DOPENSSL_ROOT_DIR=/path/to/openssl`. For a Homebrew installation, the value reported by `brew --prefix openssl@3` avoids assuming an Intel or Apple Silicon prefix. CMake can then select the appropriate SSL and crypto libraries.

The plugin is installed into the selected Codon installation. The [Jupyter guide](https://docs.exaloop.io/integrations/jupyter/) describes registering it with a frontend after the build.

Source: https://docs.exaloop.io/developers/build/
