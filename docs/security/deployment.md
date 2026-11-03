# Deployment and incident readiness

> Least privilege, isolation of risky components, careful logging, and a process for vulnerabilities.

> **Imported** from the research report *Security Practices for Modern C++* (2026-11-02, [#287](https://github.com/homer6/cpp-docs/issues/287)). It is being verified against primary sources and our compilers in [#303](https://github.com/homer6/cpp-docs/issues/303) (epic [#286](https://github.com/homer6/cpp-docs/issues/286)); until then, treat its claims as leads.

**MUST:** Run with the minimum privileges needed and restrict access to files, secrets, devices, and network destinations according to the threat model.[^8][^7]

**SHOULD:** Isolate high-risk parsing and native-code components so that a detected memory-safety failure does not automatically compromise the most privileged process.[^7]

**MUST:** Avoid logging plaintext credentials or unnecessary sensitive input. Microsoft explicitly advises against exposing cleartext credentials in development and operational materials.[^2]

**SHOULD:** Protect diagnostic artifacts and make crashes useful for investigation without turning crash reports into an uncontrolled sensitive-data channel.[^2]

**MUST:** Define a vulnerability intake, triage, patch, and deployment process. A secure-development program must continue after the binary has shipped.[^48][^2]

## Sources

[^2]: <https://learn.microsoft.com/en-us/cpp/code-quality/build-reliable-secure-programs?view=msvc-170>
[^7]: <https://www.cisa.gov/sites/default/files/2023-10/Shifting-the-Balance-of-Cybersecurity-Risk-Principles-and-Approaches-for-Secure-by-Design-Software.pdf>
[^8]: <https://learn.microsoft.com/en-gb/windows/win32/dxtecharts/best-security-practices-in-game-development>
[^48]: <https://www.sei.cmu.edu/annual-reviews/2024-year-in-review/lasting-impact-the-cert-secure-coding-initiative/>
