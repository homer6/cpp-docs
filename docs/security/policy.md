# Security policy, review and adoption

> Enforceable rules, release gates, a review checklist and an adoption order.

> **Imported** from the research report *Security Practices for Modern C++* (2026-11-02, [#287](https://github.com/homer6/cpp-docs/issues/287)). It is being verified against primary sources and our compilers in [#304](https://github.com/homer6/cpp-docs/issues/304) (epic [#286](https://github.com/homer6/cpp-docs/issues/286)); until then, treat its claims as leads.

## Enforceable project policy

### Rules to automate

The most useful coding standard is a selected, enforceable security profile—not an enormous document that developers are expected to remember.[^48][^3]

**Recommended mandatory controls:**

| Control | Suggested enforcement |
| :-- | :-- |
| No ordinary owning raw pointers | Static analysis and review of allocation sites.[^3] |
| No unreviewed manual lifetime or storage reinterpretation | Restricted wrappers, ownership rules, specialist review.[^6] |
| Explicit checks for untrusted sizes and conversions | Analyzer rules, boundary tests, fuzzing.[^23][^2] |
| No production dependency on debug-only validation | Release-configuration tests and review.[^18][^2] |
| No unresolved sanitizer findings | CI failure and tracked investigation.[^31][^2] |
| Explicit authorization for protected operations | Central policy, negative tests, resource-specific checks.[^20] |
| Production library and binary hardening | Approved profiles and artifact inspection.[^9][^2] |
| No committed secrets | Pre-commit and CI secret scanning.[^2] |
| Controlled release provenance | Trusted builder, digest verification, signing policy.[^11][^49] |

**SHOULD:** Maintain a small exception register containing the violated rule, security rationale, affected code, compensating controls, reviewer, owner, and review date. Organizational tailoring is explicitly contemplated by the Core Guidelines.[^3]

### Release-blocking criteria

The following is a proposed security release gate:

- No unexplained memory-safety, lifetime, bounds, or race findings.
- No known exploitable vulnerability accepted without an explicit risk decision.
- Required authorization and failure-policy tests pass.
- Input and resource limits are implemented and tested.
- High-risk parsers have meaningful instrumented testing and fuzzing.
- Production hardening is verified in the release artifact.
- Dependencies and release provenance are accounted for.
- Security exceptions are owned and current.
- A patch and incident-response owner is assigned.

These gates operationalize the combined prevention, verification, hardening, and lifecycle approach recommended by the cited guidance; they are not a certification that defects are impossible.[^11][^7][^2]

## Security review checklist

Use these questions for code touching a trust boundary or privileged action:

| Area | Review question |
| :-- | :-- |
| Trust | What can an attacker control, directly or indirectly? |
| Authority | What specific permission allows this action on this object? |
| Ownership | Who owns every resource, and where is ownership transferred? |
| Lifetime | Can a pointer, reference, iterator, view, or callback outlive its referent? |
| Bounds | What establishes the validity of every index, extent, and range? |
| Arithmetic | Can validation or allocation arithmetic overflow, wrap, or narrow? |
| State | Can an operation observe a partially initialized or partially updated security state? |
| Concurrency | What synchronizes every access to shared mutable state? |
| Parsing | Are ambiguous, malformed, and partially consumed inputs rejected as specified? |
| Failure | Does each relevant error leave a defined safe state? |
| Secrets | Are copies, logs, comparisons, and retention controlled? |
| Resources | Are memory, work, queues, handles, and duration bounded? |
| Deployment | Are privileges and reachable capabilities minimized? |
| Evidence | Which review, analysis, and tests establish these properties? |

This checklist is an original synthesis of the report’s security policy, grounded in the Core Guidelines, CERT rules, authorization guidance, and secure-development verification practices.[^1][^20][^3][^2]

## Adoption priorities

For an existing C++11-or-later codebase, prioritize work by exposure and potential consequence rather than by how modern the syntax looks.[^7][^3]

| Priority | Action | Expected security benefit |
| :-- | :-- | :-- |
| Immediate | Inventory exposed native-code entry points, dependencies, and privileged components | Establish where compromise is most consequential.[^7][^2] |
| Immediate | Enable supported production hardening and library checks | Add defenses without waiting for a language migration.[^9][^13] |
| Immediate | Add ASan/UBSan testing and investigate findings | Identify exercised memory and undefined-behavior defects.[^31][^46] |
| Next | Repair ownership, borrowed-lifetime, numeric, and bounds hazards in exposed code | Remove important defect classes at their source.[^6][^4][^23] |
| Next | Fuzz high-risk interfaces and test authorization failures | Exercise hostile inputs and security decisions.[^7][^20] |
| Next | Establish concurrency analysis and asynchronous-lifetime review | Address races and delayed use of destroyed state.[^17][^29] |
| Ongoing | Isolate privileged work, strengthen release assurance, and maintain patchability | Reduce blast radius and lifecycle exposure.[^7][^49][^2] |

**The governing rule is to require evidence for security properties rather than infer them from language version, coding style, library choice, or a clean test run.** Modern C++ facilities are valuable when they reduce the number of assumptions the application must manually maintain; they are not substitutes for establishing and verifying those assumptions.[^45][^3][^2]

## Sources

[^1]: <https://www.sei.cmu.edu/forms/secure-coding-cpp-form/>
[^2]: <https://learn.microsoft.com/en-us/cpp/code-quality/build-reliable-secure-programs?view=msvc-170>
[^3]: <https://github.com/isocpp/CppCoreGuidelines>
[^4]: <https://cmu-sei.github.io/secure-coding-standards/sei-cert-cpp-coding-standard/rules/containers-ctr/ctr51-cpp/>
[^6]: <https://cmu-sei.github.io/secure-coding-standards/sei-cert-cpp-coding-standard/rules/expressions-exp/exp54-cpp/>
[^7]: <https://www.cisa.gov/sites/default/files/2023-10/Shifting-the-Balance-of-Cybersecurity-Risk-Principles-and-Approaches-for-Secure-by-Design-Software.pdf>
[^9]: <https://github.com/ossf/wg-best-practices-os-developers/blob/main/docs/Compiler-Hardening-Guides/Compiler-Options-Hardening-Guide-for-C-and-C++.md>
[^11]: <https://slsa.dev/spec/v1.0/zonepage>
[^13]: <https://learn.microsoft.com/en-us/cpp/overview/cpp-conformance-improvements?view=msvc-170>
[^17]: <https://clang.llvm.org/docs//ThreadSanitizer.html>
[^18]: <https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2026/p4433r0.pdf>
[^20]: <https://github.com/OWASP/CheatSheetSeries/blob/master/cheatsheets/Authorization_Cheat_Sheet.md>
[^23]: <https://github.com/cmu-sei/secure-coding-standards/blob/main/content/4.sei-cert-c-coding-standard/03.rules/12.integers-int/2.int30-c.md>
[^29]: <https://help.klocwork.com/current/en-us/concepts/cppcoreguidelinesmappedtoklocworkcheckers.htm>
[^31]: <https://clang.llvm.org/docs//AddressSanitizer.html>
[^45]: <https://www.sei.cmu.edu/blog/rust-software-security-a-current-state-assessment/>
[^46]: <https://clang.llvm.org/docs//UsersManual.html>
[^48]: <https://www.sei.cmu.edu/annual-reviews/2024-year-in-review/lasting-impact-the-cert-secure-coding-initiative/>
[^49]: <https://slsa.dev/spec/v1.0/requirements>
