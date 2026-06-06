#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$ROOT"

BUILD_DIR="${1:-build}"

if ! command -v clang-tidy >/dev/null 2>&1; then
  echo "clang-tidy is required but was not found in PATH." >&2
  exit 1
fi

if [[ ! -f "$BUILD_DIR/compile_commands.json" ]]; then
  echo "Missing $BUILD_DIR/compile_commands.json; configuring CMake first." >&2
  cmake -S . -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Debug
fi

mapfile -d '' sources < <(
  find src tests \
    -type f \
    \( -name '*.c' -o -name '*.cc' -o -name '*.cpp' -o -name '*.cxx' \) \
    -not -path 'src/ipc/generated/*' \
    -print0
)

if ((${#sources[@]} == 0)); then
  exit 0
fi

jobs="${JOBS:-}"
if [[ -z "$jobs" ]]; then
  if command -v nproc >/dev/null 2>&1; then
    jobs="$(nproc)"
  else
    jobs="1"
  fi
fi

printf '%s\0' "${sources[@]}" |
  xargs -0 -n 1 -P "$jobs" tools/lint/clang-tidy-filter.sh \
    -p "$BUILD_DIR" \
    --quiet \
    --extra-arg-before=-Wno-unknown-warning-option \
    --config-file="$ROOT/.clang-tidy" \
    --header-filter="^${ROOT}/(src|tests)/"
