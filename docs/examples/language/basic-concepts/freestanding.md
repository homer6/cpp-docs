# Freestanding and hosted implementations

> A *hosted* implementation has the whole standard library and an OS under it; a
> *freestanding* one (kernels, firmware, bootloaders) has the whole **language**
> but only a required subset of the library. **Macro:** `__STDC_HOSTED__` ·
> **Since:** C++98 (the subset grows a lot in C++23 and C++26)

## Mental model

```
                 hosted                         freestanding
language         all of it                      all of it  ([lex] through [cpp])
library          all of it                      the headers in [compliance], and inside
                                                each only the "freestanding items"
main()           required                       implementation-defined
startup/exit     static init, main, atexit ...  implementation-defined
threads          may run concurrently           implementation-defined whether >1 thread
__STDC_HOSTED__  1                              0
```

Which kind you have is implementation-defined; with GCC and Clang it's chosen
by `-ffreestanding`, which sets `__STDC_HOSTED__` to `0` and stops the compiler
assuming that library functions like `memcpy` behave as standard (it implies
`-fno-builtin`).

## Declaration

### Required freestanding headers — C++26 ([compliance])

| Since | Headers |
|---|---|
| C++98/17 | `<cstddef>` `<cfloat>` `<climits>` `<limits>` `<cstdint>` `<cstdlib>` (partial) `<new>` `<typeinfo>` `<exception>` `<initializer_list>` `<cstdarg>` `<type_traits>` `<atomic>` |
| C++20 | `<version>` `<source_location>` `<compare>` `<coroutine>` `<concepts>` `<bit>` |
| C++23 | `<utility>` `<tuple>` `<functional>` `<memory>` `<ratio>` `<iterator>` `<ranges>` |
| C++26 | `<algorithm>` `<numeric>` `<array>` `<inplace_vector>` `<span>` `<mdspan>` `<optional>` `<variant>` `<expected>` `<string_view>` `<string>` `<cstring>` `<cwchar>` `<charconv>` `<cmath>` `<random>` `<execution>` `<cerrno>` `<system_error>` `<contracts>` `<debugging>` `<stdbit.h>` |

Rows compared across the drafts that match each standard: N4659 (C++17, which
also listed the deprecated `<ciso646>`, `<cstdalign>`, `<cstdbool>`), N4861
(C++20), N4950 (C++23) and N5050 (C++26). The current C++29 working draft at
eel.is has the same list as N5050.

**Listed doesn't mean complete.** A header in the table only has to provide
its *freestanding items*: declarations marked `// freestanding` in the
synopsis, or everything in a synopsis that starts `// all freestanding` (minus
anything marked `// hosted`). So `<string>` gives you `std::char_traits` but not
`std::string` (it allocates); `<memory>` gives you `std::unique_ptr` and the
uninitialized-memory algorithms but not `std::allocator` or `std::make_unique`;
in `<algorithm>` the execution-policy overloads are only *freestanding-deleted*
(below).

C++26 adds **freestanding-deleted** functions (marked `// freestanding-deleted`):
an implementation either provides them or declares them `= delete`, so overload
resolution doesn't silently change when you move code to a hosted build.

### Feature-test macros

Each partial header has a `__cpp_lib_freestanding_*` macro in `<version>`.
C++26 (N5050) lists 25, among them `__cpp_lib_freestanding_algorithm`,
`_array`, `_char_traits`, `_charconv`, `_cstring`, `_expected`, `_mdspan`,
`_memory`, `_operator_new`, `_optional`, `_ranges`, `_string_view`, `_tuple`, `_utility`,
`_variant` and the umbrella `__cpp_lib_freestanding_feature_test_macros`.

What the local libraries define (`echo '#include <version>' | … -dM -E`):

| Library | `__cpp_lib_freestanding_*` macros |
|---|---|
| libstdc++ (GCC 15.2 and 16.2) | `algorithm`, `array`, `cstring`, `expected`, `optional`, `string_view`, `variant` (all `202311L`) |
| libc++ (Clang 22) | none |

