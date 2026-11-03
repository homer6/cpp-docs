# Compiler and library hardening

> Production hardening profiles per platform and standard library, checked in the final artifact.

> **Imported** from the research report *Security Practices for Modern C++* (2026-11-02, [#287](https://github.com/homer6/cpp-docs/issues/287)). It is being verified against primary sources and our compilers in [#300](https://github.com/homer6/cpp-docs/issues/300) (epic [#286](https://github.com/homer6/cpp-docs/issues/286)); until then, treat its claims as leads.

## Use production-qualified protections

OpenSSF recommends compiler diagnostics, binary mitigations, and standard-library precondition checking as complementary controls for C and C++.[^9]

**MUST:** Maintain approved build profiles for each supported platform and inspect the final artifact. A flag present in a configuration file is not sufficient evidence that every component received the intended protection.[^9][^2]

## Standard-library hardening

| Implementation | Production policy | Important qualification |
| :-- | :-- | :-- |
| GCC libstdc++ | Enable `_GLIBCXX_ASSERTIONS` where supported | Adds precondition checks for many library operations; they don't cover every operation, so keep the source-level checks too.[^9] |
| LLVM libc++ | Enable an appropriate hardening mode; fast is a practical baseline | Extensive mode adds checks; debug mode has a different intended use and cost profile.[^9][^21] |
| Microsoft STL | Enable `_MSVC_STL_HARDENING=1` on supported versions | Microsoft documents project-wide enablement; verify coverage in the selected implementation.[^13] |
| libstdc++ debug mode | Use as an additional testing configuration | `_GLIBCXX_DEBUG` changes container sizes and behavior and imposes compatibility constraints across translation units.[^40] |

**MUST:** Select hardening by the standard library actually in use—not merely by the compiler executable’s name.[^9]

**MUST:** Do not confuse a production hardening mode with a debug-container ABI. Mixing incompatible configurations can itself invalidate program assumptions.[^40]

## GNU/Linux example

On a supported GCC GNU/Linux toolchain that provides `-fhardened`, an illustrative production command is:

```sh
g++ -std=c++11 -O2 -fhardened \
    -Wall -Wextra -Wconversion -Wsign-conversion \
    -Wformat=2 \
    app.cpp -o app
```

`-fhardened` is intended for production and enables a platform- and compiler-version-dependent set of protections; the documented set includes fortification, library assertions, automatic-variable initialization, position independence, linker protections, and stack protections.[^41]

**MUST:** Inspect the selected compiler’s documented expansion and effective options. Do not assume the same flag exists in Clang or produces an identical configuration across GCC releases.[^41][^9]

For toolchains without that option, assemble and validate an explicit platform profile using vendor and OpenSSF guidance rather than copying an unqualified cross-platform flag list.[^9]

## Windows example

An illustrative supported-MSVC profile is:

```bat
cl /std:c++20 /O2 /W4 /GS /sdl /guard:cf app.cpp ^
  /link /DYNAMICBASE /HIGHENTROPYVA /NXCOMPAT /guard:cf
```

Microsoft documents stack protection, additional security checks, control-flow guarding, ASLR, and DEP compatibility; high-entropy address randomization additionally depends on the relevant platform and ASLR configuration.[^42][^10]

**SHOULD:** Evaluate `/CETCOMPAT` where supported and appropriate, and run `/analyze` in a dedicated verification configuration.[^12][^10]

These commands are **illustrative starting points**, not verified builds of a particular application.

## Evaluate stronger mitigations

**SHOULD:** Evaluate control-flow integrity, hardware-assisted protections, and platform memory-error mitigations for higher-risk components. Clang’s CFI modes have linkage, visibility, target, and shared-library considerations that must be tested rather than assumed away.[^43][^44][^27]

**MUST:** Preserve source-level prevention: exploit mitigations reduce risk but do not justify retaining known memory defects.[^12][^7]

## Sources

[^2]: <https://learn.microsoft.com/en-us/cpp/code-quality/build-reliable-secure-programs?view=msvc-170>
[^7]: <https://www.cisa.gov/sites/default/files/2023-10/Shifting-the-Balance-of-Cybersecurity-Risk-Principles-and-Approaches-for-Secure-by-Design-Software.pdf>
[^9]: <https://github.com/ossf/wg-best-practices-os-developers/blob/main/docs/Compiler-Hardening-Guides/Compiler-Options-Hardening-Guide-for-C-and-C++.md>
[^10]: <https://www.learn.microsoft.com/en-us/windows/desktop/SecBP/avoiding-buffer-overruns>
[^12]: <https://learn.microsoft.com/hr-hr/cpp/security/security-best-practices-for-cpp?view=msvc-150>
[^13]: <https://learn.microsoft.com/en-us/cpp/overview/cpp-conformance-improvements?view=msvc-170>
[^21]: <https://queue.acm.org/detail.cfm?id=3773097>
[^27]: <https://clang.llvm.org//docs/ControlFlowIntegrity.html>
[^40]: <https://gcc.gnu.org/onlinedocs/libstdc++/manual/debug_mode_using.html>
[^41]: <https://www.mankier.com/1/gcc>
[^42]: <http://www.learn.microsoft.com/en-us/cpp/build/reference/dynamicbase-use-address-space-layout-randomization>
[^43]: <https://clang.llvm.org/docs//HardwareAssistedAddressSanitizer.html>
[^44]: <https://clang.llvm.org/docs/ShadowCallStack.html>
