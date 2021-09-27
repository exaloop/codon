Building from Source
====================

Unless you really need to build Seq for whatever reason, we strongly
recommend using pre-built binaries if possible.

Dependencies
------------

Seq depends on LLVM 12, which can be installed via most package managers. To
build LLVM 12 yourself, you can do the following:

.. code-block:: bash

    git clone --depth 1 -b release/12.x https://github.com/llvm/llvm-project
    mkdir -p llvm-project/llvm/build
    cd llvm-project/llvm/build
    cmake .. \
        -DCMAKE_BUILD_TYPE=Release \
        -DLLVM_INCLUDE_TESTS=OFF \
        -DLLVM_ENABLE_RTTI=ON \
        -DLLVM_ENABLE_ZLIB=OFF \
        -DLLVM_ENABLE_TERMINFO=OFF \
        -DLLVM_TARGETS_TO_BUILD=host
    make
    make install

Build
-----

The following can generally be used to build Seq. The build process will automatically
download and build several smaller dependencies.

.. code-block:: bash

    mkdir build
    (cd build && cmake .. -DCMAKE_BUILD_TYPE=Release \
                          -DLLVM_DIR=$(llvm-config --cmakedir) \
                          -DCMAKE_C_COMPILER=clang \
                          -DCMAKE_CXX_COMPILER=clang++)
    cmake --build build --config Release

This should produce the ``seqc`` executable in the ``build`` directory, as well as
``seqtest`` which runs the test suite.
