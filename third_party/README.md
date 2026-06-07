# Third Party Dependencies

No third-party dependencies are vendored yet.

System dependencies currently used by the default dev build:

- OpenSSL for MVP HTTPS fetch through the Network module adapter. See
  `docs/adr/0002-use-openssl-for-mvp-https-fetch.md`.
- X11 and Cairo for MVP GUI windowing and CPU 2D drawing through the Platform window adapter. See
  `docs/adr/0003-use-x11-cairo-for-mvp-gui-shell.md`.

Before adding one:

- Check its license and update this directory.
- Keep usage behind a narrow Speed adapter.
- Do not expose third-party types across public Speed module boundaries.
- Add an ADR when required by `docs/DEPENDENCY_RULES.md`.
