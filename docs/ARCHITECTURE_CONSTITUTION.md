# Speed Architecture Constitution

Version: v0.1  
Purpose: Define the non-negotiable principles that keep Speed coherent as it grows.

## 1. Constitutional role

This document is the highest-level architecture guide for Speed. It does not attempt to specify every implementation detail. It defines principles, boundaries, and priorities that future implementation decisions must respect.

When another document conflicts with this constitution, the constitution wins unless an accepted ADR explicitly updates the constitution.

## 2. Design philosophy

### 2.1 Memory is fuel

Speed intentionally treats memory as a resource that may be spent to buy:

- Lower latency.
- Safer process boundaries.
- Better crash recovery.
- Smoother interaction.
- More predictable caching.
- Clearer diagnostics.

However, memory spending is not unlimited. Every cache, duplicate representation, precomputed table, retained display list, or process must eventually have:

- A clear owner.
- A reason for existence.
- A pressure/reclamation policy or a tracked deferred decision.
- Observability hooks or an explicit plan to add them.

### 2.2 Safety outranks speed

Speed should be fast, but not by violating trust boundaries. It is acceptable for a secure architecture to cost more memory or implementation work.

### 2.3 Recovery is a first-class feature

Tabs, renderer processes, network operations, and future utility tasks must be designed so that failure can be contained and recovery can be coordinated by the Browser Process.

### 2.4 Aegis is a first-class subsystem

Ad/tracker defense is not an extension bolted on later. Aegis participates in request classification, navigation policy, privacy decisions, and future cosmetic filtering.

### 2.5 Schema-first architecture

IPC and long-lived public interfaces must be explicit, reviewable, versionable, and compatible with future code generation. Implicit message formats are prohibited.

### 2.6 Independent browser engine

Speed owns its document, style, layout, paint, navigation orchestration, process model, and security model. Existing libraries may be used for dangerous or oversized domains through narrow wrappers, but Speed must not become a skin around another full browser engine.

## 3. Fixed decisions

The following are Fixed.

| ID    | Decision                                                                              | Reason                                                 |
| ----- | ------------------------------------------------------------------------------------- | ------------------------------------------------------ |
| F-001 | C++23 is the primary implementation language.                                         | Enables modern C++ while keeping system-level control. |
| F-002 | Renderer processes are untrusted.                                                     | Web content is adversarial input.                      |
| F-003 | Only the Network Process performs external network communication.                     | Centralizes policy, logging, Aegis, and isolation.     |
| F-004 | Browser Process owns permissions, process lifecycle, profile authority, and recovery. | Avoids authority spread across content processes.      |
| F-005 | IPC is schema-first and future-IDL-compatible.                                        | Prevents protocol drift and unsafe ad-hoc messaging.   |
| F-006 | Aegis is part of the core architecture.                                               | Blocking/privacy cannot be an afterthought.            |
| F-007 | Dangerous parsers and complex domains must be separable into Utility Processes.       | Enables future sandboxing and crash containment.       |
| F-008 | Tests must be architecturally planned from the start.                                 | Avoids impossible-to-test engine internals.            |
| F-009 | v0.1 does not require full web compatibility.                                         | Protects the MVP from becoming unbounded.              |
| F-010 | The repository is a modular OSS monorepo.                                             | Supports long-term growth and contributions.           |

## 4. Strong defaults

Strong Defaults may be changed only by an accepted ADR.

| ID     | Default                                                              | Reason                                                       |
| ------ | -------------------------------------------------------------------- | ------------------------------------------------------------ |
| SD-001 | Start with one Renderer Process per tab or browsing context.         | Simple MVP isolation while preserving future site isolation. |
| SD-002 | Use CPU-backed painting for v0.1.                                    | Reduces early GPU complexity.                                |
| SD-003 | Use wrapper interfaces around third-party libraries.                 | Keeps dependencies replaceable.                              |
| SD-004 | Prefer explicit data structures over clever generic frameworks.      | Improves reviewability and contributor onboarding.           |
| SD-005 | Keep the Browser Process policy-heavy and content-light.             | Avoids content parsing in trusted authority.                 |
| SD-006 | Keep Renderer content-heavy but authority-light.                     | Allows complex rendering without trust.                      |
| SD-007 | Place Aegis request decision hooks before external network dispatch. | Ensures no bypass path.                                      |
| SD-008 | Build minimal observability with each subsystem.                     | Debugging multi-process code requires traces.                |
| SD-009 | Use simple durable storage before optimizing databases.              | Keeps v0.1 understandable.                                   |
| SD-010 | Prefer small milestones and issue-driven implementation.             | Reduces architecture drift in Codex sessions.                |

## 5. Deferred areas

Deferred decisions are intentionally not fixed in v0.1. Implement only placeholders or interfaces when needed.

- JavaScript engine strategy.
- WebAssembly support.
- GPU compositor.
- Advanced text shaping and font fallback policy.
- Media pipeline.
- Extension API.
- DevTools protocol.
- Service Workers and storage quota.
- Accessibility architecture.
- Browser sync.
- Password manager.
- Full cookie policy.
- Full site isolation policy.
- Web compatibility level and standards target.

## 6. Forbidden architecture moves

The following are prohibited:

- Renderer directly opens sockets or performs external network I/O.
- Renderer directly accesses profile storage.
- Renderer becomes the source of truth for permissions or navigation authority.
- Network requests bypass Aegis.
- IPC messages are added only as raw strings, unowned structs, or one-off serialization code.
- Browser, Renderer, Network, and Utility logic are merged into a privilege-equivalent monolith.
- A module imports an upper layer to avoid designing an interface.
- A feature enters MVP merely because it is interesting or impressive.
- Security-sensitive code is hidden behind undocumented convenience helpers.
- Third-party dependencies are scattered directly through engine internals.

## 7. Conflict resolution

When two design goals conflict, resolve them in this priority order:

1. Security.
2. Process and privilege boundaries.
3. Recovery and data integrity.
4. Constitution and Forbidden Designs.
5. MVP scope.
6. Dependency direction.
7. Testability and observability.
8. Performance.
9. Memory-as-fuel optimizations.
10. Developer convenience.

Examples:

- If direct renderer network access would be faster, reject it because security and process boundaries outrank performance.
- If adding a cache improves speed but creates unbounded memory use, require an owner and reclamation policy before accepting it.
- If a shortcut creates a dependency cycle, reject it even if it speeds up MVP implementation.

## 8. Change control

A constitutional change requires:

1. A new ADR using `ADR_TEMPLATE.md`.
2. A clear statement of which constitutional section changes.
3. Rationale explaining why existing constraints are insufficient.
4. Security and process-boundary impact analysis.
5. Migration plan for existing code and docs.
6. Explicit update to this file after acceptance.

## 9. Practical implementation rule

When implementing a feature, ask:

1. Which process owns authority?
2. Which process owns data?
3. Which module owns logic?
4. Does this cross a trust boundary?
5. Is IPC schema-first?
6. Does Aegis need to see this request or classification?
7. Is this in v0.1 scope?
8. Does this introduce a new dependency direction?
9. Does this require an ADR?
10. What test proves the boundary is preserved?
