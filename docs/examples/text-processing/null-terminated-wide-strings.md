# Null-terminated wide strings

> Zero-terminated arrays of `wchar_t`, with `wcs*` functions mirroring the byte
> string library and `isw*` / `tow*` classification that works on whole
> characters. **Header:** `<cwchar>`, `<cwctype>` · **Since:** C++98

## Mental model

A wide string trades bytes for `wchar_t` code units, ideally one per
character, so `wcslen` counts characters and `iswalpha` can classify `é`.
Whether that ideal holds depends on how wide `wchar_t` is, which is
implementation-defined:

```
L"héllo €"
 macOS, Linux (wchar_t = 32 bits, UTF-32):  [h][é][l][l][o][ ][€][\0]   wcslen = 7
 Windows     (wchar_t = 16 bits, UTF-16):   same here, but 😀 takes TWO units
```

What a wide character *means* (which characters are alphabetic, how they convert
to bytes) comes from the C locale's `LC_CTYPE`, exactly as for
[multibyte strings](null-terminated-multibyte-strings.md).

## Declaration

| Group | Functions | Header |
|---|---|---|
| Classify / map | `iswalnum iswalpha iswlower iswupper iswdigit iswxdigit iswcntrl iswgraph iswspace iswblank iswprint iswpunct`, `towlower towupper`, `wctype`+`iswctype`, `wctrans`+`towctrans` | `<cwctype>` |
| Convert to numbers | `wcstol wcstoll wcstoul wcstoull wcstof wcstod wcstold` | `<cwchar>` |
| Copy / concatenate | `wcscpy wcsncpy wcscat wcsncat wcsxfrm` | `<cwchar>` |
| Examine | `wcslen wcscmp wcsncmp wcscoll wcschr wcsrchr wcsspn wcscspn wcspbrk wcsstr wcstok` | `<cwchar>` |
| Wide arrays | `wmemcpy wmemmove wmemcmp wmemchr wmemset` | `<cwchar>` |
| Convert to/from multibyte | `mbrtowc wcrtomb mbsrtowcs wcsrtombs btowc wctob` | `<cwchar>` |

`wcstok` takes a third `wchar_t**` state argument, so unlike `strtok` it is
reentrant.

## When to use it

- **Windows APIs** (`CreateFileW`, …) take UTF-16 `wchar_t` strings; this is the
  main reason the family is still used.
- **Locale-aware classification of non-ASCII characters** without a Unicode
  library, on platforms where `wchar_t` is 32 bits.

Anti-fits: portable Unicode text. `wchar_t` is 32 bits on macOS and Linux but 16
on Windows, so neither "one unit = one character" nor the size of a file
written from `wchar_t` is portable. Store UTF-8 in `std::string` (or
`std::u8string`), and use [`std::wstring`](../strings/string.md) only at the
Windows API boundary.

## Examples

### Counting and classifying characters

```cpp
#include <clocale>
#include <cstdio>
#include <cwchar>
#include <cwctype>

int main() {
    const wchar_t* w = L"héllo €";            // "héllo €"
    std::printf("sizeof(wchar_t)=%zu wcslen=%zu\n", sizeof(wchar_t), std::wcslen(w));

    std::printf("C locale:     iswalpha(é)=%d towupper(é)=U+%04X\n",
                std::iswalpha(w[1]) != 0, unsigned(std::towupper(w[1])));
    if (!std::setlocale(LC_ALL, "en_US.UTF-8") && !std::setlocale(LC_ALL, "C.UTF-8"))
        return 1;
    std::printf("UTF-8 locale: iswalpha(é)=%d towupper(é)=U+%04X\n",
                std::iswalpha(w[1]) != 0, unsigned(std::towupper(w[1])));
}
```

Same output on macOS (GCC 15, Clang 22) and Linux/glibc (GCC 16):

```
sizeof(wchar_t)=4 wcslen=7
C locale:     iswalpha(é)=0 towupper(é)=U+00E9
UTF-8 locale: iswalpha(é)=1 towupper(é)=U+00C9
```

### Classification by name

```cpp
#include <cassert>
#include <cwctype>

int main() {
    std::wctype_t digit = std::wctype("digit");   // also "alpha", "space", locale-specific names
    assert(std::iswctype(L'7', digit));
    std::wctrans_t up = std::wctrans("toupper");
    assert(std::towctrans(L'a', up) == L'A');
}
```

### Wide → UTF-8 (or whatever the locale encodes)

```cpp
#include <clocale>
#include <cstdio>
#include <cwchar>
#include <string>

std::string narrow(const wchar_t* w) {
    std::mbstate_t st{};
    const wchar_t* src = w;
    std::size_t n = std::wcsrtombs(nullptr, &src, 0, &st);   // measure first
    if (n == static_cast<std::size_t>(-1)) return {};          // unrepresentable char
    std::string out(n, '\0');
    st = {};
    src = w;
    std::wcsrtombs(out.data(), &src, n, &st);
    return out;
}

int main() {
    std::setlocale(LC_ALL, "en_US.UTF-8") || std::setlocale(LC_ALL, "C.UTF-8");
    std::string s = narrow(L"héllo €");
    std::printf("%zu bytes: %s\n", s.size(), s.c_str());      // 10 bytes: héllo €
}
```

## Gotchas

- **`wchar_t`'s size, signedness and encoding are implementation-defined.**
  Verified locally: 4 bytes on both macOS and Linux, but `WCHAR_MAX` is
  2147483647 (signed) on arm64 macOS and 4294967295 (unsigned) on arm64 Linux.
  On Windows it is 16-bit UTF-16 (not verifiable locally here).
- **The `C` locale classifies only ASCII** in practice: `iswalpha(L'é')` is
  false until you call `setlocale`.
- **`towupper` maps one unit to one unit.** It cannot do `ß` → `SS` or any
  context-sensitive case mapping; use a Unicode library for real case conversion.
- **`printf("%ls")` converts through the current locale**, and what happens to
  a character the locale can't encode is up to the C library: in the `"C"`
  locale, `printf("[%ls]\n", L"héllo")` returned -1 on glibc and printed
  nothing with glibc 2.41, but `[` with glibc 2.36 (Debian 12), which writes
  the output before the failing conversion; macOS wrote the raw byte `0xE9`.
  A failed `printf` can leave partial output behind. Don't mix byte and wide output
  (`printf` vs `wprintf`) on one `FILE*`: a stream's orientation is fixed by its
  first use (`std::fwide`).
- **The `wcs*` functions share the byte versions' bugs**: `wcsncpy` may not
  terminate, `wcscpy` doesn't know the destination size.

## See also

- [Null-terminated wide strings — cppreference](https://en.cppreference.com/w/cpp/string/wide)
- [`std::wcsrtombs` — cppreference](https://en.cppreference.com/w/cpp/string/multibyte/wcsrtombs)
- [`std::iswctype` — cppreference](https://en.cppreference.com/w/cpp/string/wide/iswctype)
- [`std::fwide` — cppreference](https://en.cppreference.com/w/cpp/io/c/fwide)
- [Byte strings](null-terminated-byte-strings.md) · [Multibyte strings](null-terminated-multibyte-strings.md)
