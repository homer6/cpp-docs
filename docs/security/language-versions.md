# Language versions and toolchains

> The language version, the compiler, the standard library and the build flags are separate security decisions: check each one in the build you ship.

## Mental model

`-std=c++26` chooses the language rules. It doesn't choose your compiler's
version, your standard library or its hardening mode, your warnings, or your
binary protections, and those decide as much about security as the language
version does:

| Decision | Set by | Example |
|---|---|---|
| Language version | `-std=`, `/std:` | `-std=c++20` |
| Compiler and its version | the toolchain you install | GCC 15.2, Clang 22, MSVC 19.44 |
| Standard library and its hardening | the library, plus a macro | libstdc++ `_GLIBCXX_ASSERTIONS`, libc++ `_LIBCPP_HARDENING_MODE`, MS STL `_MSVC_STL_HARDENING` |
| Binary protections | compiler and linker flags | `-fstack-protector-strong`, `/guard:cf` (see [hardening](hardening.md)) |
| Dependencies | your package manager or vendored sources | see [supply chain](supply-chain.md) |

## Language version versus toolchain

**MUST:** Treat the language version, compiler version, standard-library
version, linker, operating system and dependencies as separate decisions, and
record each one for a release.

**SHOULD:** Use a supported, patched toolchain even when the code must stay
compatible with an older standard. A C++17 code base built with GCC 15 still
gets GCC 15's diagnostics, library checks and protections.

**MUST:** Check protections in the actual build; never infer them from the
`-std=` setting. A newer language mode turns on no hardening by itself. With
GCC 15 and 16, libstdc++ defines `_GLIBCXX_ASSERTIONS` by default only in
**unoptimized** builds, so the same code built with `-O2` has no library
checks unless you ask for them:[^gcc15]

```bash
g++-15 -std=c++26 -O0 -dM -E -include vector -x c++ /dev/null | grep _GLIBCXX_ASSERTIONS   # defined
g++-15 -std=c++26 -O2 -dM -E -include vector -x c++ /dev/null | grep _GLIBCXX_ASSERTIONS   # nothing
```

