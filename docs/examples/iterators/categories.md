# Iterator categories & concepts

> Iterators come in a hierarchy of capabilities; an algorithm states the weakest
> category it needs, and any stronger iterator works. **Header:** `<iterator>`

## The hierarchy

Each category supports everything the weaker ones do, plus more:

| Category | Can do | Examples |
|---|---|---|
| **Input** | read once, single-pass, `++` | `istream_iterator`, many ranges views |
| **Output** | write once, single-pass, `++` | `back_insert_iterator`, `ostream_iterator` |
| **Forward** | read/write, multi-pass (revisit) | `forward_list`, `unordered_*` |
| **Bidirectional** | also `--` | `list`, `set`, `map` |
| **Random access** | also `+ n`, `it[n]`, `it1 - it2`, `<` | `deque` |
| **Contiguous** (C++17) | elements are truly adjacent in memory (`&*it`) | `array`, `vector`, `string`, `span` |

```
 input   ─┐
 output  ─┤ (single-pass)
 forward ─→ bidirectional ─→ random access ─→ contiguous   (increasing power)
```

Why it matters: `std::sort` needs **random access** (won't compile on `list` —
use `list::sort`); `std::find` only needs **input**; passing `vector` data to a C
API that wants a pointer needs **contiguous**.

## C++20 iterator concepts

C++20 restated the categories as named **concepts** (in `<iterator>`), used to
constrain templates and the ranges library:

```cpp
std::input_iterator
std::output_iterator<T>          // takes the written-value type
std::forward_iterator
std::bidirectional_iterator
std::random_access_iterator
std::contiguous_iterator
```

Plus supporting concepts: `std::sentinel_for<S, I>` (an end marker comparable to
iterator `I`), `std::sized_sentinel_for`, `std::weakly_incrementable`,
`std::indirectly_readable`/`writable`, and `std::incrementable`.

```cpp
template<std::random_access_iterator It>
void needs_jump(It first, It last);          // constrained: clear error if misused
```

## The legacy tag types

Pre-concepts code tags categories via `iterator_traits<It>::iterator_category`:

```cpp
std::input_iterator_tag, std::output_iterator_tag,
std::forward_iterator_tag, std::bidirectional_iterator_tag,
std::random_access_iterator_tag, std::contiguous_iterator_tag  // C++20
```

These drive *tag dispatch* — e.g. `std::advance` jumps in O(1) for random-access
iterators and steps in O(n) otherwise, chosen by the tag.

## Gotchas

- **Picking the wrong category = a wall of template errors** (pre-C++20) or a
  clean concept failure (C++20). The fix is matching the algorithm's requirement,
  not the container you happen to have.
- **`std::sort` vs `list::sort`:** `std::sort` requires random access;
  `std::list`/`forward_list` provide only bidirectional/forward, so they ship
  their own member `sort`.
- **Contiguous ⊃ random access**, but a random-access iterator isn't necessarily
  contiguous (`deque`'s isn't) — only contiguous guarantees `&*(it+n) == &*it +
  n`.

## See also

- [Iterator library — cppreference](https://en.cppreference.com/w/cpp/iterator)
- [Iterator concepts (C++20) — cppreference](https://en.cppreference.com/w/cpp/iterator/input_iterator)
- [LegacyIterator named requirements — cppreference](https://en.cppreference.com/w/cpp/named_req/Iterator)
