# std::regex

> Regular-expression matching, searching, and replacement.
> **Header:** `<regex>` · **Since:** C++11

## Mental model

Compile a pattern into a `std::regex`, then apply one of three operations:

- **`regex_match`** — does the pattern match the **whole** string?
- **`regex_search`** — does the pattern occur **somewhere** in the string?
- **`regex_replace`** — substitute matches with a replacement.

Capture groups land in a `std::smatch` (for `std::string`) / `std::cmatch` (for
`const char*`), where `m[0]` is the whole match and `m[1]…` are the groups.

## Examples

### Match vs. search

```cpp
#include <regex>

std::regex date{R"(\d{4}-\d{2}-\d{2})"};         // raw string → no \\ escaping

std::regex_match("2026-05-26", date);             // true  (entire string)
std::regex_match("on 2026-05-26", date);          // false (not the whole string)
std::regex_search("on 2026-05-26", date);         // true  (found within)
```

### Capture groups

```cpp
std::string s = "key=value";
std::smatch m;
std::regex kv{R"((\w+)=(\w+))"};
if (std::regex_search(s, m, kv)) {
    m[0];   // "key=value"  (whole match)
    m[1];   // "key"        (group 1)
    m[2];   // "value"      (group 2)
    m.position(1); m.length(1);   // where/how long
}
```

### Iterate every match

```cpp
std::string text = "a1 b2 c3";
std::regex token{R"([a-z]\d)"};
for (std::sregex_iterator it{text.begin(), text.end(), token}, end; it != end; ++it)
    /* (*it)[0] → "a1", "b2", "c3" */;
```

### Replace (with backreferences)

```cpp
std::string out = std::regex_replace("John Smith",
                                     std::regex{R"((\w+)\s+(\w+))"},
                                     "$2, $1");        // "Smith, John"
```

### Grammar & flags

```cpp
std::regex ci{"hello", std::regex::icase};            // case-insensitive
std::regex ecma{R"(\d+)", std::regex::ECMAScript};    // default grammar
// other grammars: basic, extended, awk, grep, egrep
```

## Gotchas

- **Use raw string literals** `R"(...)"` for patterns — otherwise every `\` must
  be doubled (`"\\d"`).
- **`std::regex` is slow and heavy.** Compilation and matching have notable
  overhead, and historically `<regex>` is among the slowest standard
  implementations. For hot paths, compile the regex once (reuse the object) or
  use a dedicated library / hand-written parsing.
- **`smatch` holds iterators into the searched string** — don't let the source
  string die or change while you use the match results. Never `regex_search`
  against a **temporary** `std::string` when capturing into `smatch`.
- **Whole-string vs. substring:** `regex_match` requires the pattern to consume
  the entire input; people reach for it expecting `regex_search` behavior.

## See also

- [std::basic_regex — cppreference](https://en.cppreference.com/w/cpp/regex/basic_regex)
- [std::regex_search — cppreference](https://en.cppreference.com/w/cpp/regex/regex_search)
- [std::regex_replace — cppreference](https://en.cppreference.com/w/cpp/regex/regex_replace)
