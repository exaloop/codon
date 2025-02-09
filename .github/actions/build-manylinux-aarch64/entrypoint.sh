#!/bin/sh -l
set -e

# setup
cd /github/workspace
yum -y update
yum -y install python3 python3-devel gcc-gfortran

# env
export PYTHONPATH=$(pwd)/test/python
export CODON_PYTHON=$(python3 test/python/find-python-library.py)
python3 -m pip install -Iv pip==21.3.1 numpy==1.17.5

# deps
if [ ! -d ./llvm ]; then
  /bin/bash scripts/deps.sh 2;
fi

# build
mkdir build
export CC="$(pwd)/llvm/bin/clang"
export CXX="$(pwd)/llvm/bin/clang++"
export LLVM_DIR=$(llvm/bin/llvm-config --cmakedir)
export CODON_SYSTEM_LIBRARIES=/usr/lib64
(cd build && cmake .. -DCMAKE_BUILD_TYPE=Release \
                      -DCMAKE_C_COMPILER=${CC} \
                      -DCMAKE_CXX_COMPILER=${CXX})
cmake --build build --config Release -- VERBOSE=1
cmake --install build --prefix=codon-deploy

# build cython
export PATH=$PATH:$(pwd)/llvm/bin
python3 -m pip install cython wheel astunparse
(cd codon-deploy/python && python3 setup.py sdist)
CODON_DIR=$(pwd)/codon-deploy python3 -m pip install -v codon-deploy/python/dist/*.gz
python3 test/python/cython_jit.py

# test
export LD_LIBRARY_PATH=$(pwd)/build:$LD_LIBRARY_PATH
export PYTHONPATH=$(pwd):$PYTHONPATH
export CODON_PATH=$(pwd)/stdlib
ln -s build/libcodonrt.so .
build/codon_test

# package
export CODON_BUILD_ARCHIVE=codon-$(uname -s | awk '{print tolower($0)}')-$(uname -m).tar.gz
rm -rf codon-deploy/lib/libfmt.a codon-deploy/lib/pkgconfig codon-deploy/lib/cmake \
       codon-deploy/python/codon.egg-info codon-deploy/python/dist codon-deploy/python/build
tar -czf ${CODON_BUILD_ARCHIVE} codon-deploy
du -sh codon-deploy
