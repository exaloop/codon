#!/bin/sh -l
set -e

WORKSPACE="${1}"
ARCH="$2"

echo "Workspace: ${WORKSPACE}; arch: ${ARCH}"
cd "$WORKSPACE"

if [ "$(uname -s)" = "Linux" ]; then
  ~/.pyenv/bin/pyenv global 3.11
  export PATH="/root/.pyenv/shims:${PATH}"
  python --version
else
  python --version
fi
export CODON_PYTHON=$(python ${WORKSPACE}/test/python/find-python-library.py)
export PYTHONPATH=${WORKSPACE}/test/python

time build-${ARCH}/codon_test
test/app/test.sh build
CODON_PATH=${CODON_DIR}/lib/codon/stdlib python test/python/cython_jit.py
(cd test/python && python setup.py build_ext --inplace && python pyext.py)

