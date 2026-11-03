# Bounds and containers

> Carry extents with the data, check attacker-influenced indices before access, and respect invalidation.

> **Imported** from the research report *Security Practices for Modern C++* (2026-11-02, [#287](https://github.com/homer6/cpp-docs/issues/287)). It is being verified against primary sources and our compilers in [#291](https://github.com/homer6/cpp-docs/issues/291) (epic [#286](https://github.com/homer6/cpp-docs/issues/286)); until then, treat its claims as leads.

## Carry the extent

**MUST:** Prefer interfaces that carry sequence length with the data rather than unrelated pointer-and-count parameters. Higher-level interfaces reduce the amount of manually coordinated state that must remain correct.[^3]

For C++20 and later, `std::span` is useful vocabulary for borrowed contiguous storage, but its presence is not proof of valid construction, valid indexing, or adequate lifetime.[^5][^6]

**MUST:** Validate the original pointer/extent relationship before constructing an abstraction around it. Wrapping invalid input does not repair it.

## Check before access

**MUST:** Validate all attacker-influenced indices, offsets, lengths, dimensions, and iterator ranges before they reach an operation with a precondition. Invalid container references and ranges can produce undefined behavior.[^4][^1]

**SHOULD:** Prefer checked access at trust boundaries, then use unchecked operations only where the bounds argument is clear and local. Standard-library hardening adds useful checks but does not cover every operation or every form of invalidation.[^21][^9]

**MUST:** Keep required validation active in release builds. Do not implement it solely through debug assertions or optionally evaluated contracts.[^18]

## Respect invalidation

A `std::vector` reallocation invalidates outstanding pointers, references, and iterators into its elements; other container mutations have their own invalidation rules.[^4]

**MUST:** Review every mutation that occurs while borrowed elements or iterators exist. This includes mutation through another alias and mutation performed by a callback.[^4]

**MUST:** Distinguish allocated capacity from constructed elements. An allocation strategy must not become an excuse to access elements outside the valid container range.[^1][^4]

## Sources

[^1]: <https://www.sei.cmu.edu/forms/secure-coding-cpp-form/>
[^3]: <https://github.com/isocpp/CppCoreGuidelines>
[^4]: <https://cmu-sei.github.io/secure-coding-standards/sei-cert-cpp-coding-standard/rules/containers-ctr/ctr51-cpp/>
[^5]: <https://en.cppreference.com/cpp>
[^6]: <https://cmu-sei.github.io/secure-coding-standards/sei-cert-cpp-coding-standard/rules/expressions-exp/exp54-cpp/>
[^9]: <https://github.com/ossf/wg-best-practices-os-developers/blob/main/docs/Compiler-Hardening-Guides/Compiler-Options-Hardening-Guide-for-C-and-C++.md>
[^18]: <https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2026/p4433r0.pdf>
[^21]: <https://queue.acm.org/detail.cfm?id=3773097>
