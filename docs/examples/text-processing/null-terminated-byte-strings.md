# Null-terminated byte strings

> The C string library: `char` arrays ended by `'\0'`, with functions to
> classify, copy, compare, search and convert them. **Header:** `<cstring>`,
> `<cctype>`, `<cstdlib>`, `<cinttypes>` · **Since:** C++98 (inherited from C)

## Mental model

A byte string is a pointer to the first `char` of an array. Its length isn't
stored anywhere: every function walks forward until it finds the terminating
zero byte.

```
const char* s ─▶ [ 'h' ][ 'i' ][ '!' ][ '\0' ][ ?? ][ ?? ]
                 └──── strlen(s) == 3 ───┘   the terminator is not counted
```

Every bug in this family comes from that design: a missing terminator, a
destination array that is too small, or `O(n)` length lookups repeated in a
loop. In C++ use [`std::string`](../strings/string.md) and
[`std::string_view`](../strings/string_view.md) to hold text, and reach for
these functions at C API boundaries (`c_str()`, `argv`, OS calls).

## Declaration

| Group | Functions | Header |
|---|---|---|
| Classify / map one character | `isalnum isalpha islower isupper isdigit isxdigit iscntrl isgraph isspace isblank isprint ispunct`, `tolower toupper` | `<cctype>` |
| Convert to numbers | `atoi atol atoll atof`, `strtol strtoll strtoul strtoull strtof strtod strtold` | `<cstdlib>` |
| Convert to `intmax_t` / `uintmax_t` (C++11) | `strtoimax strtoumax`, plus the `PRI…` / `SCN…` format macros | `<cinttypes>` |
| Copy / concatenate | `strcpy strncpy strcat strncat strxfrm` | `<cstring>` |
| Examine | `strlen strcmp strncmp strcoll strchr strrchr strspn strcspn strpbrk strstr strtok` | `<cstring>` |
| Raw memory | `memchr memcmp memset memcpy memmove` | `<cstring>` |
| Misc | `strerror` | `<cstring>` |

The `<cctype>` functions take an `int` that must be representable as
`unsigned char` or equal to `EOF`, and they depend on the current C locale
(`std::setlocale`).

## When to use it

- **Calling C APIs** that take or return `const char*`.
- **`memcpy` / `memmove` / `memset` / `memcmp`** remain the right tools for raw
  bytes (or use `std::ranges::copy`, `std::bit_cast`).
- **`strtol` and friends** when you need a locale-aware parse with an end
  pointer, though [`std::from_chars`](charconv.md) is faster, never allocates and
  ignores the locale.

Anti-fits: storing or building text (use `std::string`), parsing in hot paths
(`from_chars`), tokenizing (`std::views::split` instead of `strtok`), and
anything involving Unicode characters beyond ASCII — these functions see bytes,
not characters ([multibyte strings](null-terminated-multibyte-strings.md)).

## Examples

### Examine and search

```cpp
#include <cstdio>
#include <cstring>

int main() {
    const char* hay = "needle in haystack";
    std::printf("%zu\n", std::strlen(hay));                   // 18
    std::printf("%s\n", std::strstr(hay, "hay"));              // haystack
    std::printf("%s\n", std::strrchr(hay, 'e'));               // e in haystack
    std::printf("%zu\n", std::strspn("123abc", "0123456789")); // 3: leading digits
    std::printf("%zu\n", std::strcspn("key=value", "="));      // 3: index of '='
    std::printf("%d\n", std::strcmp("apple", "banana") < 0);   // 1: sign only, not -1
}
```

### Classify characters safely

```cpp
#include <algorithm>
#include <cctype>
#include <string>

std::string upper(std::string s) {
    std::ranges::transform(s, s.begin(), [](unsigned char c) {   // unsigned char!
        return static_cast<char>(std::toupper(c));
    });
    return s;
}
```

Passing a plain `char` that holds a negative value (any byte ≥ 0x80 where `char`
is signed — it is on arm64 macOS, `CHAR_MIN == -128`, but unsigned on arm64
Linux; the signedness of `char` is implementation-defined) to `isalpha`, `toupper` and the rest is
undefined behaviour. Convert to `unsigned char` first.

### Convert text to numbers with error checking

```cpp
#include <cerrno>
#include <cstdio>
#include <cstdlib>

int main() {
    char* end = nullptr;
    errno = 0;
    long v = std::strtol("  42xyz", &end, 10);
    std::printf("%ld rest='%s'\n", v, end);           // 42 rest='xyz'  (skips leading space)

    errno = 0;
    v = std::strtol("99999999999999999999", &end, 10);
    std::printf("%ld ERANGE=%d\n", v, errno == ERANGE); // LONG_MAX, ERANGE=1
}
```

