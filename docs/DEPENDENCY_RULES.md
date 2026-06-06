# Speed Dependency Direction Rules

Purpose: prevent architectural erosion as the codebase grows.

## 1. Core dependency principle

Dependencies must flow from high-level orchestration toward lower-level services or from process-specific code toward shared abstractions. Lower-level modules must not import higher-level policy modules.

No dependency cycle is acceptable as a shortcut.

## 2. Layer order

Recommended conceptual order from lowest to highest:

1. `base`
2. `platform`
3. `ipc`
4. `aegis` core
5. `engine/*` core
6. `storage`
7. `network`
8. `renderer`
9. `browser`
10. `ui`
11. `app`

This is not a license for arbitrary upward access. It is a guardrail for reviewing includes and target links.

## 3. Allowed dependency examples

- `browser -> storage`
- `browser -> ipc`
- `browser -> platform`
- `browser -> aegis` for UI/debug policy state
- `renderer -> engine`
- `renderer -> ipc`
- `network -> aegis`
- `network -> ipc`
- `engine/layout -> engine/style`
- `engine/style -> engine/css`
- `engine/html -> engine/dom`
- `tests -> any module under test`

## 4. Forbidden dependency examples

- `engine -> browser`
- `engine -> network`
- `engine -> storage`
- `aegis -> network`
- `aegis -> ui`
- `storage -> renderer`
- `network -> ui`
- `renderer -> storage`
- `renderer -> network transport implementation`
- `base -> any Speed product module`

## 5. Third-party dependency rules

Third-party dependency use must follow these rules:

- Prefer narrow adapter modules.
- Do not include third-party headers throughout engine internals.
- Do not let third-party types become public Speed APIs unless an ADR accepts it.
- Dangerous decoders/parsers must be Utility Process-ready.
- Every dependency must have license notes.
- Every security-sensitive dependency must have update ownership.

Allowed dependency classes without changing the constitution:

- TLS/certificate validation libraries through `network` adapter.
- Image decoding libraries through future image adapter/Utility Process boundary.
- Font/text shaping libraries through font/text adapter.
- Compression libraries through adapter boundary.
- Platform windowing libraries through `platform` adapter.

Still ADR-required:

- Adding a new concrete third-party dependency.
- Exposing third-party types across module boundaries.
- Depending on another full browser engine.

## 6. IPC dependency rules

- Process modules depend on `ipc` schema/bindings.
- IPC must not depend on process business logic.
- Message handlers live in owning process modules.
- Schema definitions live in a central schema directory.
- Generated code must be treated as build output or generated source with clear ownership.

## 7. Test dependency rules

Tests may depend on modules under test, but production code must not depend on tests.

Recommended test directories:

- `tests/unit/`
- `tests/ipc/`
- `tests/aegis/`
- `tests/html/`
- `tests/css/`
- `tests/layout/`
- `tests/integration/`
- `tests/fuzz/`
- `tests/wpt/`

## 8. Enforcement strategy

v0.1 should include simple enforcement first:

- Build targets per module.
- Include path discipline.
- Review checklist.
- Optional dependency graph script later.

Future enforcement:

- CI check for forbidden includes.
- Build graph validation.
- Generated dependency reports.

## 9. Review checklist

For every PR or Codex-generated change:

- Does the new file live in the right module?
- Does it include a higher-level module?
- Does it create a cycle?
- Does it expose third-party types?
- Does it bypass IPC boundaries?
- Does it bypass Aegis?
- Does it add storage access from an untrusted process?
- Does it require an ADR?
