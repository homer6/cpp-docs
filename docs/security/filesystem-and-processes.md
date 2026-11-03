# Filesystem, process and FFI boundaries

> Avoid check-then-use, constrain process launch and loading, and treat foreign interfaces as trust boundaries.

> **Imported** from the research report *Security Practices for Modern C++* (2026-11-02, [#287](https://github.com/homer6/cpp-docs/issues/287)). It is being verified against primary sources and our compilers in [#298](https://github.com/homer6/cpp-docs/issues/298) (epic [#286](https://github.com/homer6/cpp-docs/issues/286)); until then, treat its claims as leads.

## Avoid check-then-use security

A path can change meaning during filesystem traversal, particularly when untrusted directories or symbolic links are involved; Linux provides constrained-resolution facilities specifically for this problem.[^16][^39]

**MUST:** Do not treat string normalization, path-prefix comparison, or “canonicalize and then open” as a complete containment mechanism.

**SHOULD:** Use handle-relative, operating-system-supported constrained operations where available. On Linux, `openat2` resolution flags can restrict traversal beneath a directory and disallow symbolic-link resolution, subject to the selected flags and operation semantics.[^16]

## Constrain process execution and loading

**MUST:** Control executable selection, arguments, inherited environment, handles, working directories, and privileges when launching another process.

**SHOULD:** Avoid shell interpretation for operations that can use structured process APIs. Restrict plugin and dynamic-library loading to approved locations and artifacts; these are proposed controls for reducing executable attack surface.[^11][^8][^7]

## Treat FFI as a security boundary

**MUST:** Document buffer lengths, nullability, ownership transfer, callback lifetime, allocation/release pairing, and error conventions at every C or foreign-function interface.[^6][^3]

**MUST:** Do not assume that another language, library, or ABI enforces C++ lifetime requirements on the application’s behalf. All cross-boundary pointers and retained callbacks still require a valid lifetime argument.[^6]

## Sources

[^3]: <https://github.com/isocpp/CppCoreGuidelines>
[^6]: <https://cmu-sei.github.io/secure-coding-standards/sei-cert-cpp-coding-standard/rules/expressions-exp/exp54-cpp/>
[^7]: <https://www.cisa.gov/sites/default/files/2023-10/Shifting-the-Balance-of-Cybersecurity-Risk-Principles-and-Approaches-for-Secure-by-Design-Software.pdf>
[^8]: <https://learn.microsoft.com/en-gb/windows/win32/dxtecharts/best-security-practices-in-game-development>
[^11]: <https://slsa.dev/spec/v1.0/zonepage>
[^16]: <https://www.man7.org/linux/man-pages//man2/openat2.2.html>
[^39]: <https://man7.org/linux/man-pages/man7/path_resolution.7.html>
