# Speed v0.1 MVP Scope

Purpose: prevent v0.1 from expanding into a full browser before the architecture foundation exists.

## 1. MVP definition

Speed v0.1 is successful when a developer can build and run a browser shell that opens simple static web pages in tabs, loads content through the Network Process, applies basic Aegis blocking, renders a subset of HTML/CSS through Speed's own engine pipeline, records basic history, and demonstrates recoverable multi-process boundaries.

v0.1 is not a compatibility release.

## 2. Included user-visible behavior

- Launch Speed.
- Open a basic browser window.
- Create, switch, and close tabs.
- Enter a URL.
- Navigate to a static HTTP/HTTPS page through the Network Process.
- Block requests matching Aegis v0.1 rules.
- Render simple HTML and CSS.
- Show a basic error page for failed navigation.
- Record visited URLs in basic history.
- Recover or close a tab when its Renderer Process crashes.

## 3. Included engine behavior

### HTML

Included:

- Tokenize enough HTML for static test pages.
- Build a basic DOM tree.
- Support common structural elements: `html`, `head`, `body`, `div`, `span`, `p`, `a`, `img` placeholder, headings, lists, `br`.
- Ignore unsupported tags safely.
- Preserve text nodes.

Not required:

- Full HTML5 parser compatibility.
- Scripting interactions.
- Forms beyond inert display.
- Custom elements.
- Shadow DOM.

### CSS

Included:

- Basic stylesheet parsing.
- Simple selectors: type, class, id, descendant.
- Basic cascade order.
- Basic inherited properties.
- Initial property subset:
  - `display` for block/inline/none subset.
  - `width`, `height` limited forms.
  - `margin`, `padding`, `border` simplified.
  - `color`, `background-color` simplified.
  - `font-size` simplified.
  - `text-align` optional.
- Inline styles may be implemented if simple.

Not required:

- Flexbox.
- Grid.
- Animations.
- Transforms.
- Media queries.
- Complex specificity edge cases.
- Pseudo-elements.
- Full CSSOM.

### Layout

Included:

- Basic block layout.
- Basic inline text flow or simplified inline treatment.
- Viewport root.
- Scrollable document area if feasible.
- Deterministic layout tests for simple pages.

Not required:

- Full inline formatting context.
- Floats.
- Positioning beyond static layout.
- Fragmentation.
- Complex table layout.

### Paint

Included:

- CPU-backed display list or direct paint abstraction.
- Text drawing through platform or wrapper boundary.
- Rect/background/border drawing for basic elements.
- Image placeholder rendering.

Not required:

- GPU compositor.
- Subpixel-perfect browser compatibility.
- Advanced clipping/compositing/effects.

## 4. Included process behavior

- Browser Process launches and supervises child processes.
- Renderer Process receives document content or navigation commit messages only through IPC.
- Network Process fetches resources and returns responses through IPC.
- Browser Process owns tab lifecycle.
- Renderer crash is detected and converted into a tab crash state.
- Process model must be separable from UI code.

## 5. Included Aegis behavior

Aegis v0.1 includes:

- Static rule list loading from local file or embedded test rules.
- URL/domain matching.
- Request classification before external dispatch.
- Block decision returned to Network Process and Browser Process.
- Tests for allow/block decisions.
- Logging/debug reason for blocked request.

Aegis v0.1 excludes:

- Cosmetic filtering.
- Scriptlet injection.
- ML classification.
- Subscription update service.
- Per-site user UI.
- Fingerprinting defense beyond basic request blocking.

## 6. Included storage behavior

- Profile directory abstraction.
- Basic history store.
- Basic session/tab state may be in memory or simple durable format.
- Renderer must not access storage directly.

Not required:

- Full cookies.
- Cache storage.
- IndexedDB/localStorage/sessionStorage.
- Passwords.
- Sync.

## 7. Included testing structure

v0.1 must create directory and build hooks for:

- Unit tests.
- IPC tests.
- Aegis rule tests.
- Parser tests.
- Layout tests.
- Process smoke tests.
- Future fuzz tests.
- Future WPT import.

Not all future test suites need to be populated in v0.1, but the repository layout must not block them.

## 8. Explicit v0.1 non-goals

The following must not be implemented as production features in v0.1:

- Full JavaScript engine.
- WebAssembly.
- Full video/audio/media.
- Full extension system.
- Full DevTools.
- Service Workers.
- Full cookie/storage platform.
- PDF viewer.
- GPU compositor.
- Browser sync.
- Password manager.
- Full accessibility tree.
- Complete standards compatibility.

Prototypes may exist only behind clearly marked experimental directories or flags and must not change the MVP definition.

## 9. MVP acceptance criteria

v0.1 is acceptable when:

- The browser shell starts reliably.
- Opening a URL causes Browser -> Network -> Browser/Renderer flow through schema-defined IPC.
- Aegis can block at least one known test URL and allow another.
- A simple HTML page with CSS renders visibly.
- Multiple tabs can exist independently.
- History records successful top-level navigations.
- Killing a Renderer Process does not terminate the Browser Process.
- Tests can be run with one documented command.
- Code layout matches `REPOSITORY_STRUCTURE.md`.
- No Forbidden Design is used.

## 10. Scope-change rule

Any change that adds a v0.1 production feature outside this file requires an ADR. The ADR must explain:

- Why the feature is necessary before v0.1.
- Which current milestone it displaces.
- Which process and module own it.
- Which security boundary it affects.
- Which tests will prove it does not expand hidden scope.
