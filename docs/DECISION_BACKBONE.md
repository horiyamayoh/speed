# Speed Architecture Decision Backbone v0.1

Purpose: classify architectural decisions so future implementation can move quickly without contradicting the long-term design.

## 1. Decision classes

| Class          | Meaning                                                         | Change process                                                  |
| -------------- | --------------------------------------------------------------- | --------------------------------------------------------------- |
| Fixed          | Core decision that should not change during normal development. | Requires constitutional ADR and explicit maintainer acceptance. |
| Strong Default | Default architecture direction.                                 | May change with a normal ADR and strong evidence.               |
| Deferred       | Intentionally not decided yet.                                  | Do not overbuild. Add placeholders only when needed.            |
| Experimental   | Safe to prototype behind boundaries or flags.                   | Must not leak into public architecture without ADR.             |
| Forbidden      | Must not be done.                                               | Reject implementation or redesign.                              |

## 2. Decision classification table

| ID    | Decision                                                   | Class              | Owning doc                         | Notes / trigger                                   |
| ----- | ---------------------------------------------------------- | ------------------ | ---------------------------------- | ------------------------------------------------- |
| D-001 | C++23 primary language                                     | Fixed              | `ARCHITECTURE_CONSTITUTION.md`     | Only wrappers/tools may use other languages.      |
| D-002 | Independent DOM/CSS/layout/paint engine                    | Fixed              | `MODULE_BOUNDARIES.md`             | Do not embed Chromium/WebKit/Gecko as the engine. |
| D-003 | Renderer is untrusted                                      | Fixed              | `SECURITY_MODEL.md`                | Treat all renderer output as attacker-controlled. |
| D-004 | Network Process is sole external network authority         | Fixed              | `PROCESS_MODEL.md`                 | No renderer sockets.                              |
| D-005 | Browser Process owns permissions/profile/process lifecycle | Fixed              | `PROCESS_MODEL.md`                 | Authority centralization.                         |
| D-006 | IPC schema-first                                           | Fixed              | `PROCESS_MODEL.md`                 | Future IDL generation required.                   |
| D-007 | Aegis is first-class                                       | Fixed              | `MODULE_BOUNDARIES.md`             | Network path must include Aegis hook.             |
| D-008 | Utility Process separability for dangerous domains         | Fixed              | `PROCESS_MODEL.md`                 | May be stubbed in v0.1.                           |
| D-009 | Repository is modular monorepo                             | Fixed              | `REPOSITORY_STRUCTURE.md`          | Chrome-scale growth target.                       |
| D-010 | Tests planned from first commit                            | Fixed              | `CODEX_IMPLEMENTATION_PROTOCOL.md` | Unit test skeleton required.                      |
| D-011 | One renderer per tab for v0.1                              | Strong Default     | `PROCESS_MODEL.md`                 | Future site isolation possible.                   |
| D-012 | CPU-backed painting for v0.1                               | Strong Default     | `MVP_SCOPE.md`                     | GPU compositor deferred.                          |
| D-013 | Simple platform abstraction                                | Strong Default     | `REPOSITORY_STRUCTURE.md`          | Avoid binding core engine to UI toolkit.          |
| D-014 | Third-party deps behind wrappers                           | Strong Default     | `DEPENDENCY_RULES.md`              | Direct use only inside adapter modules.           |
| D-015 | Minimal storage format first                               | Strong Default     | `MVP_SCOPE.md`                     | DB choice may change later.                       |
| D-016 | Aegis v0.1 URL/blocklist only                              | Strong Default     | `MVP_SCOPE.md`                     | Cosmetic filtering deferred.                      |
| D-017 | HTML parser starts as limited static parser                | Strong Default     | `MVP_SCOPE.md`                     | Full spec parser deferred.                        |
| D-018 | CSS subset first                                           | Strong Default     | `MVP_SCOPE.md`                     | Expand by tests.                                  |
| D-019 | Avoid template-heavy infrastructure                        | Strong Default     | `CODEX_IMPLEMENTATION_PROTOCOL.md` | Readability over cleverness.                      |
| D-020 | Build system choice                                        | Strong Default     | `REPOSITORY_STRUCTURE.md`          | CMake recommended unless ADR changes.             |
| D-021 | JavaScript engine                                          | Deferred           | `DEFERRED_DECISIONS.md`            | Choose after static engine works.                 |
| D-022 | WebAssembly                                                | Deferred           | `DEFERRED_DECISIONS.md`            | Requires JS/runtime decision.                     |
| D-023 | GPU compositor                                             | Deferred           | `DEFERRED_DECISIONS.md`            | CPU paint first.                                  |
| D-024 | Advanced font/text shaping                                 | Deferred           | `DEFERRED_DECISIONS.md`            | Wrapper required.                                 |
| D-025 | Image decoder strategy                                     | Deferred           | `DEFERRED_DECISIONS.md`            | Wrapper and future Utility Process required.      |
| D-026 | Full cookie/privacy model                                  | Deferred           | `OPEN_QUESTIONS.md`                | Basic policy only if required by MVP.             |
| D-027 | Extension API                                              | Deferred           | `DEFERRED_DECISIONS.md`            | Not v0.1.                                         |
| D-028 | DevTools protocol                                          | Deferred           | `DEFERRED_DECISIONS.md`            | Minimal logging only.                             |
| D-029 | Service Workers                                            | Deferred           | `DEFERRED_DECISIONS.md`            | Not v0.1.                                         |
| D-030 | Accessibility architecture                                 | Deferred           | `DEFERRED_DECISIONS.md`            | Do not block future support.                      |
| D-031 | Layout algorithm experiments                               | Experimental       | `MVP_SCOPE.md`                     | Allowed inside engine boundary.                   |
| D-032 | Cache strategies                                           | Experimental       | `MVP_SCOPE.md`                     | Must be observable/reclaimable.                   |
| D-033 | Parser recovery heuristics                                 | Experimental       | `MVP_SCOPE.md`                     | Tests must capture behavior.                      |
| D-034 | Aegis rule indexing data structures                        | Experimental       | `MODULE_BOUNDARIES.md`             | Keep API stable.                                  |
| D-035 | UI toolkit exploration                                     | Experimental       | `REPOSITORY_STRUCTURE.md`          | Must not leak into engine core.                   |
| D-036 | Renderer network access                                    | Forbidden          | `FORBIDDEN_DESIGNS.md`             | Reject.                                           |
| D-037 | Ad-hoc IPC                                                 | Forbidden          | `FORBIDDEN_DESIGNS.md`             | Reject.                                           |
| D-038 | Aegis bypass path                                          | Forbidden          | `FORBIDDEN_DESIGNS.md`             | Reject.                                           |
| D-039 | Profile writes from renderer                               | Forbidden          | `FORBIDDEN_DESIGNS.md`             | Reject.                                           |
| D-040 | Dependency cycles                                          | Forbidden          | `DEPENDENCY_RULES.md`              | Reject or introduce interface.                    |
| D-041 | Monolithic privilege-equivalent browser                    | Forbidden          | `FORBIDDEN_DESIGNS.md`             | Reject.                                           |
| D-042 | Full JS in v0.1                                            | Forbidden for v0.1 | `MVP_SCOPE.md`                     | Later phase only.                                 |
| D-043 | Full video/media in v0.1                                   | Forbidden for v0.1 | `MVP_SCOPE.md`                     | Later phase only.                                 |
| D-044 | Embedding another full browser engine                      | Forbidden          | `FORBIDDEN_DESIGNS.md`             | Violates independence.                            |
| D-045 | Unbounded invisible caches                                 | Forbidden          | `FORBIDDEN_DESIGNS.md`             | Memory spending must be visible.                  |

