# Concurrency and asynchronous code

> One synchronization rule per shared object, and a lifetime argument for every queued or suspended operation.

> **Imported** from the research report *Security Practices for Modern C++* (2026-11-02, [#287](https://github.com/homer6/cpp-docs/issues/287)). It is being verified against primary sources and our compilers in [#295](https://github.com/homer6/cpp-docs/issues/295) (epic [#286](https://github.com/homer6/cpp-docs/issues/286)); until then, treat its claims as leads.

Concurrency defects can affect confidentiality, integrity, and availability; CERT specifically documents security-relevant races, and ThreadSanitizer exists to detect data races during execution.[^28][^17]

## Protect shared state consistently

**MUST:** Define the synchronization rule for every shared mutable object. Use one coherent locking or atomic protocol rather than locally plausible but incompatible approaches.[^28][^3]

**MUST:** Do not treat `shared_ptr` ownership as a synchronization policy for the pointed-to object. Lifetime management and access synchronization require separate reasoning.[^6][^3]

**SHOULD:** Prefer simple locking and message-passing designs over custom lock-free algorithms unless the latter have a documented security need and specialist review.[^3]

## Review asynchronous lifetimes

**MUST:** Establish lifetime and cancellation behavior for queued callbacks, timers, completion handlers, futures, and coroutine frames. A callback must not access an object after its lifetime ends.[^6]

Review these questions explicitly:

- Can the operation outlive its initiating object?
- Can cancellation race with completion?
- Can a callback run during destruction?
- Does a captured pointer merely preserve an address, or does something preserve the object?
- Can shutdown wait indefinitely while holding a lock required by completion?

These are proposed review questions derived from lifetime and concurrency hazards.[^6][^3]

## Apply coroutine-specific rules

For C++20 and later, **SHOULD:** Apply dedicated coroutine checks, including avoiding unsafe capturing coroutine lambdas, avoiding locks held across suspension, and scrutinizing reference parameters whose referents may disappear before resumption.[^29]

**MUST:** Do not assume that passing an ordinary synchronous lifetime review is sufficient for a function that can suspend.[^29][^6]

## Sources

[^3]: <https://github.com/isocpp/CppCoreGuidelines>
[^6]: <https://cmu-sei.github.io/secure-coding-standards/sei-cert-cpp-coding-standard/rules/expressions-exp/exp54-cpp/>
[^17]: <https://clang.llvm.org/docs//ThreadSanitizer.html>
[^28]: <https://cmu-sei.github.io/secure-coding-standards/sei-cert-cpp-coding-standard/rules/concurrency-con/con52-cpp/>
[^29]: <https://help.klocwork.com/current/en-us/concepts/cppcoreguidelinesmappedtoklocworkcheckers.htm>
