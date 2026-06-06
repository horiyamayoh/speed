# Speed Codex Implementation Protocol

Purpose: give future VS Code Codex sessions a repeatable way to implement Speed without losing architectural coherence.

## 1. Session start protocol

Every Codex implementation session must start by reading:

1. `docs/PROJECT_STATE.md`
2. `docs/ARCHITECTURE_CONSTITUTION.md`
3. `docs/DECISION_BACKBONE.md`
4. The task-relevant document from the table in `PROJECT_STATE.md`

Then Codex must identify:

- The current task.
- Owning module.
- Owning process, if any.
- Decision class: Fixed, Strong Default, Deferred, Experimental, or Forbidden.
- Whether ADR is required.
- Tests to add or update.

## 2. Implementation decision rules

### Fixed

Do not change during implementation. If a task appears to require changing a Fixed decision, stop and create an ADR proposal instead of coding the change.

### Strong Default

Follow it by default. If there is a strong reason to change it, write an ADR before implementation.

### Deferred

Do not solve now. Add only:

- A narrow interface.
- A stub.
- A TODO linked to `DEFERRED_DECISIONS.md`.
- Tests proving current behavior does not accidentally depend on a future choice.

### Experimental

Allowed only when:

- It is isolated.
- It is behind a flag, separate target, or internal interface.
- It does not alter public architecture.
- It can be removed without affecting MVP.

### Forbidden

Do not implement. Redesign the task.

## 3. Per-change checklist

Before editing:

- Which module owns this?
- Which process owns this?
- Is it in v0.1 scope?
- Does it cross IPC?
- Does it cross a trust boundary?
- Does Aegis need to see it?
- Does it touch profile storage?
- Does it introduce a dependency?
- Does it require an ADR?

During editing:

- Keep changes small and reviewable.
- Do not mix unrelated refactors with feature work.
- Add tests near the module.
- Avoid global state.
- Prefer clear C++ over clever abstractions.
- Keep generated/manual IPC distinction clear.

After editing:

- Run relevant tests.
- Check dependency direction.
- Update docs if architecture changed.
- Update `PROJECT_STATE.md` if milestone state changed.
- Add or update ADR if needed.

## 4. Coding style defaults

- Use C++23.
- Use RAII.
- Prefer value types for simple data.
- Use strong ID types for process/tab/request/document IDs.
- Return explicit result/status types for fallible operations.
- Avoid exceptions across module boundaries unless an ADR standardizes exception policy.
- Avoid raw owning pointers.
- Avoid unbounded static/global mutable state.
- Keep headers minimal.
- Keep namespaces explicit.
- Write deterministic tests.

## 5. IPC implementation protocol

For a new IPC message:

1. Add schema definition first.
2. Define sender and receiver process.
3. Define authority assumptions.
4. Define validation rules.
5. Generate or manually mirror typed C++ binding from schema.
6. Implement handler in owning process module.
7. Add a validation test.
8. Add a sequence diagram if it introduces a new flow.

Do not add one-off message structs directly inside a process module.

## 6. Aegis implementation protocol

For request-blocking work:

1. Place rule representation in `src/aegis/rules/`.
2. Place classification logic in `src/aegis/classifier/`.
3. Keep direct network I/O out of Aegis.
4. Ensure Network Process calls Aegis before dispatch.
5. Return structured decision with reason.
6. Add allow and block tests.
7. Add diagnostics useful for debugging.

## 7. Parser/layout implementation protocol

For HTML/CSS/layout work:

- Keep parser output structures separate from layout structures.
- Add small fixtures for every parser feature.
- Treat malformed input as expected.
- Do not crash on malformed HTML/CSS.
- Do not implement JavaScript to solve parser problems in v0.1.
- Do not import browser process policy into engine code.

## 8. Dependency introduction protocol

Before adding a dependency:

1. Identify why it is necessary.
2. Identify whether it is dangerous or huge.
3. Check license.
4. Put it behind an adapter.
5. Prevent dependency types from leaking across Speed APIs.
6. Add ADR unless the dependency is already covered by an accepted dependency family.
7. Update `third_party/README.md` and relevant docs.

## 9. Stop conditions

Codex must stop implementation and produce an ADR or design note when:

- A Forbidden design seems necessary.
- A Fixed decision must change.
- A Strong Default must change.
- A new process type is required.
- A new cross-process authority appears.
- A new persistent storage authority appears.
- v0.1 scope would expand.
- A dependency cycle appears.

## 10. Output expectation for Codex sessions

Each implementation session should end with:

- Summary of files changed.
- Tests added/updated.
- Architecture documents consulted.
- Decision class used.
- ADR created or not required.
- Known follow-up issues.
