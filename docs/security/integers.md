# Integer and size calculations

> Define numeric domains and check before arithmetic, so untrusted sizes can't wrap, narrow or exhaust.

> **Imported** from the research report *Security Practices for Modern C++* (2026-11-02, [#287](https://github.com/homer6/cpp-docs/issues/287)). It is being verified against primary sources and our compilers in [#292](https://github.com/homer6/cpp-docs/issues/292) (epic [#286](https://github.com/homer6/cpp-docs/issues/286)); until then, treat its claims as leads.

Integer errors are especially dangerous when they influence allocation sizes, offsets, indexing, or copy lengths; unsigned wrap can produce an allocation smaller than the subsequent operation expects.[^22][^23]

## Establish numeric domains

**MUST:** Define valid ranges and units for security-relevant quantities: bytes, elements, characters, records, milliseconds, identifiers, and protocol fields.

**MUST:** Check representability before narrowing or changing signedness. A cast is not a validation mechanism; checked integer utilities or explicit range tests should establish the conversion’s validity.[^5][^1]

**MUST:** Treat attacker-controlled lengths as untrusted even when their type is `std::size_t`. The type can represent a size; it does not establish that the size is valid or affordable.[^23][^22]

## Prevent overflow before arithmetic

For a requested slice, prefer a subtraction-based check:

```cpp
// C++11 and later.
#include <cstddef>

bool valid_slice(std::size_t total,
                 std::size_t offset,
                 std::size_t length) noexcept
{
    return offset <= total && length <= total - offset;
}
```

This original example avoids calculating `offset + length` before validity has been established; preventing wrap before size-dependent operations follows CERT’s integer guidance.[^23]

For an allocation calculation constrained by a trusted byte budget:

```cpp
// C++11 and later.
// Policy: zero-sized elements are rejected.
// "bytes" is unchanged on failure.
#include <cstddef>

bool checked_allocation_size(std::size_t count,
                             std::size_t element_size,
                             std::size_t byte_budget,
                             std::size_t& bytes) noexcept
{
    if (element_size == 0 ||
        count > byte_budget / element_size) {
        return false;
    }

    bytes = count * element_size;
    return true;
}
```

The budget is an **application policy limit**, not merely the machine’s maximum representable size. Correct allocation arithmetic prevents undersized allocations, while a separate resource policy prevents otherwise valid requests from exhausting the service.[^22][^23][^7]

**MUST:** Review arithmetic inside validation itself, including multiplication, rounding, alignment, subtraction, shifts, and conversion from wire-format integers.[^23][^1]

## Sources

[^1]: <https://www.sei.cmu.edu/forms/secure-coding-cpp-form/>
[^5]: <https://en.cppreference.com/cpp>
[^7]: <https://www.cisa.gov/sites/default/files/2023-10/Shifting-the-Balance-of-Cybersecurity-Risk-Principles-and-Approaches-for-Secure-by-Design-Software.pdf>
[^22]: <https://github.com/cmu-sei/secure-coding-standards/blob/main/content/4.sei-cert-c-coding-standard/03.rules/13.memory-management-mem/6.mem35-c.md>
[^23]: <https://github.com/cmu-sei/secure-coding-standards/blob/main/content/4.sei-cert-c-coding-standard/03.rules/12.integers-int/2.int30-c.md>
