# Metaprogramming library

Compile-time inspection and computation on types and values. Mirrors
[cppreference's metaprogramming library](https://en.cppreference.com/w/cpp/meta).

| Page | Header | Since | What |
|---|---|---|---|
| [Type traits](type-traits.md) | `<type_traits>` | C++11 | inspect & transform types at compile time |
| [`ratio`](ratio.md) | `<ratio>` | C++11 | exact compile-time rational arithmetic |
| [`integer_sequence`](integer-sequence.md) | `<utility>` | C++14 | compile-time index packs for unpacking |

> In C++20+, **concepts** (see [Concepts library](../concepts/README.md)) and
> `if constexpr` replace much of what once required SFINAE and `enable_if`. Traits
> are still the vocabulary underneath.
