# ADR-0001: Record Speed architecture backbone v0.1

Status: Accepted  
Date: 2026-06-06  
Owners: Speed maintainers  
Decision class affected: Fixed / Strong Default / Deferred / Forbidden

## Context

Speed needs an initial decision backbone before implementation starts so Codex-driven development can proceed without contradicting core architecture constraints.

## Decision

Adopt the v0.1 architecture documents in `docs/` as the initial project decision backbone.

## Consequences

- Future implementation sessions must read `PROJECT_STATE.md` first.
- Fixed decisions require constitutional ADRs to change.
- Strong Defaults require ADRs to change.
- Deferred decisions must not be accidentally implemented as production scope.
- Forbidden designs must be rejected during implementation.
