# IPC

IPC is schema-first. Files in `src/ipc/schemas/` are the source of truth.

The C++ runtime in `src/ipc/runtime/` is intentionally small until code
generation is introduced. Generated files must live in `src/ipc/generated/` or
the build output and must not be edited manually.
