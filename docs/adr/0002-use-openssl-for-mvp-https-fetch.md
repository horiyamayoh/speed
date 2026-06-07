# ADR-0002: Use OpenSSL for MVP HTTPS fetch adapter

Status: Accepted
Date: 2026-06-07
Owners: Speed maintainers
Decision class affected: Strong Default / Deferred

## Context

Speed v0.1 needs to fetch simple static HTTPS pages through the Network Process. TLS and
certificate validation are dangerous domains that must not be hand-rolled. The Architecture
Constitution allows TLS/certificate validation libraries through narrow network wrappers, but adding
a concrete third-party dependency requires an ADR.

## Decision

Use OpenSSL in the Network module's MVP fetch adapter for HTTPS. OpenSSL types stay inside
`src/network/fetch/network_service.cpp`; no OpenSSL headers or types are exposed through public
Speed APIs. The adapter uses default certificate verification paths, SNI, and hostname verification.

## Consequences

- The default Network fetch adapter can fetch simple HTTP and HTTPS static pages.
- CMake now requires OpenSSL for the v0.1 build.
- Future network work should preserve the narrow adapter boundary and can replace this
  implementation without changing Browser or Renderer APIs.
- Advanced HTTP features such as redirects, compression, chunked response decoding, cookies, and
  cache remain out of v0.1 scope.