`atoi` has no way to report failure (it returns 0, and overflow is undefined
behaviour); prefer `strtol` or `std::from_chars`.

### Fixed-width integers in `printf` / `scanf`: `<cinttypes>` (C++11)

`%ld` or `%lld`? It depends on the platform (`int64_t` is `long long` on macOS
and `long` on 64-bit Linux). `<cinttypes>` gives each `<cstdint>` type a
string-literal macro for the right conversion: `PRId64`, `PRIu32`, `PRIx16`…
for `printf`, `SCNd64`, `SCNu32`… for `scanf`. They're pasted into the format
string by literal concatenation. It also has `strtoimax` / `strtoumax`, the
`strtol` family for `intmax_t`.

```cpp
#include <cerrno>
#include <cinttypes>
#include <cstdio>

int main() {
    std::int64_t big = -9'000'000'000;
    std::printf("%" PRId64 "\n", big);                 // -9000000000 ("lld" or "ld" underneath)

    std::uint32_t u = 0;
    std::sscanf("4000000000", "%" SCNu32, &u);         // reads into exactly a uint32_t
    std::printf("%" PRIu32 " %" PRIx32 "\n", u, u);    // 4000000000 ee6b2800

    char* end = nullptr;
    errno = 0;
    std::intmax_t m = std::strtoimax("  -123abc", &end, 10);
    std::printf("%" PRIdMAX " rest='%s'\n", m, end);   // -123 rest='abc'
}
```

In new C++ code, **`std::format` / `std::print` make these macros
unnecessary**: `std::format("{} {:x}", big, u)` deduces the type, so there is
no specifier to get wrong. `std::from_chars` ([charconv](charconv.md)) parses
any integer type without a macro. The macros still matter for `printf`-style
logging APIs and for code shared with C.

### `memmove` handles overlap, `memcpy` does not

```cpp
#include <cstdio>
#include <cstring>

int main() {
    char buf[] = "abcdef";
    std::memmove(buf + 2, buf, 4);   // overlapping ranges: memmove is required
    std::printf("%s\n", buf);         // ababcd
}
```

## Gotchas

- **`strncpy` does not always terminate.** If the source is at least `n` bytes
  long, no `'\0'` is written; GCC 15 even warns `-Wstringop-truncation`. It also
  pads short copies with zeros up to `n`.

  ```cpp
  #include <cstdio>
  #include <cstring>
  int main() {
      char buf[4];
      std::strncpy(buf, "hello", sizeof buf);    // 'h','e','l','l' — no terminator
      std::puts(std::memchr(buf, '\0', sizeof buf) ? "terminated" : "not terminated");
  }
  ```
- **`strcpy` / `strcat` don't know the destination size.** They are the classic
  buffer overflow. `strncat`'s `n` is the number of characters to append, not
  the buffer size. (The `_s` "bounds-checked" versions from C11 Annex K are
  optional, and neither glibc nor macOS libc provide them.)
- **`strtok` keeps hidden static state.** It is not reentrant or thread-safe,
  modifies its input, and merges adjacent delimiters (`"a,b,,c"` yields `a b c`).
- **`strlen` in a loop condition is `O(n²)`.** Keep the length, or use
  `std::string_view`, which stores it.
- **`strcmp` returns any sign, not -1/0/1.** Compare against zero.
- **`strerror` may return a pointer to a static buffer** that a later call
  overwrites; its text is implementation-defined.
- **`<cctype>` and `strtod` depend on the C locale.** `std::setlocale` in one
  thread changes the answer for every thread.
- **`std::strchr(const char*, int)` has two overloads in C++** (`const` and
  non-`const`), unlike C's single function that silently drops `const`.
  GCC's libstdc++ on macOS and FreeBSD is an exception: it relies on the C
  headers for them, and the macOS SDK and FreeBSD's libc only declare C's
  `char* strchr(const char*, int)` (glibc declares both for C++).

## See also

- [Null-terminated byte strings — cppreference](https://en.cppreference.com/w/cpp/string/byte)
- [`std::strncpy` — cppreference](https://en.cppreference.com/w/cpp/string/byte/strncpy)
- [`std::strtol` — cppreference](https://en.cppreference.com/w/cpp/string/byte/strtol)
- [`<cinttypes>` format macros — cppreference](https://en.cppreference.com/w/cpp/types/integer#Format_macro_constants) · [`std::strtoimax` — cppreference](https://en.cppreference.com/w/cpp/string/byte/strtoimax)
- [`std::isalpha` — cppreference](https://en.cppreference.com/w/cpp/string/byte/isalpha) (the `unsigned char` rule)
- [Multibyte strings](null-terminated-multibyte-strings.md) · [Wide strings](null-terminated-wide-strings.md)
- [`charconv`](charconv.md) — the modern number ↔ text tool
