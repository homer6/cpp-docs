# Security verification

> Warnings, static analysis, sanitizers, fuzzing and failure tests: complementary evidence for security properties.

> **Imported** from the research report *Security Practices for Modern C++* (2026-11-02, [#287](https://github.com/homer6/cpp-docs/issues/287)). It is being verified against primary sources and our compilers in [#301](https://github.com/homer6/cpp-docs/issues/301) (epic [#286](https://github.com/homer6/cpp-docs/issues/286)); until then, treat its claims as leads.

## Combine complementary methods

**MUST:** Use several verification methods because each answers different questions. Microsoft recommends source and binary analysis, automated testing, dependency tracking, and secret scanning as parts of secure C++ development.[^2]

| Method | Primary value | Required caution |
| :-- | :-- | :-- |
| Compiler warnings | Identify suspicious conversions, formats, initialization, and other patterns | Warning-free code is not a security proof.[^9][^45] |
| Static analysis | Find problematic paths and enforce selected rules | Configure the actual build and review findings rather than merely producing reports.[^2] |
| AddressSanitizer | Detect classes of out-of-bounds access and lifetime misuse | Detection depends on executed paths and supported instrumentation.[^31] |
| Undefined-behavior checks | Detect selected invalid operations during execution | The enabled check group is not an exhaustive security policy.[^46] |
| ThreadSanitizer | Detect data races | Exercise relevant concurrency and use a separate compatible build.[^17][^46] |
| MemorySanitizer | Detect uninitialized reads | Plan the instrumented environment and dependencies deliberately.[^47] |
| Fuzzing | Explore malformed and unexpected inputs | Target meaningful interfaces and inspect reached behavior.[^7][^2] |
| Manual security review | Examine trust, authority, invariants, and assumptions | Review design as well as individual statements.[^3][^20] |

## Establish sanitizer configurations

A useful Clang testing configuration is:

```sh
clang++ -std=c++11 -O1 -g \
    -fno-omit-frame-pointer \
    -fsanitize=address,undefined \
    -fno-sanitize-recover=undefined \
    app.cpp -o app-asan-ubsan
```

AddressSanitizer instrumentation must be applied during compilation and linking; Clang also documents that address, thread, and memory sanitizers cannot all be combined into one program.[^46][^31]

**MUST:** Run separate configurations as appropriate for race detection and uninitialized-read detection rather than expecting one omnibus sanitizer build.[^47][^17][^46]

**SHOULD:** Add targeted checks for security-relevant integer conversions, unintended unsigned wrap, and dynamic-type misuse where supported; do not assume that a default “undefined” group covers everything relevant.[^46]

**MUST:** Treat sanitizer reports as release-blocking until investigated. Do not silence a report merely because the corresponding test appears to complete successfully.[^31][^2]

**SHOULD:** Keep diagnostic testing builds separate from the approved production profile. Production deployment of instrumentation should be a deliberate security decision, not an accidental consequence of reusing a testing configuration.[^9][^2]

## Fuzz the security boundaries

**MUST:** Fuzz components that interpret attacker-controlled structure: protocol decoders, file readers, deserializers, decompression wrappers, IPC messages, and configuration parsers. Fuzzing is explicitly recognized as a useful C/C++ risk-reduction measure.[^7]

Recommended harness policy:

- Exercise the real parser or state transition, not a simplified duplicate.
- Reset mutable state between inputs unless persistence is intentional.
- Bound memory and execution time.
- Combine with suitable sanitizer configurations.
- Retain minimized regressions and important corpus inputs.
- Add sequence-aware testing for stateful protocols.
- Investigate rejection paths, not merely successful decoding.

These are proposed verification practices consistent with continuous, automated security testing.[^7][^2]

**MUST:** Track coverage of security-relevant behavior, not just elapsed fuzzing time or the number of generated inputs. Test results should demonstrate that the targeted code actually ran.[^2]

## Test the intended failure behavior

Add explicit tests for malformed input, boundary values, failed allocations, partial I/O, cancellation, shutdown races, failed authorization, invalid peer identity, and resource exhaustion where applicable.[^33][^20][^2]

**MUST:** Give every security defect a regression test and a root-cause review. Correct the recurring unsafe pattern or missing control, not only the reported instance.[^48][^2]

## Sources

[^2]: <https://learn.microsoft.com/en-us/cpp/code-quality/build-reliable-secure-programs?view=msvc-170>
[^3]: <https://github.com/isocpp/CppCoreGuidelines>
[^7]: <https://www.cisa.gov/sites/default/files/2023-10/Shifting-the-Balance-of-Cybersecurity-Risk-Principles-and-Approaches-for-Secure-by-Design-Software.pdf>
[^9]: <https://github.com/ossf/wg-best-practices-os-developers/blob/main/docs/Compiler-Hardening-Guides/Compiler-Options-Hardening-Guide-for-C-and-C++.md>
[^17]: <https://clang.llvm.org/docs//ThreadSanitizer.html>
[^20]: <https://github.com/OWASP/CheatSheetSeries/blob/master/cheatsheets/Authorization_Cheat_Sheet.md>
[^31]: <https://clang.llvm.org/docs//AddressSanitizer.html>
[^33]: <https://cheatsheetseries.owasp.org/cheatsheets/Transport_Layer_Security_Cheat_Sheet.html>
[^45]: <https://www.sei.cmu.edu/blog/rust-software-security-a-current-state-assessment/>
[^46]: <https://clang.llvm.org/docs//UsersManual.html>
[^47]: <https://rocm.docs.amd.com/projects/llvm-project/en/docs-6.4.1/LLVM/clang/html/MemorySanitizer.html>
[^48]: <https://www.sei.cmu.edu/annual-reviews/2024-year-in-review/lasting-impact-the-cert-secure-coding-initiative/>
