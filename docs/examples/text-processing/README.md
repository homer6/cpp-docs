# Text processing library

Turning values into text and back, pattern matching, and locale-aware behavior.
Mirrors [cppreference's strings/text section](https://en.cppreference.com/w/cpp/string).

| Page | Header | Since | What |
|---|---|---|---|
| [Formatting & printing](formatting.md) | `<format>`, `<print>` | C++20 / C++23 | `std::format`, `std::print`, custom formatters |
| [`charconv` conversions](charconv.md) | `<charconv>` | C++17 | `to_chars` / `from_chars` — fast, locale-independent |
| [Regular expressions](regex.md) | `<regex>` | C++11 | `regex_match` / `search` / `replace`, iterators |
| [Localization](localization.md) | `<locale>` | C++98 | `std::locale`, facets, locale-aware I/O |
| [`text_encoding`](text-encoding.md) | `<text_encoding>` | C++26 | identify the literal, environment and locale encodings |
| [Null-terminated byte strings](null-terminated-byte-strings.md) | `<cstring>`, `<cctype>` | C++98 | the C string library: `strlen`, `strtol`, `isalpha` … |
| [Null-terminated multibyte strings](null-terminated-multibyte-strings.md) | `<cwchar>`, `<cuchar>` | C++98 / C++11 | locale-dependent encodings, `mbrtoc32`, `mbrtoc8` |
| [Null-terminated wide strings](null-terminated-wide-strings.md) | `<cwchar>`, `<cwctype>` | C++98 | `wchar_t` strings, `wcslen`, `iswalpha` |

## Picking a conversion tool

- **Build a human-readable string from values** → `std::format` (or `std::print`
  straight to output).
- **Fastest, locale-independent, round-trippable number ↔ text** → `from_chars`
  / `to_chars`.
- **Quick-and-dirty number → string** → `std::to_string` (locale-affected,
  allocates) — fine for casual use, but `format`/`to_chars` are better.
- **Match/replace by pattern** → `<regex>` (note: powerful but slow; for hot
  paths consider hand-written parsing or a faster library).
