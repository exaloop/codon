#!/usr/bin/env bash
set -e

check_exists() {
  local file="$1"
  if [ ! -e "$file" ]; then
    echo "Error: File '$file' does not exist." >&2
    exit 1
  fi
}

UNAME="$(uname -s)"
if [ "$UNAME" = "Linux" ]; then
  LIBGFORTRAN_BASE="libgfortran.so.5"
  LIBQUADMATH_BASE="libquadmath.so.0"
  LIBGCC_BASE="libgcc_s.so.1"
elif [ "$UNAME" = "Darwin" ]; then
  LIBGFORTRAN_BASE="libgfortran.5.dylib"
  LIBQUADMATH_BASE="libquadmath.0.dylib"
  LIBGCC_BASE="libgcc_s.1.1.dylib"
else
  echo "WARNING: Could not autodetect platform type ('uname -s' = $UNAME); assuming Linux" >&2
  UNAME="Linux"
fi

LIBR_DIR=$1
DEST_DIR=$2

LIBGFORTRAN="${LIBR_DIR}/${LIBGFORTRAN_BASE}"
LIBQUADMATH="${LIBR_DIR}/${LIBQUADMATH_BASE}"
LIBGCC="${LIBR_DIR}/${LIBGCC_BASE}"

check_exists ${LIBGFORTRAN}
check_exists ${LIBQUADMATH}
check_exists ${LIBGCC}

cp -v ${LIBGFORTRAN} ${DEST_DIR}
cp -v ${LIBQUADMATH} ${DEST_DIR}
cp -v ${LIBGCC} ${DEST_DIR}

LIBGFORTRAN="${DEST_DIR}/${LIBGFORTRAN_BASE}"
LIBQUADMATH="${DEST_DIR}/${LIBQUADMATH_BASE}"
LIBGCC="${DEST_DIR}/${LIBGCC_BASE}"

chmod 755 ${LIBGFORTRAN}
chmod 755 ${LIBQUADMATH}
chmod 755 ${LIBGCC}

if [ "$UNAME" = "Darwin" ]; then
  install_name_tool -id "@rpath/${LIBGFORTRAN_BASE}" ${LIBGFORTRAN}
  install_name_tool -id "@rpath/${LIBQUADMATH_BASE}" ${LIBQUADMATH}
  install_name_tool -id "@rpath/${LIBGCC_BASE}" ${LIBGCC}
else
  patchelf --set-rpath '$ORIGIN' ${LIBGFORTRAN}
  patchelf --set-rpath '$ORIGIN' ${LIBQUADMATH}
  patchelf --set-rpath '$ORIGIN' ${LIBGCC}
fi
