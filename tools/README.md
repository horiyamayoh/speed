# Tools

This directory reserves stable locations for build helpers, IPC code generation,
formatting, linting, dependency checks, test running, and future WPT import.

Tooling should be small, documented, and safe to run from a fresh checkout.

Common commands:

```sh
tools/format/format-cpp.sh
tools/format/check-format.sh
tools/lint/run-clang-tidy.sh
npm run format
npm run lint
```
