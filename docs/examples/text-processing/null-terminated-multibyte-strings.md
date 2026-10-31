# Null-terminated multibyte strings

> `char` strings in which one character may take several bytes, decoded
> according to the current C locale (UTF-8, Shift-JIS, …). **Header:**
> `<cstdlib>`, `<cwchar>`, `<cuchar>` · **Since:** C++98; `<cuchar>` C++11;
> `mbrtoc8` / `c8rtomb` C++20

## Mental model

A multibyte string is still a zero-terminated `char` array, but the bytes are an
*encoding* of characters. Which encoding is decided at run time by the
`LC_CTYPE` category of the C locale — not by the compiler and not by
`std::locale`.

```
"héllo €" in UTF-8:
 bytes      68 | C3 A9 | 6C | 6C | 6F | 20 | E2 82 AC | 00
 characters  h |   é   |  l |  l |  o | ␠  |    €     |
 strlen = 10 bytes, but 7 characters
```

The conversion functions turn one multibyte character into a wide (`wchar_t`)
or Unicode (`char8_t`, `char16_t`, `char32_t`) code unit and back. The
**restartable** ones (`mbr*`, `*rtomb`) keep their position inside a character in
an `std::mbstate_t` you own; the old ones (`mblen`, `mbtowc`, `wctomb`) keep
hidden static state and aren't thread-safe.

## Declaration

| Function | Converts | Header |
|---|---|---|
| `mbrlen(s, n, &st)` | length in bytes of the next character | `<cwchar>` |
| `mbrtowc` / `wcrtomb` | one character ↔ `wchar_t` | `<cwchar>` |
| `mbsrtowcs` / `wcsrtombs` | whole string ↔ wide string | `<cwchar>` |
| `mbsinit(&st)`, `btowc`, `wctob` | state is initial?; single byte ↔ wide | `<cwchar>` |
| `mbrtoc8` / `c8rtomb` (C++20) | one character ↔ UTF-8 code units (`char8_t`) | `<cuchar>` |
| `mbrtoc16` / `c16rtomb` | one character ↔ UTF-16 code units | `<cuchar>` |
| `mbrtoc32` / `c32rtomb` | one character ↔ UTF-32 code point | `<cuchar>` |
| `mblen`, `mbtowc`, `wctomb`, `mbstowcs`, `wcstombs` | non-restartable versions | `<cstdlib>` |
| `MB_CUR_MAX` · `MB_LEN_MAX` | max bytes per character in the current locale · in any locale | `<cstdlib>` · `<climits>` |

Return values of the `mbr*` functions, as `std::size_t`:

| Value | Meaning |
|---|---|
| `0` | decoded the null character |
| `1 … n` | decoded one character from that many bytes |
| `(size_t)-1` | invalid sequence; `errno = EILSEQ` |
| `(size_t)-2` | incomplete: the `n` bytes are a valid prefix, feed more |
| `(size_t)-3` | (`mbrtoc16`, `mbrtoc8`) wrote another code unit of a previous character without consuming input |

## When to use it

- **Talking to the C library and the OS** in the user's own encoding (terminal
  output, file names on POSIX, `argv`).
- **Decoding UTF-8 without a third-party library**, when the program runs with a
  UTF-8 locale: `mbrtoc32` gives you code points.

Anti-fits: data whose encoding is fixed by a file format or protocol (it is
almost always UTF-8; don't let the user's locale decide how to read it), and
anything needing normalization, grapheme clusters or case folding — use a
Unicode library such as ICU. To *identify* the encoding, see
[`std::text_encoding`](text-encoding.md) (C++26).

## Examples

### Nothing decodes until you pick a locale

```cpp
#include <clocale>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cwchar>

int main() {
    const char* s = "h\xC3\xA9llo \xE2\x82\xAC";       // "héllo €" in UTF-8
    std::printf("%s MB_CUR_MAX=%zu\n", std::setlocale(LC_ALL, nullptr), MB_CUR_MAX);

    // Programs start in the "C" locale. Switch to the environment's (or a named) one:
    if (!std::setlocale(LC_ALL, "en_US.UTF-8") && !std::setlocale(LC_ALL, "C.UTF-8"))
        return 1;

    std::mbstate_t st{};
    std::size_t chars = 0;
    for (const char *p = s, *end = s + std::strlen(s); p < end; ++chars) {
        std::size_t len = std::mbrlen(p, end - p, &st);
        if (len == static_cast<std::size_t>(-1) || len == static_cast<std::size_t>(-2))
            return 2;                                   // invalid or truncated
        p += len;
    }
    std::printf("%zu bytes, %zu characters, MB_CUR_MAX=%zu\n",
                std::strlen(s), chars, MB_CUR_MAX);
}
```

