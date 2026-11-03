# Supply chain and release security

> Control build inputs, verify provenance, and keep the evidence for every release.

> **Imported** from the research report *Security Practices for Modern C++* (2026-11-02, [#287](https://github.com/homer6/cpp-docs/issues/287)). It is being verified against primary sources and our compilers in [#302](https://github.com/homer6/cpp-docs/issues/302) (epic [#286](https://github.com/homer6/cpp-docs/issues/286)); until then, treat its claims as leads.

## Treat build inputs as trusted code

**MUST:** Inventory and control compilers, linkers, generators, package recipes, build plugins, dependencies, and downloaded source or binary artifacts. Secure C++ guidance includes tracking external dependencies and resolving applicable vulnerabilities.[^2]

**SHOULD:** Pin reviewed inputs, verify artifact identity, and maintain controlled update procedures. Build provenance is useful only when the consumer verifies the artifact digest, trusted builder, and expected build parameters.[^49][^11]

**MUST:** Separate untrusted contribution builds from release-signing authority. SLSA’s stronger build requirements protect signing material and provenance generation from user-controlled build steps.[^49]

## Preserve release evidence

For each production release, retain:

- Source revision and dependency inventory
- Compiler, library, linker, and build configuration
- Effective security options
- Analysis and security-test results
- Fuzzing regressions and relevant coverage evidence
- Approved exceptions and their expiration conditions
- Artifact digests, signatures, and provenance
- Patch ownership and supported-version information

This is a recommended evidence package combining secure C++ verification with provenance-based release assurance.[^11][^2]

**MUST:** Do not treat an SBOM, a signature, or reproducibility as proof that an artifact is vulnerability-free. Provenance establishes information about production and identity; vulnerability assessment remains a separate responsibility.[^11][^2]

## Sources

[^2]: <https://learn.microsoft.com/en-us/cpp/code-quality/build-reliable-secure-programs?view=msvc-170>
[^11]: <https://slsa.dev/spec/v1.0/zonepage>
[^49]: <https://slsa.dev/spec/v1.0/requirements>
