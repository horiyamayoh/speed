# ADR-NNNN: Title

Status: Proposed | Accepted | Rejected | Superseded  
Date: YYYY-MM-DD  
Owners:  
Related issues:  
Supersedes:  
Superseded by:  
Decision class affected: Fixed | Strong Default | Deferred | Experimental | Forbidden

## 1. Context

Describe the problem, current architecture, and why a decision is needed now.

Include:

- Which module/process is affected.
- Which existing docs are relevant.
- Whether v0.1 scope is affected.
- Whether security or process boundaries are affected.

## 2. Decision

State the decision clearly.

Example:

> Speed will use X for Y behind interface Z. Module A may depend on interface Z, but not on X directly.

## 3. Classification

- Current classification:
- New classification if changed:
- Reason classification is appropriate:

## 4. Rationale

Explain why this option is preferred.

Address:

- Security.
- Process boundaries.
- Recovery.
- Performance.
- Memory usage.
- Testability.
- OSS maintainability.

## 5. Alternatives considered

### Alternative A

Pros:

Cons:

Reason rejected:

### Alternative B

Pros:

Cons:

Reason rejected:

## 6. Consequences

Positive consequences:

-

Negative consequences:

-

Neutral tradeoffs:

-

## 7. Migration plan

Steps:

1.
2.
3.

Rollback plan:

-

## 8. Security and privacy impact

- Does this increase process privilege?
- Does this add a new IPC path?
- Does this touch untrusted input?
- Does this affect Aegis?
- Does this affect profile data?
- What validation is required?

## 9. Testing plan

Required tests:

- Unit:
- Integration:
- IPC:
- Fuzz/layout/WPT if applicable:

## 10. Documentation updates

Update these files:

- [ ] `PROJECT_STATE.md`
- [ ] `ARCHITECTURE_CONSTITUTION.md`
- [ ] `DECISION_BACKBONE.md`
- [ ] `MVP_SCOPE.md`
- [ ] `PROCESS_MODEL.md`
- [ ] `SECURITY_MODEL.md`
- [ ] `MODULE_BOUNDARIES.md`
- [ ] `DEPENDENCY_RULES.md`
- [ ] `DEFERRED_DECISIONS.md`
- [ ] `FORBIDDEN_DESIGNS.md`
- [ ] `ROADMAP.md`

## 11. Acceptance criteria

This ADR is accepted when:

- [ ] Decision is explicit.
- [ ] Owner is identified.
- [ ] Security/process impact is reviewed.
- [ ] Tests are defined.
- [ ] Docs to update are listed.
- [ ] Migration plan exists.
