# Speed Open Questions

Purpose: track questions that are not yet architectural decisions.

Open questions are not permission to improvise architecture. If implementation needs an answer, classify it in `DECISION_BACKBONE.md` and possibly write an ADR.

## 1. Process and security

| Question                                                    | Why it matters                           | Current handling                            |
| ----------------------------------------------------------- | ---------------------------------------- | ------------------------------------------- |
| What exact sandboxing APIs will be used per OS?             | Required for real security.              | Deferred; keep process boundaries abstract. |
| When does Speed move from per-tab to site/origin isolation? | Affects process count and security.      | Deferred; keep APIs site-isolation-ready.   |
| Should Network Process have disk cache authority?           | Affects storage boundary.                | Deferred; no production disk cache in v0.1. |
| How are permissions represented?                            | Needed for geolocation/camera/etc later. | Browser Process owns; minimal stub only.    |

## 2. Engine compatibility

| Question                                                 | Why it matters                            | Current handling                       |
| -------------------------------------------------------- | ----------------------------------------- | -------------------------------------- |
| What standards compatibility target is realistic?        | Determines parser/layout complexity.      | v0.1 targets simple static pages only. |
| How close should layout be to CSS specs in early phases? | Affects test design.                      | Expand by layout tests after MVP.      |
| What text shaping library should be used?                | Required for real-world text.             | Deferred behind text/font adapter.     |
| What image formats are first supported?                  | Affects Utility Process and dependencies. | v0.1 may use placeholders.             |

## 3. JavaScript and runtime

| Question                                                      | Why it matters                  | Current handling                      |
| ------------------------------------------------------------- | ------------------------------- | ------------------------------------- |
| Build a JS engine, embed an existing one, or integrate later? | Major architecture decision.    | Deferred. No production JS in v0.1.   |
| Where does JS execute?                                        | Security and process isolation. | Future Utility/Renderer boundary ADR. |
| How is DOM binding generated?                                 | Huge API surface.               | Deferred.                             |

## 4. Aegis

| Question                                | Why it matters                     | Current handling                           |
| --------------------------------------- | ---------------------------------- | ------------------------------------------ |
| Which filter syntax is supported first? | Compatibility with existing lists. | Strong default: minimal URL/domain subset. |
| How are rule updates delivered?         | Network/security/update path.      | Deferred.                                  |
| Will Aegis include cosmetic filtering?  | Requires renderer integration.     | Deferred.                                  |
| How are user exceptions represented?    | Profile policy.                    | Deferred; v0.1 may hardcode test rules.    |

## 5. UI and platform

| Question                               | Why it matters                      | Current handling                                                                      |
| -------------------------------------- | ----------------------------------- | ------------------------------------------------------------------------------------- |
| Which GUI toolkit is used first?       | Developer velocity and portability. | Strong default: behind `platform/ui` abstraction.                                     |
| Which OS is first-class initially?     | Build/test scope.                   | Pick pragmatically during implementation; document in ADR if it affects architecture. |
| How are accessibility APIs integrated? | Required for usable browser.        | Deferred.                                                                             |

## 6. Storage

| Question                                           | Why it matters            | Current handling                                       |
| -------------------------------------------------- | ------------------------- | ------------------------------------------------------ |
| What database/storage engine is used?              | History/session/cookies.  | Strong default: simple storage first behind interface. |
| How are migrations handled?                        | Long-term profile safety. | Deferred until persistent schema stabilizes.           |
| How is cache storage separated from profile state? | Security/privacy.         | Deferred.                                              |

## 7. Build and tooling

| Question                                 | Why it matters        | Current handling                                                                   |
| ---------------------------------------- | --------------------- | ---------------------------------------------------------------------------------- |
| How strict should formatting/linting be? | OSS consistency.      | Add basic formatter early.                                                         |
| Which test framework?                    | Affects all tests.    | Strong default: choose simple C++ test framework via ADR or initial tooling issue. |
| How is IPC code generated?               | Central architecture. | Start with schema-first minimal generator or manual mirror.                        |

## 8. Open question handling rule

An open question becomes a decision when:

- Implementation cannot proceed without choosing.
- Different choices would affect public APIs.
- Different choices would affect process or security boundaries.
- Different choices would add dependencies.
- Different choices would change MVP scope.

At that point, write an ADR or update the relevant decision document.
