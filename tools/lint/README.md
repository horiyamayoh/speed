# Lint

Lint helpers:

- `run-clang-tidy.sh [build-dir]`: runs `clang-tidy` on Speed-owned translation
  units from `src/` and `tests/`.
- `lint.sh`: checks C/C++ formatting and Prettier formatting. It intentionally
  does not run `clang-tidy`.
- `clang-tidy-filter.sh`: wraps `clang-tidy` and removes only the noisy summary
  lines that report suppressed non-user warnings.

The clang-tidy path is intentionally narrow: only Speed-owned source files are
passed as translation units, and diagnostics from headers are filtered to this
repository's `src/` and `tests/` trees. `third_party/`, generated IPC output,
and build directories are not lint inputs. Run it explicitly with
`npm run lint:tidy`, `tools/lint/run-clang-tidy.sh`, or the CMake `tidy` preset.
