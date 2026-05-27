# std::string_view

> A non-owning, read-only **view** of a character sequence — a `const CharT*` plus
> a length, with a string-like interface. **Header:** `<string_view>` · **Since:** C++17

> Like [`std::span`](../containers/views/span.md) but for text, and read-only.
> It owns nothing and allocates nothing.

## Mental model

```
 std::string owner = "hello world";
 std::string_view sv = owner;        // sv = {ptr → 'h', len = 11}

         h e l l o   w o r l d
 owner ▶ ─────────────────────  (owns the bytes)
 sv    ▶ └──────── 11 ────────┘  (just points + length)
```

`substr`, copying, and slicing a `string_view` only adjust the pointer/length —
**no characters are copied**. The viewed data must outlive the view.

## Declaration

```cpp
template<class CharT, class Traits = std::char_traits<CharT>>
class basic_string_view;
```

Aliases: `std::string_view` (`char`), `std::wstring_view`, `std::u8string_view`
(C++20), `std::u16string_view`, `std::u32string_view`.

## Why it exists

The universal "read some text" parameter type. It accepts `std::string`, string
literals, `char*` buffers, and other views — without forcing a copy or an
allocation, and without templating the function on the string type.

```cpp
// One signature, zero copies, works for everything:
std::size_t count_spaces(std::string_view s) {
    return std::ranges::count(s, ' ');
}
count_spaces("a b c");                 // const char* → view, no allocation
count_spaces(std::string{"x y"});      // std::string → view, no copy
```

## Examples

### Cheap slicing

```cpp
#include <string_view>

std::string_view sv = "key=value";
auto eq = sv.find('=');
std::string_view key = sv.substr(0, eq);   // "key"  — no allocation
std::string_view val = sv.substr(eq + 1);  // "value"— no allocation
sv.remove_prefix(4);                        // now "value" (just moves the ptr)
```

### Same read API as string

```cpp
sv.size(); sv.empty(); sv[0]; sv.front(); sv.back();
sv.starts_with("val"); sv.ends_with("ue");        // C++20
sv.contains("alu");                                // C++23
for (char ch : sv) { /* ... */ }
```

## Gotchas

- **It does not own — dangling is the cardinal sin.** Never return a
  `string_view` of a local `std::string`, and beware:
  ```cpp
  std::string_view bad = std::string{"temp"};   // dangles immediately:
                                                 // the temporary dies at the ;
  ```
- **Not guaranteed null-terminated.** A `string_view` is a (ptr, len) pair, so
  there's **no `c_str()`**. Don't pass `sv.data()` to a C API expecting a
  terminated string — construct a `std::string` first.
- **`remove_prefix`/`remove_suffix` shrink the view, not the data.** They just
  move the bounds; cheap and non-owning.
- **Prefer `string_view` for parameters, `string` for storage.** A class member
  that's a `string_view` is a lifetime bug waiting to happen unless you control
  the backing buffer.

## See also

- [std::basic_string_view — cppreference](https://en.cppreference.com/w/cpp/string/basic_string_view)
- [std::span — cppreference](https://en.cppreference.com/w/cpp/container/span)
