# Integer and size calculations

> Treat every size, offset, length and count from input as untrusted, define its valid range, and check before the arithmetic, so it can't wrap, overflow, narrow or change sign on the way to an allocation or an index.

## Mental model

Most integer vulnerabilities are a bounds bug one step removed: a length from
a packet is multiplied by an element size, the product wraps, a buffer of the
wrapped (small) size is allocated, and the copy that follows uses the
original (large) length. The arithmetic is where the attacker's number becomes
a wrong number:

```
count (from input) × sizeof(T) ──wraps──▶ small allocation
copy count elements ──────────────────────▶ out-of-bounds write
```

In C++, **unsigned** arithmetic wraps silently (well defined, still wrong);
**signed** overflow is undefined behaviour, which the optimizer may assume
never happens; a **conversion** to a narrower or differently-signed type keeps
only part of the value; and comparing signed with unsigned converts the
signed side first. The Core Guidelines cover the general rules (**ES.46**,
**ES.100**–**ES.107**); this page is about values an attacker chooses.

## Define the numeric domain

**MUST:** For every security-relevant quantity (bytes, elements, characters,
records, milliseconds, identifiers, protocol fields), say what range is valid
and what unit it's in, and check input against that range, not against what
the type can hold. A `std::size_t` can represent a 16-exabyte length; that
doesn't make one acceptable.

