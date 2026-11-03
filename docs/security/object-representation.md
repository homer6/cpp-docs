# Object state and representation

> Initialize before use, serialize explicitly across boundaries, and confine representation tricks.

> **Imported** from the research report *Security Practices for Modern C++* (2026-11-02, [#287](https://github.com/homer6/cpp-docs/issues/287)). It is being verified against primary sources and our compilers in [#293](https://github.com/homer6/cpp-docs/issues/293) (epic [#286](https://github.com/homer6/cpp-docs/issues/286)); until then, treat its claims as leads.

## Initialize before use

**MUST:** Explicitly initialize security-relevant state and never read indeterminate values. CERT identifies uninitialized-memory reads as a distinct coding-rule violation.[^24]

**SHOULD:** Design constructors or factories to establish valid states immediately; avoid objects that temporarily mean “authenticated, except some fields have not yet been initialized.” Explicit state representation aligns with the Core Guidelines’ interface and initialization principles.[^3]

**MUST:** Treat compiler-inserted automatic-variable initialization as defense in depth, not a replacement for correct initialization logic.[^9]

## Do not transmit native objects

**MUST:** Serialize fields explicitly when data crosses a trust boundary. Native class representations may contain padding that leaks information, even when named members have been initialized.[^25][^26]

Define wire-format sizes, byte order, encodings, permitted values, and version behavior independently of native object layout. A layout-compatible memory copy is not a complete validation or serialization policy.[^26][^1]

**MUST:** Restrict unchecked downcasts, aliasing tricks, and raw storage reinterpretation to audited low-level code. Validate dynamic type and object lifetime where those properties affect an operation’s validity.[^27][^6]

## Sources

[^1]: <https://www.sei.cmu.edu/forms/secure-coding-cpp-form/>
[^3]: <https://github.com/isocpp/CppCoreGuidelines>
[^6]: <https://cmu-sei.github.io/secure-coding-standards/sei-cert-cpp-coding-standard/rules/expressions-exp/exp54-cpp/>
[^9]: <https://github.com/ossf/wg-best-practices-os-developers/blob/main/docs/Compiler-Hardening-Guides/Compiler-Options-Hardening-Guide-for-C-and-C++.md>
[^24]: <https://cmu-sei.github.io/secure-coding-standards/sei-cert-cpp-coding-standard/rules/expressions-exp/exp53-cpp/>
[^25]: <https://cmu-sei.github.io/secure-coding-standards/sei-cert-c-coding-standard/rules/declarations-and-initialization-dcl/dcl39-c/>
[^26]: <https://cmu-sei.github.io/secure-coding-standards/sei-cert-cpp-coding-standard/rules/declarations-and-initialization-dcl/dcl55-cpp/>
[^27]: <https://clang.llvm.org//docs/ControlFlowIntegrity.html>
