Unless you really need to build Codon for whatever reason, we strongly
recommend using pre-built binaries if possible.

# Dependencies

Codon uses an LLVM fork based on LLVM 15. To build it, you can do:

``` bash
git clone --depth 1 -b codon https://github.com/exaloop/llvm-project
mkdir -p llvm-project/llvm/build
cd llvm-project/llvm/build
cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DLLVM_INCLUDE_TESTS=OFF \
    -DLLVM_ENABLE_RTTI=ON \
    -DLLVM_ENABLE_ZLIB=OFF \
    -DLLVM_ENABLE_TERMINFO=OFF \
    -DLLVM_TARGETS_TO_BUILD=all
make
make install
```

# Build

The following can generally be used to build Codon. The build process
will automatically download and build several smaller dependencies.

``` bash
mkdir build
(cd build && cmake .. -DCMAKE_BUILD_TYPE=Release \
                      -DLLVM_DIR=$(llvm-config --cmakedir) \
                      -DCMAKE_C_COMPILER=clang \
                      -DCMAKE_CXX_COMPILER=clang++)
cmake --build build --config Release
```

This should produce the `codon` executable in the `build` directory, as
well as `codon_test` which runs the test suite.
