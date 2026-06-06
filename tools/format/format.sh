#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$ROOT"

tools/format/format-cpp.sh

if ! command -v prettier >/dev/null 2>&1; then
  echo "prettier is required for full formatting. Run npm install first." >&2
  exit 1
fi

prettier --write .

