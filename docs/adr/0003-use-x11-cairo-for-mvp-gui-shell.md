# ADR-0003: Use X11 and Cairo for the MVP GUI shell

Status: Accepted
Date: 2026-06-07
Owners: Speed maintainers
Decision class affected: Strong Default / Experimental

## Context

Speed v0.1 needs a minimal user-operable GUI BrowserShell without embedding Chromium, WebKit,
Gecko, or any other full browser engine. The GUI must draw Speed's own CPU display list and keep
toolkit/platform types out of Browser, Renderer, Network, and Engine public APIs.

Adding a concrete platform windowing and drawing dependency requires an ADR under the dependency
rules. The current MVP development environment has working X11 and Cairo headers/libraries.

## Decision

Use X11 for the Linux MVP window/event backend and Cairo for CPU 2D drawing. X11 and Cairo usage is
confined to `src/platform/window/x11_window.cpp`; public Speed APIs expose only small `Canvas`,
`PlatformWindow`, and event value types under `src/platform/window/`.

The GUI shell lives above that boundary and renders `engine::paint::DisplayList` commands into the
abstract canvas. It does not use WebView/browser-engine widgets and does not add JavaScript,
cookies, media, GPU compositing, or full CSS support.

## Consequences

- `speed-browser-gui` is Linux/X11-oriented for v0.1.
- CMake now requires system X11 and Cairo development files when building the default dev target.
- X11/Cairo types must not leak into `browser`, `renderer`, `network`, `engine`, or `ui` public
  interfaces outside the platform window adapter.
- Future Wayland, macOS, Windows, or richer text backends can replace or coexist behind the same
  platform window abstraction.
- Cairo is used as a system dependency under LGPL-2.1/MPL-1.1 terms; X11 is used as a system
  dependency under MIT/X11-style terms. Neither dependency is vendored.
