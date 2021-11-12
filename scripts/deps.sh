#!/usr/bin/env bash
set -e

export INSTALLDIR="${PWD}/llvm"
export SRCDIR="${PWD}/llvm-project"
mkdir -p "${INSTALLDIR}" "${SRCDIR}"

export JOBS=1
if [ -n "${1}" ]; then export JOBS="${1}"; fi
echo "Using ${JOBS} cores..."

LLVM_BRANCH="release/12.x"
if [ ! -f "${INSTALLDIR}/bin/llvm-config" ]; then
  git clone --depth 1 -b "${LLVM_BRANCH}" https://github.com/llvm/llvm-project "${SRCDIR}"

  # llvm
  mkdir -p "${SRCDIR}/llvm/build"
  cd "${SRCDIR}/llvm/build"
  cmake .. \
      -DCMAKE_BUILD_TYPE=Release \
      -DLLVM_INCLUDE_TESTS=OFF \
      -DLLVM_ENABLE_RTTI=ON \
      -DLLVM_ENABLE_ZLIB=OFF \
      -DLLVM_ENABLE_TERMINFO=OFF \
      -DLLVM_TARGETS_TO_BUILD=host \
      -DCMAKE_INSTALL_PREFIX="${INSTALLDIR}"
  make -j "${JOBS}"
  make install

  # libunwind
  mkdir -p "${SRCDIR}/libunwind/build"
  cd "${SRCDIR}/libunwind/build"
  cmake .. \
      -DCMAKE_BUILD_TYPE=Release \
      -DLIBUNWIND_BUILD_32_BITS=OFF \
      -DLIBUNWIND_ENABLE_SHARED=OFF \
      -DLIBUNWIND_ENABLE_STATIC=ON \
      -DLIBUNWIND_ENABLE_CROSS_UNWINDING=ON \
      -DLIBUNWIND_ENABLE_THREADS=ON \
      -DCMAKE_INSTALL_PREFIX="${INSTALLDIR}"
  make -j "${JOBS}"
  make install
  mkdir -p "${INSTALLDIR}/include"
  cp -r ${SRCDIR}/libunwind/include/* "${INSTALLDIR}/include/"

  # clang
  if ! command -v clang &> /dev/null; then
    mkdir -p "${SRCDIR}/clang/build"
    cd "${SRCDIR}/clang/build"
    cmake .. \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX="${INSTALLDIR}"
    make -j "${JOBS}"
    make install
  fi

  "${INSTALLDIR}/bin/llvm-config" --cmakedir
fi
