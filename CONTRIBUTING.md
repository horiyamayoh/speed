# Contributing to Speed

Speed is currently in the v0.1 implementation-skeleton phase. Contributions
should preserve the architecture documented in `docs/`.

## Before Coding

Read the session-start documents listed in `docs/PROJECT_STATE.md`, then identify
the owning module, owning process, decision class, and tests for the change.

## Local Checks

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build --output-on-failure
```

## Change Rules

- Keep production code in the module that owns the behavior.
- Keep IPC schema-first.
- Keep Aegis in the network request path.
- Keep renderer code authority-light and storage/network-free.
- Add or update focused tests with each behavior change.
- Add an ADR for changes required by `docs/DECISION_BACKBONE.md`.
