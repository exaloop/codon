#!/bin/sh -l
set -e

# setup
cd /github/workspace
yum -y update
yum -y install python3 python3-devel

# env
export PYTHONPATH=$(pwd)/test/python
export CODON_PYTHON=$(python3 test/python/find-python-library.py)
python3 -m pip install --upgrade pip
python3 -m pip install numpy

# deps
if [ ! -d ./llvm ]; then
  /bin/bash scripts/deps.sh 2;
fi

# build
mkdir build
export CC="$(pwd)/llvm/bin/clang"
export CXX="$(pwd)/llvm/bin/clang++"
export LLVM_DIR=$(llvm/bin/llvm-config --cmakedir)
(cd build && cmake .. -DCMAKE_BUILD_TYPE=Release \
                      -DCMAKE_C_COMPILER=${CC} \
                      -DCMAKE_CXX_COMPILER=${CXX})
cmake --build build --config Release -- VERBOSE=1
cmake --install build --prefix=codon-deploy

# build cython
export PATH=$PATH:$(pwd)/llvm/bin
export LD_LIBRARY_PATH=$(pwd)/build:$LD_LIBRARY_PATH
export CODON_DIR=$(pwd)/build
python3 -m pip install cython wheel astunparse
python3 -m pip debug --verbose
(cd codon-deploy/python && python3 setup.py sdist)
CODON_DIR=codon-deploy python3 -m pip install -v codon-deploy/python/dist/*.gz
export PYTHONPATH=$(pwd):$PYTHONPATH
python3 test/python/cython_jit.py

# test
export CODON_PATH=$(pwd)/stdlib
ln -s build/libcodonrt.so .
build/codon_test

# package
export CODON_BUILD_ARCHIVE=codon-$(uname -s | awk '{print tolower($0)}')-$(uname -m).tar.gz
rm -f codon-deploy/lib/libfmt.a codon-deploy/lib/pkgconfig codon-deploy/lib/cmake codon-deploy/python/codon.egg-info codon-deploy/python/dist codon-deploy/python/build
tar -czf ${CODON_BUILD_ARCHIVE} codon-deploy
du -sh codon-deploy
