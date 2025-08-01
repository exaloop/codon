#!/bin/sh -l
set -e

WORKSPACE="${1}"
ARCH="$2"

echo "Workspace: ${WORKSPACE}; arch: ${ARCH}"
cd "$WORKSPACE"

if [ "$(uname -s)" = "Linux" ]; then
  ~/.pyenv/bin/pyenv global 3.11
  export PATH="/root/.pyenv/shims:${PATH}"

  # needed for dylib test
  ln -s -f $(pwd)/build-${ARCH}/libcodonrt.so .
else
  # needed for dylib test
  ln -s -f $(pwd)/build-${ARCH}/libcodonrt.dylib .
fi
export CODON_PYTHON=$(python ${WORKSPACE}/test/python/find-python-library.py)
export PYTHONPATH=${WORKSPACE}/test/python
export CODON_DIR=$(pwd)/codon-deploy-${ARCH}

echo "=> Unit tests..."
mkdir -p build  # needed for some tests
time build-${ARCH}/codon_test

echo "=> Standalone test..."
CODON_PATH=${CODON_DIR}/lib/codon/stdlib test/app/test.sh build-${ARCH}

echo "=> Cython test..."
CODON_PATH=${CODON_DIR}/lib/codon/stdlib python test/python/cython_jit.py

echo "=> pyext test..."
(cd test/python && python setup.py build_ext --inplace && python pyext.py)

