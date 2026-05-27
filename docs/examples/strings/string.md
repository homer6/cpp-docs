# std::string (std::basic_string)

> An owning, growable sequence of characters — `std::string` is
> `std::basic_string<char>`. **Header:** `<string>` · **Since:** C++98

## Mental model

A `vector`-like container specialized for characters: contiguous storage, a size
and a capacity, geometric growth. Two extras matter:

- A **null terminator** is always kept just past the last character, so
  `c_str()` hands C APIs a valid `const char*` for free.
- **Small String Optimization (SSO):** short strings (typically ≤15 chars) are
  stored *inside* the string object with no heap allocation. (Implementation
  detail, not mandated, but ubiquitous.)

```
 "hello" with SSO:   [ h e l l o \0 ...inline buffer... ]   (no heap)
 long string:        ptr ─▶ heap buffer, size, capacity
```

## The basic_string family

```cpp
template<
    class CharT,
    class Traits    = std::char_traits<CharT>,
    class Allocator = std::allocator<CharT>
> class basic_string;
```

| Alias | Char type |
|---|---|
| `std::string` | `char` |
| `std::wstring` | `wchar_t` |
| `std::u8string` (C++20) | `char8_t` |
| `std::u16string` / `std::u32string` (C++11) | `char16_t` / `char32_t` |
| `std::pmr::string` (C++17) | `char`, polymorphic allocator |

`Traits` ([`char_traits`](char_traits.md)) defines how characters are compared,
copied, and found.

## Complexity

| Operation | Cost |
|---|---|
| `[]`, `front`, `back`, `c_str`, `data`, `size` | O(1) |
| `push_back` / `append` / `operator+=` | amortized O(1) |
| `insert` / `erase` mid-string | O(n) |
| `find` (substring search) | up to O(n·m) |
| `substr` | O(n) (it copies) |

## Examples

### Construction & growth

```cpp
#include <string>
using namespace std::string_literals;        // the ""s literal

std::string a = "hello";
std::string b(5, '*');                        // "*****"
std::string c = "x"s + "y";                    // ""s forces std::string, not const char*
std::string d{a.begin(), a.begin() + 3};       // "hel"

a += " world";                                 // append
a.append(3, '!');                              // "hello world!!!"
a.push_back('?');
```

### Access & C interop

```cpp
a[0];                  // unchecked
a.at(0);               // bounds-checked → std::out_of_range
const char* p = a.c_str();    // null-terminated, for C APIs
char* raw = a.data();         // contiguous; non-const since C++17
```

### Searching & substrings

```cpp
std::string s = "key=value";
auto eq = s.find('=');                 // index, or std::string::npos if absent
if (eq != std::string::npos) {
    std::string key = s.substr(0, eq); // "key"
    std::string val = s.substr(eq + 1);// "value"
}
s.starts_with("key");                  // C++20
s.ends_with("ue");                     // C++20
s.contains("alu");                     // C++23
```

### Numeric conversions

```cpp
int n = std::stoi("42");               // string → int (throws on bad input)
double x = std::stod("3.14");
std::string t = std::to_string(123);   // number → string
```

(For locale-independent, non-throwing, faster conversions prefer
`std::from_chars` / `std::to_chars` — see
[text processing](../text-processing/README.md).)

## Gotchas

- **`std::string` is not null-terminated *by you* — but `c_str()`/`data()` are
  guaranteed terminated.** Embedded `'\0'` is allowed; `size()` (not `strlen`)
  is the truth.
- **`substr` copies.** In hot paths that just inspect a slice, use
  [`std::string_view`](string_view.md) instead.
- **`operator+` builds temporaries.** Chaining many `+` allocates repeatedly;
  prefer `+=`, `append`, or `std::format`. `reserve()` if you know the size.
- **`"abc"` is a `const char*`, not a `std::string`.** Use the `""s` literal or
  an explicit `std::string` when overload resolution matters.
- **Iterator/reference invalidation mirrors `vector`:** anything that reallocates
  (growth past capacity) invalidates iterators, pointers, and references.

## See also

- [std::basic_string — cppreference](https://en.cppreference.com/w/cpp/string/basic_string)
- [std::stoi etc. — cppreference](https://en.cppreference.com/w/cpp/string/basic_string/stol)
- [String literals (`""s`) — cppreference](https://en.cppreference.com/w/cpp/string/basic_string/operator%22%22s)
