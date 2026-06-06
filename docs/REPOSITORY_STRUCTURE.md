# Speed Repository Structure

Purpose: define an OSS-friendly monorepo layout that can grow without early restructuring.

## 1. Top-level layout

```text
speed/
  CMakeLists.txt
  README.md
  LICENSE
  CONTRIBUTING.md
  CODE_OF_CONDUCT.md
  SECURITY.md
  docs/
  src/
  tests/
  tools/
  third_party/
  build/
  out/
```

`build/` and `out/` are local/generated and should usually be ignored by git.

## 2. `src/` layout

```text
src/
  app/
    speed_browser_main.cpp
    speed_renderer_main.cpp
    speed_network_main.cpp
    speed_utility_main.cpp
  base/
    logging/
    memory/
    result/
    time/
    ids/
  platform/
    process/
    files/
    window/
    event_loop/
  ipc/
    schemas/
    runtime/
    generated/
    tools/
  browser/
    tabs/
    navigation/
    permissions/
    process_host/
    profile/
    session/
  ui/
    shell/
    address_bar/
    tabs/
  renderer/
    document/
    frame/
    compositor_stub/
    render_host/
  engine/
    html/
    dom/
    css/
    style/
    layout/
    paint/
  network/
    fetch/
    http/
    tls/
    cache_stub/
  aegis/
    rules/
    classifier/
    diagnostics/
  storage/
    profile/
    history/
    session/
  utility/
    image_stub/
    font_stub/
    pdf_stub/
```

## 3. `tests/` layout

```text
tests/
  unit/
  ipc/
  aegis/
  html/
  css/
  layout/
  integration/
  fuzz/
  wpt/
  fixtures/
```

v0.1 should populate at least:

- Aegis tests.
- IPC schema/validation tests.
- HTML parser tests.
- CSS parser tests.
- Layout smoke tests.
- Process smoke test if practical.

## 4. `docs/` layout

```text
docs/
  PROJECT_STATE.md
  ARCHITECTURE_CONSTITUTION.md
  DECISION_BACKBONE.md
  MVP_SCOPE.md
  PROCESS_MODEL.md
  SECURITY_MODEL.md
  MODULE_BOUNDARIES.md
  DEPENDENCY_RULES.md
  REPOSITORY_STRUCTURE.md
  CODEX_IMPLEMENTATION_PROTOCOL.md
  ADR_TEMPLATE.md
  OPEN_QUESTIONS.md
  DEFERRED_DECISIONS.md
  FORBIDDEN_DESIGNS.md
  ROADMAP.md
  adr/
    0001-record-architecture-backbone.md
```

## 5. `tools/` layout

```text
tools/
  build/
  ipc_codegen/
  format/
  lint/
  deps/
  test_runner/
  wpt_import/
```

v0.1 may stub many tools, but paths should exist when they clarify ownership.

## 6. `third_party/` layout

```text
third_party/
  README.md
  licenses/
  patches/
```

Rules:

- Do not drop dependencies into `third_party/` without license notes.
- Prefer dependency manager or system package integration if appropriate, but keep Speed wrapper APIs stable.
- All direct usage must be documented.

## 7. Build system strong default

Initial build system: CMake.

Reasons:

- Familiar to C++ contributors.
- Works with common IDEs including VS Code.
- Supports modular targets.
- Can integrate code generation and tests.

Changing this requires an ADR.

## 8. Naming conventions

- Public Speed namespaces should start under `speed::`.
- Module namespaces may be nested, e.g. `speed::engine::dom`.
- Process-specific classes should include ownership in names where helpful, e.g. `BrowserProcess`, `RendererClient`, `NetworkService`.
- IDs should be strong types, e.g. `TabId`, `ProcessId`, `RequestId`.
- Avoid abbreviations unless standard.

## 9. Contribution friendliness

The repository should include early:

- Clear README.
- Build instructions.
- Architecture overview.
- Issue labels.
- Code style notes.
- Security reporting policy.
- Good first issue candidates.

## 10. Generated code policy

Generated IPC code must be clearly separated:

- Schemas are source of truth.
- Generated code lives under `src/ipc/generated/` or build output.
- Manual edits to generated files are forbidden.
- Codegen tool can start simple but must preserve future IDL migration.
