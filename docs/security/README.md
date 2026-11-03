# Security

> How to write C++ that holds up against an attacker: trust boundaries, untrusted input, secrets, hardening, verification and the supply chain.

> **Imported** from the research report *Security Practices for Modern C++* (2026-11-02, [#287](https://github.com/homer6/cpp-docs/issues/287)). It is being verified against primary sources and our compilers in [#288](https://github.com/homer6/cpp-docs/issues/288) (epic [#286](https://github.com/homer6/cpp-docs/issues/286)); until then, treat its claims as leads.

**Security is not the same as safety.** [Memory & safety](../guidelines/safety.md)
is about code that doesn't misbehave: no undefined behaviour, dangling, overflow
or races. Security is about code that holds up when someone is *trying* to make
it misbehave, or to misuse what it's allowed to do: trust boundaries,
authorization, secrets, attack surface and the supply chain. These pages build
on the safety pages and link to them where a safety defect is the way in.

**Secure production C++ requires a deliberately restricted coding policy, explicit trust boundaries, continuous security verification, and hardened deployment—not merely newer syntax or smart pointers.** The appropriate foundation is the security-relevant subset of the C++ Core Guidelines and SEI CERT standards, supplemented by application-security controls and verified compiler, library, and operating-system protections.[^1][^2][^3]

**Scope:** C++11 and every subsequent language generation, including implementation-qualified C++26 features. Security is the sole evaluation criterion here; maintainability, performance, and “modernity” matter only where they affect security assurance.

The **MUST**, **SHOULD**, and **MAY** labels on these pages express the report’s recommended organizational policy, not requirements imposed by the C++ language standard.

## Secure C++ is built in layers

C++ written to the [C++ Core Guidelines](../guidelines/core-guidelines/README.md),
with explicit ownership, bounded interfaces, checked conversions and the type,
bounds and lifetime [profiles](../guidelines/safety.md#the-three-safety-profiles),
and checked with analysis, sanitizers and fuzzing, gives strong guarantees about
memory and lifetimes. Security asks for more than that: deciding what an attacker
controls, keeping privileged code small, validating and bounding untrusted input,
protecting secrets, hardening the build, and keeping the release trustworthy.

Adopt this layered strategy:

| Layer | Recommended policy | Security purpose |
| :-- | :-- | :-- |
| Architecture | Minimize privileges, exposed interfaces, and trusted components | Limit opportunities and consequences of compromise.[^7][^8] |
| Source code | Enforce ownership, lifetime, bounds, initialization, and synchronization rules | Prevent common exploitable defects.[^3][^1] |
| Verification | Combine review, static analysis, instrumented tests, and fuzzing | Find defects through complementary methods.[^2][^7] |
| Runtime protection | Enable production-qualified library and binary hardening | Detect or obstruct exploitation that source-level controls missed.[^9][^10] |
| Delivery and operation | Verify build provenance, monitor dependencies, and maintain patchability | Protect the delivered artifact and respond to vulnerabilities.[^11][^2] |

**No individual layer proves that a program is secure.** Microsoft’s C++ guidance explicitly distinguishes security protections from immunity to attack.[^12]

## Pages

| Page | What it covers |
|---|---|
| [Language versions and toolchains](language-versions.md) | Language version, toolchain and libraries are separate security decisions. |
| [Threat models and trust boundaries](threat-modeling.md) | What an attacker controls, what must stay protected, and how to keep the trusted core small. |
| [Ownership and lifetime](ownership-and-lifetime.md) | Explicit ownership and reviewed borrowed lifetimes, so no resource is used after release or released twice. |
| [Bounds and containers](bounds.md) | Carry extents with the data, check attacker-influenced indices before access, and respect invalidation. |
| [Integer and size calculations](integers.md) | Define numeric domains and check before arithmetic, so untrusted sizes can't wrap, narrow or exhaust. |
| [Object state and representation](object-representation.md) | Initialize before use, serialize explicitly across boundaries, and confine representation tricks. |
| [Parsing and validation](parsing-and-validation.md) | Parse untrusted input into constrained values, define ambiguous cases, and bound the parser's work. |
| [Concurrency and asynchronous code](concurrency.md) | One synchronization rule per shared object, and a lifetime argument for every queued or suspended operation. |
| [Error handling and fail-closed behaviour](error-handling.md) | Deny when a security decision can't be established, and leave every failure in a defined state. |
| [Cryptography and secrets](cryptography-and-secrets.md) | Use maintained cryptographic libraries, a real cryptographic random source, and careful secret handling. |
| [Filesystem, process and FFI boundaries](filesystem-and-processes.md) | Avoid check-then-use, constrain process launch and loading, and treat foreign interfaces as trust boundaries. |
| [Resource limits and availability](resource-limits.md) | Make resource consumption part of validation, so valid-looking requests can't exhaust the service. |
| [Compiler and library hardening](hardening.md) | Production hardening profiles per platform and standard library, checked in the final artifact. |
| [Security verification](verification.md) | Warnings, static analysis, sanitizers, fuzzing and failure tests: complementary evidence for security properties. |
| [Supply chain and release security](supply-chain.md) | Control build inputs, verify provenance, and keep the evidence for every release. |
| [Deployment and incident readiness](deployment.md) | Least privilege, isolation of risky components, careful logging, and a process for vulnerabilities. |
| [Security policy, review and adoption](policy.md) | Enforceable rules, release gates, a review checklist and an adoption order. |

## Sources

[^1]: <https://www.sei.cmu.edu/forms/secure-coding-cpp-form/>
[^2]: <https://learn.microsoft.com/en-us/cpp/code-quality/build-reliable-secure-programs?view=msvc-170>
[^3]: <https://github.com/isocpp/CppCoreGuidelines>
[^7]: <https://www.cisa.gov/sites/default/files/2023-10/Shifting-the-Balance-of-Cybersecurity-Risk-Principles-and-Approaches-for-Secure-by-Design-Software.pdf>
[^8]: <https://learn.microsoft.com/en-gb/windows/win32/dxtecharts/best-security-practices-in-game-development>
[^9]: <https://github.com/ossf/wg-best-practices-os-developers/blob/main/docs/Compiler-Hardening-Guides/Compiler-Options-Hardening-Guide-for-C-and-C++.md>
[^10]: <https://www.learn.microsoft.com/en-us/windows/desktop/SecBP/avoiding-buffer-overruns>
[^11]: <https://slsa.dev/spec/v1.0/zonepage>
[^12]: <https://learn.microsoft.com/hr-hr/cpp/security/security-best-practices-for-cpp?view=msvc-150>
