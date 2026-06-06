# Speed Forbidden Design Choices

Purpose: list architecture moves that must be rejected during implementation.

## 1. Security boundary violations

Forbidden:

- Renderer opens external network sockets.
- Renderer directly fetches URLs through platform libraries.
- Renderer directly reads or writes profile files.
- Renderer writes history.
- Renderer grants permissions.
- Renderer spawns privileged processes.
- Utility Process gains broad profile or network authority by default.

Reason: web content is adversarial. Authority must remain brokered.

## 2. Aegis bypasses

Forbidden:

- Adding any external request path that does not call Aegis first.
- Treating Aegis as a UI-only feature.
- Letting renderer decide final block/allow for network requests.
- Silently retrying blocked requests through alternate URL loaders.

Reason: Aegis is core enforcement, not decoration.

## 3. IPC anti-patterns

Forbidden:

- Raw string message names without schema.
- One-off serialization logic hidden inside feature modules.
- IPC handlers that trust renderer-provided IDs without validation.
- IPC messages that grant authority implicitly.
- Generated code edited manually.

Reason: IPC is the bloodstream of the process architecture.

## 4. Dependency and module anti-patterns

Forbidden:

- Dependency cycles between production modules.
- `engine` depending on `browser`, `network`, or `storage`.
- `aegis` depending on `network` or `ui`.
- Third-party types leaking through broad public APIs without ADR.
- Convenience includes that import a large module into a lower layer.

Reason: early cycles become permanent architecture debt.

## 5. MVP scope violations

Forbidden for v0.1 production scope:

- Full JavaScript engine.
- WebAssembly.
- Full media pipeline.
- Full extension system.
- Full DevTools.
- Service Workers.
- Full GPU compositor.
- PDF viewer.
- Password manager.
- Browser sync.

Reason: v0.1 is an architecture foundation, not a full browser.

## 6. Browser-engine independence violations

Forbidden:

- Embedding Chromium, WebKit, Gecko, or another full browser engine as Speed's renderer.
- Delegating DOM/CSS/layout/paint wholesale to another browser engine.
- Building only a shell around an existing browser engine while calling it independent.

Allowed:

- Libraries for TLS, certificate validation, image decoding, font shaping, compression, or platform windows through wrappers.

## 7. Memory misuse

Forbidden:

- Unbounded caches with no owner.
- Hidden global caches that cannot be inspected or cleared.
- Retaining duplicate document representations without rationale.
- Using memory to avoid defining process or ownership boundaries.

Reason: memory is fuel, not an excuse for leaks or invisible state.

## 8. Error-handling anti-patterns

Forbidden:

- Crashing on malformed web input.
- Ignoring failed IPC decode.
- Treating network failure as impossible.
- Continuing with partially initialized authority objects.
- Swallowing security decisions without diagnostics.

## 9. How to respond when a forbidden design seems useful

1. Stop implementation.
2. Write down why it seemed useful.
3. Identify the actual goal.
4. Find a design that preserves the boundary.
5. If impossible, write an ADR explaining the conflict; do not implement first.
