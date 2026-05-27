# std::char_traits

> The policy class that defines how a character type is compared, copied, and
> searched — the `Traits` parameter every string type is built on.
> **Header:** `<string>` · **Since:** C++98

## Mental model

`basic_string<CharT, Traits>` and `basic_string_view<CharT, Traits>` never touch
characters directly; they go through `Traits`. So `char_traits<CharT>` is the
"how do characters behave" layer: equality, ordering, length, copy/move, find,
and what counts as end-of-file.

Swap in a custom traits type and you change string *behavior* without changing
its storage — the classic example being case-insensitive strings.

## Interface (static functions)

```cpp
template<class CharT> struct char_traits;   // specialized for char, wchar_t,
                                             // char8_t, char16_t, char32_t
```

Key static members every specialization provides:

| Member | Purpose |
|---|---|
| `char_type`, `int_type` | the character type and an integer that can also hold EOF |
| `eq(a, b)`, `lt(a, b)` | character equality / less-than |
| `compare(p, q, n)` | lexicographic compare of n chars |
| `length(p)` | length up to the null character |
| `find(p, n, ch)` | locate a character |
| `assign`, `copy`, `move` | character-wise assignment/copy |
| `eof()`, `eq_int_type`, `to_int_type`, `to_char_type` | EOF / int_type plumbing |

Many are `constexpr` (since C++17/20), enabling compile-time string work.

## Example: a case-insensitive string

```cpp
#include <string>
#include <cctype>

struct ci_traits : std::char_traits<char> {
    static bool eq(char a, char b) {
        return std::tolower((unsigned char)a) == std::tolower((unsigned char)b);
    }
    static bool lt(char a, char b) {
        return std::tolower((unsigned char)a) <  std::tolower((unsigned char)b);
    }
    static int compare(const char* a, const char* b, std::size_t n) {
        for (; n--; ++a, ++b) {
            if (lt(*a, *b)) return -1;
            if (lt(*b, *a)) return  1;
        }
        return 0;
    }
};

using ci_string = std::basic_string<char, ci_traits>;

ci_string x = "Hello";
ci_string y = "HELLO";
bool same = (x == y);          // true — comparison goes through ci_traits
```

## Gotchas

- **You rarely write `char_traits` by hand.** It exists for the few cases that
  need different character semantics (case-insensitivity, custom EOF). For most
  code it's invisible.
- **A custom-traits string is a *different type*** from `std::string`. `ci_string`
  and `std::string` won't interoperate without explicit conversion — that's the
  cost of the trick.
- **`length` stops at the null character**, matching C-string conventions; it's
  not a general "size" function.
- Don't confuse it with locale facets: `char_traits` is fixed per type, while
  locales handle culture-specific collation/formatting (see
  [text processing](../text-processing/README.md)).

## See also

- [std::char_traits — cppreference](https://en.cppreference.com/w/cpp/string/char_traits)
