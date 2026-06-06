#!/usr/bin/env bash
set -euo pipefail

clang_tidy_bin="${CLANG_TIDY:-clang-tidy}"

if [[ "${1:-}" == "--clang-tidy-binary" ]]; then
  clang_tidy_bin="$2"
  shift 2
fi

"$clang_tidy_bin" "$@" 2>&1 |
  sed -E \
    -e '/^[[:space:]]*[0-9]+ warnings generated\.$/d' \
    -e '/^Suppressed [0-9]+ warnings \([0-9]+ in non-user code\)\.$/d' \
    -e '/^Use -header-filter=.*$/d'
