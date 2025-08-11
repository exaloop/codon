#!/usr/bin/env bash
set -euo pipefail

# Exit unless on macOS
if [[ "$(uname -s)" != "Darwin" ]]; then
  echo "This script must be run on macOS (Darwin). Exiting." >&2
  exit 0
fi

DIR="${1:-codon-deploy}/lib/codon"

if [[ ! -d "$DIR" ]]; then
  echo "Directory not found: $DIR" >&2
  exit 1
fi

command -v install_name_tool >/dev/null || { echo "install_name_tool not found"; exit 1; }
command -v otool             >/dev/null || { echo "otool not found"; exit 1; }

echo "Patching dylibs/SOs in: $DIR"
echo

# Helper: does file F already have RPATH P?
has_rpath() {
  local f="$1" p="$2"
  otool -l "$f" | awk '/LC_RPATH/{show=1} show && /path/ {print $2; show=0}' | grep -qx "$p"
}

# Iterate all candidates in DIR
while IFS= read -r -d '' f; do
  base="$(basename "$f")"
  echo ">>> $base"

  # 1) Set install name (id) to @loader_path/<name> when it's a dylib
  if [[ "$f" == *.dylib ]]; then
    echo "    - set id -> @loader_path/$base"
    install_name_tool -id "@loader_path/$base" "$f"
  fi

  # 2) For each dependency that is @rpath/<name> and exists locally, rewrite to @loader_path/<name>
  #    (Skip system/Framework/other absolute deps.)
  while IFS= read -r dep; do
    dep_name="$(basename "$dep")"
    if [[ "$dep" == @rpath/* && -e "$DIR/$dep_name" ]]; then
      echo "    - change dep $dep -> @loader_path/$dep_name"
      install_name_tool -change "$dep" "@loader_path/$dep_name" "$f"
    fi
  done < <(otool -L "$f" | tail -n +2 | awk '{print $1}')

  # 3) Ensure LC_RPATH contains @loader_path and @loader_path/../lib/codon
  for rp in "@loader_path" "@loader_path/../lib/codon"; do
    if ! has_rpath "$f" "$rp"; then
      echo "    - add rpath $rp"
      install_name_tool -add_rpath "$rp" "$f" || true
    fi
  done

  # 4) Ad-hoc sign to keep the loader happy after modifications
  if command -v codesign >/dev/null; then
    codesign --force --sign - "$f" >/dev/null 2>&1 || true
  fi

  echo
done < <(find "$DIR" -maxdepth 1 -type f \( -name '*.dylib' -o -name '*.so' \) -print0)

echo "Done."
