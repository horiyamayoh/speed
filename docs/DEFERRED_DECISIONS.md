# Speed Deferred Decisions

Purpose: record decisions that are intentionally postponed so v0.1 remains focused.

## 1. Deferred does not mean ignored

Deferred means:

- Do not implement the full feature now.
- Do not design APIs that make the future feature impossible.
- Add placeholders only when needed.
- Document assumptions.
- Revisit through ADR when the trigger occurs.

## 2. Deferred decision table

| ID      | Decision                       | Why deferred                      | v0.1 action                                       | Trigger to decide                                            |
| ------- | ------------------------------ | --------------------------------- | ------------------------------------------------- | ------------------------------------------------------------ |
| DEF-001 | JavaScript engine strategy     | Huge scope and security impact.   | No production JS. Keep parser/layout independent. | Static rendering pipeline is stable.                         |
| DEF-002 | WebAssembly                    | Depends on JS/runtime sandbox.    | None.                                             | JS runtime ADR accepted.                                     |
| DEF-003 | GPU compositor                 | Complex platform/security work.   | CPU paint abstraction only.                       | CPU paint becomes bottleneck or advanced compositing begins. |
| DEF-004 | Image decoding library/process | Dangerous parsing.                | Placeholder image boxes or adapter stub.          | Real image support milestone.                                |
| DEF-005 | Font parsing/text shaping      | Complex and security-sensitive.   | Text adapter boundary.                            | Non-trivial text support milestone.                          |
| DEF-006 | PDF support                    | Huge attack surface.              | None.                                             | PDF viewer milestone.                                        |
| DEF-007 | Media pipeline                 | Huge dependency/security surface. | None.                                             | Media milestone.                                             |
| DEF-008 | Extension API                  | Large permissions/API surface.    | None.                                             | Extension architecture phase.                                |
| DEF-009 | DevTools protocol              | Large API surface.                | Logging/diagnostics only.                         | Engine debugging becomes blocker.                            |
| DEF-010 | Service Workers                | Requires storage/cache/runtime.   | None.                                             | JS/storage architecture exists.                              |
| DEF-011 | Full cookie model              | Privacy/security complexity.      | Minimal placeholder if necessary.                 | Authenticated browsing phase.                                |
| DEF-012 | HTTP cache                     | Storage + privacy concerns.       | Optional memory-only cache experiment.            | Performance/network milestone.                               |
| DEF-013 | Site/origin isolation policy   | Requires navigation/origin model. | Per-tab renderer default.                         | Cross-origin frame support.                                  |
| DEF-014 | Accessibility tree             | Platform-specific and broad.      | Avoid blocking future tree.                       | UI maturity milestone.                                       |
| DEF-015 | Browser sync                   | Backend/security scope.           | None.                                             | Profile feature phase.                                       |
| DEF-016 | Password manager               | High security requirements.       | None.                                             | Security-focused feature phase.                              |
| DEF-017 | Full Aegis cosmetic filtering  | Renderer integration needed.      | URL blocking only.                                | Aegis v0.2/v0.3.                                             |
| DEF-018 | Aegis subscription updates     | Network/update trust path.        | Local rules only.                                 | User-facing Aegis settings.                                  |
| DEF-019 | Full storage quota model       | Needed for web storage APIs.      | None.                                             | Storage APIs phase.                                          |
| DEF-020 | Internationalization strategy  | Broad platform impact.            | Basic UTF-8 assumptions where safe.               | Text/UI maturity phase.                                      |
| DEF-021 | Full URL/origin model          | Security critical.                | Minimal URL validation wrapper.                   | Cookies, JS, cross-origin features.                          |
| DEF-022 | Certificate UI policy          | Network/security UX.              | Use library result, show basic error.             | Public browsing milestone.                                   |
| DEF-023 | Crash reporting                | Privacy and infra.                | Local logs only.                                  | Public beta phase.                                           |
| DEF-024 | Plugin architecture            | Security risk.                    | None.                                             | Probably avoid unless ADR.                                   |
| DEF-025 | Web compatibility target       | Determines long-term scope.       | MVP static subset.                                | Post-MVP planning.                                           |

## 3. Placeholder rules

Allowed placeholders:

- Empty interface with no false guarantees.
- Stub process entrypoint.
- Feature flag default off.
- TODO referencing this file.
- Test fixture showing unsupported behavior fails safely.

Forbidden placeholders:

- Fake security that appears real.
- APIs that imply stable behavior but are not implemented.
- Stub code that bypasses process boundaries.
- Temporary direct dependencies that will be hard to remove.

## 4. Revisit process

To resolve a deferred decision:

1. Open or select a milestone.
2. Write ADR using `ADR_TEMPLATE.md`.
3. Update `DECISION_BACKBONE.md` classification.
4. Update affected boundary docs.
5. Add tests before relying on behavior.
