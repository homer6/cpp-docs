# Security

> How to write C++ that holds up against an attacker: trust boundaries, untrusted input, secrets, hardening, verification and the supply chain.

**Security is not the same as safety.** [Memory & safety](../guidelines/safety.md)
is about code that doesn't misbehave: no undefined behaviour, dangling, overflow
or races. Security is about code that holds up when someone is *trying* to make
it misbehave, or to misuse what it's allowed to do: trust boundaries,
authorization, secrets, attack surface and the supply chain. These pages build
on the safety pages and link to them where a safety defect is the way in.

## Secure C++ is built in layers

C++ written to the [C++ Core Guidelines](../guidelines/core-guidelines/README.md),
with explicit ownership, bounded interfaces, checked conversions and the type,
bounds and lifetime [profiles](../guidelines/safety.md#the-three-safety-profiles),
and checked with analysis, sanitizers and fuzzing, gives strong guarantees about
memory and lifetimes. Security asks for more than that: deciding what an attacker
controls, keeping privileged code small, validating and bounding untrusted input,
protecting secrets, hardening the build, and keeping the release trustworthy.
The foundation is the security-relevant part of the Core Guidelines and the
[SEI CERT C++ Coding Standard](https://cmu-sei.github.io/secure-coding-standards/sei-cert-cpp-coding-standard/),
plus application-security practice and the protections your compiler, library
and operating system provide.

| Layer | What to do | What it buys |
|---|---|---|
| Architecture | Minimize privileges, exposed interfaces and trusted components | Fewer ways in, and less damage when one is found[^cisa] |
| Source code | Enforce ownership, lifetime, bounds, initialization and synchronization rules | The common exploitable defects don't get written[^cg] |
| Verification | Combine review, static analysis, instrumented tests and fuzzing | Each method finds defects the others miss[^ms-reliable] |
| Runtime protection | Turn on production library and binary hardening | Exploitation the source-level rules missed is detected or blocked[^ossf] |
| Delivery and operation | Verify build provenance, track dependencies, stay patchable | The shipped artifact is the one you built, and you can fix it[^slsa] |

**No single layer proves a program secure.** Microsoft's C++ security guidance
says its tools and practices don't make an application immune from attack, but
make a successful attack less likely.[^ms-best] The pages below say which layer
each practice belongs to and what evidence shows it's in place.

## How these pages are written

- **Rules** are short imperatives labelled **MUST**, **SHOULD** or **MAY**: the
  policy we recommend for code that faces untrusted input, not requirements of
  the C++ standard.
- **Each rule cites its sources**: the Core Guidelines rule IDs (linked to our
  [index](../guidelines/core-guidelines/README.md)) and SEI CERT rule IDs
  (linked to CERT's site; CERT's text isn't copied), plus vendor docs, papers
  and cppreference for the facts.
- **Examples are ours and compile-checked** like every snippet in the repo;
  claims that can be checked at run time have tests in `tests/security/`, one
  file per page.
- **Status:** a page marked *imported* is still the text of the research report
  this section started from, and its claims are leads until its verification
  issue (epic [#286](https://github.com/homer6/cpp-docs/issues/286)) lands.
  A *verified* page has been checked against primary sources and our compilers.

## Pages

| Page | What it covers | Status |
|---|---|---|
| [Language versions and toolchains](language-versions.md) | Language version, toolchain and libraries are separate security decisions. | ✅ verified |
| [Threat models and trust boundaries](threat-modeling.md) | What an attacker controls, what must stay protected, and how to keep the trusted core small. | ✅ verified |
| [Ownership and lifetime](ownership-and-lifetime.md) | Explicit ownership and reviewed borrowed lifetimes, so no resource is used after release or released twice. | ✅ verified |
| [Bounds and containers](bounds.md) | Carry extents with the data, check attacker-influenced indices before access, and respect invalidation. | ✅ verified |
| [Integer and size calculations](integers.md) | Define numeric domains and check before arithmetic, so untrusted sizes can't wrap, narrow or exhaust. | ✅ verified |
| [Object state and representation](object-representation.md) | Initialize before use, serialize explicitly across boundaries, and confine representation tricks. | ✅ verified |
| [Parsing and validation](parsing-and-validation.md) | Parse untrusted input into constrained values, define ambiguous cases, and bound the parser's work. | ✅ verified |
| [Concurrency and asynchronous code](concurrency.md) | One synchronization rule per shared object, and a lifetime argument for every queued or suspended operation. | ✅ verified |
| [Error handling and fail-closed behaviour](error-handling.md) | Deny when a security decision can't be established, and leave every failure in a defined state. | ✅ verified |
| [Cryptography and secrets](cryptography-and-secrets.md) | Use maintained cryptographic libraries, a real cryptographic random source, and careful secret handling. | ✅ verified |
| [Filesystem, process and FFI boundaries](filesystem-and-processes.md) | Avoid check-then-use, constrain process launch and loading, and treat foreign interfaces as trust boundaries. | ✅ verified |
| [Resource limits and availability](resource-limits.md) | Make resource consumption part of validation, so valid-looking requests can't exhaust the service. | ✅ verified |
| [Compiler and library hardening](hardening.md) | Production hardening profiles per platform and standard library, checked in the final artifact. | ✅ verified |
| [Security verification](verification.md) | Warnings, static analysis, sanitizers, fuzzing and failure tests: complementary evidence for security properties. | ✅ verified |
| [Supply chain and release security](supply-chain.md) | Control build inputs, verify provenance, and keep the evidence for every release. | imported |
| [Deployment and incident readiness](deployment.md) | Least privilege, isolation of risky components, careful logging, and a process for vulnerabilities. | ✅ verified |
| [Security policy, review and adoption](policy.md) | Enforceable rules, release gates, a review checklist and an adoption order. | imported |

## See also

- The [`cpp-security`](../../.claude/skills/cpp-security/SKILL.md) skill
  (`/cpp-security` in Claude Code) applies these pages: security reviews,
  hardening a build, planning security tests

- [Memory & safety](../guidelines/safety.md): the profiles, ownership,
  dangling, C++26 hardening and erroneous behaviour
- [C++ Core Guidelines index](../guidelines/core-guidelines/README.md)
- [Tooling](../guidelines/tooling.md) (sanitizers, hardened libraries) and
  [Testing](../guidelines/testing.md) (fuzzing)

[^cisa]: CISA and partners, [*Secure by Design: Shifting the Balance of Cybersecurity Risk*](https://www.cisa.gov/sites/default/files/2023-10/Shifting-the-Balance-of-Cybersecurity-Risk-Principles-and-Approaches-for-Secure-by-Design-Software.pdf) (2023)
[^cg]: [C++ Core Guidelines](https://github.com/isocpp/CppCoreGuidelines)
[^ms-reliable]: Microsoft, [*Build reliable and secure C++ programs*](https://learn.microsoft.com/en-us/cpp/code-quality/build-reliable-secure-programs)
[^ossf]: OpenSSF, [*Compiler Options Hardening Guide for C and C++*](https://github.com/ossf/wg-best-practices-os-developers/blob/main/docs/Compiler-Hardening-Guides/Compiler-Options-Hardening-Guide-for-C-and-C++.md)
[^slsa]: [SLSA specification v1.0](https://slsa.dev/spec/v1.0/)
[^ms-best]: Microsoft, [*Security best practices for C++*](https://learn.microsoft.com/en-us/cpp/security/security-best-practices-for-cpp)
