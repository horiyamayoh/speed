# Speed Security Model

Purpose: define trust boundaries, threat assumptions, and minimum security rules.

## 1. Threat model summary

Speed treats web content, downloaded resources, parsed documents, images, fonts, scripts, PDFs, and media as potentially malicious. The browser must assume that an attacker can control page content and attempt to exploit parsers, layout, rendering, network behavior, and IPC.

## 2. Trust levels

| Component             |            Trust level | Notes                                               |
| --------------------- | ---------------------: | --------------------------------------------------- |
| Browser Process       |          Highest trust | Still validates inputs; owns authority.             |
| Network Process       |     Privileged service | External communication allowed; policy constrained. |
| Renderer Process      |              Untrusted | Web content processing.                             |
| Utility Process       | Untrusted or low trust | Dangerous parsing; disposable.                      |
| Aegis core rules      |          Trusted logic | Input lists may be untrusted; parser must validate. |
| Third-party libraries |        Contained trust | Use wrappers; isolate dangerous ones when feasible. |

## 3. Core security invariants

- Renderer cannot perform external network I/O.
- Renderer cannot directly access profile storage.
- Renderer cannot grant itself permissions.
- Network requests pass through Aegis policy before external dispatch.
- IPC messages are schema-defined and validated.
- Child process crashes must not corrupt Browser Process state.
- Untrusted data crossing into trusted process must be parsed minimally and defensively.
- Browser Process should avoid complex parsing of attacker-controlled content when that parsing can be delegated.

## 4. External communication rule

Only the Network Process may contact the external network.

Allowed:

- Browser Process asks Network Process to fetch.
- Network Process consults Aegis.
- Network Process returns structured result.

Forbidden:

- Renderer opens sockets.
- Renderer uses platform APIs to fetch URLs.
- Utility Process opens arbitrary external network connections.
- Aegis update code bypasses Network Process unless explicitly covered by a future ADR.

## 5. Storage authority rule

Browser Process owns profile and storage authority.

Allowed:

- Browser Process writes history after navigation commit.
- Renderer requests storage-like behavior through future brokered APIs.
- Network Process may use cache only through a defined cache authority if added later.

Forbidden:

- Renderer directly writes history.
- Renderer opens profile database/files.
- Network Process directly mutates user-visible history.

## 6. Aegis security role

Aegis participates in:

- Request allow/block decisions.
- Tracker/ad classification.
- Future cosmetic filtering policy.
- Future privacy surfaces.

Security rules:

- Aegis rule parsing must validate input and fail closed for malformed dangerous rules.
- Aegis decisions must include a reason code for debugging.
- Network Process must not provide a non-Aegis fetch bypass.
- Browser UI may expose Aegis state, but UI must not be the enforcement layer.

## 7. IPC validation rules

Every IPC receiver must:

- Validate enum values.
- Validate lengths and sizes.
- Validate IDs belong to the sending process where applicable.
- Reject unknown required fields.
- Handle duplicate, stale, or out-of-order messages.
- Avoid trusting renderer-supplied URLs, origins, titles, or file paths without Browser Process validation.

## 8. Memory safety posture

C++23 is powerful and risky. Speed must adopt a defensive C++ subset:

- Prefer RAII ownership.
- Prefer `std::unique_ptr`, `std::shared_ptr` only when ownership is genuinely shared.
- Avoid raw owning pointers.
- Avoid unchecked buffer access.
- Use spans/views only with clear lifetime rules.
- Use value types for IPC-decoded messages when practical.
- Keep unsafe conversions localized.
- Add fuzz targets for parsers as soon as infrastructure exists.

## 9. Third-party dependency security

Third-party dependencies are allowed for dangerous or huge domains, but must follow wrapper rules:

- Dependency usage lives in an adapter module.
- Engine core depends on Speed interfaces, not dependency headers.
- Security update ownership must be documented.
- Dangerous decoders/parsers must be Utility Process-ready.
- License compatibility must be checked before adoption.

## 10. v0.1 minimum security checks

Before v0.1 is considered complete:

- Renderer has no external network code path.
- Network fetch path always invokes Aegis.
- IPC messages have schema definitions.
- Renderer crash does not kill Browser Process.
- Basic IPC validation tests exist.
- Aegis block/allow tests exist.
- Profile write APIs are not linked into Renderer Process.
- Forbidden designs are not present in dependency graph.