macOS (GCC 15, Clang 22) prints `C MB_CUR_MAX=1` then
`10 bytes, 7 characters, MB_CUR_MAX=4`; glibc (GCC 16) prints `MB_CUR_MAX=6`
in `C.UTF-8`. The value is implementation-defined.

### Decode to code points and encode back (`<cuchar>`)

```cpp needs=cuchar
#include <clocale>
#include <cstdio>
#include <cstring>
#include <cuchar>

int main() {
    if (!std::setlocale(LC_ALL, "C.UTF-8") && !std::setlocale(LC_ALL, "en_US.UTF-8"))
        return 1;
    const char* s = "h\xC3\xA9llo \xE2\x82\xAC";
    std::mbstate_t st{};
    char32_t c32;
    for (const char *p = s, *end = s + std::strlen(s); p < end; ) {
        std::size_t len = std::mbrtoc32(&c32, p, end - p, &st);
        if (len > static_cast<std::size_t>(end - p)) return 2;                   // -1, -2 or -3
        std::printf("U+%04X ", static_cast<unsigned>(c32));
        p += len;
    }
    char out[8];
    st = {};
    std::size_t n = std::c32rtomb(out, U'€', &st);    // back to the locale's bytes
    std::printf("\n%zu bytes: %02X %02X %02X\n", n,
                static_cast<unsigned char>(out[0]), static_cast<unsigned char>(out[1]),
                static_cast<unsigned char>(out[2]));
}
```

```
U+0068 U+00E9 U+006C U+006C U+006F U+0020 U+20AC
3 bytes: E2 82 AC
```

### `mbrtoc8` hands out one UTF-8 code unit per call (C++20)

```cpp needs=cuchar
#include <clocale>
#include <cstddef>
#include <cstdio>
#include <cuchar>

int main() {
    std::setlocale(LC_ALL, "C.UTF-8");
    const char* e_acute = "\xC3\xA9";
    std::mbstate_t st{};
    char8_t c8;
    std::size_t r = std::mbrtoc8(&c8, e_acute, 2, &st);
    std::printf("%td %02X\n", static_cast<std::ptrdiff_t>(r), unsigned(c8)); // 2 C3: consumed 2 bytes
    r = std::mbrtoc8(&c8, e_acute + 2, 0, &st);
    std::printf("%td %02X\n", static_cast<std::ptrdiff_t>(r), unsigned(c8)); // -3 A9: stored, no input
}
```

## Gotchas

- **`<cuchar>` is missing on macOS.** The macOS SDK has no `<uchar.h>`, so
  `std::mbrtoc32`, `std::c32rtomb`, `std::mbrtoc8` etc. are not declared with
  either GCC 15 or Clang 22/libc++ on macOS. The `<cuchar>` examples above are
  tagged `needs=cuchar` and were verified with GCC 16 on Linux (glibc). Use
  `mbrtowc` (where `wchar_t` is UTF-32) or a Unicode library on macOS.
- **The encoding is global, run-time state.** Programs start in the `"C"`
  locale, where only single bytes decode (`mbrtoc32` on `"\xC3\xA9"` returns
  `(size_t)-1` on glibc). `setlocale` is not thread-safe and affects every thread.
- **Which locale names exist is implementation-defined.** `"C.UTF-8"` is
  standard on glibc; macOS offers `"en_US.UTF-8"`. `setlocale` returns `nullptr`
  when a name isn't installed — always check it.
- **`mbstate_t` must start zeroed** (`std::mbstate_t st{};`) and belongs to one
  string; reset it after an error, because the state is then unspecified.
- **`(size_t)-2` is not an error.** It means a character was split across your
  buffer boundary — keep the state and feed the next chunk.
- **`mbrtoc16` can need two calls per character.** A character outside the
  BMP becomes a surrogate pair; the second call returns `(size_t)-3`.
- **`std::codecvt` / `std::wstring_convert` are deprecated** C++ wrappers for
  the same job; see [Localization](localization.md).

## See also

- [Null-terminated multibyte strings — cppreference](https://en.cppreference.com/w/cpp/string/multibyte)
- [`std::mbrtoc32` — cppreference](https://en.cppreference.com/w/cpp/string/multibyte/mbrtoc32)
- [`std::mbrtoc8` — cppreference](https://en.cppreference.com/w/cpp/string/multibyte/mbrtoc8)
- [`std::setlocale` — cppreference](https://en.cppreference.com/w/cpp/locale/setlocale)
- [P0482 — char8_t (adds `mbrtoc8` / `c8rtomb`)](https://wg21.link/p0482)
- [Byte strings](null-terminated-byte-strings.md) · [Wide strings](null-terminated-wide-strings.md) · [`text_encoding`](text-encoding.md)
