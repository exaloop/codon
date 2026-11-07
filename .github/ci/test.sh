#!/bin/sh -l
set -e
set -x

WORKSPACE="${1}"
ARCH="$2"
SHARD_INDEX=${3:-0}
SHARD_COUNT=${4:-1}

if ! [ "$SHARD_COUNT" -gt 0 ] || ! [ "$SHARD_INDEX" -ge 0 ] || ! [ "$SHARD_INDEX" -lt "$SHARD_COUNT" ]; then
  echo "Invalid test shard: $SHARD_INDEX of $SHARD_COUNT" >&2
  exit 1
fi

echo "Workspace: ${WORKSPACE}; arch: ${ARCH}; shard: ${SHARD_INDEX}/${SHARD_COUNT}"
cd "$WORKSPACE"

if [ "$(uname -s)" = "Linux" ]; then
  ~/.pyenv/bin/pyenv global 3.11
  export PATH="/root/.pyenv/shims:${PATH}"
  export CODON_OS_TEST_REQUIRE_RAW_PATHS=1

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
RESULTS_DIR="${WORKSPACE}/test-results/${ARCH}"
mkdir -p "$RESULTS_DIR"
GTEST_TOTAL_SHARDS="$SHARD_COUNT" GTEST_SHARD_INDEX="$SHARD_INDEX" \
  build-${ARCH}/codon_test --gtest_output="xml:${RESULTS_DIR}/unit-${SHARD_INDEX}.xml"

if [ "$SHARD_INDEX" -ne 0 ]; then
  exit 0
fi

echo "=> OS differential tests..."
python test/stdlib/os_differential.py --codon "${CODON_DIR}/bin/codon"

echo "=> Standalone test..."
CODON_PATH=${CODON_DIR}/lib/codon/stdlib test/app/test.sh build-${ARCH}

echo "=> Cython test..."
CODON_PATH=${CODON_DIR}/lib/codon/stdlib python test/python/cython_jit.py

echo "=> pyext debug test..."
(cd test/python && python setup.py build_ext --inplace --debug --force && python pyext.py)

echo "=> pyext release test..."
(cd test/python && python setup.py build_ext --inplace --force && python pyext.py)

# GPU test; only on select platforms
if command -v nvcc > /dev/null 2>&1; then
  echo "=> CUDA test..."
  nvcc_version=$(nvcc --version | grep "release" | awk '{print $NF}')
  echo "CUDA Version: $nvcc_version"
  ${CODON_DIR}/bin/codon run -release test/transform/gpu.codon
fi
