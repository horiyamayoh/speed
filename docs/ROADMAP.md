# Speed Roadmap and Initial Issue Plan

Purpose: define initial implementation milestones and seed GitHub issues.

## 1. Roadmap phases

### Phase 0: Repository and architecture foundation

Goal: create the project skeleton, docs, build targets, and contribution baseline.

### Phase 1: Process and IPC foundation

Goal: Browser, Renderer, and Network process skeletons communicate through schema-first IPC.

### Phase 2: Navigation and Network MVP

Goal: URL navigation flows through Browser -> Network -> Browser/Renderer, with Aegis policy enforcement.

### Phase 3: Engine static rendering MVP

Goal: basic HTML/CSS to DOM/style/layout/paint pipeline renders simple pages.

### Phase 4: Browser shell features

Goal: tabs, history, error pages, basic recovery.

### Phase 5: Test and stabilization

Goal: add tests, fixtures, CI, crash checks, and documentation polish.

## 2. Initial GitHub issues

### Phase 0 — Repository foundation

1. **Create initial repository skeleton**  
   Labels: `phase-0`, `repo`, `good-first-issue`  
   Acceptance: top-level directories and placeholder README exist.

2. **Add architecture docs under `docs/`**  
   Labels: `phase-0`, `docs`  
   Acceptance: all v0.1 backbone docs are committed.

3. **Add CMake build skeleton**  
   Labels: `phase-0`, `build`  
   Acceptance: configure/build succeeds with empty module targets.

4. **Create module targets for base/platform/ipc/browser/renderer/network/aegis/engine/storage**  
   Labels: `phase-0`, `build`, `architecture`  
   Acceptance: each module builds as separate target or clearly separated library.

5. **Add code formatting configuration**  
   Labels: `phase-0`, `tooling`  
   Acceptance: formatter command is documented.

6. **Add contribution and security policy docs**  
   Labels: `phase-0`, `oss`, `docs`  
   Acceptance: `CONTRIBUTING.md` and `SECURITY.md` exist.

7. **Add base Result/Status type**  
   Labels: `phase-0`, `base`  
   Acceptance: fallible operations can return explicit status.

8. **Add strong ID types for TabId, ProcessId, RequestId**  
   Labels: `phase-0`, `base`  
   Acceptance: IDs are not plain interchangeable integers in public APIs.

### Phase 1 — Process and IPC foundation

9. **Add process entrypoint skeletons**  
   Labels: `phase-1`, `process`  
   Acceptance: browser, renderer, network entrypoints compile.

10. **Implement platform process launch abstraction**  
    Labels: `phase-1`, `platform`, `process`  
    Acceptance: Browser can launch a child process in a smoke test or stubbed platform implementation.

11. **Define IPC schema file format v0**  
    Labels: `phase-1`, `ipc`, `architecture`  
    Acceptance: message schema examples exist for navigation and lifecycle.

12. **Implement minimal IPC serialization runtime**  
    Labels: `phase-1`, `ipc`  
    Acceptance: typed message can encode/decode with validation.

13. **Add IPC codegen stub or schema mirror tool**  
    Labels: `phase-1`, `ipc`, `tooling`  
    Acceptance: schema-first workflow is represented even if generator is minimal.

14. **Implement Browser Process child registry**  
    Labels: `phase-1`, `browser`, `process`  
    Acceptance: Browser tracks child process IDs and roles.

15. **Implement Renderer Process handshake**  
    Labels: `phase-1`, `renderer`, `ipc`  
    Acceptance: Renderer announces ready state to Browser.

16. **Implement Network Process handshake**  
    Labels: `phase-1`, `network`, `ipc`  
    Acceptance: Network Process announces ready state to Browser.

17. **Add IPC validation tests**  
    Labels: `phase-1`, `ipc`, `tests`  
    Acceptance: malformed messages are rejected.

### Phase 2 — Navigation, Network, Aegis

18. **Implement URL request model**  
    Labels: `phase-2`, `network`  
    Acceptance: request includes URL, method, context, request ID.

19. **Implement Aegis rule representation v0**  
    Labels: `phase-2`, `aegis`  
    Acceptance: simple domain/URL block rules are represented.

20. **Implement Aegis rule parser v0**  
    Labels: `phase-2`, `aegis`, `tests`  
    Acceptance: valid rules load; malformed rules fail safely.

21. **Implement Aegis classifier v0**  
    Labels: `phase-2`, `aegis`  
    Acceptance: allow/block decision with reason code.

22. **Wire Aegis into Network Process before fetch dispatch**  
    Labels: `phase-2`, `network`, `aegis`, `security`  
    Acceptance: blocked request never reaches external fetch implementation.

