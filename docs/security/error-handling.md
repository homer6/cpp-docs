# Error handling and fail-closed behaviour

> Deny when a security decision can't be established, and leave every failure in a defined state.

> **Imported** from the research report *Security Practices for Modern C++* (2026-11-02, [#287](https://github.com/homer6/cpp-docs/issues/287)). It is being verified against primary sources and our compilers in [#296](https://github.com/homer6/cpp-docs/issues/296) (epic [#286](https://github.com/homer6/cpp-docs/issues/286)); until then, treat its claims as leads.

## Keep rejection explicit

**MUST:** Deny protected operations when authentication, authorization, or required security configuration cannot be established. Missing authorization results, errors, and timeouts must not silently become permission.[^30][^20]

**MUST:** Separate expected rejection of malformed input from evidence of internal memory corruption. Continuing after a violated internal safety invariant is not equivalent to rejecting an invalid request.[^31][^21]

A useful operating policy is to reject malformed requests locally, while terminating and replacing an isolated worker when a hardening check indicates an internal safety failure.[^21][^7]

## Handle exceptions deliberately

**SHOULD:** Use RAII to preserve resource cleanup during exceptional paths, and define exception boundaries around threads, callbacks, public APIs, and interoperability layers.[^3]

**MUST:** Avoid broad “catch and continue” behavior that leaves security-sensitive state partially updated. Failed authorization changes, credential updates, and privilege transitions should leave a documented safe state.[^30][^3]

For C++23 and later, `std::expected` can make recoverable failure explicit, but the application still needs a rule requiring callers to inspect and handle failure.[^5]

**MUST:** Do not disable exceptions indiscriminately as a security measure. Establish how allocation failures and library failures behave in the selected configuration, and test that behavior.[^2][^1]

## Sources

[^1]: <https://www.sei.cmu.edu/forms/secure-coding-cpp-form/>
[^2]: <https://learn.microsoft.com/en-us/cpp/code-quality/build-reliable-secure-programs?view=msvc-170>
[^3]: <https://github.com/isocpp/CppCoreGuidelines>
[^5]: <https://en.cppreference.com/cpp>
[^7]: <https://www.cisa.gov/sites/default/files/2023-10/Shifting-the-Balance-of-Cybersecurity-Risk-Principles-and-Approaches-for-Secure-by-Design-Software.pdf>
[^20]: <https://github.com/OWASP/CheatSheetSeries/blob/master/cheatsheets/Authorization_Cheat_Sheet.md>
[^21]: <https://queue.acm.org/detail.cfm?id=3773097>
[^30]: <https://cheatsheetseries.owasp.org/cheatsheets/Authorization_Decisions_And_Output_Handling_Cheat_Sheet.html>
[^31]: <https://clang.llvm.org/docs//AddressSanitizer.html>
