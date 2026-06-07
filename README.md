# Speed

Speed is an independent C++23 browser and browser engine. The current repository
state is an implementation skeleton: process entrypoints, module boundaries,
CMake targets, and test hooks are in place so focused development can begin.

## Current Scope

The v0.1 target is static browsing with a minimal Browser Process, Renderer
Process, Network Process, schema-first IPC, basic engine pipeline, minimal GUI
shell, history, and Aegis request blocking foundation. See
`docs/PROJECT_STATE.md` and `docs/MVP_SCOPE.md` before implementing features.

## Build

System dependencies for the default dev build:

- OpenSSL development files for MVP HTTPS fetch.
- X11 and Cairo development files for `speed-browser-gui`.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build --output-on-failure
```

You can also use presets:

```sh
cmake --preset dev
cmake --build --preset dev
ctest --preset dev
```

Speed-owned targets build with strict warnings and warnings-as-errors by default.
If a local compiler is temporarily too noisy, warnings-as-errors can be disabled
without weakening the warning set:

```sh
cmake -S . -B build -DSPEED_WARNINGS_AS_ERRORS=OFF
```

Third-party code should not be wired through `speed_configure_target()`, so these
warnings stay focused on Speed-owned source.

The main development executables are:

- `build/src/app/speed-browser`: interactive console shell, or smoke URL mode
  with `build/src/app/speed-browser https://example.com/`.
- `build/src/app/speed-browser-gui`: minimal GUI shell with tabs, URL entry,
  page display, Aegis/error display, and history panel.
- `build/src/app/speed-renderer`
- `build/src/app/speed-network`
- `build/src/app/speed-utility`

For GUI smoke verification:

```sh
build/src/app/speed-browser-gui --smoke-exit-after-ms=1000
```

The default app wiring remains in-process while the v0.1 process path stabilizes.
Use `--process-model=multi-process` on `speed-browser` or `speed-browser-gui`
to exercise the opt-in Browser -> Network -> Browser -> Renderer IPC path. The
multi-process smoke tests use `about:blank` and Aegis-blocked URLs so they do
not depend on external network availability.

## Architecture Rules

Start every implementation task by reading:

1. `docs/PROJECT_STATE.md`
2. `docs/ARCHITECTURE_CONSTITUTION.md`
3. `docs/DECISION_BACKBONE.md`
4. The task-specific document listed in `docs/PROJECT_STATE.md`

Do not add renderer network access, profile writes from renderer code, ad-hoc
IPC, Aegis bypass paths, dependency cycles, or v0.1 scope expansion.

## Formatting and Static Analysis

Daily formatting/linting:

```sh
tools/format/format-cpp.sh
tools/format/check-format.sh
npm run lint
```

For Prettier-backed Markdown/JSON/YAML formatting:

```sh
npm install
npm run format
```

`clang-tidy` is intentionally not part of the daily lint command. Run it
explicitly when needed:

```sh
npm run lint:tidy
cmake --preset tidy
cmake --build --preset tidy
```

`clang-tidy` is scoped to Speed-owned `src/` and `tests/` translation units.
Generated IPC output, build directories, and `third_party/` are excluded.

## License

The project license has not been selected yet. Replace `LICENSE` before public
distribution.
