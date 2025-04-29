#!/bin/sh -l
set -e

# setup
cd /github/workspace
yum -y update
yum -y install gcc gcc-c++ gcc-gfortran make wget openssl-devel bzip2-devel libffi-devel xz-devel zlib-devel

# python
cd /usr/src
wget https://www.openssl.org/source/openssl-1.1.1w.tar.gz
tar xzf openssl-1.1.1w.tar.gz
cd openssl-1.1.1w
./config --prefix=/opt/openssl --openssldir=/opt/openssl no-shared
make -j2
make install

cd /usr/src
wget https://www.python.org/ftp/python/3.11.9/Python-3.11.9.tgz
tar xzf Python-3.11.9.tgz
cd Python-3.11.9
./configure --enable-optimizations --prefix=/opt/python311 --with-openssl=/opt/openssl
make -j2
make altinstall
export PATH="/opt/python311/bin:$PATH"
alias python=python3.11

# env
cd /github/workspace
export PYTHONPATH=$(pwd)/test/python
export CODON_PYTHON=$(python test/python/find-python-library.py)
python -m pip install -Iv pip==21.3.1 numpy==2.0.2

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
python -m pip install cython wheel astunparse
(cd codon-deploy/python && python setup.py sdist)
CODON_DIR=$(pwd)/codon-deploy python -m pip install -v codon-deploy/python/dist/*.gz
python test/python/cython_jit.py

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
