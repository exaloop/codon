#!/usr/bin/env bash
set -euo pipefail

if [[ "$(uname -s)" != "Darwin" ]]; then
  echo "This script must be run on macOS (Darwin). Exiting." >&2
  exit 0
fi

DIR="${1:-codon-deploy}/lib/codon"

if [[ ! -d "$DIR" ]]; then
  echo "Directory not found: $DIR" >&2
  exit 1
fi

command -v install_name_tool >/dev/null || {
  echo "install_name_tool not found"
  exit 1
}
command -v otool >/dev/null || {
  echo "otool not found"
  exit 1
}

echo "Patching dylibs/SOs in: $DIR"
echo

has_rpath() {
  local f="$1" p="$2"
  otool -l "$f" |
    awk '/LC_RPATH/{show=1} show && /path/ {print $2; show=0}' |
    grep -qx "$p"
}

while IFS= read -r -d '' f; do
  base="$(basename "$f")"
  echo ">>> $base"

  # Preserve existing @rpath/@loader_path IDs. Convert absolute IDs
  # to @rpath so clients can locate these libraries via LC_RPATH.
  if [[ "$f" == *.dylib && ! "$base" =~ ^libcodon ]]; then
    current_id="$(otool -D "$f" | sed -n '2p')"

    if [[ -n "$current_id" &&
          "$current_id" != @rpath/* &&
          "$current_id" != @loader_path/* ]]; then
      echo "    - set id $current_id -> @rpath/$base"
      install_name_tool -id "@rpath/$base" "$f"
    fi
  fi

  # All libraries processed here live in lib/codon. @loader_path is
  # therefore sufficient to resolve sibling @rpath dependencies.
  if ! has_rpath "$f" "@loader_path"; then
    echo "    - add rpath @loader_path"
    install_name_tool -add_rpath "@loader_path" "$f"
  fi

  if command -v codesign >/dev/null; then
    codesign --force --sign - "$f" >/dev/null 2>&1 || true
  fi

  echo
done < <(
  find "$DIR" -maxdepth 1 -type f \
    \( -name '*.dylib' -o -name '*.so' \) -print0
)

echo "Done."
