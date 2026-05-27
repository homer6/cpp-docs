# std::locale and facets

> Culture-specific behavior — number/money/date formatting, collation, character
> classification — bundled into a `std::locale` made of **facets**.
> **Header:** `<locale>` · **Since:** C++98

## Mental model

A `std::locale` is an immutable bag of **facets**, each handling one category:

| Facet category | Handles |
|---|---|
| `std::numpunct` | decimal point, thousands separator, grouping |
| `std::collate` | string ordering rules |
| `std::ctype` | is-alpha/upper/lower, case conversion |
| `std::money_*`, `std::time_*` | currency and date/time formatting |
| `std::codecvt` | encoding conversion (largely deprecated) |

You `imbue` a stream with a locale, and the stream's `<<`/`>>` start obeying it.

## Examples

### The classic vs. the user's locale

```cpp
#include <locale>
#include <iostream>

std::locale::global(std::locale{""});       // "" = the user's environment locale
std::cout.imbue(std::locale{""});
std::cout << 1234567.89;                      // e.g. "1,234,567.89" (en_US) or
                                              //      "1.234.567,89" (de_DE)

std::cout.imbue(std::locale::classic());      // the "C" locale: no grouping
std::cout << 1234567.89;                       // "1.23457e+06"-style default
```

### Locale-aware comparison

```cpp
std::locale loc{""};
bool before = loc(std::string{"apple"}, std::string{"Äpfel"});   // uses collate
// std::locale's operator() compares strings per the locale's collate facet
```

### Querying a facet

```cpp
std::locale loc{""};
char sep = std::use_facet<std::numpunct<char>>(loc).thousands_sep();
char dec = std::use_facet<std::numpunct<char>>(loc).decimal_point();
```

## Gotchas

- **`std::locale{""}` reads the environment; `std::locale{"de_DE.UTF-8"}` names
  one explicitly** — and the named one throws `std::runtime_error` if the OS
  doesn't have it installed.
- **For data interchange, do NOT use locales.** Locale-formatted numbers aren't
  round-trippable (`,` vs `.`). Use [`charconv`](charconv.md) or `std::format`
  (locale-independent by default) for files, protocols, and logs; reserve
  locales for human-facing display.
- **`std::wstring_convert` / `<codecvt>` are deprecated.** Don't build new
  encoding conversion on them; prefer a dedicated Unicode library (and watch for
  C++26's `<text_encoding>` for encoding *identification*).
- **Locales are immutable and reference-counted** — cheap to copy, but you build
  a modified one by combining facets, not by mutating.

## See also

- [std::locale — cppreference](https://en.cppreference.com/w/cpp/locale/locale)
- [std::numpunct — cppreference](https://en.cppreference.com/w/cpp/locale/numpunct)
- [std::use_facet — cppreference](https://en.cppreference.com/w/cpp/locale/use_facet)
