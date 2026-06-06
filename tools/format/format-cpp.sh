#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$ROOT"

if ! command -v clang-format >/dev/null 2>&1; then
  echo "clang-format is required but was not found in PATH." >&2
  exit 1
fi

mapfile -d '' files < <(
  find src tests \
    -type f \
    \( -name '*.c' -o -name '*.cc' -o -name '*.cpp' -o -name '*.cxx' -o -name '*.h' -o -name '*.hh' -o -name '*.hpp' -o -name '*.hxx' \) \
    -not -path 'src/ipc/generated/*' \
    -print0
)

if ((${#files[@]} == 0)); then
  exit 0
fi

clang-format -i "${files[@]}"

