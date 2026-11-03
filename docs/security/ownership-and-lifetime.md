# Ownership and lifetime

> Explicit ownership and reviewed borrowed lifetimes, so no resource is used after release or released twice.

> **Imported** from the research report *Security Practices for Modern C++* (2026-11-02, [#287](https://github.com/homer6/cpp-docs/issues/287)). It is being verified against primary sources and our compilers in [#290](https://github.com/homer6/cpp-docs/issues/290) (epic [#286](https://github.com/homer6/cpp-docs/issues/286)); until then, treat its claims as leads.

## Make ownership explicit

The C++ Core Guidelines emphasize resource management, memory management, and interfaces because these choices affect security-relevant architecture.[^3]

Adopt the following default policy:

| Situation | Preferred representation | Security rule |
| :-- | :-- | :-- |
| Ordinary owned state | Value member or standard owning container | Keep lifetime attached to an explicit owner.[^3] |
| Exclusive dynamic ownership | `std::unique_ptr<T>` | Transfer ownership explicitly; avoid unmanaged owning aliases.[^3] |
| Genuine shared ownership | `std::shared_ptr<T>` | Require a documented sharing need and review all non-owning aliases.[^3][^6] |
| Borrowed object | Reference or clearly documented non-owning pointer | State nullability, lifetime, and retention rules.[^3][^6] |
| Borrowed contiguous sequence | `std::span<T>` in C++20+, or a vetted earlier equivalent | Carry an extent, but still establish storage lifetime and valid access.[^5][^6] |
| OS or library resource | RAII wrapper with the correct release operation | Ensure exceptional paths release the resource.[^3] |

**MUST:** Avoid owning raw pointers in ordinary application code. Confine unavoidable allocation APIs, custom storage management, and C interoperability behind small audited wrappers.[^1][^3]

**MUST:** Match the allocation and release mechanism, including allocator identity and library-specific destruction requirements. Resource ownership should include sockets, file descriptors, locks, mapped memory, cryptographic contexts, and other handles—not just heap memory.[^3]

## RAII is necessary, not sufficient

RAII controls resource release, but a pointer, iterator, reference, or view can still outlive the object it refers to.[^6][^4]

**MUST:** Review every borrowed value that is returned, stored, captured, queued, or passed to an asynchronous operation. The review must establish who owns the underlying object and what prevents destruction or invalidation before the last use.[^6]

**SHOULD:** Prefer an owning return value when a caller needs independent lifetime. Avoid returning a view into a temporary, a local object, or mutable storage whose stability the interface does not guarantee.[^15][^6]

## Restrict manual lifetime management

**MUST:** Require specialist review for placement construction, custom allocators, memory pools, manual destruction, pointer tagging, object-representation tricks, and casts that reinterpret storage as objects.[^1][^6]

Do not accept “the address is still allocated” as a lifetime argument: the validity of an object depends on its lifetime, not solely on whether storage remains accessible.[^6]

## Sources

[^1]: <https://www.sei.cmu.edu/forms/secure-coding-cpp-form/>
[^3]: <https://github.com/isocpp/CppCoreGuidelines>
[^4]: <https://cmu-sei.github.io/secure-coding-standards/sei-cert-cpp-coding-standard/rules/containers-ctr/ctr51-cpp/>
[^5]: <https://en.cppreference.com/cpp>
[^6]: <https://cmu-sei.github.io/secure-coding-standards/sei-cert-cpp-coding-standard/rules/expressions-exp/exp54-cpp/>
[^15]: <https://en.cppreference.com/w/cpp/string/basic_string_view/basic_string_view.html>
