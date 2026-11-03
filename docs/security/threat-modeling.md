# Threat models and trust boundaries

> What an attacker controls, what must stay protected, and how to keep the trusted core small.

> **Imported** from the research report *Security Practices for Modern C++* (2026-11-02, [#287](https://github.com/homer6/cpp-docs/issues/287)). It is being verified against primary sources and our compilers in [#289](https://github.com/homer6/cpp-docs/issues/289) (epic [#286](https://github.com/homer6/cpp-docs/issues/286)); until then, treat its claims as leads.

Before establishing coding rules, identify what an attacker can control and what must remain protected. Secure-development guidance treats security as a lifecycle responsibility, not simply a final code-review activity.[^7][^2]

For each component, require a short security design record covering:

- **Assets:** Secrets, credentials, privileged actions, sensitive records, executable code, and availability.
- **Entry points:** Network messages, files, IPC, command-line arguments, configuration, environment variables, plugins, and callbacks.
- **Trust transitions:** Remote-to-local, tenant-to-tenant, unprivileged-to-privileged, and untrusted-parser-to-authorized-operation.
- **Attacker capabilities:** Malformed input, replay, concurrent requests, resource exhaustion, dependency compromise, or modification of local directories.
- **Failure policy:** Which errors reject an operation, which close a connection, and which terminate an isolated worker.

These are proposed design-review requirements consistent with secure-by-design guidance; the precise threats must be determined for the actual product.[^7]

## Reduce the trusted core

**SHOULD:** Separate untrusted parsing from privileged actions, and expose narrow operations rather than unrestricted access to internal objects or operating-system facilities.[^8][^7]

For example, prefer a constrained operation such as “store this validated record under this authorized identifier” over handing a parser an unrestricted filesystem path and a privileged file handle. Authorization should follow application requirements, not merely whatever operations a chosen library makes convenient.[^20]

**MUST:** Authenticate and authorize independently of successful parsing. A syntactically valid request is not necessarily an authorized request.[^20]

## Sources

[^2]: <https://learn.microsoft.com/en-us/cpp/code-quality/build-reliable-secure-programs?view=msvc-170>
[^7]: <https://www.cisa.gov/sites/default/files/2023-10/Shifting-the-Balance-of-Cybersecurity-Risk-Principles-and-Approaches-for-Secure-by-Design-Software.pdf>
[^8]: <https://learn.microsoft.com/en-gb/windows/win32/dxtecharts/best-security-practices-in-game-development>
[^20]: <https://github.com/OWASP/CheatSheetSeries/blob/master/cheatsheets/Authorization_Cheat_Sheet.md>