## When to use it

- You're writing a kernel, firmware or a bare-metal runtime: compile with
  `-ffreestanding` (plus `-fno-exceptions -fno-rtti` if the platform has no
  unwinder) and stick to freestanding items.
- You're writing a library that should also work there: guard hosted-only code
  with `__STDC_HOSTED__` or the `__cpp_lib_freestanding_*` macros, and prefer
  facilities that don't allocate, throw or need the OS (`std::array`,
  `std::span`, `std::optional`, `std::expected`, `std::inplace_vector`).
- Ordinary applications: you're hosted; this page is background.

## Examples

### Detect the implementation kind

```cpp
#include <cstdio>

int main() {
#if __STDC_HOSTED__
    std::puts("hosted");
#else
    std::puts("freestanding");     // g++-15 -ffreestanding prints this
#endif
}
```

### Freestanding-friendly code

Nothing here allocates, throws, or calls into the OS:

```cpp
#include <array>
#include <cstdint>
#include <optional>
#include <span>

constexpr std::optional<std::size_t> find_byte(std::span<const std::uint8_t> bytes,
                                               std::uint8_t b) {
    for (std::size_t i = 0; i < bytes.size(); ++i)
        if (bytes[i] == b) return i;
    return std::nullopt;
}

constexpr std::array<std::uint8_t, 4> header{0x7F, 'E', 'L', 'F'};
static_assert(find_byte(header, 'L') == 2);
```

### Guarding a hosted-only path

```cpp
#include <version>
#include <string_view>

#if __STDC_HOSTED__
#include <cstdio>
inline void debug_log(std::string_view msg) {
    std::fwrite(msg.data(), 1, msg.size(), stderr);
}
#else
inline void debug_log(std::string_view) {}   // no stdio on bare metal
#endif
```

## Gotchas

- **`-ffreestanding` doesn't swap the library.** GCC and Clang still find the
  hosted libstdc++/libc++ headers; `-ffreestanding` only changes
  `__STDC_HOSTED__` and builtin assumptions. A real freestanding library comes
  from a toolchain built for it (e.g. libstdc++ configured with
  `--disable-hosted-libstdcxx`).
- **`main` may not exist.** In a freestanding environment the entry point,
  static initialization and static destruction are implementation-defined
  ([basic.start.main]): constructors of globals may not run unless your startup
  code runs them.
- **`operator new` is declared but may not work.** `<new>` is freestanding,
  but the replaceable allocation functions need something to allocate from; on
  bare metal you usually provide them yourself or avoid `new`.
- **Exceptions and RTTI are part of the language**, so freestanding allows
  them, but many targets turn them off. Code using `throw` or `dynamic_cast`
  then won't build.
- **Draft vs. shipping libraries.** The C++26 table is the target; libstdc++
  advertises only part of it and libc++ advertises none of it yet. Check the
  feature-test macro, not the standard version.

## See also

- [Freestanding and hosted implementations — cppreference](https://en.cppreference.com/w/cpp/freestanding)
- [Feature-test macros — cppreference](https://en.cppreference.com/w/cpp/feature_test)
- [N5050](https://wg21.link/n5050) — C++26 draft (`[compliance]`, `[version.syn]`)
- [\[compliance\]](https://eel.is/c++draft/compliance),
  [\[freestanding.item\]](https://eel.is/c++draft/freestanding.item),
  [\[intro.compliance.general\]](https://eel.is/c++draft/intro.compliance.general),
  [\[basic.start.main\]](https://eel.is/c++draft/basic.start.main) — C++ working draft
- [GCC: `-ffreestanding`](https://gcc.gnu.org/onlinedocs/gcc/C-Dialect-Options.html)
- [The object model & `main`](object-model.md) · [Feature-test macros](../../feature-test-macros/README.md)
