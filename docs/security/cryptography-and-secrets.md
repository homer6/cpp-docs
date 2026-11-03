# Cryptography and secrets

> Use maintained cryptographic libraries, a real cryptographic random source, and careful secret handling.

> **Imported** from the research report *Security Practices for Modern C++* (2026-11-02, [#287](https://github.com/homer6/cpp-docs/issues/287)). It is being verified against primary sources and our compilers in [#297](https://github.com/homer6/cpp-docs/issues/297) (epic [#286](https://github.com/homer6/cpp-docs/issues/286)); until then, treat its claims as leads.

## Use dedicated security libraries

**MUST:** Use maintained cryptographic and TLS implementations rather than custom algorithms or ad hoc protocols. Dedicated libraries provide supported encryption, authentication, password-hashing, and signature facilities.[^32][^33]

**SHOULD:** Prefer high-level operations with fewer security-sensitive choices, and document protocol selection, key management, nonce rules, and error behavior.[^32][^7]

**MUST:** Never substitute ordinary simulation-oriented random engines for a documented cryptographic random source. Even `std::random_device` has implementation-specific source selection and behavior; verify the chosen security API’s guarantees on every supported platform.[^34][^35]

## Protect secret handling

**MUST:** Keep credentials and signing keys out of source code, ordinary configuration committed to version control, build logs, and unsecured shared documents.[^2]

**SHOULD:** Minimize plaintext lifetime and unnecessary copies, use dedicated secret-handling facilities, and apply supported memory-locking and erasure operations where required by the threat model.[^36][^37]

**MUST:** Do not claim that erasing one buffer proves removal of all copies. Cryptographic-library documentation explicitly recognizes that copies may exist outside the library’s own boundaries.[^38]

**SHOULD:** Use library-provided constant-time comparison for secret authentication values rather than ordinary comparison routines.[^38]

## Configure TLS explicitly

**MUST:** Validate peer identity and certificate trust according to the intended connection model; encryption without the intended authentication does not establish the correct peer.[^33]

**SHOULD:** Default to TLS 1.3, retain TLS 1.2 only for justified compatibility, and disable deprecated protocol versions.[^33]

**MUST:** Treat “temporarily disable certificate verification” as an unacceptable production workaround.

## Sources

[^2]: <https://learn.microsoft.com/en-us/cpp/code-quality/build-reliable-secure-programs?view=msvc-170>
[^7]: <https://www.cisa.gov/sites/default/files/2023-10/Shifting-the-Balance-of-Cybersecurity-Risk-Principles-and-Approaches-for-Secure-by-Design-Software.pdf>
[^32]: <https://doc.libsodium.org/doc>
[^33]: <https://cheatsheetseries.owasp.org/cheatsheets/Transport_Layer_Security_Cheat_Sheet.html>
[^34]: <https://en.cppreference.com/cpp/named_req/RandomNumberEngine>
[^35]: <https://en.cppreference.com/cpp/numeric/random/random_device/random_device>
[^36]: <https://doc.libsodium.org/password_hashing/default_phf>
[^37]: <https://doc.libsodium.org/advanced/scrypt>
[^38]: <http://doc.libsodium.org/doc/internals>