**MUST:** Check that a value fits before narrowing it or changing its
signedness. A cast is not a check (**ES.46**, **ES.49**; CERT
[INT31-C](https://cmu-sei.github.io/secure-coding-standards/sei-cert-c-coding-standard/rules/integers-int/int31-c)).
C++20's `std::in_range` and `std::cmp_less` and friends compare **values**,
whatever the types:

```cpp run
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <utility>

int main() {
    std::int64_t from_wire = -1;
    assert(!std::in_range<std::size_t>(from_wire));   // a negative size is rejected
    assert(!std::in_range<std::uint16_t>(70000));     // doesn't fit a 16-bit field
    assert(std::cmp_less(from_wire, 0u));             // -1 < 0u, as values
    assert(!(static_cast<std::size_t>(from_wire) < 10u));   // the built-in conversion: SIZE_MAX
}
```

The same applies to enumerations: a value from input must be checked against
the enumerators before it's cast to the enum (CERT
[INT50-CPP](https://cmu-sei.github.io/secure-coding-standards/sei-cert-cpp-coding-standard/rules/integers-int/int50-cpp)).

## Check before the arithmetic

**MUST:** Make overflow and wrap impossible before computing, not after
(**ES.103**, **ES.104**; CERT
[INT30-C](https://cmu-sei.github.io/secure-coding-standards/sei-cert-c-coding-standard/rules/integers-int/int30-c),
[INT32-C](https://cmu-sei.github.io/secure-coding-standards/sei-cert-c-coding-standard/rules/integers-int/int32-c)).
For a slice of a buffer, compare with a subtraction that can't wrap instead of
adding:

```cpp
#include <cstddef>

// true if [offset, offset + length) lies inside [0, total)
constexpr bool valid_slice(std::size_t total, std::size_t offset, std::size_t length) noexcept {
    return offset <= total && length <= total - offset;   // never computes offset + length
}

static_assert(valid_slice(10, 4, 6));
static_assert(!valid_slice(10, 4, 7));
static_assert(!valid_slice(10, 4, static_cast<std::size_t>(-1)));   // offset + length would wrap to 3
```

For an allocation, check the multiplication against a **budget**, which is
an application limit (how much one request may allocate), not the largest
value the type can hold (CERT
[MEM35-C](https://cmu-sei.github.io/secure-coding-standards/sei-cert-c-coding-standard/rules/memory-management-mem/mem35-c)):

```cpp
#include <cstddef>
#include <optional>

// The byte size for `count` elements of `element_size`, if it fits the budget.
constexpr std::optional<std::size_t>
checked_allocation_size(std::size_t count, std::size_t element_size, std::size_t budget) noexcept {
    if (element_size == 0 || count > budget / element_size) return std::nullopt;
    return count * element_size;   // can't exceed budget, so can't wrap
}

static_assert(checked_allocation_size(1000, 4, 4096) == 4000);
static_assert(!checked_allocation_size(1025, 4, 4096));
static_assert(!checked_allocation_size(static_cast<std::size_t>(-1) / 2, 4, static_cast<std::size_t>(-1)));
```

Where you need the result and an overflow flag, use a checked operation
instead of writing the test by hand:

- **`<stdckdint.h>`** (`ckd_add`, `ckd_sub`, `ckd_mul`): C23, and added to the
  C++26 draft by P3370R1. Gate on the feature-test macro
  `__cpp_lib_stdckdint_h`, which libstdc++ defines (202603L) from 14.4, 15.3
  and 16. Don't gate on `__has_include(<stdckdint.h>)`: GCC 14.2's libstdc++
  has no C++ header, so the include finds GCC's C one, whose `ckd_add` casts
  to `_Bool` and doesn't compile as C++. Clang 22's own header works in C++,
  but libc++ doesn't define the macro yet, so the fallback runs there. The MS
  STL doesn't ship the header yet.[^ckd]
- **`__builtin_add_overflow`** and friends on GCC and Clang (not MSVC).
- **Saturation** (`std::saturating_add` and friends, C++26) when clamping is the
  right answer rather than rejection; see [saturation](../examples/numerics/saturation.md)
  for the names and toolchain support.

```cpp
#include <cstddef>
#include <version>
#if defined(__cpp_lib_stdckdint_h)
#  include <stdckdint.h>
#endif

bool total_length(std::size_t a, std::size_t b, std::size_t& out) {
#if defined(__cpp_lib_stdckdint_h)
    return !ckd_add(&out, a, b);              // ckd_add returns true on overflow
#else
    if (b > static_cast<std::size_t>(-1) - a) return false;
    out = a + b;
    return true;
#endif
}
```

**MUST:** Review the arithmetic inside the validation itself, including
rounding up to an alignment, subtraction of a header size, shifts (a shift by
the type's width or more is undefined; CERT
[INT34-C](https://cmu-sei.github.io/secure-coding-standards/sei-cert-c-coding-standard/rules/integers-int/int34-c)),
division by a value from input (**ES.105**; CERT
[INT33-C](https://cmu-sei.github.io/secure-coding-standards/sei-cert-c-coding-standard/rules/integers-int/int33-c)),
and converting wire-format integers (endianness, signedness, width).

## Signed overflow is undefined, so the check must come first

A check written as "compute, then see if it overflowed" doesn't work for
signed types: the overflow has already happened, and the compiler may remove
the check because UB "can't happen". UBSan reports the overflow at run time:

```cpp bad run
#include <climits>

int add_lengths(int a, int b) {
    int sum = a + b;            // BAD: overflow is UB when a + b > INT_MAX
    if (sum < a) return -1;     // too late, and the compiler may delete this
    return sum;
}

int main(int argc, char**) {
    return add_lengths(INT_MAX, argc) > 0 ? 0 : 1;   // UBSan: signed integer overflow
}
```

**ES.102** recommends signed types for arithmetic and **ES.106** warns against
using `unsigned` to "prevent" negative values: an unsigned type doesn't reject
`-1`, it turns it into a huge number. Either way, the range check is what
makes it safe.

## See also

- Core Guidelines: [ES.46, ES.100–ES.107](../guidelines/core-guidelines/expr.md)
- [Integer comparison and `in_range`](../examples/utilities/utility.md) ·
  [saturation arithmetic](../examples/numerics/saturation.md) ·
  [numeric limits](../examples/language-support/numeric-limits.md) ·
  [conversions](../examples/language/expressions/conversions.md)
- [Bounds](bounds.md) (where the checked size is used) ·
  [resource limits](resource-limits.md) (budgets)
- cppreference: [`std::in_range`](https://en.cppreference.com/w/cpp/utility/in_range),
  [`ckd_add`](https://en.cppreference.com/w/cpp/numeric/ckd_add)
- Tests: [`tests/security/integers_test.cpp`](../../tests/security/integers_test.cpp)

[^ckd]: [P3370R1](https://wg21.link/p3370r1), *Add new library headers from C23*; cppreference [`<stdckdint.h>`](https://en.cppreference.com/w/cpp/header/stdckdint.h); MS STL [issue 5143](https://github.com/microsoft/STL/issues/5143). Checked: libstdc++ 14.4.0, 15.3.0 and 16.2.0 define `__cpp_lib_stdckdint_h` (202603L) in `<version>` and ship a C++ `<stdckdint.h>`; Homebrew GCC 15.2.0 ships the header without the macro; Clang 22.1 with libc++ uses Clang's own header, which works in C++, and doesn't define the macro. GCC 14.2.0 on FreeBSD 14.5 and 15.1 (CI, [#369](https://github.com/homer6/cpp-docs/issues/369)) found GCC's C-only header through `__has_include` and failed with `'_Bool' was not declared in this scope`.
