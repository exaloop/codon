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
GTEST_PARALLEL_REVISION=cd488bdedc1d2cffb98201a17afc1b298b0b90f1
GTEST_PARALLEL="$WORKSPACE/build/gtest_parallel.py"
curl --fail --location --retry 3 \
  "https://raw.githubusercontent.com/google/gtest-parallel/${GTEST_PARALLEL_REVISION}/gtest_parallel.py" \
  --output "$GTEST_PARALLEL"
python -m unittest discover -s .github/build-linux -p test_run_tests.py
TEST_FILTER='*'
if [ "${ARCH}" = "darwin-x86_64" ]; then
  # Disable numpy tests on Intel macOS since it breaks on macOS 13
  # (macOS 14 Intel runners are not free).
  # arithmetic.codon fails in debug mode on x86 due to float16 (TODO: why?)
  export OBJC_DISABLE_INITIALIZE_FORK_SAFETY=YES
  TEST_FILTER='-*numpy*:*core_arithmetic_codon_debug*'
fi
python .github/build-linux/run_tests.py "build-${ARCH}/codon_test" \
  --gtest-parallel "$GTEST_PARALLEL" --gtest-filter="$TEST_FILTER" \
  --output-dir "build-${ARCH}/test-results"

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
if command -v nvcc &> /dev/null; then
  echo "=> CUDA test..."
  nvcc_version=$(nvcc --version | grep "release" | awk '{print $NF}')
  echo "CUDA Version: $nvcc_version"
  ${CODON_DIR}/bin/codon run -release test/transform/gpu.codon
fi