23. **Implement network fetch adapter interface**  
    Labels: `phase-2`, `network`, `dependency`  
    Acceptance: concrete HTTP implementation is behind interface.

24. **Implement basic HTTP/HTTPS fetch path**  
    Labels: `phase-2`, `network`  
    Acceptance: simple page body can be fetched through Network Process.

25. **Implement navigation request flow in Browser Process**  
    Labels: `phase-2`, `browser`, `navigation`  
    Acceptance: UI/request stub can initiate navigation.

26. **Add blocked navigation error result**  
    Labels: `phase-2`, `browser`, `aegis`  
    Acceptance: blocked request produces structured error page state.

27. **Add Aegis allow/block unit tests**  
    Labels: `phase-2`, `aegis`, `tests`  
    Acceptance: allow and block cases covered.

### Phase 3 — Engine static rendering

28. **Implement DOM node model v0**  
    Labels: `phase-3`, `engine`, `dom`  
    Acceptance: element/text/document nodes can be constructed and traversed.

29. **Implement HTML tokenizer subset**  
    Labels: `phase-3`, `engine`, `html`  
    Acceptance: simple tags/text tokenize deterministically.

30. **Implement HTML parser subset to DOM**  
    Labels: `phase-3`, `engine`, `html`, `dom`  
    Acceptance: simple HTML fixture builds expected DOM.

31. **Implement CSS tokenizer/parser subset**  
    Labels: `phase-3`, `engine`, `css`  
    Acceptance: simple selectors/declarations parse.

32. **Implement style resolver v0**  
    Labels: `phase-3`, `engine`, `style`  
    Acceptance: type/class/id selectors apply in deterministic order.

33. **Implement layout tree v0**  
    Labels: `phase-3`, `engine`, `layout`  
    Acceptance: block boxes get positions/sizes in simple fixtures.

34. **Implement paint command list v0**  
    Labels: `phase-3`, `engine`, `paint`  
    Acceptance: layout output creates rect/text paint commands.

35. **Implement renderer document commit pipeline**  
    Labels: `phase-3`, `renderer`, `engine`  
    Acceptance: committed document bytes produce paint commands.

36. **Add layout smoke tests**  
    Labels: `phase-3`, `layout`, `tests`  
    Acceptance: fixture pages have stable layout output.

### Phase 4 — Browser shell and persistence

37. **Implement minimal browser window shell**  
    Labels: `phase-4`, `ui`  
    Acceptance: window opens with address input area or navigation stub.

38. **Implement tab model v0**  
    Labels: `phase-4`, `browser`, `tabs`  
    Acceptance: create/switch/close tabs in memory.

39. **Connect rendered output to window surface**  
    Labels: `phase-4`, `ui`, `renderer`, `paint`  
    Acceptance: simple page visibly renders.

40. **Implement basic history store**  
    Labels: `phase-4`, `storage`, `history`  
    Acceptance: successful top-level navigations are recorded by Browser Process.

41. **Implement basic error page rendering**  
    Labels: `phase-4`, `browser`, `renderer`  
    Acceptance: network failure/block reason displays as simple page.

42. **Implement renderer crash detection and tab crash state**  
    Labels: `phase-4`, `process`, `recovery`  
    Acceptance: killing renderer does not kill Browser Process.

### Phase 5 — Stabilization and quality

43. **Add integration smoke test for static page navigation**  
    Labels: `phase-5`, `tests`, `integration`  
    Acceptance: local fixture page navigates and renders through full pipeline.

44. **Add dependency direction review script stub**  
    Labels: `phase-5`, `architecture`, `tooling`  
    Acceptance: script can list module includes or target dependencies.

45. **Add fuzz target skeletons for HTML/CSS/Aegis parsers**  
    Labels: `phase-5`, `fuzz`, `security`  
    Acceptance: fuzz targets compile or are clearly stubbed.

46. **Add CI build and test workflow**  
    Labels: `phase-5`, `ci`  
    Acceptance: build and core tests run in CI.

47. **Document v0.1 build/run/test commands**  
    Labels: `phase-5`, `docs`  
    Acceptance: README includes reproducible commands.

48. **Create v0.1 architecture conformance checklist**  
    Labels: `phase-5`, `architecture`, `docs`  
    Acceptance: checklist verifies no forbidden designs.

## 3. Milestone completion checklist

v0.1 architecture foundation is complete when:

- All Phase 0 issues are done.
- Browser, Renderer, and Network skeletons run.
- IPC schema-first workflow exists.
- Aegis blocks at least one request before fetch.
- Simple static page renders.
- Tabs and history work minimally.
- Renderer crash does not kill Browser Process.
- Tests and CI exist.
- Docs reflect implemented reality.