## 3. Fixed decisions list

- C++23 is the base language.
- Speed owns its browser engine layers.
- Renderer is never trusted.
- Network Process is the only external communication path.
- Browser Process is the policy authority.
- IPC is schema-first.
- Aegis is core.
- Dangerous parsing must be separable.
- Repository structure must support large OSS growth.
- Testability is not optional.

## 4. Strong defaults list

- One Renderer Process per tab/browsing context for v0.1.
- CPU painting for v0.1.
- CMake as the initial build system.
- Third-party libraries only through wrappers.
- Minimal blocklist-based Aegis first.
- Minimal static HTML/CSS support first.
- Simple readable C++ over framework cleverness.
- Small issue-driven milestones.

## 5. Deferred decision list

- JavaScript engine strategy.
- WebAssembly.
- GPU compositing.
- Advanced typography.
- Image decoding architecture details.
- Media.
- Extensions.
- DevTools.
- Service Workers.
- Browser sync.
- Full privacy/cookie policy.
- Accessibility.

## 6. Forbidden decision list

- Renderer network access.
- Renderer profile/storage authority.
- Raw ad-hoc IPC.
- Bypassing Aegis.
- Embedding another full browser engine.
- Long-term monolithic architecture.
- Cyclic dependencies.
- Invisible unbounded caches.
- Scope creep into JS/media/extensions during v0.1.

## 7. ADR trigger matrix

| Situation                                                   | ADR required?                                  |
| ----------------------------------------------------------- | ---------------------------------------------- |
| Implementing inside existing module boundary                | Usually no                                     |
| Adding a new class/function inside an established subsystem | Usually no                                     |
| Changing Fixed decision                                     | Yes, constitutional ADR                        |
| Changing Strong Default                                     | Yes                                            |
| Reclassifying Deferred to active design                     | Yes                                            |
| Adding third-party library                                  | Yes unless already covered by wrapper category |
| Adding process type                                         | Yes                                            |
| Adding IPC category                                         | Yes                                            |
| Expanding MVP scope                                         | Yes                                            |
| Introducing dependency reversal                             | Yes                                            |
| Experimental prototype behind flag                          | No, unless it creates public architecture      |

## 8. Decision workflow

1. Identify the feature or problem.
2. Identify owning module and process.
3. Check `MVP_SCOPE.md`.
4. Check `FORBIDDEN_DESIGNS.md`.
5. Classify the decision.
6. If Fixed/Strong Default is affected, write ADR before implementation.
7. If Deferred, implement only the minimum placeholder needed.
8. If Experimental, isolate behind a flag or internal interface.
9. Add tests that prove the relevant boundary.
10. Update `PROJECT_STATE.md` if project status or next steps change.