The default lives in libstdc++'s own `<bits/c++config.h>` (`! defined(__OPTIMIZE__)`),
so Clang building against libstdc++ 15 or later behaves the same way;
`-D_GLIBCXX_NO_ASSERTIONS` turns it off. libc++ leaves the default to whoever
built it (Homebrew's LLVM 22 ships with hardening off), and the MS STL's
hardening is off by default.[^libcxx][^msstl]

## What each standard adds

The rules on these pages apply to all C++11-and-later code. Newer facilities
replace riskier idioms, but only for the guarantee they actually give. Gate
each one on its **feature-test macro** (the value is the version that
introduced it; an implementation may define a higher one):

| Standard | Facility (feature-test macro) | What it gives, and what it doesn't |
|---|---|---|
| C++11 | `unique_ptr`, `shared_ptr`, `array`, `atomic`, `mutex` | RAII ownership and explicit synchronization. Doesn't make a borrowed pointer valid, or keep an object alive for an asynchronous user. |
| C++14 | `make_unique` (`__cpp_lib_make_unique` 201304L) | Ownership from the start, no naked `new`. Captured objects and escaping callbacks still need a lifetime argument. |
| C++17 | `optional`, `variant`, `string_view` (all 201606L), `from_chars` (`__cpp_lib_to_chars` 201611L), `filesystem` (201703L) | State made explicit and numeric parsing without locale or exceptions. Views don't own; `filesystem` is not a security boundary (see [filesystem and processes](filesystem-and-processes.md)). |
| C++20 | `span` (202002L), ranges (`__cpp_lib_ranges` 201911L), concepts (`__cpp_concepts` 201907L), `jthread` (201911L), coroutines (`__cpp_impl_coroutine` 201902L), `std::cmp_less` and friends (`__cpp_lib_integer_comparison_functions` 202002L) | Interfaces that carry their extent, constraints checked at compile time, comparisons that don't convert signedness. A view isn't ownership, a concept isn't run-time validation, and `jthread` doesn't prevent races. |
| C++23 | `expected` (202202L), `mdspan` (202207L) | Recoverable failure in the type. Callers must still check it; `mdspan` indexing still needs valid extents. |
| C++26 (draft) | contracts (`__cpp_contracts` 202502L), standard-library hardening (`__cpp_lib_hardened_array`, `__cpp_lib_hardened_vector` 202502L) | See [below](#c26-contracts-and-library-hardening). |

Integer comparison is the smallest example of why the version matters: before
C++20, comparing a signed and an unsigned value converts the signed one, so a
negative length passes a size check.

```cpp run
#include <cassert>
#include <cstddef>
#include <utility>

int main() {
    int requested = -1;                 // attacker-controlled
    std::size_t limit = 4096;
    // The built-in comparison converts -1 to SIZE_MAX first:
    assert(!(static_cast<std::size_t>(requested) <= limit));
    // C++20 compares the values, so the negative length is caught:
    assert(std::cmp_less(requested, 0));
    assert(std::cmp_less_equal(requested, limit));   // true: -1 <= 4096
    assert(!std::in_range<std::size_t>(requested));  // and it isn't a size
}
```

## C++26: contracts and library hardening

The C++26 draft adds contract assertions (P2900) and standard-library
hardening (P3471), which turns many library preconditions, such as
`operator[]` on `vector`, `span`, `string` and `array`, or dereferencing an
empty `optional`, into checked contract assertions. Their status is
**unsettled**: the C++26 DIS ballot closed on 22 October 2026 with, for the
first time in C++'s history, some national bodies voting against it, mainly
over contracts. The official result wasn't public when this page was written;
WG21 takes it up in Búzios on 16–21 November 2026.[^p4433]
Library hardening is specified on top of contracts, so a change to one affects
the other.

What you can use today, independent of `-std=`:

| Library | Switch | Default |
|---|---|---|
| libstdc++ (GCC) | `-D_GLIBCXX_ASSERTIONS` | on in unoptimized builds since GCC 15, off with optimization[^gcc15] |
| libc++ (LLVM) | `-D_LIBCPP_HARDENING_MODE=_LIBCPP_HARDENING_MODE_FAST` (or `_EXTENSIVE`, `_DEBUG`) | chosen by whoever built the library[^libcxx] |
| MS STL | `/D_MSVC_STL_HARDENING=1` (VS 2022 17.14 and later) | off[^msstl] |

None of the libraries we test (libstdc++ 15 and 16, libc++ 22) defines the
C++26 `__cpp_lib_hardened_*` macros yet, so a program can't detect hardening
through them; GCC 16 does define `__cpp_contracts` (see
[contracts](../examples/language/basic-concepts/contracts.md)).

**MUST:** Don't make a contract assertion the only check on untrusted input.
An assertion can be evaluated with the *ignore* semantic, and then it doesn't
run. Validate input with ordinary code that always runs, and use contracts to
catch bugs.

**MUST:** Don't wait for C++26 to harden. The library switches above work in
any language mode now; see [hardening](hardening.md).

## Gate newer facilities, and test the fallback

**MUST:** Feature-gate a newer facility on its own macro, not on
`__cplusplus`: libraries implement a standard piece by piece. In `-std=c++26`
mode, libstdc++ 15 doesn't define `__cpp_lib_mdspan`, and libc++ 22 doesn't
define `__cpp_lib_to_chars` even though its integer `from_chars` works; the
macro covers the whole facility, so check what your code actually uses. Build
and test the fallback path too, since it's the one your oldest supported
toolchain runs.

```cpp
#include <cstddef>
#include <version>
#if defined(__cpp_lib_span) && __cpp_lib_span >= 202002L
#  include <span>
std::size_t checksum(std::span<const unsigned char> data) {
    std::size_t sum = 0;
    for (unsigned char b : data) sum += b;
    return sum;
}
#else
std::size_t checksum(const unsigned char* data, std::size_t size) {   // fallback: keep the pair together
    std::size_t sum = 0;
    for (std::size_t i = 0; i < size; ++i) sum += data[i];
    return sum;
}
#endif
```

## See also

- [Feature testing](../examples/feature-test-macros/README.md) and
  cppreference's [feature-test macro list](https://en.cppreference.com/w/cpp/feature_test)
- [Contracts](../examples/language/basic-concepts/contracts.md) ·
  [C++26 hardened standard library](../guidelines/safety.md#c26-hardened-standard-library)
- [Integer comparison functions](https://en.cppreference.com/w/cpp/utility/intcmp)
  (`cmp_less`, `in_range`)
- Papers: [P2900](https://wg21.link/p2900) (contracts),
  [P3471](https://wg21.link/p3471) (standard library hardening),
  [P4433R0](https://wg21.link/p4433r0) (the DIS ballot)
- Tests: [`tests/security/language-versions_test.cpp`](../../tests/security/language-versions_test.cpp)

[^gcc15]: [GCC 15 release notes](https://gcc.gnu.org/gcc-15/changes.html), libstdc++: "Debug assertions are now enabled by default for unoptimized builds. Use `-D_GLIBCXX_NO_ASSERTIONS` to override this." Checked with GCC 15.2 and 16.2.
[^libcxx]: [libc++ hardening modes](https://libcxx.llvm.org/Hardening.html): "consult with your vendor to determine the level of hardening enabled by default."
[^msstl]: [MS STL: STL Hardening](https://github.com/microsoft/STL/wiki/STL-Hardening) (released disabled by default; `_MSVC_STL_HARDENING`) and its changelog for VS 2022 17.14.
[^p4433]: [P4433R0](https://wg21.link/p4433r0), *Do not remove Contracts from C++26* (2026-10-23), states that the ballot concluded on 22 October 2026 and that, for the first time, some national bodies voted against the DIS over contract assertions. The official result wasn't public when this page was written.
