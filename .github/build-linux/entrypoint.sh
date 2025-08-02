#!/bin/sh -l
set -e
set -x

WORKSPACE="${1:-/github/workspace}"

export ARCHDEFAULT="$(uname -s | tr '[:upper:]' '[:lower:]')-$(uname -m)"
ARCH=${2:-$ARCHDEFAULT}

TEST=${3:-no}

echo "Workspace: ${WORKSPACE}; arch: ${ARCH}"
cd "$WORKSPACE"

COMPILER_PREFIX="/opt/llvm-codon/bin/"
if [ "$(uname -s)" = "Linux" ]; then
  ~/.pyenv/bin/pyenv global 3.11
  export PATH="/root/.pyenv/shims:${PATH}"
  export CODON_SYSTEM_LIBRARIES=/usr/lib64
  python --version
else
  export CODON_SYSTEM_LIBRARIES=$(brew --prefix gcc)/lib/gcc/current
  # Clang 20 crashes with Highway on macOS so use system compiler for now...
  COMPILER_PREFIX=""
  python --version
fi
export CODON_PYTHON=$(python ${WORKSPACE}/test/python/find-python-library.py)
export CODON_DIR=$(pwd)/codon-deploy-${ARCH}

python -m pip install --upgrade pip setuptools wheel
python -m pip install cython wheel astunparse
python -m pip install --force-reinstall -v "numpy==2.0.2"

# Build Codon
CODON_EXTRA=""
if command -v nvcc &> /dev/null; then
  # Enable GPU support if CUDA is detected
  nvcc_version=$(nvcc --version | grep "release" | awk '{print $NF}')
  echo "CUDA Version: $nvcc_version"
  CODON_EXTRA="-DCODON_GPU=ON"
fi
cmake -S . -B build-${ARCH} \
    -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_C_COMPILER=${COMPILER_PREFIX}clang \
    -DCMAKE_CXX_COMPILER=${COMPILER_PREFIX}clang++ \
    -DLLVM_DIR=/opt/llvm-codon/lib/cmake/llvm ${CODON_EXTRA}
cmake --build build-${ARCH}
cmake --install build-${ARCH} --prefix=${CODON_DIR}

if [ "$(uname -s)" = "Darwin" ]; then
  # macOS needs codesigned libraries
  codesign -f -s - ${CODON_DIR}/bin/codon ${CODON_DIR}/lib/codon/*.dylib
fi

# Build codon-jit
(cd ${CODON_DIR}/python && python setup.py sdist)
python -m pip install -v ${CODON_DIR}/python/dist/*.gz

# Test
if [ "$TEST" = "yes" ]; then
  ${WORKSPACE}/.github/build-linux/test.sh "${WORKSPACE}" "${ARCH}"
fi

# Package
export CODON_BUILD_ARCHIVE=codon-${ARCH}.tar.gz
cp -rf ${CODON_DIR}/python/dist .
ls -lah dist/
# Clean-up useless install artefacts
rm -rf ${CODON_DIR}/lib/libfmt.a ${CODON_DIR}/lib/pkgconfig ${CODON_DIR}/lib/cmake \
       ${CODON_DIR}/python/codon.egg-info ${CODON_DIR}/python/dist ${CODON_DIR}/python/build
tar czf ${CODON_BUILD_ARCHIVE} -C ${WORKSPACE} codon-deploy-${ARCH}/
du -sh ${CODON_BUILD_ARCHIVE}
