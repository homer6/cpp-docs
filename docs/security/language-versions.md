# Language versions and toolchains

> Language version, toolchain and libraries are separate security decisions; adopt newer facilities only for their verified guarantees.

> **Imported** from the research report *Security Practices for Modern C++* (2026-11-02, [#287](https://github.com/homer6/cpp-docs/issues/287)). It is being verified against primary sources and our compilers in [#288](https://github.com/homer6/cpp-docs/issues/288) (epic [#286](https://github.com/homer6/cpp-docs/issues/286)); until then, treat its claims as leads.

## Language version versus toolchain

**MUST:** Treat the language version, compiler version, standard-library version, linker, operating system, and dependencies as separate security decisions.

A project constrained to C++11 source compatibility can still use a substantially newer toolchain and its available protections; conversely, selecting a newer language mode does not establish that standard-library hardening or binary mitigations are enabled.[^13][^9]

**SHOULD:** Use a supported, patched toolchain even when the source-language baseline remains C++11. Verify features and protections in the actual build rather than inferring them from a marketing version or the `-std=` setting.[^2][^14]

## Version-specific security guidance

The baseline controls in this report apply to **all C++11-and-later code**. Newer features should replace riskier idioms only when their guarantees—and their limitations—are understood.[^3][^5]

| Language generation | Security-relevant facilities | Recommended use and limitation |
| :-- | :-- | :-- |
| C++11 | `unique_ptr`, `shared_ptr`, `array`, atomics, mutexes, initialization facilities | Establish RAII ownership and explicit synchronization; smart ownership does not establish the validity of every borrowed pointer or the lifetime of asynchronous users.[^5][^6] |
| C++14 | `make_unique`, improved lambda facilities | Prefer explicit ownership construction; review captured objects and escaping callbacks for lifetime safety.[^5][^3] |
| C++17 | `optional`, `variant`, `string_view`, `from_chars`, filesystem facilities | Represent state explicitly and parse deliberately; views remain non-owning, and filesystem convenience is not a security boundary.[^5][^15][^16] |
| C++20 | `span`, ranges, concepts, `jthread`, coroutines, integer-comparison utilities | Use bounded interfaces and more explicit constraints; do not mistake a view for ownership, concepts for runtime validation, or structured threading for automatic race freedom.[^5][^6][^17] |
| C++23 | `expected`, `mdspan`, additional range and utility facilities | Make recoverable failure explicit; review multidimensional extents, indexing, borrowed storage, and error-state access.[^5][^6] |
| C++26 and later | Contracts, standard-library hardening specifications, and other evolving facilities | Adopt only against verified implementation support and documented failure semantics; do not make optional contract evaluation the sole mechanism for required input validation.[^18][^19] |

**MUST:** Feature-gate newer facilities and test the fallback implementation. Library feature-test macros identify specific facilities more precisely than assuming that an entire standard is implemented uniformly.[^14]

**MUST:** Do not defer present-day controls while waiting for future language safety features. Production library hardening is already available in supported implementations independently of whether an application adopts C++26 syntax.[^9][^13]

## Sources

[^2]: <https://learn.microsoft.com/en-us/cpp/code-quality/build-reliable-secure-programs?view=msvc-170>
[^3]: <https://github.com/isocpp/CppCoreGuidelines>
[^5]: <https://en.cppreference.com/cpp>
[^6]: <https://cmu-sei.github.io/secure-coding-standards/sei-cert-cpp-coding-standard/rules/expressions-exp/exp54-cpp/>
[^9]: <https://github.com/ossf/wg-best-practices-os-developers/blob/main/docs/Compiler-Hardening-Guides/Compiler-Options-Hardening-Guide-for-C-and-C++.md>
[^13]: <https://learn.microsoft.com/en-us/cpp/overview/cpp-conformance-improvements?view=msvc-170>
[^14]: <https://cppreference.com/cpp/utility/feature_test>
[^15]: <https://en.cppreference.com/w/cpp/string/basic_string_view/basic_string_view.html>
[^16]: <https://www.man7.org/linux/man-pages//man2/openat2.2.html>
[^17]: <https://clang.llvm.org/docs//ThreadSanitizer.html>
[^18]: <https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2026/p4433r0.pdf>
[^19]: <https://cppreference.com/Template:cpp/utility/lib_feature_macros>
