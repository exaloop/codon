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
BUILD_DIR=${CODON_BUILD_DIR:-build-${ARCH}}
export CODON_DIR=${CODON_DIR:-$(pwd)/codon-deploy-${ARCH}}
WINDOWS=0
case "$(uname -s)" in
  MINGW*|MSYS*) WINDOWS=1 ;;
esac

if [ "$WINDOWS" -eq 1 ]; then
  export CODON_DIR=$(cygpath -am "$CODON_DIR")
  export PYTHONPATH=$(cygpath -am "$WORKSPACE/test/python")
  export PATH="$(cygpath -u "$CODON_DIR")/bin:$(cygpath -u "$CODON_DIR")/lib/codon:$PWD/$BUILD_DIR:$PATH"
elif [ "$(uname -s)" = "Linux" ]; then
  ~/.pyenv/bin/pyenv global 3.11
  export PATH="/root/.pyenv/shims:${PATH}"
  export CODON_OS_TEST_REQUIRE_RAW_PATHS=1

  # needed for dylib test
  ln -s -f "$PWD/$BUILD_DIR/libcodonrt.so" .
else
  # needed for dylib test
  ln -s -f "$PWD/$BUILD_DIR/libcodonrt.dylib" .
fi
# Test the SDK's matching stdlib even when an out-of-tree build has an older
# stdlib directory beside it.
export CODON_PATH="${CODON_DIR}/lib/codon/stdlib"
CODON_PYTHON=$(python "$WORKSPACE/test/python/find-python-library.py")
export CODON_PYTHON
if [ "$WINDOWS" -eq 1 ]; then
  # Require the resources used by the native OS cases instead of silently
  # omitting symlink and UNC coverage when their optional settings are absent.
  CODON_OS_TEST_UNC_DIR=$(python "$WORKSPACE/test/stdlib/windows_test_fixture.py" "$WORKSPACE")
  export CODON_OS_TEST_UNC_DIR
fi
if [ "$WINDOWS" -eq 0 ]; then
  export PYTHONPATH=${WORKSPACE}/test/python
fi

echo "=> Unit tests..."
mkdir -p build  # needed for some tests that write into this directory
RESULTS_DIR="${WORKSPACE}/test-results/${ARCH}"
mkdir -p "$RESULTS_DIR"
if [ "$WINDOWS" -eq 1 ]; then
  workers=${CODON_TEST_WORKERS:-2}
  case "$workers" in
    ''|*[!0-9]*) echo "Invalid test worker count: $workers" >&2; exit 1 ;;
  esac
  [ "$workers" -gt 0 ] || exit 1
  pids=""
  worker=0
  while [ "$worker" -lt "$workers" ]; do
    index=$((SHARD_INDEX * workers + worker))
    GTEST_TOTAL_SHARDS=$((SHARD_COUNT * workers)) GTEST_SHARD_INDEX="$index" \
      "$BUILD_DIR/codon_test.exe" \
      --gtest_output="xml:$(cygpath -am "$RESULTS_DIR/unit-$index.xml")" \
      > "$RESULTS_DIR/unit-$index.log" 2>&1 &
    pids="$pids $!"
    worker=$((worker + 1))
  done
  failed=0
  for pid in $pids; do
    wait "$pid" || failed=1
  done
  cat "$RESULTS_DIR"/unit-*.log
  [ "$failed" -eq 0 ]
else
  GTEST_TOTAL_SHARDS="$SHARD_COUNT" GTEST_SHARD_INDEX="$SHARD_INDEX" \
    "$BUILD_DIR/codon_test" --gtest_output="xml:${RESULTS_DIR}/unit-${SHARD_INDEX}.xml"
fi

if [ "$SHARD_INDEX" -ne 0 ]; then
  exit 0
fi

echo "=> OS differential tests..."
CODON_EXECUTABLE="${CODON_DIR}/bin/codon"
if [ "$WINDOWS" -eq 1 ]; then
  CODON_EXECUTABLE="${CODON_EXECUTABLE}.exe"
fi
python test/stdlib/os_differential.py --codon "$CODON_EXECUTABLE"

echo "=> Standalone test..."
CODON_PATH=${CODON_DIR}/lib/codon/stdlib bash test/app/test.sh "$BUILD_DIR"

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
