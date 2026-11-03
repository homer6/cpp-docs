# Resource limits and availability

> Make resource consumption part of validation, so valid-looking requests can't exhaust the service.

> **Imported** from the research report *Security Practices for Modern C++* (2026-11-02, [#287](https://github.com/homer6/cpp-docs/issues/287)). It is being verified against primary sources and our compilers in [#299](https://github.com/homer6/cpp-docs/issues/299) (epic [#286](https://github.com/homer6/cpp-docs/issues/286)); until then, treat its claims as leads.

**MUST:** Make resource consumption part of input validation. A request can be within representable numeric limits yet violate the service’s security requirements for availability.[^23][^7]

Define explicit limits for:

- Memory and aggregate allocation
- Message and decompressed sizes
- Collection counts and nesting depth
- CPU-intensive operations and parsing work
- Threads, queued tasks, handles, and connections
- Request duration, retries, and concurrent expensive operations
- Per-user or per-tenant consumption

These are recommended threat-model-derived controls, not universal numeric thresholds.[^7]

**SHOULD:** Reserve expensive work until after cheap rejection checks and appropriate authorization. Apply quotas across the whole operation rather than independently approving each small allocation.[^20][^7]

**MUST:** Test resource-limit behavior and verify that rejection, cancellation, and cleanup do not create a different exhaustion path.[^2]

## Sources

[^2]: <https://learn.microsoft.com/en-us/cpp/code-quality/build-reliable-secure-programs?view=msvc-170>
[^7]: <https://www.cisa.gov/sites/default/files/2023-10/Shifting-the-Balance-of-Cybersecurity-Risk-Principles-and-Approaches-for-Secure-by-Design-Software.pdf>
[^20]: <https://github.com/OWASP/CheatSheetSeries/blob/master/cheatsheets/Authorization_Cheat_Sheet.md>
[^23]: <https://github.com/cmu-sei/secure-coding-standards/blob/main/content/4.sei-cert-c-coding-standard/03.rules/12.integers-int/2.int30-c.md>
