# Iterators library

The generalization of pointers that lets algorithms work over any container.
Mirrors [cppreference's iterator library](https://en.cppreference.com/w/cpp/iterator).

| Page | Header | Since | What |
|---|---|---|---|
| [Categories & concepts](categories.md) | `<iterator>` | C++98 / C++20 | input → contiguous; the C++20 iterator concepts |
| [Iterator adaptors](adaptors.md) | `<iterator>` | C++98+ | insert / reverse / move / stream / counted iterators |
| [Operations & traits](operations.md) | `<iterator>` | C++98+ | `advance`, `next`, `distance`, `iterator_traits`, sentinels |

## Why iterators

An algorithm written against iterators (`std::sort(first, last)`) works on any
data structure that can produce the right iterator category — array, `vector`,
`deque`, a stream, a generator. Iterators are the glue between **containers** and
**algorithms**, and the foundation the [ranges](../ranges/README.md) library
builds on.
