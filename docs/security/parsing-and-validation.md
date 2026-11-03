# Parsing and validation

> Parse untrusted input into constrained values, define ambiguous cases, and bound the parser's work.

> **Imported** from the research report *Security Practices for Modern C++* (2026-11-02, [#287](https://github.com/homer6/cpp-docs/issues/287)). It is being verified against primary sources and our compilers in [#294](https://github.com/homer6/cpp-docs/issues/294) (epic [#286](https://github.com/homer6/cpp-docs/issues/286)); until then, treat its claims as leads.

## Parse into constrained values

**MUST:** Establish syntactic validity, representability, semantic validity, and authorization as separate checks. Authorization guidance explicitly requires application-specific permission checks rather than relying on library behavior.[^20]

A recommended processing sequence is:

1. Apply transport and message-size limits.
2. Parse using a defined grammar.
3. Check numeric ranges, lengths, and structural relationships.
4. Construct validated internal values.
5. Authorize the requested action against the specific resource.
6. Execute using bounded resources.

This sequence is a proposed architecture for combining secure interfaces, input checks, and authorization; it should be adapted to the protocol and threat model.[^10][^20][^3]

## Define ambiguous cases

**MUST:** Specify how the parser handles trailing data, duplicate fields, unknown enum values, unsupported versions, malformed encoding, embedded null characters, and inconsistent length fields.

**SHOULD:** Reject ambiguous representations when different components could interpret them differently. Make parsing decisions explicit and test them rather than depending on defaults.[^2][^20]

For C++17 and later, `from_chars` offers a useful numeric-parsing interface; the application must still examine conversion failure and whether the intended input was fully consumed.[^5]

**MUST:** Never pass untrusted text as the format program, SQL structure, shell command structure, or other executable interpretation. Prefer fixed operation templates and separately supplied data; this is a recommended trust-boundary policy consistent with secure-by-design practice.[^7]

## Bound parser work

**MUST:** Establish limits on nesting, aggregate decoded size, collection counts, allocation volume, and total work—not merely the size of the initial input.

**SHOULD:** Prefer mature parsers and security libraries, but track their versions and vulnerabilities. Third-party code remains part of the application’s security exposure.[^2]

## Sources

[^2]: <https://learn.microsoft.com/en-us/cpp/code-quality/build-reliable-secure-programs?view=msvc-170>
[^3]: <https://github.com/isocpp/CppCoreGuidelines>
[^5]: <https://en.cppreference.com/cpp>
[^7]: <https://www.cisa.gov/sites/default/files/2023-10/Shifting-the-Balance-of-Cybersecurity-Risk-Principles-and-Approaches-for-Secure-by-Design-Software.pdf>
[^10]: <https://www.learn.microsoft.com/en-us/windows/desktop/SecBP/avoiding-buffer-overruns>
[^20]: <https://github.com/OWASP/CheatSheetSeries/blob/master/cheatsheets/Authorization_Cheat_Sheet.md>
