#!/bin/sh -l
set -e
set -x

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
mkdir -p build  # needed for some tests that write into this directory
if [ "${ARCH}" = "darwin-x86_64" ]; then
  # Disable numpy tests on Intel macOS since it breaks on macOS 13
  # (macOS 14 Intel runners are not free)
  export OBJC_DISABLE_INITIALIZE_FORK_SAFETY=YES
  time build-${ARCH}/codon_test --gtest_filter="-*numpy*"
  # :*python*:*core_arithmetic*:*stdlib_random_test*"
else
  time build-${ARCH}/codon_test
fi

echo "=> Standalone test..."
CODON_PATH=${CODON_DIR}/lib/codon/stdlib test/app/test.sh build-${ARCH}

echo "=> Cython test..."
CODON_PATH=${CODON_DIR}/lib/codon/stdlib python test/python/cython_jit.py

echo "=> pyext test..."
(cd test/python && python setup.py build_ext --inplace && python pyext.py)

# GPU test; only on select platforms
if command -v nvcc &> /dev/null; then
  echo "=> CUDA test..."
  nvcc_version=$(nvcc --version | grep "release" | awk '{print $NF}')
  echo "CUDA Version: $nvcc_version"
  ${CODON_DIR}/bin/codon run -release test/transform/gpu.codon
fi
