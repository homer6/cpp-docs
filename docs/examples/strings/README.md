# Strings library

Owning and non-owning text types, plus the trait layer they're built on.
Mirrors [cppreference's strings library](https://en.cppreference.com/w/cpp/string).

| Page | Header | Since | What |
|---|---|---|---|
| [`std::string` / `basic_string`](string.md) | `<string>` | C++98 | Owning, growable character sequence |
| [`std::string_view`](string_view.md) | `<string_view>` | C++17 | Non-owning read-only view of characters |
| [`char_traits`](char_traits.md) | `<string>` | C++98 | The character-operations policy both build on |

> Formatting (`std::format`/`std::print`), `to_string`/`from_chars`, and regular
> expressions live in the [Text processing](../text-processing/README.md) area.

## Which one?

- **Own and mutate text** → `std::string`.
- **Just read/parse text you don't own** (function parameters especially) →
  `std::string_view` — no allocation, no copy.
- **Different character type or comparison rules** → a `basic_string<CharT,
  Traits>` instantiation; customize via `char_traits`.
