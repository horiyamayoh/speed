# IPC

IPC is schema-first. Files in `src/ipc/schemas/` are the source of truth.

The C++ runtime in `src/ipc/runtime/` is intentionally small until code
generation is introduced. Generated files must live in `src/ipc/generated/` or
the build output and must not be edited manually.

The v0.1 manual runtime mirrors schema messages with typed C++ codecs and sends
them over length-prefixed binary frames. Process modules may own handlers, but
serialization, frame validation, and file-descriptor transport live here rather
than in Browser, Renderer, or Network business logic.
