# Security Policy

Speed treats browser security boundaries as core architecture.

## Reporting

A private reporting channel has not been published yet. Until one exists, do not
post exploit details publicly. Open a minimal issue asking maintainers to provide
a private security contact, without including sensitive details.

## Security Invariants

- Renderer processes are untrusted.
- Only the Network Process may perform external network communication.
- Renderer code must not access profile storage directly.
- Network requests must pass through Aegis classification before dispatch.
- IPC must be schema-first and reviewable.
